/*****************************************************************************
File Name   : stuart.h

Description : API Interfaces for the STUART driver.

Copyright (C) 2001 STMicroelectronics

Reference   :

ST API Definition "UART Driver API" DVD-API-22 version 1.14.
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STUART_H
#define __STUART_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* Standard definitions */

#include "stpio.h"                      /* STAPI dependencies */

#if defined (STUART_DMA_SUPPORT)
#include "stfdma.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */



/* Driver constants, required by the driver model */
#define STUART_DRIVER_ID        22
#define STUART_DRIVER_BASE      (STUART_DRIVER_ID << 16)

/* Error constants */
#define STUART_ERROR_OVERRUN    (STUART_DRIVER_BASE)
#define STUART_ERROR_PARITY     (STUART_DRIVER_BASE+1)
#define STUART_ERROR_FRAMING    (STUART_DRIVER_BASE+2)
#define STUART_ERROR_ABORT      (STUART_DRIVER_BASE+3)
#define STUART_ERROR_RETRIES    (STUART_DRIVER_BASE+4)
#define STUART_ERROR_PIO        (STUART_DRIVER_BASE+5)
    
/* Maximum timeout */
#define STUART_MAX_TIMEOUT      ((U32)-1)

/* Baud rates limits */
#define STUART_BAUDRATE_MAX     6250000
#define STUART_BAUDRATE_MIN     75

/* Option for statistics */
#ifndef STUART_STATISTICS
#define STUART_STATISTICS       0
#endif

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* STUART Handle */
typedef U32 STUART_Handle_t;

/* Parity */

typedef enum STUART_DataBits_e
{
    STUART_7BITS_ODD_PARITY,
    STUART_7BITS_EVEN_PARITY,
    STUART_8BITS_NO_PARITY,
    STUART_8BITS_ODD_PARITY,
    STUART_8BITS_EVEN_PARITY,
    STUART_8BITS_PLUS_WAKEUP,
    STUART_9BITS_NO_PARITY
} STUART_DataBits_t;

/* Stop bits */

typedef enum STUART_StopBits_e
{
    STUART_STOP_0_5,
    STUART_STOP_1_0,
    STUART_STOP_1_5,
    STUART_STOP_2_0
} STUART_StopBits_t;

/* Flow control */

typedef enum STUART_FlowControl_e
{
    STUART_NO_FLOW_CONTROL,
    STUART_SW_FLOW_CONTROL,
    STUART_RTSCTS_FLOW_CONTROL
} STUART_FlowControl_t;

typedef enum STUART_SWFlowControl_e
{
    STUART_SW_FLOW_NOTHING,
    STUART_SW_FLOW_START,
    STUART_SW_FLOW_STOP
} STUART_SWFlowControl_t;

/* Device type i.e., hardware FIFO */

typedef enum STUART_Device_e
{
    STUART_NO_HW_FIFO,
    STUART_16_BYTE_FIFO,
    STUART_RTSCTS,
    STUART_ISO7816,
    STUART_DMA,
    STUART_UNKNOWN_DEVICE
} STUART_Device_t;

/* Flow control direction */
typedef enum STUART_Direction_e
{
    STUART_DIRECTION_TRANSMIT = 1,
    STUART_DIRECTION_RECEIVE = 2,
    STUART_DIRECTION_BOTH = 3
} STUART_Direction_t;

/* Protocol definition used to transmit/receive data */
typedef struct STUART_Protocol_s
{
    STUART_DataBits_t       DataBits;   /* e.g., 7 bits, even parity */
    STUART_StopBits_t       StopBits;   /* 0,1,1.5,2, etc */
    STUART_FlowControl_t    FlowControl; /* XONXOFF/RTSCTS */
    U32                     BaudRate;   /* 1200 => 1200 baud, etc */
    ST_String_t             TermString; /* e.g., "\0\0", "\r\n\0", "\n\0" */
    U32                     TimeOut;    /* specified in 10ms intervals */
    void (*NotifyFunction)(STUART_Handle_t Handle, ST_ErrorCode_t ErrorCode);
} STUART_Protocol_t;

/* Transmit/receive parameters */
typedef struct STUART_Params_s
{
    BOOL                    SmartCardModeEnabled;
    U16                     GuardTime;
    U8                      Retries;
    STUART_Protocol_t       RxMode;
    STUART_Protocol_t       TxMode;
    BOOL                    NACKSignalDisabled;
    BOOL                    HWFifoDisabled;
} STUART_Params_t;

/* Open parameters */
typedef struct STUART_OpenParams_s
{
    BOOL                LoopBackEnabled;
    BOOL                FlushIOBuffers;
    STUART_Params_t     *DefaultParams;
} STUART_OpenParams_t;

/* STUART_Init parameters */
typedef struct STUART_InitParams_s
{
    STUART_Device_t     UARTType;       /* e.g., 16ByteFIFO */
    ST_Partition_t      *DriverPartition;
    ST_Partition_t      *NCachePartition; /* Non-cached partition */ 
    U32                 *BaseAddress; 
     /* UART register base address */
#ifdef ST_OS21
    interrupt_name_t    InterruptNumber;
#else
    U32                 InterruptNumber;    
#endif
    U32                 InterruptLevel;
    U32                 ClockFrequency;
    BOOL                SwFIFOEnable;   /* To enable receive s/w FIFO */
    U32                 FIFOLength;     /* Size of s/w FIFOs */
    STPIO_PIOBit_t      RXD;
    STPIO_PIOBit_t      TXD;
    STPIO_PIOBit_t      RTS;
    STPIO_PIOBit_t      CTS;
    STUART_Params_t     *DefaultParams;
#ifdef STUART_DMA_SUPPORT
    STFDMA_RequestSignal_t  RXDMARequestSignal;
    STFDMA_RequestSignal_t  TXDMARequestSignal;
#endif
} STUART_InitParams_t;

/* STUART_Term parameters */
typedef struct STUART_TermParams_s
{
    BOOL                ForceTerminate;
} STUART_TermParams_t;

#if STUART_STATISTICS
typedef struct STUART_Statistics_s
{
    U32                 NumberBytesReceived;
    U32                 NumberBytesTransmitted;
    U32                 NumberOverrunErrors;
    U32                 NumberFramingErrors;
    U32                 NumberParityErrors;
}STUART_Statistics_t;
#endif /* STUART_STATISTICS */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t STUART_EnableStuffing (STUART_Handle_t Handle,U8 StuffByte);
ST_ErrorCode_t STUART_DisableStuffing(STUART_Handle_t Handle);

ST_ErrorCode_t STUART_Abort(STUART_Handle_t Handle, STUART_Direction_t Direction);
ST_ErrorCode_t STUART_Close(STUART_Handle_t Handle);
ST_ErrorCode_t STUART_Flush(STUART_Handle_t Handle);
ST_ErrorCode_t STUART_GetActualBaudRate(STUART_Handle_t Handle,
                                        U32 *ActualBaudRate
                                       );
ST_ErrorCode_t STUART_GetParams(STUART_Handle_t Handle,
                                STUART_Params_t *Params
                               );
#if STUART_STATISTICS
ST_ErrorCode_t STUART_GetStatistics(STUART_Handle_t Handle,
                                    STUART_Statistics_t *Stats_p);
#endif /* STUART_STATISTICS */
ST_Revision_t STUART_GetRevision(void);
ST_ErrorCode_t STUART_Init(const ST_DeviceName_t DeviceName,
                           const STUART_InitParams_t *InitParams
                          );
ST_ErrorCode_t STUART_Open(const ST_DeviceName_t DeviceName,
                           STUART_OpenParams_t *OpenParameters,
                           STUART_Handle_t *Handle
                          );
ST_ErrorCode_t STUART_Pause(STUART_Handle_t Handle,
                            STUART_Direction_t Direction);

/* STUART_Read maps to STUART_PosixRead with MinToRead = MaxToRead */
#define STUART_Read(Handle, Buffer, NumberToRead, NumberRead, Timeout) \
  STUART_PosixRead(Handle, Buffer, (NumberToRead), (NumberToRead), \
                   NumberRead, Timeout)
  
ST_ErrorCode_t STUART_PosixRead(STUART_Handle_t Handle,
                                U8 *Buffer,
                                U32 MinToRead,
                                U32 MaxToRead,
                                U32 *NumberRead,
                                U32 TimeOut
                                );

ST_ErrorCode_t STUART_Resume(STUART_Handle_t Handle,
                             STUART_Direction_t Direction);
ST_ErrorCode_t STUART_SetParams(STUART_Handle_t Handle,
                                STUART_Params_t *Params
                               );
ST_ErrorCode_t STUART_Term(const ST_DeviceName_t DeviceName,
                           const STUART_TermParams_t *TermParams
                          );
ST_ErrorCode_t STUART_Write(STUART_Handle_t Handle,
                            U8 *Buffer,
                            U32 NumberToWrite,
                            U32 *NumberWritten,
                            U32 Timeout
                           );

#ifdef __cplusplus
}
#endif

#endif /* __STUART_H */

/* End of stuart.h */

