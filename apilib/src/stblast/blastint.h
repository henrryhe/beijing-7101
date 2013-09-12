/*****************************************************************************
File Name   : blastint.h

Description : Blaster internal definitions and routines (headers).

Copyright (C) 2000 STMicroelectronics

History     :

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BLASTDRV_H
#define __BLASTDRV_H

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include "stddefs.h"                    /* STAPI includes */
#endif

#include "stos.h"
#include "stblast.h"
#include "blasthw.h"
#include "stevt.h"
#include "timer.h"

#include "space.h"                      /* Coding routines */
#include "rc5.h"
#include "rc6a.h"
#include "shift.h"
#include "ruwido.h"
#include "rc6aM0.h"
#include "rcrf8.h"
#include "lowlatency.h"
#include "rcmm.h"
#include "rmap.h"
#include "rmapdoublebit.h"
#include "lowlatencyPro.h"

/* Exported Constants ----------------------------------------------------- */

/* Which operation to take from the pool */
#define BLAST_OP_PENDING            -1

#if defined(ST_OSLINUX)
#define STBLAST_MAPPING_WIDTH       0x400
#endif

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_7100) || defined(ST_5301) \
|| defined(ST_8010) || defined(ST_7109) || defined(ST_5525) || defined(ST_5188)  || defined(ST_5107) \
|| defined(ST_7200)
  #ifndef NEW_INTERRUPT_MECHANISM
    #define    NEW_INTERRUPT_MECHANISM
  #endif
#endif

/* Macros ----------------------------------------------------------------- */

/* Macros for converting microseconds to symbol periods */
#define SYMBOL_TIME_MICROSECONDS(f)     ( ( 1000000 + ( (f) >> 1) ) / (f) )
#define MICROSECONDS_TO_SYMBOLS(f, m)   ( (m) / SYMBOL_TIME_MICROSECONDS(f) )

/* Macro for computing min of two numbers */
#ifdef min
#undef min
#endif
#define min(x,y) (x<y?x:y)

/* Macro for computing max of two numbers */
#ifdef max
#undef max
#endif
#define max(x,y) (x>y?x:y)

/* Macro for converting events to index numbers */
#define EVENT_TO_INDEX(x)       (x - STBLAST_DRIVER_BASE)

/* Exported Types --------------------------------------------------------- */

/*****************************************************************************
BLAST_Operation_t:
A convenient placeholder used to represent the current read or write operation,
although the 'TermString' member only applies to a read operation.
Whenever a new read/write operation is started, a BLAST_Operation_t is filled
out for keeping track of the progress of the operation.
The structure will typically be updated by:
a) the ISR (e.g., when data arrives or is sent),
b) a timer callback (e.g, when a timeout occurs).
*****************************************************************************/

typedef struct BLAST_Operation_s
{
    ST_ErrorCode_t      Status;         /* Error code for callback */
    clock_t             Timeout;        /* Timeout in clock ticks */
    BOOL                RawMode;        /* For raw symbol transfers */
    U32                 *UserBuf_p;     /* Points to user buffer */
    U32                 UserBufSize;    /* Size of user buffer */
    U32                 UserRemaining;  /* Number of units to transfer */
    U32                 *UserCount_p;   /* Points to user transfer count */
    U32                 UserPending;    /* Last transfer count */
} BLAST_Operation_t;

struct BLAST_ControlBlock_s
{
    ST_DeviceName_t         DeviceName;
    STBLAST_Device_t        DeviceType;
    U8                      DeviceNumber;
    STBLAST_Handle_t        Handle;
    STPIO_Handle_t          RxPIOHandle;
    STPIO_Handle_t          TxPIOHandlePPM;
    STPIO_Handle_t          TxPIOHandleJack;
    STBLAST_InitParams_t    InitParams;
    STBLAST_OpenParams_t    OpenParams;
    STBLAST_Capability_t    Capability;

#if defined(ST_OSLINUX)
    /* Linux remap address */
    unsigned long       *MappingBaseAddress;
    unsigned long       MappingWidth;
#endif

    /* For accessing receive/transmit */
    BLAST_BlastRegs_t       Regs;
    BLAST_BlastRegs_t       *Regs_p;

    /* Pending/current operations */
    BLAST_Operation_t       *Operation;
    BLAST_Operation_t       OperationPool[1];

    /* Symbol buffer */
#if defined(ST_OSLINUX)
    STBLAST_Symbol_t        *SymbolUserBufBase_p; /* Base of User symbol buffer */
    U32                     *CmdUserBufBase_p;  /* Base of user command buffer */
    U32                     SymbolsRead; /* caculate how many symbols read already */
    U32                     CmdsReceived; /* caculate how many commands read already */
    U32                     SymbolsWritten; /* calculate how many symbols were written */
    U32                     CmdsSent; /* caculate how many commands are sent */
#endif
    STBLAST_Symbol_t        *SymbolBufBase_p; /* Base of symbol buffer */
    STBLAST_Symbol_t        *SymbolBuf_p; /* Current symbol */
    U32                      SymbolBufSize; /* Size of symbol buffer */
    U32                      SymbolBufFree; /* Free elements in buffer */
    U32                      SymbolBufCount; /* Used element in buffer */

#if defined(ST_OSLINUX)
    U32                     *CmdBufBase_p; /* Base of command buffer */
    U32                      CmdBufSize; /* Size of command buffer */
#endif

    /* Timer */
    Timer_t                 Timer;

    /* To keep certain operations atomic */
    semaphore_t             *LockSemaphore_p;
    char                    DecodeTaskName[18+ST_MAX_DEVICE_NAME+1];  /*strlen(STBLAST_TimerTask_)=18 and max(strlen (device name)=16 */
    tdesc_t                 TaskDescriptor;
    void                    *Task_p;
    
    /* Each init *might* want to use a separate event handler,
     * so we need to keep the details in the control block */

    /* Each init *might* want to use a separate event handler,
     * so we need to keep the details in the control block */
    STEVT_Handle_t          EvtHandle;
    STEVT_EventID_t         Events[STBLAST_NUM_EVENTS];

    STBLAST_Protocol_t      Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;

    U32                     MaxSymTime;
    U32                     SubCarrierFreq;
    U32                     ActualClockInKHz;
    U8                      DutyCycle;
    BOOL                    InputActiveLow;
    U32                     Delay;

    task_t                  *ReceiveTask;
    semaphore_t             *ReceiveSem_p;
    BOOL                    ReceiveTaskTerm;
    /* flag to indicate whether SetProtocol has been called */
    BOOL                    ProtocolIsSet;
    STBLAST_Protocol_t      ProtocolSet;
    U32                     TimeoutValue;

    /* flag to indicate the last read/send mode */
#if defined(ST_OSLINUX)
    BOOL                    LastRawMode;
#endif

    /* Copies of the start/stop symbol sequence given */
    STBLAST_Symbol_t        StartSymbols[STBLAST_MAX_START_SYMBOLS];
    STBLAST_Symbol_t        StopSymbols[STBLAST_MAX_STOP_SYMBOLS];
    STOS_Clock_t            LastCommandTime;
    STOS_Clock_t            PresentCommandTime;
    STOS_Clock_t            TimeDiff;
    message_queue_t        *InterruptQueue_p;
    BOOL                    DecodeKey;
    struct BLAST_ControlBlock_s *Next_p;
};

typedef struct BLAST_ControlBlock_s BLAST_ControlBlock_t;
typedef struct BlastInterruptData_s
{
    ST_DeviceName_t    DeviceName; 			    /* identiy the device for this*/
    U32                TimeDiff;   		     	/* difference b/w this interrupt and previous interrupt */
    STBLAST_Symbol_t   MessageSymbolBuffer[40]; /* assuming that there can not be more then 40 symbols 
    											   in any command */
    U32                MessageSymbolCount;
} BlastInterruptData_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* Interrupt service routine */
#if defined(ST_OSLINUX)
irqreturn_t BLAST_InterruptHandler(int irq, void* dev_id, struct pt_regs* regs);
#else
#ifdef ST_OS21
int  BLAST_InterruptHandler(BLAST_ControlBlock_t **Blaster_p);
#else
void BLAST_InterruptHandler(BLAST_ControlBlock_t **Blaster_p);
#endif
#endif /* ST_OSLINUX && LINUX_FULL_KERNEL_VERSION */

BLAST_ControlBlock_t *
   BLAST_GetControlBlockFromName(const ST_DeviceName_t DeviceName);
BLAST_ControlBlock_t
    *BLAST_GetControlBlockFromHandle(STBLAST_Handle_t Handle);
    
/* Register map function */
void BLAST_MapRegisters(BLAST_ControlBlock_t *Blaster_p);

/* Apply to device current duty cycle setting */
void BLAST_SetDutyCycle(BLAST_ControlBlock_t *Blaster_p, U8 DutyCycle);

U32 BLAST_InitSubCarrier(BLAST_BlastRegs_t *BlastRegs_p,
                         U32 Clock,
                         U32 SubCarrierFreq
                        );

/* Encode/Decode routines */
ST_ErrorCode_t BLAST_DoEncode(const U32                  *UserBuf_p,
                              const U32                  UserBufSize,
                              STBLAST_Symbol_t           *SymbolBuf_p,
                              const U32                  SymbolBufSize,
                              U32                        *SymbolsEncoded_p,
                              const STBLAST_Protocol_t   Protocol,
                              const STBLAST_ProtocolParams_t *ProtocolParams_p);

ST_ErrorCode_t BLAST_DoDecode(BLAST_ControlBlock_t *Blaster_p);


#endif /* #ifndef __BLASTDRV_H */
