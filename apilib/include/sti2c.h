/******************************************************************************

File name   : sti2c.h

Description : Interface definition for driver code for I2C (SSC) interface

Revision History:

    [...]
    26July04  Added FifoEnabled variable in InitParams to provide FIFO
              functionality
COPYRIGHT (C) STMicroelectronics 2007

******************************************************************************/

#ifndef __STI2C_H
#define __STI2C_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants ----------------------------------------------------------------*/
#define STI2C_GETREVISION              "STI2C-REL_2.4.1"
#define STI2C_DRIVER_ID                8
#define STI2C_DRIVER_BASE              (STI2C_DRIVER_ID << 16)

/* External return codes */
#define STI2C_ERROR_BASE               STI2C_DRIVER_BASE

#define STI2C_ERROR_LINE_STATE         (STI2C_ERROR_BASE + 1)
#define STI2C_ERROR_STATUS             (STI2C_ERROR_BASE + 2)
#define STI2C_ERROR_ADDRESS_ACK        (STI2C_ERROR_BASE + 3)
#define STI2C_ERROR_WRITE_ACK          (STI2C_ERROR_BASE + 4)
#define STI2C_ERROR_NO_DATA            (STI2C_ERROR_BASE + 5)
#define STI2C_ERROR_PIO                (STI2C_ERROR_BASE + 6)
#define STI2C_ERROR_BUS_IN_USE         (STI2C_ERROR_BASE + 7)
#define STI2C_ERROR_EVT_REGISTER       (STI2C_ERROR_BASE + 8)
#define STI2C_ERROR_NO_FREE_SSC        (STI2C_ERROR_BASE + 9)

#ifdef STI2C_DEBUG_BUS_STATE
#define STI2C_ERROR_SDA_LOW_SLAVE      (STI2C_ERROR_BASE + 10)
#define STI2C_ERROR_SCL_LOW_SLAVE      (STI2C_ERROR_BASE + 11)
#define STI2C_ERROR_SDA_LOW_MASTER     (STI2C_ERROR_BASE + 12)
#define STI2C_ERROR_SCL_LOW_MASTER     (STI2C_ERROR_BASE + 13)
#define STI2C_ERROR_FALSE_START        (STI2C_ERROR_BASE + 14)
#endif

/* Rate and timeout configuration parametes */

#define STI2C_RATE_NORMAL              100000
#define STI2C_RATE_FASTMODE            400000
#define STI2C_TIMEOUT_INFINITY         0x0000


/* Types --------------------------------------------------------------------*/

typedef U32 STI2C_Handle_t;

typedef enum STI2C_Address_e
{
    STI2C_ADDRESS_7_BITS,
    STI2C_ADDRESS_10_BITS
} STI2C_Address_t;

typedef enum STI2C_MasterSlave_e
{
    STI2C_MASTER,
    STI2C_SLAVE,
    STI2C_MASTER_OR_SLAVE
} STI2C_MasterSlave_t;

enum
{
   STI2C_TRANSMIT_NOTIFY_EVT  = STI2C_DRIVER_BASE,
   STI2C_RECEIVE_NOTIFY_EVT,
   STI2C_TRANSMIT_BYTE_EVT,
   STI2C_RECEIVE_BYTE_EVT,
   STI2C_TRANSMIT_COMPLETE_EVT,
   STI2C_RECEIVE_COMPLETE_EVT,
   STI2C_BUS_STUCK_EVT
};

typedef struct STI2C_InitParams_s
{
    U32                  *BaseAddress;
    STPIO_PIOBit_t       PIOforSDA;
    STPIO_PIOBit_t       PIOforSCL;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    interrupt_name_t     InterruptNumber;
#else
    U32                  InterruptNumber;
#endif
    U32                  InterruptLevel;
    STI2C_MasterSlave_t  MasterSlave;
    U32                  BaudRate;
    U32                  ClockFrequency;
    U32                  GlitchWidth;
    U32                  MaxHandles;
    ST_Partition_t       *DriverPartition;
    U32                  SlaveAddress;
    ST_DeviceName_t      EvtHandlerName;
    BOOL                 FifoEnabled;
} STI2C_InitParams_t;



typedef struct STI2C_OpenParams_s
{
    STI2C_Address_t  AddressType;
    U16              I2cAddress;
    U32              BaudRate;
    U32              BusAccessTimeOut;
} STI2C_OpenParams_t;


typedef struct STI2C_Params_s
{
    STI2C_Address_t  AddressType;
    U16              I2cAddress;
    U32              BaudRate;
    U32              BusAccessTimeOut;
} STI2C_Params_t;


typedef struct STI2C_TermParams_s
{
    BOOL   ForceTerminate;
} STI2C_TermParams_t;


/* Functions ----------------------------------------------------------------*/

ST_ErrorCode_t STI2C_Close ( STI2C_Handle_t          Handle);

ST_ErrorCode_t STI2C_GenerateClocks ( STI2C_Handle_t Handle,
                                      U32            *SclClocks);

ST_ErrorCode_t STI2C_GetParams( STI2C_Handle_t       Handle,
                                STI2C_Params_t       *Params );

ST_Revision_t  STI2C_GetRevision( void );

ST_ErrorCode_t STI2C_Init ( const ST_DeviceName_t    Name,
                            const STI2C_InitParams_t *InitParams);

ST_ErrorCode_t STI2C_Lock ( STI2C_Handle_t           Handle,
                            BOOL                     WaitForLock);

ST_ErrorCode_t STI2C_LockMultiple ( STI2C_Handle_t           *HandlePool_p,
                                    U32                      NumberOfHandles,
                                    BOOL                     WaitForLock);

ST_ErrorCode_t STI2C_Open ( const ST_DeviceName_t    Name,
                            const STI2C_OpenParams_t *OpenParams,
                            STI2C_Handle_t           *Handle);


ST_ErrorCode_t STI2C_Read ( STI2C_Handle_t           Handle,
                            U8                       *Buffer,
                            U32                      MaxLen,
                            U32                      Timeout,
                            U32                      *ActLen);

ST_ErrorCode_t STI2C_ReadNoStop ( STI2C_Handle_t     Handle,
                                  U8                 *Buffer,
                                  U32                MaxLen,
                                  U32                Timeout,
                                  U32                *ActLen);

ST_ErrorCode_t STI2C_SetParams( STI2C_Handle_t       Handle,
                                const STI2C_Params_t *Params );

ST_ErrorCode_t STI2C_SetBaudRate(STI2C_Handle_t      Handle,
                                 U32                 BaudRate);

ST_ErrorCode_t STI2C_Term ( const ST_DeviceName_t    Name,
                            const STI2C_TermParams_t *Params);

ST_ErrorCode_t STI2C_Unlock ( STI2C_Handle_t           Handle);


ST_ErrorCode_t STI2C_UnlockMultiple ( STI2C_Handle_t           *HandlePool_p,
                                      U32                      NumberOfHandles);

ST_ErrorCode_t STI2C_Write ( STI2C_Handle_t          Handle,
                             const U8                *Buffer,
                             U32                     NumberToWrite,
                             U32                     Timeout,
                             U32                     *ActLen);


ST_ErrorCode_t STI2C_WriteNoStop ( STI2C_Handle_t    Handle,
                                   const U8          *Buffer,
                                   U32               NumberToWrite,
                                   U32               Timeout,
                                   U32               *ActLen);

#ifdef __cplusplus
}
#endif

#endif /* __STI2C_H */

/* End of sti2c.h */
