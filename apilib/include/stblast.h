/*****************************************************************************
File Name   : stblast.h

Description : API Interfaces for the STBLAST driver.

Copyright (C) 2000 STMicroelectronics

Reference   : ST API Definition "ST Blaster Driver API" DVD-API-138
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STBLAST_H
#define __STBLAST_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* Standard definitions */

#include "stpio.h"                      /* STAPI dependencies */
#include "stevt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Driver constants, required by the driver model */
#define STBLAST_DRIVER_ID           138
#define STBLAST_DRIVER_BASE         (STBLAST_DRIVER_ID << 16)

/* STBLAST Release Version*/
/*should be 30 chars long.
This is precisely this long:".............................."*/
#define STBLAST_REVISION	"STBLAST-REL_3.5.0             "

/* Maximum space for start/stop symbols - can be increased up to 255, if
 * really needed.
 */
#define STBLAST_MAX_START_SYMBOLS   10
#define STBLAST_MAX_STOP_SYMBOLS    10

/* Error constants */
enum
{
    STBLAST_ERROR_EVENT_REGISTER = STBLAST_DRIVER_BASE,
    STBLAST_ERROR_EVENT_UNREGISTER,
    STBLAST_ERROR_INVALID_DEVICE_TYPE,
    STBLAST_ERROR_PIO,
    STBLAST_ERROR_OVERRUN,
    STBLAST_ERROR_ABORT,
    STBLAST_ERROR_UNDEFINED_PROTOCOL
#ifdef ST_OSLINUX
    ,
    STBLAST_ERROR_MAP
#endif
};

/* Exported Variables ----------------------------------------------------- */

/* Event types */

#define STBLAST_NUM_EVENTS          2
enum
{
    STBLAST_READ_DONE_EVT = STBLAST_DRIVER_BASE,
    STBLAST_WRITE_DONE_EVT
};

/* Exported Types --------------------------------------------------------- */

typedef U32 STBLAST_Handle_t;


/* Specifies the functionality and limitations of the RC device */
typedef enum STBLAST_Device_e
{
    STBLAST_DEVICE_IR_TRANSMITTER = 1,
    STBLAST_DEVICE_IR_RECEIVER    = 2,
    STBLAST_DEVICE_UHF_RECEIVER   = 3,
    STBLAST_DEVICE_UNKNOWN        = 4/* Must always be last enum */
} STBLAST_Device_t;


/* Backwards compatability only */
#define RC_TRANSMITTER  STBLAST_DEVICE_RC_TRANSMITTER
#define RC_RECEIVER     STBLAST_DEVICE_RC_RECEIVER


typedef enum STBLAST_Coding_e
{
    STBLAST_CODING_PULSE = 1,
    STBLAST_CODING_SPACE = 2,
    STBLAST_CODING_SHIFT = 4
} STBLAST_Coding_t;


/* Backwards compatability only */
#define PULSE_CODING    STBLAST_RC_CODING_PULSE_CODING
#define SPACE_CODING    STBLAST_RC_CODING_SPACE_CODING
#define SHIFT_CODING    STBLAST_RC_CODING_SHIFT_CODING
#define RC5_CODING      STBLAST_RC_CODING_RC5_CODING


/* IR Blaster device driver capability result structure */
typedef struct STBLAST_Capability_s
{
    STBLAST_Device_t    Device;
    U32                 SupportedProtocols; /* was SupportedRcProtocols */
    U32                 SupportedCodingTypes;
    BOOL                ProgrammableDutyCycle;
} STBLAST_Capability_t;


/* RC receiver device operating parameters */
typedef struct STBLAST_RxParams_s
{
    U16  GlitchWidth; /* Changed from U8 to U16 for new interrupt mech. */
} STBLAST_RxParams_t;

/* RC transmitter device operating parameters */
typedef void *STBLAST_TxParams_t;

/* Definition of a single RC symbol */
typedef struct STBLAST_Symbol_s
{
    U16 MarkPeriod;
    U16 SymbolPeriod;
} STBLAST_Symbol_t;

/* Termination parameters */
typedef struct STBLAST_TermParams_s
{
    BOOL    ForceTerminate;
} STBLAST_TermParams_t;

typedef enum STBLAST_Protocol_e
{
    STBLAST_PROTOCOL_USER_DEFINED = 1,
    STBLAST_PROTOCOL_RC5  = 2,
    STBLAST_LOW_LATENCY_PROTOCOL = 3,
    STBLAST_PROTOCOL_RC6A = 4,
    STBLAST_PROTOCOL_RC6_MODE0 = 5, 		/* Added for RC6 mode 0 */
    STBLAST_PROTOCOL_RCMM = 6,      		/* Added for RCMM 24 /32 Protocol */
    STBLAST_PROTOCOL_RUWIDO = 8,    		/* Added for Ruwido protocol */
    STBLAST_PROTOCOL_RCRF8 = 9,	    		/* Added for RCRF8 protocol */
    STBLAST_PROTOCOL_MULTIPLE = 10, 		/* Added for multiple protocol support*/
    STBLAST_PROTOCOL_RMAP = 11,     		/* Added for RMAP protocol with Mancester coding*/
    STBLAST_PROTOCOL_RSTEP = 12,     	 	/* Added for RSTEP protocol */
    STBLAST_PROTOCOL_RMAP_DOUBLEBIT = 13,    /* Added for RMAP protocol with Double bit coding*/
    STBLAST_LOW_LATENCY_PRO_PROTOCOL = 14
} STBLAST_Protocol_t;

/* new type - replaces STBLAST_RcProtocol_t */
typedef  struct STBLAST_ProtocolParams_u
{
    struct
    {
        U8 CustomerCode;
        U8 BufferElementsPerPayload;
        U8 NumberPayloadBits;
        BOOL DisableAddressCheck;
    } RC6A;

    struct
    {
        U8 CustomerCode;
        U8 BufferElementsPerPayload;
        U8 NumberPayloadBits;
        BOOL DisableAddressCheck;
    } RC6M0;

    struct
    {
        U8 SystemCode;
        U8 NumberPayloadBits;
    }LowLatency;
    
    struct
    {
        U8  SystemCode;
        U8  UserCode;
        U32 LongAddress;
        U8  NumberPayloadBits;
    }RFLowLatencyPro;
    
    struct
    {
        U8   NumberPayloadBits;
        BOOL ShortID; /* Short Customer ID */
        U8   SubMode; /* 0=RC, 1=Mouse , 2=Keyboard, 3=Game */
        U8   Address;
        U16  CustomerID;
    }Rcmm;

    struct
    {
    	U8   FrameLength;
        U8   CustomID;
        U8   DeviceID;
        U8   Address;
    }Ruwido;

    struct
    {
        U8 RRA;
        U8 Mode;
        U16 CustomerId;
        U8 SubMode;
        U8 Address;
    } RCRF8;

    struct
    {
    STBLAST_Coding_t    Coding;
    U16                 SubCarrierFreq;
    U8                  DutyCycle;
    U8                  NumberStartSymbols;
    U8                  NumberStopSymbols;
    U8                  BufferElementsPerPayload;
    U8                  NumberPayloadBits;
    STBLAST_Symbol_t    StartSymbols[STBLAST_MAX_START_SYMBOLS];
    STBLAST_Symbol_t    StopSymbols[STBLAST_MAX_STOP_SYMBOLS];
    STBLAST_Symbol_t    HighDataSymbol;
    STBLAST_Symbol_t    LowDataSymbol;
    U32                 Delay;
    } UserDefined;

} STBLAST_ProtocolParams_t;


/* Initialisation parameters */
typedef struct STBLAST_InitParams_s
{
    STBLAST_Device_t    DeviceType;
    ST_Partition_t      *DriverPartition;
    U32                 *BaseAddress;
#if defined(ST_OSLINUX)
    U32                 InterruptNumber;
#else
#ifndef ST_OS21
    U32                 InterruptNumber;
#else
    interrupt_name_t    InterruptNumber;
#endif
#endif /* ST_OSLINUX  */
    U32                 InterruptLevel;
    ST_DeviceName_t     EVTDeviceName;
    U32                 ClockSpeed;
    STPIO_PIOBit_t      RxPin;
    STPIO_PIOBit_t      TxPin;
    STPIO_PIOBit_t      TxInvPin;
    BOOL                InputActiveLow;
    U32                 SymbolBufferSize;
#if defined(ST_OSLINUX)
    U32                 CmdBufferSize; /* The maximum of commands to be processed */
#endif
} STBLAST_InitParams_t;


/* Parameters for the STBLAST_Open function */
typedef union STBLAST_OpenParams_s
{
    STBLAST_TxParams_t      TxParams;
    STBLAST_RxParams_t      RxParams;
} STBLAST_OpenParams_t;


/* Parameters returned to subscribers of the
 * STBLAST_READ_DONE_EVT event
 */
typedef struct STBLAST_EvtReadDoneParams_s
{
    STBLAST_Handle_t    Handle;
    ST_ErrorCode_t      Result;
} STBLAST_EvtReadDoneParams_t;

/* Parameters returned to subscribers of the
 * STBLAST_WRITE_DONE_EVT event
 */
typedef struct STBLAST_EvtWriteDoneParams_s
{
    STBLAST_Handle_t    Handle;
    ST_ErrorCode_t      Result;
} STBLAST_EvtWriteDoneParams_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* Added Functions on STLinux */
#if defined(ST_OSLINUX)
ST_ErrorCode_t STBLAST_GetSymbolRead(STBLAST_Handle_t Handle, U32 *SymbolsRead);

ST_ErrorCode_t STBLAST_CopySymbolstoBuffer(STBLAST_Handle_t Handle, STBLAST_Symbol_t *Buffer,
                                        U32 NumSymboltoCopy, U32 *NumSymbolCopied);

ST_ErrorCode_t STBLAST_GetCmdReceived(STBLAST_Handle_t Handle, U32 *NumCmdReceived);

ST_ErrorCode_t STBLAST_CopyCmdstoBuffer(STBLAST_Handle_t Handle, U32 *Cmd, U32 NumCmdtoCopy,
                                        U32 *NumCmdCopied);
ST_ErrorCode_t STBLAST_GetSymbolWritten(STBLAST_Handle_t Handle, U32 *SymbolsWritten);
ST_ErrorCode_t STBLAST_GetCmdSent(STBLAST_Handle_t Handle, U32 *NumCmdSent);
                                        
#endif

ST_ErrorCode_t STBLAST_Abort(STBLAST_Handle_t Handle);
ST_ErrorCode_t STBLAST_Close(STBLAST_Handle_t Handle);
ST_ErrorCode_t STBLAST_GetCapability(const ST_DeviceName_t DeviceName,
                                     STBLAST_Capability_t *Capability
                                    );


/* this is not an exported function */
ST_ErrorCode_t STBLAST_GetEvtHandle(STBLAST_Handle_t Handle,
                                    STEVT_Handle_t *EvtHandle);
/******/


ST_ErrorCode_t STBLAST_GetProtocol( STBLAST_Handle_t Handle,
                                    STBLAST_Protocol_t *Protocol,
                                    STBLAST_ProtocolParams_t *ProtocolParams
                                    );

ST_ErrorCode_t STBLAST_SetSubCarrierFreq( STBLAST_Handle_t Handle, U32 SubCarrierFreq );

ST_Revision_t STBLAST_GetRevision(void);
ST_ErrorCode_t STBLAST_Init(const ST_DeviceName_t DeviceName,
                            const STBLAST_InitParams_t *InitParams
                           );
ST_ErrorCode_t STBLAST_Open(const ST_DeviceName_t DeviceName,
                            const STBLAST_OpenParams_t *OpenParams,
                            STBLAST_Handle_t *Handle
                           );

ST_ErrorCode_t STBLAST_ReceiveExt(  STBLAST_Handle_t Handle,
                                    U32 *Buffer,
                                    U32 NumberToRead,
                                    U32 *NumberRead,
                                    U32 ReceiveTimeout,
                                    STBLAST_Protocol_t *ProtocolSet
                                );

ST_ErrorCode_t STBLAST_Receive(  STBLAST_Handle_t Handle,
                                 U32 *Buffer,
                                 U32 NumberToRead,
                                 U32 *NumberRead,
                                 U32 ReceiveTimeout
                                );

ST_ErrorCode_t STBLAST_Send(STBLAST_Handle_t Handle,
                              U32 *Buffer,
                              U32 NumberToSend,
                              U32 *NumberSent,
                              U32 SendTimeout
                             );

ST_ErrorCode_t STBLAST_ReadRaw(STBLAST_Handle_t Handle,
                            STBLAST_Symbol_t *Buffer,
                            U32 SymbolsToRead,
                            U32 *SymbolsRead,
                            U32 SymbolTimeout,
                            U32 ReadTimeout
                           );

ST_ErrorCode_t STBLAST_SetProtocol( STBLAST_Handle_t Handle,
                                    const STBLAST_Protocol_t Protocol,
                                    STBLAST_ProtocolParams_t *ProtocolParams
                                    );

ST_ErrorCode_t STBLAST_Term(const ST_DeviceName_t DeviceName,
                            const STBLAST_TermParams_t *TermParams
                           );

ST_ErrorCode_t STBLAST_WriteRaw(STBLAST_Handle_t Handle,
                             U32 SubCarrierFreq,
                             U8  DutyCycle,
                             const STBLAST_Symbol_t *Buffer,
                             U32 SymbolsToWrite,
                             U32 *SymbolsWritten,
                             U32 WriteTimeout
                            );

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STBLAST_H */

/* End of stblast.h */
