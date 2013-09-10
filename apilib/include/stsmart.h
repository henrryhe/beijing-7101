/*****************************************************************************
File Name   : stsmart.h

Description : API Interfaces for the STSMART driver.

Copyright (C) 2006 STMicroelectronics

Reference   :

ST API Definition "SMART Driver API" DVD-API-11
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STSMART_H
#define __STSMART_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* Standard definitions */

#ifdef ST_OSLINUX
#include "compat.h"
#endif

#include "stpio.h"                      /* STAPI dependencies */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

#define STSMART_REVISION	"STSMART-REL_SNP0705251"    /* Smart Release Verson*/

/* Driver constants, required by the driver model */
#define STSMART_DRIVER_ID               11
#define STSMART_DRIVER_BASE             (STSMART_DRIVER_ID << 16)

/* Error constants */
#define STSMART_ERROR_WRONG_TS_CHAR     (STSMART_DRIVER_BASE)
#define STSMART_ERROR_TOO_MANY_RETRIES  (STSMART_DRIVER_BASE+1)
#define STSMART_ERROR_OVERRUN           (STSMART_DRIVER_BASE+2)
#define STSMART_ERROR_PARITY            (STSMART_DRIVER_BASE+3)
#define STSMART_ERROR_FRAME             (STSMART_DRIVER_BASE+4)
#define STSMART_ERROR_PTS_NOT_SUPPORTED       (STSMART_DRIVER_BASE+5)
#define STSMART_ERROR_NO_ANSWER         (STSMART_DRIVER_BASE+6)
#define STSMART_ERROR_INVALID_STATUS_BYTE     (STSMART_DRIVER_BASE+7)
#define STSMART_ERROR_INCORRECT_REFERENCE     (STSMART_DRIVER_BASE+8)
#define STSMART_ERROR_INCORRECT_LENGTH  (STSMART_DRIVER_BASE+9)
#define STSMART_ERROR_UNKNOWN           (STSMART_DRIVER_BASE+10)
#define STSMART_ERROR_NOT_INSERTED      (STSMART_DRIVER_BASE+11)
#define STSMART_ERROR_NOT_RESET         (STSMART_DRIVER_BASE+12)
#define STSMART_ERROR_RESET_FAILED      (STSMART_DRIVER_BASE+13)
#define STSMART_ERROR_INVALID_PROTOCOL  (STSMART_DRIVER_BASE+14)
#define STSMART_ERROR_INVALID_CLASS     (STSMART_DRIVER_BASE+15)
#define STSMART_ERROR_INVALID_CODE      (STSMART_DRIVER_BASE+16)
#define STSMART_ERROR_IN_PTS_CONFIRM    (STSMART_DRIVER_BASE+17)
#define STSMART_ERROR_ABORTED           (STSMART_DRIVER_BASE+18)
#define STSMART_ERROR_EVT_REGISTER      (STSMART_DRIVER_BASE+19)
#define STSMART_ERROR_EVT_UNREGISTER    (STSMART_DRIVER_BASE+20)
#define STSMART_ERROR_CARD_ABORTED      (STSMART_DRIVER_BASE+21)
#define STSMART_ERROR_BLOCK_RETRIES     (STSMART_DRIVER_BASE+22)
#define STSMART_ERROR_BLOCK_TIMEOUT     (STSMART_DRIVER_BASE+23)
#define STSMART_ERROR_CARD_VPP          (STSMART_DRIVER_BASE+24)
#define STSMART_ERROR_EVENT_REGISTER    (STSMART_DRIVER_BASE+25)
#define STSMART_ERROR_EVENT_UNREGISTER  (STSMART_DRIVER_BASE+26)

/* Defined for legacy purposes. */
#define STSMART_WRONG_TS_CHAR           STSMART_ERROR_WRONG_TS_CHAR
#define STSMART_TOO_MANY_RETRIES        STSMART_ERROR_TOO_MANY_RETRIES
#define STSMART_OVERRUN_ERROR           STSMART_ERROR_OVERRUN
#define STSMART_PARITY_ERROR            STSMART_ERROR_PARITY
#define STSMART_FRAME_ERROR             STSMART_ERROR_FRAME
#define STSMART_PTS_NOT_SUPPORTED       STSMART_ERROR_PTS_NOT_SUPPORTED
#define STSMART_NO_ANSWER               STSMART_ERROR_NO_ANSWER
#define STSMART_INVALID_STATUS_BYTE     STSMART_ERROR_INVALID_STATUS_BYTE
#define STSMART_INCORRECT_REFERENCE     STSMART_ERROR_INCORRECT_REFERENCE
#define STSMART_INCORRECT_LENGTH        STSMART_ERROR_INCORRECT_LENGTH
#define STSMART_UNKNOWN_ERROR           STSMART_ERROR_UNKNOWN
#define STSMART_NOT_INSERTED            STSMART_ERROR_NOT_INSERTED
#define STSMART_NOT_RESET               STSMART_ERROR_NOT_RESET
#define STSMART_RESET_FAILED            STSMART_ERROR_RESET_FAILED
#define STSMART_INVALID_PROTOCOL        STSMART_ERROR_INVALID_PROTOCOL
#define STSMART_INVALID_CLASS           STSMART_ERROR_INVALID_CLASS
#define STSMART_INVALID_CODE            STSMART_ERROR_INVALID_CODE

/* Number of exported event handler events */
#define STSMART_NUMBER_EVENTS           7
#ifdef ST_OSLINUX
#define STSMART_PIO_PIN_NOT_USED        8   /* since pin no can never be 8 */
#endif

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Smartcard handle */
typedef U32 STSMART_Handle_t;


/* Smartcard clock source */
typedef enum STSMART_ClockSrc_e
{
    STSMART_CPU_CLOCK,
    STSMART_EXTERNAL_CLOCK,
    STSMART_OTHER_CLOCK
} STSMART_ClockSrc_t;

/* Smartcard supported devices */
typedef enum STSMART_Device_e
{
    STSMART_ISO,
    STSMART_ISO_NDS
} STSMART_Device_t;

/* Smartcard events */
typedef enum STSMART_EventType_e
{
    STSMART_EV_CARD_INSERTED = STSMART_DRIVER_BASE,
    STSMART_EV_CARD_REMOVED,
    STSMART_EV_CARD_RESET,
    STSMART_EV_CARD_DEACTIVATED,
    STSMART_EV_TRANSFER_COMPLETED,
    STSMART_EV_WRITE_COMPLETED,
    STSMART_EV_READ_COMPLETED,
    STSMART_EV_NO_OPERATION
} STSMART_EventType_t;

typedef enum STSMART_CodeType_e
{
    STSMART_CODETYPE_LRC,
    STSMART_CODETYPE_CRC
} STSMART_CodeType_t;

/* Available ISO7816-3 protocols */
typedef enum STSMART_Protocol_e
{
    STSMART_UNSUPPORTED_PROTOCOL = -1,
    STSMART_T0_PROTOCOL,
    STSMART_T1_PROTOCOL,
    STSMART_T14_PROTOCOL = 14
} STSMART_Protocol_t;

/* Smartcard initialization parameters */
typedef struct STSMART_InitParams_s
{
    ST_DeviceName_t         UARTDeviceName;
    STSMART_Device_t        DeviceType;
    ST_Partition_t          *DriverPartition;
    U32                     *BaseAddress;

    STPIO_PIOBit_t          ClkGenExtClk;
    STPIO_PIOBit_t          Clk;
    STPIO_PIOBit_t          Reset;
    STPIO_PIOBit_t          CmdVcc;
    STPIO_PIOBit_t          CmdVpp;
    STPIO_PIOBit_t          Detect;

    STSMART_ClockSrc_t      ClockSource;
    U32                     ClockFrequency;
    U32                     MaxHandles;
    ST_DeviceName_t         EVTDeviceName;
    BOOL                    IsInternalSmartcard;
} STSMART_InitParams_t;

/* Smartcard termination parameters */
typedef struct STSMART_TermParams_s
{
    BOOL ForceTerminate;
} STSMART_TermParams_t;

/* Smartcard open parameters */
typedef struct STSMART_OpenParams_s
{
    void (* NotifyFunction)(STSMART_Handle_t Handle,
                            STSMART_EventType_t Event,
                            ST_ErrorCode_t ErrorCode);
    BOOL    BlockingIO;

    U8      SAD;
    U8      DAD;                   /* t=1 protocol only */
} STSMART_OpenParams_t;

/* T0 specific status parameters */
typedef struct STSMART_T0Status_s
{
    U8      Ins;
    U8      Size;
    U8      PB[2];
} STSMART_T0Status_t;

/* T1 specific status parameters */
typedef struct STSMART_T1Status_s
{
    U8      ReservedForFutureUse;
} STSMART_T1Status_t;

/* Smartcard status parameters */
typedef struct STSMART_Status_s
{
    ST_ErrorCode_t          ErrorCode;
    STSMART_Protocol_t      Protocol;
    union
    {
        STSMART_T0Status_t  T0;
        STSMART_T1Status_t  T1;
    } StatusBlock;
} STSMART_Status_t;

/* Current card operating parameters */
typedef struct STSMART_Params_s
{
    STSMART_Status_t    CardStatus;
    U16                 SupportedProtocolTypes;
    BOOL                ProtocolTypeChangeable;
    U32                 BaudRate;
    U32                 MaxBaudRate;
    U32                 ClockFrequency;
    U32                 MaxClockFrequency;
    U32                 WorkETU;
    U32                 WorkWaitingTime;
    U8                  GuardDelay;
    U8                  TxRetries;
    BOOL                VppActive;
    BOOL                DirectBitConvention;
    U8                  BlockRetries;           /* t=1 only */
    U32                 BlockWaitingTime;       /* t=1 only */
    U32                 BlockGuardTime;         /* t=1 only */
    U32                 CharacterWaitingTime;   /* t=1 only */
} STSMART_Params_t;

/* Parameter used to exchange data to Event Handler */
typedef struct STSMART_EvtParams_s
{
    STSMART_Handle_t    Handle;
    ST_ErrorCode_t      Error;
} STSMART_EvtParams_t;

/* Driver capabilities */
typedef struct STSMART_Capability_s
{
    STSMART_Device_t    DeviceType;
    U16                 SupportedISOProtocols;
} STSMART_Capability_t;

#if defined (STSMART_WARM_RESET)
typedef enum STSMART_Reset_e
{
    SC_COLD_RESET=0,
    SC_WARM_RESET
} STSMART_Reset_t;
#endif


/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */


ST_ErrorCode_t STSMART_Abort(STSMART_Handle_t Handle);
ST_ErrorCode_t STSMART_Close(STSMART_Handle_t Handle);
ST_ErrorCode_t STSMART_DataAvailable(STSMART_Handle_t Handle,
                                     U32 *Size_p);
ST_ErrorCode_t STSMART_Deactivate(STSMART_Handle_t Handle);
ST_ErrorCode_t STSMART_Flush(STSMART_Handle_t Handle);
ST_ErrorCode_t STSMART_GetCapability(const ST_DeviceName_t DeviceName,
                                     STSMART_Capability_t *Capability_p);
ST_ErrorCode_t STSMART_GetHistory(STSMART_Handle_t Handle,
                                  U8 History[15],
                                  U32 *HistoryLength_p);
ST_ErrorCode_t STSMART_GetParams(STSMART_Handle_t Handle,
                                 STSMART_Params_t *Params_p);
ST_ErrorCode_t STSMART_GetPtsBytes(STSMART_Handle_t Handle,
                                   U8 PtsBytes[4]);
ST_Revision_t  STSMART_GetRevision(void);


ST_ErrorCode_t STSMART_GetStatus(STSMART_Handle_t Handle,
                                 STSMART_Status_t *Status_p);

ST_ErrorCode_t STSMART_GetRxTimeout(STSMART_Handle_t Handle,
                                    U32 *Timeout_p);
ST_ErrorCode_t STSMART_Init(/*const*/ ST_DeviceName_t DeviceName,
                            const STSMART_InitParams_t *InitParams_p);
ST_ErrorCode_t STSMART_Lock(STSMART_Handle_t Handle, BOOL WaitForLock);
ST_ErrorCode_t STSMART_Open(/*const*/ ST_DeviceName_t DeviceName,
                            const STSMART_OpenParams_t *OpenParams_p,
                            STSMART_Handle_t *Handle_p);
ST_ErrorCode_t STSMART_RawRead(STSMART_Handle_t Handle,
                               U8 *Buffer,
                               U32 NumberToRead,
                               U32 *NumberRead_p,
                               U32 Timeout);
ST_ErrorCode_t STSMART_RawWrite(STSMART_Handle_t Handle,
                                const U8 *Buffer,
                                U32 NumberToWrite,
                                U32 *NumberWritten_p,
                                U32 Timeout);

ST_ErrorCode_t STSMART_Reset(STSMART_Handle_t Handle,
                             U8 Answer[33],
#if defined( STSMART_WARM_RESET )
                             U32 *AnswerLength_p,
                             STSMART_Reset_t ResetType
                             #ifdef IRDETOCA
				 , int ri_Time
				 #endif
                             );  /* SC_COLD/WARM_RESET */
#else
                             U32 *AnswerLength_p
                             #ifdef IRDETOCA
				 , int ri_Time
				 #endif
                             );
#endif

ST_ErrorCode_t STSMART_SetBaudRate(STSMART_Handle_t Handle,
                                   U32 BaudRate);
ST_ErrorCode_t STSMART_SetBlockWaitIndex(STSMART_Handle_t Handle,
                                         U8 Index);
ST_ErrorCode_t STSMART_SetCharWaitIndex(STSMART_Handle_t Handle,
                                        U8 Index);
ST_ErrorCode_t STSMART_SetClockFrequency(STSMART_Handle_t Handle,
                                         U32 Frequency,
                                         U32 *ActualFrequency);
ST_ErrorCode_t STSMART_SetErrorCodeType(STSMART_Handle_t Handle,
                                        STSMART_CodeType_t Code);
ST_ErrorCode_t STSMART_SetGuardDelay(STSMART_Handle_t Handle,
                                     U16 GuardDelay);
ST_ErrorCode_t STSMART_SetInfoFieldSize(STSMART_Handle_t Handle,
                                        U8 IFSD);
ST_ErrorCode_t STSMART_SetProtocol(STSMART_Handle_t Handle,
                                   U8 *PtsBytes_p);
ST_ErrorCode_t STSMART_SetTxRetries(STSMART_Handle_t Handle,
                                    U8 TxRetries);

ST_ErrorCode_t STSMART_SetRxTimeout(STSMART_Handle_t Handle,
                                    U32 Timeout);

ST_ErrorCode_t STSMART_SetVpp(STSMART_Handle_t Handle,
                              BOOL VppActive);
ST_ErrorCode_t STSMART_SetWorkWaitingTime(STSMART_Handle_t Handle,
                                          U32 WorkWaitingTime);

ST_ErrorCode_t STSMART_StoreClockFrequency(STSMART_Handle_t Handle,
                                           U32 Frequency);

ST_ErrorCode_t STSMART_Term( ST_DeviceName_t DeviceName,
                            const STSMART_TermParams_t *TermParams_p);
ST_ErrorCode_t STSMART_Transfer(STSMART_Handle_t Handle,
                                U8 *Command_p,
                                U32 NumberToWrite,
                                U32 *NumberWritten_p,
                                U8 *Response_p,
                                U32 NumberToRead,
                                U32 *NumberRead_p,
                                STSMART_Status_t *Status_p);
ST_ErrorCode_t STSMART_Unlock(STSMART_Handle_t Handle);

#if defined(ST_OSLINUX)
ST_ErrorCode_t STSMART_GetReset(STSMART_Handle_t Handle,
                             U8 Answer[33],
                             U32 *AnswerLength_p);
                             
ST_ErrorCode_t STSMART_GetTransfer(STSMART_Handle_t Handle,
                                U8 *Command_p,
                                U32 NumberToWrite,
                                U32 *NumberWritten_p,
                                U8 *Response_p,
                                U32 NumberToRead,
                                U32 *NumberRead_p,
                                STSMART_Status_t *Status_p);                           
#endif                             


#ifdef __cplusplus
}
#endif

#endif /* __STSMART_H */

/* End of stsmart.h */
