/*****************************************************************************

File name: mp3pb.c

Description: Read MP3 files from HDD and decode

COPYRIGHT (C) STMicroelectronics 2005

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdevice.h"
#include "staud.h"
#include "stfdma.h"
#include "sttbx.h"
#include "stcommon.h"

/* osplus includes */
#include <osplus.h>
#include <osplus/vfs.h>

#include "mp3pb.h"
#include "stsys.h"

#ifdef ENABLE_TRACE
 #include "trace.h"
#endif

/* Private Constants ------------------------------------------------------ */
#define MP3PB_REVISION              "MP3PB-REL_1.0.0A"

#define MP3PB_SIGNATURE             (0xFA111234)

#define MP3PB_BUFFER_SIZE           (8*1024)
#define MP3PB_BUFFER_ALIGNMENT      (256)
#define MP3PB_NO_OF_BUFFERS         (2)

#define MP3PB_TASK_STACK_SIZE       (4*1024)
#define MP3PB_TASK_PRIORITY         (MIN_USER_PRIORITY)
#define MP3PB_TASK_NAME             "MP3PB_PlayTask"

#define MP3PB_MAX_HANDLES           2

#define MP3PB_NB_FRAMES_BEFORE_EOP  20

#ifdef  MP3PB_USE_CPU_INJECTION
    #define MP3PB_INJECT_TASK_STACK_SIZE    (4*1024)
    #define MP3PB_INJECT_TASK_PRIORITY      (MAX_USER_PRIORITY)
    #define MP3PB_INJECT_TASK_NAME          "MP3PB_InjectTask"
#endif

/* Private Types ---------------------------------------------------------- */

typedef enum MP3PB_TaskState_e
{
    TASK_RUNNING,
    TASK_STOPPED
}MP3PB_TaskState_t;


typedef struct MP3PBi_Handle_s
{
    U32                 Signature;

    ST_DeviceName_t     AUDDeviceName;      /* AUD device name used for AUD event subsciption */
    STAUD_Handle_t      AUDHandle;

    vfs_file_t         *FileHandle;

    task_t             *PlayTask_p;
    MP3PB_TaskState_t   PlayTaskState;

    U32                 AudNbFrames;

#ifndef  MP3PB_USE_CPU_INJECTION
    semaphore_t        *FDMACallbackSem_p;
    U32                 CallbackReason;
#else
    message_queue_t    *InjectMsgQ_p;
    task_t             *InjectTask_p;
#endif
} MP3PBi_Handle_t;


typedef struct MP3PBi_Params_s
{
    ST_DeviceName_t     DeviceName;         /* MP3PB Device name                 */
    ST_Partition_t     *CachePartition;     /* Memory cache partition            */
    ST_Partition_t     *NCachePartition;    /* Memory non cache partition        */

    STEVT_Handle_t      EVTHandle;          /* Event handle opened for reporting */
    STEVT_EventID_t     EventIDs[MP3PB_MAX_EVTS];

    MP3PBi_Handle_t    *Handles_p;
} MP3PBi_Params_t;

/* Private Variables ------------------------------------------------------ */

static BOOL                 MP3PB_Initialized = FALSE;
static MP3PBi_Params_t      MP3PBi_Params;

/* Private Macros --------------------------------------------------------- */

#ifdef ENABLE_TRACE
    #define MP3PB_Trace(__X__)          TraceBuffer(__X__)
#else
    #define MP3PB_Trace(__X__)          /* nop */
#endif


/* Private Function prototypes -------------------------------------------- */
static void MP3PBi_AUDCallback(STEVT_CallReason_t    Reason,
                               const ST_DeviceName_t RegistrantName,
                               STEVT_EventConstant_t Event,
                               const void           *EventData,
                               const void           *SubscriberData_p);
static void MP3PBi_PlayTask(MP3PBi_Handle_t *MP3PBi_Handle_p);
#ifdef  MP3PB_USE_CPU_INJECTION
static void MP3PBi_InjectTask(MP3PBi_Handle_t *MP3PBi_Handle_p);
#endif

/* Global Variables ------------------------------------------------------- */

/* Global Functions ------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : MP3PB_Init
 *
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t MP3PB_Init(ST_DeviceName_t DeviceName, MP3PB_InitParams_t *InitParams)
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    STEVT_OpenParams_t  EVT_OpenParams = { 0 };
    U8                  i;


    if(MP3PB_Initialized == TRUE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Init : Already Initialized\n"));
        return ST_ERROR_ALREADY_INITIALIZED;
    }

    MP3PB_Initialized = TRUE;

    strcpy(MP3PBi_Params.DeviceName, DeviceName);
    MP3PBi_Params.CachePartition  = InitParams->CachePartition;
    MP3PBi_Params.NCachePartition = InitParams->NCachePartition;

    MP3PBi_Params.Handles_p = memory_allocate(MP3PBi_Params.CachePartition, sizeof(MP3PBi_Handle_t)*MP3PB_MAX_HANDLES);
    if(MP3PBi_Params.Handles_p == NULL)
    {
        MP3PB_Initialized = FALSE;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Init : MP3PBi_Params.Handles_p = NULL\n"));
        return ST_ERROR_NO_MEMORY;
    }


    ErrCode = STEVT_Open(InitParams->EVTDeviceName, &EVT_OpenParams, &MP3PBi_Params.EVTHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        MP3PB_Initialized = FALSE;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Init : STEVT_Open()=%08X\n", ErrCode));
        return ST_ERROR_BAD_PARAMETER;
    }

    for(i = MP3PB_EVT_END_OF_PLAYBACK_ID; i < MP3PB_MAX_EVTS; i++)
    {
        ErrCode = STEVT_RegisterDeviceEvent(MP3PBi_Params.EVTHandle, DeviceName, MP3PB_EVT_END_OF_PLAYBACK+i,
                        &MP3PBi_Params.EventIDs[i]);
        if(ErrCode != ST_NO_ERROR)
        {
            MP3PB_Initialized = FALSE;
            STEVT_Close(MP3PBi_Params.EVTHandle);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Init : STEVT_RegisterDeviceEvent()=%08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    memset(MP3PBi_Params.Handles_p, 0, sizeof(MP3PBi_Handle_t)*MP3PB_MAX_HANDLES);

    return ErrCode;
}


/*-------------------------------------------------------------------------
 * Function : MP3PB_GetRevision
 *
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
ST_Revision_t  MP3PB_GetRevision(void)
{
    return ((ST_Revision_t) MP3PB_REVISION);
}


/*-------------------------------------------------------------------------
 * Function : MP3PB_Term
 *
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t MP3PB_Term(ST_DeviceName_t DeviceName, MP3PB_TermParams_t *TermParams)
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    U8                  i;


    if((MP3PB_Initialized == FALSE) ||
       (strcmp(MP3PBi_Params.DeviceName, DeviceName) != 0))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Term : Not Initialized\n"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    if(TermParams->ForceTerminate != TRUE)
    {
        for(i = 0; i < MP3PB_MAX_HANDLES; i++)
        {
            if(MP3PBi_Params.Handles_p[i].Signature == MP3PB_SIGNATURE)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Term : Handle not closed\n"));
                return ST_ERROR_OPEN_HANDLE;
            }
        }
    }

    for(i = MP3PB_EVT_END_OF_PLAYBACK_ID; i < MP3PB_MAX_EVTS; i++)
    {
        ErrCode = STEVT_UnregisterDeviceEvent(MP3PBi_Params.EVTHandle, DeviceName, MP3PB_EVT_END_OF_PLAYBACK+i);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Term : STEVT_UnregisterDeviceEvent()=%08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    ErrCode = STEVT_Close(MP3PBi_Params.EVTHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Term : STEVT_Close()=%08X\n", ErrCode));
        return ST_ERROR_BAD_PARAMETER;
    }

    memory_deallocate(MP3PBi_Params.CachePartition, MP3PBi_Params.Handles_p);

    MP3PB_Initialized = FALSE;

    return ErrCode;
}


/*-------------------------------------------------------------------------
 * Function : MP3PB_Open
 *
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t MP3PB_Open(ST_DeviceName_t DeviceName, MP3PB_OpenParams_t *OpenParams, MP3PB_Handle_t *Handle)
{
    MP3PBi_Handle_t    *MP3PBi_Handle_p = NULL;
    U8                  i;


    if((MP3PB_Initialized == FALSE) ||
       (strcmp(MP3PBi_Params.DeviceName, DeviceName) != 0))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Open : Not Initialized\n"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    for(i = 0; i < MP3PB_MAX_HANDLES; i++)
    {
        if(MP3PBi_Params.Handles_p[i].Signature != MP3PB_SIGNATURE)
        {
            MP3PBi_Handle_p = &MP3PBi_Params.Handles_p[i];
            MP3PBi_Handle_p->Signature = MP3PB_SIGNATURE;
            break;
        }
    }

    if(MP3PBi_Handle_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Open : No free handles\n"));
        return ST_ERROR_NO_FREE_HANDLES;
    }

    MP3PBi_Handle_p->AUDHandle = OpenParams->AUDHandle;
    strcpy(MP3PBi_Handle_p->AUDDeviceName, OpenParams->AUDDeviceName);
    MP3PBi_Handle_p->PlayTaskState = TASK_STOPPED;

    *Handle = (MP3PB_Handle_t)MP3PBi_Handle_p;

    return ST_NO_ERROR;
}


/*-------------------------------------------------------------------------
 * Function : MP3PB_Close
 *
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t MP3PB_Close(MP3PB_Handle_t Handle)
{
    MP3PBi_Handle_t    *MP3PBi_Handle_p = (MP3PBi_Handle_t *)Handle;


    if(MP3PBi_Handle_p->Signature != MP3PB_SIGNATURE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_Close : MP3PBi_Handle_p->Signature != MP3PB_SIGNATURE\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    memset(MP3PBi_Handle_p, 0, sizeof(MP3PBi_Handle_t));

    return ST_NO_ERROR;
}


/*-------------------------------------------------------------------------
 * Function : MP3PB_PlayStart
 * Input    : FileName_p
 * Output   :
 * Return   : ST_ErrorCode_t
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t MP3PB_PlayStart(MP3PB_Handle_t Handle, MP3PB_PlayStartParams_t *PlayStartParams)
{
    ST_ErrorCode_t                  ErrCode = ST_NO_ERROR;
    MP3PBi_Handle_t                *MP3PBi_Handle_p = (MP3PBi_Handle_t *)Handle;
    STAUD_StartParams_t             AUDStartParams;
    STEVT_DeviceSubscribeParams_t   STEVT_SubcribeParams;


    if(MP3PBi_Handle_p->Signature != MP3PB_SIGNATURE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : MP3PBi_Handle_p->Signature != MP3PB_SIGNATURE\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(MP3PBi_Handle_p->PlayTaskState == TASK_RUNNING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : Play already started\n"));
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    MP3PBi_Handle_p->FileHandle = vfs_open((const char *)PlayStartParams->SourceName, OSPLUS_O_RDONLY);
    if(MP3PBi_Handle_p->FileHandle == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : file open %s = FAILED\n", PlayStartParams->SourceName));
        return ST_ERROR_BAD_PARAMETER;
    }

#ifndef  MP3PB_USE_CPU_INJECTION
    MP3PBi_Handle_p->FDMACallbackSem_p = semaphore_create_fifo(MP3PB_NO_OF_BUFFERS-1);
    if(MP3PBi_Handle_p->FDMACallbackSem_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : Sem Create FAILED\n"));
        return ST_ERROR_NO_MEMORY;
    }
#endif

    MP3PBi_Handle_p->PlayTaskState = TASK_RUNNING;

    MP3PBi_Handle_p->PlayTask_p = task_create((void (*)(void *))MP3PBi_PlayTask,
                                        MP3PBi_Handle_p,
                                        MP3PB_TASK_STACK_SIZE,
                                        MP3PB_TASK_PRIORITY,
                                        MP3PB_TASK_NAME,
                                        task_flags_suspended | task_flags_no_min_stack_size);
    if(MP3PBi_Handle_p->PlayTask_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : Task Create(%s) FAILED\n", MP3PB_TASK_NAME));
        return ST_ERROR_NO_MEMORY;
    }

    MP3PBi_Handle_p->AudNbFrames = 0;

    /* subscribe device events */
    STEVT_SubcribeParams.NotifyCallback   = MP3PBi_AUDCallback;
    STEVT_SubcribeParams.SubscriberData_p = MP3PBi_Handle_p;

    ErrCode |= STEVT_SubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName,
                    STAUD_PCM_UNDERFLOW_EVT, &STEVT_SubcribeParams);
    ErrCode |= STEVT_SubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName,
                    STAUD_PCM_BUF_COMPLETE_EVT, &STEVT_SubcribeParams);
    ErrCode |= STEVT_SubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName,
                    STAUD_FIFO_OVERF_EVT, &STEVT_SubcribeParams);
    ErrCode |= STEVT_SubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName,
                    STAUD_CDFIFO_UNDERFLOW_EVT, &STEVT_SubcribeParams);
    ErrCode |= STEVT_SubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName,
                    STAUD_SYNC_ERROR_EVT, &STEVT_SubcribeParams);
    ErrCode |= STEVT_SubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName,
                    STAUD_LOW_DATA_LEVEL_EVT, &STEVT_SubcribeParams);
    ErrCode |= STEVT_SubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName,
                    STAUD_NEW_FRAME_EVT, &STEVT_SubcribeParams);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : STEVT_SubscribeDeviceEvent()=%08X\n", ErrCode));
        return ErrCode;
    }

    AUDStartParams.StreamType           = STAUD_STREAM_TYPE_ES;
    AUDStartParams.StreamContent        = STAUD_STREAM_CONTENT_MP3;
    AUDStartParams.SamplingFrequency    = 32000;
    AUDStartParams.StreamID             = STAUD_IGNORE_ID;

    ErrCode = STAUD_Start(MP3PBi_Handle_p->AUDHandle, &AUDStartParams);
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MP3PB_PlayStart : STAUD_Start(ES MP3)=%08X\n", ErrCode));

    ErrCode = STAUD_Mute(MP3PBi_Handle_p->AUDHandle, FALSE, FALSE);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MP3PB_PlayStart : STAUD_Mute(FALSE)=%s\n", ErrCode));
    }

#ifdef  MP3PB_USE_CPU_INJECTION
    MP3PBi_Handle_p->InjectMsgQ_p = message_create_queue(sizeof(STFDMA_Node_t *), MP3PB_NO_OF_BUFFERS);
    if(MP3PBi_Handle_p->InjectMsgQ_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : MsgQ Create FAILED\n"));
        return ST_ERROR_NO_MEMORY;
    }

    MP3PBi_Handle_p->InjectTask_p = task_create((void (*)(void *))MP3PBi_InjectTask,
                                        MP3PBi_Handle_p,
                                        MP3PB_INJECT_TASK_STACK_SIZE,
                                        MP3PB_INJECT_TASK_PRIORITY,
                                        MP3PB_INJECT_TASK_NAME,
                                        task_flags_no_min_stack_size );
    if(MP3PBi_Handle_p->InjectTask_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : Task Create(%s) FAILED\n", MP3PB_INJECT_TASK_NAME));
        return ST_ERROR_NO_MEMORY;
    }
#endif

    /* resume the playback task */
    task_resume(MP3PBi_Handle_p->PlayTask_p);

    return ErrCode;
}


/*-------------------------------------------------------------------------
 * Function : MP3PB_PlayStop
 * Input    :
 * Output   :
 * Return   : ST_ErrorCode_t
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t MP3PB_PlayStop(MP3PB_Handle_t Handle, MP3PB_PlayStopParams_t *PlayStopParams)
{
    ST_ErrorCode_t      ErrCode;
    MP3PBi_Handle_t    *MP3PBi_Handle_p = (MP3PBi_Handle_t *)Handle;


    if(MP3PBi_Handle_p->Signature != MP3PB_SIGNATURE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStop : MP3PBi_Handle_p->Signature != MP3PB_SIGNATURE\n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    if(MP3PBi_Handle_p->PlayTaskState == TASK_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStart : Play already stopped\n"));
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    ErrCode = STAUD_Stop(MP3PBi_Handle_p->AUDHandle, PlayStopParams->AudioStopMethod, &PlayStopParams->AudioFadingMethod);
    if ((ErrCode != ST_NO_ERROR) && (ErrCode != STAUD_ERROR_DECODER_STOPPED))
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MP3PB_PlayStop : STAUD_Stop()=%08X\n", ErrCode));

    MP3PBi_Handle_p->PlayTaskState = TASK_STOPPED;

#ifndef  MP3PB_USE_CPU_INJECTION
    semaphore_signal(MP3PBi_Handle_p->FDMACallbackSem_p);
#endif

    /* Unsubscribe to events */
    ErrCode |= STEVT_UnsubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName, STAUD_NEW_FRAME_EVT);
    ErrCode |= STEVT_UnsubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName, STAUD_LOW_DATA_LEVEL_EVT);
    ErrCode |= STEVT_UnsubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName, STAUD_SYNC_ERROR_EVT);
    ErrCode |= STEVT_UnsubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName, STAUD_CDFIFO_UNDERFLOW_EVT);
    ErrCode |= STEVT_UnsubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName, STAUD_FIFO_OVERF_EVT);
    ErrCode |= STEVT_UnsubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName, STAUD_PCM_UNDERFLOW_EVT);
    ErrCode |= STEVT_UnsubscribeDeviceEvent(MP3PBi_Params.EVTHandle, MP3PBi_Handle_p->AUDDeviceName, STAUD_PCM_BUF_COMPLETE_EVT);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PB_PlayStop : STEVT_UnsubscribeDeviceEvent()=%08X\n", ErrCode));
        return ErrCode;
    }


    task_wait(&MP3PBi_Handle_p->PlayTask_p, 1, TIMEOUT_INFINITY);
    if ( task_delete(MP3PBi_Handle_p->PlayTask_p) != 0 )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Task Delete(MP3PB_MP3PlayTask)=failed\n"));
    }

#ifndef  MP3PB_USE_CPU_INJECTION
    semaphore_delete(MP3PBi_Handle_p->FDMACallbackSem_p);
#else
    message_send(MP3PBi_Handle_p->InjectMsgQ_p, NULL);

    task_wait(&MP3PBi_Handle_p->InjectTask_p, 1, TIMEOUT_INFINITY);
    if ( task_delete(MP3PBi_Handle_p->InjectTask_p) != 0 )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Task Delete(MP3PB_MP3PlayTask)=failed\n"));
    }

    message_delete_queue(MP3PBi_Handle_p->InjectMsgQ_p);
#endif

    vfs_close(MP3PBi_Handle_p->FileHandle);

    return ST_NO_ERROR;
}


/*-------------------------------------------------------------------------
 * Function : MP3PBi_AUDCallback
 *
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void MP3PBi_AUDCallback(STEVT_CallReason_t    Reason,
                               const ST_DeviceName_t RegistrantName,
                               STEVT_EventConstant_t Event,
                               const void           *EventData,
                               const void           *SubscriberData_p)
{
    MP3PBi_Handle_t *MP3PBi_Handle_p = (MP3PBi_Handle_t *)SubscriberData_p;


    switch(Event)
    {
        case STAUD_NEW_FRAME_EVT:
            MP3PBi_Handle_p->AudNbFrames++;
            break;

        /* Error events */
        case STAUD_PCM_UNDERFLOW_EVT    :
            MP3PB_Trace(("MP3PBi_AUDCallback(%08X):**ERROR** !!! STAUD_PCM_UNDERFLOW_EVT !!!\n"   , MP3PBi_Handle_p));
            break;

        case STAUD_PCM_BUF_COMPLETE_EVT :
            MP3PB_Trace(("MP3PBi_AUDCallback(%08X):**ERROR** !!! STAUD_PCM_BUF_COMPLETE_EVT !!!\n", MP3PBi_Handle_p));
            break;

        case STAUD_FIFO_OVERF_EVT       :
            MP3PB_Trace(("MP3PBi_AUDCallback(%08X):**ERROR** !!! STAUD_FIFO_OVERF_EVT !!!\n"      ,MP3PBi_Handle_p));
            break;

        case STAUD_CDFIFO_UNDERFLOW_EVT :
            MP3PB_Trace(("MP3PBi_AUDCallback(%08X):**ERROR** !!! STAUD_CDFIFO_UNDERFLOW_EVT !!!\n",MP3PBi_Handle_p));
            break;

        case STAUD_SYNC_ERROR_EVT       :
            MP3PB_Trace(("MP3PBi_AUDCallback(%08X):**ERROR** !!! STAUD_SYNC_ERROR_EVT !!!\n"      ,MP3PBi_Handle_p));
            break;

        case STAUD_LOW_DATA_LEVEL_EVT   :
            MP3PB_Trace(("MP3PBi_AUDCallback(%08X):**ERROR** !!! STAUD_LOW_DATA_LEVEL_EVT !!!\n"  ,MP3PBi_Handle_p));
            break;

        /* Strange events */
        default :
            MP3PB_Trace(("MP3PBi_AUDCallback(%08X):**ERROR** !!! Unkown Event 0x%08X !!!\n"       ,MP3PBi_Handle_p, Event));
            break;
    }
}


#ifndef  MP3PB_USE_CPU_INJECTION
/*-------------------------------------------------------------------------
 * Function : MP3PBi_FDMACallback
 *
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void MP3PBi_FDMACallback(U32     TransferId,
                                U32     CallbackReason,
                                U32    *CurrentNode_p,
                                U32     NodeBytesRemaining,
                                BOOL    Error,
                                void   *ApplicationData_p,
                                clock_t InterruptTime)
{
    MP3PBi_Handle_t *MP3PBi_Handle_p;

    if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED ||
       CallbackReason == STFDMA_NOTIFY_TRANSFER_COMPLETE)
    {
        MP3PBi_Handle_p = (MP3PBi_Handle_t *)ApplicationData_p;

        MP3PBi_Handle_p->CallbackReason = CallbackReason;
        semaphore_signal(MP3PBi_Handle_p->FDMACallbackSem_p);
    }
}
#endif

/*-------------------------------------------------------------------------
 * Function : MP3PBi_PlayTask
 *            Function is called as subtask
 * Input    : MP3PBi_Handle_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void MP3PBi_PlayTask(MP3PBi_Handle_t *MP3PBi_Handle_p)
{
    ST_ErrorCode_t              ErrCode = ST_NO_ERROR;
#ifndef  MP3PB_USE_CPU_INJECTION
    STFDMA_TransferParams_t     FDMATransferParams;
    STFDMA_TransferId_t         FDMATransferId;
    BOOL                        FDMAStarted = FALSE;
#endif
    STFDMA_Node_t              *FirstNode_p,
                               *Node_p;
    U8                         *Temp_p;

    U8                         *BufferAlloc_p,
                               *Buffer_p[MP3PB_NO_OF_BUFFERS];
    U8                          Count;
    S32                         BytesRead;
    U32                         AudNbFrames,
                                NbFramesBeforeEOP;
#ifdef  MP3PB_USE_CPU_INJECTION
    STFDMA_Node_t              **Node_pp;
#endif
    U32                         AudBufFreeSize;


    BufferAlloc_p = memory_allocate(MP3PBi_Params.NCachePartition, MP3PB_BUFFER_SIZE*MP3PB_NO_OF_BUFFERS+MP3PB_BUFFER_ALIGNMENT-1);
    if(BufferAlloc_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : Aud Buffer allocate size[%d] fails\n",
                MP3PB_BUFFER_SIZE*MP3PB_NO_OF_BUFFERS+MP3PB_BUFFER_ALIGNMENT-1));

        ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_ERROR_ID], &MP3PBi_Handle_p);
        if(ErrCode != ST_NO_ERROR)
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify = %08X\n", ErrCode));
        return;
    }

    Buffer_p[0] = (U8 *)(((U32)BufferAlloc_p+(MP3PB_BUFFER_ALIGNMENT-1)) & ~(MP3PB_BUFFER_ALIGNMENT-1));
    for(Count = 1; Count < MP3PB_NO_OF_BUFFERS; Count++)
        Buffer_p[Count] = Buffer_p[Count-1]+MP3PB_BUFFER_SIZE;

    Temp_p = memory_allocate(MP3PBi_Params.NCachePartition, sizeof(STFDMA_Node_t)*MP3PB_NO_OF_BUFFERS+31);
    if(Temp_p == NULL)
    {
        memory_deallocate(MP3PBi_Params.NCachePartition, BufferAlloc_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : Aud FDMA node allocate size[%d] fails\n",
                sizeof(STFDMA_Node_t)+31 ));

        ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_ERROR_ID], &MP3PBi_Handle_p);
        if(ErrCode != ST_NO_ERROR)
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify = %08X\n", ErrCode));
        return;
    }

    Node_p = (STFDMA_Node_t *)(((U32)Temp_p+31) & ~31);

    /* Generic node config */
    Node_p->Next_p                           = NULL;
    Node_p->SourceStride                     = 0;
    Node_p->DestinationStride                = 0;
    Node_p->NumberBytes                      = MP3PB_BUFFER_SIZE;
    Node_p->Length                           = MP3PB_BUFFER_SIZE;
    Node_p->SourceAddress_p                  = Buffer_p[0];
    Node_p->DestinationAddress_p             = (void *)AUDIO_CD_FIFO;
    Node_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
    Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
    Node_p->NodeControl.Reserved             = 0;
    Node_p->NodeControl.PaceSignal           = (U32)STFDMA_REQUEST_SIGNAL_CD_AUDIO;
    Node_p->NodeControl.NodeCompleteNotify   = TRUE;
    Node_p->NodeControl.NodeCompletePause    = TRUE;


    FirstNode_p = Node_p;

    for(Count = 1; Count < MP3PB_NO_OF_BUFFERS; Count++)
    {
        Node_p->Next_p = Node_p+1;

        Node_p = Node_p->Next_p;

        *Node_p = *FirstNode_p;

        Node_p->SourceAddress_p = Buffer_p[Count];
    }

    Node_p->Next_p  = FirstNode_p;

    Node_p = FirstNode_p;

#ifndef  MP3PB_USE_CPU_INJECTION
    FDMATransferParams.ChannelId             = STFDMA_USE_ANY_CHANNEL;
    FDMATransferParams.CallbackFunction      = MP3PBi_FDMACallback; /* Non Blocking mode */
    FDMATransferParams.NodeAddress_p         = Node_p;
    FDMATransferParams.BlockingTimeout       = 0;
    FDMATransferParams.ApplicationData_p     = MP3PBi_Handle_p;
    FDMATransferParams.FDMABlock             = STFDMA_1;
#endif

    while(MP3PBi_Handle_p->PlayTaskState == TASK_RUNNING)
    {
#ifdef  MP3PB_USE_CPU_INJECTION
        Node_pp = message_claim(MP3PBi_Handle_p->InjectMsgQ_p);

        if(MP3PBi_Handle_p->PlayTaskState == TASK_STOPPED)
            break;
#endif

        BytesRead = vfs_read(MP3PBi_Handle_p->FileHandle, Node_p->SourceAddress_p, MP3PB_BUFFER_SIZE);
        if(BytesRead < 0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : Error file read %08X\n", MP3PBi_Handle_p->FileHandle));

            ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_ERROR_ID], &MP3PBi_Handle_p);
            if(ErrCode != ST_NO_ERROR)
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify = %08X\n", ErrCode));
            continue;
        }

        if(BytesRead < MP3PB_BUFFER_SIZE)
        {
            Node_p->NumberBytes = BytesRead;
            Node_p->Length      = BytesRead;
            Node_p->Next_p      = NULL;
        }

#ifdef  MP3PB_USE_CPU_INJECTION
        *Node_pp = Node_p;
        message_send(MP3PBi_Handle_p->InjectMsgQ_p, Node_pp);
#else

        /* wait for NODE and BUFFER to be free */
        semaphore_wait(MP3PBi_Handle_p->FDMACallbackSem_p);

        /* Workaround
         * When Injecting AUDIO data into CDFIFO through FDMA, data in audio BitBuffer is getting overwritten
         * This problem occurs only when VIDEO is also running, Still not sure about the exact problem, since
         * FDMA writes data with AUDIO CDREQ and this should not differ when enabling VIDEO
         * The below code checks if there in enough space in AUDIO BB before injecting data.
         */
        while(MP3PBi_Handle_p->PlayTaskState == TASK_RUNNING)
        {
            ErrCode = STAUD_IPGetBitBufferFreeSize(MP3PBi_Handle_p->AUDHandle, STAUD_OBJECT_INPUT_CD1, &AudBufFreeSize);
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STAUD_IPGetBitBufferFreeSize()=%08X\n", ErrCode));

                ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_ERROR_ID], &MP3PBi_Handle_p);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify()=%08X\n", ErrCode));
                break;
            }

            if(AudBufFreeSize < Node_p->NumberBytes)
                task_delay(ST_GetClocksPerSecond()/100);   /*10ms*/
            else
                break;
        }

        if(MP3PBi_Handle_p->PlayTaskState == TASK_STOPPED)
            break;

        if(FDMAStarted == FALSE)
        {
            ErrCode = STFDMA_StartTransfer(&FDMATransferParams, &FDMATransferId);
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STFDMA_StartTransfer()=%08X\n", ErrCode));

                ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_ERROR_ID], &MP3PBi_Handle_p);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify()=%08X\n", ErrCode));
                break;
            }

            FDMAStarted = TRUE;
        }
        else
        {
            ErrCode = STFDMA_ResumeTransfer(FDMATransferId);
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STFDMA_ResumeTransfer()=%08X\n", ErrCode));

                ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_ERROR_ID], &MP3PBi_Handle_p);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify()=%08X\n", ErrCode));
                break;
            }
        }
#endif

        if(BytesRead < MP3PB_BUFFER_SIZE)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "MP3PBi_PlayTask : MP3 play finished for file %08X\n",
                    MP3PBi_Handle_p->FileHandle));

            ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_END_OF_FILE_ID], &MP3PBi_Handle_p);
            if(ErrCode != ST_NO_ERROR)
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify = %08X\n", ErrCode));

#ifndef  MP3PB_USE_CPU_INJECTION
            /* wait till all the Nodes are completed */
            do
            {
                semaphore_wait(MP3PBi_Handle_p->FDMACallbackSem_p);
            }while((MP3PBi_Handle_p->CallbackReason != STFDMA_NOTIFY_TRANSFER_COMPLETE) &&
                   (MP3PBi_Handle_p->PlayTaskState == TASK_RUNNING));
#endif

            /* wait for completion of playback */
            if(MP3PBi_Handle_p->PlayTaskState == TASK_RUNNING)
            {
                AudNbFrames = 0;
                NbFramesBeforeEOP = MP3PB_NB_FRAMES_BEFORE_EOP;

                while(NbFramesBeforeEOP--)
                {
                    if(AudNbFrames != MP3PBi_Handle_p->AudNbFrames)
                    {
                        AudNbFrames = MP3PBi_Handle_p->AudNbFrames;
                        NbFramesBeforeEOP = MP3PB_NB_FRAMES_BEFORE_EOP;
                    }
                    task_delay(ST_GetClocksPerSecond()/100); /* 10 ms */
                }
            }
            break;
        }

        Node_p = Node_p->Next_p;
    }

    if(MP3PBi_Handle_p->PlayTaskState == TASK_STOPPED)  /* Abort the FDMA transfer when stoppped manually */
    {
        ErrCode = STFDMA_AbortTransfer(FDMATransferId);
        if (ErrCode != ST_NO_ERROR)
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STFDMA_AbortTransfer()=%08X\n", ErrCode));
    }
    else  /* notify END of PLAYBACK if not stopped manually */
    {
        ErrCode = STEVT_Notify(MP3PBi_Params.EVTHandle, MP3PBi_Params.EventIDs[MP3PB_EVT_END_OF_PLAYBACK_ID], &MP3PBi_Handle_p);
        if(ErrCode != ST_NO_ERROR)
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MP3PBi_PlayTask : STEVT_Notify = %08X\n", ErrCode));
    }

    memory_deallocate(MP3PBi_Params.NCachePartition, Temp_p);
    memory_deallocate(MP3PBi_Params.NCachePartition, BufferAlloc_p);
}


#ifdef  MP3PB_USE_CPU_INJECTION
/*-------------------------------------------------------------------------
 * Function : MP3PBi_InjectTask
 *            Function is called as subtask
 * Input    : Loop_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void MP3PBi_InjectTask(MP3PBi_Handle_t *MP3PBi_Handle_p)
{
    STFDMA_Node_t     **Node_pp;
    U32                *Buffer_p,
                        Size;

    while((MP3PBi_Handle_p->PlayTaskState == TASK_RUNNING))
    {
        Node_pp = message_receive(MP3PBi_Handle_p->InjectMsgQ_p);
        if(Node_pp == NULL) /* task kill */
            break;

        Buffer_p = (*Node_pp)->SourceAddress_p;
        Size     = (*Node_pp)->NumberBytes/4;

        while((MP3PBi_Handle_p->PlayTaskState == TASK_RUNNING) && (Size--))
        {
            STSYS_WriteRegDev32LE(AUDIO_CD_FIFO, *Buffer_p++);

            while(STSYS_ReadRegDev32LE(AUDIO_IF_BASE_ADDRESS+0x40)&(1<<13))   /*BBF*/
                task_delay(ST_GetClocksPerSecond()/1000);   /*1ms*/
        }

        message_release(MP3PBi_Handle_p->InjectMsgQ_p, Node_pp);
    }
}
#endif


/* EOF --------------------------------------------------------------------- */

