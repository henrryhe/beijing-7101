/******************************************************************************

File Name   : local

Description : Definitions local to STFDMA but used by severl modules within
              STFDMA.

******************************************************************************/

#ifndef  LOCAL_H
#define  LOCAL_H

/* Includes ------------------------------------------------------------ */

#ifdef STFDMA_EMU                       /* For test purposes only */
#define STTBX_Print(x)  printf x
#else
#if defined (ST_OSLINUX)
#include <asm/io.h>
#else
#include "sttbx.h"
#endif
#endif

#include "stddefs.h"                     /* Standard includes */
#include "stdevice.h"
#include "stfdma.h"
#include "stos.h"

/* Exported Constants -------------------------------------------------- */

/* Number max dreq can be present at a given time */
#define MAX_DREQ               100

#if defined(ST_5517) /*==================================================*/

/* CONFIGURATION FOR 5517 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x4000     /* Contains the control word interface */
#define IMEM_OFFSET             0x6000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0x5F88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            6

/* Number of available channel pools */
#define MAX_POOLS               1

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define NODE_PTR_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */

/* Hardware address offsets */
#define SW_ID                   0x4000     /* Block Id */

#define STATUS                  0x4004     /* Interrupt status register */

#define NODE_PTR_BASE           0x4014     /* Channel 0 Node ptr address.
                                            * Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */
#define COUNT_BASE              0x4188     /* Channel 0 bytes transfered count *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#elif defined (ST_7100)

/* CONFIGURATION FOR 7100 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define NODE_PTR_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */

/* Additional-data-region-0's offset & region interval */
#define ADD_DATA_REGION_0       0x9580
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define NODE_PTR_BASE           0x8040     /* Channel 0 Node ptr address.
                                            * Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */
#define COUNT_BASE              0x9188     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */

#elif defined  (ST_7109)

/* CONFIGURATION FOR 7109 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define NODE_PTR_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x9200
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define NODE_PTR_BASE           0x9140      /*Command Stat register*/
											/* Channel 0 Node ptr address.
                                            * Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */
#define COUNT_BASE              0x9408     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#define CURRENT_SADDR           0X940C
#define CURRENT_DADDR           0X9410

#elif defined  (ST_7200)

/* CONFIGURATION FOR 7200 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define NODE_PTR_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x9200
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define NODE_PTR_BASE           0x9140      /*Command Stat register*/
											/* Channel 0 Node ptr address.
                                            * Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */
#define COUNT_BASE              0x9588     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#define CURRENT_SADDR           0X958C
#define CURRENT_DADDR           0X9590

#elif defined (ST_5188)

/* CONFIGURATION FOR 5188 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            8

/* Number of available channel pools */
#define MAX_POOLS               5          /* No spdif pool available on 5188 */

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define NODE_PTR_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x84F0
#define REGION_INTERVAL         0x40
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define NODE_PTR_BASE           0x8030     /* Channel 0 Node ptr address.
                                            * Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */
#define COUNT_BASE              0x8078     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */

#elif defined  (ST_5525)

/* CONFIGURATION FOR 5525 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x8000     /* Contains the control word interface */
#define IMEM_OFFSET             0xC000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0xBF88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS                4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define NODE_PTR_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x9400
#define REGION_INTERVAL         0x80
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x8000     /* Block Id */

#define STATUS                  0xBFD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0xBFD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0xBFD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0xBFDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0xBFC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0xBFC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0xBFC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0xBFCC     /* Enable/Disable commands (READ?WRITE)*/

#define INTR_HOLDOFF            0xBFE0     /* Set command type with holdoff (READ ONLY)*/
#define INTR_SET_HOLDOFF        0xBFE4     /* Assert a command with holdoff (READ/WRITE)*/
#define INTR_CLR_HOLDOFF        0xBFE8     /* Acknowledge a command with holdoff (READ ONLY)*/
#define INTR_MASK_HOLDOFF       0xBFEC     /* Enable/Disable commands with holdoff(READ?WRITE)*/

#define NODE_PTR_BASE           0x9600      /*Command Stat register*/
											/* Channel 0 Node ptr address.
                                            * Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */
#define COUNT_BASE              0x9008     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#define CURRENT_SADDR           0X900C
#define CURRENT_DADDR           0X9010

#else

/* CONFIGURATION FOR 5100, 5101 and 5105 */

/* Memory section offsets from FDMA base address */
#define DMEM_OFFSET             0x4000     /* Contains the control word interface */
#define IMEM_OFFSET             0x6000     /* Contains config data */

/* Register offsets from FMDA base address */
#define ID                      0x00       /* Hardware id */
#define VERSION                 0x04       /* Hardware version number */
#define ENABLE                  0x08       /* Block enable control */
#define CLOCKGATE               0x0C       /* Clock enable control */
#define SYNCREG                 0x5F88     /* STBUS access control */

/* Number of FDMA channels */
#define NUM_CHANNELS            16

/* Number of available channel pools */
#define MAX_POOLS               4

/* Channel register offsets. Used as offsets from certain register addresses.*/
#define NODE_PTR_CHAN_OFFSET    0x04       /* Node ptr channel offset = 4 bytes */
#define COUNT_CHAN_OFFSET       0x40       /* Count register channel offset = 64 bytes */

/* Additional-data-region-0's offset, region interval & region word size */
#define ADD_DATA_REGION_0       0x44F0
#define REGION_INTERVAL         0x40
#define REGION_WORD_SIZE        0x4

/* Hardware address offsets */
#define SW_ID                   0x4000     /* Block Id */

#define STATUS                  0x5FD0     /* Channel interrupt status (READ ONLY)*/
#define STATUS_SET              0x5FD4     /* Generate an interrupt (READ only)*/
#define STATUS_CLR              0x5FD8     /* Acknowledge an interrupt (READ/WRITE)*/
#define STATUS_MASK             0x5FDC     /* Enable/disable interrupts (READ/WRITE)*/

#define INTR                    0x5FC0     /* Set command type (READ ONLY)*/
#define INTR_SET                0x5FC4     /* Assert a command (READ/WRITE)*/
#define INTR_CLR                0x5FC8     /* Acknowledge a command (READ ONLY)*/
#define INTR_MASK               0x5FCC     /* Enable/Disable commands (READ?WRITE)*/

#define NODE_PTR_BASE           0x4030     /* Channel 0 Node ptr address.
                                            * Other channels at (NODE_PTR_BASE + (Channel * NODE_PTR_CHAN_OFFSET)) */
#define COUNT_BASE              0x4078     /* Channel 0 bytes transfered count  *
                                            * Other channels at (COUNT_BASE + (Channel * COUNT_CHAN_OFFSET)) */
#endif

/* Exported Types ------------------------------------------------------ */

/* Internal message queue control messages */
enum
{
    EXIT_MESSAGE,
    INTERRUPT_NODE_COMPLETE,
    INTERRUPT_NODE_COMPLETE_PAUSED,
    INTERRUPT_LIST_COMPLETE,
    INTERRUPT_ERROR_NODE_COMPLETE,
    INTERRUPT_ERROR_NODE_COMPLETE_PAUSED,
    INTERRUPT_ERROR_LIST_COMPLETE,
    INTERRUPT_ERROR_SCLIST_OVERFLOW,
    INTERRUPT_ERROR_BUFFER_OVERFLOW
};

/* Callback task reference data */
typedef struct
{
#if defined (ST_OSLINUX)
    struct task_struct* pTask;
    tdesc_t     TaskDesc;                       /* Task descriptor */
    void        *TaskStack_p;                    /* Pointer to stack for task usage */
#else
    void        *TaskStack_p;                    /* Pointer to stack for task usage */
    tdesc_t     TaskDesc;                       /* Task descriptor */
    task_t      *pTask;
#endif
} TaskData_t;

/* Channel descriptors */
typedef struct
{
    U32                 TransferId;             /* Id of the associated transfer */
    int                 Pool;                   /* Which pool does this channel belong to */
    BOOL                Error;                  /* Set when the transfer completes with the error bit set */
    BOOL                Locked;                 /* Channel locked indicator */
    BOOL                Paused;                 /* Channel paused indicator */
    BOOL                Abort;                  /* Abort of transfer is pendind */
    BOOL                Idle;                   /* A blocking transfer has completed */
    BOOL                Blocking;               /* Transfer is a blocking transfer */
    semaphore_t*        pBlockingSemaphore;     /* Semaphore to wait on when blocking */
    U32                 BlockingReason;         /* Reason for blocking semap signal */
    STFDMA_CallbackFunction_t   CallbackFunction_p; /* Ptr to application callback */
    void                *ApplicationData_p;     /* Application data passed through to callback */
    U32                 NextFreeChannel;        /* Index to next free channel */
#if defined (ST_OSLINUX)
    wait_queue_head_t   blockingQueue;
    U32			        GenericDMA;		        /* If true, indicate the channel is requested by
                      						       Linux driver via a generic DMA driver (Not a Stapi driver) */
#endif
} ChannelElement_t;

/* Message Queue element */
typedef struct
{
    U8  ChannelNumber;
    U8  InterruptType;
    U16 InterruptData;
    STOS_Clock_t InterruptTime;
} MessageElement_t;


/* Main control block */
typedef struct
{
    ST_DeviceName_t             DeviceName;                         /* Name of STFDMA instance */
    STFDMA_Block_t              DeviceNumber;                       /* STFDMA device number */
    U32                         *BaseAddress_p;                     /* Device base address */
    ST_Partition_t              *DriverPartition_p;                 /* General partition ptr */
    ST_Partition_t              *NCachePartition_p;                 /* Non cached parition */
    U32                         FreeChannelIndex[MAX_POOLS];        /* Next available free channel */
    semaphore_t*                pAccessSemaphore;
    ChannelElement_t            ChannelTable[NUM_CHANNELS];         /* Channel data */
    U8                          TransferTotal;                      /* Count of transfers */
    U32                         NumberCallbackTasks;                /* Number of callback tasks */
    TaskData_t                  *CallbackTaskTable_p;               /* Ptr to table of callback task ids */
    U32                         ClockTicksPerSecond;                /* clock ticks per second - used for seamphore_timeout */
    BOOL                        Terminate;                          /* Driver terminating flag */
    U32                         InterruptNumber;                    /* Driver interrupt number */
    U32                         InterruptLevel;                     /* Driver interrupt level */
#if defined (ST_OS21) || defined (ST_OSWINCE)
    interrupt_t*                hInterrupt;
#endif
    BOOL                        DREQStatus[MAX_DREQ];               /* Max Dreqs */

    S32                         ProducerIndex;
    S32                         ConsumerIndex;
    BOOL                        QueueFull;
    BOOL                        QueueEmpty;
    MessageElement_t           *MessageQueue;
    semaphore_t                *MessageQueueSem;
    semaphore_t                *MessageReadySem;
} ControlBlock_t;

#if defined (STFDMA_DEBUG_SUPPORT)
typedef struct STFDMA_Status_s
{
    struct
    {
        U32 InterruptsTotal;
        U32 InterruptsBlocking;
        U32 InterruptsNonBlocking;
        U32 InterruptsMissed;
        U32 InterruptBlockingSemaphoreReceived;
        U32 Callbacks;
    }Channel[NUM_CHANNELS];
}STFDMA_Status_t;
#endif

/* Exported Variables -------------------------------------------------- */

/* Global pointer for control block reference */
extern ControlBlock_t *stfdma_ControlBlock_p[STFDMA_MAX];

/* Exported Macros ----------------------------------------------------- */

#ifdef STFDMA_SIM
/*...for simulator usage only. Uses simulator register access functions */
#define SET_REG(Offset,Value)         FDMASIM_SetReg(Offset, Value)
#define GET_REG(Offset)               FDMASIM_GetReg(Offset)
#define SET_NODE_PTR(Channel, Value)     ( SET_REG((NODE_PTR_BASE + ((Channel) * NODE_PTR_CHAN_OFFSET)), ((U32)Value)) )
#else
#if defined (ST_OSLINUX)
#define SET_REG(Offset, Value, Device)  iowrite32( ((U32)Value), (void*)(((U32)stfdma_ControlBlock_p[Device]->BaseAddress_p) + (Offset)))
#define GET_REG(Offset, Device)         ioread32((void*)(((U32)stfdma_ControlBlock_p[Device]->BaseAddress_p) + (Offset)))
#else
#define SET_REG(Offset, Value, Device)   ( *((volatile U32*)(((U32)stfdma_ControlBlock_p[Device]->BaseAddress_p) + (Offset))) = ((U32)Value) )
#define GET_REG(Offset, Device)          ( *(volatile U32*)(((U32)stfdma_ControlBlock_p[Device]->BaseAddress_p) + (Offset)) )
#endif
#define SET_NODE_PTR(Channel, Value, Device)     ( SET_REG((NODE_PTR_BASE + ((Channel) * NODE_PTR_CHAN_OFFSET)), (Value), Device) )
#endif

#define IS_INITIALISED(Device)                  (stfdma_ControlBlock_p[Device] != NULL)

#define GET_BYTE_COUNT(Channel, Device)          ( GET_REG(COUNT_BASE + ((Channel) * COUNT_CHAN_OFFSET), Device) )

#if defined (ST_5105) || defined (ST_5100)  || defined (ST_5188) || defined (ST_5107)
#define SET_BYTE_COUNT(Channel, n, Device)       ( SET_REG((COUNT_BASE + ((Channel) * COUNT_CHAN_OFFSET)), n, Device))
#endif

#define IS_BLOCKING(Channel, Device)            (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Blocking)
#define SET_BLOCKING(Channel, Device)           (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Blocking = TRUE)
#define CLEAR_BLOCKING(Channel, Device)         (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Blocking = FALSE)

#define GET_BLOCKING_SEM_PTR(Channel, Device)   (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].pBlockingSemaphore)

#define GET_BLOCKING_REASON(Channel, Device)    (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].BlockingReason)
#define SET_BLOCKING_REASON(Channel, Reason, Device)(stfdma_ControlBlock_p[Device]->ChannelTable[Channel].BlockingReason = Reason)

#define IS_ABORTING(Channel, Device)            (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Abort)
#define SET_ABORTING(Channel, Device)           (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Abort = TRUE)
#define CLEAR_ABORTING(Channel, Device)         (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Abort = FALSE)

#define IS_IDLE(Channel, Device)                (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Idle)
#define SET_IDLE(Channel, Device)               (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Idle = TRUE)
#define CLEAR_IDLE(Channel, Device)             (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Idle = FALSE)

#if !defined (ST_OSWINCE) /* Collision - WinError.h */
#define IS_ERROR(Channel, Device)               (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Error)
#endif
#define SET_ERROR(Channel, Device)              (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Error = TRUE)
#define CLEAR_ERROR(Channel, Device)            (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Error = FALSE)

#if defined(ST_5517)
#define GET_NODE_PTR(Channel, Device)     ( GET_REG(NODE_PTR_BASE + ((Channel) * NODE_PTR_CHAN_OFFSET), Device) )
#else
#define GET_NODE_PTR(Channel, Device)     ( GET_REG(NODE_PTR_BASE + ((Channel) * NODE_PTR_CHAN_OFFSET), Device) & ~0X1F )
#define GET_NODE_STATE(Channel, Device)   ( GET_REG(NODE_PTR_BASE + ((Channel) * NODE_PTR_CHAN_OFFSET), Device) &  0X1F )
#endif

/* Exported Function Prototypes ---------------------------------------- */

#if defined (ST_5525) || defined (ST_7200)
void   stfdma_MuxConf(void);
#endif

int  stfdma_FDMA1Conf(void *ProgAddr, void *DataAddr);
int  stfdma_FDMA2Conf(void *ProgAddr, void *DataAddr, STFDMA_Block_t DeviceNo);
void stfdma_InitContexts(U32 *BaseAddress_p, STFDMA_Block_t DeviceNo);
int  stfdma_TermContexts(STFDMA_Block_t DeviceNo);

void stfdma_ReqConf(ControlBlock_t *ControlBlock_p, STFDMA_Block_t DeviceNo);
BOOL stfdma_IsNodePauseEnabled(U32 Channel, STFDMA_Block_t DeviceNo);

STOS_INTERRUPT_DECLARE(stfdma_InterruptHandler, pParams);

U32 stfdma_GetNextNodePtr(U32 Channel, STFDMA_Block_t DeviceNo);

void stfdma_SendMessage(MessageElement_t * Message, STFDMA_Block_t DeviceNo);

#endif

/*eof*/
