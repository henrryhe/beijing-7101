/*****************************************************************************

File Name  : dvmcrypt.c

Description: Encrypt and Decrypt support functions

Copyright (C) 2006 STMicroelectronics

*****************************************************************************/

/* Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvmcrypt.h"
#if defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
#include "stcrypt.h"
#endif
#if defined(ST_7109)
#include "sttkdma.h"
#endif

/* Private Constants ------------------------------------------------------ */

/* Private Types ---------------------------------------------------------- */

/* Private Variables ------------------------------------------------------ */
#if defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
static  STEVT_Handle_t  STDVMi_CryptEvtHandle;
static  semaphore_t    *STDVMi_CryptStepSem;
#endif
static  semaphore_t    *STDVMi_CryptMutex;
static  U8             *STDVMi_CryptBuffer;

/* Private Macros --------------------------------------------------------- */
#define CRYPT_MUTEX_CREATE()        MUTEX_CREATE(STDVMi_CryptMutex)
#define CRYPT_MUTEX_LOCK()          MUTEX_LOCK(STDVMi_CryptMutex)
#define CRYPT_MUTEX_RELEASE()       MUTEX_RELEASE(STDVMi_CryptMutex)
#define CRYPT_MUTEX_DELETE()        MUTEX_DELETE(STDVMi_CryptMutex)

#define TKDMA_CPCW_KEY_INDEX        0

/* Private Function prototypes -------------------------------------------- */

/* Global Variables ------------------------------------------------------- */
U8                     *STDVMi_CryptBufferAligned;

/* Prototypes ------------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */

#if defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
/*******************************************************************************
Name         : STDVMi_CryptEventCallback()

Description  : Event callback function for STCRYPT events

Parameters   :

Return Value : ST_ErrorCode_t
*******************************************************************************/
static void STDVMi_CryptEventCallback(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData)
{
    /* Wake up the dma function */
    if(Event == STCRYPT_TRANSFER_COMPLETE_EVT)
    {
        semaphore_signal(STDVMi_CryptStepSem);
    }
}
#endif


/*******************************************************************************
Name         : STDVMi_CryptInit()

Description  : Required intializations for Encrypt/Decrypt functionality

Parameters   :

Return Value : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STDVMi_CryptInit(void)
{
    ST_ErrorCode_t          ErrorCode;

    UNUSED_PARAMETER(ErrorCode);
#ifndef ST_OSLINUX
    /* allocate buffer to be used by FileWrite */
    STDVMi_CryptBuffer = memory_allocate(STDVMi_CachePartition, STDVMi_TS_RECORD_CHUNK_SIZE+31);
    if(STDVMi_CryptBuffer == NULL)
#else
    ErrorCode = UTILS_AllocData_DVM(STDVMi_TS_RECORD_CHUNK_SIZE, 32, (void *)&STDVMi_CryptBuffer);
    if(ErrorCode != ST_NO_ERROR)
#endif
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "memory_allocate(%d) failed\n", STDVMi_TS_RECORD_CHUNK_SIZE+31));
        return ST_ERROR_NO_MEMORY;
    }

    /* Create CRYPT mutex */
    CRYPT_MUTEX_CREATE();

#if defined(ARCHITECTURE_ST40) && !defined(ST_OSLINUX)
    STDVMi_CryptBufferAligned = (U8 *)((((U32)STDVMi_CryptBuffer + 31) & (~31)) | 0xA0000000);
#else
    STDVMi_CryptBufferAligned = (U8 *)(((U32)STDVMi_CryptBuffer + 31) & (~31));
#endif

#if defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
    {
        STEVT_OpenParams_t      OpenParams;
        STEVT_SubscribeParams_t SubscribeParams;

        /* Initialise STCRYPT Event */
        OpenParams.dummy = 0;
        ErrorCode = STEVT_Open(STCRYPT_DEVICE_NAME, &OpenParams, &STDVMi_CryptEvtHandle);
        if(ErrorCode != ST_NO_ERROR)
        {
#ifndef ST_OSLINUX
            memory_deallocate(STDVMi_CachePartition, STDVMi_CryptBuffer);
#else
            UTILS_FreeData_DVM(STDVMi_CryptBuffer);
#endif

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Open()=%08X\n", ErrorCode));
            return ErrorCode;
        }

        /* Register for the CRYPT event */
        SubscribeParams.NotifyCallback = STDVMi_CryptEventCallback;
        ErrorCode = STEVT_Subscribe(STDVMi_CryptEvtHandle, STCRYPT_TRANSFER_COMPLETE_EVT, &SubscribeParams);
        if(ErrorCode != ST_NO_ERROR)
        {
            STEVT_Close(STDVMi_CryptEvtHandle);
#ifndef ST_OSLINUX
            memory_deallocate(STDVMi_CachePartition, STDVMi_CryptBuffer);
#else
            UTILS_FreeData_DVM(STDVMi_CryptBuffer);
#endif

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Subscribe()=%08X\n", ErrorCode));
            return ErrorCode;
        }

        /* Create Transfer complete semaphore */
        STDVMi_CryptStepSem = semaphore_create_fifo(0);
    }
#endif

    /* register with OSPLUS as non-cached since this buffer is going to accessed by h/w only */
    osplus_uncached_register(STDVMi_CryptBufferAligned, STDVMi_TS_RECORD_CHUNK_SIZE);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_CryptTerm()

Description  : Termination of intializations done for Encrypt/Decrypt functionality

Parameters   :

Return Value : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STDVMi_CryptTerm(void)
{
    ST_ErrorCode_t          ErrorCode;

    UNUSED_PARAMETER(ErrorCode);
#if defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
    ErrorCode = STEVT_Unsubscribe(STDVMi_CryptEvtHandle, STCRYPT_TRANSFER_COMPLETE_EVT);
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Unsubscribe()=%08X\n", ErrorCode));
        return ErrorCode;
    }

    ErrorCode = STEVT_Close(STDVMi_CryptEvtHandle);
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Close()=%08X\n", ErrorCode));
        return ErrorCode;
    }

    /* Delete Transfer complete semaphore */
    semaphore_delete(STDVMi_CryptStepSem);
#endif

    /* Delete CRYPT mutex */
    CRYPT_MUTEX_DELETE();

#ifndef ST_OSLINUX
    memory_deallocate(STDVMi_CachePartition, STDVMi_CryptBuffer);
#else
    UTILS_FreeData_DVM(STDVMi_CryptBuffer);
#endif

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_Decrypt()

Description  : Decrypt data in the Buffer with the key for this file handle

Parameters   : STDVMi_Handle_t *FileHandle_p        STDVM internal file handle
               void            *SrcBuffer_p         Src Buffer to decrypt
               void            *DstBuffer_p         Dst Buffer to decrypt
               U32              Size                Size in bytes

Return Value : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STDVMi_Decrypt(STDVMi_Handle_t *FileHandle_p, void *SrcBuffer_p, void *DstBuffer_p, U32 Size)
{
    ST_ErrorCode_t          ErrorCode;

    /* in some cases like EOF the size could be 0, return NO_ERROR */
    if(Size == 0)
    {
        return ST_NO_ERROR;
    }

#ifndef ST_OSLINUX
    /* with OSPLUS E2FS configured for 32KB block size, there may be memcpy from OS+ internal buffers to the Read buffer */
    /* cache purge to ensure non-coherency */
    cache_purge_data(SrcBuffer_p, Size);
#endif

    CRYPT_MUTEX_LOCK();

#if defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
    {
        STCRYPT_DmaSetup_t      DmaSetup;

        DmaSetup.Algorithm     = STCRYPT_ALGORITHM_AES;
        DmaSetup.Mode          = STCRYPT_ENGINE_MODE_DECRYPT;
        DmaSetup.Key           = STCRYPT_KEY_IS_CHANNEL_0;
#ifndef ST_OSLINUX
        DmaSetup.Source_p      = SrcBuffer_p;
        DmaSetup.Destination_p = DstBuffer_p;
#else
        DmaSetup.Source_p      = UTILS_UserToKernel(SrcBuffer_p);
        DmaSetup.Destination_p = UTILS_UserToKernel(DstBuffer_p);
#endif
        DmaSetup.Size          = Size;


        ErrorCode = STCRYPT_SetKey((const STCRYPT_Key *)&FileHandle_p->CryptKey, DmaSetup.Key);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STCRYPT_SetKey()=%08X\n", ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }

        ErrorCode = STCRYPT_StartDma(&DmaSetup);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STCRYPT_StartDma(%08X,%d)=%08X\n", DmaSetup.Source_p,DmaSetup.Size,ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }

        /* Wait for end of transfer */
        semaphore_wait(STDVMi_CryptStepSem);
    }
#endif
#if defined(ST_7109)
    {
        STTKDMA_DMAConfig_t     DmaConfig;

        ErrorCode = STTKDMA_DecryptKey(STTKDMA_COMMAND_DIRECT_CPCW, (STTKDMA_Key_t *)&FileHandle_p->CryptKey, TKDMA_CPCW_KEY_INDEX, NULL);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STTKDMA_DecryptKey()=%08X\n", ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }

        DmaConfig.algorithm           = STTKDMA_ALGORITHM_AES;
        DmaConfig.algorithm_mode      = STTKDMA_ALGORITHM_MODE_ECB;
        DmaConfig.refresh             = TRUE;
        DmaConfig.decrypt_not_encrypt = TRUE;
        DmaConfig.sck_override        = FALSE;

#ifndef ST_OSLINUX
        ErrorCode = STTKDMA_StartDMA(&DmaConfig, (U8 *)SrcBuffer_p, (U8 *)DstBuffer_p, Size, TKDMA_CPCW_KEY_INDEX, TRUE);
#else
        ErrorCode = STTKDMA_StartDMA(&DmaConfig, (U8 *)UTILS_UserToKernel(SrcBuffer_p), (U8 *)UTILS_UserToKernel(DstBuffer_p), Size, TKDMA_CPCW_KEY_INDEX, TRUE);
#endif
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"STTKDMA_StartDMA(%08X,%08X,%d)=%08X\n",SrcBuffer_p,DstBuffer_p,Size,ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }
    }
#endif

    CRYPT_MUTEX_RELEASE();

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_Encrypt()

Description  : Encrypt data in the Buffer with the key of this file handle

Parameters   : STDVMi_Handle_t *FileHandle_p        STDVM internal file handle
               void            *SrcBuffer_p         Src Buffer to decrypt
               void            *DstBuffer_p         Dst Buffer to decrypt
               U32              Size                Size in bytes

Return Value : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STDVMi_Encrypt(STDVMi_Handle_t *FileHandle_p, void *SrcBuffer_p, void *DstBuffer_p, U32 Size)
{
    ST_ErrorCode_t          ErrorCode;

    /* in some cases like EOF the size could be 0, return NO_ERROR */
    if(Size == 0)
    {
        return ST_NO_ERROR;
    }

    CRYPT_MUTEX_LOCK();

#if defined(ST_5100) || defined(ST_7710) || defined(ST_7100)
    {
        STCRYPT_DmaSetup_t      DmaSetup;

        DmaSetup.Algorithm     = STCRYPT_ALGORITHM_AES;
        DmaSetup.Mode          = STCRYPT_ENGINE_MODE_ENCRYPT;
        DmaSetup.Key           = STCRYPT_KEY_IS_CHANNEL_0;
#ifndef ST_OSLINUX
        DmaSetup.Source_p      = SrcBuffer_p;
        DmaSetup.Destination_p = DstBuffer_p;
#else
        DmaSetup.Source_p      = UTILS_UserToKernel(SrcBuffer_p);
        DmaSetup.Destination_p = UTILS_UserToKernel(DstBuffer_p);
#endif
        DmaSetup.Size          = Size;


        ErrorCode = STCRYPT_SetKey((const STCRYPT_Key *)&FileHandle_p->CryptKey, DmaSetup.Key);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STCRYPT_SetKey()=%08X\n", ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }

        ErrorCode = STCRYPT_StartDma(&DmaSetup);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STCRYPT_StartDma(%08X,%d)=%08X\n", DmaSetup.Source_p, DmaSetup.Size, ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }

        /* Wait for end of transfer */
        semaphore_wait(STDVMi_CryptStepSem);
    }
#endif

#if defined(ST_7109)
    {
        STTKDMA_DMAConfig_t     DmaConfig;

        ErrorCode = STTKDMA_DecryptKey(STTKDMA_COMMAND_DIRECT_CPCW, (STTKDMA_Key_t *)&FileHandle_p->CryptKey, TKDMA_CPCW_KEY_INDEX, NULL);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STTKDMA_DecryptKey()=%08X\n", ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }

        DmaConfig.algorithm           = STTKDMA_ALGORITHM_AES;
        DmaConfig.algorithm_mode      = STTKDMA_ALGORITHM_MODE_ECB;
        DmaConfig.refresh             = TRUE;
        DmaConfig.decrypt_not_encrypt = FALSE;
        DmaConfig.sck_override        = FALSE;

#ifndef ST_OSLINUX
        ErrorCode = STTKDMA_StartDMA(&DmaConfig, (U8 *)SrcBuffer_p, (U8 *)DstBuffer_p, Size, TKDMA_CPCW_KEY_INDEX, TRUE);
#else
        ErrorCode = STTKDMA_StartDMA(&DmaConfig, (U8 *)UTILS_UserToKernel(SrcBuffer_p), (U8 *)UTILS_UserToKernel(DstBuffer_p), Size, TKDMA_CPCW_KEY_INDEX, TRUE);
#endif
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"STTKDMA_StartDMA(%08X,%08X,%d)=%08X\n",SrcBuffer_p,DstBuffer_p,Size,ErrorCode));
            CRYPT_MUTEX_RELEASE();
            return ErrorCode;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STTKDMA_StartDMA(%08X,%08X,%d)=%08X\n",SrcBuffer_p,DstBuffer_p,Size,ErrorCode));
        }
    }
#endif

    CRYPT_MUTEX_RELEASE();

    return ST_NO_ERROR;
}

/* EOF --------------------------------------------------------------------- */
