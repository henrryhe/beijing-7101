/*****************************************************************************
File Name   : uartdat.h

Description : Private datastructures for UART driver.

Copyright (C) 1999 STMicroelectronics

History     :

    01/12/99    Module created.
                Added 'ActualBaudRate' storage to control block.

    30/08/00    Added 'LastProtocol' storage to control block.

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __UARTDAT_H
#define __UARTDAT_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* STAPI includes */
#include "stpio.h"                      /* STAPI dependencies */
#include "stuart.h"

#include "ascregs.h"                    /* Local includes */

/* Exported Constants ----------------------------------------------------- */

/* Interrupt enable/disable definitions */
#ifdef ST_OSLINUX
#define STUART_MAPPING_WIDTH 4096
#endif

#define UART_IE_ENABLE_RX_FIFO       (IE_RXHALFFULL|IE_TIMEOUTNOTEMPTY)
#define UART_IE_ENABLE_RX_NO_FIFO    (IE_RXBUFFULL|IE_TIMEOUTNOTEMPTY)
#define UART_IE_ENABLE_TX_FIFO       (IE_TXEMPTY|IE_TXHALFEMPTY)
#define UART_IE_ENABLE_TX_NO_FIFO    (IE_TXEMPTY|IE_TXHALFEMPTY)
#define UART_IE_ENABLE_TX_END        (IE_TXEMPTY)
#define UART_IE_ENABLE_ERRORS        (IE_OVERRUNERROR|IE_FRAMEERROR|IE_PARITYERROR)

/* NOTE: We can only enable IE_TXEMPTY during a transmit, otherwise we run
 * the risk of continuously getting TXEMPTY interrupts.
 */

/* This errorcode is used internally to mark an operation as pending */
#define STUART_ERROR_PENDING    ((U32)-1)

/* XON/XOFF characters */
#define STUART_XON              17
#define STUART_XOFF             19

/* Exported Types --------------------------------------------------------- */

/*****************************************************************************
UART_Operation_t:
A convenient placeholder used to represent the current read or write operation,
although the 'TermString' member only applies to a read operation.
Whenever a new read/write operation is started, a UART_Operation_t is filled
out for keeping track of the progress of the operation.
The structure will typically be updated by:
a) the ISR (e.g., when data arrives or is sent),
b) a timer callback (e.g, when a timeout occurs).
*****************************************************************************/

typedef struct UART_Operation_s
{
    ST_ErrorCode_t      Status;         /* Error code for callback */
    Timer_t             *Timer;         /* Timer for timeouts */
    STOS_Clock_t             Timeout;        /* Timeout in clock ticks */
    U8                  *UserBuffer;    /* Read/write buffer pointer */
    U32                 *NumberBytes;   /* Bytes read/written */
    U32                 NumberRemaining;/* Bytes remaining */
    U32                 LastTransferSize;
    U32                 *ErrorSet_p;
    void (*NotifyRoutine)(STUART_Handle_t Handle,
                          ST_ErrorCode_t ErrorCode); /* Callback */
} UART_Operation_t;

/*****************************************************************************
UART_Status_t:
The following definitions represent the states the UART driver may be in at
any one time.  They represent a bitmask of values that enable us to keep
track of what activities are ongoing in the driver at any given moment.
*****************************************************************************/

typedef enum
{
    UART_TERM =       0,                /* The UART is terminated (default) */
    UART_INIT =       1,                /* The UART is initialized */
    UART_OPEN =       (1<<1),           /* The UART is open */
    UART_PAUSE_RX =   (1<<2),           /* The UART is paused (rx) */
    UART_PAUSE_TX =   (1<<3),           /* The UART is paused (tx) */
    UART_SETPARAMS =  (1<<4),           /* The params are about to be set */
    UART_XOFF_TX =    (1<<5),           /* The UART is paused (tx, due to XOFF) */
    UART_XOFF_RX =    (1<<6)            /* The UART is paused (rx, due to XOFF) */

} UART_Status_t;

/*****************************************************************************
UART_ControlBlock_t:
The top-level datastructure that represents an instance of a UART.
*****************************************************************************/

struct UART_ControlBlock_s
{
    ST_DeviceName_t         DeviceName;
    STUART_Handle_t         Handle;
    U32                     OpenCount;    /* OpenCount per instance */
    BOOL                    IsHalfDuplex; /* use TXDHandle but not RXDHandle */
    STPIO_Handle_t          RXDHandle;
    STPIO_Handle_t          TXDHandle;
    STPIO_Handle_t          RTSHandle;
    STPIO_Handle_t          CTSHandle;
    STUART_InitParams_t     InitParams;
    STUART_Params_t         SetParams;
    STUART_Params_t         DefaultParams;
    STUART_Protocol_t       LastProtocol;
    U8                      *TermString_p;
    U8                      *ReceiveBufferBase;
    U8                      *ReceivePut_p;
    U8                      *ReceiveGet_p;
    U32                     ReceiveBufferCharsFree;
    U32                     HwFIFOSize;
    UART_Operation_t        *ReadOperation;
    UART_Operation_t        *WriteOperation;
    /* To keep APIs' mutually exclusive */
    semaphore_t             *ReadSemaphore_p;
    semaphore_t             *WriteSemaphore_p;

    Timer_t                 ReadTimer;
    Timer_t                 WriteTimer;
    UART_Status_t           UartStatus;
#if defined(ST_OSLINUX) /* && defined(LINUX_FULL_KERNEL_VERSION)*/
    /* Linux ioremap address */
    unsigned long          *MappingBaseAddress;
    unsigned long           MappingWidth;
#endif
    U32                     IntEnable;
    U32                     IntEnableRxFlags;
    U32                     IntEnableTxFlags;
    U32                     ActualBaudRate;
    U32                     XOFFThreshold;
    U32                     XONThreshold;
    U8                      Retries;
    ST_ErrorCode_t          ReceiveError;
    BOOL                    WriteAwaitingPioCompare;
    BOOL                    StuffingEnabled;
    U8                      StuffingValue;
#if STUART_STATISTICS
    STUART_Statistics_t     Statistics;
#endif /* STUART_STATISTICS */
    UART_Operation_t        OperationPool[2];
    struct UART_ControlBlock_s *Next_p;
};

typedef struct UART_ControlBlock_s UART_ControlBlock_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Convenient macros for checking whether 9-bits are in use - this applies
 * to 9-bit data and 8-bit data + wakeup - both cases collapse to the
 * requirement that 9-bits must be read/written from/to the FIFOs.  We
 * internally group both cases as simply "9-bit mode".
 */
#define UART_Is9BitMode(DataBits) \
    ((DataBits == STUART_9BITS_NO_PARITY || \
      DataBits == STUART_8BITS_PLUS_WAKEUP))

/* Macros for examining capabilities of the device type */
#define UART_IsHwFIFOSupported(DevType)     (DevType >= STUART_16_BYTE_FIFO)
#define UART_IsRTSCTSSupported(DevType)     (DevType >= STUART_RTSCTS)
#define UART_IsNACKSupported(DevType)       (DevType >= STUART_RTSCTS)
#define UART_IsBaudModeSupported(DevType)   (DevType >= STUART_RTSCTS)

/* Enabled if in smartcard mode without FIFOs */
#define UART_DoesNdsFlowControlApply(Uart_p) \
    (Uart_p->DefaultParams.SmartCardModeEnabled && \
    ((Uart_p->InitParams.UARTType == STUART_NO_HW_FIFO) || \
     Uart_p->DefaultParams.HWFifoDisabled))
     /* nb SmartCardModeEnabled implies TXD.PortName[0] != 0 */

/* Exported Functions ----------------------------------------------------- */

#endif /* __UARTDAT_H */
/* End of uartdat.h */
