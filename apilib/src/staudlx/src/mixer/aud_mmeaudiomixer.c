/********************************************************************************
 *   Filename       : aud_mmeaudiomixer.c                                           *
 *   Start              : 02.09.2005                                                                            *
 *   By                 : Chandandeep Singh Pabla                                           *
 *   Contact        : chandandeep.pabla@st.com                                      *
 *   Description    : The file contains the MME based Audio Mixer APIs that will be             *
 *                Exported to the Modules. It will also have Mixer task to perform actual       *
 *                Mixing                                                            *
 ********************************************************************************
 */

/* {{{ Includes */
//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include <string.h>
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_mmeaudiomixer.h"
#include "aud_fademap.h"
/* }}} */

ST_DeviceName_t Mixer_DeviceName[] = {"MIX0"};

/* {{{ Testing Params */

#define MIXER_INPUT_FROM_FILE   0
#if MIXER_INPUT_FROM_FILE
    #define     NO_FRAMES   10
    #define     MPEG        1152
    #define     AC3     1536
    #define BYTES_PER_SAMPLE    4
    #define     FSIZE   AC3*BYTES_PER_SAMPLE*10 // allocate for 10 channels

    char InputBuf[FSIZE*NO_FRAMES]; /* NO_FRAMES of MP2 audio */
#endif

#define DUMP_OUTPUTTO_FILE  0
#if DUMP_OUTPUTTO_FILE
    #define FRAMES_MIX  150
    char OutBuf1[1536*4*2*FRAMES_MIX]; /* FRAMES_MIX of AC3 audio */
#endif

#define DUMP_PTS_DIFFERENCE 0

#define PTS_TICKS_PER_MSEC 90

#define AD_PAUSE_THRESHOLD 50
#define AD_SKIP_THRESHOLD  50
#define AD_PAUSE_LIMIT     200
#define AD_SKIP_LIMIT      200


/* }}} */

/* {{{ Global Variables */

/* Initialized Mixer list. New Mixer will be added to this list on initialization. */
MixerControlBlockList_t *MixerQueueHead_p = NULL;

static ST_ErrorCode_t MixerReleaseMemory(MixerControlBlockList_t * ControlBlock_p);

/* }}} */

/******************************************************************************
 *  Function name   : STAud_MixerInit
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will create the Mixer task, initialize MME decoders's In/out
 *                paramaters and will update main structure with current values
 ***************************************************************************** */
ST_ErrorCode_t  STAud_MixerInit(const ST_DeviceName_t DeviceName,
                                                const STAud_MixInitParams_t *InitParams_p)
{
    /* Local variables */
    ST_ErrorCode_t          error = ST_NO_ERROR;
    MixerControlBlockList_t     *MixerCtrlBlockLst_p = NULL;
    partition_t                 *DriverPartition = NULL;
    MixerControlBlock_t     *MixerCtrlBlock_p;
    U32                     i;
    MixerStreamsInfo_t      *MixerStreamInfo_p;

    /* Check for Maximum number of inputs */
    if(InitParams_p->NoOfRegisteredInput > ACC_MIXER_MAX_NB_INPUT)
    {
        /* More number of streams at input than the Mixer can handle */
        return ST_ERROR_BAD_PARAMETER;
    }

    /* First Check if this Mixer is not already open */
    if(!STAud_MixerIsInit(DeviceName))
    {
        /* Mixer not already initialized. So do the initialization */

        /* Allocate memory for new Mixer control block */
        DriverPartition = (partition_t *)InitParams_p->DriverPartition;

        MixerCtrlBlockLst_p = (MixerControlBlockList_t *)memory_allocate(
                                                DriverPartition,
                                                sizeof(MixerControlBlockList_t));

        if(MixerCtrlBlockLst_p == NULL)
        {
            /* Error No memory available */
            return ST_ERROR_NO_MEMORY;
        }

        memset(MixerCtrlBlockLst_p,0,sizeof(MixerControlBlockList_t));

        /* Initialize MME Structure */
        MixerCtrlBlock_p = &MixerCtrlBlockLst_p->MixerControlBlock;

        /* Fill in the details of our new control block */
        MixerCtrlBlock_p->Handle    = 0;

        strncpy(MixerCtrlBlock_p->DeviceName, DeviceName, ST_MAX_DEVICE_NAME_TOCOPY);

        strncpy(MixerCtrlBlock_p->EvtGeneratorName, InitParams_p->EvtGeneratorName,ST_MAX_DEVICE_NAME_TOCOPY);
        MixerCtrlBlock_p->ObjectID = InitParams_p->ObjectID;
        MixerCtrlBlock_p->AVMEMPartition = InitParams_p->AVMEMPartition;
        MixerCtrlBlock_p->DriverPartition = InitParams_p->DriverPartition;

        MixerCtrlBlock_p->MMEMixerinitParams.Priority = MME_PRIORITY_NORMAL;
        MixerCtrlBlock_p->MMEMixerinitParams.StructSize = sizeof (MME_TransformerInitParams_t);
        MixerCtrlBlock_p->MMEMixerinitParams.Callback = &Mixer_TransformerCallback;
        MixerCtrlBlock_p->MMEMixerinitParams.CallbackUserData = NULL; /* To be Initialized in STAud_MixerStart */
        MixerCtrlBlock_p->MMEMixerinitParams.TransformerInitParamsSize = sizeof(MME_LxMixerTransformerInitParams_t);
        MixerCtrlBlock_p->MMEMixerinitParams.TransformerInitParams_p = memory_allocate(DriverPartition,sizeof(MME_LxMixerTransformerInitParams_t));

        if (MixerCtrlBlock_p->MMEMixerinitParams.TransformerInitParams_p)
        {
            MME_LxMixerTransformerInitParams_t  *Mixer_Init = (MME_LxMixerTransformerInitParams_t   *)MixerCtrlBlock_p->MMEMixerinitParams.TransformerInitParams_p;
            memset(MixerCtrlBlock_p->MMEMixerinitParams.TransformerInitParams_p,0,sizeof(MME_LxMixerTransformerInitParams_t));
            STAud_SetMixerPCMParams(&(Mixer_Init->GlobalParams.PcmParams));
        }
        else
        {
            /*No Memory*/
            error = MixerReleaseMemory(MixerCtrlBlockLst_p);
            return ST_ERROR_NO_MEMORY;
        }

        MixerStreamInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);

        /*Create the information of I/P and O/P */
        MixerStreamInfo_p->NoOfRegisteredInput = InitParams_p->NoOfRegisteredInput;

        /* Initialize the Output stream info */
        {
            MixerOutputStreamNode_t *OPStreamParams_p = &(MixerStreamInfo_p->OutputStreamParams);

            OPStreamParams_p->OutBufHandle          = InitParams_p->OutBMHandle;
            OPStreamParams_p->outputChannels            = InitParams_p->NumChannels;
            OPStreamParams_p->outputSampleRate      = MIXER_OUT_FREQ;
            OPStreamParams_p->outputBytesPerSample = 4; // Usually fixed. Of no use now
            OPStreamParams_p->outputSamplesPerTransform = 0;
            /* Output Stream BM */
            /* Get output Buffer Manager Handles */
            OPStreamParams_p->OutBufHandle  = InitParams_p->OutBMHandle;
            /* Register at start */

            /* Calculate number of samples per transform(based on the frame size we define in ms) */
            OPStreamParams_p->outputSamplesPerTransform = (OPStreamParams_p->outputSampleRate * MSEC_PER_MIXER_TRANSFORM)/1000;
            // Converting number of samples to even
            OPStreamParams_p->outputSamplesPerTransform = (OPStreamParams_p->outputSamplesPerTransform/2)*2;

        }
        /* Initialize the Input stream info */
        for(i = 0; i < MixerStreamInfo_p->NoOfRegisteredInput ; i++)
        {
            MixerInputStreamNode_t  *IPStreamParams_p = &(MixerStreamInfo_p->InputStreamsParams[i]);

            IPStreamParams_p->numChannels = InitParams_p->NumChannels;
            IPStreamParams_p->sampleRate = MIXER_OUT_FREQ;
#ifndef ST_51XX
            IPStreamParams_p->bytesPerSample = 4 ;
#else
            IPStreamParams_p->bytesPerSample = 2 ;
#endif
            IPStreamParams_p->RegisteredAtMixer = TRUE;

            if(i == 0)
            {
                IPStreamParams_p->StreamWithValidPTS = TRUE;
            }
            else
            {
                IPStreamParams_p->StreamWithValidPTS = FALSE;
            }

            /* Get input Buffer Manager Handles */
            IPStreamParams_p->InMemoryBlockHandle = InitParams_p->InBMHandle[i];
            /* Register to memory block manager as a consumer at start */

            /* Initialize the Input stream MixerLevel */
            IPStreamParams_p->MixLevel       = 0x7FFF;
            IPStreamParams_p->NewMixLevel    = 0x7FFF;
            IPStreamParams_p->bytesPerMSec =
                                            ((IPStreamParams_p->bytesPerSample * IPStreamParams_p->numChannels) *
                                                    (IPStreamParams_p->sampleRate))/1000;

         // For all inputs curFade is at lowest attenuation
         //if (i == MixerCtrlBlock_p->MasterInput)
         //    IPStreamParams_p->FadeInfo.curFade       = 0x7FFF;
         //else
            IPStreamParams_p->FadeInfo.curFade          = 0x0;

            IPStreamParams_p->FadeInfo.lastReceivedFade = 0x0;
            IPStreamParams_p->UserProvidedLevel         = FALSE;

        }

        /* Create the Mixer Task Semaphore */
        MixerCtrlBlock_p->MixerTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);

        /* Create the Control Block Mutex */
        MixerCtrlBlock_p->LockControlBlockMutex_p = STOS_MutexCreateFifo();

        /* Create the Command Competion Semaphore */
        MixerCtrlBlock_p->CmdCompletedSem_p = STOS_SemaphoreCreateFifo(NULL,0);

        /* Create the Callback Competion Semaphore */
        MixerCtrlBlock_p->LxCallbackSem_p = STOS_SemaphoreCreateFifo(NULL,0);

        /* Initialize and Allocate Mixer Queue's buffer pointer */
        error |=STAud_MixerInitInputQueueParams(MixerCtrlBlock_p, InitParams_p->AVMEMPartition);
        if(error != ST_NO_ERROR)
        {
            error |= MixerReleaseMemory(MixerCtrlBlockLst_p);
            return error;
        }

        error |=STAud_MixerInitOutputQueueParams(MixerCtrlBlock_p, InitParams_p->AVMEMPartition);
        if(error != ST_NO_ERROR)
        {
            error |= MixerReleaseMemory(MixerCtrlBlockLst_p);
            return error;
        }

        STAud_MixerInitFrameParams(MixerCtrlBlock_p);

        /* Set the Mixer Starting State */
        MixerCtrlBlock_p->CurMixerState = MIXER_STATE_IDLE;
        MixerCtrlBlock_p->NextMixerState = MIXER_STATE_IDLE;

#if VARIABLE_MIX_OUT_FREQ
        MixerCtrlBlock_p->OpFreqChange = FALSE;
#endif

        error |= STAud_MixerRegEvt(InitParams_p->EvtHandlerName,MixerCtrlBlock_p->EvtGeneratorName,MixerCtrlBlock_p);
        if(error != ST_NO_ERROR)
        {
            error |= MixerReleaseMemory(MixerCtrlBlockLst_p);
            return error;
        }


        /* As Consumer of incoming data */
        MixerCtrlBlock_p->consumerParam_s.Handle = (U32)MixerCtrlBlock_p;
        MixerCtrlBlock_p->consumerParam_s.sem    = MixerCtrlBlock_p->MixerTaskSem_p;


        /* Register As Producer of outgoing data */
        MixerCtrlBlock_p->ProducerParam_s.Handle = (U32)MixerCtrlBlock_p;
        MixerCtrlBlock_p->ProducerParam_s.sem    = MixerCtrlBlock_p->MixerTaskSem_p;

        /* Initialize the mixer Capability structure */
        STAud_InitMixerCapability(MixerCtrlBlock_p);

        MixerCtrlBlock_p->CompleteBuffer = FALSE;

        /*Set the Master Input default to MASTER_STREAM_ID*/
        MixerCtrlBlock_p->MasterInput = MASTER_STREAM_ID;

        /*Mixer by default will be free running*/
        MixerCtrlBlock_p->FreeRun       = FALSE;

        MixerCtrlBlock_p->UpdateFadeOnMaster = FALSE;

        /* Create the Mixer Task */
#if defined(ST_OS20)
        STOS_TaskCreate   (STAud_MixerTask_Entry_Point,(void *)MixerCtrlBlock_p,MixerCtrlBlock_p->DriverPartition,
                            AUDIO_MIXER_TASK_STACK_SIZE,(void **)&MixerCtrlBlock_p->MixerTaskstack,MixerCtrlBlock_p->DriverPartition,&MixerCtrlBlock_p->MixerTask,&MixerCtrlBlock_p->MixerTaskDesc,
                            AUDIO_MIXER_TASK_PRIORITY,DeviceName, 0);

#else
        STOS_TaskCreate   (STAud_MixerTask_Entry_Point,(void *)MixerCtrlBlock_p,NULL,
                            AUDIO_MIXER_TASK_STACK_SIZE,NULL,NULL,&MixerCtrlBlock_p->MixerTask,NULL,
                            AUDIO_MIXER_TASK_PRIORITY,DeviceName, 0);
#endif
        if (MixerCtrlBlock_p->MixerTask == NULL)
        {
           /* Error */
           error = MixerReleaseMemory(MixerCtrlBlockLst_p);
           return ST_ERROR_NO_MEMORY;
        }

        /* Update the intitialized Decoder queue with current control block */
        STAud_MixerQueueAdd(MixerCtrlBlockLst_p);
    }
    else
    {
        /* Mixer already initialized. */
        error = ST_ERROR_ALREADY_INITIALIZED;
    }

#if MIXER_INPUT_FROM_FILE
    FILE *fp = NULL;
    U32 size;
    //fp = fopen("c:\\sine48K32bit.pcm","rb");
    fp = fopen("c:\\Sine48k32bitpcm.pcm","rb");
    fseek (fp,FSIZE*6,SEEK_SET);
    size=fread(InputBuf,1,FSIZE*NO_FRAMES,fp);
#endif

    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_MixerInit: Failed to Init Successfully\n "));
    }
    else
    {
        STTBX_Print(("STAud_MixerInit: Successful \n "));
    }

    return error;
}

/******************************************************************************
 *  Function name   : MixerTerm
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will terminate the Mixer task, Free MME Mixer's In/out
 *                paramaters and  update main structure with current values
 ***************************************************************************** */

ST_ErrorCode_t  MixerTerm(MixerControlBlockList_t * Mixer_p)
{
    /* Local Variable */
    ST_ErrorCode_t              error = ST_NO_ERROR;
    MixerControlBlock_t         *ControlBlock_p;
    partition_t                 *DriverPartition = NULL;

    ControlBlock_p = &Mixer_p->MixerControlBlock;
    DriverPartition = ControlBlock_p->DriverPartition;

    /* Free Decoder queue Structures */
    error |= STAud_MixerDeInitInputQueueParams(ControlBlock_p);
    error |= STAud_MixerDeInitOutputQueueParams(ControlBlock_p);

    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_MixerTerm: Failed to deallocate queue\n "));
    }

    /*Unsubscrive the events*/
    error |= STAud_MixerUnSubscribeEvt(ControlBlock_p);

    /* Free the MME Param Structure */
    memory_deallocate(DriverPartition, ControlBlock_p->MMEMixerinitParams.TransformerInitParams_p);

    /* Terminate Mixer Semaphore */
    STOS_SemaphoreDelete(NULL,ControlBlock_p->MixerTaskSem_p);

    /* Delete Mutex */
    STOS_MutexDelete(ControlBlock_p->LockControlBlockMutex_p);

    /* Terminate Cmd Completion Semaphore */
    STOS_SemaphoreDelete(NULL,ControlBlock_p->CmdCompletedSem_p);

    STOS_SemaphoreDelete(NULL,ControlBlock_p->LxCallbackSem_p);

    /* Remove the Control Block from Queue */
    STAud_MixerQueueRemove(Mixer_p);

    /*Free the Control Block */
    memory_deallocate(DriverPartition,Mixer_p);

    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_MixerTerm: Failed to Terminate Successfully\n "));
    }
    else
    {
        STTBX_Print(("STAud_MixerTerm: Successfully\n "));
    }
    return error;
}

/******************************************************************************
 *  Function name   : STAud_MixerOpen
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will Open the Mixer
 ***************************************************************************** */

ST_ErrorCode_t  STAud_MixerOpen(const ST_DeviceName_t DeviceName,
                                    STAud_MixerHandle_t *Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    MixerControlBlockList_t *Mixer_p;

    /* Get the desired control block */
    Mixer_p = STAud_MixerGetControlBlockFromName(DeviceName);

    if(Mixer_p == NULL)
    {
        /* Device not initialized */
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    /* Check nobody else has already opened it */
        if (Mixer_p->MixerControlBlock.Handle == 0)
       {
        /* Set and return Handle */
        Mixer_p->MixerControlBlock.Handle = (STAud_MixerHandle_t)Mixer_p;
        *Handle = Mixer_p->MixerControlBlock.Handle;
    }

    STTBX_Print(("STAud_MixerOpen: Successful \n "));
    return error;
}


/******************************************************************************
 *  Function name   : STAud_MixerClose
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will close the task
 ***************************************************************************** */
ST_ErrorCode_t  STAud_MixerClose()
{
    return ST_NO_ERROR;
}


/******************************************************************************
 *  Function name : MixerCheckStateTransition
 *  Arguments       :
 *       IN         :
 *       OUT        : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description   : This function will check the sanity of the current issued command
 *
 ***************************************************************************** */

ST_ErrorCode_t      MixerCheckStateTransition(MixerControlBlock_t * ControlBlock_p, MMEAudioMixerStates_t State)
{
    ST_ErrorCode_t Error=ST_NO_ERROR;
    MMEAudioMixerStates_t CurrentState = ControlBlock_p->CurMixerState;

    switch(State)
    {
        case MIXER_STATE_START:
            STTBX_Print(("MIXER:START cmd\n"));
            if((CurrentState != MIXER_STATE_IDLE) &&
            (CurrentState != MIXER_STATE_STOP))
            {
                Error = STAUD_ERROR_INVALID_STATE;
                STTBX_Print(("MIXER:Err cmd\n"));
            }
            break;
        case MIXER_STATE_STOP:
            STTBX_Print(("MIXER:STOP cmd\n"));
            if((CurrentState != MIXER_STATE_START) &&
            (CurrentState != MIXER_STATE_MIXING))
            {
                Error = STAUD_ERROR_INVALID_STATE;
                STTBX_Print(("MIXER:Err cmd\n"));
            }
            break;
        case MIXER_STATE_TERMINATE:
            STTBX_Print(("MIXER:TERM cmd\n"));
            if((CurrentState != MIXER_STATE_IDLE) &&
            (CurrentState != MIXER_STATE_STOP))
            {
                Error = STAUD_ERROR_INVALID_STATE;
                STTBX_Print(("MIXER:Err cmd\n"));
            }
            break;
        default:
            Error = STAUD_ERROR_INVALID_STATE;
            STTBX_Print(("MIXER:Err cmd\n"));
            break;
    }

    return Error;
}

ST_ErrorCode_t      STAud_MixerSetCmd(STAud_MixerHandle_t Handle,MMEAudioMixerStates_t State)
{
    ST_ErrorCode_t          Error=ST_NO_ERROR;
    MixerControlBlockList_t * Mixer_p= (MixerControlBlockList_t*)NULL;
    MixerControlBlock_t     * ControlBlock_p = NULL;
    task_t                  * MixerTask_p = NULL;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    ControlBlock_p = &(Mixer_p->MixerControlBlock);
    STOS_MutexLock(ControlBlock_p->LockControlBlockMutex_p);
    Error = MixerCheckStateTransition(ControlBlock_p, State);
    if( Error != ST_NO_ERROR)
    {
        /*It is an invalid request, so ignore it*/
        STOS_MutexRelease(ControlBlock_p->LockControlBlockMutex_p);
        return Error;
    }

    switch(State)
    {
        case MIXER_STATE_TERMINATE:
            MixerTask_p = ControlBlock_p->MixerTask;
        break;
        default:
            break;
    }

    /*change the next state*/
    ControlBlock_p->NextMixerState = State;


    /*Safe to change the state*/
    STOS_MutexRelease(ControlBlock_p->LockControlBlockMutex_p);

    /* Signal Mixer task Semaphore */
    STOS_SemaphoreSignal(ControlBlock_p->MixerTaskSem_p);

    if (State == MIXER_STATE_TERMINATE)
    {

        STOS_TaskWait(&MixerTask_p, TIMEOUT_INFINITY);
#if defined(ST_OS20)
        STOS_TaskDelete(MixerTask_p,ControlBlock_p->DriverPartition,ControlBlock_p->MixerTaskstack,ControlBlock_p->DriverPartition);

#else
        STOS_TaskDelete(MixerTask_p,NULL,NULL,NULL);
#endif
        Error |= MixerTerm(Mixer_p);

    }
    else
    {
        // Wait for command completion transtition
        STTBX_Print(("MIXER:Waiting for cmd completion\n"));
        STOS_SemaphoreWait(ControlBlock_p->CmdCompletedSem_p);
        STTBX_Print(("MIXER:cmd completed\n"));

        if((State == MIXER_STATE_START)&&(!ControlBlock_p->Transformer_Init))
        {
            /*Mixer Transformer uninitialized*/
            Error = STAUD_ERROR_DECODER_START;
        }

    }
    return Error;
}

/******************************************************************************
 *  Function name   : STAud_MixerGetCapability
 *  Arguments       :
 *       IN         :   mixer Handle
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Returns the mixer capabilities
 ***************************************************************************** */
ST_ErrorCode_t  STAud_MixerGetCapability(STAud_MixerHandle_t Handle,
                                            STAUD_MXCapability_t* Capability_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    MixerControlBlockList_t *Mixer_p= (MixerControlBlockList_t*)NULL;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Set Mixer Capability parameter */
    *Capability_p = Mixer_p->MixerControlBlock.MixerCapability;

    return Error;

}
/******************************************************************************
 *  Function name   : STAud_MixerUpdatePTSStatus
 *  Arguments       :
 *       IN         :   mixer Handle
 *       OUT        : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Sets the PTS handling of an Input
 ***************************************************************************** */
ST_ErrorCode_t  STAud_MixerUpdatePTSStatus(STAud_MixerHandle_t Handle,
                                            U32 InputID, BOOL PTSStatus)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    MixerControlBlockList_t *Mixer_p= (MixerControlBlockList_t*)NULL;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    STOS_MutexLock(Mixer_p->MixerControlBlock.LockControlBlockMutex_p);
    switch(Mixer_p->MixerControlBlock.CurMixerState)
    {
        case MIXER_STATE_IDLE:
        case MIXER_STATE_STOP:
            /* Set PTS status for given InputID */
            if(InputID < Mixer_p->MixerControlBlock.MixerStreamsInfo.NoOfRegisteredInput)
            {
                Mixer_p->MixerControlBlock.MixerStreamsInfo.InputStreamsParams[InputID].StreamWithValidPTS = PTSStatus;
            }
            else
            {
                /*The mixer doesn't have the requested InputID */
                Error = ST_ERROR_BAD_PARAMETER;
            }
            break;
        default:
            /*We can't change the PTS Status in running condition*/
            Error = STAUD_ERROR_INVALID_STATE;
            break;
    }
    STOS_MutexRelease(Mixer_p->MixerControlBlock.LockControlBlockMutex_p);



    return Error;

}

/******************************************************************************
 *  Function name : STAud_MixerUpdateMaster
 *  Arguments       :
 *       IN         :   mixer Handle
 *                      :  InputID
 *       OUT        : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description   : Sets which Input is the master
 ***************************************************************************** */
ST_ErrorCode_t STAud_MixerUpdateMaster(STAud_MixerHandle_t Handle, U32 InputID, BOOL FreeRun)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    MixerControlBlockList_t *Mixer_p= (MixerControlBlockList_t*)NULL;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    STOS_MutexLock(Mixer_p->MixerControlBlock.LockControlBlockMutex_p);
    switch(Mixer_p->MixerControlBlock.CurMixerState)
    {
        case MIXER_STATE_IDLE:
        case MIXER_STATE_STOP:
            /*We can change the master here*/
            if(InputID < Mixer_p->MixerControlBlock.MixerStreamsInfo.NoOfRegisteredInput)
            {
                Mixer_p->MixerControlBlock.MasterInput = InputID;
                Mixer_p->MixerControlBlock.FreeRun = FreeRun;
            }
            else
            {
                /*The mixer doesn't have the requested InputID */
                Error = ST_ERROR_BAD_PARAMETER;
            }
            break;
        default:
            /*We can't change the master in running condition*/
            Error = STAUD_ERROR_INVALID_STATE;
            break;
    }
    STOS_MutexRelease(Mixer_p->MixerControlBlock.LockControlBlockMutex_p);
    return Error;
}

/******************************************************************************
*                       MIXER TASK                                          *
******************************************************************************/
/******************************************************************************
 *  Function name   : STAud_MixerTask_Entry_Point
 *  Arguments       :
 *       IN         :
 *       OUT            : void
 *       INOUT      :
 *  Return          : void
 *  Description     : This function is the entry point for Mixer task. Ihe Mixer task will be in
 *                     different states depending of the command received from upper/lower layers
 ***************************************************************************** */
void    STAud_MixerTask_Entry_Point(void* PCMMixerCtrlBlock_p)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    MME_ERROR   Status;

    /* Get the Decoder control Block for this task */
    MixerControlBlock_t *ControlBlock_p = (MixerControlBlock_t*)PCMMixerCtrlBlock_p;

    STOS_TaskEnter(PCMMixerCtrlBlock_p);
    while(1)
    {

        /* Wait on sema for start command */
        STOS_SemaphoreWait(ControlBlock_p->MixerTaskSem_p);

        switch(ControlBlock_p->CurMixerState)
        {

            case MIXER_STATE_IDLE:
                STOS_MutexLock(ControlBlock_p->LockControlBlockMutex_p);
                if(ControlBlock_p->CurMixerState != ControlBlock_p->NextMixerState)
                {
                    /* Mixer state need to be changed */
                    ControlBlock_p->CurMixerState = ControlBlock_p->NextMixerState;
                    STOS_SemaphoreSignal(ControlBlock_p->MixerTaskSem_p);
                    // Signal the command completion transtition
                    // Signal only when we have to go to stop directly from here
                    if(ControlBlock_p->NextMixerState == MIXER_STATE_STOP)
                    {
                        STOS_SemaphoreSignal(ControlBlock_p->CmdCompletedSem_p);
                    }
                }
                STOS_MutexRelease(ControlBlock_p->LockControlBlockMutex_p);
                break;

            case MIXER_STATE_START:

                Status = STAud_MixerSetAndInit(ControlBlock_p);
                if(Status != MME_SUCCESS)
                {
                    STTBX_Print(("Audio Mixer:Init failure\n"));
                    /* mixer UnInitialized*/
                    ControlBlock_p->CurMixerState = MIXER_STATE_IDLE;
                    ControlBlock_p->NextMixerState = MIXER_STATE_IDLE;
                }
                else
                {
                    /* mixer Initialized*/
                    ControlBlock_p->CurMixerState = MIXER_STATE_MIXING;
                    STOS_SemaphoreSignal(ControlBlock_p->MixerTaskSem_p);
                }

                // command completion of the MIXER_STATE_START command
                STOS_SemaphoreSignal(ControlBlock_p->CmdCompletedSem_p);
                break;

            case MIXER_STATE_MIXING:

                /* Process incoming Data */
                Error = STAud_MixerMixFrame(ControlBlock_p);
                if(Error == ST_NO_ERROR)
                {
                    //STTBX_Print(("Audio Mixer:Mix Error\n"));
                }

                STOS_MutexLock(ControlBlock_p->LockControlBlockMutex_p);
                if(ControlBlock_p->NextMixerState == MIXER_STATE_STOP)
                {
                    /* Mixer state need to be changed */
                    ControlBlock_p->CurMixerState = ControlBlock_p->NextMixerState;
                    STAud_StopMixer(ControlBlock_p);
                    STOS_SemaphoreSignal(ControlBlock_p->MixerTaskSem_p);
                    // Signal the command completion transtition
                    STOS_SemaphoreSignal(ControlBlock_p->CmdCompletedSem_p);
                }
                STOS_MutexRelease(ControlBlock_p->LockControlBlockMutex_p);

                break;
            case MIXER_STATE_STOP:

                STOS_MutexLock(ControlBlock_p->LockControlBlockMutex_p);
                if(ControlBlock_p->NextMixerState == MIXER_STATE_START)
                {
                    /* Mixer state need to be changed */
                    ControlBlock_p->CurMixerState = ControlBlock_p->NextMixerState;
                    STOS_SemaphoreSignal(ControlBlock_p->MixerTaskSem_p);
                    // Signal the command completion transtition
                    // triggered from the init transform
                    //STOS_SemaphoreSignal(ControlBlock_p->CmdCompletedSem_p);
                }
                else if(ControlBlock_p->NextMixerState == MIXER_STATE_TERMINATE)
                    {
                        /* Mixer state need to be changed */
                        ControlBlock_p->CurMixerState = ControlBlock_p->NextMixerState;
                        STOS_SemaphoreSignal(ControlBlock_p->MixerTaskSem_p);
                    }
                STOS_MutexRelease(ControlBlock_p->LockControlBlockMutex_p);
                break;

            case MIXER_STATE_TERMINATE:
                STOS_TaskExit(PCMMixerCtrlBlock_p);
#if defined( ST_OSLINUX )
                return ;
#else
                task_exit(1);
#endif
                break;
            default:
                break;

        }

    }

}


/******************************************************************************
*                       LOCAL FUNCTIONS                                        *
******************************************************************************/
MixerInStreamBlocklist_t *STAud_AllocateListElement(MixerControlBlock_t *MixerCtrlBlock_p)
{
    MixerInStreamBlocklist_t *Temp;

    Temp = (MixerInStreamBlocklist_t *)memory_allocate(
                                    MixerCtrlBlock_p->DriverPartition,
                                    sizeof(MixerInStreamBlocklist_t));

    if(Temp != NULL)
    {
        memset(Temp,0,sizeof(MixerInStreamBlocklist_t));
        Temp->Next_p = (MixerInStreamBlocklist_t *)NULL;
        Temp->BlockUsed = FALSE;
        Temp->StartOffset  = 0;
        Temp->BytesConsumed = 0;
    }

    return Temp;
}
void STAud_AddListElement(MixerInStreamBlocklist_t *TempStreamBuf,MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID, BOOL BlockUsed)
{
    MixerInStreamBlocklist_t * temp = MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID].InBlockStarList_p;

    TempStreamBuf->BlockUsed = BlockUsed;

    if(temp == NULL)
    {
        MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID].InBlockStarList_p = TempStreamBuf;
    }
    else
    {
        while(temp->Next_p != NULL)
        {
            temp = temp->Next_p;
        }

        /* Add node here */
        temp->Next_p = TempStreamBuf;

    }

}

void STAud_FreebufferList(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID)
{
    MixerInStreamBlocklist_t *Temp = NULL,*next;
    ST_ErrorCode_t  status = ST_NO_ERROR;
    MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);


    Temp = InputStreamsParams_p->InBlockStarList_p;

    while(Temp != NULL)
    {
        /* Free the IP Blcok */
        status = MemPool_FreeIpBlk( InputStreamsParams_p->InMemoryBlockHandle,(U32 *)&Temp->InputBlock,
                    MixerCtrlBlock_p->Handle);
        if(status != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_FreebufferList::MemPool_FreeIpBlk Failed\n"));
        }

        /*Notify consumption of  a PCM buffer*/
        /*We report for all inputs*/
        status |= STAudEVT_Notify(MixerCtrlBlock_p->EvtHandle, MixerCtrlBlock_p->EventIDPCMBufferComplete, (void *)&InputID, sizeof(InputID), MixerCtrlBlock_p->ObjectID);

        next = Temp->Next_p;
        memory_deallocate(MixerCtrlBlock_p->DriverPartition, Temp);
        Temp = next;

    }
    /* Full list was consumed.So mark the start pointer as NULL */
    InputStreamsParams_p->InBlockStarList_p = (MixerInStreamBlocklist_t *)NULL;

}

/******************************************************************************
 *  Function name   : STAud_StartUpCheck
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Check for first valid PTS arrival from all the incoming streams(This is ignore
 *                 the streams which has no PTS with it)
 ***************************************************************************** */
ST_ErrorCode_t STAud_StartUpCheck(MixerControlBlock_t   *MixerCtrlBlock_p)
{

    ST_ErrorCode_t              status = ST_NO_ERROR;
    U16                                 InputID = 0; /* Stream ID for input */
    MixerInStreamBlocklist_t    *TempStreamBuf;
    STAud_LpcmStreamParams_t    *StreamParams;
    MixerStreamsInfo_t          *MixerStreamsInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);
    MixerInputStreamNode_t      *InputStreamsParams_p;
//  static U32  count=0;//debug

    /* Get the current proceesing/To be Processed stream ID */
    InputID = MixerStreamsInfo_p->CurrentInputID;

    InputStreamsParams_p = &(MixerStreamsInfo_p->InputStreamsParams[InputID]);
    /* Check for FIRST valid PTS from all streams.*/
    for( ; InputID < MixerStreamsInfo_p->NoOfRegisteredInput ; )
    {
        /* If stream is supposed to have valid PTS, only then check it */
        if((InputStreamsParams_p->StreamWithValidPTS) && (InputID == MixerCtrlBlock_p->MasterInput))
        {
            /* Loop for valid PTS data */
            while(!InputStreamsParams_p->GotFirstValidPTS)
            {
                MemoryBlock_t           *IpBlkFromProducer_p;

                //  STTBX_Print(("StartUpCheck::Count %d \n",++count));
                status = MemPool_GetIpBlk(InputStreamsParams_p->InMemoryBlockHandle,
                                            (U32 *)&MixerCtrlBlock_p->CurInputBlockFromProducer,MixerCtrlBlock_p->Handle);
                if(status != ST_NO_ERROR)   /* No buffer available */
                {
                    //STTBX_Print(("STAud_StartUpCheck::MemPool_GetIpBlk Failed\n"));
                    MixerStreamsInfo_p->CurrentInputID = InputID; /* Keep Ho ld of input ID for next call. It might be reseted in Mixer Stop */
                    return ST_ERROR_NO_MEMORY;
                }

                IpBlkFromProducer_p = MixerCtrlBlock_p->CurInputBlockFromProducer;

                /* Check if the received block has valid PTS */
                if(IpBlkFromProducer_p->Flags & PTS_VALID)
                {
                    STTBX_Print(("StartUpCheck::Got PTS_VALID\n"));

                    /* Got the block with valid PTS. Add it to our list and set the flags */
                    TempStreamBuf = STAud_AllocateListElement(MixerCtrlBlock_p);
                    if(TempStreamBuf == NULL)
                    {
                        STTBX_Print(("STAud_StartUpCheck::STAud_AllocateListElement Failed\n"));
                        return ST_ERROR_MALLOC_FAILED;
                    }
                    TempStreamBuf->InputBlock = IpBlkFromProducer_p;
                    TempStreamBuf->StartOffset = 0;
                    /* Add input block to list. By Default we mark first block as Not Used block i.e. it will consumed in multiple goes.
                        but if its consumed in one go itself, then its also handled properly.
                     */
                    STAud_AddListElement(TempStreamBuf,MixerCtrlBlock_p,InputID,FALSE);

                    InputStreamsParams_p->GotFirstValidPTS = TRUE;

                    /* Set the number of bytes which are available with us for input */
                    InputStreamsParams_p->LeftOverBytes = IpBlkFromProducer_p->Size;
                    InputStreamsParams_p->BytesUsed= 0;

                    /* Set Master PTS to current PTS - 2 mixer input streams*/

                    /*extract extended PTS*/
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                    InputStreamsParams_p->ExtInPTS.BaseValue  = IpBlkFromProducer_p->Data[PTS_OFFSET];
                    InputStreamsParams_p->ExtInPTS.BaseBit32 = IpBlkFromProducer_p->Data[PTS33_OFFSET];
#endif

                    /*extended PTS to output*/
                    //MixerStreamsInfo_p->ExtOutputPTS = MixerStreamsInfo_p->ExtMasterPTS;

                    if(IpBlkFromProducer_p->Flags & FREQ_VALID)
                    {
                        /* Check for Input stream frequency change */
                        STAud_CheckInputStreamFreqChange(MixerCtrlBlock_p, InputID);
                    }

                    /*update for AppData_p update*/
                    StreamParams = (STAud_LpcmStreamParams_t*)IpBlkFromProducer_p->AppData_p;
                    if(StreamParams && (StreamParams->ChangeSet == TRUE))
                    {
                        STAud_CheckInputStreamParamsChange(MixerCtrlBlock_p, StreamParams, InputID);
                    }

                    if(InputStreamsParams_p->MixLevel != InputStreamsParams_p->NewMixLevel)
                    {
                        InputStreamsParams_p->MixLevel = InputStreamsParams_p->NewMixLevel;
                        MixerCtrlBlock_p->UpdateRequired[InputID] = TRUE;
                    }
                    if(MixerCtrlBlock_p->UpdateRequired[InputID])
                    {
                        ST_ErrorCode_t  Error = ST_NO_ERROR;
                        Error = STAud_MixerUpdateGlobalParams(MixerCtrlBlock_p, InputID);
                        if(Error != ST_NO_ERROR)
                        {
                            STTBX_Print(("STAud_CheckInputStreamFreqChange::Failure \n"));
                        }
                        MixerCtrlBlock_p->UpdateRequired[InputID] = FALSE;
                    }
                    MixerCtrlBlock_p->CurInputBlockFromProducer = NULL;

                }
                else
                {
                    /* Got the block with NO valid PTS. Drop this data */
                    status = MemPool_FreeIpBlk(InputStreamsParams_p->InMemoryBlockHandle,
                                                (U32 *)&MixerCtrlBlock_p->CurInputBlockFromProducer,
                                                MixerCtrlBlock_p->Handle);
                    if(status != ST_NO_ERROR)
                    {
                        STTBX_Print(("STAud_DecProcessOutput::MemPool_FreeIpBlk Failed\n"));
                    }
                }
            }
        }
        else
        {
            /* This registered stream doesn't have valid PTS so No need to wait here for Valid PTS */
            InputStreamsParams_p->LeftOverBytes = 0;
        }

        /* We have got the First buffer with valid ID for this stream, so check the same for next stream */
        InputID++;
        InputStreamsParams_p++;
    }

    /* reset InputID */
    MixerStreamsInfo_p->CurrentInputID /*= InputID*/ = 0;

    return ST_NO_ERROR;
}

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
void STAud_ExtrapolateExtPts(MixerControlBlock_t *MixerCtrlBlock_p , STCLKRV_ExtendedSTC_t * Pts_p, U32 BytesToExt)
{
    if(BytesToExt)
    {
        MixerInputStreamNode_t  InputParams;
        U32                         SampleToExtPol = 0;
        InputParams = MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[MixerCtrlBlock_p->MasterInput];
        SampleToExtPol = BytesToExt/(InputParams.numChannels*InputParams.bytesPerSample);

        /* Extrapolate PTS */
        SampleToExtPol = (SampleToExtPol * 90000)/InputParams.sampleRate; /*In 90 KHz ticks*/
        ADDToEPTS(Pts_p,SampleToExtPol);
    }

}
#endif

ST_ErrorCode_t STAud_GetOutputBuffer(MixerControlBlock_t    *MixerCtrlBlock_p)
{
    ST_ErrorCode_t      status = ST_NO_ERROR;
    MME_DataBuffer_t    * TempBufferOut_p;
    MemoryBlock_t       * OpBlkFromConsumer_p;
    MixerOutputStreamNode_t * OpStreamParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.OutputStreamParams);

    /* Get free output buffer */
    status = MemPool_GetOpBlk(OpStreamParams_p->OutBufHandle,
                                (U32 *)&MixerCtrlBlock_p->CurOutputBlockFromConsumer);
    if(status != ST_NO_ERROR)   /* No compressed buffer available */
    {
        //STTBX_Print(("ST Error=%d in STAud_GetOutputBuffer\n", status));
        return status;
    }

    OpBlkFromConsumer_p = MixerCtrlBlock_p->CurOutputBlockFromConsumer;
    /* Update params */
    TempBufferOut_p = MixerCtrlBlock_p->CurrentBufferToSend->MixerBufOut.BufferOut_p;
    TempBufferOut_p->ScatterPages_p->Page_p     = (void*)OpBlkFromConsumer_p->MemoryStart;

    TempBufferOut_p->ScatterPages_p->BytesUsed  = 0;
    TempBufferOut_p->ScatterPages_p->Size           = OpBlkFromConsumer_p->MaxSize;
    TempBufferOut_p->NumberOfScatterPages           = 1;
    TempBufferOut_p->TotalSize                          = OpBlkFromConsumer_p->MaxSize;

    /* Keep the output block with control block */
    OpStreamParams_p->OutputBlockFromConsumer = OpBlkFromConsumer_p;

    return status;
}

U32 ConvertBytesToPTS(MixerInputStreamNode_t        * InputStreamsParams_p, U32 InputBytes)
{
    U32 tempPTS = 0;

    if (InputBytes)
    {
        tempPTS = (InputBytes * PTS_TICKS_PER_MSEC)/InputStreamsParams_p->bytesPerMSec;
    }

    return (tempPTS);

}

ST_ErrorCode_t CheckSyncOnInputID(MixerControlBlock_t   *MixerCtrlBlock_p, U32 InputID, U32 bytesInFront, MemoryBlock_t * CurInputBlock)
{
    MixerStreamsInfo_t          * MixerStreamInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);
    // Check for sync only of mixer is not freerunning,
    // and we have slave streams with valid PTS
    if ((!MixerCtrlBlock_p->FreeRun) &&
        (InputID != MixerCtrlBlock_p->MasterInput) &&
        (MixerStreamInfo_p->InputStreamsParams[MixerCtrlBlock_p->MasterInput].GotFirstValidPTS) &&
        (MixerStreamInfo_p->InputStreamsParams[InputID].StreamWithValidPTS) &&
        (CurInputBlock->Flags & PTS_VALID))
    {
        MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerStreamInfo_p->InputStreamsParams[InputID]);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
        STCLKRV_ExtendedSTC_t curPTS, curMasterPTS;
#endif
        U32 curPTSTicksInFront;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
        curPTS.BaseValue = CurInputBlock->Data[PTS_OFFSET];
        curPTS.BaseBit32 = CurInputBlock->Data[PTS33_OFFSET];
#endif
        // There is a possibilty that some data is present in front of this buffer
        // So subtract that from cur PTS to calculate PTS of data in front of this input
        curPTSTicksInFront = ConvertBytesToPTS(InputStreamsParams_p, bytesInFront);

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
        SubtractFromEPTS(&curPTS, curPTSTicksInFront);


        curMasterPTS = MixerStreamInfo_p->InputStreamsParams[MixerCtrlBlock_p->MasterInput].ExtInPTS;

        if (EPTS_LATER_THAN(curPTS, curMasterPTS))
        {
            U32 pauseInMsec = EPTS_DIFF(curMasterPTS, curPTS);
            //PAUSE

            // Convert to MSec
            pauseInMsec = pauseInMsec/PTS_TICKS_PER_MSEC;

            if (pauseInMsec > AD_PAUSE_THRESHOLD)
            {
                if (pauseInMsec > AD_PAUSE_LIMIT)
                {
                    pauseInMsec = AD_PAUSE_LIMIT;
                }

                InputStreamsParams_p->bytesToPause = (pauseInMsec * InputStreamsParams_p->bytesPerMSec);
                InputStreamsParams_p->bytesToSkip = 0;
            }
            else
            {
                InputStreamsParams_p->bytesToPause = 0;
                InputStreamsParams_p->bytesToSkip = 0;
            }
        }
        else
        {
            U32 skipInMsec = EPTS_DIFF(curMasterPTS, curPTS);
            // Skip

            // Convert to MSec
            skipInMsec = skipInMsec/PTS_TICKS_PER_MSEC;

            if (skipInMsec > AD_SKIP_THRESHOLD)
            {
                if (skipInMsec > AD_SKIP_LIMIT)
                {
                    skipInMsec = AD_SKIP_LIMIT;
                }

                InputStreamsParams_p->bytesToSkip = (skipInMsec * InputStreamsParams_p->bytesPerMSec);
                InputStreamsParams_p->bytesToPause = 0;
            }
            else
            {
                InputStreamsParams_p->bytesToPause = 0;
                InputStreamsParams_p->bytesToSkip = 0;
            }
        }
#endif

    }

    return (ST_NO_ERROR);
}

void MixerApplyStreamProperties(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID, MemoryBlock_t   * IpBlkFromProducer_p)
{

    STAud_LpcmStreamParams_t    * StreamParams;

    if(IpBlkFromProducer_p->Flags & FREQ_VALID)
    {
        STAud_CheckInputStreamFreqChange(MixerCtrlBlock_p, InputID);
    }
    /*update for AppData_p update*/
    StreamParams = (STAud_LpcmStreamParams_t*)IpBlkFromProducer_p->AppData_p;
    if(StreamParams && (StreamParams->ChangeSet == TRUE))
    {
        STAud_CheckInputStreamParamsChange(MixerCtrlBlock_p, StreamParams, InputID);
    }

    if (IpBlkFromProducer_p->Flags & FADE_VALID)
    {
        STAud_CheckFadeParamChange(MixerCtrlBlock_p, IpBlkFromProducer_p->Data[FADE_OFFSET], InputID);
    }
    /*check for mix level for all inputs*/
    /*
    if(InputStreamsParams_p->MixLevel != InputStreamsParams_p->NewMixLevel)
    {
        InputStreamsParams_p->MixLevel = InputStreamsParams_p->NewMixLevel;
        MixerCtrlBlock_p->UpdateRequired[InputID] = TRUE;
    }
    */

    if(MixerCtrlBlock_p->UpdateRequired[InputID])
    {
        ST_ErrorCode_t  Error = ST_NO_ERROR;
        Error = STAud_MixerUpdateGlobalParams(MixerCtrlBlock_p, InputID);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print(("STAud_CheckInputStreamFreqChange::Failure \n"));
        }
        MixerCtrlBlock_p->UpdateRequired[InputID] = FALSE;
    }

}

U32 MixerApplyPause(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID, U32 TransformInBytes, U32 TransformTargetBytes)
{
    MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);

    if ((InputID != MixerCtrlBlock_p->MasterInput) &&
        (InputStreamsParams_p->bytesToPause != 0))
    {
        if (InputStreamsParams_p->bytesToPause >= (TransformTargetBytes - TransformInBytes))
        {
            InputStreamsParams_p->bytesToPause -= (TransformTargetBytes - TransformInBytes);
            TransformInBytes = TransformTargetBytes;
        }
        else
        {
            TransformInBytes += InputStreamsParams_p->bytesToPause;
            InputStreamsParams_p->bytesToPause = 0;

        }
    }
    return (TransformInBytes);
}

ST_ErrorCode_t MixerApplySkip(MixerControlBlock_t   *MixerCtrlBlock_p, U32 InputID)
{
    MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);
    ST_ErrorCode_t                  status = ST_NO_ERROR;

    if ((InputID != MixerCtrlBlock_p->MasterInput) &&
        (InputStreamsParams_p->bytesToSkip != 0))
    {
        if (InputStreamsParams_p->bytesToSkip >= MixerCtrlBlock_p->CurInputBlockFromProducer->Size)
        {
            InputStreamsParams_p->bytesToSkip -= MixerCtrlBlock_p->CurInputBlockFromProducer->Size;
            status = MemPool_FreeIpBlk( InputStreamsParams_p->InMemoryBlockHandle,
                        (U32 *)&MixerCtrlBlock_p->CurInputBlockFromProducer,
                        MixerCtrlBlock_p->Handle);
            if(status != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_FreeIpBlocks::MemPool_FreeIpBlk Failed\n"));
            }
            MixerCtrlBlock_p->CurInputBlockFromProducer = NULL;

        }
        else
        {
            MixerCtrlBlock_p->CurInputBlockFromProducer->Size -= InputStreamsParams_p->bytesToSkip;
            InputStreamsParams_p->bytesToSkip = 0;
        }
    }

    return (status);
}

ST_ErrorCode_t MixerGetNextBlock(MixerControlBlock_t    *MixerCtrlBlock_p, U32 InputID, U32 TransformInBytes)
{
    MixerInputStreamNode_t      * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);
    ST_ErrorCode_t                  status = ST_NO_ERROR;

    MixerCtrlBlock_p->CurInputBlockFromProducer = NULL;

    while ((status == ST_NO_ERROR) && (MixerCtrlBlock_p->CurInputBlockFromProducer == NULL))
    {
        /* Get IP Block */
        status = MemPool_GetIpBlk(InputStreamsParams_p->InMemoryBlockHandle,
                (U32 *)&MixerCtrlBlock_p->CurInputBlockFromProducer, MixerCtrlBlock_p->Handle);


        if (status == ST_NO_ERROR)
        {
            // Got new block so apply stream properties
            MixerApplyStreamProperties(MixerCtrlBlock_p, InputID, MixerCtrlBlock_p->CurInputBlockFromProducer);

            // do sync calculations
            CheckSyncOnInputID(MixerCtrlBlock_p, InputID, TransformInBytes, MixerCtrlBlock_p->CurInputBlockFromProducer);

            // Apply Skip if required
            MixerApplySkip(MixerCtrlBlock_p, InputID);
        }
    }

    // if BLOCK skipped then retuirn NO MEM
    return (status);
}


ST_ErrorCode_t STAud_GetInputAndAddToTransformParams(MixerControlBlock_t    *MixerCtrlBlock_p)
{
    ST_ErrorCode_t                  status = ST_NO_ERROR;
    U32                                 InputID = 0; /* Stream ID for input */
    U32                             leftInFrame,ActualByteSent ;
    U32                             TargetBytes = 0;
    U32                             TransformInBytes = 0,PageNum = 0;
    MixerInStreamBlocklist_t    * TempStreamBuf;
    MixerStreamsInfo_t          * MixerStreamsInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);
    MixerCmdParams_t                * CurrentBufferToSend_p = MixerCtrlBlock_p->CurrentBufferToSend;
    U32                             MasterGotBytes = 0,MasterTargetBytes=0;

    /* Get the current proceesing/To be Processed stream ID */
    InputID = MixerStreamsInfo_p->CurrentInputID;


    /* Get input from all the streams */
    for( ; InputID < MixerStreamsInfo_p->NoOfRegisteredInput ; )
    {
        MixerInputStreamNode_t      * InputStreamsParams_p = &(MixerStreamsInfo_p->InputStreamsParams[InputID]);

        MME_DataBuffer_t    * BufferIn_p = CurrentBufferToSend_p->MixerBufIn[InputID].BufferIn_p;
        MixerInStreamBlocklist_t * blklst;
        // convert num samples to even
        U32 NumSamples = ((MSEC_PER_MIXER_TRANSFORM * InputStreamsParams_p->sampleRate)>>1)<<1;

        U32 SampleSize = InputStreamsParams_p->bytesPerSample * InputStreamsParams_p->numChannels;

        U32 NumSamplesRequired = MixerCtrlBlock_p->MixerQueue[MixerCtrlBlock_p->MixerQueueTail%MIXER_QUEUE_SIZE].FrameStatus.InputStreamStatus[InputID].NbInSplNextTransform;

        //STTBX_Print(("%d:Next=%d\n",InputID,MixerCtrlBlock_p->MixerQueue[0].FrameStatus.InputStreamStatus[InputID].NbInSplNextTransform));

        //update the number of samples as required by mixer
        if(NumSamplesRequired)
        {
            NumSamples = NumSamplesRequired ;
        }
        else
        {
            U32 TempNumSamples = (NumSamples/1000) * 1000;
            if(TempNumSamples < NumSamples)
            {
                NumSamples = (NumSamples/1000 + 2);
            }
            else
            {
                NumSamples = NumSamples/1000;
            }

        }

        // STTBX_Print(("%d:req=%d\n",InputID,NumSamples));

        TargetBytes = (NumSamples * SampleSize);

        TransformInBytes = InputStreamsParams_p->TransformerInBytes;
        PageNum = InputStreamsParams_p->ScatterPageNum;

        blklst = InputStreamsParams_p->InBlockStarList_p;

        {
            ST_ErrorCode_t  Error = ST_NO_ERROR;
            STOS_MutexLock(MixerCtrlBlock_p->LockControlBlockMutex_p);
            if(InputStreamsParams_p->MixLevel != InputStreamsParams_p->NewMixLevel)
            {
                InputStreamsParams_p->MixLevel = InputStreamsParams_p->NewMixLevel;
                Error = STAud_MixerUpdateGlobalParams(MixerCtrlBlock_p, InputID);
            }
            if(Error!=ST_NO_ERROR)
            {
                STTBX_Print(("Error returned from STAud_MixerUpdateGlobalParams\n"));
            }
            STOS_MutexRelease(MixerCtrlBlock_p->LockControlBlockMutex_p);
        }

//      STTBX_Print(("TransformInBytes = %d:PageNum=%d\n",TransformInBytes,PageNum));
        while( TransformInBytes != TargetBytes)
        {
            /* Check if any leftover data is there from the last buffer */
            // STTBX_Print(("%d:InBytes:%5d,Target:%5d,Left:%5d\n",InputID,TransformInBytes,TargetBytes,InputStreamsParams_p->LeftOverBytes));
            if(InputStreamsParams_p->LeftOverBytes != 0)
            {
                /* Update Transformer param */
                leftInFrame = TargetBytes -TransformInBytes;    /* These many bytes can be sent to Mixer */
                ActualByteSent = (InputStreamsParams_p->LeftOverBytes >= leftInFrame) ? leftInFrame : InputStreamsParams_p->LeftOverBytes;

                /* We have some data available with us, so send it*/
                BufferIn_p->ScatterPages_p[PageNum].Page_p = (void*)((char*)blklst->InputBlock->MemoryStart + blklst->BytesConsumed);

                BufferIn_p->ScatterPages_p[PageNum].Size = ActualByteSent;

                BufferIn_p->ScatterPages_p[PageNum].BytesUsed = 0;

                //STTBX_Print(("GotRemainBytes:%5d,BytesUsed:%5d\n",ActualByteSent,InputStreamsParams_p->BytesUsed));
                /* Check if there is still some bytes lefts in the current input buffer */
                if(ActualByteSent < InputStreamsParams_p->LeftOverBytes)
                {
                    InputStreamsParams_p->LeftOverBytes -= ActualByteSent;
                }
                else
                {
                    if (blklst->Next_p)
                    {
                        // resend next block which was not consummed at all.
                        blklst = blklst->Next_p;
                        InputStreamsParams_p->LeftOverBytes = blklst->InputBlock->Size;
                    }
                    else
                    {
                        InputStreamsParams_p->LeftOverBytes =  0;
                    }
                }

                TransformInBytes += ActualByteSent;

                PageNum++;
            }
            else
            {
                MemoryBlock_t   * IpBlkFromProducer_p;

                // Apply PAUSE
                TransformInBytes = MixerApplyPause(MixerCtrlBlock_p, InputID, TransformInBytes, TargetBytes);

                // Check if we still need bytes to complete input buffer
                if (TransformInBytes < TargetBytes)
                {

                    /* Get IP Block */
                    //status = MemPool_GetIpBlk(InputStreamsParams_p->InMemoryBlockHandle,
                    //      (U32 *)&MixerCtrlBlock_p->CurInputBlockFromProducer, MixerCtrlBlock_p->Handle);
                    status = MixerGetNextBlock(MixerCtrlBlock_p, InputID, TransformInBytes);

                    /* No buffer available */
                    if(!MixerCtrlBlock_p->FreeRun)
                    {
                        if((status != ST_NO_ERROR)  &&  (InputID == MixerCtrlBlock_p->MasterInput))
                        {
                            /* This stream is the Master input i.e. its not free running. Hence we will wait till data comes on this input */

                            InputStreamsParams_p->TransformerInBytes    = TransformInBytes;
                            InputStreamsParams_p->ScatterPageNum        = PageNum;
                            MixerStreamsInfo_p->CurrentInputID          = InputID;
                            return ST_WAITING_FOR_ALL_STREAMS_DATA;
                        }
                        else if (status != ST_NO_ERROR)
                        {
                            /* No Block available. This stream is free running i.e. not a MASTER. so we will fake the mixer with dummy input */
                            BufferIn_p->ScatterPages_p[0].Page_p        = 0;
                            BufferIn_p->ScatterPages_p[0].Size          = 0;
                            BufferIn_p->ScatterPages_p[0].BytesUsed     = 0;

                            /* Free the already queued Data for this frame*/
                            STAud_FreebufferList(MixerCtrlBlock_p,InputID);

                            /*Notify that there is an underlow on this input*/
                            /*We report for all PCM inputs*/
                            status |= STAudEVT_Notify(MixerCtrlBlock_p->EvtHandle, MixerCtrlBlock_p->EventIDPCMUnderFlow, (void *)&InputID, sizeof(InputID), MixerCtrlBlock_p->ObjectID);

                            /* fake the TransformInBytes to break while loop */
                            TransformInBytes = 0;
                            PageNum = 0;
                            break;
                        }
                    }
                    else
                    {
                        if(status != ST_NO_ERROR)
                        {
                            // STTBX_Print(("Mx Get[%d]::fail \n",InputID));
                            /*All the streams are free running. So send as much is available*/
                            InputStreamsParams_p->TransformerInBytes    = TransformInBytes;
                            InputStreamsParams_p->ScatterPageNum        = PageNum;
                            //MixerStreamsInfo_p->CurrentInputID            = InputID;

                            /*Notify that there is an underlow on this input*/
                            /*We report for all PCM inputs*/
                            status |= STAudEVT_Notify(MixerCtrlBlock_p->EvtHandle, MixerCtrlBlock_p->EventIDPCMUnderFlow, (void *)&InputID, sizeof(InputID), MixerCtrlBlock_p->ObjectID);
                            /*Check for the next input if available*/
                            break;

                        }
                        else
                        {
                            // STTBX_Print(("Mx Get[%d]::sucess \n",InputID));
                        }
                    }
                    /*Initialize the local variable*/
                    IpBlkFromProducer_p = MixerCtrlBlock_p->CurInputBlockFromProducer;

#if MIXER_INPUT_FROM_FILE
                    /* CAUTION!!! - Testing from memory- remove it imme after testing*/
                    if(IpBlkFromProducer_p->Size == 0)
                    {
                        IpBlkFromProducer_p->Size = TargetBytes;
                        memset((void*)IpBlkFromProducer_p->MemoryStart, 0, TargetBytes);

#if MIXER_INPUT_FROM_FILE
                        {
                            static U32 index;
                            char *temp;
                            temp = (InputBuf + index);
                            memcpy((void*)IpBlkFromProducer_p->MemoryStart,(void*)temp,TargetBytes);
                            //IpBlkFromProducer_p->Size = 1152;
                            index+=TargetBytes;
                            if(index == TargetBytes*NO_FRAMES)
                                index=0;
                        }
#endif

                    }
#endif
                    //MixerApplyStreamProperties(MixerCtrlBlock_p, InputID, IpBlkFromProducer_p);

                    /* Update Transformer param */
                    leftInFrame = TargetBytes -TransformInBytes;    /* These many bytes can be sent to Mixer */
                    ActualByteSent = (IpBlkFromProducer_p->Size >= leftInFrame) ? leftInFrame : IpBlkFromProducer_p->Size;

                    /* We have some data available with us, so send it */
                    BufferIn_p->ScatterPages_p[PageNum].Page_p      = (void*)IpBlkFromProducer_p->MemoryStart;
                    BufferIn_p->ScatterPages_p[PageNum].Size            = ActualByteSent ;
                    BufferIn_p->ScatterPages_p[PageNum].BytesUsed   = 0;

                    TransformInBytes += ActualByteSent;

                    PageNum++;

                    /* Add the currently received block to input block list. It will used while freeing the IP blocks */
                    TempStreamBuf = STAud_AllocateListElement(MixerCtrlBlock_p);
                    if(TempStreamBuf == NULL)
                    {
                        STTBX_Print(("STAud_GetInputAndAddToTransformParams::STAud_AllocateListElement Failed\n"));
                        return ST_ERROR_MALLOC_FAILED;
                    }

                    TempStreamBuf->InputBlock = IpBlkFromProducer_p;

                    STAud_AddListElement(TempStreamBuf,MixerCtrlBlock_p,InputID,FALSE);
                }
                else /*if (TransformInBytes < TargetBytes)*/
                {
                    /*We have more or equal data*/
                    /*So we need to break out */
                    break;
                }
            }
        }/*while( TransformInBytes != TargetBytes)*/
        /* update params */
        BufferIn_p->TotalSize = TransformInBytes;
        BufferIn_p->NumberOfScatterPages = PageNum;
        CurrentBufferToSend_p->BufferPtrs[InputID] = BufferIn_p;

        /*check if we have the required number of bytes*/
        /*We may have more bytes too*/
        if(TransformInBytes >= TargetBytes)
        {
            InputStreamsParams_p->TransformerInBytes = 0;
            InputStreamsParams_p->ScatterPageNum= 0;
            MixerCtrlBlock_p->CompleteBuffer = TRUE;
            // STTBX_Print(("Mx[%d] bufcom \n",InputID));
            //STTBX_Print(("Mx[%d] nobufcom:%d,%d \n",InputID,TransformInBytes,TargetBytes));
        }
        else
        {
            //STTBX_Print(("Mx[%d] nobufcom:%d,%d \n",InputID,TransformInBytes,TargetBytes));
            if(MixerCtrlBlock_p->MasterInput == InputID)
            {
                MasterGotBytes  = TransformInBytes;
                MasterTargetBytes   = TargetBytes;
            }
            InputStreamsParams_p->TransformerInBytes = TransformInBytes;
            InputStreamsParams_p->ScatterPageNum= PageNum;
        }

        InputID++;
        InputStreamsParams_p++;
    }

    /* Update the palce where output buffer be present */
    CurrentBufferToSend_p->OutputBufferIndex = InputID;

    /* Reset InputID */
    MixerStreamsInfo_p->CurrentInputID = 0;

    if(MixerCtrlBlock_p->FreeRun)
    {
        if(!MixerCtrlBlock_p->CompleteBuffer)
        {
            //STTBX_Print(("Mx Nocomplete \n"));
            return ST_WAITING_FOR_ALL_STREAMS_DATA;
        }
        else
        {
            U32 i;
            MixerInputStreamNode_t      * InputStreamsParams_p;

            for(i=0 ; i < MixerStreamsInfo_p->NoOfRegisteredInput ; i++)
            {

                InputStreamsParams_p = &(MixerStreamsInfo_p->InputStreamsParams[i]);
                InputStreamsParams_p->TransformerInBytes = 0;
                InputStreamsParams_p->ScatterPageNum = 0;
            }

            MixerCtrlBlock_p->CompleteBuffer = FALSE;
        }
    }

    //STTBX_Print(("Mx complete \n"));
    /* Update queue tail */
    MixerCtrlBlock_p->MixerQueueTail++;

    if(MasterGotBytes != MasterTargetBytes)
    {
        //starvation on master
        MixerStreamsInfo_p->InputStreamsParams[MixerCtrlBlock_p->MasterInput].GotFirstValidPTS = FALSE;
        //InputStreamsParams_p->GotFirstValidPTS = FALSE;
        // STTBX_Print(("Mx[%d] GotPTS FALSE \n",InputID));
    }
    return ST_NO_ERROR;

}

ST_ErrorCode_t STAud_UpdateMixerTransformParams(MixerControlBlock_t *MixerCtrlBlock_p)
{

    U16 OutputOffset;
    ST_ErrorCode_t status = ST_NO_ERROR;

    OutputOffset = MixerCtrlBlock_p->CurrentBufferToSend->OutputBufferIndex ;

    MixerCtrlBlock_p->CurrentBufferToSend->BufferPtrs[OutputOffset] = MixerCtrlBlock_p->CurrentBufferToSend->MixerBufOut.BufferOut_p;

    status = acc_setAudioMixerTransformCmd(&MixerCtrlBlock_p->CurrentBufferToSend->Cmd,
                                        &MixerCtrlBlock_p->CurrentBufferToSend->FrameParams,
                                                     sizeof (MME_LxMixerTransformerFrameDecodeParams_t),
                                                     &MixerCtrlBlock_p->CurrentBufferToSend->FrameStatus,
                                                     sizeof (MME_LxMixerTransformerFrameDecodeStatusParams_t),
                                                     MixerCtrlBlock_p->CurrentBufferToSend->BufferPtrs,
                                                     ACC_CMD_PLAY,(U32)MixerCtrlBlock_p->MixerStreamsInfo.NoOfRegisteredInput );

    return status;
}

ST_ErrorCode_t  STAud_FreeIpBlocks(MixerControlBlock_t  *MixerCtrlBlock_p, BOOL Flush)
{

    ST_ErrorCode_t  status = ST_NO_ERROR;
    U32             InputID = 0;
    U32                 MixerTailIndex = MixerCtrlBlock_p->MixerQueueTail % MIXER_QUEUE_SIZE;
    MixerInStreamBlocklist_t    * Temp = NULL,*next;
    MixerStreamsInfo_t          * MixerStreamsInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);

    for(; InputID < MixerStreamsInfo_p->NoOfRegisteredInput ; )
    {
        MixerInputStreamNode_t * InputStreamParams_p = &(MixerStreamsInfo_p->InputStreamsParams[InputID]);

        Temp = InputStreamParams_p->InBlockStarList_p;

        /* Check if thre is blocks to be freed */
        while(Temp != NULL)
        {
            MME_MixerInputStatus_t * InputStatus_p;

            /*  Flush will set when we want to stop the Mixer.
                So we have to free all input buffers, whatever be the case
            */
            InputStatus_p = &(MixerCtrlBlock_p->MixerQueue[MixerTailIndex].FrameStatus.InputStreamStatus[InputID]);

            if((InputStatus_p->BytesUsed < (Temp->InputBlock->Size - Temp->StartOffset)) && (!Flush))
            {
                /* Case - IP block was spilt into multiple Block and its not a flush cmmand */
                /*  Splited buffer is NOT consumed fully.Don't free it yet.
                    This buffer should be the last element of block list. So this should become the
                    first element in the list for next go
                 */

                Temp->BytesConsumed = (Temp->StartOffset + InputStatus_p->BytesUsed);
                Temp->StartOffset = Temp->BytesConsumed;

                /*forward the Master PTS if this is MASTER_STREAM_ID*/
                //if((InputID == MixerCtrlBlock_p->MasterInput) && (InputStreamParams_p->StreamWithValidPTS))
                // We need PTS for the input streams to be extrapolated if it is available
                if(InputStreamParams_p->StreamWithValidPTS)
                {
                    U32 JumpSize = 0;
                    /*This is the master stream*/
                    if(Temp->InputBlock->Flags & PTS_VALID)
                    {
                        /*Reset the Master extended PTS*/
                        //MixerStreamsInfo_p->ExtMasterPTS.BaseValue = Temp->InputBlock->Data[PTS_OFFSET];
                        //MixerStreamsInfo_p->ExtMasterPTS.BaseBit32 = Temp->InputBlock->Data[PTS33_OFFSET];
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                        InputStreamParams_p->ExtInPTS.BaseValue = Temp->InputBlock->Data[PTS_OFFSET];
                        InputStreamParams_p->ExtInPTS.BaseBit32 = Temp->InputBlock->Data[PTS33_OFFSET];
                        InputStreamParams_p->GotFirstValidPTS = TRUE;

#endif

                        /*Bytes to Jump*/
                        JumpSize = Temp->BytesConsumed;
                    }
                    else
                    {
                        /*Bytes to Jump*/
                        JumpSize = InputStatus_p->BytesUsed;
                    }

                    /*Extrapolate extended PTS*/
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                    STAud_ExtrapolateExtPts(MixerCtrlBlock_p, &(InputStreamParams_p->ExtInPTS), JumpSize);
#endif

                    /*update with extended PTS*/
                    //MixerStreamsInfo_p->ExtOutputPTS = MixerStreamsInfo_p->ExtMasterPTS;
                }

                /*Update the leftover bytes*/
                InputStreamParams_p->LeftOverBytes = (Temp->InputBlock->Size - Temp->StartOffset);
                if ((InputStreamParams_p->LeftOverBytes % 8) != 0)
                {
                    // STTBX_Print(("Remainytes:%5d\n",(Temp->InputBlock->Size - Temp->StartOffset)));
                }

                InputStreamParams_p->BytesUsed = Temp->StartOffset;
                //STTBX_Print(("Remainytes:%5d\n",(Temp->InputBlock->Size - Temp->StartOffset)));
                InputStatus_p->BytesUsed = 0;
                //MixerCtrlBlock_p->CurrentBufferToOut->InBlockStarList_p[InputID] = Temp;
                InputStreamParams_p->InBlockStarList_p = Temp;

                // Jump out of this loop as complete block is not consumed so no need to free it
                break;
            }
            else
            {

                /*forward the Master PTS if this is MASTER_STREAM_ID*/
                //if((InputID == MixerCtrlBlock_p->MasterInput) && (InputStreamParams_p->StreamWithValidPTS))
                if(InputStreamParams_p->StreamWithValidPTS)
                {
                    U32 JumpSize = 0;
                    /*This is the master stream*/
                    if(Temp->InputBlock->Flags & PTS_VALID)
                    {
                        /*Reset the Master extended PTS*/
                        //MixerStreamsInfo_p->ExtMasterPTS.BaseValue = Temp->InputBlock->Data[PTS_OFFSET];
                        //MixerStreamsInfo_p->ExtMasterPTS.BaseBit32 = Temp->InputBlock->Data[PTS33_OFFSET];
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                        InputStreamParams_p->ExtInPTS.BaseValue = Temp->InputBlock->Data[PTS_OFFSET];
                        InputStreamParams_p->ExtInPTS.BaseBit32 = Temp->InputBlock->Data[PTS33_OFFSET];
#endif
                        InputStreamParams_p->GotFirstValidPTS = TRUE;
                        /*Bytes to Jump*/
                        JumpSize = Temp->InputBlock->Size;
                    }
                    else
                    {
                        /*Bytes to Jump*/
                        JumpSize = (Temp->InputBlock->Size - Temp->StartOffset);
                    }

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
                    /*Extrapolate extended PTS*/
                    STAud_ExtrapolateExtPts(MixerCtrlBlock_p, &(InputStreamParams_p->ExtInPTS), JumpSize);
#endif
                    /*update with extended PTS*/
                    //MixerStreamsInfo_p->ExtOutputPTS = MixerCtrlBlock_p->MixerStreamsInfo.ExtMasterPTS;
                }
            }

            InputStatus_p->BytesUsed -= (Temp->InputBlock->Size - Temp->StartOffset);

            /*we have consumed the whole buffer*/
            Temp->BytesConsumed = Temp->InputBlock->Size;
            Temp->StartOffset = Temp->BytesConsumed;

            /*Update the leftover bytes*/
            InputStreamParams_p->LeftOverBytes = 0;
            InputStreamParams_p->BytesUsed = 0;
            /*send event to inform the PCM buffer has been consumed*/

            // Need to free IP block after all the processing on this block are completed
            /* Free the IP Blcok */
            status = MemPool_FreeIpBlk( InputStreamParams_p->InMemoryBlockHandle,
                        (U32 *)&Temp->InputBlock,
                        MixerCtrlBlock_p->Handle);
            if(status != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_FreeIpBlocks::MemPool_FreeIpBlk Failed\n"));
            }

            /*Notify consumption of a  PCM buffer*/
            /*We only report for all PCM inputs*/
            status |= STAudEVT_Notify(MixerCtrlBlock_p->EvtHandle, MixerCtrlBlock_p->EventIDPCMBufferComplete, (void *)&InputID, sizeof(InputID), MixerCtrlBlock_p->ObjectID);

            next = Temp->Next_p;
            memory_deallocate(MixerCtrlBlock_p->DriverPartition, Temp);
            Temp = next;

        }

        if(Temp == NULL)
        {
            /* Full list was consumed.So mark the start pointer as NULL */
            //MixerCtrlBlock_p->CurrentBufferToOut->InBlockStarList_p[InputID] = Temp;
            InputStreamParams_p->InBlockStarList_p = Temp;
        }

        InputID++;
    }

    /* Reset InputID */
    //InputID = 0;

    return status;
}

ST_ErrorCode_t STAud_SendMixedFrame(MixerControlBlock_t *MixerCtrlBlock_p, U32 DecodedBufSize, BOOL Flush)
{

    ST_ErrorCode_t  status = ST_NO_ERROR;
    MemoryBlock_t   *OutputBlockFromConsumer_p;
    MixerStreamsInfo_t  *MixerStreamsInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);
    MixerInputStreamNode_t      * InputStreamsParams_p = &(MixerStreamsInfo_p->InputStreamsParams[MixerCtrlBlock_p->MasterInput]);

    /* Get the output block */
    if(MixerCtrlBlock_p->MixerQueueFront != MixerCtrlBlock_p->MixerQueueTail)
    {
        MixerCtrlBlock_p->CurrentBufferToOut = &MixerCtrlBlock_p->MixerQueue[MixerCtrlBlock_p->MixerQueueFront % MIXER_QUEUE_SIZE];
    }
    else
    {
        /* Nothing to send */
        return ST_ERROR_NO_BLOCK_TO_SEND;
    }

//  OutputBlockFromConsumer_p = MixerCtrlBlock_p->CurrentBufferToOut->OutputBlockFromConsumer;
    OutputBlockFromConsumer_p = MixerStreamsInfo_p->OutputStreamParams.OutputBlockFromConsumer;

    if(MixerCtrlBlock_p->MixerQueueFront % 6 == 0)
    {
        OutputBlockFromConsumer_p->Data[FREQ_OFFSET] = CovertFsCodeToSampleRate(MixerCtrlBlock_p->CurrentBufferToSend->FrameStatus.SamplingFreq);//MIXER_OUT_FREQ;
        OutputBlockFromConsumer_p->Flags |= FREQ_VALID;

        if(InputStreamsParams_p->StreamWithValidPTS && InputStreamsParams_p->GotFirstValidPTS)
        {

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            /*Update the extended PTS*/
            OutputBlockFromConsumer_p->Data[PTS_OFFSET]     = InputStreamsParams_p->ExtInPTS.BaseValue;//MixerStreamsInfo_p->ExtOutputPTS.BaseValue;

            OutputBlockFromConsumer_p->Data[PTS33_OFFSET]   = InputStreamsParams_p->ExtInPTS.BaseBit32;//MixerStreamsInfo_p->ExtOutputPTS.BaseBit32;

            OutputBlockFromConsumer_p->Flags    |= PTS_VALID;
#endif
            // STTBX_Print(("OutputPTS=base:,%u,33:,%u\n",MixerStreamsInfo_p->InputStreamsParams[MixerCtrlBlock_p->MasterInput].ExtInPTS.BaseValue,MixerStreamsInfo_p->InputStreamsParams[MixerCtrlBlock_p->MasterInput].ExtInPTS.BaseBit32));
        }
    }

    /* Get output size */
    OutputBlockFromConsumer_p->Size = DecodedBufSize;

#if DUMP_OUTPUTTO_FILE // Accumlate data before dumping

        {
            static U32 index;
            char *temp;
            temp = (OutBuf1+index);
            memcpy((void*)temp,(void*)OutputBlockFromConsumer_p->MemoryStart,DecodedBufSize);
            index+=DecodedBufSize;
            if(index >= DecodedBufSize*(FRAMES_MIX-1))
            {
                static FILE *f1 = NULL;
                if(f1==NULL)
                    f1 = fopen("C:\\Dec1.dat","wb");
                fwrite(OutBuf1, DecodedBufSize*(FRAMES_MIX-1),1,f1);
            }
        }
#endif

    /* Send output block to next unit(Consumer)*/
    status = MemPool_PutOpBlk(MixerStreamsInfo_p->OutputStreamParams.OutBufHandle,
                    (U32 *)&OutputBlockFromConsumer_p);
    if(status != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_SendMixedFrame::MemPool_PutOpBlk Failed\n"));
    }

    /* Free the input blocks */
    STAud_FreeIpBlocks(MixerCtrlBlock_p,Flush);

    /* Update the Mixer Queue */
    MixerCtrlBlock_p->MixerQueueFront++;

    return ST_NO_ERROR;
}
/******************************************************************************
 *  Function name   : STAud_MixerMixFrame
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Mix the PCM data coming from different streams.
 ***************************************************************************** */
ST_ErrorCode_t  STAud_MixerMixFrame(MixerControlBlock_t *MixerCtrlBlock_p)
{
    ST_ErrorCode_t      status = ST_NO_ERROR;
    U32                 MixedBufSize = 0;
    //static U32            ReceivedCount;//debug

    switch(MixerCtrlBlock_p->MixerState)
    {
        case MIXER_IDLE:

            /* Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_STARTUP_PTS_CHECK;

        case MIXER_STARTUP_PTS_CHECK:
            /*Check for PTS only if mixer is not free running*/
            if(!MixerCtrlBlock_p->FreeRun)
            {
                /* Check and Wait till we receive the FIRST valid PTS from ALL the registered inputs, only then we will start Mixing  */
                status = STAud_StartUpCheck(MixerCtrlBlock_p);
            }
            else
            {
                /*No need to have startupsync. start mixing immidiately*/
                status = ST_NO_ERROR;
            }


            if((status == ST_NO_STARTUP_PTS) ||(status == ST_ERROR_NO_MEMORY))
            {
                /* Valid PTS from all the registered streams not received */

                //STTBX_Print(("MIXER_STARTUP_PTS_CHECK::ST_NO_STARTUP_PTS\n"));
                return ST_NO_ERROR;
            }
            else if(status == ST_ERROR_MALLOC_FAILED)
            {
                return ST_ERROR_NO_MEMORY;
            }

            /* If success - Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_GET_BUFFER_TO_SEND;

        case MIXER_GET_BUFFER_TO_SEND:

            /* Get the Transform parameter to fill. */
            if(MixerCtrlBlock_p->MixerQueueTail - MixerCtrlBlock_p->MixerQueueFront < MIXER_QUEUE_SIZE)
            {
                /*Queue is not needed as currently its size is 1 but may be needed in future */
                 U32 MixerTailIndex = MixerCtrlBlock_p->MixerQueueTail%MIXER_QUEUE_SIZE;

                MixerCtrlBlock_p->CurrentBufferToSend = &MixerCtrlBlock_p->MixerQueue[MixerTailIndex];
            }
            else
            {
                return ST_ERROR_NO_MEMORY;
            }

            /* Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_GET_OUTPUT_BUFFER;

        case MIXER_GET_OUTPUT_BUFFER:

            status = STAud_GetOutputBuffer(MixerCtrlBlock_p);
            if(status !=ST_NO_ERROR)
            {

                //STTBX_Print(("MIXER_GET_OUTPUT_BUFFER::No Memory Available \n"));
                return ST_ERROR_NO_MEMORY;
            }

            /* Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_GET_INPUT_DATA;

        case MIXER_GET_INPUT_DATA:

            status = STAud_GetInputAndAddToTransformParams(MixerCtrlBlock_p);
            if(status == ST_WAITING_FOR_ALL_STREAMS_DATA)
            {
                //STTBX_Print(("MIXER_STARTUP_PTS_CHECK::ST_WAITING_FOR_ALL_STREAMS_DATA\n"));
                return ST_NO_ERROR;
            }
            else if(status!=ST_NO_ERROR)
            {
                return status;
            }

            /* Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_UPDATE_MIXER_PARAMS;
#if VARIABLE_MIX_OUT_FREQ
            /* Check if we need to terminate and restart the transformer in case of freq change*/
            if(MixerCtrlBlock_p->OpFreqChange)
            {
                /*terminate the transformer*/

                if(STAud_TermMixerTransformer(MixerCtrlBlock_p) == MME_SUCCESS)
                {
                    MixerCtrlBlock_p->OpFreqChange = FALSE;

                    /*Initialize the new transformer*/
                    if(STAud_MixerInitTransformer(MixerCtrlBlock_p) == MME_SUCCESS)
                    {

                        STTBX_Print(("NEW MX TRANS INITIALIZED\n"));
                    }
                    else
                    {
                        STTBX_Print(("NEW MX TRANS INITIALIZE FAIL\n"));
                    }
                }
                else
                {
                    STTBX_Print(("MX TERM FAILED\n"));
                }
            }
#endif
        case MIXER_UPDATE_MIXER_PARAMS:
            /* Update mixer command Paramters */
            status = STAud_UpdateMixerTransformParams(MixerCtrlBlock_p);
            if(status != ST_NO_ERROR)
            {

                //STTBX_Print(("MIXER_UPDATE_MIXER_PARAMS:: ERROR\n"));
                return status;
            }

            /* Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_CALCULATE_PTS;

        case MIXER_CALCULATE_PTS:

            /* Calculate the Common PTS for all incoming data streams so that we can send that to next unit */
            //status = STAud_CalculateOutputPTS(MixerCtrlBlock_p); // calculate when we free up the inputs

            // This function will calculate and apply current FADE to main input
            if (MixerCtrlBlock_p->UpdateFadeOnMaster)
            {
                CheckAndApplyFade(MixerCtrlBlock_p);
            }

            /* Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_SEND_TO_MME;


        case MIXER_SEND_TO_MME:
            /* Send MME command to LX */
               status = MME_SendCommand(MixerCtrlBlock_p->MixerTranformHandle, &MixerCtrlBlock_p->CurrentBufferToSend->Cmd);
            if(status != MME_SUCCESS)
            {
                STTBX_Print(("MIXER_SEND_TO_MME:: ERROR MME_SendCommand\n"));
                return status;
            }

            /* Wait for LX to complete the Command */
            STOS_SemaphoreWait(MixerCtrlBlock_p->LxCallbackSem_p);

            //STTBX_Print(("0:Next=%d\n",MixerCtrlBlock_p->MixerQueue[0].FrameStatus.InputStreamStatus[0].NbInSplNextTransform));
            //STTBX_Print(("1:Next=%d\n",MixerCtrlBlock_p->MixerQueue[0].FrameStatus.InputStreamStatus[1].NbInSplNextTransform));
            /* Check the completed cmd's staus */
            if(MixerCtrlBlock_p->CurrentBufferToSend->Cmd.CmdStatus.State == MME_COMMAND_FAILED)
            {
                STTBX_Print(("MIXER_SEND_TO_MME:: ERROR MME_SendCommand\n"));
                MixedBufSize = 100;//TBD properly
            }
            else
            {
                MixedBufSize = MixerCtrlBlock_p->CurrentBufferToSend->Cmd.DataBuffers_p[MixerCtrlBlock_p->CurrentBufferToSend->OutputBufferIndex]->ScatterPages_p[0].BytesUsed;
            }

            /* Intended fall through */
            MixerCtrlBlock_p->MixerState = MIXER_PROCESS_OUTPUT;

        case MIXER_PROCESS_OUTPUT:

            status = STAud_SendMixedFrame(MixerCtrlBlock_p,MixedBufSize,FALSE);
            if(status != ST_NO_ERROR)
            {

                STTBX_Print(("MIXER_PROCESS_OUTPUT:: ERROR\n"));
                return status;
            }

            /* Change State to MIXER_GET_BUFFER_TO_SEND */
            MixerCtrlBlock_p->MixerState = MIXER_GET_BUFFER_TO_SEND;

        /*if(ReceivedCount%100==99)
            STTBX_Print((" Frames Mixed = %d \n",ReceivedCount++));*/

            break;
        default:
            break;
    }

    return status;
}

void    STAud_SetMixerInConfig(MME_LxMixerInConfig_t *InConfig,  MixerControlBlock_t    *MixerCtrlBlock_p)
{

    MixerStreamsInfo_t      * StreamsInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);
    U16                         InputID;

    InConfig->Id = ACC_RENDERER_MIXER_ID;
    InConfig->StructSize = sizeof(MME_LxMixerInConfig_t);

    memset(InConfig->Config, 0x0, (sizeof(MME_MixerInputConfig_t)*ACC_MIXER_MAX_NB_INPUT));

    /* Set valid info for all input streams */
    for(InputID = 0; InputID < StreamsInfo_p->NoOfRegisteredInput ; InputID++)
    {
        MME_MixerInputConfig_t * Config_p = &(InConfig->Config[InputID]);
        MixerInputStreamNode_t * IpStreamsParams_p = &(StreamsInfo_p->InputStreamsParams[InputID]);

        Config_p->InputId           = InputID;
        Config_p->AudioMode             = ACC_MODE20t;
        Config_p->AutoFade          = ACC_MME_FALSE;
        Config_p->FirstOutputChan   = ACC_MAIN_LEFT;
        Config_p->Mono2Stereo       = ACC_MME_TRUE;
        Config_p->NbChannels        = IpStreamsParams_p->numChannels;
        Config_p->SamplingFreq      = CovertToFsCode(IpStreamsParams_p->sampleRate);
        Config_p->WordSize          = (IpStreamsParams_p->bytesPerSample == 4) ? ACC_WS32 : ACC_WS16;
        Config_p->Alpha                 = IpStreamsParams_p->MixLevel;
    }

    /* Set ID for other inputs streams*/
    for(; InputID < ACC_MIXER_MAX_NB_INPUT ; InputID++)
    {
        InConfig->Config[InputID].InputId = InputID;
    }

}

void    STAud_SetMixerOutConfig(MME_LxMixerOutConfig_t *OutConfig,  MixerControlBlock_t *MixerCtrlBlock_p)
{

    OutConfig->StructSize = sizeof(MME_LxMixerOutConfig_t);
    OutConfig->Id = ACC_RENDERER_MIXER_OUTPUT_CONFIG_ID;

    OutConfig->NbOutputSamplesPerTransform = (MixerCtrlBlock_p->MixerStreamsInfo.OutputStreamParams.outputSampleRate * MSEC_PER_MIXER_TRANSFORM)/1000;
    // Convert num samples to even
    OutConfig->NbOutputSamplesPerTransform = (OutConfig->NbOutputSamplesPerTransform>>1)<<1;

}

MME_ERROR   STAud_SetMixerTrsfrmGlobalParams(MME_LxMixerTransformerGlobalParams_t *GlobalParams,  MixerControlBlock_t   *MixerCtrlBlock_p)
{
    MME_ERROR   Error = MME_SUCCESS;

    GlobalParams->StructSize = sizeof(MME_LxMixerTransformerGlobalParams_t);

    STAud_SetMixerInConfig(&GlobalParams->InConfig,MixerCtrlBlock_p);
    STAud_SetMixerOutConfig(&GlobalParams->OutConfig,MixerCtrlBlock_p);

    /* Use Postprocessor's function */
    // This is moved to Mixer Module init so that values which are changed from Init till
    // now are retained as well as values across start/stop sequences is maintained
    //STAud_SetMixerPCMParams(&GlobalParams->PcmParams);

    return Error;

}

MME_ERROR   STAud_MixerInitParams(MME_GenericParams_t   *init_param, MixerControlBlock_t    *MixerCtrlBlock_p)
{

    MME_ERROR   Error = MME_SUCCESS;
    MME_LxMixerTransformerInitParams_t  *Mixer_Init;
    MixerOutputStreamNode_t *OutputStreamParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.OutputStreamParams);

    Mixer_Init = (MME_LxMixerTransformerInitParams_t    *)init_param;

    Mixer_Init->StructSize = sizeof(MME_LxMixerTransformerInitParams_t);
    Mixer_Init->CacheFlush = ACC_MME_ENABLED;

    /* Set no of input to no of registered inputs */
    Mixer_Init->NbInput = MixerCtrlBlock_p->MixerStreamsInfo.NoOfRegisteredInput;

    /* Set the number of samples per transform */
    Mixer_Init->MaxNbOutputSamplesPerTransform = OutputStreamParams_p->outputSamplesPerTransform;
    Mixer_Init->OutputNbChannels = OutputStreamParams_p->outputChannels;
    Mixer_Init->OutputSamplingFreq = CovertToFsCode(OutputStreamParams_p->outputSampleRate);

    Error = STAud_SetMixerTrsfrmGlobalParams(&Mixer_Init->GlobalParams, MixerCtrlBlock_p);

    return Error;
}

MME_ERROR   STAud_MixerInitTransformer(MixerControlBlock_t  *MixerCtrlBlock_p)
{
    MME_ERROR   Error = MME_SUCCESS;
    U8          j=0;
    char            TransformerName[64];

    /* Initialize the mixer transformer */
    Error = STAud_MixerInitParams(MixerCtrlBlock_p->MMEMixerinitParams.TransformerInitParams_p, MixerCtrlBlock_p);

    if(Error == MME_SUCCESS)
    {

        /* We need to try on all LX */
        for(j = 0; j < TOTAL_LX_PROCESSORS; j++)
        {
            memset(TransformerName,0,64);
            /* Generate the LX transfommer name(based on ST231 number */
            GetLxName(j,"MME_TRANSFORMER_TYPE_AUDIO_MIXER",TransformerName, sizeof(TransformerName));
            Error = MME_InitTransformer(TransformerName, &MixerCtrlBlock_p->MMEMixerinitParams, &MixerCtrlBlock_p->MixerTranformHandle);

            if(Error == MME_SUCCESS)
            {
                MixerCtrlBlock_p->Transformer_Init = TRUE;
                break;
            }
            else
            {
                MixerCtrlBlock_p->Transformer_Init = FALSE;
            }
        }
    }

    return Error;
}

/******************************************************************************
 *  Function name   : STAud_MixerSetAndInit
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : MME_ERROR
 *  Description     :
 ***************************************************************************** */
MME_ERROR   STAud_MixerSetAndInit(MixerControlBlock_t   *MixerCtrlBlock_p)
{
    MME_ERROR   Error = MME_SUCCESS;
    U32             i,j;
    MixerStreamsInfo_t      *MixerStreamInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);

    /* Initialize the Input stream info */
    for(i  = 0 ; i < MixerStreamInfo_p->NoOfRegisteredInput; i++)
    {
        MixerInputStreamNode_t  *InputStreamsParams_p = &(MixerStreamInfo_p->InputStreamsParams[i]);
        InputStreamsParams_p->GotFirstValidPTS      = FALSE;
        InputStreamsParams_p->LeftOverBytes         = 0;
        InputStreamsParams_p->BytesUsed             = 0;
        InputStreamsParams_p->TransformerInBytes    = 0;
        InputStreamsParams_p->ScatterPageNum        = 0;

        MixerCtrlBlock_p->UpdateRequired[i]         = FALSE;

        /*Register Inputs*/
        if(InputStreamsParams_p->InMemoryBlockHandle)
        {
            Error |= MemPool_RegConsumer(InputStreamsParams_p->InMemoryBlockHandle,MixerCtrlBlock_p->consumerParam_s);
        }
    }

    /*Register Output*/
    if(MixerStreamInfo_p->OutputStreamParams.OutBufHandle)
    {
        Error |= MemPool_RegProducer(MixerStreamInfo_p->OutputStreamParams.OutBufHandle, MixerCtrlBlock_p->ProducerParam_s);
    }

    /* Reset Mixer queue */
    for(i=0;i<MIXER_QUEUE_SIZE;i++)
    {
        for(j=0; j< MixerStreamInfo_p->NoOfRegisteredInput ;j++)
        {
            //MixerCtrlBlock_p->MixerQueue[i].InBlockStarList_p[j] = NULL;
            MixerStreamInfo_p->InputStreamsParams[j].InBlockStarList_p = NULL;
        }
        //MixerCtrlBlock_p->MixerQueue[i].OutputBlockFromConsumer = NULL;
        MixerStreamInfo_p->OutputStreamParams.OutputBlockFromConsumer = NULL;
        MixerCtrlBlock_p->MixerQueue[i].OutputBufferIndex = 0;
    }

    /* Reset currently processed Input stream number */
    MixerStreamInfo_p->CurrentInputID = 0;

    MixerCtrlBlock_p->CompleteBuffer = FALSE;

    /* Init callback data as Handle. It will help us find the decoder from list of decoders in multi instance mode */
    MixerCtrlBlock_p->MMEMixerinitParams.CallbackUserData = (void*)MixerCtrlBlock_p->Handle;

    Error |= STAud_MixerInitTransformer(MixerCtrlBlock_p);

    return Error;

}
/******************************************************************************
 *  Function name : STAud_TermMixerTransformer
 *  Arguments       :
 *       IN         :
 *       OUT        : Error status
 *       INOUT      :
 *  Return          : MME_ERROR
 *  Description   :
 ***************************************************************************** */
MME_ERROR   STAud_TermMixerTransformer(MixerControlBlock_t  *MixerCtrlBlock_p)
{
    MME_ERROR   Error = MME_SUCCESS;
    U32         count = 0;

    do
    {
        //Delay(50);
        /* Terminate the Transformer */
        Error = MME_TermTransformer(MixerCtrlBlock_p->MixerTranformHandle);
        count++;
    }while((Error != MME_SUCCESS)&& (count < 10));

    if(Error != MME_SUCCESS)
    {
        /* error */
        STTBX_Print(("ERROR!!!MME_TermTransformer Failed \n"));
    }
    else
    {
        MixerCtrlBlock_p->Transformer_Init = FALSE;
    }

    return Error;
}

/******************************************************************************
 *  Function name   : STAud_StopMixer
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : MME_ERROR
 *  Description     :
 ***************************************************************************** */
MME_ERROR   STAud_StopMixer(MixerControlBlock_t *MixerCtrlBlock_p)
{
    MME_ERROR   Error = MME_SUCCESS;
    U32         count = 0;
//  ST_ErrorCode_t  status = ST_NO_ERROR;
    MixerStreamsInfo_t      *MixerStreamInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);

    Error = STAud_TermMixerTransformer(MixerCtrlBlock_p);

    /* Return the input and output buffers to BM */
    while(MixerCtrlBlock_p->MixerQueueFront != MixerCtrlBlock_p->MixerQueueTail)
    {

        /* Fake to the Mixer. Free all input output */
//      status = STAud_SendMixedFrame(MixerCtrlBlock_p,1000,FALSE);
        Error |= STAud_SendMixedFrame(MixerCtrlBlock_p,1000,FALSE);

    }

    /*Unregister from Producer*/
    for(count  = 0 ; count < MixerStreamInfo_p->NoOfRegisteredInput; count++)
    {
        if(MixerStreamInfo_p->InputStreamsParams[count].InMemoryBlockHandle)
        {
            Error |= MemPool_UnRegConsumer(MixerStreamInfo_p->InputStreamsParams[count].InMemoryBlockHandle,MixerCtrlBlock_p->consumerParam_s.Handle);
        }
    }

    /*Unregister from Consumer*/
    if(MixerStreamInfo_p->OutputStreamParams.OutBufHandle)
    {
        Error |= MemPool_UnRegProducer(MixerStreamInfo_p->OutputStreamParams.OutBufHandle,MixerCtrlBlock_p->ProducerParam_s.Handle);
    }

    /* Reset mixer queues */
    MixerCtrlBlock_p->MixerQueueFront = MixerCtrlBlock_p->MixerQueueTail=0;
    /*Mixing state to idle*/
    MixerCtrlBlock_p->MixerState =MIXER_IDLE;
#if VARIABLE_MIX_OUT_FREQ
    MixerCtrlBlock_p->OpFreqChange = FALSE;
#endif

    return Error;

}


/******************************************************************************
 *  Function name   : STAud_MixerInitInputQueueParams
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Initalize Decoder queue params
 *                Allocate and initialize MME structure for input buffers
 * ***************************************************************************** */
ST_ErrorCode_t STAud_MixerInitInputQueueParams( MixerControlBlock_t *MixerCtrlBlock_p,
                                                        STAVMEM_PartitionHandle_t AVMEMPartition)
{

    /* Local Variables */
    void                *VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_AllocBlockParams_t              AllocParams;
    U32 i,j;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MixerStreamsInfo_t      *MixerStreamInfo_p = &(MixerCtrlBlock_p->MixerStreamsInfo);

    /**************** Initialize Input param ********************/
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    AllocParams.PartitionHandle = AVMEMPartition;
    AllocParams.Alignment = 32;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;


    AllocParams.Size = sizeof(MME_DataBuffer_t) * MIXER_QUEUE_SIZE * MixerStreamInfo_p->NoOfRegisteredInput;

    Error = STAVMEM_AllocBlock(&AllocParams, &MixerCtrlBlock_p->BufferInHandle);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }
    Error = STAVMEM_GetBlockAddress(MixerCtrlBlock_p->BufferInHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }

    for(i = 0; i < MIXER_QUEUE_SIZE; i++)
    {

        for(j = 0; j < MixerStreamInfo_p->NoOfRegisteredInput; j++)
        {
            MixerInBufParams_t  * MixerBufIn_p = &(MixerCtrlBlock_p->MixerQueue[i].MixerBufIn[j]);
            void * Address = (MME_DataBuffer_t *)VirtualAddress + (j + (i * MixerStreamInfo_p->NoOfRegisteredInput));

            MixerBufIn_p->BufferIn_p = STAVMEM_VirtualToCPU(Address,&VirtualMapping);
        }
    }

    AllocParams.Size = sizeof(MME_ScatterPage_t) * ACC_MIXER_MAX_INPUT_PAGES_PER_STREAM * MIXER_QUEUE_SIZE * MixerStreamInfo_p->NoOfRegisteredInput;

    Error = STAVMEM_AllocBlock(&AllocParams, &MixerCtrlBlock_p->ScatterPageInHandle);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }
    Error = STAVMEM_GetBlockAddress(MixerCtrlBlock_p->ScatterPageInHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }

    for(i = 0; i < MIXER_QUEUE_SIZE; i++)
    {

        for(j = 0; j < MixerStreamInfo_p->NoOfRegisteredInput; j++)
        {
            MixerInBufParams_t  * MixerBufIn_p = &(MixerCtrlBlock_p->MixerQueue[i].MixerBufIn[j]);
            void * Address = (MME_ScatterPage_t *)VirtualAddress + ((j + (i * MixerStreamInfo_p->NoOfRegisteredInput))* ACC_MIXER_MAX_INPUT_PAGES_PER_STREAM);

            MixerBufIn_p->ScatterPageIn_p = STAVMEM_VirtualToCPU(Address,&VirtualMapping);
        }
    }


    for(i = 0; i < MIXER_QUEUE_SIZE; i++)
    {

        for(j = 0; j < MixerStreamInfo_p->NoOfRegisteredInput ;j++)
        {
            MixerInBufParams_t  * MixerBufIn_p = &(MixerCtrlBlock_p->MixerQueue[i].MixerBufIn[j]);

            /* Basic initialization */
            MixerBufIn_p->ScatterPageIn_p->FlagsIn = (U32)MME_DATA_CACHE_COHERENT;
            MixerBufIn_p->ScatterPageIn_p->FlagsOut = 0;

            MixerBufIn_p->BufferIn_p->StructSize    = sizeof (MME_DataBuffer_t);
            MixerBufIn_p->BufferIn_p->UserData_p    = NULL;
            MixerBufIn_p->BufferIn_p->Flags         = 0;
            MixerBufIn_p->BufferIn_p->StartOffset = 0;

            MixerBufIn_p->BufferIn_p->StreamNumber  = j;/* Stream Id */

            //MixerBufIn_p->BufferIn_p->NumberOfScatterPages = 1;

            MixerBufIn_p->BufferIn_p->ScatterPages_p= MixerBufIn_p->ScatterPageIn_p;

        }
    }

    return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name   : STAud_MixerInitOutputQueueParams
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : Initalize Decoder queue params
 *                Allocate and initialize MME structure for Output buffers
 ***************************************************************************** */
ST_ErrorCode_t STAud_MixerInitOutputQueueParams(MixerControlBlock_t *MixerCtrlBlock_p,
                                                STAVMEM_PartitionHandle_t AVMEMPartition)
{

    /* Local Variables */
    void                *VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    U32 i;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    AllocBlockParams.PartitionHandle = AVMEMPartition;
    AllocBlockParams.Alignment = 32;
    AllocBlockParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.ForbiddenRangeArray_p = NULL;
    AllocBlockParams.ForbiddenBorderArray_p = NULL;
    AllocBlockParams.NumberOfForbiddenRanges = 0;
    AllocBlockParams.NumberOfForbiddenBorders = 0;

    AllocBlockParams.Size = sizeof(MME_DataBuffer_t) * MIXER_QUEUE_SIZE;

    Error = STAVMEM_AllocBlock(&AllocBlockParams, &MixerCtrlBlock_p->BufferOutHandle);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(MixerCtrlBlock_p->BufferOutHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }

    for(i = 0; i < MIXER_QUEUE_SIZE; i++)
    {
        MixerrOutBufParams_t    * MixerBufOut_p = &(MixerCtrlBlock_p->MixerQueue[i].MixerBufOut);
        void * Address = (MME_DataBuffer_t *)VirtualAddress + i;

        MixerBufOut_p->BufferOut_p = STAVMEM_VirtualToCPU(Address,&VirtualMapping);
    }

    AllocBlockParams.Size = sizeof(MME_ScatterPage_t) * MIXER_QUEUE_SIZE;

    Error = STAVMEM_AllocBlock(&AllocBlockParams, &MixerCtrlBlock_p->ScatterPageOutHandle);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(MixerCtrlBlock_p->ScatterPageOutHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        return Error;
    }

    for(i = 0; i < MIXER_QUEUE_SIZE; i++)
    {
        MixerrOutBufParams_t    * MixerBufOut_p = &(MixerCtrlBlock_p->MixerQueue[i].MixerBufOut);
        void * Address = (MME_ScatterPage_t *)VirtualAddress + i;

        MixerBufOut_p->ScatterPageOut_p = STAVMEM_VirtualToCPU(Address,&VirtualMapping);
    }

   for(i = 0; i < MIXER_QUEUE_SIZE; i++)
   {
        MixerrOutBufParams_t        * MixerBufOut_p = &(MixerCtrlBlock_p->MixerQueue[i].MixerBufOut);
        /* We will set the Page_p to point to the Empty PCM buffer, received from Consumer unit */

        MixerBufOut_p->ScatterPageOut_p->Page_p = NULL;

        MixerBufOut_p->ScatterPageOut_p->FlagsIn = (U32)MME_DATA_CACHE_COHERENT;
        MixerBufOut_p->ScatterPageOut_p->FlagsOut = 0;
        MixerBufOut_p->ScatterPageOut_p->BytesUsed= 0;

        MixerBufOut_p->BufferOut_p->StructSize    = sizeof (MME_DataBuffer_t);
        MixerBufOut_p->BufferOut_p->UserData_p    = NULL;
        MixerBufOut_p->BufferOut_p->Flags         = 0;
        MixerBufOut_p->BufferOut_p->StreamNumber  = 0;
        MixerBufOut_p->BufferOut_p->NumberOfScatterPages = 1;
        MixerBufOut_p->BufferOut_p->ScatterPages_p= MixerBufOut_p->ScatterPageOut_p;
        MixerBufOut_p->BufferOut_p->StartOffset =  0;

    }

    return ST_NO_ERROR;

}

void        STAud_MixerInitFrameParams(MixerControlBlock_t *MixerCtrlBlock_p)
{
    U32 i,j;
    for(i = 0; i < MIXER_QUEUE_SIZE; i++)
    {
        for(j=0;j<MixerCtrlBlock_p->MixerStreamsInfo.NoOfRegisteredInput;j++)
        {
            MixerCtrlBlock_p->MixerQueue[i].FrameParams.InputParam[j].Command = ACC_CMD_PLAY;
            MixerCtrlBlock_p->MixerQueue[i].FrameParams.InputParam[j].StartOffset=0;
        }
        /*Init the status parames */
        MixerCtrlBlock_p->MixerQueue[i].FrameStatus.StructSize=sizeof(MME_LxMixerTransformerFrameDecodeStatusParams_t);
    }
}

/******************************************************************************
 *  Function name   : STAud_MixerDeInitInputQueueParams
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : De allocate Decoder queue params
 *                1. Deallocate Input buffers
 ***************************************************************************** */
 ST_ErrorCode_t STAud_MixerDeInitInputQueueParams(MixerControlBlock_t *MixerCtrlBlock_p)
{

    ST_ErrorCode_t  Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
//  STAVMEM_BlockHandle_t   ScatterPageInHandle,BufferInHandle;

    FreeBlockParams.PartitionHandle = MixerCtrlBlock_p->AVMEMPartition;

    //ScatterPageInHandle =     MixerCtrlBlock_p->ScatterPageInHandle;
    if(MixerCtrlBlock_p->ScatterPageInHandle)
    {
        Error |= STAVMEM_FreeBlock(&FreeBlockParams,&MixerCtrlBlock_p->ScatterPageInHandle);
        MixerCtrlBlock_p->ScatterPageInHandle = 0;
    }

    //BufferInHandle = MixerCtrlBlock_p->BufferInHandle;
    if(MixerCtrlBlock_p->BufferInHandle)
    {
        Error |= STAVMEM_FreeBlock(&FreeBlockParams,&MixerCtrlBlock_p->BufferInHandle);
        MixerCtrlBlock_p->BufferInHandle = 0;
    }

    return Error;
}

/******************************************************************************
 *  Function name   : STAud_MixerDeInitOutputQueueParams
 *  Arguments       :
 *       IN         :
 *       OUT            : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : De allocate Decoder queue params
 *                1. Deallocate output buffers
 ***************************************************************************** */
 ST_ErrorCode_t STAud_MixerDeInitOutputQueueParams(MixerControlBlock_t *MixerCtrlBlock_p)
{

    ST_ErrorCode_t  Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    //STAVMEM_BlockHandle_t ScatterPageHandle,BufferHandle;

    FreeBlockParams.PartitionHandle = MixerCtrlBlock_p->AVMEMPartition;

    //ScatterPageHandle =   MixerCtrlBlock_p->ScatterPageOutHandle;
    if(MixerCtrlBlock_p->ScatterPageOutHandle)
    {
        Error |= STAVMEM_FreeBlock(&FreeBlockParams,&MixerCtrlBlock_p->ScatterPageOutHandle);
        MixerCtrlBlock_p->ScatterPageOutHandle = 0;
    }

    //BufferHandle = MixerCtrlBlock_p->BufferOutHandle;
    if(MixerCtrlBlock_p->BufferOutHandle)
    {
        Error |= STAVMEM_FreeBlock(&FreeBlockParams,&MixerCtrlBlock_p->BufferOutHandle);
        MixerCtrlBlock_p->BufferOutHandle = 0;
    }

    return Error;
}

/******************************************************************************
 *  Function name : MME_MixerTransformCompleted
 *  Arguments       :
 *       IN         : CallbackData : mme command
 *                      : UserData : user data
 *       OUT        : Error Status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description   : used to trigger the mixer task if there has been any callback
 ***************************************************************************** */

ST_ErrorCode_t MME_MixerTransformCompleted(MME_Command_t * CallbackData , void *UserData)
{

    MixerControlBlockList_t *Mixer_p= (MixerControlBlockList_t*)NULL;
    STAud_MixerHandle_t Handle;

    /* Get the Mixer refereance fro which this call back has come */
    Handle = (STAud_MixerHandle_t)UserData;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("ERROR!!!MME_MixerTransformCompleted::NO Decoder Handle Found \n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Signal Mixer Task Semaphore( To handle spacial case. Mixer canonly be triggred by IP or OP BM.
    If everythig is fine,which should be the case, after completing this command, no one will trigger Mixer.
    So we will explicitly do it here */
    STOS_SemaphoreSignal(Mixer_p->MixerControlBlock.MixerTaskSem_p);

    /* Signal LxCallbackSem_p Semaphore */
    STOS_SemaphoreSignal(Mixer_p->MixerControlBlock.LxCallbackSem_p);

    return ST_NO_ERROR;

}

 ST_ErrorCode_t MMEGlobalUpdateParamsCompleted(MME_Command_t * CallbackData , void *UserData)
{
    /* Local variables */
    MME_Command_t       *command_p;
    MixerControlBlockList_t *Mixer_p= (MixerControlBlockList_t*)NULL;
    STAud_MixerHandle_t Handle;

    /* Get the Mixer refereance fro which this call back has come */
    Handle = (STAud_MixerHandle_t)UserData;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("ERROR!!!MME_mixerTransformCompleted::NO Mixer Handle Found \n"));
    }

    command_p       = (MME_Command_t *) CallbackData;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("MMEGlobalUpdateParamsCompleted::Mixer - MME_COMMAND_FAILED \n"));
    }
    return ST_NO_ERROR;

}
void Mixer_TransformerCallback(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{

    MME_Command_t   *command_p;

    command_p       = (MME_Command_t *) CallbackData;

    if (Event != MME_COMMAND_COMPLETED_EVT)
    {
        if (Event == MME_NOT_ENOUGH_MEMORY_EVT)
        {
            STTBX_Print(("Receiving a underflow after last buffer\n"));
        }
        /* For debugging purpose we can add a few prints here */
        return;
    }
    /* Debug info */

    switch(command_p->CmdCode)
    {
        case MME_TRANSFORM:
            MME_MixerTransformCompleted(CallbackData,UserData);
            break;

        case MME_SET_GLOBAL_TRANSFORM_PARAMS:
            MMEGlobalUpdateParamsCompleted(CallbackData,UserData);
            break;

        case MME_SEND_BUFFERS:
            //MMESendBufferCompleted(CallbackData,UserData);
            break;
        default:
            break;
    }

}

/******************************************************************************
 *  Function name   : STAud_MixerIsInit
 *  Arguments       :
 *       IN         :
 *       OUT            : BOOL
 *       INOUT      :
 *  Return          : BOOL
 *  Description     : Search the list of decoder control blocks to find if "this" decoder is already
 *                Initialized.
 ***************************************************************************** */
 BOOL STAud_MixerIsInit(const ST_DeviceName_t DeviceName)
{
    BOOL    Initialized = FALSE;
    MixerControlBlockList_t *queue = MixerQueueHead_p;/* Global queue head pointer */

    while (queue)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(queue->MixerControlBlock.DeviceName, DeviceName) != 0)
        {
            /* Next Mixer in the queue */
            queue = queue->Next_p;
        }
        else
        {
            /* The Mixer is already initialized */
            Initialized = TRUE;
            break;
        }
    }

    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;

}

/******************************************************************************
 *  Function name   : STAud_MixerQueueAdd
 *  Arguments       :
 *       IN         :
 *       OUT            : BOOL
 *       INOUT      :
 *  Return          : BOOL
 *  Description     : This routine appends an allocated Mixer control block to the
 *                 mixer queue.
 ***************************************************************************** */
 void STAud_MixerQueueAdd(MixerControlBlockList_t *QueueItem_p)
{
    MixerControlBlockList_t     *qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if (MixerQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        MixerQueueHead_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = MixerQueueHead_p;
        while (qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
} /* STAud_MixerQueueAdd() */

/******************************************************************************
 *  Function name   : STAud_MixerQueueRemove
 *  Arguments       :
 *       IN         :
 *       OUT            : BOOL
 *       INOUT      :
 *  Return          : BOOL
 *  Description     : This routine removes a Mixer control block from the
 *                 Mixer queue.
 ***************************************************************************** */
 void *   STAud_MixerQueueRemove( MixerControlBlockList_t   *QueueItem_p)
{
    MixerControlBlockList_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if (MixerQueueHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = MixerQueueHead_p;

        /* Iterate over each queue element until we find the required
         * queue item to remove.
         */
        while (this_p != NULL && this_p != QueueItem_p)
        {
            last_p = this_p;
            this_p = this_p->Next_p;
        }

        /* Check we found the queue item */
        if (this_p == QueueItem_p)
        {
            /* Unlink the item from the queue */
            if (last_p != NULL)
            {
                last_p->Next_p = this_p->Next_p;
            }
            else
            {
                /* Recalculate the new head of the queue i.e.,
                 * we have just removed the queue head.
                 */
                MixerQueueHead_p = this_p->Next_p;
            }
        }
    }
    return (void*)MixerQueueHead_p;
} /* STAud_MixerQueueRemove() */
/******************************************************************************
 *  Function name   : STAud_MixerGetControlBlockFromName
 *  Arguments       :
 *       IN         :
 *       OUT            : BOOL
 *       INOUT      :
 *  Return          : BOOL
 *  Description     : This routine removes a Mixer control block from the
 *                 Mixer queue.
 ***************************************************************************** */
 MixerControlBlockList_t *
   STAud_MixerGetControlBlockFromName(const ST_DeviceName_t DeviceName)
{
    MixerControlBlockList_t *qp = MixerQueueHead_p; /* Global queue head pointer */

    while (qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->MixerControlBlock.DeviceName, DeviceName) != 0)
        {
            /* Next Mixer in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The Mixer entry has been found */
            break;
        }
    }

    /* Return the Mixer control block (or NULL) to the caller */
    return qp;
} /* STAud_MixerGetControlBlockFromName() */

/******************************************************************************
 *  Function name   : STAud_MixerGetControlBlockFromHandle
 *  Arguments       :
 *       IN         :
 *       OUT            : BOOL
 *       INOUT      :
 *  Return          : BOOL
 *  Description     : This routine returns the control block for a given handle
 ***************************************************************************** */
MixerControlBlockList_t *
   STAud_MixerGetControlBlockFromHandle(STAud_MixerHandle_t Handle)
{
    MixerControlBlockList_t *qp = MixerQueueHead_p; /* Global queue head pointer */

    /* Iterate over the uart queue */
    while (qp != NULL)
    {
        /* Check for a matching handle */
        if (Handle == qp->MixerControlBlock.Handle && Handle != 0)
        {
            break;  /* This is a valid handle */
        }

        /* Try the next block */
        qp = qp->Next_p;
    }

    /* Return the Mixer control block (or NULL) to the caller */
    return qp;
}


ST_ErrorCode_t STAud_InitMixerCapability(MixerControlBlock_t *MixerCtrlBlock_p)
{
    /* set the values */
    MixerCtrlBlock_p->MixerCapability.MixPerInputCapable = FALSE;
    MixerCtrlBlock_p->MixerCapability.NumberOfInputs = 2;
    return ST_NO_ERROR;
}

/* PCM param Init */
void STAud_SetEqParams(MME_EqualizerGlobalParams_t *Equalizer)
{
    Equalizer->Id = ACC_RENDERER_EQUALIZER_ID;
    Equalizer->StructSize = sizeof(MME_EqualizerGlobalParams_t);

    Equalizer->Config[EQ_BAND_0] = 0;
    Equalizer->Config[EQ_BAND_1] = 0;
    Equalizer->Config[EQ_BAND_2] = 0;
    Equalizer->Config[EQ_BAND_3] = 0;
    Equalizer->Config[EQ_BAND_4] = 0;
    Equalizer->Config[EQ_BAND_5] = 0;
    Equalizer->Config[EQ_BAND_6] = 0;
    Equalizer->Config[EQ_BAND_7] = 0;
    Equalizer->Enable = ACC_MME_FALSE;
    Equalizer->Apply = ACC_MME_DISABLED;
}

void    STAud_SetDelayParams(MME_DelayGlobalParams_t *Delay)
{

    Delay->Id = /*ACC_LAST_RENDERERPROCESS_ID;*/ACC_RENDERER_DELAY_ID;
    Delay->StructSize = sizeof(MME_DelayGlobalParams_t);
    Delay->Apply = ACC_MME_DISABLED;
    Delay->DelayUpdate = ACC_MME_FALSE;
    Delay->Delay[ACC_MAIN_LEFT ]  = 0;
    Delay->Delay[ACC_MAIN_RGHT ]  = 0;
    Delay->Delay[ACC_MAIN_CNTR ]  = 0;
    Delay->Delay[ACC_MAIN_LFE  ]  = 0;
    Delay->Delay[ACC_MAIN_LSUR ]  = 0;
    Delay->Delay[ACC_MAIN_RSUR ]  = 0;
    Delay->Delay[ACC_MAIN_CSURL]  = 0;
    Delay->Delay[ACC_MAIN_CSURR]  = 0;

}
void STAud_SetBassMgtParams(MME_BassMgtGlobalParams_t *BassMgt)
{
    U32 i = 0;
    BassMgt->Id=ACC_RENDERER_BASSMGT_ID;   //!< ID of this processing
    BassMgt->StructSize=sizeof(MME_BassMgtGlobalParams_t);
    BassMgt->Apply=/*ACC_MME_ENABLED;*/ACC_MME_DISABLED;
    for(i=0;i<BASSMGT_NB_CONFIG_ELEMENTS;i++)
    {
        BassMgt->Config[i]=0;
    }
    for(i=0;i<VOLUME_NB_CONFIG_ELEMENTS;i++)
    {
        BassMgt->Volume[i]= 0; /*MAX_ACC_VOL;*/
    }
    for(i=0;i<DELAY_NB_CONFIG_ELEMENTS;i++)
    {
        BassMgt->Delay[i]=0;
    }

    BassMgt->DelayUpdate=ACC_MME_FALSE;

}

void    STAud_SetDcrmParams(MME_DCRemoveGlobalParams_t *DcRemove)
{

    DcRemove->Id=ACC_RENDERER_DCREMOVE_ID;               //!< ID of this processing
    DcRemove->StructSize=sizeof(MME_DCRemoveGlobalParams_t);
    DcRemove->Apply=ACC_MME_DISABLED;

}
void STAud_SetTempoParams(MME_TempoGlobalParams_t *TempoControl)
{
    TempoControl->Id = ACC_RENDERER_TEMPO_ID;
    TempoControl->StructSize = sizeof(MME_TempoGlobalParams_t);
    TempoControl->Apply = ACC_MME_DISABLED;
    TempoControl->Ratio = 0;//??
}

void STAud_SetEncparams(MME_EncoderPPGlobalParams_t *EncControl)
{
    EncControl->Id    = ACC_RENDERER_ENCODERPP_ID;
    EncControl->Apply = ACC_MME_DISABLED;
    EncControl->StructSize = sizeof(MME_EncoderPPGlobalParams_t);
}

void STAud_SetSFCparams(MME_SfcPPGlobalParams_t *SfcControl)
{
    SfcControl->Id          = ACC_RENDERER_SFC_ID;
    SfcControl->StructSize  = sizeof(MME_SfcPPGlobalParams_t);
    SfcControl->Apply       = ACC_MME_DISABLED;
}

void  STAud_SetCMCparams(MME_CMCGlobalParams_t *CMCControl)
{
    CMCControl->Id =ACC_RENDERER_CMC_ID;
    CMCControl ->StructSize = sizeof(MME_CMCGlobalParams_t);
    CMCControl->SurroundMixCoeff =  ACC_M3DB;
    CMCControl->CenterMixCoeff=  ACC_M3DB;
    CMCControl->Config[CMC_OUTMODE_MAIN] = ACC_MODE_ID;
    CMCControl->Config[CMC_OUTMODE_AUX]     = (U8) ACC_MODE_ID;
    CMCControl->Config[CMC_DUAL_MODE]       = (U8) (((ACC_DUAL_LR) << 4) | (ACC_DUAL_LR)); // Dual mode for Aux / Main
    CMCControl->Config[CMC_PCM_DOWN_SCALED] = (U8) ACC_MME_FALSE;
}

void    STAud_SetDMixparams(MME_DMixGlobalParams_t * DMixControl)
{
    DMixControl->Id=ACC_RENDERER_DMIX_ID;
    DMixControl->StructSize=sizeof(MME_DMixGlobalParams_t);// - (sizeof(tCoeff15) * 80);
    DMixControl->Apply = ACC_MME_DISABLED;
    DMixControl->Config[DMIX_USER_DEFINED]= (U8) 0x0;
    DMixControl->Config[DMIX_STEREO_UPMIX]= ACC_MME_FALSE;
    DMixControl->Config[DMIX_MONO_UPMIX]= ACC_MME_FALSE;
    DMixControl->Config[DMIX_MEAN_SURROUND]= ACC_MME_TRUE;
    DMixControl->Config[DMIX_SECOND_STEREO]= ACC_MME_TRUE;
    DMixControl->Config[DMIX_NORMALIZE]= ACC_MME_TRUE;
    DMixControl->Config[DMIX_MIX_LFE]   = ACC_MME_FALSE;
    DMixControl->Config[DMIX_NORM_IDX]= 0;
    DMixControl->Config[DMIX_DIALOG_ENHANCE]= ACC_MME_FALSE;;
}
#ifdef ST_7200
void STAud_SetBTSCparams(MME_BTSCGlobalParams_t *BTSC)
{
    BTSC->Apply= ACC_MME_DISABLED;
    BTSC->DualSignal=0;
    BTSC->Id=ACC_RENDERER_BTSC_ID;
    BTSC->InputGain=0x7FFF;
    BTSC->StructSize=sizeof(MME_BTSCGlobalParams_t);
    BTSC->TXGain=0x7FFF;

}
#endif


void    STAud_SetMixerPCMParams( MME_LxPcmPostProcessingGlobalParameters_t *PcmParams)
{

    PcmParams->StructSize = sizeof(MME_LxPcmPostProcessingGlobalParameters_t);
    PcmParams->Id = ACC_RENDERER_MIXER_POSTPROCESSING_ID;

    STAud_SetBassMgtParams(&PcmParams->BassMgt);
    STAud_SetDcrmParams(&PcmParams->DCRemove);
    STAud_SetEqParams(&PcmParams->Equalizer);
    STAud_SetDelayParams(&PcmParams->Delay);
    STAud_SetTempoParams(&PcmParams->TempoControl);
    STAud_SetEncparams(&PcmParams->Encoder);
    STAud_SetSFCparams(&PcmParams->Sfc);
    STAud_SetDMixparams(&PcmParams->Dmix);
    STAud_SetCMCparams(&PcmParams->CMC);
#ifdef ST_7200
    STAud_SetBTSCparams(&PcmParams->Btsc);
#endif

}

ST_ErrorCode_t STAud_MixerUpdateGlobalParams(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID)
{
    MME_ERROR   Error = ST_NO_ERROR;
    MME_LxMixerTransformerInitParams_t  *Mixer_Init=NULL;
    MME_LxMixerTransformerGlobalParams_t    *GlobalParams = NULL;
    MME_LxMixerInConfig_t                      *InConfig;
    MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);


    Mixer_Init = (MME_LxMixerTransformerInitParams_t*) MixerCtrlBlock_p->MMEMixerinitParams.TransformerInitParams_p;
    GlobalParams = (MME_LxMixerTransformerGlobalParams_t*)&Mixer_Init->GlobalParams;

    /* Get hold of input params */
    InConfig = (MME_LxMixerInConfig_t*)&GlobalParams->InConfig;
    /* Set params */
    //InConfig->Config[InputID].SamplingFreq = CovertToFsCode(InSampleFreq);

#if 0
    if ((MixerCtrlBlock_p->MasterInput == InputID) && (!InputStreamsParams_p->UserProvidedLevel))
    {
        InConfig->Config[InputID].Alpha = FADE_MAPPING[InputStreamsParams_p->FadeInfo.curFade];
        STTBX_Print(("Applying Fade 0x%x:%d\n",InputStreamsParams_p->FadeInfo.curFade, InConfig->Config[InputID].Alpha));
    }
    else
    {
        InConfig->Config[InputID].Alpha = InputStreamsParams_p->MixLevel;
    }
#else
    {
        U32 tempAlpha;
#ifdef ST_51XX
        // TBD convert from db to absolute val
        tempAlpha = (U32)(((U32)DB_ABSOLUTE[GlobalParams->PcmParams.BassMgt.Volume[ACC_MAIN_LEFT ]] * (U32)InputStreamsParams_p->MixLevel) >> 15);
#else
        tempAlpha = InputStreamsParams_p->MixLevel;
#endif
        if (MixerCtrlBlock_p->MasterInput == InputID)
        {
            tempAlpha = (U32)(((U32)FADE_MAPPING[InputStreamsParams_p->FadeInfo.curFade] * (U32)tempAlpha) >> 15);
            InConfig->Config[InputID].Alpha = tempAlpha;
            STTBX_Print(("Applying Fade 0x%x:%d\n",InputStreamsParams_p->FadeInfo.curFade, InConfig->Config[InputID].Alpha));
        }
        else
        {
            InConfig->Config[InputID].Alpha = tempAlpha;
        }
    }


#endif

#ifdef ST_51XX
    if((GlobalParams->Swap[InputID] != 1)&& (CovertToLpcmWSCode(InputStreamsParams_p->sampleBits)== ACC_LPCM_WS16le))
    {
        GlobalParams->Swap[InputID] = 1;
    }
    else if((GlobalParams->Swap[InputID] == 1)&& (CovertToLpcmWSCode(InputStreamsParams_p->sampleBits)== ACC_LPCM_WS16))
    {
        GlobalParams->Swap[InputID] = 0;
    }
#endif
    InConfig->Config[InputID].NbChannels = InputStreamsParams_p->numChannels;
    InConfig->Config[InputID].SamplingFreq = CovertToFsCode(InputStreamsParams_p->sampleRate);
    InConfig->Config[InputID].WordSize = (InputStreamsParams_p->bytesPerSample == 4) ? ACC_WS32 : ACC_WS16;

    /* Build cmd */
    memset(&MixerCtrlBlock_p->GlobalCmd, 0, sizeof(MME_Command_t));
    MixerCtrlBlock_p->GlobalCmd.StructSize = sizeof(MME_Command_t);
    MixerCtrlBlock_p->GlobalCmd.CmdEnd   = MME_COMMAND_END_RETURN_NOTIFY;
    MixerCtrlBlock_p->GlobalCmd.CmdCode = MME_SET_GLOBAL_TRANSFORM_PARAMS;
    MixerCtrlBlock_p->GlobalCmd.DueTime  = (MME_Time_t) 0;

    MixerCtrlBlock_p->GlobalCmd.ParamSize  = sizeof(MME_LxMixerTransformerGlobalParams_t);
    MixerCtrlBlock_p->GlobalCmd.Param_p  = GlobalParams;

    Error=MME_SendCommand(MixerCtrlBlock_p->MixerTranformHandle, &MixerCtrlBlock_p->GlobalCmd);

    return Error;
}

/******************************************************************************
 *  Function name   : STAud_SetMixerMixLevel
 *  Arguments       :
 *       IN         : Mixer List Handle, InputID , Mixing Level
 *       OUT            :
 *       INOUT      :
 *  Return          : Error status
 *  Description     : This routine set the Mix_Level and set UpdateRequired
                  corresponding to InputID as requested
 ***************************************************************************** */

ST_ErrorCode_t  STAud_SetMixerMixLevel(STAud_MixerHandle_t Handle, U32 InputID, U16 MixLevel)
{
    ST_ErrorCode_t  Error=ST_NO_ERROR;
    MixerControlBlockList_t *Mixer_p= (MixerControlBlockList_t*)NULL;
    MixerStreamsInfo_t      * MixerStreamsInfo_p;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    MixerStreamsInfo_p = &(Mixer_p->MixerControlBlock.MixerStreamsInfo);
    /* Set Mix_Level for given InputID */

    if(InputID < MixerStreamsInfo_p->NoOfRegisteredInput)
    {
        STOS_MutexLock(Mixer_p->MixerControlBlock.LockControlBlockMutex_p);

        MixerStreamsInfo_p->InputStreamsParams[InputID].NewMixLevel = MixLevel;
        if (MixLevel == 0xFFFF)
        {
            MixerStreamsInfo_p->InputStreamsParams[InputID].UserProvidedLevel = FALSE;
        }
        else
        {
            MixerStreamsInfo_p->InputStreamsParams[InputID].UserProvidedLevel = TRUE;
        }


        STOS_MutexRelease(Mixer_p->MixerControlBlock.LockControlBlockMutex_p);
    }
    else
    {
        /*The mixer doesn't have the requested InputID */
        Error = ST_ERROR_BAD_PARAMETER;
    }
    return Error;
}

/******************************************************************************
 *  Function name   : STAud_GetMixerMixLevel
 *  Arguments       :
 *       IN         : Mixer List Handle, InputID , &Mixing Level
 *       OUT            :
 *       INOUT      :
 *  Return          : Error status
 *  Description     : This routine copies the Mix_Level wrt InputID to
                       pointer Mix_Level
 ***************************************************************************** */
ST_ErrorCode_t  STAud_GetMixerMixLevel(STAud_MixerHandle_t Handle, U32 InputID, U16 *MixLevel_p)
{
    ST_ErrorCode_t  Error=ST_NO_ERROR;
    MixerControlBlockList_t *Mixer_p= (MixerControlBlockList_t*)NULL;
    MixerStreamsInfo_t      * MixerStreamsInfo_p;

    Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);
    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    MixerStreamsInfo_p = &(Mixer_p->MixerControlBlock.MixerStreamsInfo);
    /* Set Mix_Level for given InputID */

    if(InputID < MixerStreamsInfo_p->NoOfRegisteredInput)
    {
        *MixLevel_p = MixerStreamsInfo_p->InputStreamsParams[InputID].MixLevel ;
    }

    else
    {
        /*The mixer doesn't have the requested InputID */
        Error = ST_ERROR_BAD_PARAMETER;
    }
    return Error;
}


ST_ErrorCode_t STAud_CheckInputStreamFreqChange(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID)
{
    U32 InputFreq = 0,CurFreq = 0;
    ST_ErrorCode_t  Error=ST_NO_ERROR;
    MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);

    InputFreq = MixerCtrlBlock_p->CurInputBlockFromProducer->Data[FREQ_OFFSET];
    CurFreq = InputStreamsParams_p->sampleRate;

    /* Check if there is change in input sampling frequency */
    if(InputFreq != CurFreq)
    {
        /* Change in input  freq -Update Mixer params */
        MixerCtrlBlock_p->UpdateRequired[InputID] = TRUE;
        InputStreamsParams_p->sampleRate = InputFreq;
        InputStreamsParams_p->bytesPerMSec = ((InputStreamsParams_p->bytesPerSample * InputStreamsParams_p->numChannels) * (InputStreamsParams_p->sampleRate))/1000;
    }
#if VARIABLE_MIX_OUT_FREQ
    {
        MixerOutputStreamNode_t * OPStreamParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.OutputStreamParams);
        /* freq change of master input */
        if(InputFreq != OPStreamParams_p->outputSampleRate && InputID == MixerCtrlBlock_p->MasterInput)
        {
#ifdef ST_51XX
            /*change only if freq in less than 48KHz.*/
            //InputFreq = (InputFreq > MIXER_OUT_FREQ)? MIXER_OUT_FREQ:InputFreq;
            OPStreamParams_p->outputSampleRate = InputFreq;
#else
            /*change only if freq in less than 48KHz.*/
            //InputFreq = (InputFreq > MIXER_OUT_FREQ)? MIXER_OUT_FREQ:InputFreq;
            switch(InputFreq)
            {
            case  48000:
            case  8000:
            case  12000:
            case  24000:
            case  96000:
            case 192000:
                OPStreamParams_p->outputSampleRate  = 48000;
            break;

            case  44100:
            case  11025:
            case  22050:
            case  88200:
            case 176400:
                OPStreamParams_p->outputSampleRate  = 44100;
            break;

            case  32000:
            case  16000:
            case  64000:
            case 128000:
                OPStreamParams_p->outputSampleRate  = 32000;
            break;

            }
            /*OPStreamParams_p->outputSampleRate = InputFreq;*/
#endif

            OPStreamParams_p->outputSamplesPerTransform = (OPStreamParams_p->outputSampleRate * MSEC_PER_MIXER_TRANSFORM)/1000;
            // Convert numsamples to even
            OPStreamParams_p->outputSamplesPerTransform = (OPStreamParams_p->outputSamplesPerTransform>>1)<<1;
            MixerCtrlBlock_p->OpFreqChange = TRUE;
        }
    }
#endif
    return Error;

}

ST_ErrorCode_t STAud_CheckInputStreamParamsChange(MixerControlBlock_t *MixerCtrlBlock_p, STAud_LpcmStreamParams_t *StreamParams,U32 InputID)
{
    MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);
    U8 BitsPerSample    = (InputStreamsParams_p->bytesPerSample << 3);
    U8 NumChannels  = InputStreamsParams_p->numChannels;
    ST_ErrorCode_t  Error=ST_NO_ERROR;

    if(StreamParams->sampleBits != ConvertToAccWSCode(BitsPerSample))
    {
        MixerCtrlBlock_p->UpdateRequired[InputID] = TRUE;
        InputStreamsParams_p->bytesPerSample    = (StreamParams->sampleBits == ACC_WS32) ? 4 : 2;
        InputStreamsParams_p->bytesPerMSec      = ((InputStreamsParams_p->bytesPerSample * InputStreamsParams_p->numChannels) * (InputStreamsParams_p->sampleRate))/1000;
        InputStreamsParams_p->bytesToPause      = 0;
        InputStreamsParams_p->bytesToSkip       = 0;
    }
#ifdef ST_51XX
    if((CovertToLpcmWSCode(StreamParams->sampleBits)==ACC_LPCM_WS16le) && (InputStreamsParams_p->sampleBits != StreamParams->sampleBits))
    {
        MixerCtrlBlock_p->UpdateRequired[InputID]   = TRUE;
        InputStreamsParams_p->sampleBits            = StreamParams->sampleBits;
    }
    else if((CovertToLpcmWSCode(StreamParams->sampleBits)==ACC_LPCM_WS16) && (InputStreamsParams_p->sampleBits != StreamParams->sampleBits))
    {
        MixerCtrlBlock_p->UpdateRequired[InputID]   = TRUE;
        InputStreamsParams_p->sampleBits            = StreamParams->sampleBits;
    }
#endif

    if(StreamParams->channels != NumChannels)
    {
        MixerCtrlBlock_p->UpdateRequired[InputID]= TRUE;
        InputStreamsParams_p->numChannels  = StreamParams->channels;
        InputStreamsParams_p->bytesPerMSec = ((InputStreamsParams_p->bytesPerSample * InputStreamsParams_p->numChannels) * (InputStreamsParams_p->sampleRate))/1000;
        InputStreamsParams_p->bytesToPause = 0;
        InputStreamsParams_p->bytesToSkip  = 0;
    }


    return Error;
}

void ApplyFade(MixerControlBlock_t *MixerCtrlBlock_p, U32 FadeValue)
{
    MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[MixerCtrlBlock_p->MasterInput].FadeInfo.curFade = FadeValue;
    MixerCtrlBlock_p->UpdateRequired[MixerCtrlBlock_p->MasterInput] = TRUE;

}

void CheckAndApplyFade(MixerControlBlock_t *MixerCtrlBlock_p)
{
    U32 InputID = 0;
    int TargetFade = 0;

    for (InputID = 0; InputID < MixerCtrlBlock_p->MixerStreamsInfo.NoOfRegisteredInput; InputID++)
    {
        if (InputID != MixerCtrlBlock_p->MasterInput)
        {
            //MixerInputStreamNode_t    * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);
            STAUD_MixFadeInfo_t * FadeBlock = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID].FadeInfo);


            TargetFade = (FadeBlock->lastReceivedFade != 0xFF00)?FadeBlock->lastReceivedFade : 0;
            if (FadeBlock->curFade != TargetFade)
            {

                if (FadeBlock->curFade < TargetFade)
                {
                    if ((FadeBlock->curFade + FadeBlock->fadeSteps) > TargetFade)
                        FadeBlock->curFade = TargetFade;
                    else
                        FadeBlock->curFade += FadeBlock->fadeSteps;
                }

                if (FadeBlock->curFade > TargetFade)
                {
                    if ((FadeBlock->curFade + FadeBlock->fadeSteps) < TargetFade)
                        FadeBlock->curFade = TargetFade;
                    else
                        FadeBlock->curFade += FadeBlock->fadeSteps;
                }

                if (FadeBlock->curFade == TargetFade)
                {
                    // We have reached target fade so no need to call this function again
                    MixerCtrlBlock_p->UpdateFadeOnMaster = FALSE;
                }

                ApplyFade(MixerCtrlBlock_p, FadeBlock->curFade);
                //MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[MixerCtrlBlock_p->MasterInput].FadeInfo.curFade = FadeBlock->curFade;
                //MixerCtrlBlock_p->UpdateRequired[MixerCtrlBlock_p->MasterInput] = TRUE;

            }
        }

    }
}

void CaluclateRamp(MixerInputStreamNode_t   * InputStreamsParams_p)
{
    int TargetFade = 0;//framesInSec = 0;

    if (InputStreamsParams_p->FadeInfo.lastReceivedFade != 0xFF00)
        TargetFade = InputStreamsParams_p->FadeInfo.lastReceivedFade;

    if (InputStreamsParams_p->FadeInfo.curFade != TargetFade)
    {
        // Calulate positive steps
        //framesInSec = 1000/MSEC_PER_MIXER_TRANSFORM;
        //InputStreamsParams_p->FadeInfo.fadeSteps = ((int)(TargetFade - InputStreamsParams_p->FadeInfo.curFade))/framesInSec;
        InputStreamsParams_p->FadeInfo.fadeSteps = TargetFade - InputStreamsParams_p->FadeInfo.curFade;
        InputStreamsParams_p->FadeInfo.fadeSteps = (InputStreamsParams_p->FadeInfo.fadeSteps * MSEC_PER_MIXER_TRANSFORM)/1000;
        if (InputStreamsParams_p->FadeInfo.fadeSteps == 0)
        {
            // We could ramp to this values
            InputStreamsParams_p->FadeInfo.fadeSteps = 1;
        }
    }

}

void STAud_CheckFadeParamChange(MixerControlBlock_t *MixerCtrlBlock_p, int CurrentFade,U32 InputID)
{
    MixerInputStreamNode_t  * InputStreamsParams_p = &(MixerCtrlBlock_p->MixerStreamsInfo.InputStreamsParams[InputID]);

    if (InputStreamsParams_p->FadeInfo.curFade != CurrentFade)
    {
        STTBX_Print(("Received Fade value 0x%x\n",CurrentFade));
        if ((InputStreamsParams_p->FadeInfo.lastReceivedFade != 0xFF00) &&
            (CurrentFade != 0xFF00))
        {
            // There is no error last time and no error in current value. So apply immediately
            InputStreamsParams_p->FadeInfo.curFade = CurrentFade;
            InputStreamsParams_p->FadeInfo.lastReceivedFade = CurrentFade;
            // TBD
            // Apply FADE on master
            ApplyFade(MixerCtrlBlock_p, CurrentFade);
        }

        if ((InputStreamsParams_p->FadeInfo.lastReceivedFade == 0xFF00) &&
            (CurrentFade != 0xFF00))
        {
            InputStreamsParams_p->FadeInfo.lastReceivedFade = CurrentFade;
            // Got first valid fade after an error so caluclate ramp (up or down) to this value
            CaluclateRamp(InputStreamsParams_p);
            MixerCtrlBlock_p->UpdateFadeOnMaster = TRUE;
        }

        if ((InputStreamsParams_p->FadeInfo.lastReceivedFade != 0xFF00) &&
            (CurrentFade == 0xFF00))
        {
            InputStreamsParams_p->FadeInfo.lastReceivedFade = CurrentFade;
            // Got first invalid fade signalling an error so caluclate ramp down to 0 from curFade
            CaluclateRamp(InputStreamsParams_p);
            MixerCtrlBlock_p->UpdateFadeOnMaster = TRUE;
        }

    }
    else
    {
        if (InputStreamsParams_p->FadeInfo.lastReceivedFade == 0xFF00)
        {
            // Rare case when we recovered/receovering from invlaid fade and got same fade value
            InputStreamsParams_p->FadeInfo.lastReceivedFade = CurrentFade;
        }
    }
}

/* Register events to be sent */
ST_ErrorCode_t STAud_MixerRegEvt(const ST_DeviceName_t EventHandlerName,const ST_DeviceName_t AudioDevice, MixerControlBlock_t *ControlBlock_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_OpenParams_t EVT_OpenParams;

    /* Open the ST Event */
    Error =STEVT_Open(EventHandlerName,&EVT_OpenParams,&ControlBlock_p->EvtHandle);

    /* Register the events */
    Error|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_PCM_UNDERFLOW_EVT,&ControlBlock_p->EventIDPCMUnderFlow);
    Error|=STEVT_RegisterDeviceEvent(ControlBlock_p->EvtHandle,AudioDevice,STAUD_PCM_BUF_COMPLETE_EVT,&ControlBlock_p->EventIDPCMBufferComplete);


    return Error;
}

/* Unsubscribe the parser */
ST_ErrorCode_t STAud_MixerUnSubscribeEvt(MixerControlBlock_t *MixerCtrlBlock_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(MixerCtrlBlock_p->EvtHandle)
    {
        Error|=STEVT_Close(MixerCtrlBlock_p->EvtHandle);
        MixerCtrlBlock_p->EvtHandle = 0;
    }
    return Error;
}

ST_ErrorCode_t  STAud_MxGetOPBMHandle(STAud_MixerHandle_t   Handle, U32 OutputId, STMEMBLK_Handle_t *BMHandle_p)
{
    MixerControlBlockList_t * Mixer_p= STAud_MixerGetControlBlockFromHandle(Handle);
    ST_ErrorCode_t  Error=ST_NO_ERROR;

    if(Mixer_p)
    {
        switch(OutputId)
        {
        case 0:
            *BMHandle_p = Mixer_p->MixerControlBlock.MixerStreamsInfo.OutputStreamParams.OutBufHandle ;
            break;
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}

ST_ErrorCode_t      STAud_MxSetIPBMHandle(STAud_MixerHandle_t   Handle, U32 InputId, STMEMBLK_Handle_t BMHandle)
{
    MixerControlBlockList_t * Mixer_p= STAud_MixerGetControlBlockFromHandle(Handle);
    ST_ErrorCode_t  Error=ST_NO_ERROR;

    if(Mixer_p)
    {
        MixerControlBlock_t     * MixerCtrlBlk_p = &(Mixer_p->MixerControlBlock);

        STOS_MutexLock(MixerCtrlBlk_p->LockControlBlockMutex_p);
        switch(MixerCtrlBlk_p->CurMixerState)
        {
        case MIXER_STATE_IDLE:
        case MIXER_STATE_STOP:
            /*We can change the handle here*/
            {
                MixerStreamsInfo_t      * MixerStreamsInfo_p = &(MixerCtrlBlk_p->MixerStreamsInfo);
                if(InputId < MixerStreamsInfo_p->NoOfRegisteredInput)
                {
                    MixerStreamsInfo_p->InputStreamsParams[InputId].InMemoryBlockHandle = BMHandle;
                }
                else
                {
                    Error = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;
        default:
            /*We can't change the handle in running condition*/
            Error = STAUD_ERROR_INVALID_STATE;
                break;
        }
        STOS_MutexRelease(MixerCtrlBlk_p->LockControlBlockMutex_p);
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}

ST_ErrorCode_t  STAud_MixerSetVol(STAud_MixerHandle_t Handle, STAUD_Attenuation_t  *Attenuation_p)
{
    /* Local variables */
    ST_ErrorCode_t          error = ST_NO_ERROR;
    MME_LxMixerTransformerInitParams_t      *init_params;
    MME_LxMixerTransformerGlobalParams_t    *gbl_params;
    MixerControlBlockList_t * Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);

    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Get mixer init params */
    init_params = (MME_LxMixerTransformerInitParams_t *)Mixer_p->MixerControlBlock.MMEMixerinitParams.TransformerInitParams_p;

    /* Get the Global Parmeters from the current mixer */
    gbl_params = (MME_LxMixerTransformerGlobalParams_t *) &init_params->GlobalParams;

    /* Set the new params here */
    gbl_params->PcmParams.BassMgt.Id = ACC_RENDERER_BASSMGT_ID;
    gbl_params->PcmParams.BassMgt.StructSize = sizeof(MME_BassMgtGlobalParams_t);

    gbl_params->PcmParams.BassMgt.Apply =  ACC_MME_ENABLED;


    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LEFT ] = Attenuation_p->Left;
    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RGHT ] = Attenuation_p->Right;
    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CNTR ] = Attenuation_p->Center;
    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LFE  ] = Attenuation_p->Subwoofer;
    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LSUR ] = Attenuation_p->LeftSurround;
    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RSUR ] = Attenuation_p->RightSurround;
    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CSURL] = 0;
    gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CSURR] = 0;
    gbl_params->PcmParams.BassMgt.Volume[ACC_AUX_LEFT  ] = 0;
    gbl_params->PcmParams.BassMgt.Volume[ACC_AUX_RGHT  ] = 0;


    if(Mixer_p->MixerControlBlock.Transformer_Init)
    {
        Mixer_p->MixerControlBlock.UpdateRequired[Mixer_p->MixerControlBlock.MasterInput] = TRUE;
    }
#ifdef ST_51XX
    {
        U32 InputID;
        for (InputID = 0; InputID < Mixer_p->MixerControlBlock.MixerStreamsInfo.NoOfRegisteredInput; InputID++)
        {
            Mixer_p->MixerControlBlock.UpdateRequired[InputID] = TRUE;
        }
    }
#endif
    return error;
}

ST_ErrorCode_t  STAud_MixerGetVol(STAud_MixerHandle_t Handle, STAUD_Attenuation_t  *Attenuation_p)
{
    /* Local variables */
    ST_ErrorCode_t          error = ST_NO_ERROR;
    MME_LxMixerTransformerInitParams_t      *init_params;
    MME_LxMixerTransformerGlobalParams_t    *gbl_params;
    MixerControlBlockList_t * Mixer_p = STAud_MixerGetControlBlockFromHandle(Handle);

    if(Mixer_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Get mixer init params */
    init_params = (MME_LxMixerTransformerInitParams_t *)Mixer_p->MixerControlBlock.MMEMixerinitParams.TransformerInitParams_p;

    /* Get the Global Parmeters from the current mixer */
    gbl_params = (MME_LxMixerTransformerGlobalParams_t *) &init_params->GlobalParams;

    /* Get the params here */
    Attenuation_p->Left = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LEFT ] ;
    Attenuation_p->Right = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RGHT ] ;
    Attenuation_p->Center = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CNTR ] ;
    Attenuation_p->Subwoofer = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LFE ] ;
    Attenuation_p->LeftSurround = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LSUR];
    Attenuation_p->RightSurround = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RSUR] ;

    return error;
}

ST_ErrorCode_t MixerReleaseMemory(MixerControlBlockList_t * ControlBlock_p)
{
    ST_ErrorCode_t  Error=ST_NO_ERROR;

    if(ControlBlock_p)
    {
        MixerControlBlock_t *Control_p = &(ControlBlock_p->MixerControlBlock);
        ST_Partition_t          * CPUPartition = Control_p->DriverPartition;

        if(Control_p->MMEMixerinitParams.TransformerInitParams_p)
        {
            memory_deallocate(CPUPartition, Control_p->MMEMixerinitParams.TransformerInitParams_p);
            Control_p->MMEMixerinitParams.TransformerInitParams_p = NULL;
        }

        Error |= STAud_MixerDeInitInputQueueParams(Control_p);
        Error |= STAud_MixerDeInitOutputQueueParams(Control_p);

        Error |= STAud_MixerUnSubscribeEvt(Control_p);

        if(Control_p->MixerTaskSem_p)
        {
            STOS_SemaphoreDelete(NULL,Control_p->MixerTaskSem_p);
            Control_p->MixerTaskSem_p = NULL;
        }

        if(Control_p->LockControlBlockMutex_p)
        {
            STOS_MutexDelete(Control_p->LockControlBlockMutex_p);
            Control_p->LockControlBlockMutex_p = NULL;
        }

        if(Control_p->CmdCompletedSem_p)
        {
            STOS_SemaphoreDelete(NULL,Control_p->CmdCompletedSem_p);
            Control_p->CmdCompletedSem_p = NULL;
        }

        if(Control_p->LxCallbackSem_p)
        {
            STOS_SemaphoreDelete(NULL,Control_p->LxCallbackSem_p);
            Control_p->LxCallbackSem_p = NULL;
        }

        memory_deallocate(CPUPartition,ControlBlock_p);
    }

    return Error;
}



