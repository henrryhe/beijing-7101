/******************************************************************************

File Name  : stdvm.h

Description: Digital Versatile Manager header

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STDVM_H
#define __STDVM_H

/* C++ support ------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ---------------------------------------------------------------- */
#include "stpti.h"
#include "stvid.h"
#if defined(ST_5528)||defined(ST_5100)||defined(ST_5301)||defined(ST_7710)
#include "staud.h"
#elif defined(ST_7100)||defined(ST_7109)
#include "staudlx.h"
#endif
#include "stclkrv.h"
#include "stttx.h"
#include "stvtg.h"

/* Exported Constants ------------------------------------------------------ */
#define STDVM_DRIVER_ID        (0x0FAD)
#define STDVM_DRIVER_BASE      (STDVM_DRIVER_ID<<16)
#define STDVM_MAX_NUMBER       (1)
#define STDVM_MAX_LENGTH_NAME  (255)
#define STDVM_MAX_PIDS         (16)
#define STDVM_MAX_CONNECTIONS  (16)
#define STDVM_MAX_PLAYBACKS    (8)
#define STDVM_MAX_RECORDS      (8)
#define STDVM_MAX_INSTANCES    ((STDVM_MAX_PLAYBACKS)+(STDVM_MAX_RECORDS))
#define STDVM_PLAY_SPEED_ONE   (100)
#define STDVM_PACKET_SIZE      (188)
#define STDVM_MAX_VIEWPORTS    (2+1)

/*20070823 add*/
#ifndef ARCHITECTURE_ST40
#define ARCHITECTURE_ST40
#endif

/* Exported Variables ------------------------------------------------------ */

/* Exported Macros --------------------------------------------------------- */

/* Exported Types ---------------------------------------------------------- */

/* 64Bit typedefs are specifically for ST40 must be changed with type long long in stddefs.h */
/*----------------- 64-bit unsigned type -----------------*/
#ifdef ARCHITECTURE_ST40
#ifndef DEFINED_U64
#define DEFINED_U64
typedef struct U64_s
{
    unsigned int LSW;
    unsigned int MSW;
}
U64;
#endif

/*The following S64 type is not truly a Signed number. You would have to use additional
  logic to use it alongwith the macro I64_IsNegative(A) provided below*/
#ifndef DEFINED_S64
#define DEFINED_S64
typedef U64 S64;
#endif

/*Value=A+B, where A & B is U64 type*/
#define I64_Add(A,B,Value)      { register long long T1,T2,Val; \
                                T1  = (long long)(A).MSW << 32 | (A).LSW; \
                                T2  = (long long)(B).MSW << 32 | (B).LSW; \
                                Val = T1 + T2; \
                                (Value).MSW = Val >> 32; \
                                (Value).LSW = (U32)Val; \
                                }

/*Value=A+B, where A is U64 type & B is 32-bit atmost*/
#define I64_AddLit(A,B,Value)   { register long long T1,Val; \
                                T1 = (long long)(A).MSW << 32 | (A).LSW; \
                                Val=T1+(B); \
                                (Value).MSW = Val >> 32; \
                                (Value).LSW = (U32)Val; \
                                }

/*A==B, A & B are U64 type*/
#define I64_IsEqual(A,B)                (((A).LSW == (B).LSW) && ((A).MSW == (B).MSW))

#define I64_GetValue(Value,Lower,Upper) ((Lower) = (Value).LSW, (Upper) = (Value).MSW)

/*A>=B, A & B are U64 type*/
#define I64_IsGreaterOrEqual(A,B)       ( ((A).MSW >  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW >= (B).LSW)))

/*A>B, A & B are U64 type*/
#define I64_IsGreaterThan(A,B)          ( ((A).MSW >  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW > (B).LSW)))

/*A<B, A & B are U64 type*/
#define I64_IsLessThan(A,B)             ( ((A).MSW <  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW < (B).LSW)))

/*A<=B, A & B are U64 type*/
#define I64_IsLessOrEqual(A,B)          ( ((A).MSW <  (B).MSW) || \
                                         (((A).MSW == (B).MSW) && ((A).LSW <= (B).LSW)))

#define I64_IsNegative(A)               ((A).MSW & 0X80000000)

/*A==0, A is U64 type*/
#define I64_IsZero(A)                   (((A).LSW == 0) && ((A).MSW == 0))

/*A!=B, A & B are U64 type*/
#define I64_AreNotEqual(A,B)            (((A).LSW != (B).LSW) || ((A).MSW != (B).MSW))

#define I64_SetValue(Lower,Upper,Value) ((Value).LSW = (Lower), (Value).MSW = (Upper))

/*Value=A-B, where A & B are U64 type*/
#define I64_Sub(A,B,Value)              ((Value).MSW  = (A).MSW - (B).MSW - (((A).LSW < (B).LSW)?1:0), \
                                         (Value).LSW  = (A).LSW - (B).LSW)

/*Value=A-B, where A is U64 type & B is 32-bit atmost*/
#define I64_SubLit(A,B,Value)           ((Value).MSW  = (A).MSW - (((A).LSW < (B))?1:0), \
                                         (Value).LSW  = (A).LSW - (B))

/*Value=A/B, where A is U64 type & B is 32-bit atmost*/
#define I64_DivLit(A,B,Value)           { register long long T1, Val;\
                                        T1 = (long long)(A).MSW << 32 | (A).LSW;\
                                        Val = T1/(B);\
                                        (Value).MSW = Val >> 32;\
                                        (Value).LSW = (U32)Val;\
                                        }

/*Value=A%B, where A is U64 type & B is 32-bit atmost*/
#define I64_ModLit(A,B,Value)           { register long long T1, Val;\
                                        T1 = (long long)(A).MSW << 32 | (A).LSW;\
                                        Val=T1%(B);\
                                        (Value).MSW = Val >> 32;\
                                        (Value).LSW = (U32)Val;\
                                        }

/*Value=A*B, where A & B are U64 type*/
#define I64_Mul(A,B,Value)              { register long long T1, T2, Val; \
                                        T1 = (long long)(A).MSW << 32 | (A).LSW; \
                                        T2 = (long long)(B).MSW << 32 | (B).LSW; \
                                        Val=T1*T2; \
                                        (Value).MSW = Val >> 32; \
                                        (Value).LSW = (U32)Val; \
                                        }

/*Value=A*B, where A is U64 type & B is 32-bit atmost*/
#define I64_MulLit(A,B,Value)           { register long long T1,Val; \
                                        T1 = (long long)(A).MSW << 32 | (A).LSW; \
                                        Val=T1*(B); \
                                        (Value).MSW = Val >> 32; \
                                        (Value).LSW = (U32)Val; \
                                        }

/*Value=Value<<Shift, where Value is U64 type*/
#define I64_ShiftLeft(Shift,Value)      { register long long T1, T2, Val; \
                                        T1 = (long long)(Value).MSW << 32 | (Value).LSW; \
                                        Val=T1 << Shift; \
                                        (Value).MSW = Val >> 32; \
                                        (Value).LSW = (U32)Val; \
                                        }

/*Value=Value>>Shift, where Value is U64 type*/
#define I64_ShiftRight(Shift,Value)     { register long long T1, T2, Val; \
                                        T1 = (long long)(Value).MSW << 32 | (Value).LSW; \
                                        Val=T1 >> Shift; \
                                        (Value).MSW = Val >> 32; \
                                        (Value).LSW = (U32)Val; \
                                        }
#endif /*#ifdef ARCHITECTURE_ST40*/


/* DVM errors */
typedef enum STDVM_Errors_e
{
    STDVM_PLAY_DECODING_ERROR,            /* Error during decode */
    STDVM_PLAY_READ_ERROR,                /* Error during read */
    STDVM_PLAY_SEEK_ERROR,                /* Error during seek */
    STDVM_RECORD_WRITE_ERROR              /* Error during write */
} STDVM_Errors_t;


/* DVM objects events */
typedef enum STDVM_Event_e
{
    STDVM_EVT_END_OF_PLAYBACK = STDVM_DRIVER_BASE,  /* Last picture displayed and last audio frame played */
    STDVM_EVT_END_OF_FILE,                          /* Reach end of file */
    STDVM_EVT_ERROR,
    STDVM_EVT_WRITE_CLOSE_TO_READ,                  /* Write will overwrite data soon, Speed passed in event data */
    STDVM_EVT_WRITE_TO_READ_MARGIN_OK,              /* Enough space to continue safe writing */
    STDVM_EVT_DISK_SPACE_LOW,                       /* a warning for the deleting temp files */
    STDVM_EVT_DISK_FULL                             /* disk completely full */
} STDVM_Event_t;


/* DVM objects events */
typedef enum STDVM_EventIDs_e
{
    STDVM_EVT_END_OF_PLAYBACK_ID,
    STDVM_EVT_END_OF_FILE_ID,
    STDVM_EVT_ERROR_ID,
    STDVM_EVT_WRITE_CLOSE_TO_READ_ID,
    STDVM_EVT_WRITE_TO_READ_MARGIN_OK_ID,
    STDVM_EVT_DISK_SPACE_LOW_ID,
    STDVM_EVT_DISK_FULL_ID,
    STDVM_MAX_EVTS
} STDVM_EventIDs_t;


/* DVM Error code definitions */
enum {
    STDVM_ERROR_INFO_FILE_OPEN_FAILED = STDVM_DRIVER_BASE,
    STDVM_ERROR_INFO_FILE_WRITE_FAILED,
    STDVM_ERROR_INFO_FILE_READ_FAILED,
    STDVM_ERROR_STREAM_UNLINK_FAILED,
    STDVM_ERROR_STREAM_ALREADY_EXISTS
};


/* Handle of STDVM */
typedef U32 STDVM_Handle_t;


/* Connection type definition */
/* -------------------------- */
typedef enum STDVM_ConnectionType_e
{
    STDVM_CONNECTION_SRC_DEMUX,
    STDVM_CONNECTION_SRC_NETWORK,
    STDVM_CONNECTION_DST_SWTS,
    STDVM_CONNECTION_DST_NETWORK,
    STDVM_CONNECTION_DST_NULL
} STDVM_ConnectionType_t;


/* Buffer connection definition */
/* ---------------------------- */
/* This connection is used when the stream is coming from Network */
/* or has to go to Network                                        */
typedef struct STDVM_ConnectionNetwork_s
{
    ST_DeviceName_t     NETDeviceName;              /* Name of the STNET device for this input */
    char                URI[STDVM_MAX_LENGTH_NAME]; /* Uniform resource Identifier Ex. rtp://224.10.10.1:1234 */
} STDVM_ConnectionNetwork_t;


/* Demux connection definition */
/* --------------------------- */
/* This connection is used when the stream is coming from the demux */
typedef struct STDVM_ConnectionDemux_s
{
    ST_DeviceName_t     PTIDeviceName;      /* Name of the PTI connected to the TS input   */
    STPTI_Handle_t      PTIHandle;          /* Handle of the PTI connected to the TS input */
    STCLKRV_Handle_t    CLKHandle;          /* Handle of the clkrv for 27MHz management    */
} STDVM_ConnectionDemux_t;


/* SWTS connection definition */
/* -------------------------- */
/* This connection is used to identify where to inject the TS stream */
/* in order to decode it with the internal hardware decoders         */
typedef struct STDVM_ConnectionSwts_s
{
    ST_DeviceName_t     PTIDeviceName;      /* Name of the PTI connected to the SWTS input   */
    STPTI_Handle_t      PTIHandle;          /* Handle of the PTI connected to the SWTS input */
    U32                 SWTSBaseAddress;    /* SWTS Fifo base address for injection          */
    U32                 SWTSRequestSignal;  /* SWTS Request signal for DMA injection         */
} STDVM_ConnectionSwts_t;


/* Generic connection definition */
/* ----------------------------- */
typedef struct STDVM_Connection_s
{
    STDVM_ConnectionType_t  Type;
    union
    {
        STDVM_ConnectionNetwork_t   Network;
        STDVM_ConnectionDemux_t     Demux;
        STDVM_ConnectionSwts_t      Swts;
    } Description;
} STDVM_Connection_t;


/* Init parameters to instance a specified object */
typedef struct STDVM_InitParams_s
{
    ST_DeviceName_t     EVTDeviceName;              /* Device name used to notify PRM events  */
    ST_Partition_t     *CachePartition;             /* Memory cache partition                 */
    ST_Partition_t     *NCachePartition;            /* Memory non cache partition             */
    ST_DeviceName_t     DMADeviceName;              /* Should be setup for chip using GPDMA   */
    U32                 NbOfConnections;            /* No of connections to be opened for the device */
    STDVM_Connection_t *Connections;                /* pointer to the array of connections    */
    U32                 ReadWriteThresholdInMs;     /* Threshold for write and read are close */
    U32                 ReadWriteThresholdOKInMs;   /* Threshold for write and read are close */
    U32                 MinDiskSpace;               /* Threshold for DISK_SPACE_LOW warning   */
} STDVM_InitParams_t;


/* Terminate parameters */
typedef struct STDVM_TermParams_s
{
    BOOL                ForceTerminate;
} STDVM_TermParams_t;


/* STREAM types */
typedef enum
{
    STDVM_STREAMTYPE_INVALID,
    STDVM_STREAMTYPE_MP1V,
    STDVM_STREAMTYPE_MP2V,
    STDVM_STREAMTYPE_MP4V,
    STDVM_STREAMTYPE_MP1A,
    STDVM_STREAMTYPE_MP2A,
    STDVM_STREAMTYPE_MP4A,
    STDVM_STREAMTYPE_TTXT,
    STDVM_STREAMTYPE_SUBT,
    STDVM_STREAMTYPE_PCR,
    STDVM_STREAMTYPE_AC3,
    STDVM_STREAMTYPE_H264,
    STDVM_STREAMTYPE_DIVX,
    STDVM_STREAMTYPE_VC1,
    STDVM_STREAMTYPE_AAC,
    STDVM_STREAMTYPE_WMA,
    STDVM_STREAMTYPE_OTHER,  /* type not specified, used for other */
    STDVM_STREAMTYPE_MP3,
    STDVM_STREAMTYPE_JPEG
} STDVM_StreamType_t;

/* Flags for STDVM seek mode */
typedef enum STDVM_SeekFlags_e
{
 /* Seek whence */
    STDVM_PLAY_SEEK_SET=0,
    STDVM_PLAY_SEEK_CUR,
    STDVM_PLAY_SEEK_END
} STDVM_SeekFlags_t;

/* Play open parameters */
typedef struct STDVM_OpenParamsPlay_s
{
    STVID_Handle_t          VIDHandle;
    STAUD_Handle_t          AUDHandle;
    STTTX_Handle_t          TTXHandle;
    STTTX_Handle_t          SUBHandle; /* To be mapped to subtitle driver - later... */
    STCLKRV_Handle_t        CLKHandle;
    STVID_ViewPortHandle_t  VIEWHandle[STDVM_MAX_VIEWPORTS];
    STVTG_Handle_t          VTGHandle;
    ST_DeviceName_t         VIDDeviceName;
    ST_DeviceName_t         AUDDeviceName;
    ST_DeviceName_t         EVTDeviceName;
    U32                     AUDClkrvSource;
} STDVM_OpenParamsPlay_t;

/* Record open parameters */
typedef struct STDVM_OpenParamsRecord_s
{
    ST_DeviceName_t         EVTDeviceName;
} STDVM_OpenParamsRecord_t;

/* Object type */
typedef enum STDVM_ObjectType_s
{
    STDVM_OBJECT_PLAYBACK = 1,
    STDVM_OBJECT_RECORD
} STDVM_ObjectType_t;

/* Open parameters */
typedef struct STDVM_OpenParams_s
{
    STDVM_ObjectType_t  Type;
    union
    {
        STDVM_OpenParamsPlay_t      OpenParamsPlay;
        STDVM_OpenParamsRecord_t    OpenParamsRecord;
    } Params;
} STDVM_OpenParams_t;

/* Stream data parameters */
typedef struct
{
    STDVM_StreamType_t  Type;
    STPTI_Pid_t         Pid;
} STDVM_StreamData_t;

/* Key for En/DeCrypt */
typedef U32 STDVM_Key_t[4];

/* Play start parameters */
typedef struct STDVM_PlayStartParams_s
{
    char                SourceName[STDVM_MAX_LENGTH_NAME];
    char                DestinationName[STDVM_MAX_LENGTH_NAME]; /* Name of the destination */
    U8                  PidsNum;
    STDVM_StreamData_t  Pids[STDVM_MAX_PIDS];
    BOOL                VideoEnableDisplay;     /* This flag should be TRUE if we want STDVM to display the video */
    BOOL                VideoDecodeOnce;        /* This flag should be TRUE if we want to decode only one picture */
    BOOL                AudioEnableOutput;      /* This flag should be TRUE if we want STDVM to unmute audio      */
    U32                 StartPositionInMs;      /* Start position */
    S32                 StartSpeed;             /* Start speed */
    BOOL                EnableCrypt;            /* If the stream has to be DeCrypted */
    STDVM_Key_t         CryptKey;               /* Key for Decryption */
} STDVM_PlayStartParams_t;


/* Play stop parameters */
typedef struct STDVM_PlayStopParams_s
{
    BOOL                VideoDisableDisplay;    /* This flag should be TRUE if we want STDVM to disable the video plane */
    STVID_Stop_t        VideoStopMethod;        /* Define the way to stop the video decoder          */
    STVID_Freeze_t      VideoFreezeMethod;      /* Define the freeze to perform on the video decoder */
    STAUD_Stop_t        AudioStopMethod;        /* Define the way to stop the audio decoder          */
    STAUD_Fade_t        AudioFadingMethod;      /* Define the fading to perform on the audio decoder */
} STDVM_PlayStopParams_t;


/* Record start parameters */
typedef struct STDVM_RecordStartParams_s
{
    char                SourceName[STDVM_MAX_LENGTH_NAME];
    char                DestinationName[STDVM_MAX_LENGTH_NAME];
    char                ChannelName[STDVM_MAX_LENGTH_NAME];
    char                Description[STDVM_MAX_LENGTH_NAME];
    U32                 RecordDateTime;
    U8                  PidsNum;
    STDVM_StreamData_t  Pids[STDVM_MAX_PIDS];
    S64                 FileSizeInPackets;
    BOOL                CircularFile;
    BOOL                EnableCrypt;            /* If the stream has to be EnCrypted */
    STDVM_Key_t         CryptKey;               /* Key for Encryption */
} STDVM_RecordStartParams_t;


/* Record stop parameters */
typedef struct STDVM_RecordStopParams_s
{
    U32 Dummy;
} STDVM_RecordStopParams_t;


/* Stream info */
typedef struct STDVM_StreamInfo_s
{
    char                ChannelName[STDVM_MAX_LENGTH_NAME];
    char                Description[STDVM_MAX_LENGTH_NAME];
    U8                  PidsNum;
    STDVM_StreamData_t  Pids[STDVM_MAX_PIDS];
    U32                 StartTimeInMs;
    U32                 DurationInMs;
    S64                 NbOfPackets;
    U32                 RecordDateTime;
} STDVM_StreamInfo_t;


/* Structure returned on all basic events */
typedef struct STDVM_EventData_s{
    STDVM_Handle_t      Handle;
    STDVM_ObjectType_t  Type;
} STDVM_EventData_t;


/* Structure returned on error events */
typedef struct STDVM_EventError_s{
    STDVM_Handle_t      Handle;
    STDVM_ObjectType_t  Type;
    STDVM_Errors_t      Error;
} STDVM_EventError_t;


/* Play File parameters */
typedef struct STDVM_PlayStatus_s
{
    char               SourceName[STDVM_MAX_LENGTH_NAME];
    char               DestinationName[STDVM_MAX_LENGTH_NAME];       /* Name of the destination */
    U8                 PidsNum;
    STDVM_StreamData_t Pids[STDVM_MAX_PIDS];
} STDVM_PlayStatus_t;


/* Record File parameters */
typedef struct STDVM_RecordStatus_s
{
    char               SourceName[STDVM_MAX_LENGTH_NAME];
    char               DestinationName[STDVM_MAX_LENGTH_NAME];       /* Name of the destination */
    U8                 PidsNum;
    STDVM_StreamData_t Pids[STDVM_MAX_PIDS];
    S64                FileSizeInPackets; /* In Packets */
    BOOL               CircularFile;
} STDVM_RecordStatus_t;


/* Play parameters */
typedef struct STDVM_PlayParams_s
{
    STPTI_Pid_t     VideoPid;               /* video pid                         */
    STPTI_Slot_t    VideoSlot;              /* Pti slot used for Video reception */
    STPTI_Pid_t     AudioPid;               /* audio pid                         */
    STPTI_Slot_t    AudioSlot;              /* Pti slot used for Audio reception */
    STPTI_Pid_t     PcrPid;                 /* pcr pid                           */
    STPTI_Slot_t    PcrSlot;                /* Pti slot used for PCR reception   */
} STDVM_PlayParams_t;


/* Record parameters */
typedef struct STDVM_RecordParams_s
{
    U32             PidsNum;                /* Number of unique pids collected */
    STPTI_Pid_t     Pids[STDVM_MAX_PIDS];   /* Pid collected list              */
    STPTI_Slot_t    Slots[STDVM_MAX_PIDS];  /* Slot collected list             */
} STDVM_RecordParams_t;


/* Exported Functions ------------------------------------------------------ */

ST_ErrorCode_t STDVM_Init                      (ST_DeviceName_t DeviceName, STDVM_InitParams_t *InitParams_p);
ST_Revision_t  STDVM_GetRevision               (void);
ST_ErrorCode_t STDVM_Open                      (ST_DeviceName_t DeviceName, STDVM_OpenParams_t *OpenParams_p, STDVM_Handle_t *Handle);
ST_ErrorCode_t STDVM_PlayStart                 (STDVM_Handle_t  PlayHandle, STDVM_PlayStartParams_t *PlayStartParams_p);
ST_ErrorCode_t STDVM_PlayStop                  (STDVM_Handle_t  PlayHandle, STDVM_PlayStopParams_t  *PlayStopParams_p);
ST_ErrorCode_t STDVM_PlaySeek                  (STDVM_Handle_t  PlayHandle, S32 TimeInMs, STDVM_SeekFlags_t Flags);
ST_ErrorCode_t STDVM_PlaySetSpeed              (STDVM_Handle_t  PlayHandle, S32 Speed);
ST_ErrorCode_t STDVM_PlayGetSpeed              (STDVM_Handle_t  PlayHandle, S32 *Speed_p);
ST_ErrorCode_t STDVM_PlayPause                 (STDVM_Handle_t  PlayHandle);
ST_ErrorCode_t STDVM_PlayResume                (STDVM_Handle_t  PlayHandle);
ST_ErrorCode_t STDVM_PlayStep                  (STDVM_Handle_t  PlayHandle);
ST_ErrorCode_t STDVM_PlayGetStatus             (STDVM_Handle_t  PlayHandle, STDVM_PlayStatus_t *PlayStatus_p);
ST_ErrorCode_t STDVM_PlayGetTime               (STDVM_Handle_t  PlayHandle, U32 *BeginOfFileTimeInMs_P, U32 *CurrentPlayTimeInMs_p, U32 *EndOfFileTimeInMs_p);
ST_ErrorCode_t STDVM_PlayGetParams             (STDVM_Handle_t  PlayHandle, STDVM_PlayParams_t *PlayParams_p);
ST_ErrorCode_t STDVM_PlayChangePids            (STDVM_Handle_t  PlayHandle, U32 NumberOfPids, STDVM_StreamData_t *Pids_p);
ST_ErrorCode_t STDVM_RecordStart               (STDVM_Handle_t  RecordHandle, STDVM_RecordStartParams_t *RecordStartParams_p);
ST_ErrorCode_t STDVM_RecordStop                (STDVM_Handle_t  RecordHandle, STDVM_RecordStopParams_t  *RecordStopParams_p);
ST_ErrorCode_t STDVM_RecordGetStatus           (STDVM_Handle_t  RecordHandle, STDVM_RecordStatus_t *RecordStatus_p);
ST_ErrorCode_t STDVM_RecordGetTime             (STDVM_Handle_t  RecordHandle, U32 *BeginOfFileTimeInMs_p, U32 *EndOfFileTimeInMs_p);
ST_ErrorCode_t STDVM_RecordGetParams           (STDVM_Handle_t  RecordHandle, STDVM_RecordParams_t *RecordParams_p);
ST_ErrorCode_t STDVM_RecordPause               (STDVM_Handle_t  RecordHandle);
ST_ErrorCode_t STDVM_RecordResume              (STDVM_Handle_t  RecordHandle);
ST_ErrorCode_t STDVM_RecordChangePids          (STDVM_Handle_t  RecordHandle, U32 NumberOfPids, STDVM_StreamData_t *Pids);
ST_ErrorCode_t STDVM_RecordChangePidsAndSource (STDVM_Handle_t  RecordHandle, char *NewSourceName, U32 NumberOfPids, STDVM_StreamData_t *Pids);
ST_ErrorCode_t STDVM_RecordInsertPacket        (STDVM_Handle_t  RecordHandle, U8 *Buffer, U8 NbOfPackets);
ST_ErrorCode_t STDVM_PlayPauseRecord           (STDVM_Handle_t  PlayHandle, STDVM_Handle_t RecordHandle, STDVM_RecordStartParams_t *RecordStartParams_P);
ST_ErrorCode_t STDVM_GetStreamInfo             (STDVM_Handle_t  Handle, char *StreamName, STDVM_StreamInfo_t *StreamInfo_p);
ST_ErrorCode_t STDVM_RemoveStream              (STDVM_Handle_t  Handle, char *StreamName);
ST_ErrorCode_t STDVM_CopyStream                (STDVM_Handle_t  Handle, char *OldStreamName, char *NewStreamName, U32 StartTimeInMs, U32 EndTimeInMs);
ST_ErrorCode_t STDVM_CropStream                (STDVM_Handle_t  Handle, char *StreamName, U32 NewStartTimeInMs, U32 NewEndTimeInMs);
ST_ErrorCode_t STDVM_SwitchCircularToLinear    (STDVM_Handle_t  RecordHandle);
ST_ErrorCode_t STDVM_Close                     (STDVM_Handle_t  Handle);
ST_ErrorCode_t STDVM_AddConnection             (ST_DeviceName_t DeviceName, STDVM_Connection_t *Connection_p);
ST_ErrorCode_t STDVM_RemoveConnection          (ST_DeviceName_t DeviceName, STDVM_Connection_t *Connection_p);
ST_ErrorCode_t STDVM_Term                      (ST_DeviceName_t DeviceName, STDVM_TermParams_t *TermParams_p);

ST_ErrorCode_t STDVM_GetKey                    (ST_DeviceName_t DeviceName, const void *Buffer, STDVM_Key_t *Key);

/* C++ support ------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif  /*__STDVM_H*/

/* EOF --------------------------------------------------------------------- */

