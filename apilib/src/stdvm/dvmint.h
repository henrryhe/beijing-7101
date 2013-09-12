/*****************************************************************************

File Name  : dvmint.h

Description: STDVM internal types

COPYRIGHT (C) 2006 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DVMINT_H
#define __DVMINT_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"    /* STAPI includes */
#include "stprm.h"
#ifdef ENABLE_MP3_PLAYBACK
 #include "mp3pb.h"
#endif
#include "stdvm.h"
#ifndef ST_OSLINUX
 #include "sttbx.h"

 /* osplus includes */
 #include <osplus.h>
 #include <osplus/vfs.h>
#else
 #include "linuxwrap.h"
 #include "utils.h"
#endif


/* Exported Constants ----------------------------------------------------- */
#define STDVMi_SIGNATURE                0xFACEC0DE
#define STDVMi_STREAM_INFO_VERSION      0x1

#define STDVMi_INFO_FILE_EXT            ".ifo"
#define STDVMi_INDEX_FILE_EXT           ".idx"
#define STDVMi_STREAM_FILE_EXT          ".trp"

#define STDVMi_BACKUP_FILE_EXT          ".bak"

#define STDVMi_MAX_NAME_LENGTH          (STDVM_MAX_LENGTH_NAME+1)
#define STDVMi_MAX_PROGRAMS_IN_FILE     (16)

#define STDVMi_INDEX_TIME_PRECISION_MS  (100)
#define STDVMi_INDEX_ENTRIES_PER_SEC    (10)
#define STDVMi_INDEX_INFO_CACHE_SIZE    (60*STDVMi_INDEX_ENTRIES_PER_SEC)

#define STDVMi_OPEN_FLAGS_DELETE        (~0)

#define STDVMi_TS_RECORD_CHUNK_SIZE     ((STDVM_PACKET_SIZE)*512*8) /* From stprm */
#define STDVMi_MINIMUM_BIT_RATE         (500*1024)   /*500 Kbps*/

#define OSPLUS_SUCCESS                  (0)
#define OSPLUS_FAILURE                  (-1)

#if defined(ST_5528)||defined(ST_5100)||defined(ST_5301)||defined(ST_7710)
#define STDVM_AUDIO_TRICK_MODES /* Enable audio trick modes      */
#endif

/* Exported Variables ----------------------------------------------------- */

extern  partition_t        *STDVMi_CachePartition,
                           *STDVMi_NCachePartition;

/* Exported Macros -------------------------------------------------------- */
#define GET_STREAM_NAME(__Name__, __FileHandle__)   \
            sprintf((__Name__), "%s/%s", (__FileHandle__)->PathName, (__FileHandle__)->FileName)

#define GET_INFO_FILE_NAME(__Name__, __FileHandle__)    \
            sprintf((__Name__), "%s/%s" STDVMi_INFO_FILE_EXT, (__FileHandle__)->PathName, (__FileHandle__)->FileName)

#define GET_INDEX_FILE_NAME(__Name__, __FileHandle__)  \
            sprintf((__Name__), "%s/~%s/%s" STDVMi_INDEX_FILE_EXT, \
                    (__FileHandle__)->PathName, \
                    (__FileHandle__)->FileName, \
                    (__FileHandle__)->FileName )

#define GET_STREAM_FILE_NAME(__Name__, __FileHandle__, __FileNo__)  \
            sprintf((__Name__), "%s/~%s/%s%02d" STDVMi_STREAM_FILE_EXT, \
                    (__FileHandle__)->PathName, \
                    (__FileHandle__)->FileName, \
                    (__FileHandle__)->FileName, \
                    (__FileNo__) )

#define GET_STREAM_DIR_NAME(__Name__, __FileHandle__)   \
            sprintf((__Name__), "%s/~%s", (__FileHandle__)->PathName, (__FileHandle__)->FileName)


#define GET_INFO_BACKUP_FILE_NAME(__Name__, __FileHandle__)    \
            sprintf((__Name__), "%s/~%s/%s" STDVMi_INFO_FILE_EXT STDVMi_BACKUP_FILE_EXT, \
                    (__FileHandle__)->PathName, (__FileHandle__)->FileName, (__FileHandle__)->FileName)

#define GET_INDEX_BACKUP_FILE_NAME(__Name__, __FileHandle__)  \
            sprintf((__Name__), "%s/~%s/%s" STDVMi_INDEX_FILE_EXT STDVMi_BACKUP_FILE_EXT, \
                    (__FileHandle__)->PathName, \
                    (__FileHandle__)->FileName, \
                    (__FileHandle__)->FileName )


#define MUTEX_CREATE(__Sem__)    __Sem__ = semaphore_create_fifo(1)
#define MUTEX_LOCK(__Sem__)      semaphore_wait(__Sem__)
#define MUTEX_RELEASE(__Sem__)   semaphore_signal(__Sem__)
#define MUTEX_DELETE(__Sem__)    semaphore_delete(__Sem__)

#define STREAM_ACCESS_MUTEX_CREATE(__Handle__)          MUTEX_CREATE((__Handle__)->StreamAccessMutex_p)
#define STREAM_ACCESS_MUTEX_LOCK(__Handle__)            MUTEX_LOCK((__Handle__)->StreamAccessMutex_p)
#define STREAM_ACCESS_MUTEX_RELEASE(__Handle__)         MUTEX_RELEASE((__Handle__)->StreamAccessMutex_p)
#define STREAM_ACCESS_MUTEX_DELETE(__Handle__)          MUTEX_DELETE((__Handle__)->StreamAccessMutex_p)

#define INDEX_ACCESS_MUTEX_CREATE(__Handle__)           MUTEX_CREATE((__Handle__)->IndexAccessMutex_p)
#define INDEX_ACCESS_MUTEX_LOCK(__Handle__)             MUTEX_LOCK((__Handle__)->IndexAccessMutex_p)
#define INDEX_ACCESS_MUTEX_RELEASE(__Handle__)          MUTEX_RELEASE((__Handle__)->IndexAccessMutex_p)
#define INDEX_ACCESS_MUTEX_DELETE(__Handle__)           MUTEX_DELETE((__Handle__)->IndexAccessMutex_p)

#define SHARED_FIELD_ACCESS_MUTEX_CREATE(__Handle__)    MUTEX_CREATE((__Handle__)->SharedFieldAccessMutex_p)
#define SHARED_FIELD_ACCESS_MUTEX_LOCK(__Handle__)      MUTEX_LOCK((__Handle__)->SharedFieldAccessMutex_p)
#define SHARED_FIELD_ACCESS_MUTEX_RELEASE(__Handle__)   MUTEX_RELEASE((__Handle__)->SharedFieldAccessMutex_p)
#define SHARED_FIELD_ACCESS_MUTEX_DELETE(__Handle__)    MUTEX_DELETE((__Handle__)->SharedFieldAccessMutex_p)

#define AUDIO_TRICK_ACCESS_MUTEX_CREATE(__Handle__)    MUTEX_CREATE((__Handle__)->AudioTrickAccessMutex_p)
#define AUDIO_TRICK_ACCESS_MUTEX_LOCK(__Handle__)      MUTEX_LOCK((__Handle__)->AudioTrickAccessMutex_p)
#define AUDIO_TRICK_ACCESS_MUTEX_RELEASE(__Handle__)   MUTEX_RELEASE((__Handle__)->AudioTrickAccessMutex_p)
#define AUDIO_TRICK_ACCESS_MUTEX_DELETE(__Handle__)    MUTEX_DELETE((__Handle__)->AudioTrickAccessMutex_p)

/* Exported Types --------------------------------------------------------- */

/* Different state of record change pid */
typedef enum STDVMi_PidChangeState_e
{
    PID_CHANGE_IDLE,
    PID_CHANGE_PENDING,
    PID_CHANGE_CHANGING
}STDVMi_PidChangeState_t;


typedef enum STDVMi_StreamFileType_e
{
    STREAM_FILE_TYPE_LINEAR,
    STREAM_FILE_TYPE_CIRCULAR,
    STREAM_FILE_TYPE_CIRCULAR_LINEAR
}STDVMi_StreamFileType_t;


typedef struct STDVMi_ProgramInfo_s
{
    U8                      NbOfPids;

    STDVM_StreamData_t      Pids[STDVM_MAX_PIDS];

    S64                     ProgStartPos;                   /* This PIDs start pos in the stream in Packets */

    S64                     ProgEndPos;                     /* This is PIDs end pos in the stream in Packets */
}STDVMi_ProgramInfo_t;


typedef struct STDVMi_StreamInfo_s
{
    U32                     Signature;

    U32                     Version;                        /* Version of STDVMi_StreamInfo_t */

    U32                     NbOfFiles;                      /* No of files recorded for this stream */

    char                    ChannelName[STDVMi_MAX_NAME_LENGTH];

    char                    Description[STDVMi_MAX_NAME_LENGTH];

    /* Circular File handling */
    STDVMi_StreamFileType_t StreamFileType;                 /* type of the stream file linear, circular ... */

    S64                     StreamStartPos;                 /* Stream start position for Circular files */

    S32                     FirstIndexPos;                  /* First index corresponding to record file data */

    /* Circular_Linear Stream handling  */
    S64                     CircularPartSize;               /* Size of the circular portion in the stream */

    S32                     CircularPartIndexEntries;       /* number of index entries in circular portion */

    /* Crop begining of Stream handling  */
    S64                     StreamStartPosAfterCrop;        /* Stream start position after crop beginning of the stream */

    S64                     StreamEndPosAfterCrop;          /* Stream end position after crop end of the stream */

    /* User Storage Record Date and Time */
    U32                     RecordDateTime;                 /* Record DateTime provided by user */

    U32                     NbOfPrograms;                   /* Number of programs recorded in this stream */

    STDVMi_ProgramInfo_t    ProgramInfo[1];                 /* info of the first program, grows as number of programs increases */

}STDVMi_StreamInfo_t;


typedef struct STDVMi_IndexInfo_s
{
    U32                     PacketTimeInMs;

    U32                     Dummy;

    S64                     PacketPosition;

}STDVMi_IndexInfo_t;


typedef struct STDVMi_Handle_s STDVMi_Handle_t;

struct STDVMi_Handle_s
{
    BOOL                    HandleInUse;                    /* Handle is occupied or free */

    void                   *Buffer_p;                       /* buffer for StreamInfo and IndexInfo */

    STDVMi_StreamInfo_t    *StreamInfo_p;                   /* StreamInfo from .ifo file */

    char                    PathName[STDVMi_MAX_NAME_LENGTH];   /* Full path name */

    char                    FileName[STDVMi_MAX_NAME_LENGTH];   /* file name */

    S64                     StreamAbsolutePos;              /* Absolute position in the stream */

    S64                     StreamSize;                     /* Complete Absolute stream size */

    U32                     CurrFileNo;                     /* CurrFile being accessed */

    U32                     CurrFilePos;                    /* Position in the current file */

    S32                     OpenFlags;                      /* Internal open flags */

    vfs_file_t             *StreamFileHandle;               /* Internal Stream file handle for current file */

    semaphore_t            *StreamAccessMutex_p;            /* To protect access of Stream related variables */

    /* Index info handling */
    vfs_file_t             *IndexFileHandle;                /* Internal Index file handle for current file */

    STDVMi_IndexInfo_t     *IndexInfo_p;                    /* Index info buffer aligned */

    U32                     IndexNbOfEntInCache;            /* Number of index entries in cache area */

    U32                     IndexPosInCache;                /* Index position in cache */

    U32                     IndexNbOfEntInFile;             /* Number of index entries in file */

    U32                     IndexPosAbsolute;               /* Index position Absolute */

    U32                     StreamMinTime;                  /* Stream first Index time */

    U32                     StreamMaxTime;                  /* Stream Total time duration */

    S64                     StreamMaxPos;                   /* Stream Max position attained so far */

    semaphore_t            *IndexAccessMutex_p;             /* To protect access of Index related variables */

    /* Limiting File size */
    S64                     MaxStreamSize;                  /* permitted stream size in bytes */

    U32                     MaxIndexEntries;                /* Max index file entries, calculated from MaxStreamSize */

    STDVMi_Handle_t        *SameFileHandle_p;               /* other File handle opened for the same file */

    semaphore_t            *SharedFieldAccessMutex_p;       /* To protect access to field SameFileHandle_p */

    /* Needed for Read Write pointer checkings handling */
    BOOL                    RdCloseToWrNotified;            /* if Rd close to wr is already notified */

    S64                     ElapsedNumPackets;              /* Packets written since last index file update */

    U32                     ElapsedTimeInMs;                /* Time elapsed since last index file update */

    /* Needed for Circular file  */
    STDVMi_ProgramInfo_t    FirstProgramInfo;               /* ProgramInfo of the first available program in circular files */

    /* WA for a seek issue with circular files */
    BOOL                    FirstJump;                      /* TRUE for the first Jump done by PRM when starting playback */

    STPRM_Handle_t          PRM_Handle;                     /* PRM_Handle for further ref */

    STDVM_ObjectType_t      ObjectType;                     /* ObjectType for further ref */

#ifdef ENABLE_MP3_PLAYBACK
    /* MP3_Handle for further ref */
    MP3PB_Handle_t          MP3_Handle;

    BOOL                    PlayStartByPRM;
#endif

    /* for record change PID */
    U32                     CurrentProgNo;                  /* current program no in the recorded stream */

    STDVMi_PidChangeState_t PidChangeState;

    /* for audio trick mode */
    semaphore_t            *AudioTrickAccessMutex_p;        /* To protect Access to Audio Trick mode related variables */

    BOOL                    AudioOnlyProg;                  /* Program with no video */

    BOOL                    AudioInTrickMode;               /* In case of sspeed is not equal 100 && 200 */

    U32                     AudioTrickStartTime;            /* Time when enter in audiotrick mode */

    U32                     CurrentTime;                    /* current time in audiotrick mode */

    S32                     CurrentSpeed;                   /* Current Speed in audiotrick mode */

    BOOL                    AudioInPause;                   /* Pause in audio only mode */

    BOOL                    AudioTrickEndOfFile;            /* To notify END of file in STDVM for audiotrick mode */

    /* for Backward trickmode time jitter */
    BOOL                    BackwardTrick;                  /* TRUE for backard trickmodes */

    U32                     BackwardPrevTimeInMs;           /* returned in Backward trickmodes when there is a jitter */

    /* for En/Decrypt Support */
    BOOL                    EnableCrypt;

    STDVM_Key_t             CryptKey;

    void                   *TSBufferStart;

    void                   *TSBufferEnd;

};


/* Exported Functions ----------------------------------------------------- */

#endif /*__DVMINT_H*/

/* EOF --------------------------------------------------------------------- */

