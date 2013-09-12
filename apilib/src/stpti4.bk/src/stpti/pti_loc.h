/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005,2006,2007. All rights reserved.

  File Name: pti_loc.h
Description: shared (local) definitions for stpti

******************************************************************************/

#ifndef __PTI_LOC_H
 #define __PTI_LOC_H


/* Includes ------------------------------------------------------------ */


#include "stpti.h"
#include "osx.h"
#include "stddefs.h"
#include "tcdefs.h" /* HAL Loader data */

#include "memapi/handle.h"
#include "memapi/pti_mem.h"

#ifndef STPTI_NO_INDEX_SUPPORT
 #include "pti_indx.h"
#endif

#ifdef STPTI_CAROUSEL_SUPPORT
 #include "pti_car.h"
#endif

#ifdef STPTI_PCP_SUPPORT
 #include "stpcp.h"
#endif

#define VIRTUAL_PTI_RAMCAM_SUPPORT
/* Exported Constants -------------------------------------------------- */
#if defined(ST_OSLINUX)
#define TASK_QUIT_PATTERN   0xfffffefe
#endif

#define UNININITIALISED_VALUE    (0XFFFFFFFF)

#define SESSION_NOT_ALLOCATED 0xDEAD

#define PTI_LOC_NUMBER_OF_EVENTS (STPTI_EVENT_END - STPTI_EVENT_BASE)

#define SEM_MUTUAL_EXCLUSION 1
#define SEM_SYNCHRONIZATION  0

#define DTV_TS_PACKET_LENGTH     130
#define DVB_TS_PACKET_LENGTH     188
#define DTV_HEADER_SIZE          3
#define DVB_SECTION_START        6
#define SYNC_BYTE                0x47
#define DTV_SYNC_BYTE_SIZE       2
#define TSMERGER_TAG_SIZE        6
#define DMAScratchAreaSize       32
#define DVB_MAXIMUM_PID_VALUE    0x1fff
#define DTV_MAXIMUM_SCID_VALUE   0xfff
#define DVB_PID_BIT_SIZE         (13)
#define DTV_PID_BIT_SIZE         (12)

#define STPTI_MAX_BUFFERS_PER_SLOT    2           /* One normal buffer and One record buffer */


/* In systems in which the host is tied up (context locked) for extended periods of time you may
   need to increase the number of status blocks.  Status blocks are used for communicating stream
   metadata between the tc and the  host. */
#define NO_OF_DTV_STATUS_BLOCKS  200
#define NO_OF_DVB_STATUS_BLOCKS  200 


/* Generic fix to allow customers to supply their own values 
 for queues at build time via cflags instead of hacking this file */

#ifndef EVENT_QUEUE_SIZE
 #define EVENT_QUEUE_SIZE       256
#endif

#ifndef INTERRUPT_QUEUE_SIZE
 #define INTERRUPT_QUEUE_SIZE   256
#endif

#ifndef INDEX_QUEUE_SIZE
 #define INDEX_QUEUE_SIZE       4096    /* some customers use 512 */
#endif

#ifndef SIGNAL_QUEUE_SIZE
 #define SIGNAL_QUEUE_SIZE       32     /* some customers may need 64 */
#endif

/* BUFFER ALIGNMENT definitions */
/* pcpdat.h * DES requires size % 16 == 0, AES requires size % 64 */
#define STPTI_BUFFER_SIZE_MULTIPLE 0x10
/* pcphal.h */
#define STPTI_BUFFER_ALIGN_MULTIPLE 0x20

/* Exported Macros ----------------------------------------------------- */


#define MINIMUM(a, b) (((a) > (b)) ? (b) : (a))
#define MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

/* Exported Types ------------------------------------------------------ */


typedef volatile struct EventData_s
{
    FullHandle_t          Source;
    BOOL                  PacketDumped;
    BOOL                  PacketSignalled;
    STEVT_EventConstant_t Event;
    STPTI_EventData_t     EventData;
} EventData_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Driver_s
{
    MemCtl_t         MemCtl;

    message_queue_t *EventQueue_p;
    task_t          *EventTaskId_p;
    size_t           EventTaskStackSize;
    S32              EventTaskPriority;

    message_queue_t *InterruptQueue_p;
    task_t          *InterruptTaskId_p;
    size_t           InterruptTaskStackSize;
    S32              InterruptTaskPriority;

#ifndef STPTI_NO_INDEX_SUPPORT
    message_queue_t *IndexQueue_p;
    task_t          *IndexTaskId_p;
    size_t           IndexTaskStackSize;
    S32              IndexTaskPriority;
#endif
} Driver_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct TCUserDma_s
{
    BOOL DirectDmaUsed[4];
    U32  DirectDmaCDAddr[4];
    U32  DirectDmaCompleteAddress[4];
} TCUserDma_t;


/* ------------------------------------------------------------------------- */

typedef volatile struct TCPrivateData_s
{
    TCUserDma_t          *TCUserDma_p;

    STPTI_Buffer_t       *BufferHandles_p;
    STPTI_Slot_t         *SlotHandles_p;
    STPTI_Slot_t         *PCRSlotHandles_p;
    STPTI_Filter_t       *SectionFilterHandles_p;
    STPTI_Slot_t         *SectionFilterStatus_p;
    STPTI_Filter_t       *TransportFilterHandles_p;
    STPTI_Filter_t       *PesFilterHandles_p;
    STPTI_Filter_t       *PesStreamIdFilterHandles_p;
    STPTI_Descrambler_t  *DescramblerHandles_p;
    U8                   *SeenStatus_p;
    void                 *InterruptBufferStart_p;
#if defined( ST_OSLINUX )
    size_t               InterruptDMABufferSize;
#if defined( __MODULE__ )
    dma_addr_t           InterruptDMABufferInfo;
#else
    U32                  InterruptDMABufferInfo;
#endif
#endif
#ifndef STPTI_NO_INDEX_SUPPORT
    STPTI_Index_t        *IndexHandles_p;
    U32                  SCDTableAllocation;            /* Start Code Detector Table Allocation BitArray (B0=First SCD Filter, B1=Second SCD Filter...) */
#endif

    STPTI_TCParameters_t  TC_Params;    /* parameters & addresses specific to each loader */
    struct SlotList_s    *SlotList;     /* keep track of slot allocations per session     */
    struct FilterList_s  *FilterList;   /* keep track of filter allocations per session   */
} TCPrivateData_t;


/* ------------------------------------------------------------------------- */
#ifdef ST_OSLINUX
/*
   The driver maintains an array of these structures.
   There for each STPTI EVENT
*/
#define STPTI_OSLINUX_EVT_ARRAY_SIZE STPTI_EVENT_END-STPTI_EVENT_BASE
#define SESSION_EVT_QSIZE 0x10 /* Size of event queue buffer for this device. */

typedef struct STPTI_OSLINUX_evt_struct_s{
    wait_queue_head_t wait_queue;
    unsigned int evt_queue_size;
    EventData_t *evt_event_p;
    unsigned int evt_read_index;
    unsigned int evt_write_index;
    struct STPTI_OSLINUX_evt_struct_s *next_p;
} STPTI_OSLINUX_evt_struct_t;

/* ------------------------------------------------------------------------- */
#endif

typedef volatile struct Device_s
{
    MemCtl_t                       MemCtl;
    FullHandle_t                   Handle;
    ST_DeviceName_t                DeviceName;
    ST_DeviceName_t                EventHandlerName;
#ifdef STPTI_CAROUSEL_SUPPORT
    STPTI_CarouselEntryType_t      EntryType;
    STPTI_Carousel_t               CarouselHandle;
    semaphore_t                   *CarouselSem_p;
    STPTI_TimeStamp_t              CarouselSemTime;
    S32                            CarouselProcessPriority;
    U32                            CarouselBufferCount;
#endif
    STPTI_Slot_t                   WildCardUse;
    STPTI_Slot_t                   NegativePIDUse;
    STEVT_Handle_t                 EventHandle;
    BOOL                           EventHandleValid;
    STEVT_EventID_t                EventID[ PTI_LOC_NUMBER_OF_EVENTS ];
    volatile U32                  *TCDeviceAddress_p;
    STPTI_Device_t                 Architecture;
    STPTI_SupportedTCCodes_t       TCCodes;
    STPTI_DescramblerAssociation_t DescramblerAssociationType;
#ifndef STPTI_NO_INDEX_SUPPORT
    STPTI_IndexAssociation_t       IndexAssociationType;
    BOOL                           IndexEnable;
#endif
    ST_Partition_t                *DriverPartition_p;
    ST_Partition_t                *NonCachedPartition_p;
    S32                            InterruptLevel;
    S32                            InterruptNumber;
    U32                            SyncLock;
    U32                            SyncDrop;
    STPTI_FilterOperatingMode_t    SectionFilterOperatingMode;
    STPTI_Handle_t                 AlternateOutputSet;
    STPTI_AlternateOutputType_t    AlternateOutputType;
    STPTI_AlternateOutputTag_t     AlternateOutputTag;
    BOOL                           DiscardSyncByte;
    TCPrivateData_t                TCPrivateData;

    STPTI_StreamID_t               StreamID;        /* tsmerger tag bytes */
    STPTI_ArrivalTimeSource_t      PacketArrivalTimeSource;

    U16                            Session;         /* index into TCSessionInfo_t */
    U16                            NumberOfSlots;   /* per session */
    BOOL                           DiscardOnCrcError;
    U8                             AlternateOutputLatency;
    U8                             NumberOfSectionFilters;
    BOOL                           IRQHolder;/* Flag to indicate that IRQS installed for this device. */
#ifdef ST_OSLINUX
    /* When initialised there will be one per event.*/
    /* When a task wants to wait for an event it hooks a STPTI_OSLINUX_evt_struct_t
      on to the cooresponding event ID entry. */
    STPTI_OSLINUX_evt_struct_t    *STPTI_Linux_evt_array_p[STPTI_OSLINUX_EVT_ARRAY_SIZE];
#endif
#if defined (STPTI_FRONTEND_HYBRID)
    FullHandle_t                   FrontendListHandle; /* Added for STFE support to store association */
#endif
} Device_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Session_s
{
    MemCtl_t        MemCtl;
    FullHandle_t    Handle;
    ST_Partition_t *DriverPartition_p;
    ST_Partition_t *NonCachedPartition_p;
    FullHandle_t    AllocatedList[OBJECT_TYPE_LIST + 1];
#ifdef ST_OSLINUX
    STPTI_OSLINUX_evt_struct_t *evt_p; /* One per session. Created when opened */
#endif
} Session_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Signal_s
{
    STPTI_Handle_t   OwnerSession;
    STPTI_Signal_t   Handle;
    U32              QueueWaiting;
    message_queue_t *Queue_p;
    STPTI_Buffer_t   BufferWithData;
    FullHandle_t     BufferListHandle;  /* Associations */
} Signal_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Slot_s
{
    STPTI_Handle_t              OwnerSession;
    STPTI_Slot_t                Handle;
    U32                         TC_SlotIdent;
    STPTI_Slot_t                PCRReturnHandle;
    U32                         SFStatusIdent;
    STPTI_SlotType_t            Type;
    STPTI_SlotFlags_t           Flags;
    STPTI_Pid_t                 Pid;
    BOOL                        UseDefaultAltOut;
    STPTI_AlternateOutputType_t AlternateOutputType;
    STPTI_AlternateOutputTag_t  AlternateOutputTag;
    U32                         TC_DMAIdent;
    U32                         CDFifoAddr;
    U32                         Holdoff;
    U32                         Burst;
    U16                         AssociatedFilterMode;
    FullHandle_t                DescramblerListHandle;  /* Associations */
    FullHandle_t                FilterListHandle;
    FullHandle_t                BufferListHandle;
#ifndef STPTI_NO_INDEX_SUPPORT
    FullHandle_t                IndexListHandle;
#endif
} Slot_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Filter_s
{
    STPTI_Handle_t     OwnerSession;
    STPTI_Filter_t     Handle;
    STPTI_FilterType_t Type;
    U32                TC_FilterIdent;
    U8	    	       PESStreamIdFilterData;
    BOOL               Enabled;
    BOOL               DiscardOnCrcError;
    FullHandle_t       MatchFilterListHandle;   /* Match Action */
    FullHandle_t       SlotListHandle;          /* Associations */
} Filter_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Descrambler_s
{
    STPTI_Handle_t          OwnerSession;
    STPTI_Descrambler_t     Handle;
    STPTI_DescramblerType_t Type;
    U32                     TC_DescIdent;
    STPTI_Pid_t             AssociatedPid;
    FullHandle_t            SlotListHandle;     /* Associations */
} Descrambler_t;


/* ------------------------------------------------------------------------- */


typedef volatile struct Buffer_s
{
    STPTI_Handle_t   OwnerSession;
    STPTI_Buffer_t   Handle;
#if defined(ST_OSLINUX)
    void            *MappedStart_p; /* ioremapped buffer address. Used for CPU reads. */
    U32              UserSpaceAddressDiff; /* for converting to user-space mapped address */
#endif
    U8              *Start_p;
    U8              *RealStart_p;
    size_t           ActualSize;
    size_t           RequestedSize;
    STPTI_SlotType_t Type;
    U32              TC_DMAIdent;
    U32              DirectDma;      /*  The DMA engine being used for link to CDFifo ( This is fixed on TC1 by the */
                                     /* address of the CDFifo, on TC it is open to any DMA engine ( 1,2, or 3 )    */
    U32              CDFifoAddr;     /*  The CDfifo the buffer is connected to, if being used for back buffering    */
    U32              Holdoff;        /* DMA parameters for back buffering                                           */
    U32              Burst;          /* ----- ditto ---- */
    message_queue_t *Queue_p;
    U32              BufferPacketCount;
    U32              MultiPacketSize;   /* Top 16 bits used for Buffer Level Signal (if set), Bottom 16 bits for multipacket */
    FullHandle_t     SlotListHandle;    /* Associations */
    FullHandle_t     SignalListHandle;
#ifdef STPTI_CAROUSEL_SUPPORT
    FullHandle_t     CarouselListHandle;
#endif
#if defined(STPTI_GPDMA_SUPPORT)
    STGPDMA_Handle_t STGPDMAHandle;
    STGPDMA_Timing_t STGPDMATimingModel;
    U32              STGPDMAEventLine;
    U32              STGPDMATimeOut;
#endif
#if defined(STPTI_PCP_SUPPORT)
    STPCP_Handle_t STPCPHandle;
    STPCP_EngineMode_t STPCPEngineModeEncrypt;
    /* STPCP_Mode_t STPCPBlockingMode; always STPCP_MODE_BLOCK */
    STPCP_TransferStatus_t STPCPTransferStatus;
#endif
    BOOL             ManualAllocation;
    BOOL             IgnoreOverflow;
    BOOL             IsRecordBuffer;  /* For Watch & Record Support */

    BOOL             EventLevelChange; /* Generate a buffer level change event if TRUE*/
} Buffer_t;

#if defined (STPTI_FRONTEND_HYBRID)
typedef volatile struct Frontend_s
{
    STPTI_Handle_t          OwnerSession;
    STPTI_Frontend_t        Handle;
    STPTI_FrontendType_t    Type;
    STPTI_FrontendParams_t  UserParams;

    /* TSIN related params */
    BOOL                    IsAssociatedWithHW;
    U8 *                    PIDTableStart_p;     /* Pointer to memory aligned for PID table       */
    U8 *                    PIDTableRealStart_p; /* Pointer to memory returned by memory_allocate */
    BOOL                    PIDFilterEnabled;
    U8 *                    RAMStart_p;          /* Pointer to memory aligned for packet DMA transfers */
    U8 *                    RAMRealStart_p;      /* Pointer to memory returned by memory_allocate      */
    U32                     AllocatedNumOfPkts;  /* Actual number in packets of RAM allocated */
    U32                     WildCardPidCount;    /* Counter to decide if we enable the PID filtering of stfe */

     /* Associations */
    FullHandle_t            SessionListHandle;
    FullHandle_t            CloneFrontendHandle;
} Frontend_t;
#endif /* #if defined (ST_7200) */


/* ------------------------------------------------------------------------- */


typedef struct BackendInterruptData_s
{
    FullHandle_t FullHandle;
    U32          InterruptStatus;
} BackendInterruptData_t;


typedef struct BufferInterrupt_s
{
    STPTI_Buffer_t BufferHandle;
    STPTI_Slot_t   SlotHandle;
    U32            DMANumber;
    U32            BufferPacketCount;
} BufferInterrupt_t;


/* Local Error Codes ------------------------------------------------------- */


enum
{
    STPTI_ERROR_INTERNAL_ALL_MESSED_UP = (STPTI_ERROR_END + 1),
    STPTI_ERROR_INTERNAL_NOT_EXPLICITLY_SET,
    STPTI_ERROR_INTERNAL_NOT_WRITTEN,
    STPTI_ERROR_INTERNAL_NO_CAROUSEL_ENTRIES,
    STPTI_ERROR_INTERNAL_NO_MORE_HANDLES,
    STPTI_ERROR_INTERNAL_OBJECT_ALLOCATED,
    STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED,
    STPTI_ERROR_INTERNAL_OBJECT_NOT_FOUND
};


/* Exported Variables -------------------------------------------------- */

#if defined(STPTI_GPDMA_SUPPORT)
extern STGPDMA_DmaTransfer_t GPDMATransferParams;
#endif

/* Exported Function Prototypes ---------------------------------------- */

ST_ErrorCode_t stpti_SkipInvalidSections(FullHandle_t BufferHandle);
ST_ErrorCode_t stpti_SlotQueryPid       (FullHandle_t DeviceHandle, STPTI_Pid_t Pid, STPTI_Slot_t * Slot_p);
ST_ErrorCode_t stpti_NextSlotQueryPid   (FullHandle_t DeviceHandle, STPTI_Pid_t Pid, STPTI_Slot_t * Slot_p);

/* change parameters to pass partition for task_init GNBvd21068*/
ST_ErrorCode_t stpti_InterruptQueueInit(ST_Partition_t *Partition_p);
ST_ErrorCode_t stpti_InterruptQueueTerm(void);

size_t stpti_BufferSizeAdjust(size_t Size);
void *stpti_BufferBaseAdjust(void *Base_p);

#endif  /* __PTI_LOC_H */
