/*****************************************************************************
File Name   : scdat.h

Description : Internal data structures.

Copyright (C) 2006 STMicroelectronics

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SCDAT_H
#define __SCDAT_H

/* Includes --------------------------------------------------------------- */

#include "stos.h"
#include "stevt.h"
#include "stsmart.h"
#include "stddefs.h"
#include "scio.h"

/* Exported Constants ----------------------------------------------------- */
#define IN
#define OUT

/* Board/chip derived values */

/* Define ASC4_PRESENT, depending on what we're building for */
#if defined(ST_5514) || defined(ST_5516)
    #define ASC4_PRESENT
#endif

#ifdef ST_OSLINUX
#define STSMART_MAPPING_WIDTH 4096
#endif
/* Smartcard setup parameters */
#define SMART_INITIAL_BAUD      9600
#define SMART_INITIAL_ETU       372

#define SMART_MAX_HISTORY       15
#define SMART_MAX_ANSWER        256
#define SMART_MAX_PTS           6
#define SMART_MAX_ATR           30
#define SMART_MIN_ATR           2

/* Taken from ISO7816:3 in section 5.2 Activation */
#define SMART_MAX_RESET_FREQUENCY   5000000
#define SMART_MIN_RESET_FREQUENCY   1000000


#define SMART_RX_TIMEOUT        1000 /* 1s */
#define SMART_TX_TIMEOUT        1000 /* 1s */

/* Special values */
#define MHZ                     1000000 /* Definition of 1x10^6 */
#define SMART_RESERVED          0
#define SMART_INTERNAL_CLOCK    ((U16)-2)

/* Default values */
#define SMART_N_DEFAULT         2       /* Extra guardtime */
#define SMART_F_DEFAULT         1       /* Encodes clockrate conversion */
#define SMART_D_DEFAULT         1       /* Encodes bitrate conversion */
#define SMART_W_DEFAULT         10      /* Encodes work waiting time */
#define SMART_RETRIES_DEFAULT   3       /* Transmit retries */
#define SMART_GUARD_DELAY_MIN   2
#define SMART_GUARD_DELAY_MAX   255

/* PTS defines */
#define PTS_INITIAL_CHAR        0xFF

/* Maximum number of handles we allow per smartcard device -- always
 * a power of two.
 */
#define HANDLE_EXPONENT         5       /* Yields 32 handles limit */
#define SMART_MAX_HANDLES       32

/* Open index from handle conversion */
#define INDEX_FROM_HANDLE(h)    (h % SMART_MAX_HANDLES)

/* Convert event number to EvtId array offset */
#define EventToId(x)            (x - STSMART_DRIVER_BASE)
#define SMART_UsingEventHandler(x) \
           ((x->InitParams.EVTDeviceName[0] != 0) ? TRUE : FALSE)

/* Task stack size */
#ifdef ST_OS21
#define SMART_TASK_STACK_SIZE       (1024 * 20)  /* 16k min stack size */
#else
#define SMART_TASK_STACK_SIZE       (1024 * 3)   /* 3k */
#endif

#if !defined(DVD_USE_GLOBAL_PRIORITIES)
/* Allow override of task priority */
#if !defined(STSMART_EVENTMGR_TASK_PRIORITY)
    #define STSMART_EVENTMGR_TASK_PRIORITY    MAX_USER_PRIORITY
#endif
#endif

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Smartcard bit convention */
typedef enum
{
    ATR_INVERSE_CONVENTION = 0x3F,
    ATR_DIRECT_CONVENTION  = 0x3B
} SMART_BitConvention_t;

/*****************************************************************************
SMART_CardLock_t:
This structure is used for managing exclusive access to a smartcard device.
When a smartcard is locked, only the owner can access it.
*****************************************************************************/

typedef struct
{
    semaphore_t                 *Semaphore_p;
    STSMART_Handle_t            Owner;
} SMART_CardLock_t;

/*****************************************************************************
SMART_OpenBlock_t:
Represents the parameters that the client opened with -- this may contain
additional protocol information, for example, SAD and DAD for T=1.
*****************************************************************************/

typedef struct
{
    BOOL InUse;
    STSMART_Handle_t Handle;
    STSMART_OpenParams_t OpenParams;
    void                 *ControlBlock;
} SMART_OpenBlock_t;

/*****************************************************************************
SMART_Context_t:
Each operation to the smartcard must have a context e.g., reading, writing.
The context of the operation is required in order for the UART callback
handling routines to determine what events to generate when an operation
completes.
*****************************************************************************/

typedef enum
{
    SMART_CONTEXT_NONE,                 /* default (none) */
    SMART_CONTEXT_READ,                 /* read operation */
    SMART_CONTEXT_WRITE,                /* write operation */
    SMART_CONTEXT_TRANSFER,             /* write & read operation */
    SMART_CONTEXT_RESET,                /* reset operation */
    SMART_CONTEXT_SETPROTOCOL           /* set protocol operation */
#if defined( STSMART_WARM_RESET )
   ,SMART_CONTEXT_WARM_RESET           /* warm reset */
#endif
} SMART_Context_t;

/*****************************************************************************
SMART_Operation_t:
Any communications with the smartcard (via the UART) is assigned an
operation.  This block of information enables us to keep track of whom
to notify when the operation completes, what the operation context is,
and the result of the operation.
*****************************************************************************/

typedef struct SMART_Operation_s
{
    /* Input parameters */
    SMART_Context_t             Context;          /* Context of operation e.g., ATR, TRANSFER */
    SMART_OpenBlock_t           *OpenBlock_p;     /* Client-specific parameters e.g, SAD/DAD */
    U8                          *ReadBuffer_p;    /* Pointer to user's read buffer */
    U8                          *WriteBuffer_p;   /* Pointer to user's read buffer */
    U32                         NumberToWrite;
    U32                         NumberToRead;
    U32                         Timeout;

    /* Output parameters */
    U32                         *ReadBytes_p;     /* Pointer to final number read */
    U32                         *WrittenBytes_p;  /* Pointer to final number written */
    STSMART_Status_t            *Status_p;        /* Status block to complete */
    ST_ErrorCode_t              *Error_p;         /* Final error-code storage */
    semaphore_t                 *NotifySemaphore_p; /* Semaphore to wake-up on completion */
} SMART_Operation_t;


/*****************************************************************************
SMART_MessageType_t:
The smartcard event manager must handle messages of the following types.
*****************************************************************************/

typedef enum
{
    SMART_EVENT,                        /* A smartcard event message */
    SMART_DEBUG,                        /* Debug message output */
    SMART_TASK_CONTROL,                 /* Task control message */
    SMART_IO_OPERATION                  /* IO operation */
} SMART_MessageType_t;

/*****************************************************************************
SMART_Message_t:
All messages passed to the smartcard manager task should adhere to
this structure.  Message types are defined in SMART_MessageType_t.
*****************************************************************************/

typedef struct SMART_Message_s
{
    SMART_MessageType_t MessageType;    /* Message type */
    union                               /* Message params: union of msg types */
    {
        struct                          /* Event params */
        {
            void *Smart_p;                 /* Message originator */
            BOOL Broadcast;                /* If the event needs to be broadcast */
            STSMART_Handle_t Handle;       /* For an event to a specific client */
            STSMART_EventType_t Event;     /* Event for generating callback */
            ST_ErrorCode_t Error;          /* Errorcode associated with event */
        }  EventParams_s;
        struct
        {
            BOOL ExitTask;              /* For terminating the task */
        } TaskControlParams_s;
        struct                          /* Debug params */
        {
            char DebugMessage[256];     /* For displaying debug info */
        } DebugParams_s;
        struct                          /* For initiating an IO operation */
        {
            void *Smart_p;
        } IoParams_s;
    } MessageParams_u;
} SMART_Message_t;

/* Message queue */
#define SMART_MQUEUE_MAX_MESSAGES   10

#if 0
#define SMART_MQUEUE_SIZE           ( SMART_MQUEUE_MAX_MESSAGES * \
                                    ( sizeof(message_hdr_t) + \
                                    (( sizeof(SMART_Message_t) + 3 ) & ~3 )))
#else
#define SMART_MQUEUE_SIZE           ( SMART_MQUEUE_MAX_MESSAGES * \
                              ( 80 + (( sizeof(SMART_Message_t) + 3 ) & ~3 )))
#endif

/*****************************************************************************
SMART_T1Details_t:
Contains details about T=1 state/implementation
*****************************************************************************/
typedef struct SMART_T1Details_s
{
    U32     State;
    U8      TxMaxInfoSize;
    U8      RxMaxInfoSize;
    U8      BWTMultiplier;
    U8      OurSequence;        /* Sequence number we're just {about to|did}
                                transmit */
    U8      TheirSequence;      /* Last number we got from them.    */
    U8      TxCount;
    U8      RxCount;
    U32     BWT,CWT,BGT;
    BOOL    FirstBlock;
    BOOL    Aborting;
    STOS_Clock_t LastReceiveTime, NextTransmitTime;
} SMART_T1Details_t;

/*****************************************************************************
SMART_ControlBlock_t:
The top-level datastructure that represents an instance of a smartcard,
connected clients and current operational parameters.
*****************************************************************************/

struct SMART_ControlBlock_s
{
    /* STSMART initialization */
    ST_DeviceName_t             DeviceName;
    STSMART_InitParams_t        InitParams;
    STSMART_Capability_t        Capability;
    U32                         Magic;

#if defined(ST_OSLINUX)  && defined(LINUX_FULL_KERNEL_VERSION)
    /* Linux ioremap address */
    unsigned long              *MappingBaseAddress;
    unsigned long               SmartMappingWidth;
#endif
    /* Event handler parameters */
    STEVT_EventID_t             EvtId[STSMART_NUMBER_EVENTS];
    STEVT_Handle_t              EVTHandle;

    /* IO parameters */
    SMART_IO_Handle_t           IoHandle;
    U32                         RxTimeout;
    U32                         TxTimeout;

    /* STPIO handles */
    STPIO_Handle_t              ClkGenExtClkHandle;
    STPIO_Handle_t              ClkHandle;
    STPIO_Handle_t              ResetHandle;
    STPIO_Handle_t              CmdVccHandle;
    STPIO_Handle_t              CmdVppHandle;
    STPIO_Handle_t              DetectHandle;

    /* Event manager task data */
    tdesc_t                     EventManagerTaskDescriptor;
    void                        *EventManagerTaskStack_p;
    task_t                      *EventManagerTask_p;

    /* Event manager message queue */
#ifndef ST_OS21
    message_queue_t             MessageQueue;
    U8                          MessageQueueMemory[SMART_MQUEUE_SIZE];
#endif
    message_queue_t             *MessageQueue_p;

    /* Smartcard private parameters */
    SMART_OpenBlock_t           OpenHandles[SMART_MAX_HANDLES]; /* Per client */
    SMART_CardLock_t            CardLock; /* For exclusive access to smartcard */
    U32                         FreeHandles; /* Number of clients allowed */
    BOOL                        CardInserted; /* Indicates card presence */
    ST_ErrorCode_t              CardReset; /* Indicates whether we're reset */

    /* Pending operation */
    SMART_Operation_t           IoOperation;
    BOOL                        IoAbort;

    /* Communications buffers */
    U8                          HistoricalBytes[SMART_MAX_HISTORY];
    U8                          ATRBytes[SMART_MAX_ATR];

    /* Current operational status */
    STSMART_Params_t            SmartParams;
    STSMART_Status_t            *Status_p;
    SMART_T1Details_t           T1Details;

    /* Current IO information */
    U32                         AnswerSize; /* Size of last answer */
    U32                         ExpectedAnswerSize; /* Anticipated answer length */
    U8                          INS;    /* Instruction byte of last command */
    U32                         TxSize; /* Size of tx command */
    U32                         TxSent; /* Bytes written for last transfer */
    U32                         NextTxSize; /* Next exchange size requested */
    U8                          Retries; /* Transmit retries */
    BOOL                        VppState;

    U32                         Fi;     /* Initial clock frequency to smartcard */
    U32                         Fs;     /* Subsequent clock frequency */
    U32                         Fmax;   /* Maximum clock frequency */
#if defined (STSMART_WARM_RESET)
    U32                         WarmResetFreq;
#endif
    /* Interface parameters (from ATR) */
    SMART_BitConvention_t       BitConvention;
    U32                         ATRMsgSize; /* Size of current ATR message */
    U32                         ATRHistoricalSize; /* Size of ATR historical data */
    U8                          TS;     /* ATR's TS value NOTE: see ISO7816-3 */
    U8                          FInt;   /* "F" variables integer represenation */
    U8                          DInt;   /* "D" variables integer represenation */
    U8                          WorkingType; /* Current working protocol type */
    BOOL                        SpecificMode; /* Specific mode of card indicated? */
    U8                          SpecificType; /* Specific protocol type */
    BOOL                        SpecificTypeChangable; /* Type changeable? */
    U8                          IFSC;   /* Card's maximum block size for (T=1) */
    U8                          IInt;   /* "II" variables integer represenation */
    U8                          PInt1;  /* "PI1" variables integer represenation */
    U8                          PInt2;  /* "PI2" variables integer represenation */
    U8                          CWInt;  /* "CWT" variables integer represenation */
    U8                          BWInt;  /* "BWT" variables integer represenation */
    U16                         N;      /* "N" setting */
    U8                          WInt;   /* "WI" variables integer represenation */
    U8                          RC;     /* Redundancy Check used CRC=1 LRC=0 (T=1) */
    U32                         WorkETU; /* Elementary time unit */
    U32                         WWT;    /* Work waiting time */
    BOOL                        JustAfterAtr; /* TRUE when no other command has been sent after ATR */
    U32                         ResetFrequency; /* If non-zero, this is the frequency the user wishes to reset at */

    /* Bytes sent by the card from the last PTS operation; PCK stripped */
    U8                          PtsBytes[4];
    /* internal smartcard support */
    BOOL                        IsInternalSmartcard;

    /* Control semaphores */
    semaphore_t                 *ControlLock_p; /* For exclusive access to private data */
    semaphore_t                 *IoSemaphore_p; /* Allows blocking IO */
    semaphore_t                 *RxSemaphore_p; /* Used by t=1 for callback */
    struct SMART_ControlBlock_s *Next_p; /* Next control block in list */
};

typedef struct SMART_ControlBlock_s SMART_ControlBlock_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

#endif /* __SCDAT_H */

/* End of scdat.h */
