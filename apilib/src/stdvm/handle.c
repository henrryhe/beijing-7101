/*****************************************************************************

File Name  : handle.c

Description: STDVM Handle manipulation funnctions

Copyright (C) 2005 STMicroelectronics

*****************************************************************************/

/* Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "handle.h"


/* Private Constants ------------------------------------------------------ */

/* Private Types ---------------------------------------------------------- */

/* Private Variables ------------------------------------------------------ */
static  semaphore_t        *STDVMi_HandleMutex_p;

/* Private Macros --------------------------------------------------------- */
#define FS_FILE_HANDLE_MUTEX_CREATE()      MUTEX_CREATE(STDVMi_HandleMutex_p)
#define FS_FILE_HANDLE_MUTEX_LOCK()        MUTEX_LOCK(STDVMi_HandleMutex_p)
#define FS_FILE_HANDLE_MUTEX_RELEASE()     MUTEX_RELEASE(STDVMi_HandleMutex_p)
#define FS_FILE_HANDLE_MUTEX_DELETE()      MUTEX_DELETE(STDVMi_HandleMutex_p)


/* Private Function prototypes -------------------------------------------- */

/* Global Variables ------------------------------------------------------- */
STDVMi_Handle_t            *STDVMi_Handles_p;
U32                         STDVMi_NbOfHandles;

/* Prototypes ------------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */


/*******************************************************************************
Name         : STDVMi_AllocateHandles()

Description  : Allocate necessary number of STDVM Handles

Parameters   : STDVM_InitParams_t  *InitParams_p     Init params passed to STDVM_Init
               U32                  MaxHandles       Maximum handles required

Return Value : Error code
*******************************************************************************/
ST_ErrorCode_t STDVMi_AllocateHandles(U32 MaxHandles)
{

    STDVMi_Handles_p = memory_allocate(STDVMi_CachePartition, sizeof(STDVMi_Handle_t)*MaxHandles);
    if(STDVMi_Handles_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_AllocateHandles : No Memory\n"));
        return ST_ERROR_NO_MEMORY;
    }

    FS_FILE_HANDLE_MUTEX_CREATE();
    if(STDVMi_HandleMutex_p == NULL)
    {
        memory_deallocate(STDVMi_CachePartition, STDVMi_Handles_p);

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_AllocateHandles : Mutex init\n"));
        return ST_ERROR_NO_MEMORY;
    }

    memset(STDVMi_Handles_p, 0 , sizeof(STDVMi_Handle_t)*MaxHandles);

    STDVMi_NbOfHandles = MaxHandles;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_DeallocateHandles()

Description  : Allocate necessary number of STDVM Handles

Parameters   : ST_Partition_t  *Partition           Partiton to allocate handles
               U32              MaxHandles          Maximum handles required

Return Value : Error code
*******************************************************************************/
ST_ErrorCode_t STDVMi_DeallocateHandles(void)
{
    FS_FILE_HANDLE_MUTEX_DELETE();

    memory_deallocate(STDVMi_CachePartition, STDVMi_Handles_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_GetFreeHandle()

Description  : Allocate necessary number of STDVM Handles

Parameters   : None

Return Value : Pointer to STDVMi_Handle_t
*******************************************************************************/
STDVMi_Handle_t *STDVMi_GetFreeHandle(void)
{
    STDVMi_Handle_t    *Handle_p = NULL;
    U32                 i;


    FS_FILE_HANDLE_MUTEX_LOCK();

    for(i = 0; i < STDVMi_NbOfHandles; i++)
    {
        if(STDVMi_Handles_p[i].HandleInUse == FALSE)
        {
            Handle_p = &STDVMi_Handles_p[i];

            memset(Handle_p, 0, sizeof(STDVMi_Handle_t));

            /* allocate memory for IndexInfo cache */
            Handle_p->Buffer_p = memory_allocate(STDVMi_NCachePartition,
                                    sizeof(STDVMi_StreamInfo_t) + sizeof(STDVMi_IndexInfo_t)*STDVMi_INDEX_INFO_CACHE_SIZE + 31);
            if(Handle_p->Buffer_p == NULL )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Handle_p->Buffer_p : NULL\n"));
                FS_FILE_HANDLE_MUTEX_RELEASE();
                return NULL;
            }

            Handle_p->IndexInfo_p  = (STDVMi_IndexInfo_t *)(((U32)Handle_p->Buffer_p+31)&(~31));

            Handle_p->StreamInfo_p = (STDVMi_StreamInfo_t *)(Handle_p->IndexInfo_p+STDVMi_INDEX_INFO_CACHE_SIZE);

            memset(Handle_p->StreamInfo_p, 0, sizeof(STDVMi_StreamInfo_t));

            Handle_p->HandleInUse  = TRUE;
            break;
        }
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return Handle_p;
}


/*******************************************************************************
Name         : STDVMi_ReleaseHandle()

Description  : Allocate necessary number of STDVM Handles

Parameters   : Pointer to STDVMi_Handle_t

Return Value : Error code
*******************************************************************************/
ST_ErrorCode_t STDVMi_ReleaseHandle(STDVMi_Handle_t *Handle_p)
{

    if(Handle_p->HandleInUse == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_ReleaseHandle : Invalid handle\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    memory_deallocate(STDVMi_NCachePartition, Handle_p->Buffer_p);

    memset(Handle_p, 0, sizeof(STDVMi_Handle_t));

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_GetHandleFromPRMHandle()

Description  : Returns the STDVM handle from the PRM handle

Parameters   : STPRM_Handle_t   PRM_Handle      PRM handle to get STDVM handle

Return Value : Pointer to STDVMi_Handle_t
*******************************************************************************/
STDVMi_Handle_t *STDVMi_GetHandleFromPRMHandle(STPRM_Handle_t   PRM_Handle)
{
    STDVMi_Handle_t    *Handle_p = NULL;
    U32                 i;


    FS_FILE_HANDLE_MUTEX_LOCK();

    for(i = 0; i < STDVMi_NbOfHandles; i++)
    {
        Handle_p = &STDVMi_Handles_p[i];

        if(Handle_p->HandleInUse == TRUE && Handle_p->PRM_Handle == PRM_Handle)
            break;
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return (i < STDVMi_NbOfHandles) ? Handle_p : NULL;
}


#ifdef ENABLE_MP3_PLAYBACK
/*******************************************************************************
Name         : STDVMi_GetHandleFromMP3PBHandle()

Description  : Returns the STDVM handle from the PRM handle

Parameters   : STPRM_Handle_t   PRM_Handle      PRM handle to get STDVM handle

Return Value : Pointer to STDVMi_Handle_t
*******************************************************************************/
STDVMi_Handle_t *STDVMi_GetHandleFromMP3PBHandle(MP3PB_Handle_t MP3_Handle)
{
    STDVMi_Handle_t    *Handle_p;
    U32                 i;


    FS_FILE_HANDLE_MUTEX_LOCK();

    for(i = 0; i < STDVMi_NbOfHandles; i++)
    {
        Handle_p = &STDVMi_Handles_p[i];

        if(Handle_p->HandleInUse == TRUE && Handle_p->MP3_Handle == MP3_Handle)
            break;
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return (i < STDVMi_NbOfHandles) ? Handle_p : NULL;
}
#endif


/*******************************************************************************
Name         : STDVMi_ValidateOpen()

Description  : Returns ST_NO_ERROR if the file is not open for delete else ST_ERROR_OPEN_HANDLE

Parameters   : STDVMi_Handle_t *Handle_p        Handle to check with other open handles

Return Value : Pointer to STDVMi_Handle_t
*******************************************************************************/
ST_ErrorCode_t STDVMi_ValidateOpen(STDVMi_Handle_t *Handle_p)
{
    STDVMi_Handle_t    *HandleTemp_p;
    U32                 i;


    FS_FILE_HANDLE_MUTEX_LOCK();

    for(i = 0; i < STDVMi_NbOfHandles; i++)
    {
        HandleTemp_p = &STDVMi_Handles_p[i];

        if((HandleTemp_p != Handle_p) &&
           (HandleTemp_p->HandleInUse == TRUE) &&
           (HandleTemp_p->StreamInfo_p->Signature == STDVMi_SIGNATURE) &&
           (strcmp(HandleTemp_p->PathName, Handle_p->PathName) == 0) &&
           (strcmp(HandleTemp_p->FileName, Handle_p->FileName) == 0) &&
           ((HandleTemp_p->OpenFlags == STDVMi_OPEN_FLAGS_DELETE) ||
            (Handle_p->OpenFlags == STDVMi_OPEN_FLAGS_DELETE) ||
            ((HandleTemp_p->OpenFlags & OSPLUS_O_ACCMODE) == OSPLUS_O_RDONLY) ||    /* already opened in read mode */
            (((HandleTemp_p->OpenFlags & OSPLUS_O_ACCMODE) == OSPLUS_O_WRONLY) &&
             ((Handle_p->OpenFlags & OSPLUS_O_ACCMODE) == OSPLUS_O_WRONLY))))       /* already opened in write mode */
            break;
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return (i < STDVMi_NbOfHandles) ? ST_ERROR_OPEN_HANDLE : ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_GetAlreadyOpenHandle()

Description  : Returns ST_NO_ERROR if the file is not open for delete else ST_ERROR_OPEN_HANDLE

Parameters   : STDVMi_Handle_t *Handle_p        Handle to check with other open handles

Return Value : Pointer to STDVMi_Handle_t
*******************************************************************************/
STDVMi_Handle_t *STDVMi_GetAlreadyOpenHandle(STDVMi_Handle_t *Handle_p)
{
    STDVMi_Handle_t    *HandleTemp_p = NULL;
    U32                 i;


    FS_FILE_HANDLE_MUTEX_LOCK();

    for(i = 0; i < STDVMi_NbOfHandles; i++)
    {
        HandleTemp_p = &STDVMi_Handles_p[i];

        if((HandleTemp_p != Handle_p) &&
           (HandleTemp_p->HandleInUse == TRUE) &&
           (HandleTemp_p->StreamInfo_p->Signature == STDVMi_SIGNATURE) &&
           (strcmp(HandleTemp_p->PathName, Handle_p->PathName) == 0) &&
           (strcmp(HandleTemp_p->FileName, Handle_p->FileName) == 0) &&
           ((HandleTemp_p->OpenFlags & OSPLUS_O_ACCMODE) == OSPLUS_O_WRONLY)) /* consider only files already opened in write mode */
            break;
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return (i < STDVMi_NbOfHandles) ? HandleTemp_p : NULL;
}


/*******************************************************************************
Name         : STDVMi_GetHandleFromBuffer()

Description  : Returns ST_NO_ERROR if the file is not open for delete else ST_ERROR_OPEN_HANDLE

Parameters   : void *Buffer     Address in the TS buffer

Return Value : Pointer to STDVMi_Handle_t
*******************************************************************************/
STDVMi_Handle_t *STDVMi_GetHandleFromBuffer(void *Buffer)
{
    STDVMi_Handle_t    *HandleTemp_p = NULL;
    U32                 i;


    FS_FILE_HANDLE_MUTEX_LOCK();

    for(i = 0; i < STDVMi_NbOfHandles; i++)
    {
        HandleTemp_p = &STDVMi_Handles_p[i];

        if((HandleTemp_p->HandleInUse == TRUE) &&
           (HandleTemp_p->StreamInfo_p->Signature == STDVMi_SIGNATURE) &&
           (Buffer >= HandleTemp_p->TSBufferStart) &&
           (Buffer <= HandleTemp_p->TSBufferEnd))
            break;
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return (i < STDVMi_NbOfHandles) ? HandleTemp_p : NULL;
}


/*******************************************************************************
Name         : STDVMi_IsHandleValid()

Description  : check if the handle is valid

Parameters   : None

Return Value : TRUE - Valid Handle
*******************************************************************************/
BOOL STDVMi_IsHandleValid(STDVMi_Handle_t *Handle_p)
{
    BOOL                HandleValid = FALSE;


    FS_FILE_HANDLE_MUTEX_LOCK();

    if((Handle_p >= STDVMi_Handles_p) && (Handle_p < (STDVMi_Handles_p+STDVMi_NbOfHandles)) &&
       (((U32)Handle_p - (U32)STDVMi_Handles_p) % sizeof(STDVMi_Handle_t) == 0) &&
       (Handle_p->HandleInUse == TRUE))
    {
        HandleValid = TRUE;
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return HandleValid;
}


/*******************************************************************************
Name         : STDVMi_IsAnyHandleOpen()

Description  : Check if any handle is open

Parameters   : None

Return Value : TRUE - Handle open
*******************************************************************************/
BOOL STDVMi_IsAnyHandleOpen(void)
{
    BOOL                HandleOpen = FALSE;
    U32                 i;


    FS_FILE_HANDLE_MUTEX_LOCK();

    for(i = 0; i < STDVMi_NbOfHandles; i++)
    {
        if(STDVMi_Handles_p[i].HandleInUse == TRUE)
        {
            HandleOpen = TRUE;
            break;
        }
    }

    FS_FILE_HANDLE_MUTEX_RELEASE();

    return HandleOpen;
}

/* EOF ---------------------------------------------------------------------- */

