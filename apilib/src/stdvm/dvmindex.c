/*****************************************************************************

File Name  : dvmindex.c

Description: Index manipulation functions for STPRM

Copyright (C) 2005 STMicroelectronics

*****************************************************************************/

/* Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvmindex.h"
#include "dvmfs.h"
#include "handle.h"
#include "dvmtask.h"

#ifdef ENABLE_TRACE
 #include "trace.h"
#endif


/* Private Constants ------------------------------------------------------ */
#if defined(USE_BACKUP_FILES) && !defined(OPEN_CLOSE_FILE_ON_WRITE)
    /* OPEN_CLOSE_FILE_ON_WRITE must be defined when defining USE_BACKUP_FILES */
    #define OPEN_CLOSE_FILE_ON_WRITE
#endif

#define INDEX_FILE_LOOKUP_OFFSET    (100)           /* No of entries to retain in previous buffer */

/* Private Types ---------------------------------------------------------- */

/* Private Variables ------------------------------------------------------ */

/* Private Macros --------------------------------------------------------- */

/* debug options */
#if defined(ENABLE_TRACE)
    #define DVMINDEX_Error(__X__)      TraceBuffer(__X__)
    #define DVMINDEX_Trace(__X__)      TraceBuffer(__X__)
#elif defined(STDVM_DEBUG)
    #define DVMINDEX_Error(__X__)      STTBX_Print(__X__)
    #define DVMINDEX_Trace(__X__)      /*nop*/
#else
    #define DVMINDEX_Error(__X__)      /*nop*/
    #define DVMINDEX_Trace(__X__)      /*nop*/
#endif


/* Private Function prototypes -------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

/* Prototypes ------------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */

/*******************************************************************************
Name         : STDVMi_RecordCallbackStart()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_RecordCallbackStart(STPRM_Handle_t PRM_Handle)
{
    ST_ErrorCode_t          ErrorCode;
    STDVMi_Handle_t        *Handle_p;
    char                    LocalName[STDVMi_MAX_NAME_LENGTH];
    STPRM_RecordParams_t    PRMRecordParams;


    DVMINDEX_Trace(("STDVMi_RecordCallbackStart(%08X)\n", PRM_Handle));

    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL)
    {
        DVMINDEX_Error(("RCBStart(%08X) Get STDVMi_Handle failed\n", PRM_Handle));
        return ST_ERROR_BAD_PARAMETER;
    }

    INDEX_ACCESS_MUTEX_CREATE(Handle_p);

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    GET_INDEX_FILE_NAME(LocalName, Handle_p);

    Handle_p->IndexFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(Handle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("RCBStart(%08X) file open(%s) failed\n", Handle_p, LocalName));
        INDEX_ACCESS_MUTEX_DELETE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Reset IndexInfo_p buffer to 0 */
    memset(Handle_p->IndexInfo_p, 0, STDVMi_INDEX_INFO_CACHE_SIZE*sizeof(STDVMi_IndexInfo_t));

    Handle_p->IndexNbOfEntInCache = 0;
    Handle_p->IndexNbOfEntInFile  = 0;

    Handle_p->StreamMinTime    = 0;
    Handle_p->StreamMaxTime    = 0;

    Handle_p->IndexPosAbsolute = 0;
    Handle_p->IndexPosInCache  = 0;

    I64_SetValue(0, 0, Handle_p->ElapsedNumPackets);
    Handle_p->ElapsedTimeInMs = 0;

    I64_SetValue(0, 0, Handle_p->StreamMaxPos);

    Handle_p->StreamInfo_p->FirstIndexPos = 0;

    Handle_p->StreamInfo_p->CircularPartIndexEntries = 0;

#ifdef OPEN_CLOSE_FILE_ON_WRITE
    vfs_close(Handle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

#ifdef USE_BACKUP_FILES
    GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);

    Handle_p->IndexFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(Handle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("RCBStart(%08X) file open(%s) failed\n", Handle_p, LocalName));
        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    vfs_close(Handle_p->IndexFileHandle);
#endif  /* USE_BACKUP_FILES */

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

#ifndef ST_OSLINUX  /* mutex not supported properly in STLinux */
    ErrorCode = STPRM_RecordGetParams(PRM_Handle, &PRMRecordParams);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMINDEX_Error(("PCBStop(%08X) STPRM_RecordGetParams(%08X) failed\n", Handle_p, PRM_Handle));
#ifndef OPEN_CLOSE_FILE_ON_WRITE
        vfs_close(Handle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */
        return ST_ERROR_BAD_PARAMETER;
    }

    osplus_uncached_register(PRMRecordParams.TSBuffer, PRMRecordParams.TSBufferSize);

    /* Store TS Buffer limits used for Encrypt/Decrypt */
    Handle_p->TSBufferStart = (void *)(PRMRecordParams.TSBuffer);
    Handle_p->TSBufferEnd   = (void *)(PRMRecordParams.TSBuffer+PRMRecordParams.TSBufferSize-1);
#else
    UNUSED_PARAMETER(ErrorCode);
    UNUSED_PARAMETER(PRMRecordParams);
#endif

    DVMINDEX_Trace(("\tSTDVMi_RecordCallbackStart(%08X)\n\n", Handle_p));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_RecordCallbackPacket()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_RecordCallbackPacket(STPRM_Handle_t PRM_Handle, U32 PacketTimeInMs, S64 PacketPosition)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STDVMi_Handle_t        *Handle_p;
    S32                     BytesWritten;


    DVMINDEX_Trace(("STDVMi_RecordCallbackPacket(%08X): PackTimemMS[%u] PackPos[%u,%u]\n", PRM_Handle,
            PacketTimeInMs, PacketPosition.MSW, PacketPosition.LSW));

    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL)
    {
        DVMINDEX_Error(("RCBPacket(%08X) Get STDVMi_Handle failed\n", PRM_Handle));
        return ST_ERROR_BAD_PARAMETER;
    }

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    /* Add this index to the cache */
    Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache].PacketTimeInMs = PacketTimeInMs;
    Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache].PacketPosition = PacketPosition;
    Handle_p->StreamMaxTime                                             = PacketTimeInMs;
    Handle_p->StreamMaxPos                                              = PacketPosition;

    Handle_p->IndexNbOfEntInCache++;

    Handle_p->IndexPosAbsolute++;
    Handle_p->IndexPosInCache++;

    /* updating stream MAX time for the other opened handle */
    if(Handle_p->SameFileHandle_p != NULL)
    {
        SHARED_FIELD_ACCESS_MUTEX_LOCK(Handle_p);

        Handle_p->SameFileHandle_p->StreamMaxTime = PacketTimeInMs;
        Handle_p->SameFileHandle_p->StreamMaxPos  = PacketPosition;

        SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    /* Flush datas on the hard disk */
    if(Handle_p->IndexNbOfEntInCache >= STDVMi_INDEX_INFO_CACHE_SIZE)
    {
#ifdef USE_BACKUP_FILES
        {
            char                LocalName[STDVMi_MAX_NAME_LENGTH];

            GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);

            Handle_p->IndexFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
            if(Handle_p->IndexFileHandle == NULL)
            {
                DVMINDEX_Error(("RCBPacket(%08X) file open(%s) failed\n", Handle_p, LocalName));
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }
        }

        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR) ||
           (Handle_p->IndexNbOfEntInFile < Handle_p->MaxIndexEntries))
        {
            /* seek to the end of Index file */
            vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);
        }
        else
        {
            /* seek to the start of Index in circular files */
            vfs_fseek(Handle_p->IndexFileHandle,
                      Handle_p->StreamInfo_p->FirstIndexPos*sizeof(STDVMi_IndexInfo_t),
                      OSPLUS_SEEK_SET);
        }

        BytesWritten = vfs_write(Handle_p->IndexFileHandle,
                                 Handle_p->IndexInfo_p,
                                 Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t));
        if(BytesWritten != Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t))
        {
            DVMINDEX_Error(("RCBPacket(%08X) file write(%d)=%d failed\n", Handle_p,
                    Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t), BytesWritten));
            ErrorCode = ST_ERROR_NO_MEMORY;
        }

        vfs_close(Handle_p->IndexFileHandle);
#endif  /* USE_BACKUP_FILES */

#ifdef OPEN_CLOSE_FILE_ON_WRITE
        {
            char                LocalName[STDVMi_MAX_NAME_LENGTH];

            GET_INDEX_FILE_NAME(LocalName, Handle_p);

            Handle_p->IndexFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
            if(Handle_p->IndexFileHandle == NULL)
            {
                DVMINDEX_Error(("RCBPacket(%08X) file open(%s) failed\n", Handle_p, LocalName));
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }
        }

        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR) ||
           (Handle_p->IndexNbOfEntInFile < Handle_p->MaxIndexEntries))
        {
            /* seek to the end of Index file */
            vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);
        }
        else
        {
            /* seek to the start of Index in circular files */
            vfs_fseek(Handle_p->IndexFileHandle,
                      Handle_p->StreamInfo_p->FirstIndexPos*sizeof(STDVMi_IndexInfo_t),
                      OSPLUS_SEEK_SET);
        }
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

        /* estimate current bandwidth */
        I64_Sub(Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache - 1].PacketPosition,
                Handle_p->IndexInfo_p[0].PacketPosition,
                Handle_p->ElapsedNumPackets);

        Handle_p->ElapsedTimeInMs = Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache - 1].PacketTimeInMs
                                        - Handle_p->IndexInfo_p[0].PacketTimeInMs;

        BytesWritten = vfs_write(Handle_p->IndexFileHandle,
                                 Handle_p->IndexInfo_p,
                                 Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t));
        if(BytesWritten != Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t))
        {
            DVMINDEX_Error(("RCBPacket(%08X) file write(%d)=%d failed\n", Handle_p,
                    Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t), BytesWritten));
            ErrorCode = ST_ERROR_NO_MEMORY;
        }

        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR) &&
           (Handle_p->IndexPosAbsolute >= Handle_p->MaxIndexEntries))
        {
            if(I64_IsZero(Handle_p->StreamInfo_p->StreamStartPos))
            {
                DVMINDEX_Error(("RCBPacket(%08X) Index file entries[%u] wraps before Stream file\n", Handle_p,
                        Handle_p->IndexPosAbsolute));
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }

            Handle_p->IndexPosAbsolute = 0;

            /* seek to the start of file */
            vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_SET);
            DVMINDEX_Trace(("\tSTDVMi_RecordCallbackPacket(%08X) circular Index file wrapped FirstIndexPos[%u]\n", Handle_p,
                    Handle_p->StreamInfo_p->FirstIndexPos));
        }

#ifdef OPEN_CLOSE_FILE_ON_WRITE
        vfs_close(Handle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

        if(Handle_p->SameFileHandle_p != NULL)
        {
            SHARED_FIELD_ACCESS_MUTEX_LOCK(Handle_p);
        }

        /* update IndexNbOfEntInFile till file wrap around */
        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR) ||
           (Handle_p->IndexNbOfEntInFile < Handle_p->MaxIndexEntries))
        {
            Handle_p->IndexNbOfEntInFile += Handle_p->IndexNbOfEntInCache;
        }
        else
        {
            /* FirstIndexPos updated after wrap around whenever there is a write to index file */
            Handle_p->StreamInfo_p->FirstIndexPos = Handle_p->IndexPosAbsolute;

            /* update INFO file since change in FirstIndexPos */
            STDVMi_UpdateStreamInfoToDisk(Handle_p);
        }

        Handle_p->IndexNbOfEntInCache = 0;

        Handle_p->IndexPosInCache     = 0;

        /* Reset IndexInfo_p buffer to 0 */
        memset(Handle_p->IndexInfo_p, 0, STDVMi_INDEX_INFO_CACHE_SIZE*sizeof(STDVMi_IndexInfo_t));

        if(Handle_p->SameFileHandle_p != NULL)
        {
            Handle_p->SameFileHandle_p->IndexNbOfEntInFile = Handle_p->IndexNbOfEntInFile;
            Handle_p->SameFileHandle_p->StreamInfo_p->FirstIndexPos = Handle_p->StreamInfo_p->FirstIndexPos;

            SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);
        }
    }

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    DVMINDEX_Trace(("\tSTDVMi_RecordCallbackPacket(%08X): IPos{C[%u] A[%u]} NbEnt{C[%d] F[%d]}\n\n", Handle_p,
            Handle_p->IndexPosInCache, Handle_p->IndexPosAbsolute,
            Handle_p->IndexNbOfEntInCache, Handle_p->IndexNbOfEntInFile));

    return ErrorCode;
}


/*******************************************************************************
Name         : STDVMi_RecordCallbackStop()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_RecordCallbackStop(STPRM_Handle_t PRM_Handle)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STDVMi_Handle_t        *Handle_p;
    S32                     BytesWritten;
    STPRM_RecordParams_t    PRMRecordParams;


    DVMINDEX_Trace(("STDVMi_RecordCallbackStop(%08X):\n", PRM_Handle));

    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL)
    {
        DVMINDEX_Error(("RCBStop(%08X) Get STDVMi_Handle failed\n", PRM_Handle));
        return ST_ERROR_BAD_PARAMETER;
    }

#ifndef ST_OSLINUX  /* mutex not supported properly in STLinux */
    ErrorCode = STPRM_RecordGetParams(PRM_Handle, &PRMRecordParams);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMINDEX_Error(("PCBStop(%08X) STPRM_RecordGetParams(%08X) failed\n", Handle_p, PRM_Handle));
        return ST_ERROR_BAD_PARAMETER;
    }

    osplus_uncached_unregister(PRMRecordParams.TSBuffer, PRMRecordParams.TSBufferSize);
#else
    UNUSED_PARAMETER(ErrorCode);
    UNUSED_PARAMETER(PRMRecordParams);
#endif

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    /* Flush datas on the hard disk */
    if(Handle_p->IndexNbOfEntInCache != 0)
    {
#ifdef OPEN_CLOSE_FILE_ON_WRITE
        {
            char                LocalName[STDVMi_MAX_NAME_LENGTH];

            GET_INDEX_FILE_NAME(LocalName, Handle_p);

            Handle_p->IndexFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
            if(Handle_p->IndexFileHandle == NULL)
            {
                DVMINDEX_Error(("RCBStop(%08X) file open(%s) failed\n", Handle_p, LocalName));
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }
        }

        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR) ||
           (Handle_p->IndexNbOfEntInFile < Handle_p->MaxIndexEntries))
        {
            /* seek to the end of Index file */
            vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);
        }
        else
        {
            /* seek to the start of Index in circular files */
            vfs_fseek(Handle_p->IndexFileHandle,
                      Handle_p->StreamInfo_p->FirstIndexPos*sizeof(STDVMi_IndexInfo_t),
                      OSPLUS_SEEK_SET);
        }
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

        BytesWritten = vfs_write(Handle_p->IndexFileHandle,
                                 Handle_p->IndexInfo_p,
                                 Handle_p->IndexNbOfEntInCache*sizeof(STDVMi_IndexInfo_t));
        if(BytesWritten != Handle_p->IndexNbOfEntInCache*sizeof(STDVMi_IndexInfo_t))
        {
            DVMINDEX_Error(("RCBStop(%08X) file write(%d)=%d failed\n", Handle_p,
                    Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t), BytesWritten));
            ErrorCode = ST_ERROR_NO_MEMORY;
        }

#ifdef OPEN_CLOSE_FILE_ON_WRITE
        vfs_close(Handle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

#ifdef USE_BACKUP_FILES
        {
            char                LocalName[STDVMi_MAX_NAME_LENGTH];

            GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);

            Handle_p->IndexFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
            if(Handle_p->IndexFileHandle == NULL)
            {
                DVMINDEX_Error(("RCBStop(%08X) file open(%s) failed\n", Handle_p, LocalName));
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }
        }

        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR) ||
           (Handle_p->IndexNbOfEntInFile < Handle_p->MaxIndexEntries))
        {
            /* seek to the end of Index file */
            vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);
        }
        else
        {
            /* seek to the start of Index in circular files */
            vfs_fseek(Handle_p->IndexFileHandle,
                      Handle_p->StreamInfo_p->FirstIndexPos*sizeof(STDVMi_IndexInfo_t),
                      OSPLUS_SEEK_SET);
        }

        /* Flush datas to the hard disk */
        BytesWritten = vfs_write(Handle_p->IndexFileHandle,
                                 Handle_p->IndexInfo_p,
                                 Handle_p->IndexNbOfEntInCache*sizeof(STDVMi_IndexInfo_t));
        if(BytesWritten != Handle_p->IndexNbOfEntInCache*sizeof(STDVMi_IndexInfo_t))
        {
            DVMINDEX_Error(("RCBStop(%08X) file write(%d)=%d failed\n", Handle_p,
                    Handle_p->IndexNbOfEntInCache * sizeof(STDVMi_IndexInfo_t), BytesWritten));
            ErrorCode = ST_ERROR_NO_MEMORY;
        }

        vfs_close(Handle_p->IndexFileHandle);
#endif /* USE_BACKUP_FILES */

        /* FirstIndexPos updated after wrap around whenever there is a write to index file */
        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR) &&
           (Handle_p->IndexNbOfEntInFile >= Handle_p->MaxIndexEntries))
        {
            Handle_p->StreamInfo_p->FirstIndexPos = Handle_p->IndexPosAbsolute;

            /* update INFO file since change in FirstIndexPos */
            STDVMi_UpdateStreamInfoToDisk(Handle_p);
        }
    }

#ifndef OPEN_CLOSE_FILE_ON_WRITE
    vfs_close(Handle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    INDEX_ACCESS_MUTEX_DELETE(Handle_p);

    DVMINDEX_Trace(("\tSTDVMi_RecordCallbackStop(%08X):\n\n", PRM_Handle));

    return ErrorCode;
}


/*******************************************************************************
Name         :  STDVMi_RecordCallbackGetTime()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t  STDVMi_RecordCallbackGetTime(STPRM_Handle_t PRM_Handle,
                                             U32           *StartTimeInMs,
                                             U32           *EndTimeInMs)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STDVMi_Handle_t    *Handle_p;


    DVMINDEX_Trace(("STDVMi_RecordCallbackGetTime(%08X):\n", PRM_Handle));

    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL)
    {
        DVMINDEX_Error(("RCBGetTime(%08X) Get STDVMi_Handle failed\n", PRM_Handle));
        return ST_ERROR_BAD_PARAMETER;
    }

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    *EndTimeInMs     = Handle_p->StreamMaxTime;
    *StartTimeInMs   = 0;

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    ErrorCode = STDVMi_GetStreamTime(Handle_p, Handle_p->StreamInfo_p->StreamStartPos, StartTimeInMs);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMINDEX_Error(("RCBGetTime(%08X) STDVMi_GetStreamTime()=%08X\n", Handle_p, ErrorCode));
        return ErrorCode;
    }

    DVMINDEX_Trace(("\tSTDVMi_RecordCallbackGetTime(%08X) TimeMS{S[%u], E[%u]}\n\n", PRM_Handle, *StartTimeInMs, *EndTimeInMs));

    return ErrorCode;
}


/*******************************************************************************
Name         : STDVMi_PlayCallbackStart()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_PlayCallbackStart(STPRM_Handle_t PRM_Handle)
{
    ST_ErrorCode_t      ErrorCode;
    STDVMi_Handle_t    *Handle_p;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    S32                 BytesRead;
    U32                 IndexEntries2Read;
    S32                 RetVal;
    STPRM_PlayParams_t  PRMPlayParams;


    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL || Handle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        /* It's a live decoding, no file context found */
        return ST_NO_ERROR;
    }

    DVMINDEX_Trace(("STDVMi_PlayCallbackStart(%08X):\n", Handle_p));

    INDEX_ACCESS_MUTEX_CREATE(Handle_p);

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  IdxStatus,
                    BakStatus;
        BOOL        IdxExists = TRUE,
                    BakExists = TRUE;


        GET_INDEX_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &IdxStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("PCBStart(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            IdxStatus.st_size = 0;
            IdxExists = FALSE;
        }

        GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("PCBStart(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            BakStatus.st_size = 0;
            BakExists = FALSE;
        }

        if((IdxExists == FALSE) && (BakExists == FALSE))
        {
            DVMINDEX_Error(("PCBStart(%08X) both Index and Backup file does not exist\n", Handle_p));
            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(IdxStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        }
        else
        {
            GET_INDEX_FILE_NAME(LocalName, Handle_p);
        }
    }
#else
    GET_INDEX_FILE_NAME(LocalName, Handle_p);
#endif /* USE_BACKUP_FILES */

    Handle_p->IndexFileHandle = vfs_open(LocalName, Handle_p->OpenFlags);
    if(Handle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("PCBStart(%08X) file open(%s) failed\n", Handle_p, LocalName));
        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* seek to find the no of entries in file */
    vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);

    Handle_p->IndexNbOfEntInFile = vfs_ftell(Handle_p->IndexFileHandle) / sizeof(STDVMi_IndexInfo_t);

    /* Take MaxTime and MaxPos if a recording is still active */
    if(Handle_p->SameFileHandle_p != NULL)
    {
        SHARED_FIELD_ACCESS_MUTEX_LOCK(Handle_p);

        Handle_p->StreamMinTime = 0;
        Handle_p->StreamMaxTime = Handle_p->SameFileHandle_p->StreamMaxTime;
        Handle_p->StreamMaxPos  = Handle_p->SameFileHandle_p->StreamMaxPos;

        SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);
    }

    if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) || (Handle_p->StreamInfo_p->FirstIndexPos == 0))
    {
        if(Handle_p->IndexNbOfEntInFile != 0)   /* data in the file */
        {
            /* Take MaxTime and MaxPos from file if recording is not active */
            if(Handle_p->SameFileHandle_p == NULL)
            {
                /* if End position of the stream is edited then last index entry is no more valid */
                if(I64_IsZero(Handle_p->StreamInfo_p->StreamEndPosAfterCrop))
                {
                    /* seek to read the last Index record */
                    RetVal = vfs_fseek(Handle_p->IndexFileHandle, -sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_END);
                    if(RetVal != OSPLUS_SUCCESS)
                    {
                        DVMINDEX_Error(("PCBStart(%08X) file seek(%d) failed\n", Handle_p, -sizeof(STDVMi_IndexInfo_t)));
                        vfs_close(Handle_p->IndexFileHandle);
                        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                        return ST_ERROR_NO_MEMORY;
                    }

                    /* Read last index data in buffer */
                    BytesRead = vfs_read(Handle_p->IndexFileHandle, Handle_p->IndexInfo_p, sizeof(STDVMi_IndexInfo_t));
                    if (BytesRead != sizeof(STDVMi_IndexInfo_t))
                    {
                        DVMINDEX_Error(("PCBStart(%08X) file read(%d)=%d failed\n", Handle_p, sizeof(STDVMi_IndexInfo_t), BytesRead));
                        vfs_close(Handle_p->IndexFileHandle);
                        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                        return ST_ERROR_NO_MEMORY;
                    }

                    Handle_p->StreamMinTime = 0;
                    Handle_p->StreamMaxTime = Handle_p->IndexInfo_p[0].PacketTimeInMs;
                    Handle_p->StreamMaxPos  = Handle_p->IndexInfo_p[0].PacketPosition;
                }
                else
                {
                    Handle_p->StreamMinTime = 0;

                    ErrorCode = STDVMi_GetStreamTime(Handle_p,
                                                     Handle_p->StreamInfo_p->StreamEndPosAfterCrop,
                                                     &Handle_p->StreamMaxTime);
                    if(ErrorCode != ST_NO_ERROR)
                    {
                        DVMINDEX_Error(("PCBStart(%08X) STDVMi_GetStreamTime()=%08X\n", Handle_p, ErrorCode));
                        return ErrorCode;
                    }

                    I64_DivLit(Handle_p->StreamInfo_p->StreamEndPosAfterCrop, STPRM_PACKET_SIZE, Handle_p->StreamMaxPos);
                }
            }

            vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_SET);

            /* Read index data in buffer */
            BytesRead = vfs_read(Handle_p->IndexFileHandle, Handle_p->IndexInfo_p,
                                    STDVMi_INDEX_INFO_CACHE_SIZE*sizeof(STDVMi_IndexInfo_t));
            if(BytesRead <= 0)
            {
                DVMINDEX_Error(("PCBStart(%08X) file read(%d)=%d failed\n", Handle_p,
                        STDVMi_INDEX_INFO_CACHE_SIZE*sizeof(STDVMi_IndexInfo_t), BytesRead));
                vfs_close(Handle_p->IndexFileHandle);
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_NO_MEMORY;
            }

            Handle_p->IndexNbOfEntInCache  = BytesRead/sizeof(STDVMi_IndexInfo_t);
        }
        else
        {
            if(Handle_p->SameFileHandle_p != NULL)
            {
                SHARED_FIELD_ACCESS_MUTEX_LOCK(Handle_p);

                if(Handle_p->SameFileHandle_p->IndexNbOfEntInCache == 0)
                {
                    /* No Index data in File and Buffer */
                    SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);

                    DVMINDEX_Error(("PCBStart(%08X) No Index entries in File and Buffer\n", Handle_p));
                    vfs_close(Handle_p->IndexFileHandle);
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_BAD_PARAMETER;
                }

                Handle_p->IndexNbOfEntInFile  = 0;
                Handle_p->IndexNbOfEntInCache = Handle_p->SameFileHandle_p->IndexNbOfEntInCache;

                memcpy(Handle_p->IndexInfo_p,
                       Handle_p->SameFileHandle_p->IndexInfo_p,
                       sizeof(STDVMi_IndexInfo_t) * Handle_p->IndexNbOfEntInCache);

                SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);
            }
            else
            {
                /* No Index data in File and Buffer */
                DVMINDEX_Error(("PCBStart(%08X) No Index entries in File and Buffer\n", Handle_p));
                vfs_close(Handle_p->IndexFileHandle);
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }
        }
    }
    else
    {
        if(Handle_p->IndexNbOfEntInFile != 0)   /* data in the file */
        {
            /* Take MaxTime and MaxPos from file if recording is not active */
            if(Handle_p->SameFileHandle_p == NULL)
            {
                if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR) ||
                   (Handle_p->IndexNbOfEntInFile == Handle_p->StreamInfo_p->CircularPartIndexEntries)) /*no entries in linear part*/
                {
                    /* seek to read the last Index record */
                    RetVal = vfs_fseek(Handle_p->IndexFileHandle,
                                       (Handle_p->StreamInfo_p->FirstIndexPos - 1) * sizeof(STDVMi_IndexInfo_t),
                                       OSPLUS_SEEK_SET);
                    if(RetVal != OSPLUS_SUCCESS)
                    {
                        DVMINDEX_Error(("PCBStart(%08X) file seek(%d) failed\n", Handle_p,
                                (Handle_p->StreamInfo_p->FirstIndexPos - 1) * sizeof(STDVMi_IndexInfo_t)));
                        vfs_close(Handle_p->IndexFileHandle);
                        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                        return ST_ERROR_NO_MEMORY;
                    }
                }
                else /* circular_linear, with entries in linear part */
                {
                    /* seek to read the last Index record */
                    RetVal = vfs_fseek(Handle_p->IndexFileHandle, -sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_END);
                    if(RetVal != OSPLUS_SUCCESS)
                    {
                        DVMINDEX_Error(("PCBStart(%08X) file seek(%d) failed\n", Handle_p, -sizeof(STDVMi_IndexInfo_t)));
                        vfs_close(Handle_p->IndexFileHandle);
                        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                        return ST_ERROR_NO_MEMORY;
                    }
                }

                /* Read last index data in buffer */
                BytesRead = vfs_read(Handle_p->IndexFileHandle, Handle_p->IndexInfo_p, sizeof(STDVMi_IndexInfo_t));
                if (BytesRead != sizeof(STDVMi_IndexInfo_t))
                {
                    DVMINDEX_Error(("PCBStart(%08X) file read(%d)=%d failed\n", Handle_p, sizeof(STDVMi_IndexInfo_t), BytesRead));
                    vfs_close(Handle_p->IndexFileHandle);
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_NO_MEMORY;
                }

                Handle_p->StreamMinTime = 0;
                Handle_p->StreamMaxTime = Handle_p->IndexInfo_p[0].PacketTimeInMs;
                Handle_p->StreamMaxPos  = Handle_p->IndexInfo_p[0].PacketPosition;
            }

            vfs_fseek(Handle_p->IndexFileHandle,
                      Handle_p->StreamInfo_p->FirstIndexPos * sizeof(STDVMi_IndexInfo_t),
                      OSPLUS_SEEK_SET);

            IndexEntries2Read = STDVMi_INDEX_INFO_CACHE_SIZE;

            /* index info read into cache must be in ascending order */
            if((Handle_p->StreamInfo_p->CircularPartIndexEntries - Handle_p->StreamInfo_p->FirstIndexPos) < IndexEntries2Read)
            {
                IndexEntries2Read = Handle_p->StreamInfo_p->CircularPartIndexEntries - Handle_p->StreamInfo_p->FirstIndexPos;
            }

            /* Read index data in buffer */
            BytesRead = vfs_read(Handle_p->IndexFileHandle, Handle_p->IndexInfo_p, IndexEntries2Read * sizeof(STDVMi_IndexInfo_t));
            if(BytesRead <= 0)
            {
                DVMINDEX_Error(("PCBStart(%08X) file read(%d)=%d failed\n", Handle_p,
                        IndexEntries2Read * sizeof(STDVMi_IndexInfo_t), BytesRead));
                vfs_close(Handle_p->IndexFileHandle);
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_NO_MEMORY;
            }

            Handle_p->IndexNbOfEntInCache  = BytesRead/sizeof(STDVMi_IndexInfo_t);
        }
        else
        {
            DVMINDEX_Error(("PCBStart(%08X) No Index entries in Circular File\n", Handle_p));
            vfs_close(Handle_p->IndexFileHandle);
            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    Handle_p->IndexPosAbsolute  = 0;
    Handle_p->IndexPosInCache   = 0;

    Handle_p->FirstJump         = TRUE;

    I64_SetValue(0, 0, Handle_p->ElapsedNumPackets);
    Handle_p->ElapsedTimeInMs = 0;

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

#ifndef ST_OSLINUX  /* mutex not supported properly in STLinux */
    ErrorCode = STPRM_PlayGetParams(PRM_Handle, &PRMPlayParams);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMINDEX_Error(("PCBStart(%08X) STPRM_PlayGetParams(%08X) failed\n", Handle_p, PRM_Handle));
        vfs_close(Handle_p->IndexFileHandle);
        return ST_ERROR_BAD_PARAMETER;
    }

    osplus_uncached_register(PRMPlayParams.TSBuffer, PRMPlayParams.TSBufferSize);

    /* Store TS Buffer limits used for Encrypt/Decrypt */
    Handle_p->TSBufferStart = (void *)(PRMPlayParams.TSBuffer);
    Handle_p->TSBufferEnd   = (void *)(PRMPlayParams.TSBuffer+PRMPlayParams.TSBufferSize-1);
#else
    UNUSED_PARAMETER(PRMPlayParams);
#endif

    DVMINDEX_Trace(("\tSTDVMi_PlayCallbackStart(%08X) NbEnt{C[%d], F[%d]} MaxTime[%u] MaxPos[%u,%u]\n\n", Handle_p,
            Handle_p->IndexNbOfEntInCache, Handle_p->IndexNbOfEntInFile,
            Handle_p->StreamMaxTime, Handle_p->StreamMaxPos.MSW, Handle_p->StreamMaxPos.LSW));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         :  STDVMi_PlayCallbackJump()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_PlayCallbackJump(STPRM_Handle_t PRM_Handle, U32 PacketTimeInMs, S32 JumpInMs, S64 *NewPacketPosition_p)
{
    STDVMi_Handle_t    *Handle_p;
    U32                 JumpPacketTime;
    U32                 Index;
    U32                 IndexEnd;
    S32                 BytesRead;
    S32                 SeekPos = 0;
    S32                 CurStartPos;
    U32                 IndexEntries2Read = 0;
    S32                 RetVal;
    S64                 StreamStartPos;
    BOOL                ReachedStartOfFile = FALSE,
                        ReachedEndOfFile   = FALSE;
    U8                  CheckIndexCache    = 0;
    U32                 IndexFromFile,
                        IndexFromCache;
    BOOL                FirstSeek = TRUE;


    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL || Handle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        Handle_p->FirstJump = FALSE;
        /* It's a live decoding, no file context found */
        return ST_NO_ERROR;
    }

    DVMINDEX_Trace(("STDVMi_PlayCallbackJump(%08X) PacketTimeInMs[%u] JumpInMs[%d]\n", Handle_p, PacketTimeInMs, JumpInMs));

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    JumpPacketTime = PacketTimeInMs + JumpInMs;

    /* jump to the start of the stream */
    if(Handle_p->FirstJump==TRUE)
    {
        Handle_p->FirstJump = FALSE;
        if(JumpPacketTime == 0)
        {
            if(!I64_IsZero(Handle_p->StreamInfo_p->StreamStartPosAfterCrop))    /* stream cropped */
            {
                I64_DivLit(Handle_p->StreamInfo_p->StreamStartPosAfterCrop, STPRM_PACKET_SIZE, *NewPacketPosition_p);
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                DVMINDEX_Trace(("\tSTDVMi_PlayCallbackJump(%08X) NewPos[%u,%u]\n\n", Handle_p,
                        NewPacketPosition_p->MSW, NewPacketPosition_p->LSW));
                return ST_NO_ERROR;
            }
            else if(!I64_IsZero(Handle_p->StreamInfo_p->StreamStartPos))        /* circular file wrapped around */
            {
                I64_DivLit(Handle_p->StreamInfo_p->StreamStartPos, STPRM_PACKET_SIZE, *NewPacketPosition_p);
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                DVMINDEX_Trace(("\tSTDVMi_PlayCallbackJump(%08X) NewPos[%u,%u]\n\n", Handle_p,
                        NewPacketPosition_p->MSW, NewPacketPosition_p->LSW));
                return ST_NO_ERROR;
            }


        }
    }

    /* check if the Jump requested in out of range
     * TODO: StreamMinTime to be calculated in circular file cases
     */
    if ((JumpPacketTime < Handle_p->StreamMinTime) || (JumpPacketTime > Handle_p->StreamMaxTime))
    {
        DVMINDEX_Error(("PCBJump(%08X) JumpPacketTime[%u] exceeds Limits{S[%u], E[%u]}\n", Handle_p,
                JumpPacketTime, Handle_p->StreamMinTime, Handle_p->StreamMaxTime));
        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    while(TRUE)
    {
        DVMINDEX_Trace(("\tSTDVMi_PlayCallbackJump(%08X) Time{0[%u] %d[%u]}\n", Handle_p,
                Handle_p->IndexInfo_p[0].PacketTimeInMs,
                Handle_p->IndexNbOfEntInCache-1,
                Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache-1].PacketTimeInMs));

        if(JumpPacketTime >= Handle_p->IndexInfo_p[0].PacketTimeInMs &&
           JumpPacketTime <= Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache-1].PacketTimeInMs)
        {
            /* splitting the search buffer into 2 with IndexPosInCache */
            if((Handle_p->IndexPosInCache != 0) && (Handle_p->IndexPosInCache != Handle_p->IndexNbOfEntInCache))
            {
                if(JumpPacketTime <= Handle_p->IndexInfo_p[Handle_p->IndexPosInCache-1].PacketTimeInMs)
                {
                    Index       = 0;
                    IndexEnd    = Handle_p->IndexPosInCache;
                }
                else
                {
                    Index       = Handle_p->IndexPosInCache;
                    IndexEnd    = Handle_p->IndexNbOfEntInCache;
                }
            }
            else
            {
                Index       = 0;
                IndexEnd    = Handle_p->IndexNbOfEntInCache;
            }

            for(; Index < IndexEnd; Index++)
            {
                if(JumpPacketTime <= Handle_p->IndexInfo_p[Index].PacketTimeInMs)
                {
                    Handle_p->IndexPosAbsolute  = Handle_p->IndexPosAbsolute - Handle_p->IndexPosInCache + Index;
                    Handle_p->IndexPosInCache   = Index;

                    if(JumpPacketTime == Handle_p->IndexInfo_p[Index].PacketTimeInMs)
                    {
                        *NewPacketPosition_p = Handle_p->IndexInfo_p[Index].PacketPosition;
                    }
                    else
                    {
                        /* Interpolating packet position */
                        float               Ratio = 0;
                        S64                 Proportion = {0, 0};

                        Ratio = (float)((float)JumpPacketTime - Handle_p->IndexInfo_p[Index - 1].PacketTimeInMs) /
                                       ((float)Handle_p->IndexInfo_p[Index].PacketTimeInMs -
                                               Handle_p->IndexInfo_p[Index - 1].PacketTimeInMs);
                        I64_Sub(Handle_p->IndexInfo_p[Index].PacketPosition,
                                Handle_p->IndexInfo_p[Index - 1].PacketPosition,
                                Proportion);
                        Proportion.LSW = (U32)((float)Proportion.LSW * Ratio);
                        I64_Add(Handle_p->IndexInfo_p[Index - 1].PacketPosition, Proportion, *NewPacketPosition_p);
                    }

                    /* check if the calulated PacketPosition is within range */
                    if(I64_IsZero(Handle_p->StreamInfo_p->StreamStartPosAfterCrop))
                    {
                        I64_DivLit(Handle_p->StreamInfo_p->StreamStartPos, STPRM_PACKET_SIZE, StreamStartPos);
                    }
                    else
                    {
                        I64_DivLit(Handle_p->StreamInfo_p->StreamStartPosAfterCrop, STPRM_PACKET_SIZE, StreamStartPos);
                    }
                    /* TODO: strange without the below condition Jump fails at end of stream need to debug */
                    if(!I64_IsZero(StreamStartPos))
                    {
                        /* check if the Packetposition found is within the available range */
                        if (I64_IsLessThan(*NewPacketPosition_p, StreamStartPos) ||
                            I64_IsGreaterThan(*NewPacketPosition_p, Handle_p->StreamMaxPos))
                        {
                            DVMINDEX_Error(("PCBJump(%08X) PositionS[%u,%u] exceeds Limits{S[%u,%u], E[%u,%u]}\n", Handle_p,
                                    NewPacketPosition_p->MSW, NewPacketPosition_p->LSW,
                                    StreamStartPos.MSW, StreamStartPos.LSW,
                                    Handle_p->StreamMaxPos.MSW, Handle_p->StreamMaxPos.LSW));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }

                    /* More than 1 program is stored in the file correct till next program start position */
                    if(Handle_p->StreamInfo_p->NbOfPrograms > 1)
                    {
                        /* First calculate what will be the current program number after jump */
                        /* Forward jump */
                        if((Handle_p->CurrentProgNo < (Handle_p->StreamInfo_p->NbOfPrograms - 1)) &&
                           (I64_IsGreaterOrEqual(*NewPacketPosition_p,Handle_p->StreamInfo_p->ProgramInfo->ProgEndPos)))
                        {
                            while(Handle_p->CurrentProgNo < Handle_p->StreamInfo_p->NbOfPrograms)
                            {
                                Handle_p->CurrentProgNo++;
                                STDVMi_ReadProgramInfo(Handle_p);
                                if(I64_IsLessThan(*NewPacketPosition_p, Handle_p->StreamInfo_p->ProgramInfo->ProgEndPos))
                                {
                                    break;
                                }
                            }
                            STDVMi_NotifyPidChange(Handle_p);
                         }
                        /* Reverse jump */
                        else if((Handle_p->CurrentProgNo > 0) &&
                                (I64_IsLessThan(*NewPacketPosition_p,Handle_p->StreamInfo_p->ProgramInfo->ProgStartPos)))
                        {
                            while(Handle_p->CurrentProgNo > 0)
                            {
                                Handle_p->CurrentProgNo--;
                                STDVMi_ReadProgramInfo(Handle_p);
                                if(I64_IsGreaterOrEqual(*NewPacketPosition_p, Handle_p->StreamInfo_p->ProgramInfo->ProgStartPos))
                                {
                                    break;
                                }
                            }
                            STDVMi_NotifyPidChange(Handle_p);
                        }
                    }

                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

                    DVMINDEX_Trace(("\tSTDVMi_PlayCallbackJump(%08X) NewPos[%u,%u]\n\n", Handle_p,
                            NewPacketPosition_p->MSW, NewPacketPosition_p->LSW));

                    return ST_NO_ERROR;
                }
            }
        }
        else
        {
            /* Index info in File and the Other file index buffer are all checked */
            if(CheckIndexCache >= 2)
            {
                DVMINDEX_Error(("PCBJump(%08X) Could not locate JumpTime in File and Buffer\n", Handle_p));
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }

            DVMINDEX_Trace(("\tSTDVMi_PlayCallbackJump(%08X) IPos{%C[%u] A[%u]} EntInFile[%d]\n", Handle_p,
                    Handle_p->IndexPosInCache, Handle_p->IndexPosAbsolute, Handle_p->IndexNbOfEntInFile));

            if((Handle_p->IndexNbOfEntInFile == 0) || (CheckIndexCache > 0))   /* no index entries in file */
            {
                CheckIndexCache++;
            }
            else
            {
                /* getting the current position of in index file */
                CurStartPos = Handle_p->IndexPosAbsolute - Handle_p->IndexPosInCache;

                IndexEntries2Read = STDVMi_INDEX_INFO_CACHE_SIZE;

                if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
                   (Handle_p->StreamInfo_p->FirstIndexPos == 0))
                {
                    if(JumpPacketTime < Handle_p->IndexInfo_p[0].PacketTimeInMs)
                    {
                        if(ReachedStartOfFile == FALSE)
                        {
                            if(FirstSeek == TRUE)
                            {
                                SeekPos = (((Handle_p->IndexInfo_p[0].PacketTimeInMs - JumpPacketTime) /
                                                STDVMi_INDEX_TIME_PRECISION_MS)
                                                + STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                                FirstSeek = FALSE;
                            }
                            else
                            {
                                SeekPos = STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET;
                            }

                            SeekPos = CurStartPos - SeekPos;
                            if(SeekPos <= 0)
                            {
                                SeekPos = 0;
                                ReachedStartOfFile = TRUE;
                            }
                        }
                        else    /* Requesting Index less than 0 */
                        {
                            DVMINDEX_Error(("PCBJump(%08X) Requesting Time[%u] < I[0].Time[%u]\n", Handle_p,
                                    JumpPacketTime,  Handle_p->IndexInfo_p[0].PacketTimeInMs));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    else /* need to go forward */
                    {
                        if(ReachedEndOfFile == FALSE)
                        {
                            if(FirstSeek == TRUE)
                            {
                                SeekPos = (((JumpPacketTime - Handle_p->IndexInfo_p[0].PacketTimeInMs) /
                                                STDVMi_INDEX_TIME_PRECISION_MS) - INDEX_FILE_LOOKUP_OFFSET);
                                FirstSeek = FALSE;
                            }
                            else
                            {
                                SeekPos = STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET;
                            }

                            SeekPos = CurStartPos + SeekPos;
                            if(SeekPos >= Handle_p->IndexNbOfEntInFile)
                            {
                                SeekPos = (Handle_p->IndexNbOfEntInFile > INDEX_FILE_LOOKUP_OFFSET) ?
                                            (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                ReachedEndOfFile = TRUE;
                            }
                        }
                        else    /* Requesting Index greater than total entries in file */
                        {
                            CheckIndexCache++;
                        }
                    }
                }
                else if(Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
                {
                    if(JumpPacketTime < Handle_p->IndexInfo_p[0].PacketTimeInMs)
                    {
                        if(ReachedStartOfFile == FALSE)
                        {
                            if(FirstSeek == TRUE)
                            {
                                SeekPos = (((Handle_p->IndexInfo_p[0].PacketTimeInMs - JumpPacketTime) /
                                                STDVMi_INDEX_TIME_PRECISION_MS)
                                                + STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                                FirstSeek = FALSE;
                            }
                            else
                            {
                                SeekPos = STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET;
                            }

                            SeekPos = CurStartPos - SeekPos;
                            if(CurStartPos < Handle_p->StreamInfo_p->FirstIndexPos)
                            {
                                if(CurStartPos <= 0)
                                    SeekPos = (Handle_p->IndexNbOfEntInFile > INDEX_FILE_LOOKUP_OFFSET) ?
                                                (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                else if(SeekPos < 0)
                                    SeekPos = 0;
                            }
                            else
                            {
                                if(SeekPos <= Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    SeekPos = Handle_p->StreamInfo_p->FirstIndexPos;
                                    ReachedStartOfFile = TRUE;
                                }
                            }

                            /* index info read into cache must be in ascending order */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }
                        }
                        else    /* Requesting Index less than 0 */
                        {
                            DVMINDEX_Error(("PCBJump(%08X) Requesting Time[%u] < I[0].Time[%u]\n", Handle_p,
                                    JumpPacketTime,  Handle_p->IndexInfo_p[0].PacketTimeInMs));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    else
                    {
                        if(ReachedEndOfFile == FALSE)
                        {
                            if(FirstSeek == TRUE)
                            {
                                SeekPos = (((JumpPacketTime - Handle_p->IndexInfo_p[0].PacketTimeInMs) /
                                                STDVMi_INDEX_TIME_PRECISION_MS) - INDEX_FILE_LOOKUP_OFFSET);
                                FirstSeek = FALSE;
                            }
                            else
                            {
                                SeekPos = STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET;
                            }

                            SeekPos = CurStartPos + SeekPos;
                            if(CurStartPos > Handle_p->StreamInfo_p->FirstIndexPos)
                            {
                                if(CurStartPos >= (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET))
                                    SeekPos = 0;
                                else if(SeekPos > (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET))
                                    SeekPos = Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET;
                            }
                            else
                            {
                                if(SeekPos >= Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    SeekPos = (Handle_p->StreamInfo_p->FirstIndexPos > INDEX_FILE_LOOKUP_OFFSET) ?
                                                (Handle_p->StreamInfo_p->FirstIndexPos - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                    ReachedEndOfFile = TRUE;
                                }
                            }

                            /* index info read into cache must be in ascending order */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }
                        }
                        else
                        {
                            CheckIndexCache++;
                        }
                    }
                }
                else /* circular_linear */
                {
                    if(JumpPacketTime < Handle_p->IndexInfo_p[0].PacketTimeInMs)
                    {
                        if(ReachedStartOfFile == FALSE)
                        {
                            if(FirstSeek == TRUE)
                            {
                                SeekPos = (((Handle_p->IndexInfo_p[0].PacketTimeInMs - JumpPacketTime) /
                                                STDVMi_INDEX_TIME_PRECISION_MS)
                                                + STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                                FirstSeek = FALSE;
                            }
                            else
                            {
                                SeekPos = STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET;
                            }

                            SeekPos = CurStartPos - SeekPos;
                            if(CurStartPos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
                            {
                                if(CurStartPos < Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    if(CurStartPos <= 0)
                                        SeekPos = (Handle_p->StreamInfo_p->CircularPartIndexEntries > INDEX_FILE_LOOKUP_OFFSET) ?
                                                  (Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                    else if(SeekPos < 0)
                                        SeekPos = 0;
                                }
                                else
                                {
                                    if(SeekPos <= Handle_p->StreamInfo_p->FirstIndexPos)
                                    {
                                        SeekPos = Handle_p->StreamInfo_p->FirstIndexPos;
                                        ReachedStartOfFile = TRUE;
                                    }
                                }
                            }
                            else
                            {
                                if(SeekPos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
                                {
                                    if(CurStartPos == Handle_p->StreamInfo_p->CircularPartIndexEntries)
                                    {
                                        SeekPos = (Handle_p->StreamInfo_p->FirstIndexPos > INDEX_FILE_LOOKUP_OFFSET) ?
                                                    (Handle_p->StreamInfo_p->FirstIndexPos - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                    }
                                    else
                                    {
                                        SeekPos = Handle_p->StreamInfo_p->CircularPartIndexEntries;
                                    }
                                }
                            }

                            /* index info read into cache must be in ascending order, circular end */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }

                            /* index info read into cache must be in ascending order, circular part end */
                            if((SeekPos < Handle_p->StreamInfo_p->CircularPartIndexEntries) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->CircularPartIndexEntries))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->CircularPartIndexEntries - SeekPos;
                            }
                        }
                        else    /* Requesting Index less than 0 */
                        {
                            DVMINDEX_Error(("PCBJump(%08X) Requesting Time[%u] < I[0].Time[%u]\n", Handle_p,
                                    JumpPacketTime,  Handle_p->IndexInfo_p[0].PacketTimeInMs));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    else
                    {
                        if(ReachedEndOfFile == FALSE)
                        {
                            if(FirstSeek == TRUE)
                            {
                                SeekPos = (((JumpPacketTime - Handle_p->IndexInfo_p[0].PacketTimeInMs) /
                                                STDVMi_INDEX_TIME_PRECISION_MS) - INDEX_FILE_LOOKUP_OFFSET);
                                FirstSeek = FALSE;
                            }
                            else
                            {
                                SeekPos = STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET;
                            }

                            SeekPos = CurStartPos + SeekPos;
                            if(CurStartPos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
                            {
                                if(CurStartPos > Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    if(CurStartPos >= (Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET))
                                        SeekPos = 0;
                                    else if(SeekPos > (Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET))
                                        SeekPos = Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET;
                                }
                                else
                                {
                                    if(SeekPos >= Handle_p->StreamInfo_p->FirstIndexPos)
                                    {
                                        SeekPos = Handle_p->StreamInfo_p->CircularPartIndexEntries;
                                    }
                                }
                            }
                            else
                            {
                                if(SeekPos >= Handle_p->IndexNbOfEntInFile)
                                {
                                    SeekPos = ((Handle_p->IndexNbOfEntInFile - Handle_p->StreamInfo_p->CircularPartIndexEntries) >
                                                                               INDEX_FILE_LOOKUP_OFFSET) ?
                                              (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET) :
                                              Handle_p->StreamInfo_p->CircularPartIndexEntries;
                                    ReachedEndOfFile = TRUE;
                                }
                            }

                            /* index info read into cache must be in ascending order, circular end */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }

                            /* index info read into cache must be in ascending order, circular part end */
                            if((SeekPos < Handle_p->StreamInfo_p->CircularPartIndexEntries) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->CircularPartIndexEntries))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->CircularPartIndexEntries - SeekPos;
                            }
                        }
                        else
                        {
                            CheckIndexCache++;
                        }
                    }
                }
            }

            if(CheckIndexCache == 0)
            {
                DVMINDEX_Trace(("\tSTDVMi_PlayCallbackJump(%08X) SeekPos[%u] StartPos[%d]\n", Handle_p,
                        SeekPos, Handle_p->StreamInfo_p->FirstIndexPos));

                RetVal  = vfs_fseek(Handle_p->IndexFileHandle, SeekPos * sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_SET);
                if(RetVal != OSPLUS_SUCCESS)
                {
                    DVMINDEX_Error(("PCBJump(%08X) file seek(%d) failed\n", Handle_p,
                            SeekPos * sizeof(STDVMi_IndexInfo_t)));
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_BAD_PARAMETER;
                }

                /* TODO: optimize reads to be done for only the new data required &
                 *       do mutiple reads in case file wrap around to fill index cache
                 */

                /* Read index data in buffer */
                BytesRead = vfs_read(Handle_p->IndexFileHandle,
                                     Handle_p->IndexInfo_p,
                                     IndexEntries2Read * sizeof(STDVMi_IndexInfo_t));
                if(BytesRead <= 0) /* TODO: handle BytesRead = 0 */
                {
                    DVMINDEX_Error(("PCBJump(%08X) file read(%d)=%d SeekPos[%d] EntInFile[%d] failed\n", Handle_p,
                            IndexEntries2Read * sizeof(STDVMi_IndexInfo_t), BytesRead,
                            SeekPos, Handle_p->IndexNbOfEntInFile));
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_NO_MEMORY;
                }

                Handle_p->IndexPosAbsolute  = SeekPos;
                Handle_p->IndexPosInCache   = 0;

                Handle_p->IndexNbOfEntInCache  = BytesRead/sizeof(STDVMi_IndexInfo_t);
            }
            else
            {
                /* checking the other file handles IndexInfo_p  */
                if(Handle_p->SameFileHandle_p != NULL)
                {
                    SHARED_FIELD_ACCESS_MUTEX_LOCK(Handle_p);

                    if(Handle_p->SameFileHandle_p->IndexNbOfEntInCache == 0)
                    {
                        /* No Index data in File and Buffer */
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);

                        DVMINDEX_Error(("PCBJump(%08X) No Index entries in File and Buffer\n", Handle_p));
                        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                        return ST_ERROR_BAD_PARAMETER;
                    }

                    IndexFromCache = Handle_p->SameFileHandle_p->IndexNbOfEntInCache;

                    /* when refering from Cache take the last index entry of File */
                    if((CheckIndexCache == 1) && (Handle_p->IndexNbOfEntInFile > 0) && (Handle_p->IndexNbOfEntInCache > 0))
                    {
                        IndexFromFile = (Handle_p->IndexNbOfEntInCache > INDEX_FILE_LOOKUP_OFFSET) ?
                                                INDEX_FILE_LOOKUP_OFFSET : Handle_p->IndexNbOfEntInCache;
                        memcpy(Handle_p->IndexInfo_p,
                               Handle_p->IndexInfo_p + (Handle_p->IndexNbOfEntInCache-IndexFromFile),
                               IndexFromFile);

                        if((IndexFromFile + IndexFromCache) > IndexEntries2Read)
                        {
                            IndexFromCache -= IndexFromFile;
                        }
                    }
                    else
                    {
                        IndexFromFile = 0;
                        CheckIndexCache++;
                    }

                    Handle_p->IndexPosAbsolute  = Handle_p->IndexNbOfEntInFile - IndexFromFile;
                    Handle_p->IndexPosInCache   = 0;

                    memcpy(Handle_p->IndexInfo_p + IndexFromFile,
                           Handle_p->SameFileHandle_p->IndexInfo_p,
                           sizeof(STDVMi_IndexInfo_t) * IndexFromCache);

                    Handle_p->IndexNbOfEntInCache = IndexFromFile + IndexFromCache;

                    SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);
                }
                else
                {
                    DVMINDEX_Error(("PCBJump(%08X) No Index entries in File and Buffer\n", Handle_p));
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_BAD_PARAMETER;
                }
            } /*if(CheckIndexBuffer == FALSE)*/
        }
    } /*while(TRUE)*/

    DVMINDEX_Error(("PCBJump(%08X) Could not locate JumpTime\n", Handle_p));
    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    return ST_ERROR_BAD_PARAMETER;
}


/*******************************************************************************
Name         : STDVMi_PlayCallbackPacket()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_PlayCallbackPacket(   STPRM_Handle_t  PRM_Handle,
                                            S64             PacketPosition,
                                            U32            *NbPacketsToInject_p,
                                            U32            *FirstPacketTimeInMs_p,
                                            U32            *TimeDurationInMs_p)
{
    STDVMi_Handle_t    *Handle_p;
    U32                 Index;
    U32                 IndexEnd;
    BOOL                EndPacketTimeFound;
    BOOL                StartPacketTimeFound;
    S32                 BytesRead;
    S32                 SeekPos = 0;
    S32                 CurStartPos;
    U32                 IndexEntries2Read = 0;
    S32                 RetVal;
    S64                 TempPacketPosition;
    S64                 StreamStartPos;
    BOOL                ReachedStartOfFile = FALSE,
                        ReachedEndOfFile   = FALSE;
    U8                  CheckIndexCache    = 0;
    U32                 IndexFromFile,
                        IndexFromCache;


    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL || Handle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        /* It's a live decoding, no file context found */
        return ST_NO_ERROR;
    }

    if(FirstPacketTimeInMs_p == NULL || TimeDurationInMs_p == NULL)
    {
        DVMINDEX_Error(("PCBPacket(%08X) FirstPacketTimeInMs_p[%08X] TimeDurationInMs_p[%08X]\n", Handle_p,
                FirstPacketTimeInMs_p, TimeDurationInMs_p));
        return ST_ERROR_BAD_PARAMETER;
    }

    DVMINDEX_Trace(("STDVMi_PlayCallbackPacket(%08X) PackPos[%u,%u] P2Inj[%u]\n", Handle_p,
            PacketPosition.MSW, PacketPosition.LSW, *NbPacketsToInject_p));

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    /* first calculate END packet position */
    I64_AddLit(PacketPosition, *NbPacketsToInject_p, TempPacketPosition);

    if(I64_IsZero(Handle_p->StreamInfo_p->StreamStartPosAfterCrop))
    {
        I64_DivLit(Handle_p->StreamInfo_p->StreamStartPos, STPRM_PACKET_SIZE, StreamStartPos);
    }
    else
    {
        I64_DivLit(Handle_p->StreamInfo_p->StreamStartPosAfterCrop, STPRM_PACKET_SIZE, StreamStartPos);
    }

    /* check if the Packetposition requested is within the available range */
    if (I64_IsLessThan(PacketPosition, StreamStartPos) ||
        I64_IsGreaterThan(TempPacketPosition, Handle_p->StreamMaxPos))
    {
        /* TODO: If End position is greater tham Max pos then no of packets to inject must be altered
                 Not supported by STPRM at present due to alignment
         */
        DVMINDEX_Error(("PCBPacket(%08X) Position{S[%u,%u], E[%u,%u]} exceeds Limits{S[%u,%u], E[%u,%u]}\n", Handle_p,
                PacketPosition.MSW, PacketPosition.LSW, TempPacketPosition.MSW, TempPacketPosition.LSW,
                StreamStartPos.MSW, StreamStartPos.LSW, Handle_p->StreamMaxPos.MSW, Handle_p->StreamMaxPos.LSW));
        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* more than 1 program is stored in the file */
    if(Handle_p->StreamInfo_p->NbOfPrograms > 1)
    {
        switch(STDVMi_GetChangePidState(Handle_p))
        {
            case PID_CHANGE_PENDING:
                Handle_p->CurrentProgNo++;
                STDVMi_ReadProgramInfo(Handle_p);
                STDVMi_NotifyPidChange(Handle_p);
                /* No break */

            case PID_CHANGE_CHANGING:
                *NbPacketsToInject_p = 0;
                *TimeDurationInMs_p  = 0;
                break;

            case PID_CHANGE_IDLE:
                if(Handle_p->CurrentProgNo < (Handle_p->StreamInfo_p->NbOfPrograms - 1))
                {
                    /* calculate if this pkt is within current PID set */
                    if(I64_IsGreaterOrEqual(TempPacketPosition, Handle_p->StreamInfo_p->ProgramInfo->ProgEndPos))
                    {
                        if(STDVMi_GetChangePidState(Handle_p) == PID_CHANGE_IDLE)
                        {
#if 0   /* TODO: STPRM at present does not work when manipulating Packets to inject, due to alignment reasons */
                            S64     PacketDiff;

                            /* calculate END packet position */
                            TempPacketPosition = Handle_p->StreamInfo_p->ProgramInfo->ProgEndPos;

                            /* calculate how many pkt remaining for this current pid set */
                            I64_Sub(TempPacketPosition, PacketPosition, PacketDiff);

                            *NbPacketsToInject_p = PacketDiff.LSW;
#endif
                            STDVMi_SetChangePidState(Handle_p, PID_CHANGE_PENDING);
                        }
                        else
                        {
                            DVMINDEX_Error(("PCBPacket(%08X) ChangePidState != PID_CHANGE_IDLE\n", Handle_p));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }
                }
                break;

            default:
                break;
        }
    } /* end of NbOfPrograms > 1 */

    /* Reset variables so that corresponding time will be found */
    StartPacketTimeFound = FALSE;
    EndPacketTimeFound   = (*NbPacketsToInject_p != 0) ? FALSE : TRUE;

    /* First find EndPacketTime and StartPacketTime */
    while(EndPacketTimeFound == FALSE || StartPacketTimeFound == FALSE)
    {
        DVMINDEX_Trace(("\tSTDVMi_PlayCallbackPacket(%08X) Pos{0[%u,%u] %d[%u,%u]}\n", Handle_p,
                Handle_p->IndexInfo_p[0].PacketPosition.MSW, Handle_p->IndexInfo_p[0].PacketPosition.LSW,
                Handle_p->IndexNbOfEntInCache-1,
                Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache-1].PacketPosition.MSW,
                Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache-1].PacketPosition.LSW));

        if(I64_IsGreaterOrEqual(TempPacketPosition, Handle_p->IndexInfo_p[0].PacketPosition) &&
           I64_IsLessOrEqual(TempPacketPosition, Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache-1].PacketPosition))
        {
            /* splitting the search buffer into 2 with IndexPosInCache */
            if((Handle_p->IndexPosInCache != 0) && (Handle_p->IndexPosInCache != Handle_p->IndexNbOfEntInCache))
            {
                if(I64_IsLessOrEqual(TempPacketPosition, Handle_p->IndexInfo_p[Handle_p->IndexPosInCache-1].PacketPosition))
                {
                    Index       = 0;
                    IndexEnd    = Handle_p->IndexPosInCache;
                }
                else
                {
                    Index       = Handle_p->IndexPosInCache;
                    IndexEnd    = Handle_p->IndexNbOfEntInCache;
                }
            }
            else
            {
                Index       = 0;
                IndexEnd    = Handle_p->IndexNbOfEntInCache;
            }

            for(; Index < IndexEnd; Index++)
            {
                if(I64_IsLessOrEqual(TempPacketPosition, Handle_p->IndexInfo_p[Index].PacketPosition))
                {
                    if(I64_IsEqual(TempPacketPosition, Handle_p->IndexInfo_p[Index].PacketPosition))
                    {
                        if(EndPacketTimeFound == FALSE)
                        {
                            *TimeDurationInMs_p = Handle_p->IndexInfo_p[Index].PacketTimeInMs;
                        }
                        else
                        {
                            *FirstPacketTimeInMs_p = Handle_p->IndexInfo_p[Index].PacketTimeInMs;
                        }
                    }
                    else
                    {
                        /* Interpolating the PacketTime */
                        S64 DeltaPacket, DeltaPacket1;
                        U32 DeltaTime;

                        I64_Sub(Handle_p->IndexInfo_p[Index].PacketPosition,
                                Handle_p->IndexInfo_p[Index-1].PacketPosition,
                                DeltaPacket);
                        DeltaTime = Handle_p->IndexInfo_p[Index].PacketTimeInMs
                                        - Handle_p->IndexInfo_p[Index-1].PacketTimeInMs;

                        I64_Sub(TempPacketPosition,
                                Handle_p->IndexInfo_p[Index-1].PacketPosition,
                                DeltaPacket1);

                        if(EndPacketTimeFound == FALSE)
                        {
                            *TimeDurationInMs_p = Handle_p->IndexInfo_p[Index-1].PacketTimeInMs
                                                        + ((DeltaPacket1.LSW * DeltaTime) / DeltaPacket.LSW);
                        }
                        else
                        {
                            *FirstPacketTimeInMs_p = Handle_p->IndexInfo_p[Index-1].PacketTimeInMs
                                                        + ((DeltaPacket1.LSW * DeltaTime) / DeltaPacket.LSW);
                        }
                    }

                    Handle_p->IndexPosAbsolute  = Handle_p->IndexPosAbsolute - Handle_p->IndexPosInCache + Index;
                    Handle_p->IndexPosInCache   = Index;

                    if(EndPacketTimeFound == FALSE)
                    {
                        EndPacketTimeFound = TRUE;
                        TempPacketPosition = PacketPosition;

                        /* reset the variables to find start Time */
                        ReachedStartOfFile = FALSE;
                        ReachedEndOfFile   = FALSE;
                        CheckIndexCache    = 0;
                    }
                    else
                    {
                        StartPacketTimeFound = TRUE;
                    }
                    break;
                } /* end pkt position < [Index].pkt position */
            } /* end of for( Index ) */
        } /* end pkt position is > oth pkt position & < (cache -1) th pkt position */
        else
        {
            /* Index info in File and the Other file index buffer are all checked */
            if(CheckIndexCache >= 2)
            {
                DVMINDEX_Error(("PCBPacket(%08X) Could not locate PacketPosition in File and Buffer\n", Handle_p));
                INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                return ST_ERROR_BAD_PARAMETER;
            }

            DVMINDEX_Trace(("\tSTDVMi_PlayCallbackPacket(%08X) IPos{%C[%u] A[%u]} EntInFile[%d]\n", Handle_p,
                    Handle_p->IndexPosInCache, Handle_p->IndexPosAbsolute, Handle_p->IndexNbOfEntInFile));

            if((Handle_p->IndexNbOfEntInFile == 0) || (CheckIndexCache > 0))   /* no index entries in file */
            {
                CheckIndexCache++;
            }
            else
            {
                /* approximately getting the current position of the file */
                CurStartPos = Handle_p->IndexPosAbsolute - Handle_p->IndexPosInCache;

                IndexEntries2Read = STDVMi_INDEX_INFO_CACHE_SIZE;

                if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
                   (Handle_p->StreamInfo_p->FirstIndexPos == 0))
                {
                    if(I64_IsLessThan(TempPacketPosition, Handle_p->IndexInfo_p[0].PacketPosition))
                    {
                        if(ReachedStartOfFile == FALSE)
                        {
                            SeekPos = CurStartPos - (STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                            if(SeekPos < 0)
                            {
                                SeekPos = 0;
                                ReachedStartOfFile = TRUE;
                            }
                        }
                        else    /* Requesting Index less than 0 */
                        {
                            DVMINDEX_Error(("PCBPacket(%08X) Requesting Postion[%u,%u] < I[0].Pos[%u,%u]\n", Handle_p,
                                    TempPacketPosition.MSW, TempPacketPosition.LSW,
                                    Handle_p->IndexInfo_p[0].PacketPosition.MSW, Handle_p->IndexInfo_p[0].PacketPosition.LSW));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    else
                    {
                        if(ReachedEndOfFile == FALSE)
                        {
                            SeekPos = CurStartPos + (STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                            if(SeekPos >= Handle_p->IndexNbOfEntInFile)
                            {
                                SeekPos = (Handle_p->IndexNbOfEntInFile > INDEX_FILE_LOOKUP_OFFSET) ?
                                            (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                ReachedEndOfFile = TRUE;
                            }
                        }
                        else    /* Requesting Index greater than total entries in file */
                        {
                            CheckIndexCache++;
                        }
                    }
                }
                else if(Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
                {
                    if(I64_IsLessThan(TempPacketPosition, Handle_p->IndexInfo_p[0].PacketPosition))
                    {
                        if(ReachedStartOfFile == FALSE)
                        {
                            SeekPos = CurStartPos - (STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                            if(CurStartPos < Handle_p->StreamInfo_p->FirstIndexPos)
                            {
                                if(CurStartPos <= 0)
                                    SeekPos = (Handle_p->IndexNbOfEntInFile > INDEX_FILE_LOOKUP_OFFSET) ?
                                                (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                else if(SeekPos < 0)
                                    SeekPos = 0;
                            }
                            else
                            {
                                if(SeekPos <= Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    SeekPos = Handle_p->StreamInfo_p->FirstIndexPos;
                                    ReachedStartOfFile = TRUE;
                                }
                            }

                            /* index info read into cache must be in ascending order */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }
                        }
                        else    /* Requesting Index less than 0 */
                        {
                            DVMINDEX_Error(("PCBPacket(%08X) Requesting Postion[%u,%u] < I[0].Pos[%u,%u]\n", Handle_p,
                                    SeekPos * sizeof(STDVMi_IndexInfo_t),
                                    TempPacketPosition.MSW, TempPacketPosition.LSW,
                                    Handle_p->IndexInfo_p[0].PacketPosition.MSW, Handle_p->IndexInfo_p[0].PacketPosition.LSW));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    else
                    {
                        if(ReachedEndOfFile == FALSE)
                        {
                            SeekPos = CurStartPos + (STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                            if(CurStartPos > Handle_p->StreamInfo_p->FirstIndexPos)
                            {
                                if(CurStartPos >= (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET))
                                    SeekPos = 0;
                                else if(SeekPos > (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET))
                                    SeekPos = Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET;
                            }
                            else
                            {
                                if(SeekPos >= Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    SeekPos = (Handle_p->StreamInfo_p->FirstIndexPos > INDEX_FILE_LOOKUP_OFFSET) ?
                                                (Handle_p->StreamInfo_p->FirstIndexPos - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                    ReachedEndOfFile = TRUE;
                                }
                            }

                            /* index info read into cache must be in ascending order */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }
                        }
                        else
                        {
                            CheckIndexCache++;
                        }
                    }
                }
                else /* circular_linear */
                {
                    if(I64_IsLessThan(TempPacketPosition, Handle_p->IndexInfo_p[0].PacketPosition))
                    {
                        if(ReachedStartOfFile == FALSE)
                        {
                            SeekPos = CurStartPos - (STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                            if(CurStartPos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
                            {
                                if(CurStartPos < Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    if(CurStartPos <= 0)
                                        SeekPos = (Handle_p->StreamInfo_p->CircularPartIndexEntries > INDEX_FILE_LOOKUP_OFFSET) ?
                                                  (Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                    else if(SeekPos < 0)
                                        SeekPos = 0;
                                }
                                else
                                {
                                    if(SeekPos <= Handle_p->StreamInfo_p->FirstIndexPos)
                                    {
                                        SeekPos = Handle_p->StreamInfo_p->FirstIndexPos;
                                        ReachedStartOfFile = TRUE;
                                    }
                                }
                            }
                            else
                            {
                                if(SeekPos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
                                {
                                    if(CurStartPos == Handle_p->StreamInfo_p->CircularPartIndexEntries)
                                    {
                                        SeekPos = (Handle_p->StreamInfo_p->FirstIndexPos > INDEX_FILE_LOOKUP_OFFSET) ?
                                                    (Handle_p->StreamInfo_p->FirstIndexPos - INDEX_FILE_LOOKUP_OFFSET) : 0;
                                    }
                                    else
                                    {
                                        SeekPos = Handle_p->StreamInfo_p->CircularPartIndexEntries;
                                    }
                                }
                            }

                            /* index info read into cache must be in ascending order, circular end */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }

                            /* index info read into cache must be in ascending order, circular part end */
                            if((SeekPos < Handle_p->StreamInfo_p->CircularPartIndexEntries) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->CircularPartIndexEntries))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->CircularPartIndexEntries - SeekPos;
                            }
                        }
                        else    /* Requesting Index less than 0 */
                        {
                            DVMINDEX_Error(("PCBPacket(%08X) Requesting Postion[%u,%u] < I[0].Pos[%u,%u]\n", Handle_p,
                                    SeekPos * sizeof(STDVMi_IndexInfo_t),
                                    TempPacketPosition.MSW, TempPacketPosition.LSW,
                                    Handle_p->IndexInfo_p[0].PacketPosition.MSW, Handle_p->IndexInfo_p[0].PacketPosition.LSW));
                            INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                            return ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    else
                    {
                        if(ReachedEndOfFile == FALSE)
                        {
                            SeekPos = CurStartPos + (STDVMi_INDEX_INFO_CACHE_SIZE - INDEX_FILE_LOOKUP_OFFSET);
                            if(CurStartPos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
                            {
                                if(CurStartPos > Handle_p->StreamInfo_p->FirstIndexPos)
                                {
                                    if(CurStartPos >= (Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET))
                                        SeekPos = 0;
                                    else if(SeekPos > (Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET))
                                        SeekPos = Handle_p->StreamInfo_p->CircularPartIndexEntries - INDEX_FILE_LOOKUP_OFFSET;
                                }
                                else
                                {
                                    if(SeekPos >= Handle_p->StreamInfo_p->FirstIndexPos)
                                    {
                                        SeekPos = Handle_p->StreamInfo_p->CircularPartIndexEntries;
                                    }
                                }
                            }
                            else
                            {
                                if(SeekPos >= Handle_p->IndexNbOfEntInFile)
                                {
                                    SeekPos = ((Handle_p->IndexNbOfEntInFile - Handle_p->StreamInfo_p->CircularPartIndexEntries) >
                                                                               INDEX_FILE_LOOKUP_OFFSET) ?
                                              (Handle_p->IndexNbOfEntInFile - INDEX_FILE_LOOKUP_OFFSET) :
                                              Handle_p->StreamInfo_p->CircularPartIndexEntries;
                                    ReachedEndOfFile = TRUE;
                                }
                            }

                            /* index info read into cache must be in ascending order, circular end */
                            if((SeekPos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - SeekPos;
                            }

                            /* index info read into cache must be in ascending order, circular part end */
                            if((SeekPos < Handle_p->StreamInfo_p->CircularPartIndexEntries) &&
                               ((SeekPos + IndexEntries2Read) > Handle_p->StreamInfo_p->CircularPartIndexEntries))
                            {
                                IndexEntries2Read = Handle_p->StreamInfo_p->CircularPartIndexEntries - SeekPos;
                            }
                        }
                        else
                        {
                            CheckIndexCache++;
                        }
                    }
                }
            }

            if(CheckIndexCache == 0)
            {
                DVMINDEX_Trace(("\tSTDVMi_PlayCallbackPacket(%08X) SeekPos[%u] StartPos[%d]\n", Handle_p,
                        SeekPos, Handle_p->StreamInfo_p->FirstIndexPos));

                RetVal  = vfs_fseek(Handle_p->IndexFileHandle, SeekPos * sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_SET);
                if(RetVal != OSPLUS_SUCCESS)
                {
                    DVMINDEX_Error(("PCBPacket(%08X) file seek(%d) failed\n", Handle_p,
                            SeekPos * sizeof(STDVMi_IndexInfo_t)));
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_BAD_PARAMETER;
                }

                /* Read index data in buffer */
                BytesRead = vfs_read(Handle_p->IndexFileHandle,
                                     Handle_p->IndexInfo_p,
                                     IndexEntries2Read * sizeof(STDVMi_IndexInfo_t));
                if(BytesRead <= 0) /* TODO: handle BytesRead = 0 */
                {
                    DVMINDEX_Error(("PCBPacket(%08X) file read(%d)=%d SeekPos[%d] EntInFile[%d] failed\n", Handle_p,
                            IndexEntries2Read * sizeof(STDVMi_IndexInfo_t), BytesRead,
                            SeekPos, Handle_p->IndexNbOfEntInFile));
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_NO_MEMORY;
                }

                Handle_p->IndexNbOfEntInCache = BytesRead/sizeof(STDVMi_IndexInfo_t);

                Handle_p->IndexPosAbsolute  = SeekPos;
                Handle_p->IndexPosInCache   = 0;
            }
            else
            {
                /* checking the other file handles IndexInfo_p */
                if(Handle_p->SameFileHandle_p != NULL)
                {
                    SHARED_FIELD_ACCESS_MUTEX_LOCK(Handle_p);

                    if(Handle_p->SameFileHandle_p->IndexNbOfEntInCache == 0)
                    {
                        /* No Index data in File and Buffer */
                        SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);

                        DVMINDEX_Error(("PCBPacket(%08X) No Index entries in File and Buffer\n", Handle_p));
                        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                        return ST_ERROR_BAD_PARAMETER;
                    }

                    IndexFromCache = Handle_p->SameFileHandle_p->IndexNbOfEntInCache;

                    /* when refering from Cache take the last index entry of File */
                    if((CheckIndexCache == 1) && (Handle_p->IndexNbOfEntInFile > 0) && (Handle_p->IndexNbOfEntInCache > 0))
                    {
                        IndexFromFile = (Handle_p->IndexNbOfEntInCache > INDEX_FILE_LOOKUP_OFFSET) ?
                                                INDEX_FILE_LOOKUP_OFFSET : Handle_p->IndexNbOfEntInCache;
                        memcpy(Handle_p->IndexInfo_p,
                               Handle_p->IndexInfo_p + (Handle_p->IndexNbOfEntInCache-IndexFromFile),
                               IndexFromFile);

                        if((IndexFromFile + IndexFromCache) > IndexEntries2Read)
                        {
                            IndexFromCache -= IndexFromFile;
                        }
                    }
                    else
                    {
                        IndexFromFile = 0;
                        CheckIndexCache++;
                    }

                    Handle_p->IndexPosAbsolute  = Handle_p->IndexNbOfEntInFile - IndexFromFile;
                    Handle_p->IndexPosInCache   = 0;

                    memcpy(Handle_p->IndexInfo_p + IndexFromFile,
                           Handle_p->SameFileHandle_p->IndexInfo_p,
                           sizeof(STDVMi_IndexInfo_t) * IndexFromCache);

                    Handle_p->IndexNbOfEntInCache = IndexFromFile + IndexFromCache;

                    SHARED_FIELD_ACCESS_MUTEX_RELEASE(Handle_p);
                }
                else
                {
                    DVMINDEX_Error(("PCBPacket(%08X) No Index entries in File and Buffer\n", Handle_p));
                    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
                    return ST_ERROR_BAD_PARAMETER;
                }
            } /*if(CheckIndexBuffer == FALSE)*/
        }
    } /*while()*/

    /* Stream End time is stored in TimeDurationInMs_p */
    if(*NbPacketsToInject_p != 0)
    {
        *TimeDurationInMs_p -= *FirstPacketTimeInMs_p;
    }
    else
    {
        *TimeDurationInMs_p = 0;
    }

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    DVMINDEX_Trace(("\tSTDVMi_PlayCallbackPacket(%08X) PackPos[%u,%u] P2Inj[%u] PTime[%u] PDur[%u]\n\n", Handle_p,
            PacketPosition.MSW, PacketPosition.LSW, *NbPacketsToInject_p,
            (FirstPacketTimeInMs_p != NULL) ? *FirstPacketTimeInMs_p : 0,
            (TimeDurationInMs_p != NULL) ? *TimeDurationInMs_p : 0));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_PlayCallbackStop()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_PlayCallbackStop(STPRM_Handle_t PRM_Handle)
{
    ST_ErrorCode_t      ErrorCode;
    STDVMi_Handle_t    *Handle_p;
    STPRM_PlayParams_t  PRMPlayParams;


    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL || Handle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        /* It's a live decoding, no file context found */
        return ST_NO_ERROR;
    }

    DVMINDEX_Trace(("STDVMi_PlayCallbackStop(%08X):\n", Handle_p));

#ifndef ST_OSLINUX  /* mutex not supported properly in STLinux */
    ErrorCode = STPRM_PlayGetParams(PRM_Handle, &PRMPlayParams);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMINDEX_Error(("PCBStop(%08X) STPRM_PlayGetParams(%08X) failed\n", Handle_p, PRM_Handle));
        return ST_ERROR_BAD_PARAMETER;
    }

    osplus_uncached_unregister(PRMPlayParams.TSBuffer, PRMPlayParams.TSBufferSize);
#else
    UNUSED_PARAMETER(ErrorCode);
    UNUSED_PARAMETER(PRMPlayParams);
#endif

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    vfs_close(Handle_p->IndexFileHandle);

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    INDEX_ACCESS_MUTEX_DELETE(Handle_p);

    DVMINDEX_Trace(("\tSTDVMi_PlayCallbackStop(%08X):\n\n", Handle_p));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         :  STDVMi_PlayCallbackGetTime()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t  STDVMi_PlayCallbackGetTime(STPRM_Handle_t   PRM_Handle,
                                           S64             *CurrentPacketPos,
                                           U32             *StartTimeInMs,
                                           U32             *CurrentTimeInMs,
                                           U32             *EndTimeInMs)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STDVMi_Handle_t    *Handle_p;
    U32                 NbPacketsToInject;
    S64                 StreamStartPos;
    U32                 StreamMaxTime;


    Handle_p = STDVMi_GetHandleFromPRMHandle(PRM_Handle);
    if(Handle_p == NULL || Handle_p->StreamInfo_p->Signature != STDVMi_SIGNATURE)
    {
        /* It's a live decoding, no file context found */
        return ST_NO_ERROR;
    }

    DVMINDEX_Trace(("STDVMi_PlayCallbackGetTime(%08X): CurPos[%u,%u]\n", PRM_Handle, CurrentPacketPos->MSW, CurrentPacketPos->LSW));

    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    /* check we have a valid index file handle */
    if(Handle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("PCBGetTime(%08X) Index file not yet opened\n", Handle_p));
        INDEX_ACCESS_MUTEX_RELEASE(Handle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* StreamMaxTime is properly updated by record CB functions */
    StreamMaxTime = Handle_p->StreamMaxTime;

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    *EndTimeInMs      = 0;
    *CurrentTimeInMs  = 0;
    *StartTimeInMs    = 0;

    NbPacketsToInject = 0;

    /* we may always find cur pos from the index buffer or the adjacent */
    ErrorCode = STDVMi_PlayCallbackPacket(PRM_Handle, *CurrentPacketPos, &NbPacketsToInject, CurrentTimeInMs, EndTimeInMs);
    if(ErrorCode != ST_NO_ERROR)
    {
        /* WA: When the first index position in not 0, PlayCallbackPacket returns error,
         * idealy CurrentPacketPos must start from the first index position but when CurrentPacketPos
         * goes negative due to more data in VBB, STPRM_PlayGetTime resets CurrentPacketPos to 0 */
        if((I64_IsZero(*CurrentPacketPos)) && (I64_IsZero(Handle_p->StreamInfo_p->StreamStartPos)))
        {
            *CurrentTimeInMs = 0;
             ErrorCode = ST_NO_ERROR;
        }
        else
        {
            DVMINDEX_Error(("PCBGetTime(%08X) STDVMi_PlayCallbackPacket()=%08X\n", Handle_p, ErrorCode));
            return ErrorCode;
        }
    }

    /* find actual start position of the stream */
    StreamStartPos = (I64_IsZero(Handle_p->StreamInfo_p->StreamStartPosAfterCrop)) ?
                            Handle_p->StreamInfo_p->StreamStartPos :
                            Handle_p->StreamInfo_p->StreamStartPosAfterCrop;

    ErrorCode = STDVMi_GetStreamTime(Handle_p, StreamStartPos, StartTimeInMs);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMINDEX_Error(("PCBGetTime(%08X) STDVMi_GetStreamTime()=%08X\n", Handle_p, ErrorCode));
        return ErrorCode;
    }

    /* assign end time */
    *EndTimeInMs = StreamMaxTime;

    DVMINDEX_Trace(("\tSTDVMi_PlayCallbackGetTime(%08X) TimeMS{S[%u], C[%u], E[%u]}\n\n", PRM_Handle,
            *StartTimeInMs, *CurrentTimeInMs, *EndTimeInMs));

    return ErrorCode;
}


/*******************************************************************************
Name         :  STDVMi_GetStreamTime()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_GetStreamTime(STDVMi_Handle_t *Handle_p, S64 Position, U32 *TimeInMs_p)
{
    STDVMi_Handle_t    *LocalHandle_p;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    S32                 BytesRead;
    U32                 IndexEntries2Read;
    S32                 FilePos;
    U32                 Index,
                        low,
                        mid,
                        high;


    *TimeInMs_p = 0;

    /* begining of file */
    if(I64_IsZero(Position))
    {
        return ST_NO_ERROR;
    }

    /* convert to packets */
    I64_DivLit(Position, STPRM_PACKET_SIZE, Position);

    /* First search in index buffer */
    LocalHandle_p = (Handle_p->ObjectType == STDVM_OBJECT_RECORD) ? Handle_p : Handle_p->SameFileHandle_p;

    if((LocalHandle_p != NULL) && (LocalHandle_p->StreamInfo_p->Signature == STDVMi_SIGNATURE))
    {
        INDEX_ACCESS_MUTEX_LOCK(LocalHandle_p);

        if((I64_IsGreaterOrEqual(Position, LocalHandle_p->IndexInfo_p[0].PacketPosition)) &&
           (I64_IsLessOrEqual(Position,LocalHandle_p->IndexInfo_p[LocalHandle_p->IndexNbOfEntInCache-1].PacketPosition)))
        {
            for(Index = 0; Index < LocalHandle_p->IndexNbOfEntInCache; Index++)
            {
                if(I64_IsLessOrEqual(Position, LocalHandle_p->IndexInfo_p[Index].PacketPosition))
                {
                    if(I64_IsEqual(Position, LocalHandle_p->IndexInfo_p[Index].PacketPosition))
                    {
                        *TimeInMs_p = LocalHandle_p->IndexInfo_p[Index].PacketTimeInMs;
                    }
                    else
                    {
                        /* Interpolating the PacketTime */
                        S64 DeltaPacket, DeltaPacket1;
                        U32 DeltaTime;

                        I64_Sub(LocalHandle_p->IndexInfo_p[Index].PacketPosition,
                                LocalHandle_p->IndexInfo_p[Index-1].PacketPosition,
                                DeltaPacket);
                        DeltaTime = LocalHandle_p->IndexInfo_p[Index].PacketTimeInMs
                                        - LocalHandle_p->IndexInfo_p[Index-1].PacketTimeInMs;

                        I64_Sub(Position,
                                LocalHandle_p->IndexInfo_p[Index-1].PacketPosition,
                                DeltaPacket1);

                        *TimeInMs_p = LocalHandle_p->IndexInfo_p[Index-1].PacketTimeInMs
                                            + ((DeltaPacket1.LSW * DeltaTime) / DeltaPacket.LSW);
                    }

                    INDEX_ACCESS_MUTEX_RELEASE(LocalHandle_p);
                    return ST_NO_ERROR;
                }
            }
        }

        INDEX_ACCESS_MUTEX_RELEASE(LocalHandle_p);
    }

    /* get a free handle since the Index info in actual handle will be used by record */
    LocalHandle_p = STDVMi_GetFreeHandle();
    if(LocalHandle_p == NULL)
    {
        DVMINDEX_Error(("STDVMi_GetStreamTime(%08X) No free handles\n", Handle_p));
        return ST_ERROR_NO_FREE_HANDLES;
    }

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  IdxStatus,
                    BakStatus;
        BOOL        IdxExists = TRUE,
                    BakExists = TRUE;


        GET_INDEX_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &IdxStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_GetStreamTime(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            IdxStatus.st_size = 0;
            IdxExists = FALSE;
        }

        GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_GetStreamTime(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            BakStatus.st_size = 0;
            BakExists = FALSE;
        }

        if((IdxExists == FALSE) && (BakExists == FALSE))
        {
            DVMINDEX_Error(("STDVMi_GetStreamTime(%08X) both Index and Backup file does not exist\n", Handle_p));
            STDVMi_ReleaseHandle(LocalHandle_p);
            return ST_ERROR_BAD_PARAMETER;
        }

        if(IdxStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        }
        else
        {
            GET_INDEX_FILE_NAME(LocalName, Handle_p);
        }
    }
#else
    GET_INDEX_FILE_NAME(LocalName, Handle_p);
#endif /* USE_BACKUP_FILES */

    LocalHandle_p->IndexFileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
    if(LocalHandle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("STDVMi_GetStreamTime(%08X) file open(%s) failed\n", Handle_p, LocalName));
        STDVMi_ReleaseHandle(LocalHandle_p);
        return ST_ERROR_BAD_PARAMETER;
    }

    vfs_fseek(LocalHandle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);

    LocalHandle_p->IndexNbOfEntInFile = vfs_ftell(LocalHandle_p->IndexFileHandle) / sizeof(STDVMi_IndexInfo_t);

    vfs_fseek(LocalHandle_p->IndexFileHandle, 0, OSPLUS_SEEK_SET);


    /* Perform binary search to find position */
    low  = 0;
    high = LocalHandle_p->IndexNbOfEntInFile;

    while(low <= high)
    {
        mid = (low + high) / 2;

        IndexEntries2Read = STDVMi_INDEX_INFO_CACHE_SIZE;

        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (Handle_p->StreamInfo_p->FirstIndexPos == 0))
        {
            FilePos = mid;
        }
        else if(Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
        {
            FilePos = Handle_p->StreamInfo_p->FirstIndexPos + mid;
            if(FilePos >= LocalHandle_p->IndexNbOfEntInFile)
                FilePos -= LocalHandle_p->IndexNbOfEntInFile;

            /* index info read into cache must be in ascending order */
            if((FilePos < Handle_p->StreamInfo_p->FirstIndexPos) &&
               ((FilePos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
            {
                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - FilePos;
            }
        }
        else /* circular_linear */
        {
            FilePos = mid;
            if(FilePos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
            {
                FilePos = Handle_p->StreamInfo_p->FirstIndexPos + mid;
                if(FilePos >= Handle_p->StreamInfo_p->CircularPartIndexEntries)
                    FilePos -= Handle_p->StreamInfo_p->CircularPartIndexEntries;

                /* index info read into cache must be in ascending order, cicular end */
                if((FilePos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                   ((FilePos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                {
                    IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - FilePos;
                }

                /* index info read into cache must be in ascending order, circular part end */
                if((FilePos < Handle_p->StreamInfo_p->CircularPartIndexEntries) &&
                   ((FilePos + IndexEntries2Read) > Handle_p->StreamInfo_p->CircularPartIndexEntries))
                {
                    IndexEntries2Read = Handle_p->StreamInfo_p->CircularPartIndexEntries - FilePos;
                }
            }
        }

        vfs_fseek(LocalHandle_p->IndexFileHandle, FilePos * sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_SET);

        /* Read index data into buffer */
        BytesRead = vfs_read(LocalHandle_p->IndexFileHandle,
                             LocalHandle_p->IndexInfo_p,
                             IndexEntries2Read * sizeof(STDVMi_IndexInfo_t));
        if(BytesRead <= 0)
        {
            DVMINDEX_Error(("STDVMi_GetStreamTime(%08X) file read(%d)=%d SeekPos[%d] EntInFile[%d] failed\n", Handle_p,
                    IndexEntries2Read * sizeof(STDVMi_IndexInfo_t), BytesRead,
                    FilePos, LocalHandle_p->IndexNbOfEntInFile));
            break;
        }

        LocalHandle_p->IndexNbOfEntInCache = BytesRead / sizeof(STDVMi_IndexInfo_t);

        /* Is the current packet within the index range */
        if((I64_IsGreaterOrEqual(Position, LocalHandle_p->IndexInfo_p[0].PacketPosition)) &&
           (I64_IsLessOrEqual(Position, LocalHandle_p->IndexInfo_p[LocalHandle_p->IndexNbOfEntInCache-1].PacketPosition)))
        {
            DVMINDEX_Trace(("STDVMi_GetStreamTime(%08X) Pos{0[%u,%u] %d[%u,%u]}\n", Handle_p,
                    LocalHandle_p->IndexInfo_p[0].PacketPosition.MSW, LocalHandle_p->IndexInfo_p[0].PacketPosition.LSW,
                    LocalHandle_p->IndexNbOfEntInCache-1,
                    LocalHandle_p->IndexInfo_p[LocalHandle_p->IndexNbOfEntInCache-1].PacketPosition.MSW,
                    LocalHandle_p->IndexInfo_p[LocalHandle_p->IndexNbOfEntInCache-1].PacketPosition.LSW));

            for(Index = 0; Index < LocalHandle_p->IndexNbOfEntInCache; Index++)
            {
                if(I64_IsLessOrEqual(Position, LocalHandle_p->IndexInfo_p[Index].PacketPosition))
                {
                    if(I64_IsEqual(Position, LocalHandle_p->IndexInfo_p[Index].PacketPosition))
                    {
                        *TimeInMs_p = LocalHandle_p->IndexInfo_p[Index].PacketTimeInMs;
                    }
                    else
                    {
                        /* Interpolating the PacketTime */
                        S64 DeltaPacket, DeltaPacket1;
                        U32 DeltaTime;

                        I64_Sub(LocalHandle_p->IndexInfo_p[Index].PacketPosition,
                                LocalHandle_p->IndexInfo_p[Index-1].PacketPosition,
                                DeltaPacket);
                        DeltaTime = LocalHandle_p->IndexInfo_p[Index].PacketTimeInMs
                                        - LocalHandle_p->IndexInfo_p[Index-1].PacketTimeInMs;

                        I64_Sub(Position,
                                LocalHandle_p->IndexInfo_p[Index-1].PacketPosition,
                                DeltaPacket1);

                        *TimeInMs_p = LocalHandle_p->IndexInfo_p[Index-1].PacketTimeInMs
                                            + ((DeltaPacket1.LSW * DeltaTime) / DeltaPacket.LSW);
                    }

                    vfs_close(LocalHandle_p->IndexFileHandle);

                    STDVMi_ReleaseHandle(LocalHandle_p);

                    /* Found time return */
                    return ST_NO_ERROR;
                }
            }
        }

        if (I64_IsLessThan(Position, LocalHandle_p->IndexInfo_p[0].PacketPosition))
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }

    DVMINDEX_Error(("STDVMi_GetStreamTime(%08X) could not locate Position[%u,%u]\n", Handle_p, Position.MSW, Position.LSW));
    vfs_close(LocalHandle_p->IndexFileHandle);
    STDVMi_ReleaseHandle(LocalHandle_p);

    return ST_ERROR_BAD_PARAMETER;
}


/*******************************************************************************
Name         :  STDVMi_GetStreamPosition()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_GetStreamPosition(STDVMi_Handle_t *Handle_p, U32 TimeInMs, U64 *Position_p)
{
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    S32                 BytesRead;
    U32                 IndexEntries2Read;
    S32                 FilePos;
    U32                 Index,
                        low,
                        mid,
                        high;


    /* TODO: Add finding position from index CACHE also, not needed at present */

    I64_SetValue(0, 0, *Position_p);

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  IdxStatus,
                    BakStatus;
        BOOL        IdxExists = TRUE,
                    BakExists = TRUE;


        GET_INDEX_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &IdxStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_GetStreamPosition(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            IdxStatus.st_size = 0;
            IdxExists = FALSE;
        }

        GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_GetStreamPosition(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            BakStatus.st_size = 0;
            BakExists = FALSE;
        }

        if((IdxExists == FALSE) && (BakExists == FALSE))
        {
            DVMINDEX_Error(("STDVMi_GetStreamPosition(%08X) both Index and Backup file does not exist\n", Handle_p));
            return ST_ERROR_BAD_PARAMETER;
        }

        if(IdxStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        }
        else
        {
            GET_INDEX_FILE_NAME(LocalName, Handle_p);
        }
    }
#else
    GET_INDEX_FILE_NAME(LocalName, Handle_p);
#endif /* USE_BACKUP_FILES */

    Handle_p->IndexFileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
    if(Handle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("STDVMi_GetStreamPosition(%08X) file open(%s) failed\n", Handle_p, LocalName));
        return ST_ERROR_BAD_PARAMETER;
    }

    vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);

    Handle_p->IndexNbOfEntInFile = vfs_ftell(Handle_p->IndexFileHandle) / sizeof(STDVMi_IndexInfo_t);

    vfs_fseek(Handle_p->IndexFileHandle, 0, OSPLUS_SEEK_SET);

    /* locate the start position requested */
    if(Handle_p->IndexNbOfEntInFile == 0)
    {
        DVMINDEX_Error(("STDVMi_GetStreamPosition(%08X) file open(%s) failed\n", Handle_p, LocalName));
        vfs_close(Handle_p->IndexFileHandle);
        return ST_ERROR_BAD_PARAMETER;
    }

    /* locate the position requested */

    GET_INDEX_FILE_NAME(LocalName, Handle_p);

    /* Perform binary search to find position of first index */
    low  = 0;
    high = Handle_p->IndexNbOfEntInFile;

    while(low <= high)
    {
        mid = (low + high) / 2;

        IndexEntries2Read = STDVMi_INDEX_INFO_CACHE_SIZE;

        if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (Handle_p->StreamInfo_p->FirstIndexPos == 0))
        {
            FilePos = mid;
        }
        else if(Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
        {
            FilePos = Handle_p->StreamInfo_p->FirstIndexPos + mid;
            if(FilePos >= Handle_p->IndexNbOfEntInFile)
                FilePos -= Handle_p->IndexNbOfEntInFile;

            /* index info read into cache must be in ascending order */
            if((FilePos < Handle_p->StreamInfo_p->FirstIndexPos) &&
               ((FilePos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
            {
                IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - FilePos;
            }
        }
        else /* circular_linear */
        {
            FilePos = mid;
            if(FilePos < Handle_p->StreamInfo_p->CircularPartIndexEntries)
            {
                FilePos = Handle_p->StreamInfo_p->FirstIndexPos + mid;
                if(FilePos >= Handle_p->StreamInfo_p->CircularPartIndexEntries)
                    FilePos -= Handle_p->StreamInfo_p->CircularPartIndexEntries;

                /* index info read into cache must be in ascending order, cicular end */
                if((FilePos < Handle_p->StreamInfo_p->FirstIndexPos) &&
                   ((FilePos + IndexEntries2Read) > Handle_p->StreamInfo_p->FirstIndexPos))
                {
                    IndexEntries2Read = Handle_p->StreamInfo_p->FirstIndexPos - FilePos;
                }

                /* index info read into cache must be in ascending order, circular part end */
                if((FilePos < Handle_p->StreamInfo_p->CircularPartIndexEntries) &&
                   ((FilePos + IndexEntries2Read) > Handle_p->StreamInfo_p->CircularPartIndexEntries))
                {
                    IndexEntries2Read = Handle_p->StreamInfo_p->CircularPartIndexEntries - FilePos;
                }
            }
        }

        vfs_fseek(Handle_p->IndexFileHandle, FilePos * sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_SET);

        /* Read index data into buffer */
        BytesRead = vfs_read(Handle_p->IndexFileHandle,
                             Handle_p->IndexInfo_p,
                             IndexEntries2Read * sizeof(STDVMi_IndexInfo_t));
        if(BytesRead <= 0)
        {
            DVMINDEX_Error(("STDVMi_GetStreamPosition(%08X) File rd(%d)=%d SeekPos[%d] EntInFile[%d] failed\n",
                    Handle_p,
                    IndexEntries2Read * sizeof(STDVMi_IndexInfo_t), BytesRead,
                    FilePos, Handle_p->IndexNbOfEntInFile));
            break;
        }

        Handle_p->IndexNbOfEntInCache = BytesRead / sizeof(STDVMi_IndexInfo_t);

        /* used down below when copying data */
        FilePos += Handle_p->IndexNbOfEntInCache;

        /* Is the current packet within the index range */
        if((TimeInMs >= Handle_p->IndexInfo_p[0].PacketTimeInMs) &&
           (TimeInMs <= Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache - 1].PacketTimeInMs))
        {
            for(Index = 0; Index < Handle_p->IndexNbOfEntInCache; Index++)
            {
                if(TimeInMs <= Handle_p->IndexInfo_p[Index].PacketTimeInMs)
                {
                    *Position_p = Handle_p->IndexInfo_p[Index].PacketPosition;

                    if(TimeInMs == Handle_p->IndexInfo_p[Index].PacketTimeInMs)
                    {
                        *Position_p = Handle_p->IndexInfo_p[Index].PacketPosition;
                    }
                    else
                    {
                        /* Interpolating packet position */
                        float               Ratio = 0;
                        S64                 Proportion = {0, 0};

                        Ratio = (float)((float)TimeInMs - Handle_p->IndexInfo_p[Index - 1].PacketTimeInMs) /
                                       ((float)Handle_p->IndexInfo_p[Index].PacketTimeInMs -
                                               Handle_p->IndexInfo_p[Index - 1].PacketTimeInMs);
                        I64_Sub(Handle_p->IndexInfo_p[Index].PacketPosition,
                                Handle_p->IndexInfo_p[Index - 1].PacketPosition,
                                Proportion);
                        Proportion.LSW = (U32)((float)Proportion.LSW * Ratio);
                        I64_Add(Handle_p->IndexInfo_p[Index - 1].PacketPosition, Proportion, *Position_p);
                    }

                    /* align packet position to file system max block size 32k Bytes -> 8k Packets */
                    Position_p->LSW = Position_p->LSW & 0xFFFFE000;

                    /* return position in bytes */
                    I64_MulLit(*Position_p, STDVM_PACKET_SIZE, *Position_p);

                    vfs_close(Handle_p->IndexFileHandle);
                    return ST_NO_ERROR;
                }
            }
        }

        if(TimeInMs < Handle_p->IndexInfo_p[0].PacketTimeInMs)
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }

    DVMINDEX_Error(("STDVMi_GetStreamPosition(%08X) could not locate Time[%d]\n", Handle_p, TimeInMs));
    vfs_close(Handle_p->IndexFileHandle);

    return ST_ERROR_BAD_PARAMETER;
}


/*******************************************************************************
Name         :  STDVMi_GetTimeAndSize()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_GetTimeAndSize(STDVMi_Handle_t *Handle_p, U32 *StartTimeInMs_p, U32 *DurationInMs_p, S64 *NbOfPackets_p)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    U32                 BytesRead;
    BOOL                FoundDuration = FALSE;
    S64                 StreamStartPos;


    *StartTimeInMs_p = 0;
    *DurationInMs_p  = 0;
    I64_SetValue(0, 0, *NbOfPackets_p);

    if(Handle_p->SameFileHandle_p != NULL)
    {
        /* TODO: Better way when doing new time APIs */
        /* SHARED mutex does not exist inside this function so locking INDEX mutex */
        /* INDEX_ACCESS_MUTEX_LOCK(Handle_p->SameFileHandle_p); */

        *DurationInMs_p = Handle_p->SameFileHandle_p->StreamMaxTime;
        *NbOfPackets_p  = Handle_p->SameFileHandle_p->StreamMaxPos;

        FoundDuration = TRUE;

        /* INDEX_ACCESS_MUTEX_RELEASE(Handle_p->SameFileHandle_p); */
    }

#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  IdxStatus,
                    BakStatus;
        BOOL        IdxExists = TRUE,
                    BakExists = TRUE;


        GET_INDEX_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &IdxStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_GetTimeAndSize(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            IdxStatus.st_size = 0;
            IdxExists = FALSE;
        }

        GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_GetTimeAndSize(%08X) file stat(%s) failed\n", Handle_p, LocalName));
            BakStatus.st_size = 0;
            BakExists = FALSE;
        }

        if((IdxExists == FALSE) && (BakExists == FALSE))
        {
            DVMINDEX_Error(("STDVMi_GetTimeAndSize(%08X) both Index and Backup file does not exist\n", Handle_p));
        }

        if(IdxStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INDEX_BACKUP_FILE_NAME(LocalName, Handle_p);
        }
        else
        {
            GET_INDEX_FILE_NAME(LocalName, Handle_p);
        }
    }
#else
    GET_INDEX_FILE_NAME(LocalName, Handle_p);
#endif /* USE_BACKUP_FILES */

    if(FoundDuration == FALSE)
    {
        Handle_p->IndexFileHandle = vfs_open(LocalName, OSPLUS_O_RDONLY);
        if(Handle_p->IndexFileHandle == NULL)
        {
            DVMINDEX_Error(("STDVMi_GetTimeAndSize(%08X) file open(%s) failed\n", Handle_p, LocalName));
            return ST_ERROR_BAD_PARAMETER;
        }

        if(I64_IsZero(Handle_p->StreamInfo_p->StreamEndPosAfterCrop))
        {
            /* seek to read the last Index record */
            if((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
               (Handle_p->StreamInfo_p->FirstIndexPos == 0) ||
               ((Handle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR_LINEAR) &&
                (Handle_p->IndexNbOfEntInFile > Handle_p->StreamInfo_p->CircularPartIndexEntries))) /* entries in linear part */
            {
                vfs_fseek(Handle_p->IndexFileHandle, -sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_END);
            }
            else
            {
                vfs_fseek(Handle_p->IndexFileHandle,
                            (Handle_p->StreamInfo_p->FirstIndexPos - 1) * sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_SET);
            }

            /* Read last index data in buffer */
            BytesRead = vfs_read(Handle_p->IndexFileHandle, Handle_p->IndexInfo_p, sizeof(STDVMi_IndexInfo_t));
            if (BytesRead == sizeof(STDVMi_IndexInfo_t))
            {
                *DurationInMs_p = Handle_p->IndexInfo_p[0].PacketTimeInMs;
                *NbOfPackets_p  = Handle_p->IndexInfo_p[0].PacketPosition;
            }

            vfs_close(Handle_p->IndexFileHandle);
        }
        else
        {
            vfs_close(Handle_p->IndexFileHandle);

            I64_DivLit(Handle_p->StreamInfo_p->StreamEndPosAfterCrop, STDVM_PACKET_SIZE, *NbOfPackets_p);

            ErrorCode = STDVMi_GetStreamTime(Handle_p, Handle_p->StreamInfo_p->StreamEndPosAfterCrop, DurationInMs_p);
            if(ErrorCode != ST_NO_ERROR)
            {
                DVMINDEX_Error(("STDVMi_GetTimeAndSize(%08X) STDVMi_GetStreamTime()=%08X\n", Handle_p, ErrorCode));
                return ErrorCode;
            }
        }
    }

    if(I64_IsZero(Handle_p->StreamInfo_p->StreamStartPos) && I64_IsZero(Handle_p->StreamInfo_p->StreamStartPosAfterCrop))
    {
        return ErrorCode;
    }

    /* find actual start position of the stream */
    StreamStartPos = (I64_IsZero(Handle_p->StreamInfo_p->StreamStartPosAfterCrop)) ?
                            Handle_p->StreamInfo_p->StreamStartPos :
                            Handle_p->StreamInfo_p->StreamStartPosAfterCrop;

    ErrorCode = STDVMi_GetStreamTime(Handle_p, StreamStartPos, StartTimeInMs_p);
    if(ErrorCode != ST_NO_ERROR)
    {
        DVMINDEX_Error(("STDVMi_GetTimeAndSize(%08X) STDVMi_GetStreamTime()=%08X\n", Handle_p, ErrorCode));
        return ErrorCode;
    }

    if(*DurationInMs_p < *StartTimeInMs_p)
    {
        DVMINDEX_Error(("STDVMi_GetTimeAndSize(%08X) DurationInMs < StartTimeInMs\n", Handle_p, *DurationInMs_p, *StartTimeInMs_p));
    }
    else
    {
        *DurationInMs_p -= *StartTimeInMs_p;
    }

    return ErrorCode;
}


/*******************************************************************************
Name         :  STDVMi_GetStreamRateBpmS()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
U32 STDVMi_GetStreamRateBpmS(STDVMi_Handle_t *Handle_p)
{
    S64     ElapsedBytes;
    U32     ElapsedTimeInMs;


    INDEX_ACCESS_MUTEX_LOCK(Handle_p);

    if(Handle_p->ElapsedTimeInMs == 0)  /* not yet updated to file */
    {
        /* estimate current bandwidth */
        I64_Sub(Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache -1].PacketPosition,
                Handle_p->IndexInfo_p[0].PacketPosition,
                ElapsedBytes);

        ElapsedTimeInMs = Handle_p->IndexInfo_p[Handle_p->IndexNbOfEntInCache -1].PacketTimeInMs
                            - Handle_p->IndexInfo_p[0].PacketTimeInMs;

    }
    else
    {
        ElapsedBytes    = Handle_p->ElapsedNumPackets;
        ElapsedTimeInMs = Handle_p->ElapsedTimeInMs;
    }

    I64_MulLit(ElapsedBytes, STPRM_PACKET_SIZE, ElapsedBytes);
    I64_DivLit(ElapsedBytes, ElapsedTimeInMs, ElapsedBytes);

    INDEX_ACCESS_MUTEX_RELEASE(Handle_p);

    return ElapsedBytes.LSW;    /* Bytes per milli sec */
}


/*******************************************************************************
Name         :  STDVMi_CopyIndexFileAndGetStreamPosition()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_CopyIndexFileAndGetStreamPosition(STDVMi_Handle_t    *OldFileHandle_p,
                                                        STDVMi_Handle_t    *NewFileHandle_p,
                                                        U32                 StartTimeInMs,
                                                        U32                 EndTimeInMs,
                                                        U64                *StartPosition_p,
                                                        U64                *EndPosition_p)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    char                LocalName[STDVMi_MAX_NAME_LENGTH];
    S32                 BytesRead,
                        BytesWritten;
    U32                 IndexEntries2Read;
    S32                 FilePos;
    U32                 Index,
                        low,
                        mid,
                        high;
    BOOL                FoundStartPos = FALSE,
                        FoundEndPos   = FALSE;
    U32                 StartIndex,
                        EndIndex;
    U32                 PrevIndexTime;


    I64_SetValue(0, 0, *StartPosition_p);
    I64_SetValue(0, 0, *EndPosition_p);


#ifdef USE_BACKUP_FILES
    {
        vfs_stat_t  IdxStatus,
                    BakStatus;
        BOOL        IdxExists = TRUE,
                    BakExists = TRUE;


        GET_INDEX_FILE_NAME(LocalName, OldFileHandle_p);
        if(vfs_stat(LocalName, &IdxStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file stat(%s) failed\n", OldFileHandle_p, LocalName));
            IdxStatus.st_size = 0;
            IdxExists = FALSE;
        }

        GET_INDEX_BACKUP_FILE_NAME(LocalName, OldFileHandle_p);
        if(vfs_stat(LocalName, &BakStatus) != OSPLUS_SUCCESS)
        {
            DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file stat(%s) failed\n", OldFileHandle_p, LocalName));
            BakStatus.st_size = 0;
            BakExists = FALSE;
        }

        if((IdxExists == FALSE) && (BakExists == FALSE))
        {
            DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) both Index and Backup file does not exist\n",
                    OldFileHandle_p));
            return ST_ERROR_BAD_PARAMETER;
        }

        if(IdxStatus.st_size < BakStatus.st_size)   /* use backup */
        {
            GET_INDEX_BACKUP_FILE_NAME(LocalName, OldFileHandle_p);
        }
        else
        {
            GET_INDEX_FILE_NAME(LocalName, OldFileHandle_p);
        }
    }
#else
    GET_INDEX_FILE_NAME(LocalName, OldFileHandle_p);
#endif /* USE_BACKUP_FILES */

    OldFileHandle_p->IndexFileHandle = vfs_open(LocalName, OldFileHandle_p->OpenFlags);
    if(OldFileHandle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file open(%s) failed\n", OldFileHandle_p, LocalName));
        return ST_ERROR_BAD_PARAMETER;
    }

    vfs_fseek(OldFileHandle_p->IndexFileHandle, 0, OSPLUS_SEEK_END);

    OldFileHandle_p->IndexNbOfEntInFile = vfs_ftell(OldFileHandle_p->IndexFileHandle) / sizeof(STDVMi_IndexInfo_t);

    vfs_fseek(OldFileHandle_p->IndexFileHandle, 0, OSPLUS_SEEK_SET);

    StartIndex = 0;
    EndIndex   = STDVMi_INDEX_INFO_CACHE_SIZE;

    /* locate the start position requested */
    if(OldFileHandle_p->IndexNbOfEntInFile > 0)
    {
        GET_INDEX_FILE_NAME(LocalName, OldFileHandle_p);

        /* Perform binary search to find position of first index */
        low  = 0;
        high = OldFileHandle_p->IndexNbOfEntInFile;

        while(low <= high)
        {
            mid = (low + high) / 2;

            IndexEntries2Read = STDVMi_INDEX_INFO_CACHE_SIZE;

            if((OldFileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
               (OldFileHandle_p->StreamInfo_p->FirstIndexPos == 0))
            {
                FilePos = mid;
            }
            else if(OldFileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
            {
                FilePos = OldFileHandle_p->StreamInfo_p->FirstIndexPos + mid;
                if(FilePos >= OldFileHandle_p->IndexNbOfEntInFile)
                    FilePos -= OldFileHandle_p->IndexNbOfEntInFile;

                /* index info read into cache must be in ascending order */
                if((FilePos < OldFileHandle_p->StreamInfo_p->FirstIndexPos) &&
                   ((FilePos + IndexEntries2Read) > OldFileHandle_p->StreamInfo_p->FirstIndexPos))
                {
                    IndexEntries2Read = OldFileHandle_p->StreamInfo_p->FirstIndexPos - FilePos;
                }
            }
            else /* circular_linear */
            {
                FilePos = mid;
                if(FilePos < OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries)
                {
                    FilePos = OldFileHandle_p->StreamInfo_p->FirstIndexPos + mid;
                    if(FilePos >= OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries)
                        FilePos -= OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries;

                    /* index info read into cache must be in ascending order, cicular end */
                    if((FilePos < OldFileHandle_p->StreamInfo_p->FirstIndexPos) &&
                       ((FilePos + IndexEntries2Read) > OldFileHandle_p->StreamInfo_p->FirstIndexPos))
                    {
                        IndexEntries2Read = OldFileHandle_p->StreamInfo_p->FirstIndexPos - FilePos;
                    }

                    /* index info read into cache must be in ascending order, circular part end */
                    if((FilePos < OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries) &&
                       ((FilePos + IndexEntries2Read) > OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries))
                    {
                        IndexEntries2Read = OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries - FilePos;
                    }
                }
            }

            vfs_fseek(OldFileHandle_p->IndexFileHandle, FilePos * sizeof(STDVMi_IndexInfo_t), OSPLUS_SEEK_SET);

            /* Read index data into buffer */
            BytesRead = vfs_read(OldFileHandle_p->IndexFileHandle,
                                 OldFileHandle_p->IndexInfo_p,
                                 IndexEntries2Read * sizeof(STDVMi_IndexInfo_t));
            if(BytesRead <= 0)
            {
                DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) File rd(%d)=%d SeekPos[%d] EntInFile[%d] failed\n",
                        OldFileHandle_p,
                        IndexEntries2Read * sizeof(STDVMi_IndexInfo_t), BytesRead,
                        FilePos, OldFileHandle_p->IndexNbOfEntInFile));
                vfs_close(OldFileHandle_p->IndexFileHandle);
                return ST_ERROR_NO_MEMORY;
            }

            OldFileHandle_p->IndexNbOfEntInCache = BytesRead / sizeof(STDVMi_IndexInfo_t);

            /* used down below when copying data */
            FilePos += OldFileHandle_p->IndexNbOfEntInCache;

            /* Is the current packet within the index range */
            if((StartTimeInMs >= OldFileHandle_p->IndexInfo_p[0].PacketTimeInMs) &&
               (StartTimeInMs <= OldFileHandle_p->IndexInfo_p[OldFileHandle_p->IndexNbOfEntInCache - 1].PacketTimeInMs))
            {
                for(Index = 0; Index < OldFileHandle_p->IndexNbOfEntInCache; Index++)
                {
                    if(StartTimeInMs <= OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs)
                    {
                        StartTimeInMs    = OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs;
                        *StartPosition_p = OldFileHandle_p->IndexInfo_p[Index].PacketPosition;
                        FoundStartPos    = TRUE;
                        StartIndex       = Index;
                        break;
                    }
                }

                if(FoundStartPos == TRUE)  /* data found, break from while */
                    break;
            }

            if(StartTimeInMs < OldFileHandle_p->IndexInfo_p[0].PacketTimeInMs)
            {
                high = mid - 1;
            }
            else
            {
                low = mid + 1;
            }
        }
    }
    else if(OldFileHandle_p->SameFileHandle_p != NULL)  /* no entries in the file search in index buffer */
    {
        SHARED_FIELD_ACCESS_MUTEX_LOCK(OldFileHandle_p);

        if(OldFileHandle_p->SameFileHandle_p->IndexNbOfEntInCache == 0)
        {
            SHARED_FIELD_ACCESS_MUTEX_RELEASE(OldFileHandle_p);
        }
        else
        {
            OldFileHandle_p->IndexNbOfEntInCache = OldFileHandle_p->SameFileHandle_p->IndexNbOfEntInCache;
            memcpy(OldFileHandle_p->IndexInfo_p,
                   OldFileHandle_p->SameFileHandle_p->IndexInfo_p,
                   sizeof(STDVMi_IndexInfo_t) * OldFileHandle_p->IndexNbOfEntInCache);

            SHARED_FIELD_ACCESS_MUTEX_RELEASE(OldFileHandle_p);

            /* check if start and end time is also in the buffer */
            if((StartTimeInMs >= OldFileHandle_p->IndexInfo_p[0].PacketTimeInMs) &&
               (StartTimeInMs <= OldFileHandle_p->IndexInfo_p[OldFileHandle_p->IndexNbOfEntInCache - 1].PacketTimeInMs) &&
               (EndTimeInMs   >= OldFileHandle_p->IndexInfo_p[0].PacketTimeInMs) &&
               (EndTimeInMs   <= OldFileHandle_p->IndexInfo_p[OldFileHandle_p->IndexNbOfEntInCache - 1].PacketTimeInMs))
            {
                /* index file handle is no more required does not have any entry */
                vfs_close(OldFileHandle_p->IndexFileHandle);

                StartIndex = 0;
                EndIndex   = OldFileHandle_p->IndexNbOfEntInCache;

                for(Index = 0; Index < OldFileHandle_p->IndexNbOfEntInCache; Index++)
                {
                    if(FoundStartPos == FALSE)
                    {
                        if(StartTimeInMs <= OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs)
                        {
                            StartTimeInMs    = OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs;
                            *StartPosition_p = OldFileHandle_p->IndexInfo_p[Index].PacketPosition;

                            OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs = 0;
                            I64_SetValue(0, 0, OldFileHandle_p->IndexInfo_p[Index].PacketPosition);

                            StartIndex = Index;
                            FoundStartPos = TRUE;
                        }
                    }
                    else
                    {
                        if(EndTimeInMs > OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs)
                        {
                            OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs -= StartTimeInMs;
                            I64_Sub(OldFileHandle_p->IndexInfo_p[Index].PacketPosition,
                                    *StartPosition_p,
                                    OldFileHandle_p->IndexInfo_p[Index].PacketPosition);
                        }
                        else
                        {
                            EndTimeInMs    = OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs;
                            *EndPosition_p = OldFileHandle_p->IndexInfo_p[Index].PacketPosition;

                            OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs -= StartTimeInMs;
                            I64_Sub(OldFileHandle_p->IndexInfo_p[Index].PacketPosition,
                                    *StartPosition_p,
                                    OldFileHandle_p->IndexInfo_p[Index].PacketPosition);

                            EndIndex       = Index+1;
                            FoundEndPos    = TRUE;
                            break;
                        }
                    }
                }

                /* could not locate end pos */
                if(FoundEndPos == FALSE)
                {
                    DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) could not locate END Time\n", OldFileHandle_p));
                    return ST_ERROR_BAD_PARAMETER;
                }

#ifdef USE_BACKUP_FILES
                GET_INDEX_BACKUP_FILE_NAME(LocalName, NewFileHandle_p);

                NewFileHandle_p->IndexFileHandle = vfs_open(LocalName, NewFileHandle_p->OpenFlags);
                if(NewFileHandle_p->IndexFileHandle == NULL)
                {
                    DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file open(%s) failed\n",
                            NewFileHandle_p, LocalName));
                    return ST_ERROR_BAD_PARAMETER;
                }

                BytesWritten = vfs_write(NewFileHandle_p->IndexFileHandle,
                                         OldFileHandle_p->IndexInfo_p + StartIndex,
                                         (EndIndex - StartIndex) * sizeof(STDVMi_IndexInfo_t));

                vfs_close(NewFileHandle_p->IndexFileHandle);

                if(BytesWritten != (EndIndex - StartIndex) * sizeof(STDVMi_IndexInfo_t))
                {
                    DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file write(%d)=%d failed\n",
                            NewFileHandle_p, (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t), BytesWritten));
                    return ST_ERROR_NO_MEMORY;
                }
#endif  /* USE_BACKUP_FILES */

                GET_INDEX_FILE_NAME(LocalName, NewFileHandle_p);

                NewFileHandle_p->IndexFileHandle = vfs_open(LocalName, NewFileHandle_p->OpenFlags);
                if(NewFileHandle_p->IndexFileHandle == NULL)
                {
                    DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file open(%s) failed\n",
                            NewFileHandle_p, LocalName));
                    return ST_ERROR_BAD_PARAMETER;
                }

                BytesWritten = vfs_write(NewFileHandle_p->IndexFileHandle,
                                         OldFileHandle_p->IndexInfo_p + StartIndex,
                                         (EndIndex - StartIndex) * sizeof(STDVMi_IndexInfo_t));

                vfs_close(NewFileHandle_p->IndexFileHandle);

                if(BytesWritten != (EndIndex - StartIndex) * sizeof(STDVMi_IndexInfo_t))
                {
                    DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file write(%d)=%d failed\n",
                            NewFileHandle_p, (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t), BytesWritten));
                    return ST_ERROR_NO_MEMORY;
                }

                return ST_NO_ERROR;
            }
        }
    }

    /* could not locate start pos */
    if(FoundStartPos == FALSE)
    {
        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) could not locate START Time\n", OldFileHandle_p));
        vfs_close(OldFileHandle_p->IndexFileHandle);
        return ST_ERROR_BAD_PARAMETER;
    }

#ifndef OPEN_CLOSE_FILE_ON_WRITE
    GET_INDEX_FILE_NAME(LocalName, NewFileHandle_p);

    NewFileHandle_p->IndexFileHandle = vfs_open(LocalName, NewFileHandle_p->OpenFlags);
    if(NewFileHandle_p->IndexFileHandle == NULL)
    {
        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file open(%s) failed\n",
                NewFileHandle_p, LocalName));
        vfs_close(OldFileHandle_p->IndexFileHandle);
        return ST_ERROR_BAD_PARAMETER;
    }
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

    EndIndex      = OldFileHandle_p->IndexNbOfEntInCache;
    PrevIndexTime = 0;

    while(TRUE)
    {
        for(Index = StartIndex; Index < EndIndex; Index++)
        {
            /* finding wrap around within index file */
            if(PrevIndexTime > OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs)
                break;

            PrevIndexTime = OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs;

            if(EndTimeInMs > OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs)
            {
                OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs -= StartTimeInMs;
                I64_Sub(OldFileHandle_p->IndexInfo_p[Index].PacketPosition,
                        *StartPosition_p,
                        OldFileHandle_p->IndexInfo_p[Index].PacketPosition);
            }
            else
            {
                EndTimeInMs    = OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs;
                *EndPosition_p = OldFileHandle_p->IndexInfo_p[Index].PacketPosition;

                OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs -= StartTimeInMs;
                I64_Sub(OldFileHandle_p->IndexInfo_p[Index].PacketPosition,
                        *StartPosition_p,
                        OldFileHandle_p->IndexInfo_p[Index].PacketPosition);

                EndIndex       = Index+1;
                FoundEndPos    = TRUE;
                break;
            }
        }

        BytesWritten = vfs_write(NewFileHandle_p->IndexFileHandle,
                                 OldFileHandle_p->IndexInfo_p + StartIndex,
                                 (EndIndex - StartIndex) * sizeof(STDVMi_IndexInfo_t));
        if(BytesWritten != (EndIndex - StartIndex) * sizeof(STDVMi_IndexInfo_t))
        {
            DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file write(%d)=%d failed\n",
                    NewFileHandle_p, (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t), BytesWritten));
#ifndef OPEN_CLOSE_FILE_ON_WRITE
            vfs_close(NewFileHandle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */
            vfs_close(OldFileHandle_p->IndexFileHandle);
            return ST_ERROR_NO_MEMORY;
        }

        if(FoundEndPos == TRUE)
            break;

        IndexEntries2Read = STDVMi_INDEX_INFO_CACHE_SIZE;

        if((OldFileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_LINEAR) ||
           (OldFileHandle_p->StreamInfo_p->FirstIndexPos == 0))
        {
            /* nothing to do for linear file, continue reading below */
        }
        else if(OldFileHandle_p->StreamInfo_p->StreamFileType == STREAM_FILE_TYPE_CIRCULAR)
        {
            if(FilePos >= OldFileHandle_p->IndexNbOfEntInFile)
            {
                vfs_fseek(OldFileHandle_p->IndexFileHandle, 0, OSPLUS_SEEK_SET);
            }

            /* index info read into cache must be in ascending order */
            if((FilePos < OldFileHandle_p->StreamInfo_p->FirstIndexPos) &&
               ((FilePos + IndexEntries2Read) > OldFileHandle_p->StreamInfo_p->FirstIndexPos))
            {
                IndexEntries2Read = OldFileHandle_p->StreamInfo_p->FirstIndexPos - FilePos;
            }
        }
        else /* circular_linear */
        {
            if(FilePos <= OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries)
            {
                if(FilePos == OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries)
                {
                    vfs_fseek(OldFileHandle_p->IndexFileHandle, 0, OSPLUS_SEEK_SET);
                }

                if(FilePos == OldFileHandle_p->StreamInfo_p->FirstIndexPos)
                {
                    vfs_fseek(OldFileHandle_p->IndexFileHandle,
                                OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries, OSPLUS_SEEK_SET);
                }

                /* index info read into cache must be in ascending order, cicular end */
                if((FilePos < OldFileHandle_p->StreamInfo_p->FirstIndexPos) &&
                   ((FilePos + IndexEntries2Read) > OldFileHandle_p->StreamInfo_p->FirstIndexPos))
                {
                    IndexEntries2Read = OldFileHandle_p->StreamInfo_p->FirstIndexPos - FilePos;
                }

                /* index info read into cache must be in ascending order, circular part end */
                if((FilePos < OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries) &&
                   ((FilePos + IndexEntries2Read) > OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries))
                {
                    IndexEntries2Read = OldFileHandle_p->StreamInfo_p->CircularPartIndexEntries - FilePos;
                }
            }
        }

        BytesRead = vfs_read(OldFileHandle_p->IndexFileHandle,
                             OldFileHandle_p->IndexInfo_p,
                             IndexEntries2Read * sizeof(STDVMi_IndexInfo_t));
        if(BytesRead <= 0)
        {
            DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file read(%d)=%d failed\n",
                    NewFileHandle_p, IndexEntries2Read * sizeof(STDVMi_IndexInfo_t), BytesRead));
#ifndef OPEN_CLOSE_FILE_ON_WRITE
            vfs_close(NewFileHandle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */
            vfs_close(OldFileHandle_p->IndexFileHandle);
            return ST_ERROR_NO_MEMORY;
        }

        StartIndex = 0;
        EndIndex   = BytesRead / sizeof(STDVMi_IndexInfo_t);

        FilePos   += EndIndex;
    }

    if((FoundEndPos == FALSE) && (OldFileHandle_p->SameFileHandle_p != NULL))  /* no entries in the file search in index buffer */
    {
        SHARED_FIELD_ACCESS_MUTEX_LOCK(OldFileHandle_p);

        if(OldFileHandle_p->SameFileHandle_p->IndexNbOfEntInCache == 0)
        {
            SHARED_FIELD_ACCESS_MUTEX_RELEASE(OldFileHandle_p);
        }
        else
        {
            OldFileHandle_p->IndexNbOfEntInCache = OldFileHandle_p->SameFileHandle_p->IndexNbOfEntInCache;
            memcpy(OldFileHandle_p->IndexInfo_p,
                   OldFileHandle_p->SameFileHandle_p->IndexInfo_p,
                   sizeof(STDVMi_IndexInfo_t) * OldFileHandle_p->IndexNbOfEntInCache);

            SHARED_FIELD_ACCESS_MUTEX_RELEASE(OldFileHandle_p);

            /* check if start and end time is also in the buffer */
            if((EndTimeInMs >= OldFileHandle_p->IndexInfo_p[0].PacketTimeInMs) &&
               (EndTimeInMs <= OldFileHandle_p->IndexInfo_p[OldFileHandle_p->IndexNbOfEntInCache - 1].PacketTimeInMs))
            {
                StartIndex = 0;
                EndIndex   = OldFileHandle_p->IndexNbOfEntInCache;

                for(Index = 0; Index < OldFileHandle_p->IndexNbOfEntInCache; Index++)
                {
                    if(EndTimeInMs > OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs)
                    {
                        OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs -= StartTimeInMs;
                        I64_Sub(OldFileHandle_p->IndexInfo_p[Index].PacketPosition,
                                *StartPosition_p,
                                OldFileHandle_p->IndexInfo_p[Index].PacketPosition);
                    }
                    else
                    {
                        EndTimeInMs    = OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs;
                        *EndPosition_p = OldFileHandle_p->IndexInfo_p[Index].PacketPosition;

                        OldFileHandle_p->IndexInfo_p[Index].PacketTimeInMs -= StartTimeInMs;
                        I64_Sub(OldFileHandle_p->IndexInfo_p[Index].PacketPosition,
                                *StartPosition_p,
                                OldFileHandle_p->IndexInfo_p[Index].PacketPosition);

                        EndIndex       = Index+1;
                        FoundEndPos    = TRUE;
                        break;
                    }
                }

                /* could not locate end pos */
                if(FoundEndPos == TRUE)
                {
#ifdef USE_BACKUP_FILES
                    GET_INDEX_BACKUP_FILE_NAME(LocalName, NewFileHandle_p);

                    NewFileHandle_p->IndexFileHandle = vfs_open(LocalName, NewFileHandle_p->OpenFlags);
                    if(NewFileHandle_p->IndexFileHandle == NULL)
                    {
                        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file open(%s) failed\n",
                                NewFileHandle_p, LocalName));
                        vfs_close(OldFileHandle_p->IndexFileHandle);
                        return ST_ERROR_BAD_PARAMETER;
                    }

                    BytesWritten = vfs_write(NewFileHandle_p->IndexFileHandle,
                                             OldFileHandle_p->IndexInfo_p + StartIndex,
                                             (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t));
                    if(BytesWritten != (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t))
                    {
                        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file write(%d)=%d failed\n",
                                NewFileHandle_p, (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t), BytesWritten));
                        vfs_close(NewFileHandle_p->IndexFileHandle);
                        vfs_close(OldFileHandle_p->IndexFileHandle);
                        return ST_ERROR_NO_MEMORY;
                    }

                    vfs_close(NewFileHandle_p->IndexFileHandle);
#endif  /* USE_BACKUP_FILES */

#ifdef OPEN_CLOSE_FILE_ON_WRITE
                    GET_INDEX_FILE_NAME(LocalName, NewFileHandle_p);

                    NewFileHandle_p->IndexFileHandle = vfs_open(LocalName, NewFileHandle_p->OpenFlags);
                    if(NewFileHandle_p->IndexFileHandle == NULL)
                    {
                        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file open(%s) failed\n",
                                NewFileHandle_p, LocalName));
                        vfs_close(OldFileHandle_p->IndexFileHandle);
                        return ST_ERROR_BAD_PARAMETER;
                    }
#endif /* OPEN_CLOSE_FILE_ON_WRITE */

                    BytesWritten = vfs_write(NewFileHandle_p->IndexFileHandle,
                                             OldFileHandle_p->IndexInfo_p + StartIndex,
                                             (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t));
                    if(BytesWritten != (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t))
                    {
                        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) file write(%d)=%d failed\n",
                                NewFileHandle_p, (EndIndex - StartIndex)*sizeof(STDVMi_IndexInfo_t), BytesWritten));
                        vfs_close(NewFileHandle_p->IndexFileHandle);
                        vfs_close(OldFileHandle_p->IndexFileHandle);
                        return ST_ERROR_NO_MEMORY;
                    }

#ifdef OPEN_CLOSE_FILE_ON_WRITE
                    vfs_close(NewFileHandle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */
                }
            }
        }
    }

    /* could not locate end pos */
    if(FoundEndPos == FALSE)
    {
        DVMINDEX_Error(("STDVMi_CopyIndexFileAndGetStreamPosition(%08X) could not locate END Time\n", OldFileHandle_p));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

#ifndef OPEN_CLOSE_FILE_ON_WRITE
    vfs_close(NewFileHandle_p->IndexFileHandle);
#endif /* OPEN_CLOSE_FILE_ON_WRITE */
    vfs_close(OldFileHandle_p->IndexFileHandle);

    return ErrorCode;
}

/* EOF --------------------------------------------------------------------- */

