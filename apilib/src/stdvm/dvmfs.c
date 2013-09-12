/*****************************************************************************

File Name  : dvmfs.c

Description: Filesystem Callback functions for STPRM

Copyright (C) 2006 STMicroelectronics

*****************************************************************************/

/* Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvmfs.h"
#include "dvmindex.h"
#include "handle.h"
#include "event.h"
#ifdef STDVM_ENABLE_CRYPT
 #include "dvmcrypt.h"
#endif

#ifdef ENABLE_TRACE
 #include "trace.h"
#endif


/* Private Constants ------------------------------------------------------ */
#ifdef SMALLER_STREAM_FILES
    #ifdef STDVM_SUB_FILE_SIZE
        #define FS_MAX_SUB_FILE_SIZE        (((STDVM_SUB_FILE_SIZE) / STDVMi_TS_RECORD_CHUNK_SIZE) * STDVMi_TS_RECORD_CHUNK_SIZE)
    #else
        #define FS_MAX_SUB_FILE_SIZE        (0x125C000)
    #endif
#else
    #define FS_MAX_SUB_FILE_SIZE        ((U32)(0x7FFFFFFF / STDVMi_TS_RECORD_CHUNK_SIZE) * STDVMi_TS_RECORD_CHUNK_SIZE) /*7FF70000*/
#endif

#define FS_NULL_HANDLE              ((U32)NULL)

#define FS_SUCCESS                  (0)
#define FS_FAILURE                  ((U32)STPRM_RETURN_FS_ERROR)

#define DEFAULT_PROGRAM             (0)

#define HDD_SECTOR_SIZE             (512)

#define INTERMEDIATE_BUFFER_SIZE    ((STDVM_PACKET_SIZE)*HDD_SECTOR_SIZE*8)

#define STDVM_MIN_DISK_SPACE        (STDVMi_TS_RECORD_CHUNK_SIZE)

#define DVB_SOP                     (0x47)

#define NB_OF_PACKETS_TO_CHECK      (3)

#define OSPLUS_FILE_CACHE_SECTORS   (64)    /* Have to read atleast these many sectors otherwise OS+ reads to its CACHE */

/* defined in linuxwrap.h for Linux */
#ifndef FS_PARTITION_FREE_SIZE
    #define FS_PARTITION_FREE_SIZE(__Info__)    ((__Info__).free_blocks)
#endif

#ifndef DIRENT_IS_FILE
    #define DIRENT_IS_FILE(__DirEnt__)          (((__DirEnt__)->mode & VFS_MODE_DIR) == 0)
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Variables ------------------------------------------------------ */

/* Private Macros --------------------------------------------------------- */

#define GET_PATH_AND_FILE_NAME(__StreamName__, __FileHandle__)   \
            {   \
                char *Src_p   = (char *)(__StreamName__),   \
                     *Dest_p  = (__FileHandle__)->PathName, \
                     *Slash_p = Dest_p;                     \
                \
                do                          \
                {                           \
                    *Dest_p++ = *Src_p++;   \
                                            \
                    if(*Src_p == '/')       \
                        Slash_p = Dest_p;   \
                                            \
                }while(*Src_p != '\0');     \
                *Dest_p  = '\0';            \
                *Slash_p = '\0';            \
                \
                strcpy((__FileHandle__)->FileName, Slash_p+1);  \
            }

#define GET_ROOT_DIR(__FileHandle__, __RootDir__)   \
            {   \
                char *Src_p   = (__FileHandle__)->PathName,     \
                     *Dest_p  = (__RootDir__);                  \
                \
                do                          \
                {                           \
                    *Dest_p++ = *Src_p++;   \
                }while((*Src_p != '/') && (*Src_p != '\0'));    \
                *Dest_p  = '\0';            \
            }


/* debug options */
#if defined(ENABLE_TRACE)
    #define DVMFS_Error(__X__)          TraceBuffer(__X__)
    #define DVMFS_Trace(__X__)          TraceBuffer(__X__)
#elif defined(STDVM_DEBUG)
    #define DVMFS_Error(__X__)          STTBX_Print(__X__)
    #define DVMFS_Trace(__X__)          /*nop*/
#else
    #define DVMFS_Error(__X__)          /*nop*/
    #define DVMFS_Trace(__X__)          /*nop*/
#endif


/* Private Function prototypes -------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

/* Prototypes ------------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */

/*******************************************************************************
Name         : STDVMi_GetStreamSize()

Description  : internal function to calculate stream size

Parameters   : STDVMi_Handle_t *FileHandle_p        DVM File handle

Return Value :
*******************************************************************************/
static void STDVMi_GetStreamSize(STDVMi_Handle_t *FileHandle_p)
{
    char                    LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_stat_t              Status;
    S64                     CircularPartNbOfFiles;


    if((FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||   /* linear file or circular not wrapped */
       (I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPos)))
    {
        /* last file size */
        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->StreamInfo_p->NbOfFiles - 1);
        vfs_stat(LocalName, &Status);

        /* add all */
        I64_SetValue(FS_MAX_SUB_FILE_SIZE, 0, FileHandle_p->StreamSize);
        I64_MulLit(FileHandle_p->StreamSize, FileHandle_p->StreamInfo_p->NbOfFiles - 1, FileHandle_p->StreamSize);
        I64_AddLit(FileHandle_p->StreamSize, Status.st_size, FileHandle_p->StreamSize);
    }
    else if(FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
    {
        FileHandle_p->StreamSize = FileHandle_p->StreamInfo_p->CircularPartSize;
    }
    else /* circular_linear */
    {
        /* number of files in circular part */
        I64_AddLit(FileHandle_p->StreamInfo_p->CircularPartSize, (FS_MAX_SUB_FILE_SIZE - 1), CircularPartNbOfFiles);
        I64_DivLit(CircularPartNbOfFiles, FS_MAX_SUB_FILE_SIZE, CircularPartNbOfFiles);

        /* linear part last file size */
        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->StreamInfo_p->NbOfFiles - 1);
        vfs_stat(LocalName, &Status);

        /* linear part size */
        I64_SetValue(FS_MAX_SUB_FILE_SIZE, 0, FileHandle_p->StreamSize);
        I64_MulLit(FileHandle_p->StreamSize,
                   FileHandle_p->StreamInfo_p->NbOfFiles - CircularPartNbOfFiles.LSW - 1,
                   FileHandle_p->StreamSize);
        I64_AddLit(FileHandle_p->StreamSize, Status.st_size, FileHandle_p->StreamSize);

        /* add all */
        I64_Add(FileHandle_p->StreamSize, FileHandle_p->StreamInfo_p->CircularPartSize, FileHandle_p->StreamSize);
    }
}


/*******************************************************************************
Name         : STDVMi_FileOpen()

Description  : File open glue for STPRM

Parameters   : STPRM_Handle_t   PRM_Handle          PRM handle
               U8              *FileName            File name
               STPRM_FSflags_t  Flags               Open Flags
               S64              FileSizeInPackets   Max file size to be fixed

Return Value :
*******************************************************************************/
U32 STDVMi_FileOpen(STPRM_Handle_t PRM_Handle, U8 *FileName, STPRM_FSflags_t Flags, S64 FileSizeInPackets)
{
    char                    LocalName[STDVMi_MAX_NAME_LENGTH];
    S32                     OpenFlags;
    STDVMi_Handle_t        *FileHandle_p;
    vfs_file_t             *InfoFileHandle;
    STDVMi_StreamFileType_t StreamFileType;
    S32                     BytesRdOrWr;


    DVMFS_Trace(("STDVMi_FileOpen(%08X): Name[%s] Flags[%d] MaxSize[%u,%u]\n", PRM_Handle,
            FileName, Flags,
            FileSizeInPackets.MSW, FileSizeInPackets.LSW));

    /* check for circular file */
    if((Flags & STPRM_FS_CIRCULAR) == STPRM_FS_CIRCULAR)
    {
        StreamFileType = STREAM_FILE_TYPE_CIRCULAR;
        Flags         &= ~STPRM_FS_CIRCULAR;
    }
    else
    {
        StreamFileType = STREAM_FILE_TYPE_LINEAR;
    }

    /* Mapping the STPRM Flags type to OSPlus Flags type */
    switch(Flags)
    {
    case STPRM_FS_RD:
        OpenFlags = OSPLUS_O_RDONLY;
        break;

    case STPRM_FS_WR:
        OpenFlags = OSPLUS_O_WRONLY | OSPLUS_O_CREAT;
        break;

    case STPRM_FS_RD|STPRM_FS_WR:
        OpenFlags = OSPLUS_O_RDWR | OSPLUS_O_CREAT;
        break;

    default:
        DVMFS_Error(("File Open Invalid Flags\n"));
        return FS_FAILURE;
    }


    FileHandle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(FileHandle_p == NULL)
    {
        DVMFS_Error(("File Open STDVMi_GetHandleFromPRMHandle failed\n"));
        return FS_FAILURE;
    }

    STREAM_ACCESS_MUTEX_CREATE(FileHandle_p);

    STREAM_ACCESS_MUTEX_LOCK(FileHandle_p);

    /* Seperate File name and dir name */
    GET_PATH_AND_FILE_NAME(FileName, FileHandle_p);

    /* update the OpenFlags needed for Validate */
    FileHandle_p->OpenFlags = OpenFlags;

    if(STDVMi_ValidateOpen(FileHandle_p) != ST_NO_ERROR)
    {
        DVMFS_Error(("File Open STDVMi_ValidateOpen OpenFlags[%02X] failed\n", OpenFlags));
        STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
        return FS_FAILURE;
    }

    if((StreamFileType == STREAM_FILE_TYPE_CIRCULAR) && (I64_IsZero(FileSizeInPackets)))
    {
        DVMFS_Error(("File Open FileSize not specified for circular file\n"));
        STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
        return FS_FAILURE;
    }

    /* calulate MaxStreamSize to update StreamInfo */
    if(!I64_IsZero(FileSizeInPackets))
    {
        S64     Temp;

        /* calculate max stream file size */
        I64_MulLit(FileSizeInPackets, STPRM_PACKET_SIZE, FileHandle_p->MaxStreamSize);
        /* Stream size should be a multiple of STDVMi_TS_RECORD_CHUNK_SIZE */
        I64_DivLit(FileHandle_p->MaxStreamSize, STDVMi_TS_RECORD_CHUNK_SIZE, FileHandle_p->MaxStreamSize);
        I64_MulLit(FileHandle_p->MaxStreamSize, STDVMi_TS_RECORD_CHUNK_SIZE, FileHandle_p->MaxStreamSize);

        /* calculate max index file size */
        I64_DivLit(FileHandle_p->MaxStreamSize, (STDVMi_MINIMUM_BIT_RATE/8), Temp);
        FileHandle_p->MaxIndexEntries = Temp.LSW * STDVMi_INDEX_ENTRIES_PER_SEC;
        /* FileHandle_p->MaxIndexEntries should be multiple of STDVMi_INDEX_INFO_CACHE_SIZE */
        FileHandle_p->MaxIndexEntries = FileHandle_p->MaxIndexEntries + STDVMi_INDEX_INFO_CACHE_SIZE -
                                        (FileHandle_p->MaxIndexEntries % STDVMi_INDEX_INFO_CACHE_SIZE);
    }
    else
    {
        I64_SetValue(0, 0, FileHandle_p->MaxStreamSize);
        FileHandle_p->MaxIndexEntries = 0;
    }

    /* open info file */
    GET_INFO_FILE_NAME(LocalName, FileHandle_p);

    InfoFileHandle = vfs_open(LocalName, OpenFlags);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("File Open(%s) OpenFlags[%02X] failed\n", LocalName, OpenFlags));
        FileHandle_p->StreamInfo_p->Signature = 0;
        STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
        return FS_FAILURE;
    }

    if(Flags & STPRM_FS_WR)
    {
        /* update stream info fields */
        FileHandle_p->StreamInfo_p->Signature       = STDVMi_SIGNATURE;

        FileHandle_p->StreamInfo_p->Version         = STDVMi_STREAM_INFO_VERSION;

        FileHandle_p->StreamInfo_p->NbOfPrograms    = 1;

        FileHandle_p->StreamInfo_p->NbOfFiles       = 1;

        FileHandle_p->StreamInfo_p->StreamFileType  = StreamFileType;

        /* for circular stream and circular_linear stream */
        I64_SetValue(0, 0, FileHandle_p->StreamInfo_p->StreamStartPos);
        FileHandle_p->StreamInfo_p->FirstIndexPos = 0;

        FileHandle_p->StreamInfo_p->CircularPartSize = FileHandle_p->MaxStreamSize;
        FileHandle_p->StreamInfo_p->CircularPartIndexEntries = FileHandle_p->MaxIndexEntries;

        /* for crop stream */
        I64_SetValue(0, 0, FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop);
        I64_SetValue(0, 0, FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop);

        BytesRdOrWr = vfs_write(InfoFileHandle, FileHandle_p->StreamInfo_p, sizeof(STDVMi_StreamInfo_t));

        vfs_close(InfoFileHandle);

        if(BytesRdOrWr != sizeof(STDVMi_StreamInfo_t))
        {
            DVMFS_Error(("Info file write failed %d!=%d\n", BytesRdOrWr, sizeof(STDVMi_StreamInfo_t)));
            FileHandle_p->StreamInfo_p->Signature = 0;
            STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
            return FS_FAILURE;
        }

        GET_STREAM_DIR_NAME(LocalName, FileHandle_p);
        vfs_mkdir(LocalName);

        I64_SetValue(0, 0, FileHandle_p->StreamSize);

#ifdef USE_BACKUP_FILES
        {
            vfs_file_t         *BakFileHandle;

            GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);

            BakFileHandle = vfs_open(LocalName, OpenFlags);
            if(BakFileHandle == NULL)
            {
                DVMFS_Error(("File Open(%s) OpenFlags[%02X] failed\n", LocalName, OpenFlags));
                FileHandle_p->StreamInfo_p->Signature = 0;
                STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
                return FS_FAILURE;
            }

            BytesRdOrWr = vfs_write(BakFileHandle, FileHandle_p->StreamInfo_p, sizeof(STDVMi_StreamInfo_t));

            vfs_close(BakFileHandle);

            if(BytesRdOrWr != sizeof(STDVMi_StreamInfo_t))
            {
                DVMFS_Error(("Info file write failed %d!=%d\n", BytesRdOrWr, sizeof(STDVMi_StreamInfo_t)));
                FileHandle_p->StreamInfo_p->Signature = 0;
                STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
                return FS_FAILURE;
            }
        }
#endif /* USE_BACKUP_FILES */

        FileHandle_p->CurrFilePos = 0;

        FileHandle_p->CurrFileNo  = 0;
        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->CurrFileNo);

        FileHandle_p->StreamFileHandle = vfs_open(LocalName, OpenFlags);
        if(FileHandle_p->StreamFileHandle == NULL)
        {
            DVMFS_Error(("File Open(%s) failed\n", LocalName));
            FileHandle_p->StreamInfo_p->Signature = 0;
            STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
            return FS_FAILURE;
        }
    }
    else
    {
#ifdef USE_BACKUP_FILES
        vfs_close(InfoFileHandle);

        {
            vfs_stat_t  InfStatus,
                        BakStatus;

            GET_INFO_FILE_NAME(LocalName, FileHandle_p);
            if(vfs_stat(LocalName, &InfStatus) != OSPLUS_SUCCESS)
            {
                DVMFS_Error(("File Open(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
                InfStatus.st_size = 0;
            }

            GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
            if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
            {
                DVMFS_Error(("File Open(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
                BakStatus.st_size = 0;
            }

            if((InfStatus.st_size == 0) && (BakStatus.st_size == 0))
            {
                DVMFS_Error(("File Open(%08X) both Index and Backup file does not exist\n", FileHandle_p));
                FileHandle_p->StreamInfo_p->Signature = 0;
                STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
                return FS_FAILURE;
            }

            if(InfStatus.st_size < BakStatus.st_size)   /* use backup */
            {
                GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
            }
            else
            {
                GET_INFO_FILE_NAME(LocalName, FileHandle_p);
            }
        }

        InfoFileHandle = vfs_open(LocalName, OpenFlags);
        if(InfoFileHandle == NULL)
        {
            DVMFS_Error(("File Open(%s) OpenFlags[%02X] failed\n", LocalName, OpenFlags));
            FileHandle_p->StreamInfo_p->Signature = 0;
            STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
            return FS_FAILURE;
        }
#endif /* USE_BACKUP_FILES */

        BytesRdOrWr = vfs_read(InfoFileHandle, FileHandle_p->StreamInfo_p, sizeof(STDVMi_StreamInfo_t));

        vfs_close(InfoFileHandle);

        if(BytesRdOrWr != sizeof(STDVMi_StreamInfo_t))
        {
            DVMFS_Error(("Info file read failed %d!=%d\n", BytesRdOrWr, sizeof(STDVMi_StreamInfo_t)));
            FileHandle_p->StreamInfo_p->Signature = 0;
            STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
            return FS_FAILURE;
        }

        if(FileHandle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
        {
            DVMFS_Error(("File Open(%s) Signature[%08X] failed\n", LocalName, FileHandle_p->StreamInfo_p->Signature));
            FileHandle_p->StreamInfo_p->Signature = 0;
            STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);
            return FS_FAILURE;
        }

        /* calculate stream size */
        STDVMi_GetStreamSize(FileHandle_p);

        /* first stream file number can differ in case of circular and cropped files
         * seek is done at the end of this function
         */
        FileHandle_p->CurrFilePos = 0;
        FileHandle_p->CurrFileNo  = FileHandle_p->StreamInfo_p->NbOfFiles;  /* correct file is opened in Seek */
    }

    /* Reset PID change state and CurrentProgNo */
    FileHandle_p->PidChangeState = PID_CHANGE_IDLE;
    FileHandle_p->CurrentProgNo  = 0;

    FileHandle_p->RdCloseToWrNotified = FALSE;

    FileHandle_p->SameFileHandle_p = STDVMi_GetAlreadyOpenHandle(FileHandle_p);
    if(FileHandle_p->SameFileHandle_p != NULL)
    {
        /* SameFileHandle_p is used in Cicular files and Index buffers to refer the other Index buffer */
        SHARED_FIELD_ACCESS_MUTEX_CREATE(FileHandle_p);

        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);

        /* sharing the same MUTEX */
        FileHandle_p->SameFileHandle_p->SharedFieldAccessMutex_p = FileHandle_p->SharedFieldAccessMutex_p;

        FileHandle_p->SameFileHandle_p->SameFileHandle_p = FileHandle_p;

#if 1
        /* TODO: Check if this would be sufficient
         * case: Read starts when Write is at the end of the buffer has to be DONE
         */
        if(!I64_IsZero(FileHandle_p->SameFileHandle_p->StreamInfo_p->StreamStartPos))
        {
            I64_AddLit(FileHandle_p->SameFileHandle_p->StreamInfo_p->StreamStartPos,
                       STDVMi_TS_RECORD_CHUNK_SIZE,
                       FileHandle_p->StreamInfo_p->StreamStartPos);
        }
        else
#endif
        {
            FileHandle_p->StreamInfo_p->StreamStartPos = FileHandle_p->SameFileHandle_p->StreamInfo_p->StreamStartPos;
        }

        FileHandle_p->MaxStreamSize     = FileHandle_p->SameFileHandle_p->MaxStreamSize;
        FileHandle_p->MaxIndexEntries   = FileHandle_p->SameFileHandle_p->MaxIndexEntries;
        FileHandle_p->StreamAbsolutePos = FileHandle_p->StreamInfo_p->StreamStartPos;

        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);
    }
    else
    {
        if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop))
        {
            FileHandle_p->StreamAbsolutePos = FileHandle_p->StreamInfo_p->StreamStartPos;
        }
        else
        {
            FileHandle_p->StreamAbsolutePos = FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop;
        }
    }

    STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);

    DVMFS_Trace(("\tSTDVMi_FileOpen:Handle(%08X, %08X) T[%d] SizeM[%u,%u] Pos{S[%u,%u] AC[%u,%u] A[%u,%u]}\n\n",
            FileHandle_p, FileHandle_p->SameFileHandle_p,
            FileHandle_p->StreamInfo_p->StreamFileType,
            FileHandle_p->MaxStreamSize.MSW, FileHandle_p->MaxStreamSize.LSW,
            FileHandle_p->StreamInfo_p->StreamStartPos.MSW, FileHandle_p->StreamInfo_p->StreamStartPos.LSW,
            FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop.MSW, FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop.LSW,
            FileHandle_p->StreamAbsolutePos.MSW, FileHandle_p->StreamAbsolutePos.LSW));

    /* Seek to the correct start position in case of Read mode */
    if(Flags & STPRM_FS_RD)
    {
        S64 PacketOffset;

        I64_DivLit(FileHandle_p->StreamAbsolutePos, STPRM_PACKET_SIZE, PacketOffset);

        STDVMi_FileSeek(PRM_Handle, (U32)FileHandle_p, PacketOffset, STPRM_FS_SEEK_SET);
    }

    return (U32)FileHandle_p;
}


/*******************************************************************************
Name         : STDVMi_FileRead()

Description  : Large file read glue for STPRM

Parameters   : STPRM_Handle_t   PRM_Handle          PRM handle
               U32              FileHandle          File handle returned in file open
               U8              *Buffer              Buffer to read data from
               U32              SizeInPackets       Size to read in packets

Return Value :
*******************************************************************************/
U32 STDVMi_FileRead(STPRM_Handle_t PRM_Handle, U32 FileHandle, U8 *Buffer, U32 SizeInPackets)
{
    STDVMi_Handle_t    *FileHandle_p = (STDVMi_Handle_t *)FileHandle;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    S32                 BytesRead,
                        TotalBytesRead = 0;
    U32                 Size = SizeInPackets*STPRM_PACKET_SIZE;


    DVMFS_Trace(("STDVMi_FileRead(%08X): CurPos[%u,%u] SizeInPack[%d] Type[%d]\n", FileHandle_p,
            FileHandle_p->StreamAbsolutePos.MSW, FileHandle_p->StreamAbsolutePos.LSW,
            SizeInPackets, FileHandle_p->StreamInfo_p->StreamFileType));

    STREAM_ACCESS_MUTEX_LOCK(FileHandle_p);

    /* check if the file read has already reached the cropped end */
    if((!I64_IsZero(FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)) &&
       (I64_IsGreaterOrEqual(FileHandle_p->StreamAbsolutePos, FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)))
    {
        DVMFS_Error(("Read(%08X) EndPosAfterCrop[%u,%u] reached\n", FileHandle_p,
                FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.MSW,
                FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.LSW));
        STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
        return 0;
    }

    if(FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR)
    {
        while(TotalBytesRead < Size)
        {
            if(FileHandle_p->CurrFilePos >= FS_MAX_SUB_FILE_SIZE)
            {
                vfs_close(FileHandle_p->StreamFileHandle);

                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->CurrFileNo+1);

                DVMFS_Trace(("\tRead File end opening new file %s\n", LocalName));

                FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                if(FileHandle_p->StreamFileHandle == NULL)
                {
                    DVMFS_Error(("Read(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                    break;
                }

                FileHandle_p->CurrFileNo++;
                FileHandle_p->CurrFilePos = 0;
            }

            DVMFS_Trace(("\tRead(%d)\n", Size-TotalBytesRead));

            BytesRead = vfs_read(FileHandle_p->StreamFileHandle, Buffer+TotalBytesRead, Size-TotalBytesRead);
            if(BytesRead <= 0)
            {
                DVMFS_Error(("Read(%08X, %d)=%d failed\n", FileHandle_p, Size-TotalBytesRead, BytesRead));
                if(BytesRead < 0)
                    STDVMi_NotifyErrorEvent(STDVM_EVT_ERROR, STDVM_PLAY_READ_ERROR, FileHandle_p);
                break;
            }

            FileHandle_p->CurrFilePos += BytesRead;

            TotalBytesRead += BytesRead;

            /* Update absolute file position */
            I64_AddLit(FileHandle_p->StreamAbsolutePos, BytesRead, FileHandle_p->StreamAbsolutePos);

            /* check if the file read has already reached the cropped end */
            if((!I64_IsZero(FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)) &&
               (I64_IsGreaterOrEqual(FileHandle_p->StreamAbsolutePos, FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)))
            {
                DVMFS_Error(("Read(%08X) EndPosAfterCrop[%u,%u] reached\n", FileHandle_p,
                        FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.MSW,
                        FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.LSW));
                break;
            }

            /* Check for Read & Write differences and notify corresponding events */
            if(FileHandle_p->SameFileHandle_p != NULL)
            {
                S64     StreamReadPtr,
                        StreamWritePtr,
                        ReadWriteDiff;

                I64_SetValue(0, 0, ReadWriteDiff);

                StreamReadPtr  = FileHandle_p->StreamAbsolutePos;

                SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                StreamWritePtr = FileHandle_p->SameFileHandle_p->StreamAbsolutePos;
                SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                DVMFS_Trace(("\tRead StreamReadPtr[%u,%u] StreamWritePtr[%u,%u]\n",
                        StreamReadPtr.MSW, StreamReadPtr.LSW,
                        StreamWritePtr.MSW, StreamWritePtr.LSW));

                if(I64_IsLessOrEqual(StreamReadPtr, StreamWritePtr))
                {
                    I64_Sub(StreamWritePtr, StreamReadPtr, ReadWriteDiff);
                }
                else
                {
                    DVMFS_Error(("Read(%08X) StreamReadPtr EXCEEDS [%u,%u] StreamWritePtr[%u,%u]\n", FileHandle_p,
                            StreamReadPtr.MSW, StreamReadPtr.LSW,
                            StreamWritePtr.MSW, StreamWritePtr.LSW));
                }

                /* check for STDVM_EVT_WRITE_CLOSE_TO_READ_ID here */
                if(FileHandle_p->RdCloseToWrNotified == FALSE)
                {
                    I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p->SameFileHandle_p), ReadWriteDiff);
                    if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW <= STDVMi_GetReadWriteThresholdInMs()))
                    {
                        FileHandle_p->RdCloseToWrNotified = TRUE;

                        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                        FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = TRUE;
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                        DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_CLOSE_TO_READ\n"));

                        STDVMi_NotifyEvent(STDVM_EVT_WRITE_CLOSE_TO_READ, FileHandle_p);
                    }
                }
                else if(FileHandle_p->RdCloseToWrNotified == TRUE)
                {
                    I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p->SameFileHandle_p), ReadWriteDiff);
                    if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW >= STDVMi_GetReadWriteThresholdOKInMs()))
                    {
                        FileHandle_p->RdCloseToWrNotified = FALSE;

                        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                        FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = FALSE;
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                        DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_TO_READ_MARGIN_OK\n"));

                        STDVMi_NotifyEvent(STDVM_EVT_WRITE_TO_READ_MARGIN_OK, FileHandle_p);
                    }
                }
            }
        }
    }
    else if(FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
    {
        S64     CircularStreamReadPtr,
                CircularStreamWritePtr,
                StreamSize,
                Temp;

        /* check if the current Packet position is still valid, sanity check for timeshift */
        if(I64_IsLessThan(FileHandle_p->StreamAbsolutePos, FileHandle_p->StreamInfo_p->StreamStartPos))
        {
            DVMFS_Error(("Read(%08X) StreamAbsolutePosition[%u,%u] < StreamStartPos[%u,%u]\n",
                    FileHandle_p, FileHandle_p->StreamAbsolutePos.MSW, FileHandle_p->StreamAbsolutePos.LSW,
                    FileHandle_p->StreamInfo_p->StreamStartPos.MSW, FileHandle_p->StreamInfo_p->StreamStartPos.LSW));
            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
            return 0;
        }

        /* calculate the end position */
        I64_AddLit(FileHandle_p->StreamAbsolutePos, Size, CircularStreamReadPtr);

        /* calculate stream max position in bytes */
        I64_Add(FileHandle_p->StreamInfo_p->StreamStartPos, FileHandle_p->StreamSize, Temp);

        if(I64_IsGreaterThan(CircularStreamReadPtr, Temp))
        {
            DVMFS_Trace(("\tRead exceeds EndPos StreamStart[%u,%u] StreamEnd[%u,%u] CurPos[%u,%u] ReqEnd[%u,%u]\n",
                    FileHandle_p->StreamInfo_p->StreamStartPos.MSW, FileHandle_p->StreamInfo_p->StreamStartPos.LSW,
                    Temp.MSW, Temp.LSW,
                    FileHandle_p->StreamAbsolutePos.MSW, FileHandle_p->StreamAbsolutePos.LSW,
                    CircularStreamReadPtr.MSW, CircularStreamReadPtr.LSW));

            I64_Sub(Temp, FileHandle_p->StreamAbsolutePos, Temp);
            Size = Temp.LSW;    /* read till the END position */
        }

        while(TotalBytesRead < Size)
        {
            CircularStreamReadPtr = FileHandle_p->StreamAbsolutePos;
            StreamSize            = FileHandle_p->StreamSize;

            DVMFS_Trace(("\tRead StreamReadPtr[%u,%u] StreamSize[%u,%u]\n",
                    CircularStreamReadPtr.MSW, CircularStreamReadPtr.LSW,
                    StreamSize.MSW, StreamSize.LSW));

            /* CircularStreamReadPtr % StreamSize */
            while(I64_IsGreaterThan(CircularStreamReadPtr, StreamSize))
            {
                I64_Sub(CircularStreamReadPtr, StreamSize, CircularStreamReadPtr);
            }

            if(FileHandle_p->SameFileHandle_p != NULL)
            {
                SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                CircularStreamWritePtr = FileHandle_p->SameFileHandle_p->StreamAbsolutePos;
                StreamSize             = FileHandle_p->SameFileHandle_p->StreamSize;
                SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                DVMFS_Trace(("\tRead StreamWritePtr[%u,%u] StreamSize[%u,%u]\n",
                        CircularStreamWritePtr.MSW, CircularStreamWritePtr.LSW,
                        StreamSize.MSW, StreamSize.LSW));

                /* CircularStreamWritePtr % StreamSize */
                while(I64_IsGreaterThan(CircularStreamWritePtr, StreamSize))
                {
                    I64_Sub(CircularStreamWritePtr, StreamSize, CircularStreamWritePtr);
                }

                /* Check if the Read exceeds Write */
                {
                    S64     ReadWriteDiff;

                    if(I64_IsEqual(CircularStreamReadPtr, CircularStreamWritePtr))
                    {
                        I64_SetValue(0, 0, ReadWriteDiff);

                        /* RD == WR -> RD has catched up no more data to read */
                        DVMFS_Trace(("\tRead RD = WR StreamWritePtr[%u,%u] StreamWritePtr[%u,%u]\n",
                                CircularStreamReadPtr.MSW, CircularStreamReadPtr.LSW,
                                CircularStreamWritePtr.MSW, CircularStreamWritePtr.LSW));
                        break;
                    }
                    else if(I64_IsLessThan(CircularStreamReadPtr, CircularStreamWritePtr))
                    {
                        I64_Sub(CircularStreamWritePtr, CircularStreamReadPtr, ReadWriteDiff);

                        I64_SetValue((Size-TotalBytesRead), 0, Temp);
                        if(I64_IsLessThan(ReadWriteDiff, Temp))
                        {
                            Size = TotalBytesRead + ReadWriteDiff.LSW;  /* reduce the Size to read to data available */
                        }
                    }
                    else
                    {
                        I64_Sub(StreamSize, CircularStreamReadPtr, ReadWriteDiff);
                        I64_Add(ReadWriteDiff, CircularStreamWritePtr, ReadWriteDiff);

                        I64_SetValue((Size-TotalBytesRead), 0, Temp);
                        if(I64_IsLessThan(ReadWriteDiff, Temp))
                        {
                            Size = TotalBytesRead + ReadWriteDiff.LSW;  /* reduce the Size to read to data available */
                        }
                    }

                    /* check for STDVM_EVT_WRITE_CLOSE_TO_READ_ID here */
                    if(FileHandle_p->RdCloseToWrNotified == FALSE)
                    {
                        I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p->SameFileHandle_p), ReadWriteDiff);
                        if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW <= STDVMi_GetReadWriteThresholdInMs()))
                        {
                            FileHandle_p->RdCloseToWrNotified = TRUE;

                            SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                            FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = TRUE;
                            SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                            DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_CLOSE_TO_READ\n"));

                            STDVMi_NotifyEvent(STDVM_EVT_WRITE_CLOSE_TO_READ, FileHandle_p);
                        }
                    }
                    else if(FileHandle_p->RdCloseToWrNotified == TRUE)
                    {
                        I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p->SameFileHandle_p), ReadWriteDiff);
                        if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW >= STDVMi_GetReadWriteThresholdOKInMs()))
                        {
                            FileHandle_p->RdCloseToWrNotified = FALSE;

                            SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                            FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = FALSE;
                            SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                            DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_TO_READ_MARGIN_OK\n"));

                            STDVMi_NotifyEvent(STDVM_EVT_WRITE_TO_READ_MARGIN_OK, FileHandle_p);
                        }
                    }
                }
            }

            /* reached end of the last file move to 0 position */
            if(I64_IsGreaterOrEqual(CircularStreamReadPtr, StreamSize))
            {
                vfs_close(FileHandle_p->StreamFileHandle);

                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, 0);

                DVMFS_Trace(("\tRead reached END of Stream WRAP AROUND, file %s\n", LocalName));

                FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                if(FileHandle_p->StreamFileHandle == NULL)
                {
                    DVMFS_Error(("Read(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                    break;
                }

                FileHandle_p->CurrFileNo  = 0;
                FileHandle_p->CurrFilePos = 0;

                I64_SetValue(0, 0, CircularStreamReadPtr);
            }

            if(FileHandle_p->CurrFilePos >= FS_MAX_SUB_FILE_SIZE)
            {
                vfs_close(FileHandle_p->StreamFileHandle);

                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->CurrFileNo+1);

                FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                if(FileHandle_p->StreamFileHandle == NULL)
                {
                    DVMFS_Error(("Read(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                    break;
                }

                FileHandle_p->CurrFileNo++;
                FileHandle_p->CurrFilePos = 0;
            }

            BytesRead = vfs_read(FileHandle_p->StreamFileHandle, Buffer+TotalBytesRead, Size-TotalBytesRead);
            if(BytesRead <= 0)
            {
                DVMFS_Error(("Read(%08X, %d)=%d failed\n", FileHandle_p, Size-TotalBytesRead, BytesRead));
                if(BytesRead < 0)
                    STDVMi_NotifyErrorEvent(STDVM_EVT_ERROR, STDVM_PLAY_READ_ERROR, FileHandle_p);
                break;
            }

            FileHandle_p->CurrFilePos += BytesRead;

            TotalBytesRead += BytesRead;

            /* Update absolute file position */
            I64_AddLit(FileHandle_p->StreamAbsolutePos, BytesRead, FileHandle_p->StreamAbsolutePos);

            /* check if the file read has already reached the cropped end */
            if((!I64_IsZero(FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)) &&
               (I64_IsGreaterOrEqual(FileHandle_p->StreamAbsolutePos, FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)))
            {
                DVMFS_Error(("Read(%08X) EndPosAfterCrop[%u,%u] reached\n", FileHandle_p,
                        FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.MSW,
                        FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.LSW));
                break;
            }
        }
    }
    else    /* circular_linear */
    {
        S32     Bytes2Read;
        S64     CircularPartEndPos,
                CircularStreamReadPtr,
                Temp;

        while(TotalBytesRead < Size)
        {
            /* set default size to read */
            Bytes2Read = Size - TotalBytesRead;

            I64_Add(FileHandle_p->StreamInfo_p->StreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize, CircularPartEndPos);

            /* position is still in circular part of the stream */
            if(I64_IsLessOrEqual(FileHandle_p->StreamAbsolutePos, CircularPartEndPos))
            {
                I64_AddLit(FileHandle_p->StreamAbsolutePos, Bytes2Read, Temp);
                if(I64_IsGreaterThan(Temp, CircularPartEndPos))
                {
                    /* reached end of the circular part of the stream switch to linear part */
                    if(I64_IsGreaterOrEqual(FileHandle_p->StreamAbsolutePos, CircularPartEndPos))
                    {
                        S64     CirPartLastFileNb;

                        I64_SubLit(FileHandle_p->StreamInfo_p->CircularPartSize, 1, CirPartLastFileNb);
                        I64_DivLit(CirPartLastFileNb, FS_MAX_SUB_FILE_SIZE, CirPartLastFileNb);

                        vfs_close(FileHandle_p->StreamFileHandle);

                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, CirPartLastFileNb.LSW + 1);

                        DVMFS_Trace(("\tRead reached END of Stream in Circular part switch to linear part, file %s\n", LocalName));

                        FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                        if(FileHandle_p->StreamFileHandle == NULL)
                        {
                            DVMFS_Error(("Read(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                            break;
                        }

                        FileHandle_p->CurrFileNo  = CirPartLastFileNb.LSW + 1;
                        FileHandle_p->CurrFilePos = 0;
                    }
                    else
                    {
                        I64_Sub(CircularPartEndPos, FileHandle_p->StreamAbsolutePos, Temp);
                        Bytes2Read = Temp.LSW;
                    }
                }
                else
                {
                    CircularStreamReadPtr = FileHandle_p->StreamAbsolutePos;

                    /* CircularStreamReadPtr % StreamSize */
                    while(I64_IsGreaterThan(CircularStreamReadPtr, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        I64_Sub(CircularStreamReadPtr, FileHandle_p->StreamInfo_p->CircularPartSize, CircularStreamReadPtr);
                    }

                    /* reached end of the last file move to 0 position */
                    if(I64_IsGreaterOrEqual(CircularStreamReadPtr, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        vfs_close(FileHandle_p->StreamFileHandle);

                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, 0);

                        DVMFS_Trace(("\tRead reached END of Stream WRAP AROUND, file %s\n", LocalName));

                        FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                        if(FileHandle_p->StreamFileHandle == NULL)
                        {
                            DVMFS_Error(("Read(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                            break;
                        }

                        FileHandle_p->CurrFileNo  = 0;
                        FileHandle_p->CurrFilePos = 0;
                    }
                }
            }

            if(FileHandle_p->CurrFilePos >= FS_MAX_SUB_FILE_SIZE)
            {
                vfs_close(FileHandle_p->StreamFileHandle);

                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->CurrFileNo+1);

                DVMFS_Trace(("\tRead File end opening new file %s\n", LocalName));

                FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                if(FileHandle_p->StreamFileHandle == NULL)
                {
                    DVMFS_Error(("Read(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                    break;
                }

                FileHandle_p->CurrFileNo++;
                FileHandle_p->CurrFilePos = 0;
            }

            DVMFS_Trace(("\tRead(%d)\n", Size-TotalBytesRead));

            BytesRead = vfs_read(FileHandle_p->StreamFileHandle, Buffer+TotalBytesRead, Bytes2Read);
            if(BytesRead <= 0)
            {
                DVMFS_Error(("Read(%08X, %d)=%d failed\n", FileHandle_p, Size-TotalBytesRead, BytesRead));
                if(BytesRead < 0)
                    STDVMi_NotifyErrorEvent(STDVM_EVT_ERROR, STDVM_PLAY_READ_ERROR, FileHandle_p);
                break;
            }

            FileHandle_p->CurrFilePos += BytesRead;

            TotalBytesRead += BytesRead;

            /* Update absolute file position */
            I64_AddLit(FileHandle_p->StreamAbsolutePos, BytesRead, FileHandle_p->StreamAbsolutePos);

            /* check if the file read has already reached the cropped end */
            if((!I64_IsZero(FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)) &&
               (I64_IsGreaterOrEqual(FileHandle_p->StreamAbsolutePos, FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop)))
            {
                DVMFS_Error(("Read(%08X) EndPosAfterCrop[%u,%u] reached\n", FileHandle_p,
                        FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.MSW,
                        FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop.LSW));
                break;
            }

            /* Check for Read & Write differences and notify corresponding events */
            if(FileHandle_p->SameFileHandle_p != NULL)
            {
                S64     StreamReadPtr,
                        StreamWritePtr,
                        ReadWriteDiff;

                I64_SetValue(0, 0, ReadWriteDiff);

                StreamReadPtr  = FileHandle_p->StreamAbsolutePos;

                SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                StreamWritePtr = FileHandle_p->SameFileHandle_p->StreamAbsolutePos;
                SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                DVMFS_Trace(("\tRead StreamReadPtr[%u,%u] StreamWritePtr[%u,%u]\n",
                        StreamReadPtr.MSW, StreamReadPtr.LSW,
                        StreamWritePtr.MSW, StreamWritePtr.LSW));

                if(I64_IsLessOrEqual(StreamReadPtr, StreamWritePtr))
                {
                    I64_Sub(StreamWritePtr, StreamReadPtr, ReadWriteDiff);
                }
                else
                {
                    DVMFS_Error(("Read(%08X) StreamReadPtr EXCEEDS [%u,%u] StreamWritePtr[%u,%u]\n", FileHandle_p,
                            StreamReadPtr.MSW, StreamReadPtr.LSW,
                            StreamWritePtr.MSW, StreamWritePtr.LSW));
                }

                /* check for STDVM_EVT_WRITE_CLOSE_TO_READ_ID here */
                if(FileHandle_p->RdCloseToWrNotified == FALSE)
                {
                    I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p->SameFileHandle_p), ReadWriteDiff);
                    if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW <= STDVMi_GetReadWriteThresholdInMs()))
                    {
                        FileHandle_p->RdCloseToWrNotified = TRUE;

                        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                        FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = TRUE;
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                        DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_CLOSE_TO_READ\n"));

                        STDVMi_NotifyEvent(STDVM_EVT_WRITE_CLOSE_TO_READ, FileHandle_p);
                    }
                }
                else if(FileHandle_p->RdCloseToWrNotified == TRUE)
                {
                    I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p->SameFileHandle_p), ReadWriteDiff);
                    if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW >= STDVMi_GetReadWriteThresholdOKInMs()))
                    {
                        FileHandle_p->RdCloseToWrNotified = FALSE;

                        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                        FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = FALSE;
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                        DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_TO_READ_MARGIN_OK\n"));

                        STDVMi_NotifyEvent(STDVM_EVT_WRITE_TO_READ_MARGIN_OK, FileHandle_p);
                    }
                }
            }
        }
    }

    STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);

#ifdef STDVM_ENABLE_CRYPT
    if(FileHandle_p->EnableCrypt)
    {
        if(STDVMi_Decrypt(FileHandle_p, Buffer, Buffer, TotalBytesRead) != ST_NO_ERROR)
        {
            DVMFS_Error(("Read(%08X) STDVMi_Decrypt(%08X,%d) failed\n", FileHandle_p, Buffer, TotalBytesRead));
            return FS_FAILURE;
        }
    }
#endif

    DVMFS_Trace(("\tSTDVMi_FileRead(%08X)=%d\n\n", FileHandle_p, TotalBytesRead));

    return TotalBytesRead/STPRM_PACKET_SIZE;
}


/*******************************************************************************
Name         : STDVMi_FileWrite()

Description  : Large file write glue for STPRM

Parameters   : STPRM_Handle_t   PRM_Handle          PRM handle
               U32              FileHandle          File handle returned in file open
               U8              *Buffer              Buffer to write
               U32              SizeInPackets       Size to write in packets

Return Value :
*******************************************************************************/
U32 STDVMi_FileWrite(STPRM_Handle_t PRM_Handle, U32 FileHandle, U8 *Buffer, U32 SizeInPackets)
{
    STDVMi_Handle_t    *FileHandle_p = (STDVMi_Handle_t *)FileHandle;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    S32                 BytesWritten,
                        TotalBytesWritten = 0,
                        Bytes2Write;
    U32                 Size = SizeInPackets*STPRM_PACKET_SIZE;
    S64                 TempSize;


    DVMFS_Trace(("STDVMi_FileWrite(%08X): CurPos[%u,%u] SizeInPack[%d] Type[%d]\n", FileHandle_p,
            FileHandle_p->StreamAbsolutePos.MSW, FileHandle_p->StreamAbsolutePos.LSW,
            SizeInPackets, FileHandle_p->StreamInfo_p->StreamFileType));

#ifdef STDVM_ENABLE_CRYPT
    if(FileHandle_p->EnableCrypt)
    {
        if(Size > STDVMi_TS_RECORD_CHUNK_SIZE)
        {
            DVMFS_Error(("Write(%08X) Size[%d] larger than Crypt buffer size[%d]\n",FileHandle_p,Size,STDVMi_TS_RECORD_CHUNK_SIZE));
            return FS_FAILURE;
        }

        /* Encrytption should not be done in the same source buffer since in case of timeshift STPRM uses the data even after write
           This buffer access must be protected by a MUTEX but since all HDD calls from STPRM are invoked by a single task
           it should not be a problem
         */
        if(STDVMi_Encrypt(FileHandle_p, Buffer, STDVMi_CryptBufferAligned, Size) != ST_NO_ERROR)
        {
            DVMFS_Error(("Write(%08X) STDVMi_Encrypt(%08X,%d) failed\n", FileHandle_p, Buffer, Size));
            return FS_FAILURE;
        }
        Buffer = STDVMi_CryptBufferAligned;
    }
#endif

    STREAM_ACCESS_MUTEX_LOCK(FileHandle_p);

    if((FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
       (FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR))
    {
        /* check for max file size */
        if(!I64_IsZero(FileHandle_p->MaxStreamSize))
        {
            I64_AddLit(FileHandle_p->StreamSize, Size, TempSize);
            if(I64_IsGreaterThan(TempSize, FileHandle_p->MaxStreamSize))
            {
                if(I64_IsGreaterOrEqual(FileHandle_p->StreamSize, FileHandle_p->MaxStreamSize))
                {
                    /* max file size reached */
                    DVMFS_Trace(("\tSTDVMi_FileWrite(%08X): MaxFileSize[%u,%u] Reached\n", FileHandle_p,
                            FileHandle_p->MaxStreamSize.MSW, FileHandle_p->MaxStreamSize.LSW));
                    STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);

                    STDVMi_NotifyEvent(STDVM_EVT_END_OF_FILE, FileHandle_p);
                    return 0;   /* don't write anymore since max file size is reached */
                }
                else
                {
                    I64_Sub(FileHandle_p->MaxStreamSize, FileHandle_p->StreamSize, TempSize);
                    Size = (TempSize.LSW/STPRM_PACKET_SIZE)*STPRM_PACKET_SIZE;
                }
            }
        }

        while(TotalBytesWritten < Size)
        {
            if(FileHandle_p->CurrFilePos >= FS_MAX_SUB_FILE_SIZE)
            {
                vfs_close(FileHandle_p->StreamFileHandle);

                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->CurrFileNo+1);

                FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                if(FileHandle_p->StreamFileHandle == NULL)
                {
                    DVMFS_Error(("Write(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                    break;
                }

                FileHandle_p->CurrFileNo++;
                FileHandle_p->CurrFilePos = 0;

                FileHandle_p->StreamInfo_p->NbOfFiles++;

                /* update INFO file */
                STDVMi_UpdateStreamInfoToDisk(FileHandle_p);

                /* if a another file is opened for the same stream update NbOfFiles for that also */
                if(FileHandle_p->SameFileHandle_p != NULL)
                {
                    SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                    FileHandle_p->SameFileHandle_p->StreamInfo_p->NbOfFiles = FileHandle_p->StreamInfo_p->NbOfFiles;
                    SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);
                }

                Bytes2Write = FS_MAX_SUB_FILE_SIZE;
            }
            else
            {
                Bytes2Write = FS_MAX_SUB_FILE_SIZE - FileHandle_p->CurrFilePos;
            }

            if(FileHandle_p->SameFileHandle_p != NULL)
            {
                S64     StreamReadPtr,
                        StreamWritePtr,
                        ReadWriteDiff;

                I64_SetValue(0, 0, ReadWriteDiff);

                StreamWritePtr  = FileHandle_p->StreamAbsolutePos;

                SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                StreamReadPtr = FileHandle_p->SameFileHandle_p->StreamAbsolutePos;
                SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                DVMFS_Trace(("\tRead StreamReadPtr[%u,%u] StreamWritePtr[%u,%u]\n",
                        StreamReadPtr.MSW, StreamReadPtr.LSW,
                        StreamWritePtr.MSW, StreamWritePtr.LSW));

                if(I64_IsLessOrEqual(StreamReadPtr, StreamWritePtr))
                {
                    I64_Sub(StreamWritePtr, StreamReadPtr, ReadWriteDiff);
                }
                else
                {
                    DVMFS_Error(("Read(%08X) StreamReadPtr EXCEEDS [%u,%u] StreamWritePtr[%u,%u]\n", FileHandle_p,
                            StreamReadPtr.MSW, StreamReadPtr.LSW,
                            StreamWritePtr.MSW, StreamWritePtr.LSW));
                }

                /* check for STDVM_EVT_WRITE_CLOSE_TO_READ_ID here */
                if(FileHandle_p->RdCloseToWrNotified == FALSE)
                {
                    I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p), ReadWriteDiff);
                    if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW <= STDVMi_GetReadWriteThresholdInMs()))
                    {
                        FileHandle_p->RdCloseToWrNotified = TRUE;

                        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                        FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = TRUE;
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                        DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_CLOSE_TO_READ\n"));

                        STDVMi_NotifyEvent(STDVM_EVT_WRITE_CLOSE_TO_READ, FileHandle_p);
                    }
                }
                else if(FileHandle_p->RdCloseToWrNotified == TRUE)
                {
                    I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p), ReadWriteDiff);
                    if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW >= STDVMi_GetReadWriteThresholdOKInMs()))
                    {
                        FileHandle_p->RdCloseToWrNotified = FALSE;

                        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                        FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = FALSE;
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                        DVMFS_Trace(("\tRead NotifyEvent STDVM_EVT_WRITE_TO_READ_MARGIN_OK\n"));

                        STDVMi_NotifyEvent(STDVM_EVT_WRITE_TO_READ_MARGIN_OK, FileHandle_p);
                    }
                }
            }

            if(Bytes2Write > (Size-TotalBytesWritten))
                Bytes2Write = (Size-TotalBytesWritten);

            BytesWritten = vfs_write(FileHandle_p->StreamFileHandle, Buffer+TotalBytesWritten, Bytes2Write);
            if(BytesWritten <= 0)
            {
                DVMFS_Error(("Write(%08X, %d)=%d failed\n", FileHandle_p, Size-TotalBytesWritten, BytesWritten));
                break;
            }

            FileHandle_p->CurrFilePos += BytesWritten;

            TotalBytesWritten += BytesWritten;

            /* Update absolute file position */
            I64_AddLit(FileHandle_p->StreamAbsolutePos, TotalBytesWritten, FileHandle_p->StreamAbsolutePos);

            /* Update the Stream size also */
            FileHandle_p->StreamSize = FileHandle_p->StreamAbsolutePos;

            /* if a another file is opened for the same stream update stream size for that also */
            if(FileHandle_p->SameFileHandle_p != NULL)
            {
                SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                FileHandle_p->SameFileHandle_p->StreamSize = FileHandle_p->StreamSize;
                SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);
            }
        }
    }
    else /* circular */
    {
        S64     CircularStreamWritePtr,
                CircularStreamReadPtr,
                StreamSize,
                Temp;

        while(TotalBytesWritten < Size)
        {
            CircularStreamWritePtr = FileHandle_p->StreamAbsolutePos;
            StreamSize             = FileHandle_p->MaxStreamSize;

            DVMFS_Trace(("\tWrite StreamWritePtr[%u,%u] StreamSize[%u,%u]\n",
                    CircularStreamWritePtr.MSW, CircularStreamWritePtr.LSW,
                    StreamSize.MSW, StreamSize.LSW));

            /* CircularStreamWritePtr % StreamSize */
            while(I64_IsGreaterThan(CircularStreamWritePtr, StreamSize))
            {
                I64_Sub(CircularStreamWritePtr, StreamSize, CircularStreamWritePtr);
            }

            if(FileHandle_p->SameFileHandle_p != NULL)
            {
                SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                CircularStreamReadPtr  = FileHandle_p->SameFileHandle_p->StreamAbsolutePos;
                StreamSize             = FileHandle_p->SameFileHandle_p->MaxStreamSize;
                SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                DVMFS_Trace(("\tWrite StreamReadPtr[%u,%u] StreamSize[%u,%u]\n",
                        CircularStreamReadPtr.MSW, CircularStreamReadPtr.LSW,
                        StreamSize.MSW, StreamSize.LSW));

                /* CircularStreamReadPtr % StreamSize */
                while(I64_IsGreaterThan(CircularStreamReadPtr, StreamSize))
                {
                    I64_Sub(CircularStreamReadPtr, StreamSize, CircularStreamReadPtr);
                }

                /* Check if the Write overwrites Read */
                {
                    S64     ReadWriteDiff;

                    if(I64_IsEqual(CircularStreamWritePtr, CircularStreamReadPtr))
                    {
                        I64_SetValue(0, 0, ReadWriteDiff);
                        /* WR == RD -> RD has catched up with write so proceed to write data */
                    }
                    else if(I64_IsLessThan(CircularStreamWritePtr, CircularStreamReadPtr))
                    {
                        I64_Sub(CircularStreamReadPtr, CircularStreamWritePtr, ReadWriteDiff);

                        I64_SetValue((Size-TotalBytesWritten), 0, Temp);
                        if(I64_IsLessThan(ReadWriteDiff, Temp))
                        {
                            Size = TotalBytesWritten + ReadWriteDiff.LSW;   /* reduce the Size to write to space available */
                        }
                    }
                    else
                    {
                        I64_Sub(StreamSize, CircularStreamWritePtr, ReadWriteDiff);
                        I64_Add(ReadWriteDiff, CircularStreamReadPtr, ReadWriteDiff);

                        I64_SetValue((Size-TotalBytesWritten), 0, Temp);
                        if(I64_IsLessThan(ReadWriteDiff, Temp))
                        {
                            Size = TotalBytesWritten + ReadWriteDiff.LSW;   /* reduce the Size to write to space available */
                        }
                    }

                    /* check for STDVM_EVT_WRITE_CLOSE_TO_READ_ID here */
                    if(FileHandle_p->RdCloseToWrNotified == FALSE)
                    {
                        I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p), ReadWriteDiff);
                        if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW <= STDVMi_GetReadWriteThresholdInMs()))
                        {
                            FileHandle_p->RdCloseToWrNotified = TRUE;

                            SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                            FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = TRUE;
                            SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                            DVMFS_Trace(("\tWrite NotifyEvent STDVM_EVT_WRITE_CLOSE_TO_READ\n"));

                            STDVMi_NotifyEvent(STDVM_EVT_WRITE_CLOSE_TO_READ, FileHandle_p);
                        }
                    }
                    else if(FileHandle_p->RdCloseToWrNotified == TRUE)
                    {
                        I64_DivLit(ReadWriteDiff, STDVMi_GetStreamRateBpmS(FileHandle_p), ReadWriteDiff);
                        if((ReadWriteDiff.MSW == 0) && (ReadWriteDiff.LSW >= STDVMi_GetReadWriteThresholdOKInMs()))
                        {
                            FileHandle_p->RdCloseToWrNotified = FALSE;

                            SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                            FileHandle_p->SameFileHandle_p->RdCloseToWrNotified = FALSE;
                            SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

                            DVMFS_Trace(("\tWrite NotifyEvent STDVM_EVT_WRITE_TO_READ_MARGIN_OK\n"));

                            STDVMi_NotifyEvent(STDVM_EVT_WRITE_TO_READ_MARGIN_OK, FileHandle_p);
                        }
                    }
                }
            }

            I64_AddLit(CircularStreamWritePtr, (Size-TotalBytesWritten), Temp);
            if(I64_IsGreaterThan(Temp, FileHandle_p->MaxStreamSize))
            {
                if(I64_IsGreaterOrEqual(CircularStreamWritePtr, FileHandle_p->MaxStreamSize))
                {
                    vfs_close(FileHandle_p->StreamFileHandle);

                    GET_STREAM_FILE_NAME(LocalName, FileHandle_p, 0);

                    DVMFS_Trace(("\tWrite reached END of Stream WRAP AROUND, file %s\n", LocalName));

                    FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                    if(FileHandle_p->StreamFileHandle == NULL)
                    {
                        DVMFS_Error(("Write(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                        break;
                    }

                    FileHandle_p->CurrFileNo  = 0;
                    FileHandle_p->CurrFilePos = 0;
                }
                else
                {
                    I64_Sub(FileHandle_p->MaxStreamSize, CircularStreamWritePtr, Temp);
                    Size = TotalBytesWritten + Temp.LSW;
                }
            }

            if(FileHandle_p->CurrFilePos >= FS_MAX_SUB_FILE_SIZE)
            {
                vfs_close(FileHandle_p->StreamFileHandle);

                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->CurrFileNo+1);

                FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
                if(FileHandle_p->StreamFileHandle == NULL)
                {
                    DVMFS_Error(("Write(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                    break;
                }

                FileHandle_p->CurrFileNo++;
                FileHandle_p->CurrFilePos = 0;

                if(I64_IsLessThan(FileHandle_p->StreamSize, FileHandle_p->MaxStreamSize))
                {
                    FileHandle_p->StreamInfo_p->NbOfFiles++;

                    /* update INFO file */
                    STDVMi_UpdateStreamInfoToDisk(FileHandle_p);

                    /* if a another file is opened for the same stream update NbOfFiles for that also */
                    if(FileHandle_p->SameFileHandle_p != NULL)
                    {
                        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                        FileHandle_p->SameFileHandle_p->StreamInfo_p->NbOfFiles = FileHandle_p->StreamInfo_p->NbOfFiles;
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);
                    }
                }

                Bytes2Write = FS_MAX_SUB_FILE_SIZE;
            }
            else
            {
                Bytes2Write = FS_MAX_SUB_FILE_SIZE - FileHandle_p->CurrFilePos;
            }

            if(Bytes2Write > (Size-TotalBytesWritten))
                Bytes2Write = (Size-TotalBytesWritten);

            BytesWritten = vfs_write(FileHandle_p->StreamFileHandle, Buffer+TotalBytesWritten, Bytes2Write);
            if(BytesWritten <= 0)
            {
                DVMFS_Error(("Write(%08X, %d)=%d failed\n", FileHandle_p, Size-TotalBytesWritten, BytesWritten));
                break;
            }

            FileHandle_p->CurrFilePos += BytesWritten;

            TotalBytesWritten += BytesWritten;

            /* Update absolute file position */
            I64_AddLit(FileHandle_p->StreamAbsolutePos, BytesWritten, FileHandle_p->StreamAbsolutePos);

            if(I64_IsLessThan(FileHandle_p->StreamSize, FileHandle_p->MaxStreamSize))
            {
                /* Update Stream size also */
                FileHandle_p->StreamSize = FileHandle_p->StreamAbsolutePos;

                if(FileHandle_p->SameFileHandle_p != NULL)
                {
                    SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                    FileHandle_p->SameFileHandle_p->StreamSize = FileHandle_p->StreamSize;
                    SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);
                }
            }
            else
            {
                /* Update StartPosition since wrap around */
                I64_AddLit(FileHandle_p->StreamInfo_p->StreamStartPos, BytesWritten, FileHandle_p->StreamInfo_p->StreamStartPos);

                if(FileHandle_p->SameFileHandle_p != NULL)
                {
                    SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                    FileHandle_p->SameFileHandle_p->StreamInfo_p->StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                    SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);
                }

                DVMFS_Trace(("\tWrite(%08X) StartPos[%u,%u]\n", FileHandle_p,
                        FileHandle_p->StreamInfo_p->StreamStartPos.MSW, FileHandle_p->StreamInfo_p->StreamStartPos.LSW));

#ifndef STDVM_DONT_STORE_CIRCULAR_FILES
                /* update INFO file since change in start pos */
                STDVMi_UpdateStreamInfoToDisk(FileHandle_p);
#endif
                /* Removal of overwritten pids in case of circular file containing more than one program*/
                if(FileHandle_p->StreamInfo_p->NbOfPrograms > 1)
                {
                    vfs_file_t             *InfoFileHandle;
                    S64                     ProgEndPos;
                    S32                     ProgInfosSize;
                    STDVMi_ProgramInfo_t   *TempProgInfo_p;
                    U8                      ProgsOverWritten;

                    I64_MulLit(FileHandle_p->FirstProgramInfo.ProgEndPos, STPRM_PACKET_SIZE, ProgEndPos);
                    if(I64_IsGreaterOrEqual(FileHandle_p->StreamInfo_p->StreamStartPos, ProgEndPos))
                    {
                        GET_INFO_FILE_NAME(LocalName, FileHandle_p);

                        InfoFileHandle = vfs_open(LocalName, OSPLUS_O_RDWR);
                        if(InfoFileHandle == NULL)
                        {
                            DVMFS_Error(("Write(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            return FS_FAILURE;
                        }

                        if(vfs_fseek(InfoFileHandle, sizeof(STDVMi_StreamInfo_t), OSPLUS_SEEK_SET) != OSPLUS_SUCCESS)
                        {
                            DVMFS_Error(("Write(%08X) seek failed\n", InfoFileHandle));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        ProgInfosSize = (FileHandle_p->StreamInfo_p->NbOfPrograms - 1) * sizeof(STDVMi_ProgramInfo_t);
                        TempProgInfo_p = memory_allocate(STDVMi_NCachePartition, ProgInfosSize);
                        if(TempProgInfo_p == NULL)
                        {
                            DVMFS_Error(("Write(%08X) memory_allocate(%d) failed\n", FileHandle_p, ProgInfosSize));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        if(ProgInfosSize != vfs_read(InfoFileHandle, TempProgInfo_p, ProgInfosSize))
                        {
                            DVMFS_Error(("Write(%08X) info file Read(%d) failed\n", FileHandle_p, ProgInfosSize));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        for(ProgsOverWritten = 0; ProgsOverWritten < FileHandle_p->StreamInfo_p->NbOfPrograms; ProgsOverWritten++)
                        {
                            I64_MulLit(TempProgInfo_p[ProgsOverWritten].ProgEndPos, STPRM_PACKET_SIZE, ProgEndPos);
                            if(I64_IsLessThan(FileHandle_p->StreamInfo_p->StreamStartPos, ProgEndPos))
                            {
                                break;
                            }
                        }

                        if(vfs_fseek(InfoFileHandle, (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)), OSPLUS_SEEK_SET)
                                != OSPLUS_SUCCESS)
                        {
                            DVMFS_Error(("Write(%08X) seek failed\n", InfoFileHandle));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        ProgInfosSize -= ProgsOverWritten * sizeof(STDVMi_ProgramInfo_t);

                        BytesWritten = vfs_write(InfoFileHandle, TempProgInfo_p + ProgsOverWritten, ProgInfosSize);
                        if(BytesWritten != ProgInfosSize)
                        {
                            DVMFS_Error(("Write(%08X) info file write(%d)=%d failed\n", FileHandle_p,Size, BytesWritten));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        vfs_resize(InfoFileHandle, sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t) + ProgInfosSize);

                        if(vfs_fseek(InfoFileHandle, 0, OSPLUS_SEEK_SET) != OSPLUS_SUCCESS)
                        {
                            DVMFS_Error(("Write(%08X) seek failed\n", InfoFileHandle));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        FileHandle_p->StreamInfo_p->NbOfPrograms -= ProgsOverWritten + 1;

                        BytesWritten = vfs_write(InfoFileHandle, FileHandle_p->StreamInfo_p,
                                                 (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)));
                        if(BytesWritten != (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)))
                        {
                            DVMFS_Error(("Write(%08X) info file write(%d)=%d failed\n", FileHandle_p,
                                    (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)), BytesWritten));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        vfs_close(InfoFileHandle);

#ifdef USE_BACKUP_FILES
                        GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);

                        InfoFileHandle = vfs_open(LocalName, OSPLUS_O_RDWR);
                        if(InfoFileHandle == NULL)
                        {
                            DVMFS_Error(("Write(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            return FS_FAILURE;
                        }

                        if(vfs_fseek(InfoFileHandle, (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)), OSPLUS_SEEK_SET)
                                != OSPLUS_SUCCESS)
                        {
                            DVMFS_Error(("Write(%08X) seek failed\n", InfoFileHandle));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        BytesWritten = vfs_write(InfoFileHandle, TempProgInfo_p + ProgsOverWritten, ProgInfosSize);
                        if(BytesWritten != ProgInfosSize)
                        {
                            DVMFS_Error(("Write(%08X) info file write(%d)=%d failed\n", FileHandle_p,Size, BytesWritten));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        vfs_resize(InfoFileHandle, sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t) + ProgInfosSize);

                        if(vfs_fseek(InfoFileHandle, 0, OSPLUS_SEEK_SET) != OSPLUS_SUCCESS)
                        {
                            DVMFS_Error(("Write(%08X) seek failed\n", InfoFileHandle));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        BytesWritten = vfs_write(InfoFileHandle, FileHandle_p->StreamInfo_p,
                                                 (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)));
                        if(BytesWritten != (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)))
                        {
                            DVMFS_Error(("Write(%08X) info file write(%d)=%d failed\n", FileHandle_p,
                                    (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)), BytesWritten));
                            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                            memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);
                            vfs_close(InfoFileHandle);
                            return FS_FAILURE;
                        }

                        vfs_close(InfoFileHandle);
#endif /* USE_BACKUP_FILES */

                        FileHandle_p->FirstProgramInfo = TempProgInfo_p[ProgsOverWritten];

                        memory_deallocate(STDVMi_NCachePartition, TempProgInfo_p);

                        if(FileHandle_p->SameFileHandle_p != NULL)
                        {
                            SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
                            FileHandle_p->SameFileHandle_p->StreamInfo_p->NbOfPrograms = FileHandle_p->StreamInfo_p->NbOfPrograms;
                            FileHandle_p->SameFileHandle_p->CurrentProgNo -= ProgsOverWritten + 1;
                            SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);
                        }
                    }
                } /*Removal of overwritten pids*/
            }
        }
    }

    STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);

    /* Check for Minimum Disk space and notify */
    {
        vfs_info_t  DiskInfo;

        GET_ROOT_DIR(FileHandle_p, LocalName);

        if(vfs_fscntl(LocalName, VFS_FSCNTL_INFO_GET, (void *)&DiskInfo) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("Write(%08X) vfs_fscntl(%s) failed\n", FileHandle_p, LocalName));
        }
        else
        {
            if(FS_PARTITION_FREE_SIZE(DiskInfo) <= (STDVM_MIN_DISK_SPACE/HDD_SECTOR_SIZE))
            {
                DVMFS_Trace(("\tWrite NotifyEvent STDVM_EVT_DISK_FULL\n"));
                STDVMi_NotifyEvent(STDVM_EVT_DISK_FULL, FileHandle_p);
            }
            else if(FS_PARTITION_FREE_SIZE(DiskInfo) <= ((STDVMi_GetMinDiskSpace()+STDVM_MIN_DISK_SPACE)/HDD_SECTOR_SIZE))
            {
                DVMFS_Trace(("\tWrite NotifyEvent STDVM_EVT_DISK_SPACE_LOW\n"));
                STDVMi_NotifyEvent(STDVM_EVT_DISK_SPACE_LOW, FileHandle_p);
            }
        }
    }

    DVMFS_Trace(("\tSTDVMi_FileWrite(%08X)=%d\n\n", FileHandle_p, TotalBytesWritten));

    return TotalBytesWritten/STPRM_PACKET_SIZE;
}


/*******************************************************************************
Name         : STDVMi_FileSeek()

Description  : Large file seek glue for STPRM

Parameters   : STPRM_Handle_t   PRM_Handle          PRM handle
               U32              FileHandle          File handle returned in file open
               S64              PacketOffset        seek offset in packets
               STPRM_FSflags_t  Flags               seek flags

Return Value :
*******************************************************************************/
U32 STDVMi_FileSeek(STPRM_Handle_t PRM_Handle, U32 FileHandle, S64 PacketOffset, STPRM_FSflags_t Flags)
{
    STDVMi_Handle_t    *FileHandle_p = (STDVMi_Handle_t *)FileHandle;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    U32                 FileNb;
    S64                 SeekPosInBytes,
                        StreamAbsolutePos;
    U32                 LinearFileStartNb;


    DVMFS_Trace(("STDVMi_FileSeek(%08X): PackOff[%u,%u] Flags[%u] CurPos[%u,%u]\n", FileHandle_p,
            PacketOffset.MSW, PacketOffset.LSW,
            Flags,
            FileHandle_p->StreamAbsolutePos.MSW, FileHandle_p->StreamAbsolutePos.LSW));

    /* other flag options and negative offsets are not supported, not required by STPRM at present */
    if((Flags != STPRM_FS_SEEK_SET) || I64_IsNegative(PacketOffset))
    {
        DVMFS_Error(("Seek(%08X) Invalid Flag[%u] or Negative Offset[%u,%u]\n", FileHandle_p, Flags,
                PacketOffset.MSW, PacketOffset.LSW));
        return FS_FAILURE;
    }

    /* packet position should be a multiple of file system max block size 32k Bytes -> 8k Packets */
    if((PacketOffset.LSW & 0x1FFF) != 0)
    {
        DVMFS_Error(("Seek(%08X) PacketOffset[%u,%u] not a multiple of FS block size\n", FileHandle_p,
                PacketOffset.MSW, PacketOffset.LSW));
        return FS_FAILURE;
    }

    STREAM_ACCESS_MUTEX_LOCK(FileHandle_p);

    I64_MulLit(PacketOffset, STPRM_PACKET_SIZE, SeekPosInBytes);

    /* stored to assign the absolute position at the end */
    StreamAbsolutePos = SeekPosInBytes;

    /* set default start file to 0 */
    LinearFileStartNb = 0;

    /* Seek position is < start position */
    if((I64_IsLessThan(SeekPosInBytes, FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop)) ||
       (I64_IsLessThan(SeekPosInBytes, FileHandle_p->StreamInfo_p->StreamStartPos)))
    {
        DVMFS_Error(("Seek(%08X) Position[%u,%u] < StreamStartPos[%u,%u] || StreamStartPosAfterCrop[%u,%u]\n", FileHandle_p,
                SeekPosInBytes.MSW, SeekPosInBytes.LSW,
                FileHandle_p->StreamInfo_p->StreamStartPos.MSW, FileHandle_p->StreamInfo_p->StreamStartPos.LSW,
                FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop.MSW,
                FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop.LSW));
        STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
        return FS_FAILURE;
    }

    if(FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR)
    {
        /* nothing to do for linear file, proceed to seek below */
    }
    else if(FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
    {
        /* File has wrapped */
        if(!I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPos))
        {
            S64     Temp;

            I64_Add(FileHandle_p->StreamInfo_p->StreamStartPos, FileHandle_p->StreamSize, Temp);
            if(I64_IsGreaterThan(SeekPosInBytes, Temp))
            {
                DVMFS_Error(("Seek(%08X) Position[%u,%u] > StreamEndPos[%u,%u]\n", FileHandle_p,
                        SeekPosInBytes.MSW, SeekPosInBytes.LSW, Temp.MSW, Temp.LSW));
                STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
                return FS_FAILURE;
            }

            /* SeekPosInBytes could be more than the stream size since STDVM always assume the file to be growing */
            /* SeekPosInBytes % StreamSize will give the offset to seek */
            while(I64_IsGreaterThan(SeekPosInBytes, FileHandle_p->StreamSize))
            {
                I64_Sub(SeekPosInBytes, FileHandle_p->StreamSize, SeekPosInBytes);
            }
        }
    }
    else /* circular_linear */
    {
        S64     CircularPartEndPos,
                Temp;

        I64_Add(FileHandle_p->StreamInfo_p->StreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize, CircularPartEndPos);

        if(I64_IsLessThan(SeekPosInBytes, CircularPartEndPos))
        {
            /* SeekPosInBytes could be more than the stream size since STDVM always assume the file to be growing */
            /* SeekPosInBytes % StreamSize will give the offset to seek */
            while(I64_IsGreaterThan(SeekPosInBytes, FileHandle_p->StreamInfo_p->CircularPartSize))
            {
                I64_Sub(SeekPosInBytes, FileHandle_p->StreamInfo_p->CircularPartSize, SeekPosInBytes);
            }
        }
        else
        {
            I64_Sub(SeekPosInBytes, CircularPartEndPos, SeekPosInBytes);

            I64_AddLit(FileHandle_p->StreamInfo_p->CircularPartSize, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
            I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);

            LinearFileStartNb = Temp.LSW;
        }
    }

    if(I64_IsGreaterThan(SeekPosInBytes, FileHandle_p->StreamSize))
    {
        DVMFS_Error(("Seek(%08X) SeekPosInBytes[%u,%u] > FileHandle_p->StreamSize[%u,%u]\n", FileHandle_p,
                SeekPosInBytes.MSW, SeekPosInBytes.LSW,
                FileHandle_p->StreamSize.MSW, FileHandle_p->StreamSize.LSW));
        STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
        return FS_FAILURE;
    }

    for(FileNb = LinearFileStartNb; FileNb < FileHandle_p->StreamInfo_p->NbOfFiles; FileNb++)
    {
        if(SeekPosInBytes.LSW < FS_MAX_SUB_FILE_SIZE)
        {
            if(SeekPosInBytes.MSW == 0)
                break;

            SeekPosInBytes.MSW--;
            SeekPosInBytes.LSW += ((U32)0xFFFFFFFF - FS_MAX_SUB_FILE_SIZE) + 1;
        }
        else
        {
            SeekPosInBytes.LSW -= FS_MAX_SUB_FILE_SIZE;
        }
    }

    if(FileNb >= FileHandle_p->StreamInfo_p->NbOfFiles)
    {
        DVMFS_Error(("Seek(%08X) FileNb[%d] > NbOfFiles[%d] %u\n", FileHandle_p,
                FileNb, FileHandle_p->StreamInfo_p->NbOfFiles));
        STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
        return FS_FAILURE;
    }

    if(FileHandle_p->CurrFileNo != FileNb)
    {
        if(FileHandle_p->CurrFileNo < FileHandle_p->StreamInfo_p->NbOfFiles)    /* the file is not opened in FileOpen */
            vfs_close(FileHandle_p->StreamFileHandle);

        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);

        FileHandle_p->StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
        if(FileHandle_p->StreamFileHandle == NULL)
        {
            DVMFS_Error(("Seek(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
            return FS_FAILURE;
        }

        FileHandle_p->CurrFileNo = FileNb;
    }

    if(vfs_fseek(FileHandle_p->StreamFileHandle, SeekPosInBytes.LSW, OSPLUS_SEEK_SET) != OSPLUS_SUCCESS)
    {
        DVMFS_Error(("Seek(%08X) file seek(%d) failed\n", FileHandle_p, SeekPosInBytes.LSW));
        STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
        return FS_FAILURE;
    }

    FileHandle_p->CurrFilePos       = SeekPosInBytes.LSW;

    FileHandle_p->StreamAbsolutePos = StreamAbsolutePos;

    STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);

    DVMFS_Trace(("\tSTDVMi_FileSeek(%08X)=Pos[%u,%u]\n\n", FileHandle_p, StreamAbsolutePos.MSW, StreamAbsolutePos.LSW));

    return FS_SUCCESS;
}


/*******************************************************************************
Name         : STDVMi_FileClose()

Description  : File close glue for STPRM

Parameters   : STPRM_Handle_t   PRM_Handle          PRM handle
               U32              FileHandle          File handle returned in file open

Return Value :
*******************************************************************************/
U32 STDVMi_FileClose(STPRM_Handle_t PRM_Handle, U32 FileHandle)
{
    STDVMi_Handle_t    *FileHandle_p = (STDVMi_Handle_t *)FileHandle;


    DVMFS_Trace(("STDVMi_FileClose(%08X)\n", FileHandle_p));

    STREAM_ACCESS_MUTEX_LOCK(FileHandle_p);

    /* reset the File handle and the Same file handle's reference */
    if(FileHandle_p->SameFileHandle_p != NULL)
    {
        SHARED_FIELD_ACCESS_MUTEX_LOCK(FileHandle_p);
        FileHandle_p->SameFileHandle_p->SameFileHandle_p = NULL;
        FileHandle_p->SameFileHandle_p = NULL;
        SHARED_FIELD_ACCESS_MUTEX_RELEASE(FileHandle_p);

        SHARED_FIELD_ACCESS_MUTEX_DELETE(FileHandle_p);
    }

    vfs_close(FileHandle_p->StreamFileHandle);

    /* update INFO file if only opened in write mode */
    if(FileHandle_p->OpenFlags & OSPLUS_O_WRONLY)
    {
        STDVMi_UpdateStreamInfoToDisk(FileHandle_p);
    }

    FileHandle_p->StreamInfo_p->Signature = 0;

    STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);

    STREAM_ACCESS_MUTEX_DELETE(FileHandle_p);

    DVMFS_Trace(("\tSTDVMi_FileClose(%08X)\n\n", FileHandle_p));

    return FS_SUCCESS;
}


/*******************************************************************************
Name         : STDVMi_UpdateStreamInfoToDisk()

Description  : Writes current stream info to HDD

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_UpdateStreamInfoToDisk(STDVMi_Handle_t *Handle_p)
{
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_file_t         *InfoFileHandle;
    S32                 BytesWritten;


    /* this function need not be protected with STREAM_ACCESS_MUTEX since this is accessed internally */

    DVMFS_Trace(("STDVMi_UpdateStreamInfoToDisk(%08X)\n", Handle_p));

    GET_INFO_FILE_NAME(LocalName, Handle_p);

    InfoFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("UpdateStreamInfo(%08X) file open(%s) failed\n", Handle_p, LocalName));
        return ST_ERROR_BAD_PARAMETER;
    }

    BytesWritten = vfs_write(InfoFileHandle, Handle_p->StreamInfo_p, (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)));

    vfs_close(InfoFileHandle);

    if(BytesWritten != (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)))
    {
        DVMFS_Error(("UpdateStreamInfo(%08X) info file write(%d)=%d failed\n", Handle_p,
                (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)), BytesWritten));
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;
    }

#ifdef USE_BACKUP_FILES
    GET_INFO_BACKUP_FILE_NAME(LocalName, Handle_p);

    InfoFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("UpdateStreamInfo(%08X) file open(%s) failed\n", Handle_p, LocalName));
        return ST_ERROR_BAD_PARAMETER;
    }

    BytesWritten = vfs_write(InfoFileHandle, Handle_p->StreamInfo_p, (sizeof(STDVMi_StreamInfo_t)-sizeof(STDVMi_ProgramInfo_t)));

    vfs_close(InfoFileHandle);

    if(BytesWritten != (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)))
    {
        DVMFS_Error(("UpdateStreamInfo(%08X) info backup file write(%d)=%d failed\n", Handle_p,
                (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)), BytesWritten));
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;
    }
#endif /* USE_BACKUP_FILES */

    DVMFS_Trace(("\tSTDVMi_UpdateStreamInfoToDisk(%08X)\n\n", Handle_p));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_UpdateStreamChangePIDs()

Description  : Writes current stream info to HDD

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_UpdateStreamChangePIDs(STDVMi_Handle_t *Handle_p, U32 NumberOfPids, STDVM_StreamData_t *Pids_p)
{
    S64                     StreamEndPos;
    char                    LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_file_t             *InfoFileHandle;
    S32                     BytesWritten;
    STDVMi_ProgramInfo_t    ProgramInfoTemp[2];

    STREAM_ACCESS_MUTEX_LOCK(Handle_p);

    I64_DivLit(Handle_p->StreamAbsolutePos, STPRM_PACKET_SIZE, StreamEndPos);

    /* update current program info */
    ProgramInfoTemp[0] = Handle_p->StreamInfo_p->ProgramInfo[0];

    ProgramInfoTemp[0].ProgEndPos = StreamEndPos;

    /* update next program info */
    memset(&ProgramInfoTemp[1], 0, sizeof(STDVMi_ProgramInfo_t));

    ProgramInfoTemp[1].NbOfPids = NumberOfPids;
    memcpy(ProgramInfoTemp[1].Pids, Pids_p, sizeof(STDVM_StreamData_t) * NumberOfPids);

    /* start position for the new program is the end position for the old program */
    ProgramInfoTemp[1].ProgStartPos = StreamEndPos;
    I64_SetValue(0xFFFFFFFF, 0xFFFFFFFF, ProgramInfoTemp[1].ProgEndPos);

    /* update current program info with new program info */
    Handle_p->StreamInfo_p->ProgramInfo[0] = ProgramInfoTemp[1];

    GET_INFO_FILE_NAME(LocalName, Handle_p);

    InfoFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("UpdateStreamChangePIDs(%08X) file open(%s) failed\n", Handle_p, LocalName));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    if(vfs_fseek(InfoFileHandle, -(sizeof(STDVMi_ProgramInfo_t)), OSPLUS_SEEK_END) != OSPLUS_SUCCESS)
    {
        DVMFS_Error(("UpdateStreamChangePIDs(%08X) seek failed\n", InfoFileHandle));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;  /* TODO: to change error code */
    }

    BytesWritten = vfs_write(InfoFileHandle, ProgramInfoTemp, sizeof(ProgramInfoTemp));
    if(BytesWritten != sizeof(ProgramInfoTemp))
    {
        DVMFS_Error(("UpdateStreamInfo(%08X) info file write(%d)=%d failed\n", Handle_p,
                sizeof(ProgramInfoTemp), BytesWritten));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;
    }

    /* update INFO file since change in NbOfPrograms */
    if(vfs_fseek(InfoFileHandle, 0, OSPLUS_SEEK_SET) != OSPLUS_SUCCESS)
    {
        DVMFS_Error(("UpdateStreamChangePIDs(%08X) seek failed\n", InfoFileHandle));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;  /* TODO: to change error code */
    }

    Handle_p->StreamInfo_p->NbOfPrograms++;

    BytesWritten = vfs_write(InfoFileHandle, Handle_p->StreamInfo_p, (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)));

    vfs_close(InfoFileHandle);

    if(BytesWritten != (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)))
    {
        DVMFS_Error(("UpdateStreamInfo(%08X) info file write(%d)=%d failed\n", Handle_p,
                sizeof(STDVMi_ProgramInfo_t), BytesWritten));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;
    }

#ifdef USE_BACKUP_FILES
    GET_INFO_BACKUP_FILE_NAME(LocalName, Handle_p);

    InfoFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("UpdateStreamChangePIDs(%08X) file open(%s) failed\n", Handle_p, LocalName));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    if(vfs_fseek(InfoFileHandle, -(sizeof(STDVMi_ProgramInfo_t)), OSPLUS_SEEK_END) != OSPLUS_SUCCESS)
    {
        DVMFS_Error(("UpdateStreamChangePIDs(%08X) seek failed\n", InfoFileHandle));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;  /* TODO: to change error code */
    }

    BytesWritten = vfs_write(InfoFileHandle, ProgramInfoTemp, sizeof(ProgramInfoTemp));
    if(BytesWritten != sizeof(ProgramInfoTemp))
    {
        DVMFS_Error(("UpdateStreamChangePIDs(%08X) info file write(%d)=%d failed\n", Handle_p,
                sizeof(ProgramInfoTemp), BytesWritten));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;
    }

    /* update INFO file since change in NbOfPrograms */
    if(vfs_fseek(InfoFileHandle, 0, OSPLUS_SEEK_SET) != OSPLUS_SUCCESS)
    {
        DVMFS_Error(("UpdateStreamChangePIDs(%08X) seek failed\n", InfoFileHandle));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;  /* TODO: to change error code */
    }

    BytesWritten = vfs_write(InfoFileHandle, Handle_p->StreamInfo_p, (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)));

    vfs_close(InfoFileHandle);

    if(BytesWritten != (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)))
    {
        DVMFS_Error(("UpdateStreamInfo(%08X) info file write(%d)=%d failed\n", Handle_p,
                sizeof(STDVMi_ProgramInfo_t), BytesWritten));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;
    }
#endif  /* USE_BACKUP_FILES */

    /* update first program info */
    if((Handle_p->FirstProgramInfo.ProgEndPos.LSW == 0xFFFFFFFF) &&
       (Handle_p->FirstProgramInfo.ProgEndPos.MSW == 0xFFFFFFFF))
    {
        Handle_p->FirstProgramInfo.ProgEndPos  = StreamEndPos;
    }

    /* Update end position in play programinfo in case of timeshift */
    if(Handle_p->SameFileHandle_p != NULL)
    {
        SHARED_FIELD_ACCESS_MUTEX_LOCK(Handle_p);
        Handle_p->SameFileHandle_p->StreamInfo_p->NbOfPrograms++;
        if((Handle_p->SameFileHandle_p->StreamInfo_p->ProgramInfo->ProgEndPos.LSW == 0xFFFFFFFF) &&
           (Handle_p->SameFileHandle_p->StreamInfo_p->ProgramInfo->ProgEndPos.MSW == 0xFFFFFFFF))
        {
            Handle_p->SameFileHandle_p->StreamInfo_p->ProgramInfo->ProgEndPos = StreamEndPos;
        }
        SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    STREAM_ACCESS_MUTEX_RELEASE(Handle_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_ReadProgramInfo()

Description  : Read current stream info from HDD

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_ReadProgramInfo(STDVMi_Handle_t *Handle_p)
{
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_file_t         *InfoFileHandle;
    S32                 BytesRead;


    STREAM_ACCESS_MUTEX_LOCK(Handle_p);

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  InfStatus,
                    BakStatus;

        GET_INFO_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &InfStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("ReadProgramInfo(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            InfStatus.st_size = 0;
        }

        GET_INFO_BACKUP_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("ReadProgramInfo(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            BakStatus.st_size = 0;
        }

        if((InfStatus.st_size == 0) && (BakStatus.st_size == 0))
        {
            DVMFS_Error(("ReadProgramInfo(%08X) both Index and Backup file does not exist\n", Handle_p));
            STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(InfStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INFO_BACKUP_FILE_NAME(LocalName, Handle_p);
        }
        else
        {
            GET_INFO_FILE_NAME(LocalName, Handle_p);
        }
    }
#else
    GET_INFO_FILE_NAME(LocalName, Handle_p);
#endif /* USE_BACKUP_FILES */

    InfoFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("ReadProgramInfo(%08X) file open(%s) failed\n", Handle_p, LocalName));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    if(vfs_fseek(InfoFileHandle,
            (sizeof(STDVMi_StreamInfo_t) - sizeof(STDVMi_ProgramInfo_t)) + Handle_p->CurrentProgNo * sizeof(STDVMi_ProgramInfo_t),
            OSPLUS_SEEK_SET) != OSPLUS_SUCCESS)
    {
        DVMFS_Error(("ReadProgramInfo(%08X) seek failed\n", Handle_p));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_INFO_FILE_READ_FAILED;   /* TODO: to change error code */
    }

    BytesRead = vfs_read(InfoFileHandle, Handle_p->StreamInfo_p->ProgramInfo, sizeof(STDVMi_ProgramInfo_t));

    vfs_close(InfoFileHandle);

    if(BytesRead != sizeof(STDVMi_ProgramInfo_t))
    {
        DVMFS_Error(("ReadStreamInfo(%08X) info file Read(%d)=%d failed\n", Handle_p, sizeof(STDVMi_ProgramInfo_t), BytesRead));
        STREAM_ACCESS_MUTEX_RELEASE(Handle_p);
        return STDVM_ERROR_INFO_FILE_READ_FAILED;
    }

    STREAM_ACCESS_MUTEX_RELEASE(Handle_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_SetStreamInfo()

Description  : Writes stream info to HDD

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle
               char                *StreamName          Stream name
               STDVM_StreamInfo_t  *StreamInfo_p        Stream info to write into HDD

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_SetStreamInfo(STDVMi_Handle_t *Handle_p, char *StreamName, STDVM_StreamInfo_t *StreamInfo_p)
{
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_file_t         *InfoFileHandle;
    S32                 BytesWritten;


    if(Handle_p->StreamInfo_p->Signature == STDVMi_SIGNATURE)
    {
        DVMFS_Error(("SetStreamInfo(%08X) Record already active\n", Handle_p));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* check if the file is in use already */
    /* Seperate File name and dir name */
    GET_PATH_AND_FILE_NAME(StreamName, Handle_p);

    Handle_p->OpenFlags = OSPLUS_O_WRONLY | OSPLUS_O_CREAT;

    if(STDVMi_ValidateOpen(Handle_p) != ST_NO_ERROR)
    {
        DVMFS_Error(("SetStreamInfo(%08X) stream(%s) already in use\n", Handle_p, StreamName));
        return ST_ERROR_BAD_PARAMETER;
    }

    GET_INFO_FILE_NAME(LocalName, Handle_p);

    /* check if the file already exist */
    InfoFileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
    if(InfoFileHandle != NULL)
    {
        DVMFS_Error(("SetStreamInfo(%08X) stream(%s) already exist\n", Handle_p, LocalName));
        vfs_close(InfoFileHandle);
        return STDVM_ERROR_STREAM_ALREADY_EXISTS;
    }

    InfoFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("SetStreamInfo(%08X) file open(%s) failed\n", Handle_p, LocalName));
        return STDVM_ERROR_INFO_FILE_OPEN_FAILED;
    }

    memset(Handle_p->StreamInfo_p, 0, sizeof(STDVMi_StreamInfo_t));

    strcpy(Handle_p->StreamInfo_p->ChannelName, StreamInfo_p->ChannelName);
    strcpy(Handle_p->StreamInfo_p->Description, StreamInfo_p->Description);

    Handle_p->StreamInfo_p->ProgramInfo->NbOfPids = StreamInfo_p->PidsNum;
    memcpy( Handle_p->StreamInfo_p->ProgramInfo->Pids, StreamInfo_p->Pids, StreamInfo_p->PidsNum*sizeof(STDVM_StreamData_t));

    I64_SetValue(0xFFFFFFFF, 0xFFFFFFFF, Handle_p->StreamInfo_p->ProgramInfo->ProgEndPos);

    Handle_p->StreamInfo_p->RecordDateTime = StreamInfo_p->RecordDateTime;

    Handle_p->FirstProgramInfo = Handle_p->StreamInfo_p->ProgramInfo[DEFAULT_PROGRAM];

    BytesWritten = vfs_write(InfoFileHandle, Handle_p->StreamInfo_p, sizeof(STDVMi_StreamInfo_t));

    vfs_close(InfoFileHandle);

    if(BytesWritten != sizeof(STDVMi_StreamInfo_t))
    {
        DVMFS_Error(("SetStreamInfo(%08X) file write(%d)=%d failed\n", Handle_p, sizeof(STDVMi_StreamInfo_t), BytesWritten));
        return STDVM_ERROR_INFO_FILE_WRITE_FAILED;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_GetStreamInfo()

Description  : Reads stream info from HDD

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle
               char                *StreamName          Stream name
               STDVM_StreamInfo_t  *StreamInfo_p        Stream info to write into HDD

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_GetStreamInfo(STDVMi_Handle_t *Handle_p, char *StreamName, STDVM_StreamInfo_t *StreamInfo_p)
{
    ST_ErrorCode_t      ErrorCode;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    STDVMi_Handle_t    *FileHandle_p;
    vfs_file_t         *InfoFileHandle;
    S32                 BytesRead;

    FileHandle_p = STDVMi_GetFreeHandle();
    if(FileHandle_p == NULL)
    {
        DVMFS_Error(("GetStreamInfo(%08X) No free handles\n", Handle_p));
        return ST_ERROR_NO_FREE_HANDLES;
    }
    FileHandle_p->ObjectType = STDVM_OBJECT_PLAYBACK;

    /* Seperate File name and dir name */
    GET_PATH_AND_FILE_NAME(StreamName, FileHandle_p);

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  InfStatus,
                    BakStatus;

        GET_INFO_FILE_NAME(LocalName, FileHandle_p);
        if(vfs_stat(LocalName, &InfStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("GetStreamInfo(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
            InfStatus.st_size = 0;
        }

        GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("GetStreamInfo(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
            BakStatus.st_size = 0;
        }

        if((InfStatus.st_size == 0) && (BakStatus.st_size == 0))
        {
            DVMFS_Error(("GetStreamInfo(%08X) both Index and Backup file does not exist\n", Handle_p));
            STDVMi_ReleaseHandle(FileHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(InfStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
        }
        else
        {
            GET_INFO_FILE_NAME(LocalName, FileHandle_p);
        }
    }
#else
    GET_INFO_FILE_NAME(LocalName, FileHandle_p);
#endif /* USE_BACKUP_FILES */

    InfoFileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
    if(InfoFileHandle == NULL)
    {
        DVMFS_Error(("GetStreamInfo(%08X) file open(%s) failed\n", Handle_p, LocalName));
        STDVMi_ReleaseHandle(FileHandle_p);
        return STDVM_ERROR_INFO_FILE_OPEN_FAILED;
    }

    BytesRead = vfs_read(InfoFileHandle, FileHandle_p->StreamInfo_p, sizeof(STDVMi_StreamInfo_t));

    vfs_close(InfoFileHandle);

    if(BytesRead < sizeof(STDVMi_StreamInfo_t))
    {
        DVMFS_Error(("GetStreamInfo(%08X) file read(%d)=%d failed\n", Handle_p, sizeof(STDVMi_StreamInfo_t), BytesRead));
        STDVMi_ReleaseHandle(FileHandle_p);
        return STDVM_ERROR_INFO_FILE_READ_FAILED;
    }

    strcpy(StreamInfo_p->ChannelName, FileHandle_p->StreamInfo_p->ChannelName);

    strcpy(StreamInfo_p->Description, FileHandle_p->StreamInfo_p->Description);

    /* if the File is being played return the PID details of the current program being played */
    /* WARNING: Accessing Handle_p without MUTEX_LOCK might cause an issue, but values are only read should not be a problem */
    StreamInfo_p->PidsNum = FileHandle_p->StreamInfo_p->ProgramInfo->NbOfPids;
    memcpy( StreamInfo_p->Pids,
            FileHandle_p->StreamInfo_p->ProgramInfo->Pids,
            StreamInfo_p->PidsNum*sizeof(STPRM_StreamData_t));

    FileHandle_p->SameFileHandle_p = STDVMi_GetAlreadyOpenHandle(FileHandle_p);

    ErrorCode = STDVMi_GetTimeAndSize(  FileHandle_p,
                                        &StreamInfo_p->StartTimeInMs,
                                        &StreamInfo_p->DurationInMs,
                                        &StreamInfo_p->NbOfPackets);

    FileHandle_p->SameFileHandle_p = NULL;

    StreamInfo_p->RecordDateTime = FileHandle_p->StreamInfo_p->RecordDateTime;

    STDVMi_ReleaseHandle(FileHandle_p);

    return ErrorCode;
}


/*******************************************************************************
Name         : STDVMi_RemoveStreamInfo()

Description  : Removes the info file created, called when PRM record start fails after creating info file

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle
               char                *StreamName          Stream name

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_RemoveStreamInfo(STDVMi_Handle_t *Handle_p, char *StreamName)
{
    char                LocalName[STDVMi_MAX_NAME_LENGTH];

    /* Seperate File name and dir name */
    GET_PATH_AND_FILE_NAME(StreamName, Handle_p);

    GET_INFO_FILE_NAME(LocalName, Handle_p);
    vfs_unlink(LocalName);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_RemoveStream()

Description  : Removes all the files created for a recording

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle
               char                *StreamName          Stream name

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_RemoveStream(STDVMi_Handle_t *Handle_p, char *StreamName)
{
    ST_ErrorCode_t      ErrorCode;
    S32                 FileNb;
    STDVMi_Handle_t    *FileHandle_p;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_file_t         *FileHandle;
    S32                 BytesRead;


    FileHandle_p = STDVMi_GetFreeHandle();
    if(FileHandle_p == NULL)
    {
        DVMFS_Error(("RemoveStream(%08X) No free handles\n", Handle_p));
        return ST_ERROR_NO_FREE_HANDLES;
    }

    /* Seperate File name and dir name */
    GET_PATH_AND_FILE_NAME(StreamName, FileHandle_p);

    /* update the OpenFlags needed for Validate */
    FileHandle_p->OpenFlags = STDVMi_OPEN_FLAGS_DELETE;

    ErrorCode = STDVMi_ValidateOpen(FileHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        DVMFS_Error(("RemoveStream(%08X) File already open\n", Handle_p));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ErrorCode;
    }

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  InfStatus,
                    BakStatus;

        GET_INFO_FILE_NAME(LocalName, FileHandle_p);
        if(vfs_stat(LocalName, &InfStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("RemoveStream(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
            InfStatus.st_size = 0;
        }

        GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("RemoveStream(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
            BakStatus.st_size = 0;
        }

        if((InfStatus.st_size == 0) && (BakStatus.st_size == 0))
        {
            DVMFS_Error(("RemoveStream(%08X) both Index and Backup file does not exist\n", Handle_p));
            STDVMi_ReleaseHandle(FileHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(InfStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
        }
        else
        {
            GET_INFO_FILE_NAME(LocalName, FileHandle_p);
        }
    }
#else
    GET_INFO_FILE_NAME(LocalName, FileHandle_p);
#endif /* USE_BACKUP_FILES */

    FileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
    if(FileHandle == NULL)
    {
        DVMFS_Error(("RemoveStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
        STDVMi_ReleaseHandle(FileHandle_p);
        return STDVM_ERROR_INFO_FILE_OPEN_FAILED;
    }

    BytesRead = vfs_read(FileHandle, FileHandle_p->StreamInfo_p, sizeof(STDVMi_StreamInfo_t));

    vfs_close(FileHandle);

    if(BytesRead < sizeof(STDVMi_StreamInfo_t))
    {
        DVMFS_Error(("RemoveStream(%08X) file read(%d)=%d failed\n", Handle_p, sizeof(STDVMi_StreamInfo_t), BytesRead));
        STDVMi_ReleaseHandle(FileHandle_p);
        return STDVM_ERROR_INFO_FILE_READ_FAILED;
    }

    if(FileHandle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        DVMFS_Error(("RemoveStream(%08X) not a valid signature %08X\n", Handle_p, FileHandle_p->StreamInfo_p->Signature));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    for(FileNb = FileHandle_p->StreamInfo_p->NbOfFiles - 1; FileNb >= 0; FileNb--)
    {
        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
        vfs_unlink(LocalName);
    }

    GET_INDEX_FILE_NAME(LocalName, FileHandle_p);
    vfs_unlink(LocalName);

#ifdef USE_BACKUP_FILES
    GET_INDEX_BACKUP_FILE_NAME(LocalName, FileHandle_p);
    vfs_unlink(LocalName);

    GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
    vfs_unlink(LocalName);
#endif /* USE_BACKUP_FILES */

    GET_STREAM_DIR_NAME(LocalName, FileHandle_p);
    vfs_rmdir(LocalName);

    GET_INFO_FILE_NAME(LocalName, FileHandle_p);
    vfs_unlink(LocalName);

    STDVMi_ReleaseHandle(FileHandle_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_CopyStream()

Description  : Copy part of Old stream to a New stram

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle
               char                *OldStreamName       Old Stream name
               U32                  StartTimeInMs       Start Time
               U32                  EndTimeInMs         End Time
               char                *NewStreamName       New Stream name

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_CopyStream(STDVMi_Handle_t   *Handle_p,
                                 char              *OldStreamName,
                                 U32                StartTimeInMs,
                                 U32                EndTimeInMs,
                                 char              *NewStreamName)
{
    ST_ErrorCode_t      ErrorCode;
    STDVMi_Handle_t    *OldFileHandle_p,
                       *NewFileHandle_p;
    STDVM_StreamInfo_t  StreamInfo;
    S32                 PacksRead,
                        Packs2Read,
                        PacksWritten;
    U64                 StreamStartPos,
                        StreamEndPos;
    U8                 *Buffer_p,
                       *AlignedBuffer_p;
    S64                 SizeInPackets;


    /* check if any stream exists with the new name */
    ErrorCode = STDVMi_GetStreamInfo(Handle_p, NewStreamName, &StreamInfo);
    if(ErrorCode == ST_NO_ERROR)
    {
        DVMFS_Error(("CopyStream(%08X) Stream(%s) already exists\n", Handle_p, NewStreamName));
        return ErrorCode;
    }

    /* get old stream info */
    ErrorCode = STDVMi_GetStreamInfo(Handle_p, OldStreamName, &StreamInfo);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMFS_Error(("CopyStream(%08X) STDVMi_GetStreamInfo(%s)=%08X failed\n", Handle_p, OldStreamName, ErrorCode));
        return ErrorCode;
    }

    /* when starttime is 0 for circular files take the actual start */
    if((StartTimeInMs == 0) && (StreamInfo.StartTimeInMs != 0))
        StartTimeInMs = StreamInfo.StartTimeInMs;

    /* copy full stream if End time is Zero */
    if(EndTimeInMs == 0)
        EndTimeInMs = StreamInfo.StartTimeInMs + StreamInfo.DurationInMs;

    if((StartTimeInMs < StreamInfo.StartTimeInMs) ||
       (EndTimeInMs > (StreamInfo.StartTimeInMs + StreamInfo.DurationInMs)) ||
       (StartTimeInMs >= EndTimeInMs))
    {
        DVMFS_Error(("CopyStream(%08X) invalid time limits Old{S[%u],E[%u]} New{S[%u],E[%u]}\n", Handle_p,
            StreamInfo.StartTimeInMs, StreamInfo.StartTimeInMs + StreamInfo.DurationInMs, StartTimeInMs, EndTimeInMs));
        return ST_ERROR_BAD_PARAMETER;
    }

    OldFileHandle_p = STDVMi_GetFreeHandle();
    if(OldFileHandle_p == NULL)
    {
        DVMFS_Error(("CopyStream(%08X) No free handles\n", Handle_p));
        return ST_ERROR_NO_FREE_HANDLES;
    }
    OldFileHandle_p->ObjectType = STDVM_OBJECT_PLAYBACK;

    NewFileHandle_p = STDVMi_GetFreeHandle();
    if(NewFileHandle_p == NULL)
    {
        DVMFS_Error(("CopyStream(%08X) No free handles\n", Handle_p));
        STDVMi_ReleaseHandle(OldFileHandle_p);
        return ST_ERROR_NO_FREE_HANDLES;
    }
    NewFileHandle_p->ObjectType = STDVM_OBJECT_RECORD;

     /* set new stream info */
    ErrorCode = STDVMi_SetStreamInfo(NewFileHandle_p, NewStreamName, &StreamInfo);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMFS_Error(("CopyStream(%08X) STDVMi_SetStreamInfo(%s)=%08X failed\n", Handle_p, NewStreamName, ErrorCode));
        STDVMi_ReleaseHandle(NewFileHandle_p);
        STDVMi_ReleaseHandle(OldFileHandle_p);
        return ErrorCode;
    }

    I64_SetValue(0, 0, SizeInPackets);

    OldFileHandle_p->PRM_Handle = (STPRM_Handle_t)OldFileHandle_p;
    if(STDVMi_FileOpen((STPRM_Handle_t)OldFileHandle_p, (U8 *)OldStreamName, STPRM_FS_RD, SizeInPackets) == FS_FAILURE)
    {
        DVMFS_Error(("CopyStream(%08X) STDVMi_FileOpen(%s) failed\n", Handle_p, OldStreamName));
        STDVMi_RemoveStreamInfo(NewFileHandle_p, NewStreamName);

        STDVMi_ReleaseHandle(NewFileHandle_p);
        STDVMi_ReleaseHandle(OldFileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    NewFileHandle_p->PRM_Handle = (STPRM_Handle_t)NewFileHandle_p;
    if(STDVMi_FileOpen((STPRM_Handle_t)NewFileHandle_p, (U8 *)NewStreamName, STPRM_FS_WR, SizeInPackets) == FS_FAILURE)
    {
        DVMFS_Error(("CopyStream(%08X) STDVMi_FileOpen(%s) failed\n", Handle_p, NewStreamName));
        STDVMi_RemoveStreamInfo(NewFileHandle_p, NewStreamName);

        STDVMi_ReleaseHandle(NewFileHandle_p);
        STDVMi_ReleaseHandle(OldFileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Allocate temp buffer for copying file */
    Buffer_p = memory_allocate(STDVMi_NCachePartition, INTERMEDIATE_BUFFER_SIZE + 31);
    if(Buffer_p == NULL)
    {
        DVMFS_Error(("CopyStream(%08X) memory_allocate(%d) failed\n", Handle_p, INTERMEDIATE_BUFFER_SIZE + 31));

        /* remove new stream */
        STDVMi_FileClose((STPRM_Handle_t)NewFileHandle_p, (U32)NewFileHandle_p);
        STDVMi_RemoveStream(NewFileHandle_p, NewStreamName);

        STDVMi_FileClose((STPRM_Handle_t)OldFileHandle_p, (U32)OldFileHandle_p);

        STDVMi_RemoveStreamInfo(NewFileHandle_p, NewStreamName);

        STDVMi_ReleaseHandle(NewFileHandle_p);
        STDVMi_ReleaseHandle(OldFileHandle_p);

        return ST_ERROR_NO_MEMORY;
    }
    /* align it to 32 bytes for DMA transfers */
    AlignedBuffer_p = (U8 *) (((U32)Buffer_p + 31) & ~31);

    /* copy index file and get start and end position of stream */
    ErrorCode = STDVMi_CopyIndexFileAndGetStreamPosition(OldFileHandle_p, NewFileHandle_p,
                        StartTimeInMs, EndTimeInMs, &StreamStartPos, &StreamEndPos);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMFS_Error(("CopyStream(%08X) copy index file failed\n", Handle_p));

        memory_deallocate(STDVMi_NCachePartition, Buffer_p);

        /* remove new stream */
        STDVMi_FileClose((STPRM_Handle_t)NewFileHandle_p, (U32)NewFileHandle_p);
        STDVMi_RemoveStream(NewFileHandle_p, NewStreamName);

        STDVMi_FileClose((STPRM_Handle_t)OldFileHandle_p, (U32)OldFileHandle_p);

        STDVMi_RemoveStreamInfo(NewFileHandle_p, NewStreamName);

        STDVMi_ReleaseHandle(NewFileHandle_p);
        STDVMi_ReleaseHandle(OldFileHandle_p);

        return ErrorCode;
    }

    /* align packet position to file system max block size 32k Bytes -> 8k Packets */
    StreamStartPos.LSW &= 0xFFFFE000;

    /* seek to the requested start position */
    if(STDVMi_FileSeek((STPRM_Handle_t)OldFileHandle_p, (U32)OldFileHandle_p, StreamStartPos, STPRM_FS_SEEK_SET) == FS_FAILURE)
    {
        DVMFS_Error(("CopyStream(%08X) copy index file failed\n", Handle_p));

        memory_deallocate(STDVMi_NCachePartition, Buffer_p);

        /* remove new stream */
        STDVMi_FileClose((STPRM_Handle_t)NewFileHandle_p, (U32)NewFileHandle_p);
        STDVMi_RemoveStream(NewFileHandle_p, NewStreamName);

        STDVMi_FileClose((STPRM_Handle_t)OldFileHandle_p, (U32)OldFileHandle_p);

        STDVMi_RemoveStreamInfo(NewFileHandle_p, NewStreamName);

        STDVMi_ReleaseHandle(NewFileHandle_p);
        STDVMi_ReleaseHandle(OldFileHandle_p);

        return ST_ERROR_BAD_PARAMETER;
    }

    while(TRUE)
    {
        I64_Sub(StreamEndPos, StreamStartPos, SizeInPackets);

        Packs2Read = ((SizeInPackets.MSW == 0) && (SizeInPackets.LSW < INTERMEDIATE_BUFFER_SIZE/STDVM_PACKET_SIZE)) ?
                            SizeInPackets.LSW : INTERMEDIATE_BUFFER_SIZE/STDVM_PACKET_SIZE;

        if(Packs2Read <= 0) /* finished */
            break;

         PacksRead = STDVMi_FileRead((STPRM_Handle_t)OldFileHandle_p, (U32)OldFileHandle_p, AlignedBuffer_p, Packs2Read);
        if(PacksRead <= 0)
        {
            DVMFS_Error(("CopyStream(%08X) STDVMi_FileRead(%d)=%d failed\n", Handle_p, Packs2Read, PacksRead));
            break;
        }

        I64_AddLit(StreamStartPos, PacksRead, StreamStartPos);

        PacksWritten = STDVMi_FileWrite((STPRM_Handle_t)NewFileHandle_p, (U32)NewFileHandle_p, AlignedBuffer_p, PacksRead);
        if(PacksWritten != PacksRead)
        {
            DVMFS_Error(("CopyStream(%08X) STDVMi_FileWrite(%d)=%d failed\n", Handle_p, PacksRead, PacksWritten));
            break;
        }
    }

    memory_deallocate(STDVMi_NCachePartition, Buffer_p);

    /* close files */
    STDVMi_FileClose((STPRM_Handle_t)NewFileHandle_p, (U32)NewFileHandle_p);
    STDVMi_FileClose((STPRM_Handle_t)OldFileHandle_p, (U32)OldFileHandle_p);

    STDVMi_ReleaseHandle(NewFileHandle_p);
    STDVMi_ReleaseHandle(OldFileHandle_p);

    return ErrorCode;
}


/*******************************************************************************
Name         : STDVMi_CropStream()

Description  : Copy part of Old stream to a New stram

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle
               char                *StreamName          Stream to crop
               U32                  StartTimeInMs       Start Time
               U32                  EndTimeInMs         End Time

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_CropStream(STDVMi_Handle_t   *Handle_p,
                                 char              *StreamName,
                                 U32                StartTimeInMs,
                                 U32                EndTimeInMs)
{
    ST_ErrorCode_t      ErrorCode;
    STDVMi_Handle_t    *FileHandle_p;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    S64                 AbsoluteStreamStartPos,
                        AbsoluteStreamEndPos,
                        StreamStartPos,
                        StreamEndPos,
                        NewStreamStartPos,
                        NewStreamEndPos,
                        CircularPartEndPos,
                        Temp;
    S32                 FileNb,
                        StartFileNb,
                        EndFileNb;
    vfs_file_t         *FileHandle;
    S32                 BytesRead;
    U32                 StreamStartTimeInMs,
                        StreamDurationInMs;
    S64                 StreamNbOfPackets;


    FileHandle_p = STDVMi_GetFreeHandle();
    if(FileHandle_p == NULL)
    {
        DVMFS_Error(("CropStream(%08X) No free handles\n", Handle_p));
        return ST_ERROR_NO_FREE_HANDLES;
    }
    FileHandle_p->ObjectType = STDVM_OBJECT_PLAYBACK;   /* default object type */

    /* Seperate File name and dir name */
    GET_PATH_AND_FILE_NAME(StreamName, FileHandle_p);

    /* update the OpenFlags needed for Validating no other handle is opened for this stream */
    FileHandle_p->OpenFlags = STDVMi_OPEN_FLAGS_DELETE;

    ErrorCode = STDVMi_ValidateOpen(FileHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        DVMFS_Error(("CropStream(%08X) File already open\n", Handle_p));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ErrorCode;
    }

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  InfStatus,
                    BakStatus;

        GET_INFO_FILE_NAME(LocalName, FileHandle_p);
        if(vfs_stat(LocalName, &InfStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("CropStream(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
            InfStatus.st_size = 0;
        }

        GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMFS_Error(("CropStream(%08X) file stat(%s) failed\n", FileHandle_p, LocalName));
            BakStatus.st_size = 0;
        }

        if((InfStatus.st_size == 0) && (BakStatus.st_size == 0))
        {
            DVMFS_Error(("CropStream(%08X) both Index and Backup file does not exist\n", Handle_p));
            STDVMi_ReleaseHandle(FileHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(InfStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INFO_BACKUP_FILE_NAME(LocalName, FileHandle_p);
        }
        else
        {
            GET_INFO_FILE_NAME(LocalName, FileHandle_p);
        }
    }
#else
    GET_INFO_FILE_NAME(LocalName, FileHandle_p);
#endif /* USE_BACKUP_FILES */

    FileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
    if(FileHandle == NULL)
    {
        DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
        STDVMi_ReleaseHandle(FileHandle_p);
        return STDVM_ERROR_INFO_FILE_OPEN_FAILED;
    }

    BytesRead = vfs_read(FileHandle, FileHandle_p->StreamInfo_p, sizeof(STDVMi_StreamInfo_t));

    vfs_close(FileHandle);

    if(BytesRead < sizeof(STDVMi_StreamInfo_t))
    {
        DVMFS_Error(("CropStream(%08X) file read(%d)=%d failed\n", Handle_p, sizeof(STDVMi_StreamInfo_t), BytesRead));
        STDVMi_ReleaseHandle(FileHandle_p);
        return STDVM_ERROR_INFO_FILE_READ_FAILED;
    }

    if(FileHandle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        DVMFS_Error(("CropStream(%08X) not a valid signature %08X\n", Handle_p, FileHandle_p->StreamInfo_p->Signature));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    ErrorCode = STDVMi_GetTimeAndSize(FileHandle_p, &StreamStartTimeInMs, &StreamDurationInMs, &StreamNbOfPackets);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMFS_Error(("CropStream(%08X) STDVMi_GetTimeAndSize()=%08X failed\n", Handle_p, ErrorCode));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ErrorCode;
    }

    if((StartTimeInMs < StreamStartTimeInMs) ||
       (EndTimeInMs > (StreamStartTimeInMs + StreamDurationInMs)) ||
       (StartTimeInMs >= EndTimeInMs))
    {
        DVMFS_Error(("CropStream(%08X) invalid time limits Old{S[%u],E[%u]} New{S[%u],E[%u]}\n", Handle_p,
                StreamStartTimeInMs, StreamStartTimeInMs + StreamDurationInMs, StartTimeInMs, EndTimeInMs));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* calculate stream size */
    STDVMi_GetStreamSize(FileHandle_p);

    /* need to crop start position */
    if(StartTimeInMs > StreamStartTimeInMs)
    {
        ErrorCode = STDVMi_GetStreamPosition(FileHandle_p, StartTimeInMs, &AbsoluteStreamStartPos);
        if(ErrorCode != ST_NO_ERROR)
        {
            DVMFS_Error(("CropStream(%08X) STDVMi_GetStreamPosition(%d)=%08X failed\n", Handle_p, StartTimeInMs, ErrorCode));
            STDVMi_ReleaseHandle(FileHandle_p);
            return ErrorCode;
        }

        /* current start position */
        if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop))
        {
            StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPos;
        }
        else
        {
            StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop;
        }
        /* check if new start is not same as current start */
        if(!I64_IsEqual(AbsoluteStreamStartPos, StreamStartPos))
        {
            if((FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
               (I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPos)))
            {
                /* actual stream start position */
                StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop;

                /* new stream start position */
                NewStreamStartPos = AbsoluteStreamStartPos;

                /* find start file to delete from */
                I64_DivLit(StreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);
                FileNb = Temp.LSW;

                /* find end file to delete till */
                I64_DivLit(NewStreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);

                /* delete files */
                for(; FileNb < Temp.LSW; FileNb++)
                {
                    GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                    vfs_unlink(LocalName);
                }
            }
            else if(FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
            {
                /* actual stream start position */
                if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop))
                {
                    StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                }
                else
                {
                    StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop;
                }
                /* StreamStartPos % StreamSize */
                while(I64_IsGreaterThan(StreamStartPos, FileHandle_p->StreamSize))
                {
                    I64_Sub(StreamStartPos, FileHandle_p->StreamSize, StreamStartPos);
                }
                I64_DivLit(StreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);
                StartFileNb = Temp.LSW;

                /* actual stream end position */
                if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop))
                {
                    StreamEndPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                }
                else
                {
                    StreamEndPos = FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop;
                }
                /* StreamEndPos % StreamSize */
                while(I64_IsGreaterThan(StreamEndPos, FileHandle_p->StreamSize))
                {
                    I64_Sub(StreamEndPos, FileHandle_p->StreamSize, StreamEndPos);
                }
                I64_DivLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                EndFileNb = Temp.LSW;

                /* New stream start position */
                NewStreamStartPos = AbsoluteStreamStartPos;
                /* NewStreamStartPos % StreamSize */
                while(I64_IsGreaterThan(NewStreamStartPos, FileHandle_p->StreamSize))
                {
                    I64_Sub(NewStreamStartPos, FileHandle_p->StreamSize, NewStreamStartPos);
                }

                /* find start file to delete from */
                if(StartFileNb == EndFileNb)    /* Start and End position are in the same file, dont delete start file */
                {
                    I64_AddLit(StreamStartPos, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                    I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                    FileNb = Temp.LSW;

                    /* resize the start file if the NewStreamStartPos is not in the start file */
                    I64_DivLit(NewStreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    if(StartFileNb != Temp.LSW)
                    {
                        I64_ModLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                        if(Temp.LSW != 0)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                            FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                            if(FileHandle == NULL)
                            {
                                DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                                STDVMi_ReleaseHandle(FileHandle_p);
                                return ST_ERROR_NO_MEMORY;
                            }
                            vfs_resize(FileHandle, Temp.LSW);
                            vfs_close(FileHandle);
                        }
                    }
                }
                else    /* Start and End position are not in the same file, stream is already cropped */
                {
                    /* remove the start file */
                    FileNb = StartFileNb;
                }

                if(I64_IsGreaterThan(NewStreamStartPos, StreamStartPos))
                {
                    /* NewStreamStartPos & StreamStartPos are in the same half of the stream */

                    /* delete files inbetween StreamStartPos and NewStreamStartPos */

                    /* find end file to delete till */
                    I64_DivLit(NewStreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);

                    /* delete files */
                    for(; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }
                }
                else
                {
                    /* NewStreamStartPos & StreamStartPos are not in same half of the stream, stream will no more be circular */

                    /* delete files from StreamStartPos till end */

                    /* find end file to delete till */
                    Temp.LSW = FileHandle_p->StreamInfo_p->NbOfFiles;
                    FileHandle_p->StreamInfo_p->NbOfFiles = FileNb;     /* update new number of files */

                    /* delete files */
                    for(; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }

                    /* delete files from start till NewStreamStartPos */

                    /* find end file to delete till */
                    I64_DivLit(NewStreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);

                    /* delete files */
                    for(FileNb = 0; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }
                }
            }
            else /* circular_linear */
            {
                I64_Add(FileHandle_p->StreamInfo_p->StreamStartPos,FileHandle_p->StreamInfo_p->CircularPartSize,CircularPartEndPos);

                if(I64_IsLessThan(AbsoluteStreamStartPos, CircularPartEndPos))
                {
                    /* actual stream start position */
                    if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop))
                    {
                        StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                    }
                    else
                    {
                        StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop;
                    }
                    /* StreamStartPos % CircularPartSize */
                    while(I64_IsGreaterThan(StreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        I64_Sub(StreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize, StreamStartPos);
                    }
                    I64_DivLit(StreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    StartFileNb = Temp.LSW;

                    /* circular part end position */
                    StreamEndPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                    /* StreamEndPos % CircularPartSize */
                    while(I64_IsGreaterThan(StreamEndPos, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        I64_Sub(StreamEndPos, FileHandle_p->StreamInfo_p->CircularPartSize, StreamEndPos);
                    }
                    I64_DivLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    EndFileNb = Temp.LSW;

                    /* New stream start position */
                    NewStreamStartPos = AbsoluteStreamStartPos;
                    /* NewStreamStartPos % CircularPartSize */
                    while(I64_IsGreaterThan(NewStreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        I64_Sub(NewStreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize, NewStreamStartPos);
                    }

                    /* find start file to delete from */
                    if(StartFileNb == EndFileNb)    /* Start and End position are in the same file, dont delete start file */
                    {
                        I64_AddLit(StreamStartPos, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                        I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                        FileNb = Temp.LSW;

                        /* resize the start file if the NewStreamStartPos is not in the start file */
                        I64_DivLit(NewStreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);
                        if(StartFileNb != Temp.LSW)
                        {
                            I64_ModLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                            if(Temp.LSW != 0)
                            {
                                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                                FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                                if(FileHandle == NULL)
                                {
                                    DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                                    STDVMi_ReleaseHandle(FileHandle_p);
                                    return ST_ERROR_NO_MEMORY;
                                }
                                vfs_resize(FileHandle, Temp.LSW);
                                vfs_close(FileHandle);
                            }
                        }
                    }
                    else    /* Start and End position are not in the same file, stream is already cropped */
                    {
                        /* remove the start file */
                        FileNb = StartFileNb;
                    }

                    if(I64_IsGreaterThan(NewStreamStartPos, StreamStartPos))
                    {
                        /* NewStreamStartPos & StreamStartPos are in the same half of the stream */

                        /* delete files inbetween StreamStartPos and NewStreamStartPos */

                        /* find end file to delete till */
                        I64_DivLit(NewStreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);

                        /* delete files */
                        for(; FileNb < Temp.LSW; FileNb++)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                            vfs_unlink(LocalName);
                        }
                    }
                    else
                    {
                        /* NewStreamStartPos & StreamStartPos are not in same half of the stream, stream will no more be circular */

                        /* delete files from StreamStartPos till circular part end */

                        /* find end file to delete till */
                        I64_AddLit(FileHandle_p->StreamInfo_p->CircularPartSize, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                        I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);

                        /* delete files */
                        for(; FileNb < Temp.LSW; FileNb++)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                            vfs_unlink(LocalName);
                        }

                        /* delete files from start till NewStreamStartPos */

                        /* find end file to delete till */
                        I64_DivLit(NewStreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);

                        /* delete files */
                        for(FileNb = 0; FileNb < Temp.LSW; FileNb++)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                            vfs_unlink(LocalName);
                        }
                    }
                }
                else
                {
                    NewStreamStartPos = AbsoluteStreamStartPos;

                    /* TODO : Find the start file to delete from */

                    /* delete all streams till NewStreamStartPos (Circular part + Linear part) */
                    /* find end file to delete till */
                    I64_AddLit(FileHandle_p->StreamInfo_p->CircularPartSize, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                    I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);   /* nb of files in circular part */
                    FileNb = Temp.LSW;

                    I64_Sub(NewStreamStartPos, CircularPartEndPos, Temp);
                    I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);   /* nb of files in linear part */
                    Temp.LSW += FileNb; /* add cicular part nb of files */

                    /* delete files */
                    for(FileNb = 0; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }
                }
            }
            FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop = AbsoluteStreamStartPos;
        }
    }

    /* need to crop end */
    if(EndTimeInMs < (StreamStartTimeInMs + StreamDurationInMs))
    {
        ErrorCode = STDVMi_GetStreamPosition(FileHandle_p, EndTimeInMs, &AbsoluteStreamEndPos);
        if(ErrorCode != ST_NO_ERROR)
        {
            DVMFS_Error(("CropStream(%08X) STDVMi_GetStreamPosition(%s)=%08X failed\n", Handle_p, StreamName, ErrorCode));
            STDVMi_ReleaseHandle(FileHandle_p);
            return ErrorCode;
        }

        /* current end position */
        if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop))
        {
            StreamEndPos = FileHandle_p->StreamInfo_p->StreamStartPos;
        }
        else
        {
            StreamEndPos = FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop;
        }
        /* check if new end is not same as current end */
        if(!I64_IsEqual(AbsoluteStreamEndPos, StreamEndPos))
        {
            if((FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
               (I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPos)))
            {
                /* actual stream end position */
                StreamEndPos = FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop;

                /* new stream end position */
                NewStreamEndPos = AbsoluteStreamEndPos;

                /* find start file to delete from */
                I64_AddLit(NewStreamEndPos, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                FileNb = Temp.LSW;

                /* resize the new last file */
                I64_ModLit(NewStreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                if(Temp.LSW != 0)
                {
                    GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                    FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                    if(FileHandle == NULL)
                    {
                        DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                        STDVMi_ReleaseHandle(FileHandle_p);
                        return ST_ERROR_NO_MEMORY;
                    }
                    vfs_resize(FileHandle, Temp.LSW);
                    vfs_close(FileHandle);
                }

                FileHandle_p->StreamInfo_p->NbOfFiles = FileNb;     /* update new number of files */

                /* find end file to delete till */
                I64_DivLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);

                /* delete files */
                for(; FileNb <= Temp.LSW; FileNb++)
                {
                    GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                    vfs_unlink(LocalName);
                }
            }
            else if(FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
            {
                /* actual stream start position */
                if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop))
                {
                    StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                }
                else
                {
                    StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop;
                }
                /* StreamStartPos % StreamSize */
                while(I64_IsGreaterThan(StreamStartPos, FileHandle_p->StreamSize))
                {
                    I64_Sub(StreamStartPos, FileHandle_p->StreamSize, StreamStartPos);
                }
                I64_DivLit(StreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);
                StartFileNb = Temp.LSW;

                /* actual stream end position */
                if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop))
                {
                    StreamEndPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                }
                else
                {
                    StreamEndPos = FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop;
                }
                /* StreamEndPos % StreamSize */
                while(I64_IsGreaterThan(StreamEndPos, FileHandle_p->StreamSize))
                {
                    I64_Sub(StreamEndPos, FileHandle_p->StreamSize, StreamEndPos);
                }

                /* New stream end position */
                NewStreamEndPos = AbsoluteStreamEndPos;
                /* NewStreamEndPos % StreamSize */
                while(I64_IsGreaterThan(NewStreamEndPos, FileHandle_p->StreamSize))
                {
                    I64_Sub(NewStreamEndPos, FileHandle_p->StreamSize, NewStreamEndPos);
                }

                if(I64_IsGreaterThan(NewStreamEndPos, StreamEndPos))
                {
                    /* NewStreamEndPos & StreamEndPos are not in same half of the stream, stream will no more be circular */

                    /* find start file to delete from */
                    I64_AddLit(NewStreamEndPos, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                    I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                    FileNb = Temp.LSW;

                    /* resize the new last file */
                    I64_ModLit(NewStreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    if(Temp.LSW != 0)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                        FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                        if(FileHandle == NULL)
                        {
                            DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                            STDVMi_ReleaseHandle(FileHandle_p);
                            return ST_ERROR_NO_MEMORY;
                        }
                        vfs_resize(FileHandle, Temp.LSW);
                        vfs_close(FileHandle);
                    }

                    Temp.LSW = FileHandle_p->StreamInfo_p->NbOfFiles;   /* End file to delete till */
                    FileHandle_p->StreamInfo_p->NbOfFiles = FileNb;     /* update new number of files */

                    /* delete files from NewStreamEndPos till end */
                    for(; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }

                    /* delete files from start till StreamEndPos */
                    /* find end file to delete till */
                    I64_DivLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    if(StartFileNb != Temp.LSW) /* if start position is not in the same file as end position, delete end file */
                    {
                        Temp.LSW++;
                    }

                    /* delete files */
                    for(FileNb = 0; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }
                }
                else
                {
                    /* NewStreamEndPos & StreamEndPos are in the same half of the stream */

                    /* find start file to delete from */
                    I64_AddLit(NewStreamEndPos, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                    I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                    FileNb = Temp.LSW;

                    /* resize the new last file if the NewStreamEndPos is not in the start file */
                    I64_DivLit(NewStreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    if((StartFileNb != Temp.LSW) || (I64_IsLessThan(StreamStartPos, NewStreamEndPos)))
                    {
                        I64_ModLit(NewStreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                        if(Temp.LSW != 0)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                            FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                            if(FileHandle == NULL)
                            {
                                DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                                STDVMi_ReleaseHandle(FileHandle_p);
                                return ST_ERROR_NO_MEMORY;
                            }
                            vfs_resize(FileHandle, Temp.LSW);
                            vfs_close(FileHandle);
                        }
                    }

                    /* delete files inbetween NewStreamEndPos and StreamEndPos */
                    /* find end file to delete till */
                    I64_DivLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    if(StartFileNb != Temp.LSW) /* if start position is not in the same file as end position, delete end file */
                    {
                        Temp.LSW++;
                    }

                    /* delete files */
                    for(; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }
                }
            }
            else /* circular_linear */
            {
                I64_Add(FileHandle_p->StreamInfo_p->StreamStartPos,FileHandle_p->StreamInfo_p->CircularPartSize,CircularPartEndPos);

                if(I64_IsLessThan(AbsoluteStreamEndPos, CircularPartEndPos))
                {
                    /* actual stream start position */
                    if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop))
                    {
                        StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                    }
                    else
                    {
                        StreamStartPos = FileHandle_p->StreamInfo_p->StreamStartPosAfterCrop;
                    }
                    /* StreamStartPos % CircularPartSize */
                    while(I64_IsGreaterThan(StreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        I64_Sub(StreamStartPos, FileHandle_p->StreamInfo_p->CircularPartSize, StreamStartPos);
                    }
                    I64_DivLit(StreamStartPos, FS_MAX_SUB_FILE_SIZE, Temp);
                    StartFileNb = Temp.LSW;

                    /* circular part end position */
                    StreamEndPos = FileHandle_p->StreamInfo_p->StreamStartPos;
                    /* StreamEndPos % CircularPartSize */
                    while(I64_IsGreaterThan(StreamEndPos, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        I64_Sub(StreamEndPos, FileHandle_p->StreamInfo_p->CircularPartSize, StreamEndPos);
                    }

                    /* New stream end position */
                    NewStreamEndPos = AbsoluteStreamEndPos;
                    /* AbsoluteStreamEndPos % CircularPartSize */
                    while(I64_IsGreaterThan(NewStreamEndPos, FileHandle_p->StreamInfo_p->CircularPartSize))
                    {
                        I64_Sub(NewStreamEndPos, FileHandle_p->StreamInfo_p->CircularPartSize, NewStreamEndPos);
                    }

                    if(I64_IsGreaterThan(NewStreamEndPos, StreamEndPos))
                    {
                        /* NewStreamEndPos & StreamEndPos are not in same half of the stream, stream will no more be circular */

                        /* find start file to delete from */
                        I64_AddLit(NewStreamEndPos, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                        I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                        FileNb = Temp.LSW;

                        /* resize the new last file */
                        I64_ModLit(NewStreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                        if(Temp.LSW != 0)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                            FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                            if(FileHandle == NULL)
                            {
                                DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                                STDVMi_ReleaseHandle(FileHandle_p);
                                return ST_ERROR_NO_MEMORY;
                            }
                            vfs_resize(FileHandle, Temp.LSW);
                            vfs_close(FileHandle);
                        }

                        Temp.LSW = FileHandle_p->StreamInfo_p->NbOfFiles;   /* End file to delete till */
                        FileHandle_p->StreamInfo_p->NbOfFiles = FileNb;     /* update new number of files */

                        /* delete files from NewStreamEndPos till end */
                        for(; FileNb < Temp.LSW; FileNb++)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                            vfs_unlink(LocalName);
                        }

                        /* delete files from start till StreamEndPos */
                        /* find end file to delete till */
                        I64_DivLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                        if(StartFileNb != Temp.LSW) /* if start position is not in the same file as end position, delete end file */
                        {
                            Temp.LSW++;
                        }

                        /* delete files */
                        for(FileNb = 0; FileNb < Temp.LSW; FileNb++)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                            vfs_unlink(LocalName);
                        }

                        /* update circular_linear variables since the resulting file is no more circular_linear */
                        FileHandle_p->StreamInfo_p->StreamFileType = STREAM_FILE_TYPE_CIRCULAR;
                    }
                    else
                    {
                        /* NewStreamEndPos & StreamEndPos are in the same half of the stream */

                        /* find start file to delete from */
                        I64_AddLit(NewStreamEndPos, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                        I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                        FileNb = Temp.LSW;

                        /* resize the new last file if the NewStreamEndPos is not in the start file */
                        I64_DivLit(NewStreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                        if((StartFileNb != Temp.LSW) || (I64_IsLessThan(StreamStartPos, NewStreamEndPos)))
                        {
                            I64_ModLit(NewStreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                            if(Temp.LSW != 0)
                            {
                                GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                                FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                                if(FileHandle == NULL)
                                {
                                    DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                                    STDVMi_ReleaseHandle(FileHandle_p);
                                    return ST_ERROR_NO_MEMORY;
                                }
                                vfs_resize(FileHandle, Temp.LSW);
                                vfs_close(FileHandle);
                            }
                        }

                        /* delete files inbetween NewStreamEndPos and StreamEndPos */
                        /* find end file to delete till */
                        I64_DivLit(StreamEndPos, FS_MAX_SUB_FILE_SIZE, Temp);
                        if(StartFileNb != Temp.LSW) /* if start position is not in the same file as end position, delete end file */
                        {
                            Temp.LSW++;
                        }

                        /* delete files */
                        for(; FileNb < Temp.LSW; FileNb++)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                            vfs_unlink(LocalName);
                        }

                        /* delete files in the linear part */
                        I64_AddLit(FileHandle_p->StreamInfo_p->CircularPartSize, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                        I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                        for(FileNb = Temp.LSW; FileNb < FileHandle_p->StreamInfo_p->NbOfFiles; FileNb++)
                        {
                            GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                            vfs_unlink(LocalName);
                        }

                        FileHandle_p->StreamInfo_p->NbOfFiles = Temp.LSW;   /* update new number of files */

                        /* update circular_linear variables since the resulting file is no more circular_linear */
                        FileHandle_p->StreamInfo_p->StreamFileType = STREAM_FILE_TYPE_CIRCULAR;
                    }
                }
                else
                {
                    /* New stream end position */
                    NewStreamEndPos = AbsoluteStreamEndPos;

                    /* find start file to delete from */
                    I64_AddLit(FileHandle_p->StreamInfo_p->CircularPartSize, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                    I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                    FileNb = Temp.LSW;  /* cicular part number of files */

                    I64_Sub(NewStreamEndPos, CircularPartEndPos, Temp);
                    I64_AddLit(Temp, (FS_MAX_SUB_FILE_SIZE - 1), Temp);
                    I64_DivLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                    FileNb += Temp.LSW; /* linear part number of files */

                    /* resize the new last file */
                    I64_Sub(NewStreamEndPos, CircularPartEndPos, Temp);
                    I64_ModLit(Temp, FS_MAX_SUB_FILE_SIZE, Temp);
                    if(Temp.LSW != 0)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb - 1);
                        FileHandle = vfs_open(LocalName, OSPLUS_O_WRONLY);
                        if(FileHandle == NULL)
                        {
                            DVMFS_Error(("CropStream(%08X) file open(%s) failed\n", Handle_p, LocalName));
                            STDVMi_ReleaseHandle(FileHandle_p);
                            return ST_ERROR_NO_MEMORY;
                        }
                        vfs_resize(FileHandle, Temp.LSW);
                        vfs_close(FileHandle);
                    }

                    Temp.LSW = FileHandle_p->StreamInfo_p->NbOfFiles;   /* End file to delete till */
                    FileHandle_p->StreamInfo_p->NbOfFiles = FileNb;     /* update new number of files */

                    /* delete files from NewStreamEndPos till end */
                    for(; FileNb < Temp.LSW; FileNb++)
                    {
                        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileNb);
                        vfs_unlink(LocalName);
                    }
                }
            }
            FileHandle_p->StreamInfo_p->StreamEndPosAfterCrop = AbsoluteStreamEndPos;
        }
    }

    /* to update stream info */
    FileHandle_p->OpenFlags = OSPLUS_O_WRONLY;
    STDVMi_UpdateStreamInfoToDisk(FileHandle_p);

    STDVMi_ReleaseHandle(FileHandle_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_SwitchCircularToLinear()

Description  : Change stream type from circular to linear

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_SwitchCircularToLinear(STDVMi_Handle_t *FileHandle_p)
{
    ST_ErrorCode_t      ErrorCode;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_file_t         *StreamFileHandle;


    if(FileHandle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        DVMFS_Error(("SwitchCircularToLinear(%08X) file not Open [%08X]\n", FileHandle_p, FileHandle_p->StreamInfo_p->Signature));
        return ST_ERROR_BAD_PARAMETER;
    }

    if(FileHandle_p->ObjectType != STDVM_OBJECT_RECORD)
    {
        DVMFS_Error(("SwitchCircularToLinear(%08X) not a record Object[%u]\n", FileHandle_p, FileHandle_p->ObjectType));
        return ST_ERROR_BAD_PARAMETER;
    }

    STREAM_ACCESS_MUTEX_LOCK(FileHandle_p);

    if((FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
       (FileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR))
    {
        DVMFS_Error(("SwitchCircularToLinear(%08X) Stream type[%u] not circular\n", FileHandle_p,
                FileHandle_p->StreamInfo_p->StreamFileType));
        STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* stream type is circular but not wrapped, just change the stream type */
    if(I64_IsZero(FileHandle_p->StreamInfo_p->StreamStartPos))
    {
        FileHandle_p->StreamInfo_p->StreamFileType = STREAM_FILE_TYPE_LINEAR;

        I64_SetValue(0, 0, FileHandle_p->MaxStreamSize);
        FileHandle_p->MaxIndexEntries = 0;

        if(FileHandle_p->SameFileHandle_p != NULL)
        {
            STREAM_ACCESS_MUTEX_LOCK(FileHandle_p->SameFileHandle_p);

            FileHandle_p->SameFileHandle_p->StreamInfo_p->StreamFileType = STREAM_FILE_TYPE_LINEAR;

            I64_SetValue(0, 0, FileHandle_p->SameFileHandle_p->MaxStreamSize);
            FileHandle_p->SameFileHandle_p->MaxIndexEntries = 0;

            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p->SameFileHandle_p);
        }
    }
    else
    {
        GET_STREAM_FILE_NAME(LocalName, FileHandle_p, FileHandle_p->StreamInfo_p->NbOfFiles);

        StreamFileHandle = vfs_open(LocalName, FileHandle_p->OpenFlags);
        if(StreamFileHandle == NULL)
        {
            DVMFS_Error(("SwitchCircularToLinear(%08X) file open(%s) failed\n", FileHandle_p, LocalName));
            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        vfs_close(FileHandle_p->StreamFileHandle);
        FileHandle_p->StreamFileHandle = StreamFileHandle;

        FileHandle_p->CurrFileNo  = FileHandle_p->StreamInfo_p->NbOfFiles;
        FileHandle_p->CurrFilePos = 0;

        FileHandle_p->StreamInfo_p->NbOfFiles++;

        /* update all fields getting affected */
        FileHandle_p->StreamInfo_p->StreamFileType = STREAM_FILE_TYPE_CIRCULAR_LINEAR;

        I64_SetValue(0, 0, FileHandle_p->MaxStreamSize);
        FileHandle_p->MaxIndexEntries = 0;

        if(FileHandle_p->SameFileHandle_p != NULL)
        {
            STREAM_ACCESS_MUTEX_LOCK(FileHandle_p->SameFileHandle_p);

            FileHandle_p->SameFileHandle_p->StreamInfo_p->NbOfFiles = FileHandle_p->StreamInfo_p->NbOfFiles;

            FileHandle_p->SameFileHandle_p->StreamInfo_p->StreamFileType = STREAM_FILE_TYPE_CIRCULAR_LINEAR;

            I64_SetValue(0, 0, FileHandle_p->SameFileHandle_p->MaxStreamSize);
            FileHandle_p->SameFileHandle_p->MaxIndexEntries = 0;

            STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p->SameFileHandle_p);
        }
    }

    ErrorCode = STDVMi_UpdateStreamInfoToDisk(FileHandle_p);

    STREAM_ACCESS_MUTEX_RELEASE(FileHandle_p);

    return ErrorCode;
}


/*******************************************************************************
Name         : STDVMi_CheckKey()

Description  : Checks whether the key passed is a right by looking for SOP

Parameters   : STDVMi_Handle_t     *Handle_p            STDVM internal handle
               char                *OldStreamName       Old Stream name
               U32                  StartTimeInMs       Start Time
               U32                  EndTimeInMs         End Time
               char                *NewStreamName       New Stream name

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_CheckKey(STDVMi_Handle_t *Handle_p, char *StreamName, STDVM_Key_t Key)
{
    ST_ErrorCode_t      ErrorCode;
    STDVMi_Handle_t    *FileHandle_p;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    vfs_file_t         *FileHandle;
    S32                 BytesRead;
    U8                 *Buffer_p;
    U8                 *AlignedBuffer_p;
    U8                  PacketCount;


    FileHandle_p = STDVMi_GetFreeHandle();
    if(FileHandle_p == NULL)
    {
        DVMFS_Error(("CheckKey(%08X) No free handles\n", Handle_p));
        return ST_ERROR_NO_FREE_HANDLES;
    }

    /* Seperate File name and dir name */
    GET_PATH_AND_FILE_NAME(StreamName, FileHandle_p);

    /* update the OpenFlags needed for Validate */
    Handle_p->OpenFlags = OSPLUS_O_RDONLY;

    ErrorCode = STDVMi_ValidateOpen(FileHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        DVMFS_Error(("CheckKey(%08X) File already open\n", Handle_p));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ErrorCode;
    }

#ifndef ST_OSLINUX
    Buffer_p = memory_allocate(STDVMi_NCachePartition, OSPLUS_FILE_CACHE_SECTORS*HDD_SECTOR_SIZE + 31);
    if(Buffer_p == NULL)
#else
    ErrorCode = UTILS_AllocData_DVM(STDVMi_TS_RECORD_CHUNK_SIZE, 32, (void *)&Buffer_p);
    if(ErrorCode != ST_NO_ERROR)
#endif
    {
        DVMFS_Error(("CheckKey(%08X) memory alloc(%d) failed\n", Handle_p, OSPLUS_FILE_CACHE_SECTORS*HDD_SECTOR_SIZE + 31));
        STDVMi_ReleaseHandle(FileHandle_p);
        return ST_ERROR_NO_MEMORY;
    }

    /* open any trp file in the directory to check key validity */
    {
        vfs_dir_t              *CurDir_p=NULL;
        vfs_dirent_t            Dirent,
                               *Dirent_p = &Dirent;
        U32                     StrLen;

        GET_STREAM_DIR_NAME(LocalName, FileHandle_p);

        CurDir_p = vfs_opendir(LocalName);
        if (CurDir_p == NULL)
        {
            DVMFS_Error(("CheckKey(%08X) Directory open(%s) failes\n", Handle_p, LocalName));
#ifndef ST_OSLINUX
            memory_deallocate(STDVMi_NCachePartition, Buffer_p);
#else
            UTILS_FreeData_DVM((void *)Buffer_p);
#endif
            STDVMi_ReleaseHandle(FileHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        while((vfs_readdir(CurDir_p, Dirent_p)) == OSPLUS_SUCCESS)
        {
            if(DIRENT_IS_FILE(Dirent_p))
            {
                StrLen = strlen(Dirent_p->d_name);
                if((StrLen > 4) && (strcmp(Dirent_p->d_name + StrLen - 4, STDVMi_STREAM_FILE_EXT)==0))
                {
                    /* stream file, open this file for checking key validity */
                    strcat(LocalName, "/");
                    strcat(LocalName, Dirent_p->d_name);
                    break;
                }
            }
        }
        vfs_closedir(CurDir_p);
    }

    FileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
    if(FileHandle == NULL)
    {
        DVMFS_Error(("CheckKey(%08X) file open(%s) failed\n", Handle_p, LocalName));
#ifndef ST_OSLINUX
        memory_deallocate(STDVMi_NCachePartition, Buffer_p);
#else
        UTILS_FreeData_DVM(Buffer_p);
#endif
        STDVMi_ReleaseHandle(FileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    AlignedBuffer_p = (U8 *)(((U32)Buffer_p + 31) & (~31));

    /* set the handle to be Active */
    FileHandle_p->StreamInfo_p->Signature = STDVMi_SIGNATURE;

    /* Set params for Decrypt */
    FileHandle_p->TSBufferStart = AlignedBuffer_p;
    FileHandle_p->TSBufferEnd   = AlignedBuffer_p + OSPLUS_FILE_CACHE_SECTORS*HDD_SECTOR_SIZE - 1;

    FileHandle_p->EnableCrypt   = TRUE;
    memcpy(FileHandle_p->CryptKey, Key, sizeof(STDVM_Key_t));

    BytesRead = vfs_read(FileHandle, AlignedBuffer_p, OSPLUS_FILE_CACHE_SECTORS*HDD_SECTOR_SIZE);

    vfs_close(FileHandle);

    if(BytesRead < (OSPLUS_FILE_CACHE_SECTORS*HDD_SECTOR_SIZE))
    {
        DVMFS_Error(("CheckKey(%08X) file read(%d)=%d failed\n", Handle_p, OSPLUS_FILE_CACHE_SECTORS*HDD_SECTOR_SIZE,BytesRead));
#ifndef ST_OSLINUX
        memory_deallocate(STDVMi_NCachePartition, Buffer_p);
#else
        UTILS_FreeData_DVM(Buffer_p);
#endif
        STDVMi_ReleaseHandle(FileHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

#ifdef STDVM_ENABLE_CRYPT
    if(FileHandle_p->EnableCrypt)
    {
        if(STDVMi_Decrypt(FileHandle_p, AlignedBuffer_p, AlignedBuffer_p, BytesRead) != ST_NO_ERROR)
        {
            DVMFS_Error(("CheckKey(%08X) STDVMi_Decrypt(%08X,%d) failed\n", FileHandle_p, AlignedBuffer_p, BytesRead));
#ifndef ST_OSLINUX
            memory_deallocate(STDVMi_NCachePartition, Buffer_p);
#else
            UTILS_FreeData_DVM(Buffer_p);
#endif
            STDVMi_ReleaseHandle(FileHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }
    }
#endif

    for(PacketCount = 0; PacketCount < NB_OF_PACKETS_TO_CHECK; PacketCount++)
    {
        if(AlignedBuffer_p[PacketCount*STDVM_PACKET_SIZE] != DVB_SOP)
        {
            DVMFS_Error(("CheckKey(%08X) Packet[%d]=0x%02X failed\n", Handle_p, PacketCount, DVB_SOP));
#ifndef ST_OSLINUX
            memory_deallocate(STDVMi_NCachePartition, Buffer_p);
#else
            UTILS_FreeData_DVM(Buffer_p);
#endif
            STDVMi_ReleaseHandle(FileHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }
    }

#ifndef ST_OSLINUX
    memory_deallocate(STDVMi_NCachePartition, Buffer_p);
#else
    UTILS_FreeData_DVM(Buffer_p);
#endif

    STDVMi_ReleaseHandle(FileHandle_p);

    return ST_NO_ERROR;
}


/* EOF --------------------------------------------------------------------- */
