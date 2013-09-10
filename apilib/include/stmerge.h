/*****************************************************************************
File Name   : stmerge.h

Description : API Interface  for the STMERGE driver.

History     :

Copyright (C) 2005 STMicroelectronics

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STMERGE_H
#define __STMERGE_H

/* Includes --------------------------------------------------------------- */
#include "stos.h"
#include "stddefs.h"         /* Standard definitions */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Driver revision value */
/*This is precisely this long:".............................."*/

#define STMERGE_REVISION      "STMERGE-REL_2.2.1          "

/* Driver constants, required by the driver model */
#define STMERGE_DRIVER_ID           (0x0CAF)      /* OBTAIN REAL NUMBR! */
#define STMERGE_DRIVER_BASE         (STMERGE_DRIVER_ID << 16)

/* Driver to use default  RAM memory map */
#define STMERGE_DEFAULT_MERGE_MEMORY_MAP         (0)

/* _NO_DEST permits routing of the Source to be turned off. */
#define STMERGE_NO_DEST   (0/*0xFF0FF0FF*/)
#define STMERGE_NO_SRC    (0xFFFFFFFF)


/* Source configuration defines for parameters in STMERGE_SourceConfig_t */

#define STMERGE_PACE_AUTO            (0)

#define STMERGE_COUNTER_AUTO_LOAD    (0)

#define STMERGE_COUNTER_NO_INC       (0)

#define STMERGE_1394_INPUT_CLK       (0)

/* Error constants returned by function calls */
enum
{
    STMERGE_ERROR_NOT_INITIALIZED = STMERGE_DRIVER_BASE,
    STMERGE_ERROR_DEVICE_NOT_SUPPORTED,
    STMERGE_ERROR_BYPASS_MODE,
    STMERGE_ERROR_INVALID_BYTEALIGN,
    STMERGE_ERROR_ILLEGAL_CONNECTION,
    STMERGE_ERROR_INVALID_COUNTER_RATE,
    STMERGE_ERROR_INVALID_COUNTER_VALUE,
    STMERGE_ERROR_INVALID_DESTINATION_ID,
    STMERGE_ERROR_INVALID_ID,
    STMERGE_ERROR_INVALID_LENGTH,
    STMERGE_ERROR_INVALID_MODE,
    STMERGE_ERROR_INVALID_PACE,
    STMERGE_ERROR_INVALID_PARALLEL,
    STMERGE_ERROR_INVALID_PRIORITY,
    STMERGE_ERROR_INVALID_SOURCE_ID,
    STMERGE_ERROR_INVALID_SYNCDROP,
    STMERGE_ERROR_INVALID_SYNCLOCK,
    STMERGE_ERROR_INVALID_TAGGING,
    STMERGE_ERROR_INVALID_TRIGGER, /* from 5100 onwards */
    STMERGE_ERROR_MERGE_MEMORY,
    STMERGE_ERROR_PARAMETERS_NOT_SET,
    STMERGE_ERROR_FEATURE_NOT_SUPPORTED
    /* Error codes for each configurable parameter, rather than just BAD_PARAMETER. */
};


/* An TSMERGE object Id value is U32 of (TYPE | INSTANCE).
 * Used in all communication with driver except Init/Term.
 * Values provided can be used or Id value can be obtained using macro provided.
 */

/* ObjectId types */
enum
{
    STMERGE_IDTYPE_TSIN = (0x10),
    STMERGE_IDTYPE_SWTS,
    STMERGE_IDTYPE_ALTOUT,
    STMERGE_IDTYPE_1394IN,
    STMERGE_IDTYPE_PTI/*,*/
    /*STMERGE_IDTYPE_1394OUT*/
};

/* ObjectId instance ID values */

/* TSIN Sources */
enum
{
    STMERGE_TSIN_0 = (STMERGE_IDTYPE_TSIN << 16),
    STMERGE_TSIN_1,
    STMERGE_TSIN_2,
    STMERGE_TSIN_3,
    STMERGE_TSIN_4
};

/* SWTS SOURCEs */
enum
{
    STMERGE_SWTS_0 = (STMERGE_IDTYPE_SWTS << 16),
    STMERGE_SWTS_1,
    STMERGE_SWTS_2,
    STMERGE_SWTS_3
};

/* ALT_OUT streams */
enum
{
    STMERGE_ALTOUT_0 = (STMERGE_IDTYPE_ALTOUT << 16)
};

/* 1394 streams */
enum
{
     STMERGE_1394IN_0 = (STMERGE_IDTYPE_1394IN << 16)
};


/* PTI Destinations. */
enum
{
     STMERGE_PTI_0     = (STMERGE_IDTYPE_PTI << 16)|1,
     STMERGE_PTI_1     = (STMERGE_IDTYPE_PTI << 16)|2,
     STMERGE_1394OUT_0 = (STMERGE_IDTYPE_PTI << 16)|4
};

#if 0
/* 1394 Destinations */
enum
{
    STMERGE_1394OUT_0 = (STMERGE_IDTYPE_1394OUT << 16)
};
#endif

/* Exported Macros -------------------------------------------------------- */

/* ObjectId creation macro.
 * Dumb macro for creating an ObjectId value from values provided.
 * May not create a valid Id, depends on values provided.
 */
#define STMERGE_GetObjectId(Type, Instance) ((Type << 16) | Instance)

#define GetInstanceFromId(Type, ID) (ID - Type)


/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */


/* Supported TMERGE interface device types for STMERGE_Init()  */
typedef enum STMERGE_Device_e
{
    STMERGE_DEVICE_1
} STMERGE_Device_t;


/* Tagging control values for Source configuration */
typedef enum STMERGE_TagControl_e
{
    STMERGE_TAG_NO_CHANGE,
    STMERGE_TAG_ADD,
    STMERGE_TAG_REMOVE,
    STMERGE_TAG_REPLACE_ID
}STMERGE_Tagging_t;


/* For bringing block up in bypass mode */
typedef enum STMERGE_Mode_e
{
    STMERGE_NORMAL_OPERATION,
    STMERGE_BYPASS_TSIN_TO_PTI_ONLY,
    STMERGE_BYPASS_SWTS_TO_PTI_ONLY,
    STMERGE_BYPASS_GENERIC
}STMERGE_Mode_t;


/* For setting priorities of stream */
typedef enum STMERGE_Priority_e
{
    STMERGE_PRIORITY_LOWEST,
    STMERGE_PRIORITY_LOW,
    STMERGE_PRIORITY_MID,
    STMERGE_PRIORITY_HIGH,
    STMERGE_PRIORITY_HIGHEST
}STMERGE_Priority_t;

/* For setting Packet length */
typedef enum STMERGE_PacketLength_e
{
#if defined(STMERGE_DTV_PACKET)
    STMERGE_PACKET_LENGTH_DTV = 130,
    STMERGE_PACKET_LENGTH_DTV_WITH_1B_STUFFING,
    STMERGE_PACKET_LENGTH_DTV_WITH_2B_STUFFING,
#endif
    STMERGE_PACKET_LENGTH_DVB = 188
}   STMERGE_PacketLength_t;


/* ObjectId type for communication with configurable elements. */
typedef U32 STMERGE_ObjectId_t;


/* TSMERGE device Init parameters for STMERGE_Init() */
typedef struct STMERGE_InitParams_s
{
    STMERGE_Device_t    DeviceType;         /* TSMERGE device */
    ST_Partition_t      *DriverPartition_p; /* Mem partion for driver use */
    U32                 *BaseAddress_p;     /* Base address of TSMERGER block */
    void                *MergeMemoryMap_p;  /* Source RAM usage. Expect SourceId and memory multiple of 64 */
#if defined(STMERGE_INTERRUPT_SUPPORT)
#ifdef ST_OS21
    interrupt_name_t    InterruptNumber;
#else
    U32                 InterruptNumber;
#endif
    U32                 InterruptLevel;
#endif
    STMERGE_Mode_t      Mode;               /* Operation mode of block */
} STMERGE_InitParams_t;


/* Counter configuration */
typedef struct STMERGE_Counter_s
{
    U32         Value;          /* The value to load to counter. 0 == Auto */
    U8          Rate;           /* Counter increment rate for each tick */
} STMERGE_Counter_t;


/* Source specific configuration parameter types */
typedef struct STMERGE_ConfigTSIN_s
{
    BOOL        SerialNotParallel;  /* Serial/parallel control */
    BOOL        SyncNotAsync;       /* Synchronous/Asynchronous control */
    BOOL        InvertByteClk;      /* Byte clock invert control */
    BOOL        ByteAlignSOPSymbol; /* Serial only. Wait for SOP byte alignment */
    BOOL        ReplaceSOPSymbol;   /* Start of packet symbol replace flag */
} STMERGE_ConfigTSIN_t;


typedef struct STMERGE_ConfigSWTS_s
{
    STMERGE_Tagging_t   Tagging;        /* Tagging control */
    U32                 Pace;           /* If not auto, this value controls streamrate */
    STMERGE_Counter_t   Counter;        /* Intial counter configuration */
#if !defined(ST_5528)
    U32                 Trigger;
#endif
} STMERGE_ConfigSWTS_t;


typedef struct STMERGE_ConfigALTOUT_s
{
    STMERGE_ObjectId_t  PTISource;   /* The ALT OUT source (PTI0, PTI1) */
    U32                 Pace;        /* If not auto, this value controls streamrate */
    STMERGE_Counter_t   Counter;     /* Intial counter configuration */
} STMERGE_ConfigALTOUT_t;


typedef struct STMERGE_Config1394_s
{
    BOOL                P1394ClkSrc;    /* Either Input clk src or 1394 clk src */
    U32                 Pace;           /* If !0 divide ratio is applied else input clock used */
    STMERGE_Tagging_t   Tagging;        /* Tagging control */
} STMERGE_Config1394_t;


/* Source configuration parameters */
typedef struct STMERGE_ObjectConfig_s
{
    /* Common configuration parameters */
    STMERGE_Priority_t  Priority;       /* Priority of the Source based on data rate */
    U8                  SyncLock;       /* Number of SOP symbols to be found before concluding lock */
    U8                  SyncDrop;       /* Number of SOP symbols missing before concluding no lock */
    U8                  SOPSymbol;      /* Start of packet symbol */
    STMERGE_PacketLength_t   PacketLength;   /* Number of bytes in the packet */
#if defined(ST_7100) || defined(ST_7109)
    BOOL                 Channel_rst;
    BOOL                 Disable_mid_pkt_err;
    BOOL                 Disable_start_pkt_err;
    U8                   Short_pkt_count;
#endif
#if defined(ST_5525) || defined(ST_5524)
    BOOL                 Counter_num;
#endif
    union    /* Non common Source configuration parameters */
    {
        STMERGE_ConfigTSIN_t      TSIN;        /* TSIN specific parameters */
        STMERGE_ConfigSWTS_t      SWTS;        /* SWTS specific parameters */
        STMERGE_ConfigALTOUT_t    ALTOUT;      /* ALTOUT specific parameters */
        STMERGE_Config1394_t      P1394;       /* 1394 interface specific parameters */
    }u;
}STMERGE_ObjectConfig_t;

/* Source status */
typedef struct STMERGE_Status_s
{
    BOOL                Lock;           /* Source locked */
    BOOL                InputOverflow;  /* Input FIFO has overflowed */
    BOOL                RAMOverflow;    /* RAM overflowed */
    U32                 ErrorPackets;   /* Number of errorneous packets thrown away */
    U32                 Counter;        /* The counter value in ALTOUT or SWTS Sources header */
    STMERGE_ObjectId_t  DestinationId;  /* DestinationId of the Sources current destination */
#if defined(ST_7100) || defined(ST_7109)
    U16                 Read_p;
    U16                 Write_p;
#endif
} STMERGE_Status_t;


/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t STMERGE_Init(const ST_DeviceName_t DeviceName,
                            const STMERGE_InitParams_t *InitParams_p);

ST_Revision_t STMERGE_GetRevision(void);

ST_ErrorCode_t STMERGE_Term(const ST_DeviceName_t DeviceName,
                            const void  *Reserved_p);

ST_ErrorCode_t STMERGE_SetParams(STMERGE_ObjectId_t Id,
                                 STMERGE_ObjectConfig_t *Config_p);

ST_ErrorCode_t STMERGE_SetStatus(STMERGE_ObjectId_t Id);

ST_ErrorCode_t STMERGE_GetParams(STMERGE_ObjectId_t Id,
                                 STMERGE_ObjectConfig_t *Config_p);

ST_ErrorCode_t STMERGE_GetDefaultParams(STMERGE_ObjectId_t Id,
                                        STMERGE_ObjectConfig_t *Source_p);

ST_ErrorCode_t STMERGE_GetStatus(STMERGE_ObjectId_t Id,
                                 STMERGE_Status_t *Status_p);

ST_ErrorCode_t STMERGE_Connect(STMERGE_ObjectId_t SourceId,
                               STMERGE_ObjectId_t DestinationId);

ST_ErrorCode_t STMERGE_ConnectGeneric(STMERGE_ObjectId_t SourceId,
                               STMERGE_ObjectId_t DestinationId, STMERGE_Mode_t Mode);

ST_ErrorCode_t STMERGE_Disconnect(STMERGE_ObjectId_t SourceId,
                               STMERGE_ObjectId_t DestinationId);

ST_ErrorCode_t STMERGE_DuplicateInput(STMERGE_ObjectId_t SourceId);
                               
ST_ErrorCode_t STMERGE_Reset(void);

#if defined(ST_OSLINUX)
ST_ErrorCode_t STMERGE_SWTSXInjectBuf(STMERGE_ObjectId_t SWTS_Port, U8 *Buf, U32 BufSize);
ST_ErrorCode_t STMERGE_GetSWTSBaseAddress(STMERGE_ObjectId_t SWTS_Port, void **Address);
#endif

#ifdef __cplusplus
}
#endif

#endif
