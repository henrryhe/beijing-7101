/******************************************************************************/
/*                                                                            */
/* File name   : STPRM.H                                                      */
/*                                                                            */
/* Description : [ST] - [P]lay & [R]ecord [M]anager                           */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/*                                                                            */
/* !! WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING !!  */
/*                                                                            */
/* Please note that this code is the property of ST MicroElectronics.         */
/* Please note that there are some patents pending.                           */
/* Any usage or copy of a part of this code outside the context of ST         */
/* chipsets is absolutely forbidden.                                          */
/* Thanks for you understanding.                                              */
/*                                                                            */
/* !! WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING !!  */
/*                                                                            */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 02/03/05           Created                      M.CHAPPELLIER              */
/*                                                                            */
/******************************************************************************/

#ifndef _STPRM_H_
#define _STPRM_H_

/* C++ support */
/* ----------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
/* -------- */
#include "stddefs.h"
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

/* Constants */
/* --------- */
#define STPRM_DRIVER_ID         (0x7FAC)                    /* Unique driver identifier                  */
#define STPRM_DRIVER_BASE       (STPRM_DRIVER_ID<<16)       /* Driver base constant for event and errors */
#define STPRM_MAX_NUMBER        (1)                         /* Maximum number of STPRM instance          */
#define STPRM_MAX_LENGTH_NAME   (255)                       /* Maximum length size of the filename       */
#define STPRM_MAX_PIDS          (16)                        /* Maximum number of pids by record/playback */
#define STPRM_MAX_CONNECTIONS   (16)                        /* Maximum number of connections setup       */
#define STPRM_MAX_PLAYBACKS     (8)                         /* Maximum number of playbacks in parallel   */
#define STPRM_MAX_RECORDS       (8)                         /* Maximum number of records in parallel     */
#define STPRM_PLAY_SPEED_ONE    (100)                       /* Value for speed equal to one              */
#define STPRM_PACKET_SIZE       (188)                       /* Transport packet size                     */
#define STPRM_TIME_PRECISION_MS (100)                       /* Time notification precision for indexing  */
#define STPRM_RETURN_FS_ERROR   (STPRM_DRIVER_BASE|0xFE0F)  /* Return parameter used by filesystem calls */
#define STPRM_MAX_VIEWPORTS     (2+1)                       /* Maximum view port supported by video      */
#define STPRM_MAX_PACKET_INSERT (128)                       /* Maximum packet that could be insert in record */


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
#endif  /*ARCHITECTURE_ST40*/


/* Errors event definition */
/* ----------------------- */
/* This error parameter is coming when STPRM_EVT_ERROR is notified */
typedef enum STPRM_Errors_s
{
 STPRM_PLAY_DECODING_ERROR,  /* Error during decode */
 STPRM_PLAY_READ_ERROR,      /* Error during read   */
 STPRM_PLAY_SEEK_ERROR,      /* Error during seek   */
 STPRM_RECORD_WRITE_ERROR    /* Error during write  */
} STPRM_Errors_t;

/* PRM objects events */
/* ------------------ */
typedef enum STPRM_Event_s
{
 STPRM_EVT_END_OF_PLAYBACK=STPRM_DRIVER_BASE, /* Last picture displayed and last audio frame played */
 STPRM_EVT_END_OF_FILE,                       /* Reach end of file                                  */
 STPRM_EVT_ERROR                              /* An error has occur, check error parameter          */
} STPRM_Event_t;

/* PRM Error code definitions */
/* -------------------------- */
enum
{
 STPRM_ERROR_BAD_PARAMETER=STPRM_DRIVER_BASE,
 STPRM_ERROR_NO_MEMORY,
 STPRM_ERROR_FEATURE_NOT_SUPPORTED,
 STPRM_ERROR_ALREADY_INITIALIZED,
 STPRM_ERROR_NOT_INITIALIZED,
 STPRM_ERROR_INIT_FAILED,
 STPRM_ERROR_TERM_FAILED,
 STPRM_ERROR_NOT_STOPPED,
 STPRM_ERROR_CONNECTION_FULL,
 STPRM_ERROR_CONNECTION_ALREADY_INITIALIZED,
 STPRM_ERROR_CONNECTION_NOT_FOUND,
 STPRM_ERROR_DATAPATH_INIT_FAILED,
 STPRM_ERROR_DATAPATH_START_FAILED,
 STPRM_ERROR_DATAPATH_STOP_FAILED,
 STPRM_ERROR_DATAPATH_UPDATE_FAILED,
 STPRM_ERROR_PLAYBACK_FULL,
 STPRM_ERROR_PLAYBACK_ALREADY_STARTED,
 STPRM_ERROR_PLAYBACK_ALREADY_STOPPED,
 STPRM_ERROR_PLAYBACK_NOT_STARTED,
 STPRM_ERROR_PLAYBACK_NOT_PAUSED,
 STPRM_ERROR_PLAYBACK_NO_PIDS,
 STPRM_ERROR_PLAYBACK_VIDEO_INIT_FAILED,
 STPRM_ERROR_PLAYBACK_VIDEO_START_FAILED,
 STPRM_ERROR_PLAYBACK_VIDEO_STOP_FAILED,
 STPRM_ERROR_PLAYBACK_VIDEO_CALL_FAILED,
 STPRM_ERROR_PLAYBACK_AUDIO_INIT_FAILED,
 STPRM_ERROR_PLAYBACK_AUDIO_START_FAILED,
 STPRM_ERROR_PLAYBACK_AUDIO_STOP_FAILED,
 STPRM_ERROR_PLAYBACK_AUDIO_CALL_FAILED,
 STPRM_ERROR_PLAYBACK_CLKRV_INIT_FAILED,
 STPRM_ERROR_PLAYBACK_CLKRV_START_FAILED,
 STPRM_ERROR_PLAYBACK_CLKRV_STOP_FAILED,
 STPRM_ERROR_PLAYBACK_SEEK_ALREADY_IN_PROGRESS,
 STPRM_ERROR_PLAYBACK_SEEK_FAILED,
 STPRM_ERROR_PLAYBACK_SPEED_FAILED,
 STPRM_ERROR_PLAYBACK_TIME_FAILED,
 STPRM_ERROR_RECORD_FULL,
 STPRM_ERROR_RECORD_ALREADY_STARTED,
 STPRM_ERROR_RECORD_ALREADY_STOPPED,
 STPRM_ERROR_RECORD_NOT_STARTED,
 STPRM_ERROR_RECORD_NO_PIDS,
 STPRM_ERROR_RECORD_TIME_FAILED,
 STPRM_ERROR_RECORD_INSERT_FAILED
};

/* Handle of STPRM */
/* --------------- */
typedef U32 STPRM_Handle_t;

/* Connection type definition */
/* -------------------------- */
typedef enum STPRM_ConnectionType_e
{
 STPRM_CONNECTION_SRC_BUFFER = 0x01,
 STPRM_CONNECTION_SRC_DEMUX  = 0x02,
 STPRM_CONNECTION_DST_SWTS   = 0x03,
 STPRM_CONNECTION_DST_TCPIP  = 0x04,
 STPRM_CONNECTION_DST_NULL   = 0x05
} STPRM_ConnectionType_t;

/* SWTS connection definition */
/* -------------------------- */
/* This connection is used to identify where to inject the TS stream */
/* in order to decode it with the internal hardware decoders         */
typedef struct STPRM_ConnectionSwts_s
{
 ST_DeviceName_t PTIDeviceName;     /* Name of the PTI connected to the SWTS input   */
 STPTI_Handle_t  PTIHandle;         /* Handle of the PTI connected to the SWTS input */
 U32             SWTSBaseAddress;   /* SWTS Fifo base address for injection          */
 U32             SWTSRequestSignal; /* SWTS Request signal for DMA injection         */
} STPRM_ConnectionSwts_t;

/* IP connection definition */
/* -------------------------- */
/* This connection is used to identify where to inject the TS stream */
/* in order to decode it with the internal hardware decoders         */
typedef struct STPRM_ConnectionTCPIP_s
{
 void            *TCPIPHandle;                                                      /* Handle to be passed when calling function IPInject */
 ST_ErrorCode_t (*TCPIPInjectTS)(void *TCPIPHandle, U8 *Buffer, U32 SizeInPackets);    /* Method to inject data to IP                        */
} STPRM_ConnectionTCPIP_t;

/* Buffer connection definition */
/* ---------------------------- */
/* This connection is used when the stream is coming from memory */
/* or has to go to memory                                        */
typedef struct STPRM_ConnectionBuffer_s
{
 U32              BufferBase;                                       /* Base address of the buffer, has to be aligned to 512 bytes */
 U32              BufferSize;                                       /* Size of the buffer                                         */
 void            *UserHandle;                                       /* Handle to passed to methods GetProducer & SetConsumer      */
 ST_ErrorCode_t (*GetProducer)(void *UserHandle, U32 **Producer);   /* Method to get the producer pointer                         */
 ST_ErrorCode_t (*SetConsumer)(void *UserHandle, U32  *Consumer);   /* Method to set the consumer pointer                         */
} STPRM_ConnectionBuffer_t;

/* Demux connection definition */
/* --------------------------- */
/* This connection is used when the stream is coming from the demux */
typedef struct STPRM_ConnectionDemux_s
{
 ST_DeviceName_t  PTIDeviceName; /* Name of the PTI connected to the TS input   */
 STPTI_Handle_t   PTIHandle;     /* Handle of the PTI connected to the TS input */
 STCLKRV_Handle_t CLKHandle;     /* Handle of the clkrv for 27MHz management    */
} STPRM_ConnectionDemux_t;

/* Generic connection definition */
/* ----------------------------- */
typedef struct STPRM_Connection_s
{
 STPRM_ConnectionType_t Type;
 union
  {
   STPRM_ConnectionSwts_t   Swts;
   STPRM_ConnectionTCPIP_t  TCPIP;
   STPRM_ConnectionBuffer_t Buffer;
   STPRM_ConnectionDemux_t  Demux;
  } Description;
} STPRM_Connection_t;

/* Init parameters */
/* --------------- */
typedef struct STPRM_InitParams_s
{
 ST_DeviceName_t  EVTDeviceName;   /* Device name used to notify PRM events */
 ST_Partition_t  *CachePartition;  /* Memory cache partition                */
 ST_Partition_t  *NCachePartition; /* Memory non cache partition            */
 ST_DeviceName_t  DMADeviceName;   /* Should be setup for chip using GPDMA  */
} STPRM_InitParams_t;

 /* Terminate parameters */
/* -------------------- */
typedef struct STPRM_TermParams_s
{
  BOOL ForceTerminate;
} STPRM_TermParams_t;

/* Stream types */
/* ------------ */
typedef enum
{
 STPRM_STREAMTYPE_INVALID,
 STPRM_STREAMTYPE_MP1V,
 STPRM_STREAMTYPE_MP2V,
 STPRM_STREAMTYPE_MP4V,
 STPRM_STREAMTYPE_MP1A,
 STPRM_STREAMTYPE_MP2A,
 STPRM_STREAMTYPE_MP4A,
 STPRM_STREAMTYPE_TTXT,
 STPRM_STREAMTYPE_SUBT,
 STPRM_STREAMTYPE_PCR,
 STPRM_STREAMTYPE_AC3,
 STPRM_STREAMTYPE_H264,
 STPRM_STREAMTYPE_DIVX,
 STPRM_STREAMTYPE_VC1,
 STPRM_STREAMTYPE_AAC,
 STPRM_STREAMTYPE_WMA,
 STPRM_STREAMTYPE_OTHER
} STPRM_StreamType_t;

/* Flags for file system */
/* --------------------- */
typedef enum STPRM_FSflags_e
{
 /* Open Flags */
 STPRM_FS_RD       = 0x01,
 STPRM_FS_WR       = 0x02,
 STPRM_FS_CIRCULAR = 0x04,
 /* Seek whence */
 STPRM_FS_SEEK_SET = 0x08,
 STPRM_FS_SEEK_CUR = 0x10,
 STPRM_FS_SEEK_END = 0x20
} STPRM_FSflags_t;

/* Flags for STPRM seek mode */
/* ------------------------- */
typedef enum STPRM_SeekFlags_e
{
 /* Seek whence */
 STPRM_PLAY_SEEK_SET=0,
 STPRM_PLAY_SEEK_CUR,
 STPRM_PLAY_SEEK_END
} STPRM_SeekFlags_t;

/* File system interface */
/* --------------------- */
typedef struct STPRM_FSInterface_s
{
 U32 (*CALL_Open) (STPRM_Handle_t Handle,U8 *FileName,STPRM_FSflags_t Flags,S64 FileSizeInPackets);
 U32 (*CALL_Read) (STPRM_Handle_t Handle,U32 FILE_Handle,U8 *Buffer,U32 SizeInPackets);
 U32 (*CALL_Write)(STPRM_Handle_t Handle,U32 FILE_Handle,U8 *Buffer,U32 SizeInPackets);
 U32 (*CALL_Seek) (STPRM_Handle_t Handle,U32 FILE_Handle,S64 PacketOffset,STPRM_FSflags_t Flags);
 U32 (*CALL_Close)(STPRM_Handle_t Handle,U32 FILE_Handle);
} STPRM_FSInterface_t;

/* Play open parameters */
/* -------------------- */
typedef struct STPRM_OpenParamsPlay_s
{
 STVID_Handle_t          VIDHandle;
 STAUD_Handle_t          AUDHandle;
 STTTX_Handle_t          TTXHandle;
 STTTX_Handle_t          SUBHandle; /* To be mapped to subtitle driver - later... */
 STCLKRV_Handle_t        CLKHandle;
 STVID_ViewPortHandle_t  VIEWHandle[STPRM_MAX_VIEWPORTS];
 STVTG_Handle_t          VTGHandle;
 ST_DeviceName_t         VIDDeviceName;
 ST_DeviceName_t         AUDDeviceName;
 ST_DeviceName_t         EVTDeviceName;
 U32                     AUDClkrvSource;
 /* System of callbacks */
 ST_ErrorCode_t         (*CALL_PlayStart)   (STPRM_Handle_t Handle);
 ST_ErrorCode_t         (*CALL_PlayStop)    (STPRM_Handle_t Handle);
 ST_ErrorCode_t         (*CALL_PlayJump)    (STPRM_Handle_t Handle,U32  PacketTimeInMs,S32 JumpInMs,S64 *NewPacketPosition);
 ST_ErrorCode_t         (*CALL_PlayPacket)  (STPRM_Handle_t Handle,S64  PacketPosition,U32 *NbPacketsToInject,U32 *FirstPacketTimeInMs,U32 *TimeDurationInMs);
 ST_ErrorCode_t         (*CALL_PlayGetTime) (STPRM_Handle_t Handle,S64 *CurrentPacketPosition,U32 *StartTimeInMs,U32 *CurrentTimeInMs,U32 *EndTimeInMs);
} STPRM_OpenParamsPlay_t;

/* Record open parameters */
/* ---------------------- */
typedef struct STPRM_OpenParamsRecord_s
{
 ST_DeviceName_t EVTDeviceName;
 /* System of callbacks */
 ST_ErrorCode_t  (*CALL_RecordStart)   (STPRM_Handle_t Handle);
 ST_ErrorCode_t  (*CALL_RecordStop)    (STPRM_Handle_t Handle);
 ST_ErrorCode_t  (*CALL_RecordPacket)  (STPRM_Handle_t Handle,U32  PacketTimeInMs,S64 PacketPosition);
 ST_ErrorCode_t  (*CALL_RecordGetTime) (STPRM_Handle_t Handle,U32 *StartTimeInMs,U32 *EndTimeInMs);
} STPRM_OpenParamsRecord_t;

/* Object type */
/* ----------- */
typedef enum STPRM_ObjectType_s
{
 STPRM_OBJECT_PLAYBACK = 0x1,
 STPRM_OBJECT_RECORD   = 0x2
} STPRM_ObjectType_t;

/* Open parameters */
/* --------------- */
typedef struct STPRM_OpenParams_s
{
 STPRM_ObjectType_t  Type;
 STPRM_FSInterface_t FSInterface;
 union
  {
   STPRM_OpenParamsPlay_t   OpenParamsPlay;
   STPRM_OpenParamsRecord_t OpenParamsRecord;
  } Params;
} STPRM_OpenParams_t;

/* Stream data parameters */
/* ---------------------- */
typedef struct
{
 STPRM_StreamType_t Type;
 STPTI_Pid_t        Pid;
} STPRM_StreamData_t;

/* Play start parameters */
/* --------------------- */
typedef struct STPRM_PlayStartParams_s
{
 char               SourceName[STPRM_MAX_LENGTH_NAME];      /* Name of the source, connection name or filename      */
 char               DestinationName[STPRM_MAX_LENGTH_NAME]; /* Name of the destination, should be "SWTS" by default */
 U8                 PidsNum;
 STPRM_StreamData_t Pids[STPRM_MAX_PIDS];
 BOOL               VideoEnableDisplay;   /* This flag should be TRUE if we want STPRM to display the video */
 BOOL               VideoDecodeOnce;      /* This flag should be TRUE if we want to decode only one picture */
 BOOL               AudioEnableOutput;    /* This flag should be TRUE if we want STPRM to unmute audio      */
 U32                StartPositionInMs;    /* Start position                                                 */
 S32                StartSpeed;           /* Start speed                                                    */
} STPRM_PlayStartParams_t;

/* Play stop parameters */
/* -------------------- */
typedef struct STPRM_PlayStopParams_s
{
 BOOL               VideoDisableDisplay; /* This flag should be TRUE if we want STPRM to disable the video plane */
 STVID_Stop_t       VideoStopMethod;     /* Define the way to stop the video decoder          */
 STVID_Freeze_t     VideoFreezeMethod;   /* Define the freeze to perform on the video decoder */
 STAUD_Stop_t       AudioStopMethod;     /* Define the way to stop the audio decoder          */
 STAUD_Fade_t       AudioFadingMethod;   /* Define the fading to perform on the audio decoder */
} STPRM_PlayStopParams_t;

/* Record start parameters */
/* ----------------------- */
typedef struct STPRM_RecordStartParams_s
{
 char               SourceName[STPRM_MAX_LENGTH_NAME];
 char               DestinationName[STPRM_MAX_LENGTH_NAME];
 U8                 PidsNum;
 STPRM_StreamData_t Pids[STPRM_MAX_PIDS];
 S64                FileSizeInPackets;
 BOOL               CircularFile;
 U32                FlushThresholdInPackets;
} STPRM_RecordStartParams_t;

/* Record stop parameters */
/* ---------------------- */
typedef struct STPRM_RecordStopParams_s
{
 U32 Dummy;
} STPRM_RecordStopParams_t;

/* Structure returned on all basic events */
/* -------------------------------------- */
typedef struct STPRM_EventData_s
{
 STPRM_Handle_t     Handle;
 STPRM_ObjectType_t Type;
} STPRM_EventData_t;

/* Structure returned on error events */
/* ---------------------------------- */
typedef struct STPRM_EventError_s
{
 STPRM_Handle_t     Handle;
 STPRM_ObjectType_t Type;
 STPRM_Errors_t     Error;
} STPRM_EventError_t;

/* Play File status */
/* ---------------- */
typedef struct STPRM_PlayStatus_s
{
 char               SourceName[STPRM_MAX_LENGTH_NAME];
 char               DestinationName[STPRM_MAX_LENGTH_NAME];
 U8                 PidsNum;
 STPRM_StreamData_t Pids[STPRM_MAX_PIDS];
} STPRM_PlayStatus_t;

/* Record File status */
/* ------------------ */
typedef struct STPRM_RecordStatus_s
{
 char               SourceName[STPRM_MAX_LENGTH_NAME];
 char               DestinationName[STPRM_MAX_LENGTH_NAME];
 U8                 PidsNum;
 STPRM_StreamData_t Pids[STPRM_MAX_PIDS];
 S64                FileSizeInPackets;
 BOOL               CircularFile;
} STPRM_RecordStatus_t;

/* Play parameters */
/* --------------- */
typedef struct STPRM_PlayParams_s
{
 U8           *TSBuffer;               /* TS buffer pointer                 */
 U32           TSBufferSize;           /* TS buffer size                    */
 U32           TSBufferFreeSize;       /* TS buffer size not used           */
 U32           PESVideoBufferSize;     /* Video pes buffer size             */
 U32           PESVideoBufferFreeSize; /* Video pes buffer size not used    */
 U32           PESAudioBufferSize;     /* Audio pes buffer size             */
 U32           PESAudioBufferFreeSize; /* Audio pes buffer size not used    */
 U32           ESVideoBufferSize;      /* Video bit buffer size             */
 U32           ESVideoBufferFreeSize;  /* Video bit buffer size not used    */
 U32           ESAudioBufferSize;      /* Audio bit buffer size             */
 U32           ESAudioBufferFreeSize;  /* Audio bit buffer size not used    */
 STPTI_Pid_t   VideoPid;               /* Pti video pid                     */
 STPTI_Slot_t  VideoSlot;              /* Pti slot used for Video reception */
 STPTI_Pid_t   AudioPid;               /* Pti audio pid                     */
 STPTI_Slot_t  AudioSlot;              /* Pti slot used for Audio reception */
 STPTI_Pid_t   PcrPid;                 /* Pti pcr pid                       */
 STPTI_Slot_t  PcrSlot;                /* Pti slot used for PCR reception   */
} STPRM_PlayParams_t;

/* Record parameters */
/* ----------------- */
typedef struct STPRM_RecordParams_s
{
 U8           *TSBuffer;              /* TS buffer pointer               */
 U32           TSBufferSize;          /* TS buffer size                  */
 U32           TSBufferFreeSize;      /* TS buffer size not used         */
 U32           PidsNum;               /* Number of unique pids collected */
 STPTI_Pid_t   Pids[STPRM_MAX_PIDS];  /* Pid collected list              */                                        /* List of union pids to capture      */
 STPTI_Slot_t  Slots[STPRM_MAX_PIDS]; /* Slot collected list             */
} STPRM_RecordParams_t;

/* Prototypes */
/* ---------- */
ST_ErrorCode_t STPRM_Init                      (ST_DeviceName_t DeviceName,STPRM_InitParams_t *InitParams);
ST_ErrorCode_t STPRM_Term                      (ST_DeviceName_t DeviceName,STPRM_TermParams_t *TermParams);
ST_Revision_t  STPRM_GetRevision               (void);
ST_ErrorCode_t STPRM_SetConnection             (ST_DeviceName_t DeviceName,char *Connection_Name,STPRM_Connection_t *Connection);
ST_ErrorCode_t STPRM_GetConnection             (ST_DeviceName_t DeviceName,char *Connection_Name,STPRM_Connection_t *Connection);
ST_ErrorCode_t STPRM_Open                      (ST_DeviceName_t DeviceName,STPRM_OpenParams_t *OpenParams,STPRM_Handle_t *Handle);
ST_ErrorCode_t STPRM_Close                     (STPRM_Handle_t Handle);
ST_ErrorCode_t STPRM_PlayStart                 (STPRM_Handle_t PlayHandle,STPRM_PlayStartParams_t *PlayStartParams);
ST_ErrorCode_t STPRM_PlayStop                  (STPRM_Handle_t PlayHandle,STPRM_PlayStopParams_t *PlayStopParams);
ST_ErrorCode_t STPRM_PlaySeek                  (STPRM_Handle_t PlayHandle,S32 TimeInMs,STPRM_SeekFlags_t Flags);
ST_ErrorCode_t STPRM_PlaySetSpeed              (STPRM_Handle_t PlayHandle,S32 Speed);
ST_ErrorCode_t STPRM_PlayGetSpeed              (STPRM_Handle_t PlayHandle,S32 *Speed);
ST_ErrorCode_t STPRM_PlayGetStatus             (STPRM_Handle_t PlayHandle,STPRM_PlayStatus_t *PlayStatus);
ST_ErrorCode_t STPRM_PlayGetTime               (STPRM_Handle_t PlayHandle,U32 *BeginOfFileTimeInMs,U32 *CurrentPlayTimeInMs,U32 *EndOfFileTimeInMs);
ST_ErrorCode_t STPRM_PlayGetParams             (STPRM_Handle_t PlayHandle,STPRM_PlayParams_t *PlayParams);
ST_ErrorCode_t STPRM_PlayChangePids            (STPRM_Handle_t PlayHandle,U32 PidsNum,STPRM_StreamData_t *Pids);
ST_ErrorCode_t STPRM_PlayPause                 (STPRM_Handle_t PlayHandle);
ST_ErrorCode_t STPRM_PlayResume                (STPRM_Handle_t PlayHandle);
ST_ErrorCode_t STPRM_PlayStep                  (STPRM_Handle_t PlayHandle);
ST_ErrorCode_t STPRM_RecordStart               (STPRM_Handle_t RecordHandle,STPRM_RecordStartParams_t *RecordStartParams);
ST_ErrorCode_t STPRM_RecordStop                (STPRM_Handle_t RecordHandle,STPRM_RecordStopParams_t *RecordStopParams);
ST_ErrorCode_t STPRM_RecordGetStatus           (STPRM_Handle_t RecordHandle,STPRM_RecordStatus_t *RecordStatus);
ST_ErrorCode_t STPRM_RecordGetTime             (STPRM_Handle_t RecordHandle,U32 *BeginOfFileTimeInMs,U32 *EndOfFileTimeInMs);
ST_ErrorCode_t STPRM_RecordGetParams           (STPRM_Handle_t RecordHandle,STPRM_RecordParams_t *RecordParams);
ST_ErrorCode_t STPRM_RecordPause               (STPRM_Handle_t RecordHandle);
ST_ErrorCode_t STPRM_RecordResume              (STPRM_Handle_t RecordHandle);
ST_ErrorCode_t STPRM_RecordChangePids          (STPRM_Handle_t RecordHandle,U32 NumberOfPids, STPRM_StreamData_t *Pids);
ST_ErrorCode_t STPRM_RecordChangePidsAndSource (STPRM_Handle_t RecordHandle,char *NewSourceName,U32 NumberOfPids, STPRM_StreamData_t *Pids);
ST_ErrorCode_t STPRM_RecordInsertPacket        (STPRM_Handle_t RecordHandle,U8 *Buffer,U8 NbOfPackets);
ST_ErrorCode_t STPRM_PlayPauseRecord           (STPRM_Handle_t PlayHandle,STPRM_Handle_t RecordHandle, STPRM_RecordStartParams_t *RecordStartParams);

/* C++ support */
/* ----------- */
#ifdef __cplusplus
}
#endif
#endif  /*_STPRM_H_*/

