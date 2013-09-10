/*****************************************************************************
File Name   : stttx.h

Description : API Interfaces for the Teletext driver.

Copyright (C) 2005 STMicroelectronics

Reference   :

$ClearCase (VOB: stttx)

ST API Definition "Teletext Driver API" DVD-API-003 Revision 3.4
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STTTX_H
#define __STTTX_H

/* Includes --------------------------------------------------------------- */

#include "stos.h"
#include "stcommon.h"

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
#include "pti.h"
#elif defined(DVD_TRANSPORT_DEMUX)
#include "stdemux.h"
#else
#include "stpti.h"
#endif

#include "stavmem.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

#define STTTX_MAX_VBI_OPEN      1    /* maximum no. of VBI Opens */
#define STTTX_MAX_STB_OPEN      255  /* current limit due to partitioning of
                                        Handle bits.  Easily raised within
                                        the driver if PTI constraints permit */

#define STTTX_FREE_REQUEST_TAG  0xFFFFFFFF  /* Use any Tag value EXCEPT this */

#define STTTX_LINESIZE          46

#define STTTX_MAXLINES          32

#define STTTX_DRIVER_ID         3
#define STTTX_DRIVER_BASE       ( STTTX_DRIVER_ID << 16 )

/* (Max.) size of a PES packet */
#define PES_POINTER_SIZE        8192
/* Size of the buffer that the application should allocate, if using
 * STPTI */
#define STTTX_BUFFER_SIZE       3 * PES_POINTER_SIZE

/* Driver specific error constants */

#define STTTX_ERROR_CANT_START          ( STTTX_DRIVER_BASE )
#define STTTX_ERROR_CANT_STOP           ( STTTX_DRIVER_BASE + 1 )
#define STTTX_ERROR_CANT_FILTER         ( STTTX_DRIVER_BASE + 2 )
#define STTTX_ERROR_CANT_CLEAR          ( STTTX_DRIVER_BASE + 3 )
#define STTTX_ERROR_NO_MORE_REQUESTS    ( STTTX_DRIVER_BASE + 4 )
#define STTTX_ERROR_TASK_CREATION       ( STTTX_DRIVER_BASE + 5 )
#define STTTX_ERROR_PID_NOT_SET         ( STTTX_DRIVER_BASE + 6 )
#define STTTX_ERROR_EVT_REGISTER        ( STTTX_DRIVER_BASE + 7 )
#define STTTX_ERROR_DECODER_RUNNING     ( STTTX_DRIVER_BASE + 8 )
#define STTTX_ERROR_STVBI_SETUP         ( STTTX_DRIVER_BASE + 9 )
#define STTTX_ERROR_STDENC_SETUP        ( STTTX_DRIVER_BASE + 10 )
#define STTTX_ERROR_STCLKRV_SETUP       ( STTTX_DRIVER_BASE + 11 )
#define STTTX_ERROR_STVMIX_SETUP        ( STTTX_DRIVER_BASE + 12 )

/* Exported events */

enum
{
    STTTX_EVENT_DEFAULT = STTTX_DRIVER_BASE, /* Internal use only */
    STTTX_EVENT_PES_LOST = STTTX_DRIVER_BASE,
    STTTX_EVENT_PES_NOT_CONSUMED,
    STTTX_EVENT_PES_INVALID,
    STTTX_EVENT_PES_AVAILABLE,
    STTTX_EVENT_PACKET_DISCARDED,
    STTTX_EVENT_PACKET_CONSUMED,
    STTTX_EVENT_DATA_OVERFLOW,
    STTTX_EVENT_VPS_DATA,
    STTTX_EVENT_TIME_DIFFERENCE_TOO_LARGE,
    STTTX_EVENT_DECODER_TOO_SLOW,
    STTTX_EVENT_DECODER_TOO_FAST,
    STTTX_EVENT_NO_STC
};

/* Number of events registered by STTTX */
#define STTTX_NUMBER_EVENTS             12

/* Exported Types --------------------------------------------------------- */

/* Handle type */

typedef U32 STTTX_Handle_t;

/* Filter criteria definition */
typedef enum STTTX_FilterType_e
{
    STTTX_FILTER_ANY = 0,               /* Filter on any teletext packet */
    STTTX_FILTER_MAGAZINE = 1,          /* Filter on specified magazine */
    STTTX_FILTER_PAGE = 2,              /* Filter on specified page */
    STTTX_FILTER_SUBPAGE = 4,           /* Filter on specified subpage */
    STTTX_FILTER_PACKET_0 = 8,          /* Filter on X/0 packet */
    STTTX_FILTER_PACKET_30 = 16,        /* Filter on X/30 packet */
    STTTX_FILTER_PAGE_COMPLETE_ODD = 32,/* Filter on complete page odd field */
    STTTX_FILTER_PAGE_COMPLETE_EVEN= 64 /* Filter on complete page, even */
} STTTX_FilterType_t;

/* Enumeration type of teletext output options */

typedef enum STTTX_ObjectType_e
{
    STTTX_VBI,                          /* each open selects either VBI */
    STTTX_STB,                          /*  or STB output type          */
    STTTX_STB_VBI                       /* Combined output type         */
} STTTX_ObjectType_t;

/* Enumeration type of teletext input options. */
typedef enum STTTX_Source_e
{
    STTTX_SOURCE_PTI_SLOT = 0,          /* are we getting data from pti   */
    STTTX_SOURCE_STPTI_SIGNAL,          /* or from stpti                  */
    STTTX_SOURCE_STDEMUX_SIGNAL,        /* or from stdemux                  */
    STTTX_SOURCE_USER_BUFFER            /* or from the user               */
} STTTX_Source_t;

/* STTTX Device type */
typedef enum STTTX_Device_e
{
    STTTX_DEVICE_OUTPUT,                  
    STTTX_DEVICE_INPUT,                   
    STTTX_DEVICE_TYPE_7020,               /* Teletext insertion using 7020DMA */
    STTTX_DEVICE_TYPE_5105                /* Teletext insertion using GDMA */
}STTTX_Device_t;

/* Initialization parameters */

typedef struct STTTX_InitParams_e
{
    STTTX_Device_t          DeviceType;
    ST_Partition_t          *DriverPartition;
    ST_DeviceName_t         EVTDeviceName;
    ST_DeviceName_t         VBIDeviceName;
    ST_DeviceName_t         DENCDeviceName;
    U32                     *BaseAddress;
#ifndef ST_OS21    
    U32                     InterruptNumber;
#else
    interrupt_name_t        InterruptNumber;
#endif 
    U32                     InterruptLevel;
    U32                     NumVBIObjects;
    U32                     NumVBIBuffers;
    U32                     NumSTBObjects;
    U32                     NumRequestsAllowed;
#ifdef STTTX_SUBT_SYNC_ENABLE    
    ST_DeviceName_t         CLKRVDeviceName;
#endif
    ST_DeviceName_t         VMIXDeviceName;
    STAVMEM_PartitionHandle_t AVMEMPartition;
} STTTX_InitParams_t;

/* Termination parameters */

typedef struct STTTX_TermParams_s
{
    BOOL                    ForceTerminate;
} STTTX_TermParams_t;

/* Open parameters */

typedef struct STTTX_OpenParams_s
{
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
    slot_t                  Slot;       /* associated pti slot */
#endif
    STTTX_ObjectType_t      Type;       /* output destination */
} STTTX_OpenParams_t;

/* Source parameters */

typedef struct STTTX_SourceParams_s
{
    STTTX_Source_t DataSource;          /* where the data's coming from */

    union                               /* details for the sources */
    {
#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
        struct
        {
            slot_t Slot;
        } PTISlot_s;
#elif defined(DVD_TRANSPORT_DEMUX)
        struct
        {
            STDEMUX_Signal_t Signal;
            STDEMUX_Buffer_t Buffer;
        } STDEMUXSignal_s;
#else
        struct
        {
            STPTI_Signal_t Signal;
            STPTI_Buffer_t Buffer;
        } STPTISignal_s;
#endif
        struct
        {
            semaphore_t *DataReady_p;
            U8 **PesBuf_p;
            U32 *BufferSize;
        } UserBuf_s;
    } Source_u;
} STTTX_SourceParams_t;

/* Teletext page definition */

typedef struct STTTX_Page_s
{
    U32 ValidLines;                     /* bit-field lines  0..31 */
    U8  Lines[STTTX_MAXLINES][STTTX_LINESIZE];
} STTTX_Page_t;

/* Filter request type */

typedef struct STTTX_Request_s
{
    U32                     RequestTag;  /* Unique Request Identifier */
    STTTX_FilterType_t      RequestType; /* Filter flags */
    U32                     MagazineNumber;
    U32                     PageNumber;
    U32                     PageSubCode;
    U32                     PacketMask; /* type of packet required */
    STTTX_Page_t            *OddPage;   /* odd page assembly struc */
    STTTX_Page_t            *EvenPage;  /* even page assembly struc */
    void (* NotifyFunction)(U32 Tag,
                            STTTX_Page_t *OddPage,
                            STTTX_Page_t *EvenPage );
} STTTX_Request_t;

/* Structure for notifying events; currently only used for VPS_DATA
 * event. Planned to be used for all events at some point in the future.
 */
typedef struct STTTX_EventData_s
{
    /* The handle that generated the event */
    STTTX_Handle_t      Handle;
    /* How much data is there... */
    U32                 DataLength;
    /* Any event-specific data to be carried */
    void                *Data;
} STTTX_EventData_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t STTTX_ClearRequest( STTTX_Handle_t Handle,
                                   U32         RequestTag );
ST_ErrorCode_t STTTX_Close( STTTX_Handle_t Handle );
ST_ErrorCode_t STTTX_GetDataIdentifier( STTTX_Handle_t  Handle,
                                        U8              *DataIdentifier );
ST_Revision_t STTTX_GetRevision( void );
ST_ErrorCode_t STTTX_GetStreamID( STTTX_Handle_t  Handle,
                                  U32             *ProgramID );
ST_ErrorCode_t STTTX_Init( const ST_DeviceName_t DeviceName,
                           const STTTX_InitParams_t    *InitParams );
ST_ErrorCode_t STTTX_Open( const ST_DeviceName_t    DeviceName,
                           const STTTX_OpenParams_t *OpenParams,
                           STTTX_Handle_t           *Handle      );
ST_ErrorCode_t STTTX_SetDataIdentifier( STTTX_Handle_t  Handle,
                                        U8              DataIdentifier );
ST_ErrorCode_t STTTX_SetRequest( STTTX_Handle_t  Handle,
                                 STTTX_Request_t *Request );
ST_ErrorCode_t STTTX_SetSource( STTTX_Handle_t          Handle,
                                STTTX_SourceParams_t    *SourceParams_p);
ST_ErrorCode_t STTTX_SetStreamID( STTTX_Handle_t  Handle,
                                  U32             ProgramID );
ST_ErrorCode_t STTTX_ModeChange( STTTX_Handle_t     Handle,
                                  STTTX_ObjectType_t Mode );
#ifdef STTTX_SUBT_SYNC_ENABLE                                    
ST_ErrorCode_t STTTX_SetSyncOffset( STTTX_Handle_t     Handle,
                                    U32               SyncOffset );
#endif
ST_ErrorCode_t STTTX_Start( STTTX_Handle_t Handle );
ST_ErrorCode_t STTTX_Stop( STTTX_Handle_t Handle );
ST_ErrorCode_t STTTX_Term( const ST_DeviceName_t DeviceName,
                           const STTTX_TermParams_t *TermParams );

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STTTX_H */

/* End of stttx.h */
