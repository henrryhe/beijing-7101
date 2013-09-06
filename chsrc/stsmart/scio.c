/*****************************************************************************
File Name   : scio.c

Description : Smartcard I/O routines to UART.

Copyright (C) 2006 STMicroelectronics

History     :

    30/05/00    New IO routines created to encapsulate all communications
                with the STUART driver.  This simplifies the
                main driver code e.g., all echo'd bytes are read back,
                and the bit convention is respected at all times.

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if defined(ST_OS20) || defined(ST_OS21)
#include <string.h>                     /* C libs */
#include <stdio.h>
#endif

#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "stlite.h"
#endif
#include "sttbx.h"
#include "stsys.h"


#include "stcommon.h"

#include "stuart.h"
#include "stsmart.h"                    /* STAPI includes */

#include "scregs.h"                     /* Local includes */
#include "scio.h"
#include "scdat.h"

/* Private types/constants ------------------------------------------------ */

/* Private variables ------------------------------------------------------ */

/* Pointer to base element of SC control block list */
extern SMART_ControlBlock_t *SmartQueueHead_p;

/* Private macros --------------------------------------------------------- */

/* Macro for obtaining IO params from an IO handle */
#define IOPARAMS(x) ((SMART_IO_Params_t *)x)

/* Private function prototypes -------------------------------------------- */

static SMART_IO_Handle_t GetIoBlock(STUART_Handle_t);
static ST_ErrorCode_t UartToSmartError(ST_ErrorCode_t UartError);
static void UartReadHandler(STUART_Handle_t Handle,
                            ST_ErrorCode_t ErrorCode);
static void UartWriteHandler(STUART_Handle_t Handle,
                             ST_ErrorCode_t ErrorCode);
static void BufferInverseConvention(U8 *ptr, U32 sz);

/* Exported Functions ----------------------------------------------------- */

/*****************************************************************************
SMART_IO_Init()
Description:
    Initializes the half-duplex IO interface for the smartcard.
    This involves opening a handle to a UART device and configuring
    the default parameters for a smartcard interface.
Parameters:
    DeviceName          UART devicename to use.
    MemoryPartition     pointer to memory partition for allocating memory
    Handle_p            user pointer for allocated handle
Return Value:
    ST_NO_ERROR
    ST_ERROR_NO_MEMORY
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_Init(const ST_DeviceName_t DeviceName,
                             ST_Partition_t *MemoryPartition,
                             STSMART_Device_t DeviceType,
                             SMART_IO_Handle_t *Handle_p)
{
    STUART_OpenParams_t OpenParams;
    STUART_Params_t DefaultParams;
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Attempt to allocate IO block */
    *Handle_p = (SMART_IO_Handle_t)
                STOS_MemoryAllocate(MemoryPartition,
                                sizeof(SMART_IO_Params_t));

    if (*Handle_p != NULL)              /* Did allocate succeed? */
    {
        /* Clear structure to zero, for anything we don't explicitly set */
        memset(*Handle_p, 0, sizeof(SMART_IO_Params_t));

        /* Guard time settings -- initially zero but will be updated when an
         * ATR is performed.
         */
        DefaultParams.SmartCardModeEnabled = TRUE; /* Enables smartcard mode */
        DefaultParams.GuardTime = 0;
        DefaultParams.Retries = SMART_RETRIES_DEFAULT;
        DefaultParams.NACKSignalDisabled = FALSE;

        /* If it's an NDS smartcard, we need to disable the HW FIFO.
         * According to the spec for the block, that combined with smartcard
         * mode will put the ASC into NDS mode (ie it will do the required
         * flow control).
         */
        if (DeviceType == STSMART_ISO_NDS)
        {
            DefaultParams.HWFifoDisabled = TRUE;
        }
        else
        {
            DefaultParams.HWFifoDisabled = FALSE;
        }

        /* Setup the protocol modes for both TX and RX on the UART.  */

        /* RxMode settings */
        DefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        DefaultParams.RxMode.StopBits = STUART_STOP_1_5;
        DefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        DefaultParams.RxMode.BaudRate = SMART_INITIAL_BAUD;
        DefaultParams.RxMode.TermString = NULL;
        DefaultParams.RxMode.NotifyFunction = NULL;
        DefaultParams.RxMode.NotifyFunction = UartReadHandler;

        /* TxMode settings */
        DefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        DefaultParams.TxMode.StopBits = STUART_STOP_1_5;
        DefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
        DefaultParams.TxMode.BaudRate = SMART_INITIAL_BAUD;
        DefaultParams.TxMode.TermString = NULL;
        DefaultParams.TxMode.NotifyFunction = NULL;
        DefaultParams.TxMode.NotifyFunction = UartWriteHandler;

        /* Open settings */
        OpenParams.LoopBackEnabled = FALSE;
        OpenParams.FlushIOBuffers  = TRUE;
        OpenParams.DefaultParams   = &DefaultParams;

        /* Make sure that any parameters are set, at least the
         * first time.
         */
        IOPARAMS(*Handle_p)->RxParamsFlush = TRUE;
        IOPARAMS(*Handle_p)->TxParamsFlush = TRUE;

        /* Open handle to UART driver */
        error = STUART_Open(DeviceName,
                            &OpenParams,
                            &IOPARAMS(*Handle_p)->UARTHandle);

        /* Set new parameters, if open succeeded */
        if (error == ST_NO_ERROR)
        {
            /* Set internal IO block parameters */
            IOPARAMS(*Handle_p)->UARTParams = DefaultParams;
            IOPARAMS(*Handle_p)->MemoryPartition = MemoryPartition;
            IOPARAMS(*Handle_p)->InverseConvention = FALSE;
        }
    }
    else
    {
        /* Unable to allocate IO block */
        error = ST_ERROR_NO_MEMORY;
    }

    return error;
}

/*****************************************************************************
SMART_IO_Term()
Description:
    Ends usage of the half-duplex IO interface for the smartcard.
    This involves closing a handle to a UART device deallocating
    memory.
Parameters:
    Handle              user allocated IO block handle
Return Value:
    ST_NO_ERROR
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_Term(SMART_IO_Handle_t Handle)
{
    /* Close the UART handle */
    STUART_Close(IOPARAMS(Handle)->UARTHandle);

    /* De-allocate all memory associated with the IO block */
    STOS_MemoryDeallocate(IOPARAMS(Handle)->MemoryPartition,
                      Handle);

    return ST_NO_ERROR;
}

/*****************************************************************************
SMART_IO_Rx()
Description:
    Commences a read on the half-duplex interface.
    The current bit convention will be respected and all read bytes will
    be converted accordingly.
Parameters:
    Handle          handle to IO block
    Buf             buffer to store read data
    NumberToRead    Number of bytes to read into buffer
    NumberRead      Updated with number of bytes read
    Timeout         Characeter timeout (in ms) for transfer
    RxCallback      Callback function to invoke in completion of transfer
    Arg             User argument supplied to be passed to callback function
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_Rx(SMART_IO_Handle_t Handle,
                           U8 *Buf,
                           U32 NumberToRead,
                           U32 *NumberRead,
                           U32 Timeout,
                           void (*RxCallback)(ST_ErrorCode_t,
                                              void *),
                           void *Arg
                         )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    void (*UartNotifyFunction)(STUART_Handle_t, ST_ErrorCode_t);
    static U32 PreviousRate = 0;
    static U32 PreviousStop = 255;
    static U32 PreviousData = 255;

    /* Set notify function for UART driver */
    UartNotifyFunction = (RxCallback != NULL) ? UartReadHandler
                                              : NULL;

    /* Install UART notify function via SetParams() */
    IOPARAMS(Handle)->UARTParams.RxMode.NotifyFunction = UartNotifyFunction;

    /* Set this flag, so the notify function will do the right thing */
    IOPARAMS(Handle)->TransferIsRead = TRUE;

    /* Don't set the parameters if we don't need to */
    if ((IOPARAMS(Handle)->RxParamsFlush == TRUE) ||
        (PreviousStop != IOPARAMS(Handle)->UARTParams.RxMode.StopBits) ||
        (PreviousData != IOPARAMS(Handle)->UARTParams.RxMode.DataBits) ||
        (PreviousRate != IOPARAMS(Handle)->UARTParams.RxMode.BaudRate))
    {
#if defined(STSMART_DEBUG)
        /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     "SMART_IO_Rx(): calling STUART_SetParams()\n"));*/
#endif
        Error = STUART_SetParams(IOPARAMS(Handle)->UARTHandle,
                                 &IOPARAMS(Handle)->UARTParams);
#if defined(STSMART_DEBUG)
        if (Error != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                         "!! Received error 0x%08x!\n", (U32)Error));
        }
#endif

        IOPARAMS(Handle)->RxParamsFlush = FALSE;
        PreviousStop = IOPARAMS(Handle)->UARTParams.RxMode.StopBits;
        PreviousData = IOPARAMS(Handle)->UARTParams.RxMode.DataBits;
        PreviousRate = IOPARAMS(Handle)->UARTParams.RxMode.BaudRate;
    }

    /* If this is a callback read, then we need to store away the
     * caller's buffer and number read pointer -- this allows us to
     * access them once the callback is invoked.
     */
    if (RxCallback != NULL)
    {
        IOPARAMS(Handle)->Buf = Buf;    /* Buffer */
        IOPARAMS(Handle)->ActualSize = NumberRead; /* Actual number read */
        IOPARAMS(Handle)->Callback = RxCallback;
        IOPARAMS(Handle)->CallbackArg = Arg;
    }

    /* Commence reading data */
    Error = STUART_Read(IOPARAMS(Handle)->UARTHandle,
                        Buf,
                        NumberToRead,
                        NumberRead,
                        Timeout);
    /* If the user chose not to callback, then the read was blocking
     * and hence the data should now be available.
     * We can now check whether or not to invert the bit-convention.
     */
    if (RxCallback == NULL && ((IOPARAMS(Handle)->InverseConvention)))
    {
        /* User has specified inverse bit-convention */
        BufferInverseConvention(Buf, *NumberRead);
    }
    /* Convert to native errorcode */
    return UartToSmartError(Error);
}

/*****************************************************************************
SMART_IO_Tx()
Description:
    Commences a write on the half-duplex interface.
    The current bit convention will be respected and all written bytes will
    be 'read back' to ensure they were transmitted correctly.
Parameters:
    Handle          handle to IO block
    Buf             buffer containing data to transmit
    NumberToWrite   Number of bytes contained in buffer
    NumberWritten   Updated with number of bytes written
    Timeout         Characeter timeout (in ms) for transfer
    TxCallback      Callback function to invoke in completion of transfer
    Arg             User argument supplied to be passed to callback function
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_Tx(SMART_IO_Handle_t Handle,
                           U8 *Buf,
                           U32 NumberToWrite,
                           U32 *NumberWritten,
                           U32 Timeout,
                           void (*TxCallback)(ST_ErrorCode_t,
                                              void *),
                           void *Arg
                         )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    static U32 PreviousRate = 0;
    static U32 PreviousStop = 255;
    static U32 PreviousData = 255;
    static U32 PreviousNack = 255;
    static U32 PreviousGuard = 0;
    void (*UartNotifyFunction)(STUART_Handle_t, ST_ErrorCode_t);

    /* Set notify function for UART driver */
    UartNotifyFunction = (TxCallback != NULL) ? UartWriteHandler : NULL;

    /* Install notification routine */
    IOPARAMS(Handle)->UARTParams.TxMode.NotifyFunction = UartNotifyFunction;

    /* Set this flag, so the read notify function will do the right
     * thing when we consume back the data we transmitted.
     */
    IOPARAMS(Handle)->TransferIsRead = FALSE;

    /* Don't set the parameters if we don't need to */
    if ((IOPARAMS(Handle)->TxParamsFlush == TRUE) ||
        (PreviousStop  != IOPARAMS(Handle)->UARTParams.TxMode.StopBits) ||
        (PreviousData  != IOPARAMS(Handle)->UARTParams.TxMode.DataBits) ||
        (PreviousNack  != IOPARAMS(Handle)->UARTParams.NACKSignalDisabled) ||
        (PreviousRate  != IOPARAMS(Handle)->UARTParams.TxMode.BaudRate) ||
        (PreviousGuard != IOPARAMS(Handle)->UARTParams.GuardTime))
    {
#if defined(STSMART_DEBUG)
        /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     "SMART_IO_Tx(): calling STUART_SetParams()\n"));*/
#endif
        Error = STUART_SetParams(IOPARAMS(Handle)->UARTHandle,
                                 &IOPARAMS(Handle)->UARTParams);
#if defined(STSMART_DEBUG)
        if (Error != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                         "!! Received error 0x%08x!\n", (U32)Error));
        }
#endif

        IOPARAMS(Handle)->TxParamsFlush = FALSE;
        PreviousStop  = IOPARAMS(Handle)->UARTParams.TxMode.StopBits;
        PreviousData  = IOPARAMS(Handle)->UARTParams.TxMode.DataBits;
        PreviousRate  = IOPARAMS(Handle)->UARTParams.TxMode.BaudRate;
        PreviousNack  = IOPARAMS(Handle)->UARTParams.NACKSignalDisabled;
        PreviousGuard = IOPARAMS(Handle)->UARTParams.GuardTime;
    }


    /* If this is a callback write, then we need to store away the
     * caller's buffer and number read pointer -- this allows us to
     * access them once the callback is invoked.
     */
    if (TxCallback != NULL)
    {
        IOPARAMS(Handle)->Buf = Buf;    /* Buffer */
        IOPARAMS(Handle)->ActualSize = NumberWritten; /* Actual number written */
        IOPARAMS(Handle)->Callback = TxCallback;
        IOPARAMS(Handle)->CallbackArg = Arg;
    }

    /* Check whether or not to invert the bit-convention.
     */
    if (IOPARAMS(Handle)->InverseConvention)
    {
        U8 WorkBuf[256];

        /* Copy to local buffer (never modify user buffer) */
        memcpy(WorkBuf, Buf, NumberToWrite);

        /* Do convention inversion */
        BufferInverseConvention(WorkBuf,
                                NumberToWrite);

        /* Commence writing data */
        Error = STUART_Write(IOPARAMS(Handle)->UARTHandle,
                             WorkBuf,
                             NumberToWrite,
                             NumberWritten,
                             Timeout);

    }
    else
    {
        /* Commence writing data */
        Error = STUART_Write(IOPARAMS(Handle)->UARTHandle,
                             Buf,
                             NumberToWrite,
                             NumberWritten,
                             Timeout);

    }
    if (TxCallback == NULL && Error == ST_NO_ERROR)
    {
        U8 WorkBuf[256];
        U32 i;

        /* Consume transmitted bytes to ensure they were sent
         * correctly.
         */
        Error = STUART_Read(IOPARAMS(Handle)->UARTHandle,
                            WorkBuf,
                            *NumberWritten,
                            &i,
                            Timeout);
#if defined(STSMART_DEBUG)
        if (Error != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                  "!! Received error in verification 0x%08x!\n", (U32)Error));
        }
#endif
        Error = ST_NO_ERROR; /* ignore this error :RC */

    }

    /* Convert to native errorcode */
    return UartToSmartError(Error);
}

/*****************************************************************************
SMART_IO_Abort()
Description:
    Aborts any pending IO operation to the smartcard UART.
Parameters:
    Handle          handle to IO block
Return Value:
    ST_NO_ERROR
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_Abort(SMART_IO_Handle_t Handle)
{
    /* Abort transfers in both directions */
    return STUART_Abort(IOPARAMS(Handle)->UARTHandle,
                        STUART_DIRECTION_BOTH);
}

/*****************************************************************************
SMART_IO_Flush()
Description:
    Flushes the internal FIFOs of the UART.
Parameters:
    Handle          handle to IO block
Return Value:
    ST_NO_ERROR             No error
    ST_ERROR_DEVICE_BUSY    Transfer is in progress, can't flush.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_Flush(SMART_IO_Handle_t Handle)
{
    /* Flush UART's receive FIFO and internal buffers */
    return STUART_Flush(IOPARAMS(Handle)->UARTHandle);
}

/*****************************************************************************
SMART_IO_NackDisable()
Description:
    Sets UART parameter to disable NACK signal
Parameters:
    Handle          handle to IO block
Return Value:
    ST_NO_ERROR             No error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_NackDisable(SMART_IO_Handle_t Handle)
{
    /* Update this value in the UART params stored; these automatically
     * get written to STUART whenever a read or write takes place.
     * This parameter is only written to hardware if user initialised
     * UART as ISO7816.
     */
    IOPARAMS(Handle)->UARTParams.NACKSignalDisabled = TRUE;
    return ST_NO_ERROR;
}

/*****************************************************************************
SMART_IO_NackEnable()
Description:
    Sets UART parameter to enable NACK signal
Parameters:
    Handle          handle to IO block
Return Value:
    ST_NO_ERROR             No error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_NackEnable(SMART_IO_Handle_t Handle)
{
    /* Update this value in the UART params stored; these automatically
     * get written to STUART whenever a read or write takes place.
     * This parameter is only written to hardware if user initialised
     * UART as ISO7816.
     */
    IOPARAMS(Handle)->UARTParams.NACKSignalDisabled = FALSE;
    return ST_NO_ERROR;
}

/*****************************************************************************
SMART_IO_SetBitCoding()
Description:
    Sets the bit convention to be used for all transfers on this
    half-duplex interface.
Parameters:
    Handle          handle to IO block
    BitCon          Specifies the required bit convention
Return Value:
    ST_NO_ERROR     No error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_SetBitCoding(SMART_IO_Handle_t Handle,
                                     SMART_IO_BitConvention_t BitCon)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Set bit coding scheme */
    IOPARAMS(Handle)->InverseConvention = BitCon == DIRECT ? FALSE : TRUE;

    if (IOPARAMS(Handle)->InverseConvention)
    {
#if defined(ST_OSLINUX)
        printk("scio.c=>SMART_IO_SetBitCoding(),STUART_8BITS_ODD_PARITY\n");
#endif
        IOPARAMS(Handle)->UARTParams.RxMode.DataBits = STUART_8BITS_ODD_PARITY;
        IOPARAMS(Handle)->UARTParams.TxMode.DataBits = STUART_8BITS_ODD_PARITY;
    }
    else
    {
#if defined(ST_OSLINUX)
        printk("scio.c=>SMART_IO_SetBitCoding(),STUART_8BITS_EVEN_PARITY\n");
#endif
        IOPARAMS(Handle)->UARTParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
        IOPARAMS(Handle)->UARTParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    }

    /* Invoke new parameters */
    Error = STUART_SetParams(IOPARAMS(Handle)->UARTHandle,
                             &IOPARAMS(Handle)->UARTParams);

    return Error;
}

/*****************************************************************************
SMART_IO_SetBitRate()
Description:
    Sets the transmit and receive bitrate to use for all subsequent
    transfers.
Parameters:
    Handle          handle to IO block
    BitRate         New bitrate to use
Return Value:
    ST_NO_ERROR     No error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_SetBitRate(SMART_IO_Handle_t Handle,
                                   U32 BitRate)
{
    /* Set new bitrate */
    IOPARAMS(Handle)->UARTParams.TxMode.BaudRate = BitRate;
    IOPARAMS(Handle)->UARTParams.RxMode.BaudRate = BitRate;

    /* Invoke new parameters */
    return STUART_SetParams(IOPARAMS(Handle)->UARTHandle,
                            &IOPARAMS(Handle)->UARTParams);
}

/*****************************************************************************
SMART_IO_SetBitCoding()
Description:
    Sets the transmit retries count.  This specifies the number of times
    the UART driver will re-transmit a character after being NAKed by
    the receiving smartcard.
Parameters:
    Handle          handle to IO block
    Retries         new retries count
Return Value:
    ST_NO_ERROR     No error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_SetRetries(SMART_IO_Handle_t Handle,
                                   U8 Retries)
{
    IOPARAMS(Handle)->UARTParams.Retries = Retries;
    return STUART_SetParams(IOPARAMS(Handle)->UARTHandle,
                            &IOPARAMS(Handle)->UARTParams);
}

/*****************************************************************************
SMART_IO_SetGuardTime()
Description:
    Sets the guard time interval (in etus) from the leading startbit
    of one character to the startbit of the next.
Parameters:
    Handle          handle to IO block
    N               extra guard time interval.
Return Value:
    ST_NO_ERROR     No error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_SetGuardTime(SMART_IO_Handle_t Handle,
                                     U16 N)
{
    IOPARAMS(Handle)->UARTParams.GuardTime = N;
    /* 11etu 'total' time requires use of 0.5 stop bits */
    if (N == 1)
        IOPARAMS(Handle)->UARTParams.RxMode.StopBits = STUART_STOP_0_5;
    else
        IOPARAMS(Handle)->UARTParams.RxMode.StopBits = STUART_STOP_1_5;

    return STUART_SetParams(IOPARAMS(Handle)->UARTHandle,
                            &IOPARAMS(Handle)->UARTParams);
}

/*****************************************************************************
SMART_IO_SetNAK()
Description:
    Enables or disables NAK detection.
Parameters:
    Handle          IO handle
    Enable          boolean for enabling/disabling NAK detection.
Return Value:
    ST_NO_ERROR     No error
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t SMART_IO_SetNAK(SMART_IO_Handle_t Handle,
                               BOOL Enable)
{
    /* NAK detection may only be used if 0_5 stop bits are in use. */
    if (Enable)
        IOPARAMS(Handle)->UARTParams.RxMode.StopBits = STUART_STOP_0_5;
    else
        IOPARAMS(Handle)->UARTParams.RxMode.StopBits = STUART_STOP_1_5;

    return STUART_SetParams(IOPARAMS(Handle)->UARTHandle,
                            &IOPARAMS(Handle)->UARTParams);
}

/* Private Functions ------------------------------------------------------ */

/*****************************************************************************
GetIoBlock()
Description:
    Runs through the queue of initialized smartcard devices and checks for
    the smartcard with the specified UART handle.
Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a IO handle.
See Also:
    Nothing.
*****************************************************************************/

static SMART_IO_Handle_t GetIoBlock(STUART_Handle_t Handle)
{
    SMART_ControlBlock_t *qp = SmartQueueHead_p; /* Global queue head pointer */
    U32 Index = 0;

    /* Iterate over the smartcard queue */
    while (qp != NULL)
    {
        /* Check for a valid matching handle */
        if (Handle == IOPARAMS(qp->IoHandle)->UARTHandle)
            return qp->IoHandle;

        /* Try the next smartcard */
        qp = qp->Next_p;
        Index++;
    }

    /* Return the smartcard control block (or NULL) to the caller */
    return NULL;
} /* GetIoBlock() */

/*****************************************************************************
UartReadHandler()
Description:
    This callback is invoked whenever a UART read operation completes.
Parameters:
    Handle,             handle for UART device.
    ErrorCode,          reason for callback.
Return Value:
    None.
See Also:
    UartWriteHandler()
*****************************************************************************/

static void UartReadHandler(STUART_Handle_t Handle,
                            ST_ErrorCode_t ErrorCode)
{
    SMART_IO_Handle_t IoHandle;

    IoHandle = GetIoBlock(Handle);
    if ((IoHandle != NULL) && (IOPARAMS(IoHandle)->TransferIsRead == TRUE))
    {
        /* Convert to native errorcode */
        ErrorCode = UartToSmartError(ErrorCode);

        /* Check for inverse bit convention */
        if (IOPARAMS(IoHandle)->InverseConvention)
        {
            BufferInverseConvention(IOPARAMS(IoHandle)->Buf,
                                    *(IOPARAMS(IoHandle)->ActualSize));
        }

        /* Call smartcard callback function */
        IOPARAMS(IoHandle)->Callback(ErrorCode,
                                     IOPARAMS(IoHandle)->CallbackArg);
    }
} /* UartReadHandler() */

/*****************************************************************************
UartWriteHandler()
Description:
    This callback is invoked whenever a UART write operation completes.
Parameters:
    Handle,             handle for UART device.
    ErrorCode,          reason for callback.
Return Value:
    None.
See Also:
    UartReadHandler()
*****************************************************************************/

static void UartWriteHandler(STUART_Handle_t Handle,
                             ST_ErrorCode_t ErrorCode)
{
    U32 i;
    U8 WorkBuf[256];
    SMART_IO_Handle_t IoHandle;

    IoHandle = GetIoBlock(Handle);
    if (IoHandle != NULL && ErrorCode == ST_NO_ERROR)
    {
        /* Consume transmitted bytes to ensure they were sent
         * correctly.
         */
        ErrorCode = STUART_Read(IOPARAMS(IoHandle)->UARTHandle,
                                WorkBuf,
                                *IOPARAMS(IoHandle)->ActualSize,
                                &i,
                                1000);
    }

    /* Convert to native errorcode */
    ErrorCode = UartToSmartError(ErrorCode);

    /* Call smartcard callback function */
    IOPARAMS(IoHandle)->Callback(ErrorCode,
                                 IOPARAMS(IoHandle)->CallbackArg);
} /* UartWriteHandler() */

static ST_ErrorCode_t UartToSmartError(ST_ErrorCode_t UartError)
{
    ST_ErrorCode_t ErrorCode;

    switch (UartError)
    {
        case STUART_ERROR_OVERRUN:
            ErrorCode = STSMART_ERROR_OVERRUN;
            break;
        case STUART_ERROR_PARITY:
            ErrorCode = STSMART_ERROR_PARITY;
            break;
        case STUART_ERROR_FRAMING:
            ErrorCode = STSMART_ERROR_FRAME;
            break;
        case STUART_ERROR_ABORT:
            ErrorCode = STSMART_ERROR_ABORTED;
            break;
        case STUART_ERROR_RETRIES:
            ErrorCode = STSMART_ERROR_TOO_MANY_RETRIES;
            break;
        default:
            ErrorCode = UartError;
            break;
    }

    return ErrorCode;
}

/*****************************************************************************
BufferInverseConvention()
Description:
    This routine is responsible for converting all bytes in a buffer to
    the ISO7816-3 inverse convention byte encoding.
Parameters:
    ptr,        pointer to first byte of buffer
    sz,         length of buffer (in bytes)
Return Value:
    None.
See Also:
    Nothing.
*****************************************************************************/

static void BufferInverseConvention(U8 *ptr, U32 sz)
{
    /* Byte encodings for inverse convention e.g,
     * 0 => 0xff, 1 => 0x7f, 2 => 0xbf, ... ,
     * 0xfe => 0x80, 0xff => 0x00
     */
    static const U8 InverseConventionLUT[] =
    {
        0xff, 0x7f, 0xbf, 0x3f, 0xdf, 0x5f, 0x9f, 0x1f, 0xef, 0x6f, 0xaf,
        0x2f, 0xcf, 0x4f, 0x8f, 0x0f, 0xf7, 0x77, 0xb7, 0x37, 0xd7, 0x57,
        0x97, 0x17, 0xe7, 0x67, 0xa7, 0x27, 0xc7, 0x47, 0x87, 0x07, 0xfb,
        0x7b, 0xbb, 0x3b, 0xdb, 0x5b, 0x9b, 0x1b, 0xeb, 0x6b, 0xab, 0x2b,
        0xcb, 0x4b, 0x8b, 0x0b, 0xf3, 0x73, 0xb3, 0x33, 0xd3, 0x53, 0x93,
        0x13, 0xe3, 0x63, 0xa3, 0x23, 0xc3, 0x43, 0x83, 0x03, 0xfd, 0x7d,
        0xbd, 0x3d, 0xdd, 0x5d, 0x9d, 0x1d, 0xed, 0x6d, 0xad, 0x2d, 0xcd,
        0x4d, 0x8d, 0x0d, 0xf5, 0x75, 0xb5, 0x35, 0xd5, 0x55, 0x95, 0x15,
        0xe5, 0x65, 0xa5, 0x25, 0xc5, 0x45, 0x85, 0x05, 0xf9, 0x79, 0xb9,
        0x39, 0xd9, 0x59, 0x99, 0x19, 0xe9, 0x69, 0xa9, 0x29, 0xc9, 0x49,
        0x89, 0x09, 0xf1, 0x71, 0xb1, 0x31, 0xd1, 0x51, 0x91, 0x11, 0xe1,
        0x61, 0xa1, 0x21, 0xc1, 0x41, 0x81, 0x01, 0xfe, 0x7e, 0xbe, 0x3e,
        0xde, 0x5e, 0x9e, 0x1e, 0xee, 0x6e, 0xae, 0x2e, 0xce, 0x4e, 0x8e,
        0x0e, 0xf6, 0x76, 0xb6, 0x36, 0xd6, 0x56, 0x96, 0x16, 0xe6, 0x66,
        0xa6, 0x26, 0xc6, 0x46, 0x86, 0x06, 0xfa, 0x7a, 0xba, 0x3a, 0xda,
        0x5a, 0x9a, 0x1a, 0xea, 0x6a, 0xaa, 0x2a, 0xca, 0x4a, 0x8a, 0x0a,
        0xf2, 0x72, 0xb2, 0x32, 0xd2, 0x52, 0x92, 0x12, 0xe2, 0x62, 0xa2,
        0x22, 0xc2, 0x42, 0x82, 0x02, 0xfc, 0x7c, 0xbc, 0x3c, 0xdc, 0x5c,
        0x9c, 0x1c, 0xec, 0x6c, 0xac, 0x2c, 0xcc, 0x4c, 0x8c, 0x0c, 0xf4,
        0x74, 0xb4, 0x34, 0xd4, 0x54, 0x94, 0x14, 0xe4, 0x64, 0xa4, 0x24,
        0xc4, 0x44, 0x84, 0x04, 0xf8, 0x78, 0xb8, 0x38, 0xd8, 0x58, 0x98,
        0x18, 0xe8, 0x68, 0xa8, 0x28, 0xc8, 0x48, 0x88, 0x08, 0xf0, 0x70,
        0xb0, 0x30, 0xd0, 0x50, 0x90, 0x10, 0xe0, 0x60, 0xa0, 0x20, 0xc0,
        0x40, 0x80, 0x00
    };
    U32 i;

    for (i = 0; i < sz; i++)
    {
        *ptr = InverseConventionLUT[*ptr];
        ptr++;

    }
}

/* End of scio.c */
