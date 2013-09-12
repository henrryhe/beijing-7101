/*****************************************************************************
File Name   : stttxdef.h

Description : Definitions for the Teletext driver.

Copyright (C) 2004 STMicroelectronics

References  :

$ClearCase (VOB: stttx)

ST API Definition "Teletext Driver API" DVD-API-003 Revision 3.4
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STTTXDEF_H
#define __STTTXDEF_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stevt.h"
#include "stttx.h"

#if !defined(STTTX_USE_DENC)
#include "stvbi.h"
#else
#include "stdenc.h"
#endif

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
#include "pti.h"
#elif defined(DVD_TRANSPORT_DEMUX)
#include "stdemux.h"
#else
#include "stpti.h"
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#include "stvmix.h"
#endif

#if defined(STTTX_SUBT_SYNC_ENABLE)
#include "stclkrv.h"
#endif

/* Private Constants ------------------------------------------------------ */

/* The amount of memory to be used for the circular buffer */
#define CIRCULAR_BUFFER_SIZE    8192

/* The number of message slots in the queue. This and the data size
 * below will define the amount of memory taken by a queue. (Calculated
 * with the MESSAGE_MEMSIZE_QUEUE macro, toolset supplied.)
 */
#define MAX_MESSAGES            10
/* Able to hold 2048bytes of PES data in a single message */
#define MESSAGE_DATA_SIZE       2048

#define PES_DATA_START_POS      46                  /* skip over header line */
#define VBI_SLOT                0                   /* only  VBI slot (legacy)*/
#define STB_SLOT                0                   /* first STB slot */
#define MAX_VBI_OBJECTS         1
#define OPEN_HANDLE_WID         8                   /* Open bits width */
#define OPEN_HANDLE_MSK         ( ( 1 << OPEN_HANDLE_WID ) - 1)
#define BASE_HANDLE_MSK         ( ~OPEN_HANDLE_MSK )/* to strip off Open index */
#define MIN_STB_OBJECTS         0
#define MAX_STB_OBJECTS         ( ( 1 << OPEN_HANDLE_WID ) - MAX_VBI_OBJECTS )
#define VALID_LINES_BIT         1                   /* shifted left by line no. */
#define INVALID_PID             0x2000
#define PTS_DTS_FLAG_02         0x02                /* flags indicating ... */
#define PTS_DTS_FLAG_03         0x03                /*  ...PTS_DTS data present */
#define LINE_NUMB_MASK          0x1F                /* line number mask */
#define ODD_FIELD_FLAG          0x20                /* on line number byte */
#define PRIVATE_STREAM_1        0xBD
#define LO_NYBBLE_MASK          0x0F
#define HO_NYBBLE_MASK          0xF0
#define BAD_INV_HAMMING         0xFF
#define MAGAZINE_MASK           0x07
#define SUBCODE2_MASK           0x07
#define SUBCODE4_MASK           0x03
#define MAGAZINE_FILTER         ( STTTX_MAG_PAGE_SUB | STTTX_MAG_AND_PAGE )
#define NYBBLE_SHIFT_CNT        4
#define STREAM_ID_POS           3                   /* PES header offsets */
#define PCKT_LEN_MSB_POS        4
#define PCKT_LEN_LSB_POS        5
#define PCKT_HDR_LENGTH         6
#define LINE_FIELD_POS          2                   /* TTX line offsets */
#define PACKET_ADDR_POS1        4                   /* mag/1 bit line, hammed */
#define PACKET_ADDR_POS2        5                   /* 4 bits of line, hammed */
#define PAGE_NO_UNIT_POS        6                   /* LO hex nybble, hammed */
#define PAGE_NO_TENS_POS        7                   /* HO hex nybble, hammed */
#define SUBPAGE_UNIT_POS        8                   /* LO hex nybble, hammed */
#define SUBPAGE_TENS_POS        9
#define SUBPAGE_HUND_POS        10
#define SUBPAGE_THOU_POS        11                  /* HO hex nybble, hammed */
#define REQ_SLOT_UNUSED         0xFFFFFFFF          /* invalid OpenIndex */
#define TASK_EXIT               ((U32)-1)
#define DEFAULT_DATA_IDENTIFIER 0x10

#define VALID_LINES_OFFSET      7                   /* The first possible
                            valid TTX line in packet */
#define MAG_TRANS_MODE_POS      13                  /* Position of the contol byte/bit containig */
#define MAG_TRANS_MODE_BIT      0x02            /* transmission mode. (serial or parallel) */
#define SERIAL_MODE_TRANS_BIT   1

#if defined(DVD_TRANSPORT_PTI)
#define BUFF_ALIGN_MASK         15                  /* PTI DMA requirement */
#elif defined(DVD_TRANSPORT_LINK)
#define BUFF_ALIGN_MASK         255
#else
#define BUFF_ALIGN_MASK         0
#endif

#ifndef POOLING_TASK_DIVIDER
#define POOLING_TASK_DIVIDER                1000
#endif

/* Task priorities */
#ifndef STTTX_HANDLER_TASK_PRIORITY
#define STTTX_HANDLER_TASK_PRIORITY         12/*cofigured through compilation flags*/
#endif

#ifndef STTTX_POOLING_TASK_PRIORITY
#define STTTX_POOLING_TASK_PRIORITY         15 /*cofigured through compilation flags*/
#endif

/* Sti5100 has this problem frequently, rarely seen on 5516 */
#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
#ifndef STTTX_TIMEOUT_TASK_PRIORITY
#define STTTX_TIMEOUT_TASK_PRIORITY        MAX_USER_PRIORITY /*cofigured through compilation flags*/
#endif
#endif


/* Task stack sizes */

#ifndef STTTX_TTX_STACK_DEPTH
/* Make this smaller */
#ifdef ST_OS20
#define STTTX_TTX_STACK_DEPTH           ( 16384 )
#else
#define STTTX_TTX_STACK_DEPTH           ( 16384 ) +  ( 16384 )
#endif
#endif

#ifndef STTTX_VBI_STACK_DEPTH
#ifdef ST_OS20
#define STTTX_VBI_STACK_DEPTH           ( 4096 )
#else
#define STTTX_VBI_STACK_DEPTH           ( 4096 ) +  ( 16384 )
#endif
#endif

#ifndef STTTX_STB_STACK_DEPTH
#ifdef ST_OS20
#define STTTX_STB_STACK_DEPTH           ( 4096 )
#else
#define STTTX_STB_STACK_DEPTH           ( 4096 ) + ( 16384 )
#endif

#endif

/* Used for comparing PES mode to something that's more readable */
#define PES_NORMAL_MODE     0
#define PES_CAPTURE_MODE    1
#define PES_INSERT_MODE     2

#define MAX_NUMBER_OF_PES_PACKET_IN_LINEAR_BUFFER 20

#define MAX_VPS_DATA    255

/* Private Types ---------------------------------------------------------- */
#if defined (ST_7020)
#define ST7020_DENC_BASE_ADDRESS  STI7020_BASE_ADDRESS+ST7020_DENC_OFFSET
#endif

#if !defined (ST_7710) && !defined (ST_5105) && !defined (ST_5188) && !defined(ST_5107)
#pragma ST_device( device_access_t )
#endif
typedef volatile U32 device_access_t;

typedef struct Handled_Request_s
{
    U32             OpenOff;     /* illegal offset if unused */
    U32             OddOffset;   /* offset from users memory loc to store next filtered ...*/
    U32             EvenOffset;  /* packet.  Will be zero unless filtering for whole page  */
    BOOL            HeaderFound; /* To check whether page header is found or not */
    STTTX_Request_t Request;
} Handled_Request_t;

/* The following construct is dimensioned for the number of STB objects
   advised in InitParams plus one (for VBI insertion).  All buffers that
   use DMA (PTI/TTX circular, TTX linear & TTX pages) MUST be located
   within the ncache_partition.  Pointers are provided below for the
   for these arrays in ncache.                                           */

typedef struct ttx_Message_s
{
    U32     BytesWritten;
    BOOL    FailedPacket;
    U8      Data[MESSAGE_DATA_SIZE];
} ttx_Message_t;

typedef struct data_store_s
{
    /* Not required unless using legacy PTI */
    U8              *LinearBuff;
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    U8              *CircleBuff;                    /* physical base */
    U8              *CircleBase;                    /* access base, aligned */
    U32             CircleSize;                     /* usable size */
    U8              *CircleProd;                    /* PTI write pointer */
    U8              *CircleCons;                    /* TTX read pointer */
    slot_t          Slot;
#endif
    U32             Object;                         /* Need by STB output */
    U32             PIDNum;
    BOOL            Open;                           /* to prevent VBI > 1 */
    BOOL            Playing;
    BOOL            HandlerRunning;
    U32             LastEvent;
    U8              DataIdentifier;

#ifdef ST_OS20
    semaphore_t     WaitForPES;                     /* signalled from ptiint.c */
    semaphore_t     WakeUp;                         /* stop sleeping, start doing */
    semaphore_t     PoolingWakeUp;
    semaphore_t     Stopping;
#endif
    semaphore_t     *WaitForPES_p;
    semaphore_t     *WakeUp_p;
    semaphore_t     *PoolingWakeUp_p;
    semaphore_t     *Stopping_p;

    BOOL            TasksActive;                    /* while loop task control */
    tdesc_t         TTXtdesc;
    void            *TTXStack_p;
    task_t          *TTXttask_p;

    /* U8           *LinearBase; */
    U8              *RxLinearBuff;
    U32             LinearSize;

    tdesc_t         TTXPooltdesc;
    void            *TTXPoolStack_p;
    task_t          *TTXPoolttask_p;
    
    semaphore_t     *PESArrived_p;
    message_queue_t *PESQueue_p;

    U8              TmpLinearBuff[MESSAGE_DATA_SIZE];

    STTTX_SourceParams_t    SourceParams;           /* data source */
    STTTX_ObjectType_t  ObjectType;                 /* VBI or STB */
    STTTX_ObjectType_t  Mode;

    /* Used for notifying users of a VPS packet, to avoid passing
     * internal references out.
     */
    U8              VpsDataBuffer[MAX_VPS_DATA];

    void            *Element;
#if defined(DVD_TRANSPORT_STPTI)
    STPTI_Buffer_t  STPTI_Buffer;
#elif defined(DVD_TRANSPORT_DEMUX)
    STDEMUX_Buffer_t STDEMUX_Buffer;
#endif
} data_store_t;

/* The following structure is used for passing messages to the STB/VBI
   message queues.  It defines odd/even page contents and a set of
   additional control flags.
*/

typedef struct page_store_s
{
    STTTX_Page_t    OddPage;                        /* Field1 Lines */
    STTTX_Page_t    EvenPage;                       /* Field2 Lines */
    U32             Flags;                          /* Control flags */
} page_store_t;

/* The following structure is dynamically allocated during Initialization for
   each invocation/PTI.  BaseHand supports up to MAX_OPN_OBJECTS Open calls
   per PTI (the memory location is shifted OPEN_HANDLE_WID bits left to make
   a unique tag value; the lower order bits freed are used to count subsequent
   Open/PID calls).  Only memory for the NumSTBObjects specified during Init
   will be actually allocated.  Array element data_store_t subscript 0 is for
   the VBI, with higher elements for STB objects.  This extends the structure
   beyond the typedef; it is not known at compile time how many Opens the
   user will require.  'C' performs no subscript checking; this extension is
   safe provided that NumSTBObjects passed in Init is not exceeded by Open
   calls (which is checked for).                                              */

struct context_t
{
    struct context_t    *Next;                      /* ptr. to next (or NULL) */
    ST_DeviceName_t     Name;                       /* Initialization Name */
    STTTX_Handle_t      BaseHandle;                 /* base handle */
    device_access_t     *BaseAddress;               /* teletext hardware base */
    STTTX_InitParams_t  InitParams;                 /* initialization params */
    Handled_Request_t   *RequestsPtr;               /* pointer to Request Queue */
    BOOL                TasksActive;                /* while loop task control */
    BOOL                SuppressVBI;                /* for hidden API call */
    U32                 ObjectsOpen;                /* incremented on Open(s) */
    void (* DMAReport)( U32 FFlag, U32 Start, U32 Count ); /* diagnostic c/b */

    semaphore_t         *ISRComplete_p;
    /* Event handler information */
    STEVT_EventID_t     RegisteredEvents[STTTX_NUMBER_EVENTS];
    STEVT_Handle_t      EVTHandle;
#if defined(STTTX_SUBT_SYNC_ENABLE)
    STCLKRV_Handle_t    CLKRVHandle;
#endif

#if !defined(STTTX_USE_DENC)
    STVBI_Handle_t      VBIHandle;
#else
    STDENC_Handle_t     DENCHandle;
    U8                  TTXLineInsertRegs[5];
    U32                 NumberOfTTXLines;
#endif
#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
    /* Time out task variables */
    BOOL                TimeOutTaskActive;
    tdesc_t             ISRTimeOuttdesc;
    void                *ISRTimeOutStack_p;
    task_t              *ISRTimeOuttask_p;
    semaphore_t         *ISRTimeOutWakeUp_p;
    semaphore_t         *TTXDMATimeOut_p;
#endif

    BOOL                VBIUsed;
    U32                 NumActiveRequests;

#ifdef STTTX_SUBT_SYNC_ENABLE
    U32                 PTSValue;
    U32                 SubtSyncOffset;
    U8                  HugeTimeDiffCount;
    U8                  DecoderSlowCount;
    U8                  DecoderFastCount;
    U8                  NoSTCCount;
#endif
    /* Used for VBI sync */
    BOOL                OddDataReady;
    BOOL                EvenDataReady;
    BOOL                TTXSubTitleData;
#if defined (ST_7020)
    U32                     *TTXAVMEM_Buffer_Address;      /* teletext hardware base */
    STAVMEM_BlockHandle_t   AVMEMBlockHandle;
#endif
#if defined (ST_5105) || defined(ST_5188) || defined(ST_5107)
    STVMIX_Handle_t             VMIXHandle;
    STVMIX_VBIViewPortHandle_t  VBIVPDisplayEven;
    STVMIX_VBIViewPortHandle_t  VBIVPDisplayOdd;
#endif

    data_store_t        Pes[1];                     /* [NumSTBObjects + 1] */
    /* No elements must come here, due to use of Pes array - see init */
};

typedef struct context_t stttx_context_t;

#endif /* #ifndef __STTTXDEF_H */

/* End of stttxdef.h */
