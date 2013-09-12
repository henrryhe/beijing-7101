/********************************************************************************
 *   Filename       : aud_mmeaudiostreamdecoder.c                               *
 *   Start          : 02.09.2005                                                *
 *   By             : Chandandeep Singh Pabla                                   *
 *   Contact        : chandandeep.pabla@st.com                                  *
 *   Description    : The file contains the streaming MME based Audio Decoder   *
 *                      APIs that will be exported to the Modules. It will also *
 *                      have decoder task to perform actual decoding            *
 ********************************************************************************
 */

/* {{{ Includes */
//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include <string.h>
#include <ctype.h>
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_mmeaudiostreamdecoder.h"
#include "aud_mmeaudiodecoderpp.h"
#include "acc_multicom_toolbox.h"
/* }}} */

/* Decoder Instance Array */
ST_DeviceName_t Decoder_DeviceName[] = {"DEC0","DEC1","DEC2"};

/* {{{ Global Variables */

/* Initialized Decoder list. New decoder will be added to this list on initialization. */
DecoderControlBlockList_t       *DecoderQueueHead_p = NULL;

#if ENABLE_START_UP_PATTERN_GENERATION || ENABLE_START_UP_PATTERN_GENERATION_FOR_TRANSCODER
/* Only Audio test certification */
#define START_UP_PATTERN_LENGTH_IN_SAMPLES 192 * 2  // Should be a multiple of 192

char startuppattern2Channel[]="  PL  AY  FI  LE";
char startuppattern10Channel[]="  PL  PL  PL  PL  PL  AY  AY  AY  AY  AY  FI  FI  FI  FI  FI  LE  LE  LE  LE  LE";
#endif

#if ENABLE_EOF_PATTERN_GENERATION
#define EOF_PATTERN_LENGTH_IN_SAMPLES      192 * 2  // Should be a multiple of 192
char eofpattern[]    ="STOPFILE";
#endif
#define REUSE_NULL_TRANSCODED_BUFFERS 1         // This flag should only be set for certification
/* }}} */

/**************************************************************************************************
 *  Function name   : STAud_DecInit
 *  Arguments       :
 *       IN         :
 *       OUT        : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 *  Description     : This function will create the decoder task, initialize MME decoders's In/out
 *                      paramaters and will update main HAL structure with current values
 **************************************************************************************************/
ST_ErrorCode_t  STAud_DecInit(const ST_DeviceName_t DeviceName,
                           const STAud_DecInitParams_t *InitParams)
{
    /* Local variables */
    ST_ErrorCode_t              error = ST_NO_ERROR;
    DecoderControlBlockList_t   *DecCtrlBlock_p = NULL;
    partition_t                 *DriverPartition = NULL;
    DecoderControlBlock_t       *TempDecBlock;
    TARGET_AUDIODECODER_INITPARAMS_STRUCT   *init_params;
    TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

    /* First Check if this decoder is not already open */
    if(!STAud_DecIsInit(DeviceName))
    {
        /* Decoder not already initialized. So do the initialization */

        /* Allocate memory for new Decoder control block */
        DriverPartition = (partition_t *)InitParams->DriverPartition;

        DecCtrlBlock_p = (DecoderControlBlockList_t *)memory_allocate(
        DriverPartition,
        sizeof(DecoderControlBlockList_t));

        if(DecCtrlBlock_p == NULL)
        {
            /* Error No memory available */
            return ST_ERROR_NO_MEMORY;
        }

        /* Reset the structure */
        memset((void *)DecCtrlBlock_p, 0,sizeof(DecoderControlBlockList_t));

        /* Fill in the details of our new control block */
        DecCtrlBlock_p->DecoderControlBlock.Handle    = 0;

        strncpy(DecCtrlBlock_p->DecoderControlBlock.DeviceName,DeviceName,ST_MAX_DEVICE_NAME_TOCOPY);

        DecCtrlBlock_p->DecoderControlBlock.InitParams = *InitParams;

        /* Initialize MME Structure */
        TempDecBlock = &DecCtrlBlock_p->DecoderControlBlock;

        /* Initialize the Decoder Capability structure */
        error = STAud_InitDecCapability(TempDecBlock);
        if( error != ST_NO_ERROR)
        {
            /* Problem with LX F/W or we passed wrong parameter to MME_GetTransformerCapability() */
            /*Free the Control Block */
            memory_deallocate(DriverPartition,DecCtrlBlock_p);
            return error;
        }

        TempDecBlock->MMEinitParams.Priority = MME_PRIORITY_NORMAL;
        TempDecBlock->MMEinitParams.StructSize = sizeof (MME_TransformerInitParams_t);
        TempDecBlock->MMEinitParams.Callback = &TransformerCallback_AudioDec;
        TempDecBlock->MMEinitParams.CallbackUserData = NULL; /* To be Initialized in STAud_DecStart */
        TempDecBlock->MMEinitParams.TransformerInitParamsSize = sizeof(TARGET_AUDIODECODER_INITPARAMS_STRUCT);
        TempDecBlock->MMEinitParams.TransformerInitParams_p = memory_allocate(DriverPartition,TempDecBlock->MMEinitParams.TransformerInitParamsSize);
        if(TempDecBlock->MMEinitParams.TransformerInitParams_p == NULL)
        {
            /*deallocate the control block*/
            memory_deallocate(DriverPartition,DecCtrlBlock_p);
            return ST_ERROR_NO_MEMORY;
        }
        memset((void *)TempDecBlock->MMEinitParams.TransformerInitParams_p, 0, TempDecBlock->MMEinitParams.TransformerInitParamsSize);

        /* Create the Decoder Task Semaphore */
        TempDecBlock->DecoderTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);
        /* Create the Control Block Mutex */
        TempDecBlock->LockControlBlockMutex_p = STOS_MutexCreateFifo();

        /* Create the Command Competion Semaphore */
        TempDecBlock->CmdCompletedSem_p = STOS_SemaphoreCreateFifo(NULL,0);

        /* Create the Post processing Command list Mutex */
        TempDecBlock->PPCmdMutex_p = STOS_MutexCreateFifo();

        TempDecBlock->NumChannels = InitParams->NumChannels;

        /* Initialize and Allocate Decoder Queue's buffer pointer */
        error |=STAud_DecInitDecoderQueueInParams(TempDecBlock, InitParams->AVMEMPartition);
        error |=STAud_DecInitDecoderQueueOutParams(TempDecBlock, InitParams->AVMEMPartition);
        STAud_DecInitDecoderFrameParams(TempDecBlock);

        /* Init Mute param */
#ifdef MSPP_PARSER
#ifdef ST_OSWINCE
        TempDecBlock->PcmProcess.Mute = TRUE;
#else
        TempDecBlock->PcmProcess.Mute = FALSE;
#endif
#else
        TempDecBlock->PcmProcess.Mute = FALSE;
#endif

        /* Set the Decoder Starting State */
        TempDecBlock->CurDecoderState = MME_DECODER_STATE_IDLE;
        TempDecBlock->NextDecoderState = MME_DECODER_STATE_IDLE;

        /*Set Default Beep Tone Frequency */
        TempDecBlock->BeepToneFrequency = 0x4B; /*default is being 440 Hz*/

        /* Set Input processing state */
        TempDecBlock->DecInState = MME_DECODER_INPUT_STATE_IDLE;

        /* Force default values of PP params*/
        TempDecBlock->HoldCompressionMode.Apply = FALSE;
        TempDecBlock->HoldDynamicRange.Apply    = FALSE;
        TempDecBlock->HoldStereoMode.Apply      = FALSE;
        TempDecBlock->HoldDownMixParams.Apply   = FALSE;

        /*Reset The transformer restart required*/
        TempDecBlock->TransformerRestart = TRUE;
        TempDecBlock->streamingTransformerFirstTransfer = FALSE;
        TempDecBlock->StreamingEofFound = FALSE;
        TempDecBlock->decoderInByPassMode = FALSE;

        TempDecBlock->DDPOPSetting = 0;
        TempDecBlock->SBRFlag           = ACC_MME_FALSE;

        /* Reset Decoder queues */
        TempDecBlock->DecoderInputQueueFront = TempDecBlock->DecoderInputQueueTail=0;
        TempDecBlock->DecoderOutputQueueFront = TempDecBlock->DecoderOutputQueueTail=0;

        STOS_TaskCreate(STAud_DecTask_Entry_Point, &DecCtrlBlock_p->DecoderControlBlock, NULL,
                        STREAMING_AUDIO_DECODER_TASK_STACK_SIZE, NULL, NULL, &TempDecBlock->DecoderTask,
                        NULL, STREAMING_AUDIO_DECODER_TASK_PRIORITY, DeviceName, 0);

        if (TempDecBlock->DecoderTask == NULL)
        {
            /* Error */
            /*deallocate the TransformerInitParams_p*/
            memory_deallocate(DriverPartition,TempDecBlock->MMEinitParams.TransformerInitParams_p);

            /* Terminate Decoder Semaphore */
            STOS_SemaphoreDelete(NULL,TempDecBlock->DecoderTaskSem_p);

            /* Delete Mutex */
            STOS_MutexDelete(TempDecBlock->LockControlBlockMutex_p);

            /* Delete Mutex - PP cmd*/
            STOS_MutexDelete(TempDecBlock->PPCmdMutex_p);

            /* Terminate Cmd Completion Semaphore */
            STOS_SemaphoreDelete(NULL,TempDecBlock->CmdCompletedSem_p);

            /*deallocate the control block*/
            memory_deallocate(DriverPartition,DecCtrlBlock_p);
            return ST_ERROR_NO_MEMORY;
        }

        /* Get Buffer Manager Handles */
        DecCtrlBlock_p->DecoderControlBlock.InMemoryBlockManagerHandle = InitParams->InBufHandle;
        DecCtrlBlock_p->DecoderControlBlock.OutMemoryBlockManagerHandle0 = InitParams->OutBufHandle0;
        DecCtrlBlock_p->DecoderControlBlock.OutMemoryBlockManagerHandle1 = InitParams->OutBufHandle1;

        /* Only Initialize with Buffer Manager Params. We will register/unregister with Block manager in DRStart/DRStop */
        /* As Consumer of incoming data */
        TempDecBlock->consumerParam_s.Handle = (U32)DecCtrlBlock_p;
        TempDecBlock->consumerParam_s.sem    = DecCtrlBlock_p->DecoderControlBlock.DecoderTaskSem_p;

        /* As Producer of outgoing data */
        TempDecBlock->ProducerParam_s.Handle = (U32)DecCtrlBlock_p;
        TempDecBlock->ProducerParam_s.sem    = DecCtrlBlock_p->DecoderControlBlock.DecoderTaskSem_p;

        /* Init Global update param list */
        TempDecBlock->UpdateGlobalParamList = NULL;

        /* Get hold of PCM Params settings.*/
        init_params = TempDecBlock->MMEinitParams.TransformerInitParams_p;

        /* Get the Global Parmeters from the current decoder */
        gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

        /* Initialize decoder level PCM processing settings */
        acc_setAudioDecoderPcmParams(&gbl_params->PcmParams, (U32)TempDecBlock->NumChannels);

        /* Get hold of PCM Params */
        TempDecBlock->DecPcmProcessing = &gbl_params->PcmParams;

        /* Update the intitialized Decoder queue with current control block */
        STAud_DecQueueAdd(DecCtrlBlock_p);
    }
    else
    {
        /* Decoder already initialized. */
        error = ST_ERROR_ALREADY_INITIALIZED;
    }

    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_DecInit: Failed to Init Successfully\n "));
    }
    else
    {
        STTBX_Print(("STAud_DecInit: Successful \n "));
    }

    return error;
}

/**************************************************************************************************
 *  Function name       : STAud_DecTerm
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will terminate the decoder task, Free MME decoders's In/out
 *                    paramaters and will update main HAL structure with current values
**************************************************************************************************/
ST_ErrorCode_t STAud_DecTerm(STAud_DecHandle_t  Handle)
{
    /* Local Variable */
    ST_ErrorCode_t                      error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p;
    partition_t                                 *DriverPartition = NULL;
    task_t                      *TempTask_p;

    STTBX_Print(("STAud_DecTerm: Entered\n "));
    /* Obtain the Decoder control block associated with the device name */
    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);

    if(Dec_p == NULL)
    {
        /* No sunch Device found */
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Terminate Decoder Task */
    TempTask_p = Dec_p->DecoderControlBlock.DecoderTask;

    error |= STAud_DecSendCommand(MME_DECODER_STATE_TERMINATE, &Dec_p->DecoderControlBlock);
    if(error == STAUD_ERROR_INVALID_STATE)
    {
        STTBX_Print(("STAud_DecTerm: Error !!!\n "));
        return error;
    }

    STOS_TaskWait(&TempTask_p,TIMEOUT_INFINITY);
    STOS_TaskDelete(TempTask_p,NULL,NULL,NULL);

    DriverPartition = Dec_p->DecoderControlBlock.InitParams.DriverPartition;

    /* Free Decoder queue Structures */
    error |= STAud_DecDeInitDecoderQueueInParam(&Dec_p->DecoderControlBlock);
    error |= STAud_DecDeInitDecoderQueueOutParam(&Dec_p->DecoderControlBlock);
    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_DecTerm: Failed to deallocate queue\n "));
    }

    /* Free the MME Param Structure */
    memory_deallocate(DriverPartition, Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p);

    /* Terminate Decoder Semaphore */
    STOS_SemaphoreDelete(NULL,Dec_p->DecoderControlBlock.DecoderTaskSem_p);
    /* Delete Mutex */
    STOS_MutexDelete(Dec_p->DecoderControlBlock.LockControlBlockMutex_p);

    /* Delete Mutex - PP cmd*/
    STOS_MutexDelete(Dec_p->DecoderControlBlock.PPCmdMutex_p);

    /* Free any Update Param command list */
    while(Dec_p->DecoderControlBlock.UpdateGlobalParamList != NULL)
    {
        STAud_DecPPRemCmdFrmList(&Dec_p->DecoderControlBlock);
    }

    /* Terminate Cmd Completion Semaphore */
    STOS_SemaphoreDelete(NULL,Dec_p->DecoderControlBlock.CmdCompletedSem_p);

    /* Remove the Control Block from Queue */
    STAud_DecQueueRemove(Dec_p);

    /*Free the Control Block */
    memory_deallocate(DriverPartition,Dec_p);

    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_DecTerm: Failed to Terminate Successfully\n "));
    }
    else
    {
        STTBX_Print(("STAud_DecTerm: Successfully\n "));
    }
    return error;
}


/******************************************************************************
 *  Function name       : STAud_DecOpen
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will Open the decoder
 ***************************************************************************** */
ST_ErrorCode_t  STAud_DecOpen(const ST_DeviceName_t DeviceName, STAud_DecHandle_t *Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p;

    /* Get the desired control block */
    Dec_p = STAud_DecGetControlBlockFromName(DeviceName);

    if(Dec_p == NULL)
    {
        /* Device not initialized */
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    /* Check nobody else has already opened it */
    if (Dec_p->DecoderControlBlock.Handle == 0)
    {
        /* Set and return Handle */
        Dec_p->DecoderControlBlock.Handle = (STAud_DecHandle_t)Dec_p;
        *Handle = Dec_p->DecoderControlBlock.Handle;
    }

    STTBX_Print(("STAud_DecOpen: Successful \n "));
    return error;
}


/******************************************************************************
 *  Function name       : STAud_DecClose
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will close the task
 ***************************************************************************** */
ST_ErrorCode_t STAud_DecClose()
{
    return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_DecSetStreamParams(STAud_DecHandle_t               Handle,
                                        STAUD_StreamParams_t * StreamParams_p)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;
    STAUD_StreamParams_t *DecStreamParams_p = NULL;
    DecoderControlBlock_t               *DecCtr_p = NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    DecCtr_p = &(Dec_p->DecoderControlBlock);
    DecStreamParams_p = &(DecCtr_p->StreamParams);

    /* Set decoder specidifc parameters */
    if(StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_ALSA ||
    StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_WAVE ||
    StreamParams_p->StreamContent == STAUD_STREAM_CONTENT_AIFF)
    {
        DecStreamParams_p->StreamContent        = STAUD_STREAM_CONTENT_PCM;
    }
    else
    {
        DecStreamParams_p->StreamContent = StreamParams_p->StreamContent;
    }

    DecCtr_p->StreamContent = StreamParams_p->StreamContent;/*Preserve original stream content in case of PCM*/
    DecStreamParams_p->StreamType = StreamParams_p->StreamType;
    DecStreamParams_p->SamplingFrequency = StreamParams_p->SamplingFrequency;

    return Error;
}

/**************************************************************************************************
 *  Function name       : STAud_DecStart
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will send init and play cmd to Decoder Task. This
 *                                 function should be called from DRStart() function
**************************************************************************************************/
ST_ErrorCode_t STAud_DecStart(STAud_DecHandle_t         Handle)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Send the Start command to decoder. */
    Error = STAud_DecSendCommand(MME_DECODER_STATE_START, &Dec_p->DecoderControlBlock);

    if(Error == ST_NO_ERROR)
    {
        /* Wait on sema for START completion */
        STOS_SemaphoreWait(Dec_p->DecoderControlBlock.CmdCompletedSem_p);
    }

    if ((Dec_p->DecoderControlBlock.Transformer_Init==FALSE) &&  !(Dec_p->DecoderControlBlock.decoderInByPassMode))
    {
        Error = STAUD_ERROR_DECODER_START;
    }

    return Error;
}

/**************************************************************************************************
 *  Function name       : STAud_DecStop
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will Send the Stop cmd to Decoder Task. This
 *                                 function should be called from DRStop() function
**************************************************************************************************/
ST_ErrorCode_t  STAud_DecStop(STAud_DecHandle_t  Handle)
{

    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    STTBX_Print(("STAud_DecStop: Called \n "));

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Dec_p->DecoderControlBlock.Transformer_Init==FALSE) && !(Dec_p->DecoderControlBlock.decoderInByPassMode))
    {
        return STAUD_ERROR_DECODER_STOP;
    }

    /* Send the Stop command to decoder. */
    Error = STAud_DecSendCommand(MME_DECODER_STATE_STOP, &Dec_p->DecoderControlBlock);

    Dec_p->DecoderControlBlock.decoderInByPassMode = FALSE;

    if(Error == ST_NO_ERROR)
    {
        /* Wait on sema for START completion */
        STOS_SemaphoreWait(Dec_p->DecoderControlBlock.CmdCompletedSem_p);
    }

    STTBX_Print(("STAud_DecStop: Successful\n "));
    return Error;
}

/**************************************************************************************************
 *  Function name   : STAud_DecMute
 *  Arguments       :
 *       IN                 :BOOL - Mute if TRUE ,Unmute for FALSE
 *       OUT        : Error status
 *       INOUT      :
 *  Return          : ST_ErrorCode_t
 **************************************************************************************************/
ST_ErrorCode_t  STAud_DecMute(STAud_DecHandle_t         Handle, BOOL Mute)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Set decoder mute parameter */
    Dec_p->DecoderControlBlock.PcmProcess.Mute = Mute;

    return Error;
}

/**************************************************************************************************
 *  Function name       : STAud_DecGetSamplingFrequency
 *  Arguments           :
 *       IN                     :       Variable from application to store frequency
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This
 **************************************************************************************************/
ST_ErrorCode_t STAud_DecGetSamplingFrequency(STAud_DecHandle_t          Handle,
                                                U32 * SamplingFrequency_p)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Dec_p->DecoderControlBlock.CurDecoderState!=MME_DECODER_STATE_PLAYING)||(Dec_p->DecoderControlBlock.SamplingFrequency==ACC_FS_reserved))
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    *SamplingFrequency_p = CovertFsCodeToSampleRate(Dec_p->DecoderControlBlock.SamplingFrequency);
    return Error;
}

/**************************************************************************************************
 *  Function name       : STAud_DecGetCapability
 *  Arguments           :
 *       IN                     :       Dec Handle
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Returns the decoder capabilities
 **************************************************************************************************/
ST_ErrorCode_t  STAud_DecGetCapability(STAud_DecHandle_t        Handle,
                                        STAUD_DRCapability_t * Capability_p)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Set decoder Capability parameter */
    *Capability_p = Dec_p->DecoderControlBlock.DecCapability;
    return Error;
}
/**************************************************************************************************
*                       DECODER TASK                                                              *
**************************************************************************************************/
/**************************************************************************************************
 *  Function name       : STAud_DecTask_Entry_Point
 *  Arguments           :
 *       IN                     :
 *       OUT                    : void
 *       INOUT          :
 *  Return              : void
 *  Description         : This function is the entry point for decoder task. Ihe decoder task
 *                         will be in different states depending of the command received from
 *                         upper/lower layers
 **************************************************************************************************/
void    STAud_DecTask_Entry_Point(void* params)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MME_ERROR   Status;

    /* Get the Decoder control Block for this task */
    DecoderControlBlock_t       *ControlBlock = (DecoderControlBlock_t*)params;

    STOS_TaskEnter(params);
    while(1)
    {

        /* Wait on sema for start command */
        STOS_SemaphoreWait(ControlBlock->DecoderTaskSem_p);

        /* Check if some command is recevied. We check this by looking for next desired state of decoder */
        if(ControlBlock->CurDecoderState != ControlBlock->NextDecoderState)
        {
            /* Decoder state need to be changed */
            STOS_MutexLock(ControlBlock->LockControlBlockMutex_p);
            ControlBlock->CurDecoderState = ControlBlock->NextDecoderState;
            STOS_MutexRelease(ControlBlock->LockControlBlockMutex_p);
        }

        switch(ControlBlock->CurDecoderState)
        {

            case MME_DECODER_STATE_IDLE:
            break;

            case MME_DECODER_STATE_START:

            Status = STAud_DecSetAndInitDecoder(ControlBlock);
            if(Status != MME_SUCCESS)
            {
                STTBX_Print(("Audio Deocder:Init failure\n"));
            }
            else
            {
                STTBX_Print(("Audio Deocder:Init Success\n"));
            }

            Status |= ResetQueueParams(ControlBlock);
            /* Decoder Initialized. Now Send Play Command to Decoder Task i.e. this task itself */
            if(Status ==ST_NO_ERROR)
            {
                STAud_DecSendCommand(MME_DECODER_STATE_PLAYING, ControlBlock);
            }
            else
            {
                STAud_DecSendCommand(MME_DECODER_STATE_IDLE, ControlBlock);
            }
            break;

            case MME_DECODER_STATE_PLAYING:

            if (ControlBlock->decoderInByPassMode)
            {
                /*  Bypass mode - If the companion bin doesn't support the requested decoder.
                    Eg - if AC3 is removed from bin, we go to bypass mode. User wil still get
                    output on SPDIF in compressed mode
                */
                ControlBlock->DecInState = MME_DECODER_INPUT_STATE_BYPASS;
            }
            else
            {
                /* Process incoming Data */
                if (ControlBlock->UseStreamingTransformer)
                {
                    Error = STAud_DecProcessInput(ControlBlock);
                }
                else
                {
                    while (Error == ST_NO_ERROR)
                    {
                        Error = STAud_DecProcessInput(ControlBlock);
                    }
                    Error = ST_NO_ERROR;
                }
            }

            if(Error == ST_NO_ERROR)
            {
                /* Process outgoing Data */
                /* Currently being done in Decoder callback but shall be moved here in future */
                //STAud_DecProcessOutput();

            }
            break;

            case MME_DECODER_STATE_STOP:

            Error = STAud_DecStopDecoder(ControlBlock);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_DecStopDecoder::Failed %x \n",Error));
            }
            /* Decoder Stoped. Now Send Stopped Command to Decoder Task i.e. this task itself */
            STAud_DecSendCommand(MME_DECODER_STATE_STOPPED, ControlBlock);

            break;

            case MME_DECODER_STATE_STOPPED:
            /* Nothing to do in stopped state */
            STTBX_Print(("STAud_Decoder::STOPPED state \n"));
            break;

            case MME_DECODER_STATE_TERMINATE:
            STOS_TaskExit(params);
#if defined( ST_OSLINUX )
            return;
#else
            task_exit(1);
#endif
            break;

            default:
            STTBX_Print(("STAud_Decoder::Invalid state \n"));
            break;
        }
    }
}


/******************************************************************************
*                       LOCAL FUNCTIONS                                        *
******************************************************************************/
ST_ErrorCode_t  ResetQueueParams(DecoderControlBlock_t  *ControlBlock)
{
    U32 i,j;
    // Reset Input Queue params
    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        for (j = 0; j < NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            /* We will set the Page_p to point to the compressed buffer, received from
            Parser unit, in Process_Input function */
            ControlBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->Page_p = NULL;

            /* Data Buffers initialisation */
            ControlBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->Size = 0;//COMPRESSED_BUFFER_SIZE;//2048;  update by the input buffer size
            ControlBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->BytesUsed = 0;
            ControlBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->FlagsIn = (unsigned int)MME_DATA_CACHE_COHERENT;
            ControlBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->FlagsOut = 0;
        }

        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->StructSize    = sizeof (MME_DataBuffer_t);
        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->UserData_p    = NULL;
        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->Flags         = 0;
        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->StreamNumber  = 0;
        /* TBD needs to be updated */
        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->NumberOfScatterPages = 0;//NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM;
        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->ScatterPages_p= ControlBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[0];
        /* TempDecBlock->DecoderQueue[i].DecoderBufIn.BufferIn_p->TotalSize     = TempDecBlock->DecoderQueue[i].DecoderBufIn.BufferIn_p->ScatterPages_p->Size;*/
        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->TotalSize                   = 0;
        ControlBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->StartOffset         = 0;

        ControlBlock->DecoderInputQueue[i].BufferParams.StructSize = sizeof(MME_LxAudioDecoderBufferParams_t);

        for (j = 0; j < ACC_MAX_BUFFER_PARAMS; j++)
            ControlBlock->DecoderInputQueue[i].BufferParams.BufferParams[j] = 0;
    }


    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        for (j = 0; j < NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            /* We will set the Page_p to point to the Empty PCM buffer, received from
            Consumer unit, in Process_Input function */
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.ScatterPageOut_p[j]->Page_p = NULL;

            /* Data Buffers initialisation */
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.ScatterPageOut_p[j]->Size = 0;
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.ScatterPageOut_p[j]->BytesUsed = 0;
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.ScatterPageOut_p[j]->FlagsIn = (unsigned int)MME_DATA_CACHE_COHERENT;
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.ScatterPageOut_p[j]->FlagsOut = 0;

            ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.ScatterPageOut_p[j]->Page_p = NULL;

            /* Data Buffers initialisation */
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.ScatterPageOut_p[j]->Size = 0;
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.ScatterPageOut_p[j]->BytesUsed = 0;
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.ScatterPageOut_p[j]->FlagsIn = (unsigned int)MME_DATA_CACHE_COHERENT;
            ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.ScatterPageOut_p[j]->FlagsOut = 0;
        }

        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->StructSize    = sizeof (MME_DataBuffer_t);
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->UserData_p    = NULL;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->Flags         = 0;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->StreamNumber  = 0;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->NumberOfScatterPages = 0;//NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->ScatterPages_p= ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.ScatterPageOut_p[0];
        //TempDecBlock->DecoderOutputQueue[i].DecoderBufOut.BufferOut_p->TotalSize     = TempDecBlock->DecoderInputQueue[i].DecoderBufOut.ScatterPageOut_p->Size;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->TotalSize     = 0;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut0.BufferOut_p->StartOffset   = 0;

        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->StructSize    = sizeof (MME_DataBuffer_t);
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->UserData_p    = NULL;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->Flags         = 0xCD; // Modified for DDplus transcoding 0;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->StreamNumber  = 0;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->NumberOfScatterPages = 0;//NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->ScatterPages_p= ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.ScatterPageOut_p[0];
        //TempDecBlock->DecoderOutputQueue[i].DecoderBufOut.BufferOut_p->TotalSize     = TempDecBlock->DecoderInputQueue[i].DecoderBufOut.ScatterPageOut_p->Size;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->TotalSize     = 0;
        ControlBlock->DecoderOutputQueue[i].DecoderBufOut1.BufferOut_p->StartOffset   = 0;
        //TempDecBlock->DecoderOutputQueue[i].NumberOutputScatterPages   = 0;

    }

    ControlBlock->SamplingFrequency = ACC_FS_reserved;
    ControlBlock->TranscodedSamplingFrequency = ACC_FS_reserved;
    ControlBlock->AudioCodingMode = ACC_MODE_undefined_3F;

    return ST_NO_ERROR;
}

ST_ErrorCode_t  STAud_DecHandleInput(DecoderControlBlock_t      *ControlBlock)
{
    ST_ErrorCode_t      status = ST_NO_ERROR;
    U32                         QueueTailIndex,Index;
    BOOL                        SendMMEComamand = TRUE;
    MME_DataBuffer_t    *BufferIn_p=NULL;
    BOOL                        WmaLastBlock=FALSE;
    BOOL            StreamingLastBlock=FALSE;
    STAud_LpcmStreamParams_t    *StreamParams;
    STAud_DtsStreamParams_t             *DTSStreamParams;
    DecoderInputCmdParam_t              *InputBufferToSend=NULL;

    /* Geting Input for streaming transformer */
    while (SendMMEComamand)
    {
        SendMMEComamand = FALSE;
        if (ControlBlock->DecoderInputQueueTail - ControlBlock->DecoderInputQueueFront < STREAMING_DECODER_QUEUE_SIZE)
        {
            /* We have space to send another command */
            QueueTailIndex = ControlBlock->DecoderInputQueueTail%STREAMING_DECODER_QUEUE_SIZE;

            /* Get the command structure to fill */
            InputBufferToSend = ControlBlock->CurrentInputBufferToSend = &ControlBlock->DecoderInputQueue[QueueTailIndex];
            BufferIn_p = ControlBlock->CurrentInputBufferToSend->DecoderBufIn.BufferIn_p;

            if((ControlBlock->StreamParams.StreamContent != STAUD_STREAM_CONTENT_BEEP_TONE) &&
                (ControlBlock->StreamParams.StreamContent != STAUD_STREAM_CONTENT_PINK_NOISE))
            {
                while ((BufferIn_p->NumberOfScatterPages != ControlBlock->NumInputScatterPagesPerTransform) &&
                        (MemPool_GetIpBlk(ControlBlock->InMemoryBlockManagerHandle, (U32 *)&InputBufferToSend->InputBlockFromProducer[BufferIn_p->NumberOfScatterPages],ControlBlock->Handle) == ST_NO_ERROR))
                {
                    Index = BufferIn_p->NumberOfScatterPages;
                    STAud_DecSetStreamInfo(ControlBlock, InputBufferToSend->InputBlockFromProducer[Index]);
                    BufferIn_p->ScatterPages_p[Index].Page_p = (void*)InputBufferToSend->InputBlockFromProducer[Index]->MemoryStart;
                    BufferIn_p->ScatterPages_p[Index].Size = InputBufferToSend->InputBlockFromProducer[Index]->Size;
                    BufferIn_p->TotalSize += InputBufferToSend->InputBlockFromProducer[Index]->Size;
                    BufferIn_p->NumberOfScatterPages++;
                }
            }
            else
            {
                /* Fake for input buffer here for beep tone and pink Noise */
                while (BufferIn_p->NumberOfScatterPages != ControlBlock->NumInputScatterPagesPerTransform)
                {
                    Index = BufferIn_p->NumberOfScatterPages;
                    BufferIn_p->ScatterPages_p[Index].Page_p = (void*)NULL;
                    BufferIn_p->ScatterPages_p[Index].Size = 0;
                    BufferIn_p->TotalSize += 0;
                    BufferIn_p->NumberOfScatterPages++;
                }
            }
            if ((BufferIn_p->NumberOfScatterPages != ControlBlock->NumInputScatterPagesPerTransform) &&
                (!ControlBlock->UseStreamingTransformer))
            {
                /* Could not get block for a Frame Based transformer*/
                return ST_ERROR_NO_MEMORY;
            }
            if (!(ControlBlock->UseStreamingTransformer))
            {
                /* For LPCM streams, Update Transformer params, if there is some change in stream properties */
                if((ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_LPCM)||
                    (ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_LPCM_DVDA) ||
                    (ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_PCM))
                {
                    StreamParams = (STAud_LpcmStreamParams_t*)InputBufferToSend->InputBlockFromProducer[0]->AppData_p; // Check only for the first block as there is 1 Block / frame for frame based transformer
                    /* Check if there is some change in stream parameters */
                    if(StreamParams && (StreamParams->ChangeSet == TRUE))
                    {
                        HandleTransformUpdate(ControlBlock, StreamParams,ControlBlock->StreamParams.StreamContent);
                        StreamParams->ChangeSet = FALSE;
                    }
                }
                else if(ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_DTS)
                {
                    DTSStreamParams = (STAud_DtsStreamParams_t*)InputBufferToSend->InputBlockFromProducer[0]->AppData_p; // Check only for the first block as there is 1 Block / frame for frame based transformer
                    if(DTSStreamParams && (DTSStreamParams->ChangeSet == TRUE))
                    /*if(DTSStreamParams && (DTSStreamParams->ChangeSet == TRUE))
                    {
                    HandleDtsTransformUpdate(ControlBlock, DTSStreamParams);
                    DTSStreamParams->ChangeSet = FALSE;
                    }*/
                    if(DTSStreamParams)
                    {
                        BufferIn_p->ScatterPages_p[0].FlagsOut = DTSStreamParams->DelayLossLess;
                        BufferIn_p->ScatterPages_p[0].FlagsIn  = DTSStreamParams->CoreSize;
                    }
                }
                /* Got input for a Frame Based transformer */
                ControlBlock->DecoderInputQueueTail++;
            }

            if ((BufferIn_p->NumberOfScatterPages == ControlBlock->NumInputScatterPagesPerTransform) &&
                (ControlBlock->UseStreamingTransformer))
            {
                /* Condition to fill PTS info for stream based decoders*/
                if (InputBufferToSend->InputBlockFromProducer[0]->Flags & PTS_VALID)
                {
                    /* We consider PTS with only first scatter page as valid as currently
                    there is no mechaism t send PTS with next scatter pages */
                    //InputBufferToSend->FrameStatus.PTSflag  = 0x1; // LSB bit is for PTS Valid
                    //InputBufferToSend->FrameStatus.PTSflag |= InputBufferToSend->InputBlockFromProducer[0]->Data[PTS33_OFFSET] << 15;
                    //InputBufferToSend->FrameStatus.PTS      = InputBufferToSend->InputBlockFromProducer[0]->Data[PTS_OFFSET];

                    //sending PTS values in buffer params instead of frame status
                    ControlBlock->CurrentInputBufferToSend->BufferParams.BufferParams[0]  = 0x1 << 16; // LSB bit is for PTS Valid
                    ControlBlock->CurrentInputBufferToSend->BufferParams.BufferParams[0] |= InputBufferToSend->InputBlockFromProducer[0]->Data[PTS33_OFFSET] << 23;
                    ControlBlock->CurrentInputBufferToSend->BufferParams.BufferParams[1] = InputBufferToSend->InputBlockFromProducer[0]->Data[PTS_OFFSET];
                }
                else
                {
                    //explicit set to zero
                    //ControlBlock->CurrentInputBufferToSend->BufferParams.BufferParams[0] |= ~((0x81) << 16);
                    ControlBlock->CurrentInputBufferToSend->BufferParams.BufferParams[0] = 0x0;
                    ControlBlock->CurrentInputBufferToSend->BufferParams.BufferParams[1] = 0x0;
                }

                if(InputBufferToSend->InputBlockFromProducer[0]->Flags & EOF_VALID)
                {
                    StreamingLastBlock = TRUE;
                }

                if(((ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_WMA) || (ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_WMAPROLSL))
                &&(InputBufferToSend->InputBlockFromProducer[(BufferIn_p->NumberOfScatterPages - 1)]->Flags & WMA_LAST_DATA_BLOCK_VALID))
                {
                    STTBX_Print(("\n Sending last WMA data block \n"));
                    /* update command params */
#ifdef WMA_IN_LOOP
                    WmaLastBlock = FALSE;
                    StreamingLastBlock = FALSE;
#else
                    WmaLastBlock = TRUE;
                    StreamingLastBlock = TRUE;
#endif
                 }

                status = acc_setAudioDecoderBufferCmd(&ControlBlock->CurrentInputBufferToSend->Cmd,
                                                    &ControlBlock->CurrentInputBufferToSend->BufferParams,//FrameParams,
                                                    sizeof (MME_LxAudioDecoderBufferParams_t),//(MME_LxAudioDecoderFrameParams_t),
                                                    &ControlBlock->CurrentInputBufferToSend->FrameStatus,
                                                    sizeof (MME_LxAudioDecoderFrameStatus_t),
                                                    &(ControlBlock->CurrentInputBufferToSend->DecoderBufIn.BufferIn_p),
                                                    ACC_CMD_PLAY, FALSE,StreamingLastBlock,ControlBlock->StreamParams.StreamContent);

                status = MME_SendCommand(ControlBlock->TranformHandleDecoder, &ControlBlock->CurrentInputBufferToSend->Cmd);
                ControlBlock->CurrentInputBufferToSend = NULL;

                /* Update decoder tail */
                ControlBlock->DecoderInputQueueTail++;
                SendMMEComamand = TRUE;
                ControlBlock->streamingTransformerFirstTransfer = TRUE;
            }
            else if ((BufferIn_p->NumberOfScatterPages == ControlBlock->NumInputScatterPagesPerTransform) &&
                    (!ControlBlock->UseStreamingTransformer)&&(ControlBlock->DecPcmProcessing->TempoCtrl.Apply))
            {
                if (InputBufferToSend->InputBlockFromProducer[0]->Flags & PTS_VALID)
                {
                    ControlBlock->CurrentInputBufferToSend->FrameParams.PtsFlags = PTS_VALID; // LSB bit is for PTS Valid
                    ControlBlock->CurrentInputBufferToSend->FrameParams.PtsFlags |= InputBufferToSend->InputBlockFromProducer[0]->Data[PTS33_OFFSET] << 16;
                    ControlBlock->CurrentInputBufferToSend->FrameParams.PTS= InputBufferToSend->InputBlockFromProducer[0]->Data[PTS_OFFSET];
                    InputBufferToSend->InputBlockFromProducer[0]->Flags &= !(PTS_VALID); // To invalidate the PTS_VALID bit of PTS flag. PTS validity will be checked with the flags returned from LX.
                }
                else
                {
                    ControlBlock->CurrentInputBufferToSend->FrameParams.PtsFlags = 0x0;
                    ControlBlock->CurrentInputBufferToSend->FrameParams.PTS = 0x0;
                }
            }
        }
    }
    return status;
}

ST_ErrorCode_t  STAud_DecGetOutBuf0(DecoderControlBlock_t       *ControlBlock)
{
    ST_ErrorCode_t      status = ST_NO_ERROR;
    U32                         Index;
    BOOL                        SendMMEComamand = TRUE;
    MME_DataBuffer_t    *BufferOut_p=NULL;
    U32                         QueueTailIndex;
    DecoderOutputCmdParam_t     *OutputBufferToSend=NULL;

#if ENABLE_START_UP_PATTERN_GENERATION
    if (ControlBlock->GeneratePattern)
    {
        MemoryBlock_t   *tempOutputBlock;
        status = MemPool_GetOpBlk(ControlBlock->OutMemoryBlockManagerHandle0, (U32 *)&tempOutputBlock);
        if (status == ST_NO_ERROR)
        {
            ControlBlock->StartPatternBufferAddr = (U32)tempOutputBlock->MemoryStart;
            SetStartPattern(tempOutputBlock, (U32)ControlBlock->NumChannels);
            status = MemPool_PutOpBlk(ControlBlock->OutMemoryBlockManagerHandle0, (U32 *)&tempOutputBlock);
            ControlBlock->GeneratePattern = FALSE;
        }
        else //if NO MEM
        {
            /* this is done because if no mem is coming here means that
             blockmanagers are not started yet and returning NO MEM would cause the main state
             m/c to get stuck when BM's start
            */
            if(ControlBlock->UseStreamingTransformer)
            {
                return ST_NO_ERROR;
            }
            else
            {
                return ST_ERROR_NO_MEMORY;
            }
        }
    }
#endif

    if ((!ControlBlock->UseStreamingTransformer)
        || ((ControlBlock->streamingTransformerFirstTransfer) && (!ControlBlock->StreamingEofFound)))
    {
        while (SendMMEComamand)
        {
            SendMMEComamand = FALSE;

            if (ControlBlock->DecoderOutputQueueTail - ControlBlock->DecoderOutputQueueFront < STREAMING_DECODER_QUEUE_SIZE)
            {
                /* We have space to send another command */
                QueueTailIndex = ControlBlock->DecoderOutputQueueTail%STREAMING_DECODER_QUEUE_SIZE;

                OutputBufferToSend = ControlBlock->CurrentOutputBufferToSend = &ControlBlock->DecoderOutputQueue[QueueTailIndex];
                BufferOut_p = OutputBufferToSend->DecoderBufOut0.BufferOut_p;

                /* Dont ask for new output block if last time decoded data recieved is 0 bytes (Tempo processing Case)*/
                while ((BufferOut_p->NumberOfScatterPages != ControlBlock->NumOutputScatterPagesPerTransform) &&
                        (MemPool_GetOpBlk(ControlBlock->OutMemoryBlockManagerHandle0, (U32 *)&OutputBufferToSend->OutputBlockFromConsumer0[BufferOut_p->NumberOfScatterPages]) == ST_NO_ERROR))
                {
                    Index = BufferOut_p->NumberOfScatterPages;

#if ENABLE_START_UP_PATTERN_GENERATION
                    if(ControlBlock->StartPatternBufferAddr)
                    {
                        if(ControlBlock->StartPatternBufferAddr == OutputBufferToSend->OutputBlockFromConsumer0[Index]->MemoryStart)
                        {
                            memset(OutputBufferToSend->OutputBlockFromConsumer0[Index]->MemoryStart,0,OutputBufferToSend->OutputBlockFromConsumer0[Index]->MaxSize);
                            ControlBlock->StartPatternBufferAddr = 0;
                        }
                    }
#endif
                    BufferOut_p->ScatterPages_p[Index].Page_p = (void*)OutputBufferToSend->OutputBlockFromConsumer0[Index]->MemoryStart;
                    BufferOut_p->ScatterPages_p[Index].Size = OutputBufferToSend->OutputBlockFromConsumer0[Index]->MaxSize;
                    BufferOut_p->ScatterPages_p[Index].BytesUsed = 0;
                    BufferOut_p->TotalSize += OutputBufferToSend->OutputBlockFromConsumer0[Index]->MaxSize;
                    BufferOut_p->NumberOfScatterPages++;
                }


                if ((BufferOut_p->NumberOfScatterPages != ControlBlock->NumOutputScatterPagesPerTransform) &&
                    (!ControlBlock->UseStreamingTransformer))
                {
                    /* Could not get block for a Frame Based transformer*/
                    return ST_ERROR_NO_MEMORY;
                }

                if (!(ControlBlock->UseStreamingTransformer) && (ControlBlock->EnableTranscoding == FALSE))
                {
                    /* Got input for a Frame Based transformer */
                    ControlBlock->DecoderOutputQueueTail++;
                }

                if ((BufferOut_p->NumberOfScatterPages == ControlBlock->NumOutputScatterPagesPerTransform) &&
                    (ControlBlock->UseStreamingTransformer))
                {
                    U16     CmdCode;
                    if(!ControlBlock->PcmProcess.Mute)
                    {
                        /* Unmute the Decoder */
                        CmdCode = ACC_CMD_PLAY;
                    }
                    else
                    {
                        /* Mute the Decoder */
                        CmdCode = ACC_CMD_MUTE;
                    }

                    /* update command params */
                    status = acc_setAudioDecoderTransformCmd(&ControlBlock->CurrentOutputBufferToSend->Cmd,
                                                            &ControlBlock->CurrentOutputBufferToSend->FrameParams,
                                                            sizeof (MME_LxAudioDecoderFrameParams_t),
                                                            &ControlBlock->CurrentOutputBufferToSend->FrameStatus,
                                                            sizeof (MME_LxAudioDecoderFrameStatus_t),
                                                            &(ControlBlock->CurrentOutputBufferToSend->DecoderBufOut0.BufferOut_p),
                                                            /*ACC_CMD_PLAY*/CmdCode, 0, FALSE,0, 1);


                    status = MME_SendCommand(ControlBlock->TranformHandleDecoder, &ControlBlock->CurrentOutputBufferToSend->Cmd);
                    ControlBlock->CurrentOutputBufferToSend = NULL;
                    /* Update decoder tail */
                    ControlBlock->DecoderOutputQueueTail++;
                    SendMMEComamand = TRUE;
                }
            }
        }
    }
    return status;
}

ST_ErrorCode_t  STAud_DecGetOutBuf1(DecoderControlBlock_t       *ControlBlock)
{
    ST_ErrorCode_t      status = ST_NO_ERROR;
    U32                         QueueTailIndex, Index;
    BOOL                        SendMMEComamand = TRUE;
    MME_DataBuffer_t    *BufferOut_p=NULL;
    DecoderOutputCmdParam_t     *OutputBufferToSend=NULL;

    if ((!ControlBlock->UseStreamingTransformer)
        ||((ControlBlock->streamingTransformerFirstTransfer) && (!ControlBlock->StreamingEofFound)))
    {
        if (ControlBlock->EnableTranscoding)
        {
#if ENABLE_START_UP_PATTERN_GENERATION_FOR_TRANSCODER
            if (ControlBlock->GeneratePattern)
            {
                MemoryBlock_t   *tempOutputBlock;
                status = MemPool_GetOpBlk(ControlBlock->OutMemoryBlockManagerHandle1, (U32 *)&tempOutputBlock);
                if (status == ST_NO_ERROR)
                {
                    SetTranscoderStartPattern(tempOutputBlock, ControlBlock->DefaultCompressedFrameSize);
                    status = MemPool_PutOpBlk(ControlBlock->OutMemoryBlockManagerHandle1, (U32 *)&tempOutputBlock);
                    ControlBlock->GeneratePattern = FALSE;
                }
            }
#endif
            while (SendMMEComamand)
            {
                SendMMEComamand = FALSE;

                if (ControlBlock->DecoderOutputQueueTail - ControlBlock->DecoderOutputQueueFront < STREAMING_DECODER_QUEUE_SIZE)
                {
                    // We have space to send another command
                    QueueTailIndex = ControlBlock->DecoderOutputQueueTail%STREAMING_DECODER_QUEUE_SIZE;
                    OutputBufferToSend = &ControlBlock->DecoderOutputQueue[QueueTailIndex];
                    BufferOut_p = OutputBufferToSend->DecoderBufOut1.BufferOut_p;

                    while ((BufferOut_p->NumberOfScatterPages != ControlBlock->NumOutputScatterPagesPerTransform) &&
                            (MemPool_GetOpBlk(ControlBlock->OutMemoryBlockManagerHandle1, (U32 *)&OutputBufferToSend->OutputBlockFromConsumer1[BufferOut_p->NumberOfScatterPages]) == ST_NO_ERROR))
                    {
                        Index = BufferOut_p->NumberOfScatterPages;
                        BufferOut_p->ScatterPages_p[Index].Page_p = (void*)OutputBufferToSend->OutputBlockFromConsumer1[Index]->MemoryStart;
                        BufferOut_p->ScatterPages_p[Index].Size = OutputBufferToSend->OutputBlockFromConsumer1[Index]->MaxSize;
                        BufferOut_p->ScatterPages_p[Index].BytesUsed = 0;
                        BufferOut_p->TotalSize += OutputBufferToSend->OutputBlockFromConsumer1[Index]->MaxSize;
                        BufferOut_p->NumberOfScatterPages++;
                    }

                    if ((BufferOut_p->NumberOfScatterPages != ControlBlock->NumOutputScatterPagesPerTransform) &&
                            (!ControlBlock->UseStreamingTransformer))
                    {
                        /* Could not get block for a Frame Based transformer*/
                        return ST_ERROR_NO_MEMORY;
                    }

                    if (!(ControlBlock->UseStreamingTransformer))
                    {
                        /* Got input for a Frame Based transformer */
                        ControlBlock->DecoderOutputQueueTail++;
                    }

                    if ((BufferOut_p->NumberOfScatterPages == ControlBlock->NumOutputScatterPagesPerTransform) &&
                        (ControlBlock->UseStreamingTransformer))
                    {
                        /* update command params */
                        status = acc_setAudioDecoderTransformCmd(&ControlBlock->CurrentOutputBufferToSend->Cmd,
                                                                &ControlBlock->CurrentOutputBufferToSend->FrameParams,
                                                                sizeof (MME_LxAudioDecoderFrameParams_t),
                                                                &ControlBlock->CurrentOutputBufferToSend->FrameStatus,
                                                                sizeof (MME_LxAudioDecoderFrameStatus_t),
                                                                &(ControlBlock->CurrentOutputBufferToSend->DecoderBufOut1.BufferOut_p),
                                                                ACC_CMD_PLAY, 0, FALSE, 0, 1);


                        status = MME_SendCommand(ControlBlock->TranformHandleDecoder, &ControlBlock->CurrentOutputBufferToSend->Cmd);
                        ControlBlock->CurrentOutputBufferToSend = NULL;
                        /* Update decoder tail */
                        ControlBlock->DecoderOutputQueueTail++;
                        SendMMEComamand = TRUE;
                    }
                }
            }
        }
    }
    return status;
}

ST_ErrorCode_t  STAud_DecSendCmd(DecoderControlBlock_t  *ControlBlock)
{
    ST_ErrorCode_t      status = ST_NO_ERROR;
    U16         CmdCode, numInputBuffers, numOutputBuffers;
    DecoderOutputCmdParam_t     *OutputBufferToSend = NULL;

    if (!ControlBlock->UseStreamingTransformer)
    {
        /* Check for command params to be build for Decoder */
        if(!ControlBlock->PcmProcess.Mute)
        {
            /* Unmute the Decoder */
            CmdCode = ACC_CMD_PLAY;
        }
        else
        {
            /* Mute the Decoder */
            CmdCode = ACC_CMD_MUTE;
        }

        OutputBufferToSend = ControlBlock->CurrentOutputBufferToSend;
        ControlBlock->CurrentOutputBufferToSend->FrameParams.PTS = ControlBlock->CurrentInputBufferToSend->FrameParams.PTS;
        ControlBlock->CurrentOutputBufferToSend->FrameParams.PtsFlags= ControlBlock->CurrentInputBufferToSend->FrameParams.PtsFlags;
        OutputBufferToSend->BufferPtrs[0] = ControlBlock->CurrentInputBufferToSend->DecoderBufIn.BufferIn_p;
        OutputBufferToSend->BufferPtrs[1] = OutputBufferToSend->DecoderBufOut0.BufferOut_p;
        numOutputBuffers = numInputBuffers = 1;

        if (ControlBlock->EnableTranscoding)
        {
            OutputBufferToSend->BufferPtrs[2] = OutputBufferToSend->DecoderBufOut1.BufferOut_p;
            numOutputBuffers++;
        }

        /* update command params */
        status = acc_setAudioDecoderTransformCmd(&ControlBlock->CurrentOutputBufferToSend->Cmd,
                                                &ControlBlock->CurrentOutputBufferToSend->FrameParams,
                                                sizeof (MME_LxAudioDecoderFrameParams_t),
                                                &ControlBlock->CurrentOutputBufferToSend->FrameStatus,
                                                sizeof (MME_LxAudioDecoderFrameStatus_t),
                                                ControlBlock->CurrentOutputBufferToSend->BufferPtrs,
                                                CmdCode, 0, ControlBlock->TransformerRestart/*FALSE*/,
                                                numInputBuffers, numOutputBuffers);
        /*Reset the restart flag*/
        ControlBlock->TransformerRestart = FALSE;
        status = MME_SendCommand(ControlBlock->TranformHandleDecoder, &ControlBlock->CurrentOutputBufferToSend->Cmd);
    }
    return status;
}

/**************************************************************************************************
 *  Function name       : STAud_DecProcessInput
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Get the comressed input buffers, get empty PCM buffer,Update MME params
 *                                 and send command to MME decoder
 **************************************************************************************************/
ST_ErrorCode_t  STAud_DecProcessInput(DecoderControlBlock_t     *ControlBlock)
{

    ST_ErrorCode_t              status = ST_NO_ERROR;

    switch(ControlBlock->DecInState)
    {

        case MME_DECODER_INPUT_STATE_IDLE:

            /* Intended fall through */
            ControlBlock->DecInState = MME_DECODER_INPUT_STATE_GET_INPUT_DATA;

        case MME_DECODER_INPUT_STATE_GET_INPUT_DATA:

            /* Get the input buffers from parser */
            status = STAud_DecHandleInput(ControlBlock);
            if(status != ST_NO_ERROR)
            {
                return status;
            }
            /* Intended fall through */
            ControlBlock->DecInState = MME_DECODER_INPUT_STATE_GET_OUTPUT_BUFFER0;

        case MME_DECODER_INPUT_STATE_GET_OUTPUT_BUFFER0:

            /* Get the first output buffer. It will hold decoded data */
            status = STAud_DecGetOutBuf0(ControlBlock);
            if(status != ST_NO_ERROR)
            {
                return status;
            }

            /* Intended fall through */
            ControlBlock->DecInState = MME_DECODER_INPUT_STATE_GET_OUTPUT_BUFFER1;

        case MME_DECODER_INPUT_STATE_GET_OUTPUT_BUFFER1:

            /* Get the second output buffer. It will hold Transcoded data */
            status = STAud_DecGetOutBuf1(ControlBlock);
            if(status != ST_NO_ERROR)
            {
                return status;
            }

            /* Intended fall through */
            ControlBlock->DecInState = MME_DECODER_INPUT_STATE_UPDATE_PARAMS_SEND_TO_MME;

        case MME_DECODER_INPUT_STATE_UPDATE_PARAMS_SEND_TO_MME:
            /* Send the command to LX */
            status = STAud_DecSendCmd(ControlBlock);

            /* Change State to IDLE */
            ControlBlock->DecInState = MME_DECODER_INPUT_STATE_IDLE;

        case MME_DECODER_INPUT_STATE_BYPASS:
            /* Special state -  For handling AC3 playback without support from companion bin */
            break;

        default:
            break;
    }

    return status;
}

/**************************************************************************************************
 *  Function name       : STAud_DecProcessOutput
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Send the decoded data to next unit
 **************************************************************************************************/
ST_ErrorCode_t  STAud_DecProcessOutput(DecoderControlBlock_t    *ControlBlock, MME_Command_t    *command_p)
{
    ST_ErrorCode_t status = ST_NO_ERROR;
    DecoderOutputCmdParam_t *CurrentBufferToDeliver;
    U32 i = 0;
    MME_LxAudioDecoderFrameStatus_t     *DecodedFrameStatus=NULL;
    MemoryBlock_t *OutputBlockFromConsumer0=NULL, *OutputBlockFromConsumer1=NULL ;

    /* Check if there is data to be sent to output */
    if(ControlBlock->DecoderOutputQueueFront != ControlBlock->DecoderOutputQueueTail)
    {
        CurrentBufferToDeliver = &ControlBlock->DecoderOutputQueue[ControlBlock->DecoderOutputQueueFront % STREAMING_DECODER_QUEUE_SIZE];
        for (i=0; i < ControlBlock->NumOutputScatterPagesPerTransform; i++)
        {
            /* TBD for streaming buffer we have to pick data from DataBuffer[0] as this is were we will put output buffer
                Instead of picking from Cmd buffers pick scatterpages from decoder queue
            */
            OutputBlockFromConsumer0 = CurrentBufferToDeliver->OutputBlockFromConsumer0[i];
            OutputBlockFromConsumer1 = CurrentBufferToDeliver->OutputBlockFromConsumer1[i];

            if (command_p->CmdStatus.State == MME_COMMAND_FAILED)
            {
                STTBX_Print(("STAud_DecProcessOutput:command_p->CmdStatus.Error= %d \n",command_p->CmdStatus.Error));
                            OutputBlockFromConsumer0->Size = ControlBlock->DefaultDecodedSize;
                if (ControlBlock->EnableTranscoding)
                        OutputBlockFromConsumer1->Size = ControlBlock->DefaultCompressedFrameSize;
            }
            else
            {
                OutputBlockFromConsumer0->Size = CurrentBufferToDeliver->DecoderBufOut0.BufferOut_p->ScatterPages_p[i].BytesUsed;
                if (ControlBlock->EnableTranscoding)
                {
                    OutputBlockFromConsumer1->Size = CurrentBufferToDeliver->DecoderBufOut1.BufferOut_p->ScatterPages_p[i].BytesUsed;
#if REUSE_NULL_TRANSCODED_BUFFERS
                    /* If we want to reuse the buffer then no need to set to default size */
#else
                    if (OutputBlockFromConsumer1->Size == 0)
                    {
                        STTBX_Print(("Error in compressed frame size returned by LX\n"));
                        OutputBlockFromConsumer1->Size = ControlBlock->DefaultCompressedFrameSize;
                        memset((void*)CurrentBufferToDeliver->DecoderBufOut1.BufferOut_p->ScatterPages_p[i].Page_p, 0, OutputBlockFromConsumer1->Size);
                    }
#endif

                }
            }

            DecodedFrameStatus = (MME_LxAudioDecoderFrameStatus_t*)&CurrentBufferToDeliver->FrameStatus;
            STTBX_Print(("Decoder: Elapsed:,%d\n",DecodedFrameStatus->ElapsedTime));
            if (!ControlBlock->UseStreamingTransformer)
            {
                /* Put in required info for Frame Based transformer */
                /* Copy the PTS etc values from input to output block.*/
                /* Extract PTS from LX in case of tempo */
                if(ControlBlock->DecPcmProcessing->TempoCtrl.Apply== TRUE)
                {
                    if ((((CurrentBufferToDeliver->FrameStatus.PTSflag & 0x3) == 0x1) || ((CurrentBufferToDeliver->FrameStatus.PTSflag & 0x3) == 0x2)))
                    {
                        /* Received PTS info for frame based decoder from LX */
                        OutputBlockFromConsumer0->Flags                    |= PTS_VALID;
                        OutputBlockFromConsumer0->Data[PTS33_OFFSET]    = CurrentBufferToDeliver->FrameStatus.PTSflag>>16;
                        OutputBlockFromConsumer0->Data[PTS_OFFSET]              = CurrentBufferToDeliver->FrameStatus.PTS;
                    }
                }
                else
                {
                    OutputBlockFromConsumer0->Data[PTS_OFFSET]      = ControlBlock->PTS;
                    OutputBlockFromConsumer0->Data[PTS33_OFFSET]        = ControlBlock->PTS33Bit;
                }

                if(ControlBlock->Flags & LAYER_VALID)
                {
                    OutputBlockFromConsumer0->Data[LAYER_OFFSET] = ControlBlock->LayerInfo;
                }
                if(ControlBlock->Flags & FADE_VALID)
                {
                    OutputBlockFromConsumer0->Data [FADE_OFFSET] = ControlBlock->ADControl.Fade;
                }
                if(ControlBlock->Flags & PAN_VALID)
                {
                    OutputBlockFromConsumer0->Data [PAN_OFFSET] = ControlBlock->ADControl.Pan;
                }

                /* Copy other needed information from Input to output */
                OutputBlockFromConsumer0->Flags |= ControlBlock->Flags;

                OutputBlockFromConsumer0->Flags |= SAMPLESIZE_VALID;
                OutputBlockFromConsumer0->Data[SAMPLESIZE_OFFSET] = ACC_WS32;
#if ENABLE_EOF_PATTERN_GENERATION
                if(OutputBlockFromConsumer0->Flags & EOF_VALID)
                {
                    SetEOFPattern(OutputBlockFromConsumer0, (U32)ControlBlock->NumChannels);
                }
#endif

                if (ControlBlock->EnableTranscoding)
                {
                    /* Put in required info for Frame Based transformer */
                    /* Copy the PTS etc values from input to second output block */
                    OutputBlockFromConsumer1->Data[PTS_OFFSET] = ControlBlock->PTS;
                    OutputBlockFromConsumer1->Data[PTS33_OFFSET] = ControlBlock->PTS33Bit;

                    /* Copy other needed information from Input to output */
                    OutputBlockFromConsumer1->Flags = ControlBlock->Flags;
                }
		 ControlBlock->InputStereoMode = (U8)DecodedFrameStatus->FrameStatus[0];
            }
            else
            {
                U8 * TempFrameStatus;

                /* For Stream based decoders all the information should be extracted from frame status */
                OutputBlockFromConsumer0->Flags = 0;
                if ( ( ((CurrentBufferToDeliver->FrameStatus.PTSflag & 0x3) == 0x1) || ((CurrentBufferToDeliver->FrameStatus.PTSflag & 0x3) == 0x2) ) && (i == 0))
                {
                    /* PTS should be checked only for first scatter page */
                    /* Received PTS info for stream based decoder from LX */
                    OutputBlockFromConsumer0->Flags     |= PTS_VALID;
                    OutputBlockFromConsumer0->Data[PTS33_OFFSET]        = CurrentBufferToDeliver->FrameStatus.PTSflag>>15;
                    OutputBlockFromConsumer0->Data[PTS_OFFSET]          = CurrentBufferToDeliver->FrameStatus.PTS;
                }

                if((ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_WMA) ||
                    (ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_WMAPROLSL))
                {
                    tMMEWmaStatus *wma_status = (tMMEWmaStatus *)DecodedFrameStatus->FrameStatus;

                    if(wma_status->Status == WMA_STATUS_EOF)
                    {
                        /*check the Id of the command and abort all transform command sent after this one */
                        STTBX_Print(("EOF reached\n"));
                        ControlBlock->StreamingEofFound = TRUE;
                        /* Only for last scatter page generate EOF pattern */
                        if (i == (ControlBlock->NumOutputScatterPagesPerTransform - 1))
                        {
                            OutputBlockFromConsumer0->Flags |= EOF_VALID;
#if ENABLE_EOF_PATTERN_GENERATION
                            SetEOFPattern(OutputBlockFromConsumer0, (U32)ControlBlock->NumChannels);

#else
                            //if size is zero then send dummy buffer
                            if (!OutputBlockFromConsumer0->Size)
                            {
                                OutputBlockFromConsumer0->Size = ControlBlock->DefaultDecodedSize;
                                memset((char*)OutputBlockFromConsumer0->MemoryStart,0,OutputBlockFromConsumer0->Size);
                                OutputBlockFromConsumer0->Flags |= DUMMYBLOCK_VALID;
                            }
#endif
                        }
                    }
                }
                else
                {
                    TempFrameStatus =(U8 *) DecodedFrameStatus->FrameStatus;
                    if(*TempFrameStatus == STREAMING_STATUS_EOF)
                    {
                        STTBX_Print(("Decoder--> EOF reached\n"));
                        OutputBlockFromConsumer0->Flags |= EOF_VALID;
                        ControlBlock->StreamingEofFound = TRUE;

                        if (i == (ControlBlock->NumOutputScatterPagesPerTransform - 1))
                        {
                            OutputBlockFromConsumer0->Flags |= EOF_VALID;
#if ENABLE_EOF_PATTERN_GENERATION
                            SetEOFPattern(OutputBlockFromConsumer0, (U32)ControlBlock->NumChannels);
#else
                            //if size is zero then send dummy buffer
                            if (!OutputBlockFromConsumer0->Size)
                            {
                                OutputBlockFromConsumer0->Size = ControlBlock->DefaultDecodedSize;
                                memset((char*)OutputBlockFromConsumer0->MemoryStart,0,OutputBlockFromConsumer0->Size);
                                OutputBlockFromConsumer0->Flags |= DUMMYBLOCK_VALID;
                            }
#endif
                        }
                    }
                }
            }

            if (command_p->CmdStatus.State == MME_COMMAND_FAILED)
            {
                OutputBlockFromConsumer0->Flags |= DUMMYBLOCK_VALID;
                if (ControlBlock->EnableTranscoding)
                    OutputBlockFromConsumer1->Flags |= DUMMYBLOCK_VALID;
            }

            if (ControlBlock->EnableTranscoding)
            {
                /* Put in required info for Frame Based transformer */
                /* Copy the PTS etc values from input to second output block */
                OutputBlockFromConsumer1->Data[PTS_OFFSET]      = ControlBlock->PTS;
                OutputBlockFromConsumer1->Data[PTS33_OFFSET]    = ControlBlock->PTS33Bit;

                /* Copy other needed information from Input to output */
                OutputBlockFromConsumer1->Flags = ControlBlock->Flags;
                OutputBlockFromConsumer1->Flags |= DATAFORMAT_VALID;
                OutputBlockFromConsumer1->Data[DATAFORMAT_OFFSET] = ControlBlock->TrancodedDataAlignment;
            }

            if(DecodedFrameStatus->AudioMode  != ControlBlock->AudioCodingMode)
            {
                /* New frequency received from decoder. Update control block information */
                ControlBlock->AudioCodingMode = DecodedFrameStatus->AudioMode;

                /* Send the mode to Players */
                OutputBlockFromConsumer0->Data[CODING_OFFSET] =
                DecodedFrameStatus->AudioMode;
                /* Mark the mode valid flag */
                OutputBlockFromConsumer0->Flags |= MODE_VALID;
                STTBX_Print(("Delivered Audio Coding Mode %d\n",OutputBlockFromConsumer0->Data[CODING_OFFSET]));
            }
            else
            {
                /* Reset the mode valid flag */
                OutputBlockFromConsumer0->Flags &= ~MODE_VALID;
            }

            if(DecodedFrameStatus->FrameStatus[2]!=ControlBlock->DMixTableParams)
            {
                ControlBlock->DMixTableParams = DecodedFrameStatus->FrameStatus[2];
                OutputBlockFromConsumer0->Data[NEW_DMIX_TABLE_OFFSET] =ControlBlock->DMixTableParams;
                OutputBlockFromConsumer0->Flags |= NEW_DMIX_TABLE_VALID;
            }
            else
            {
                OutputBlockFromConsumer0->Flags &= ~NEW_DMIX_TABLE_VALID;
            }

            if(DecodedFrameStatus->DecAudioMode  != ControlBlock->InputCodingMode)
            {
                /* New frequency received from decoder. Update control block information */
                ControlBlock->InputCodingMode = DecodedFrameStatus->DecAudioMode;

                STTBX_Print(("Input Audio Coding Mode %d\n",ControlBlock->InputCodingMode));
            }
            if(DecodedFrameStatus->Emphasis!= ControlBlock->Emphasis)
            {
                /* New frequency received from decoder. Update control block information */
                ControlBlock->Emphasis= DecodedFrameStatus->Emphasis;

                STTBX_Print(("Emphasis %d\n",ControlBlock->Emphasis));
            }
            if(ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_PCM)
            {
                /*Sending for all blocks. only send when there is a change*/
                OutputBlockFromConsumer0->Data[FREQ_OFFSET] =   ControlBlock->SamplingFrequency;
                /* Mark the frequency valid flag */
                OutputBlockFromConsumer0->Flags |= FREQ_VALID;
            }
            else
            {
                if((DecodedFrameStatus->SamplingFreq  != ControlBlock->SamplingFrequency)&& (OutputBlockFromConsumer0->Size != 0))
                {
                    U32 TempNumChannel;
                    /* New frequency received from decoder. Update control block information */
                    ControlBlock->SamplingFrequency = DecodedFrameStatus->SamplingFreq;
                    TempNumChannel = ControlBlock->NumChannels;
                    /* Send the frequency to Players */
                    OutputBlockFromConsumer0->Data[FREQ_OFFSET] =
                    CovertFsCodeToSampleRate(ControlBlock->SamplingFrequency);
                    /* Mark the frequency valid flag */
                    OutputBlockFromConsumer0->Flags |= FREQ_VALID;
                    ControlBlock->ADControl.NumFramesPerSecond = (OutputBlockFromConsumer0->Data[FREQ_OFFSET] * 4 * TempNumChannel)/(OutputBlockFromConsumer0->Size);
                    STTBX_Print(("Delivered Sampling frequency %d\n",OutputBlockFromConsumer0->Data[FREQ_OFFSET]));
                }
                else
                {
                    /* Reset the frequency valid flag */
                    OutputBlockFromConsumer0->Flags &= ~FREQ_VALID;
                }

                if (ControlBlock->EnableTranscoding)
                {
                    if (ControlBlock->TranscodedSamplingFrequency != (enum eAccFsCode)(DecodedFrameStatus->FrameStatus[3]))
                    {
                        ControlBlock->TranscodedSamplingFrequency = (enum eAccFsCode)(DecodedFrameStatus->FrameStatus[3]);
                        OutputBlockFromConsumer1->Flags |= FREQ_VALID;
                        OutputBlockFromConsumer1->Data[FREQ_OFFSET] =
                        CovertFsCodeToSampleRate(DecodedFrameStatus->FrameStatus[3]);
                    }
                    else
                    {
                        OutputBlockFromConsumer1->Flags &= ~FREQ_VALID;
                    }
                }
            }

            if(ControlBlock->EnableTranscoding)
            {
                if(OutputBlockFromConsumer1->Size != 0)
                {
                    status = MemPool_PutOpBlk(ControlBlock->OutMemoryBlockManagerHandle1 ,
                    (U32 *)&(OutputBlockFromConsumer1));

                    if(status != ST_NO_ERROR)
                    {
                        STTBX_Print(("STAud_DecProcessOutput::MemPool_PutOpBlk Failed\n"));
                    }
                    CurrentBufferToDeliver->DecoderBufOut1.BufferOut_p->NumberOfScatterPages = 0;
                    CurrentBufferToDeliver->DecoderBufOut1.BufferOut_p->TotalSize = 0;
                }
                else
                {
                    /* Handling for the case when transcoded data size is 0 bytes*/
                    status = MemPool_ReturnOpBlk(ControlBlock->OutMemoryBlockManagerHandle1 ,(U32 *)&(OutputBlockFromConsumer1));
                    if(status != ST_NO_ERROR)
                    {
                        STTBX_Print(("STAud_DecProcessOutput::(Transcoding)MemPool_ReturnOpBlk Failed\n"));
                    }
                }
            }
            if(OutputBlockFromConsumer0->Size != 0)
            {
                //STTBX_Print(("STAud_DecProcessOutput::Size == %d\n",OutputBlockFromConsumer0->Size));
                status = MemPool_PutOpBlk(ControlBlock->OutMemoryBlockManagerHandle0 ,
                                            (U32 *)&(OutputBlockFromConsumer0));
                //STTBX_Print(("DECODING ELAPSED TIME:,%d\n",DecodedFrameStatus->ElapsedTime));

                if(status != ST_NO_ERROR)
                {
                    STTBX_Print(("STAud_DecProcessOutput::MemPool_PutOpBlk Failed\n"));
                }

            }
            else
            {
                /* Handling for the case when Tempo is applied and decoded data size is 0 bytes*/
                if(ControlBlock->DecPcmProcessing->TempoCtrl.Apply)
                {
                    status = MemPool_ReturnOpBlk(ControlBlock->OutMemoryBlockManagerHandle0 ,(U32 *)&(OutputBlockFromConsumer0));
                    if(status != ST_NO_ERROR)
                    {
                        STTBX_Print(("STAud_DecProcessOutput::ReturnOpBlk Failed\n"));
                    }
                }
                else
                {
                    OutputBlockFromConsumer0->Flags |= DUMMYBLOCK_VALID;
                    OutputBlockFromConsumer0->Size = ControlBlock->DefaultDecodedSize;
                    status = MemPool_PutOpBlk(ControlBlock->OutMemoryBlockManagerHandle0 ,
                                                (U32 *)&(OutputBlockFromConsumer0));
                    if(status != ST_NO_ERROR)
                    {
                        STTBX_Print(("STAud_DecProcessOutput::PutOpBlk Failed\n"));
                    }
                }
            }
            /* As all the scatter pages have been released set it back to 0 */
            CurrentBufferToDeliver->DecoderBufOut0.BufferOut_p->NumberOfScatterPages = 0;
            CurrentBufferToDeliver->DecoderBufOut0.BufferOut_p->TotalSize = 0;
            /* update queue front */
            ControlBlock->DecoderOutputQueueFront++;
            ControlBlock->Flags = 0;
        }
    }
    return ST_NO_ERROR;
}

/**************************************************************************************************
 *  Function name       : STAud_DecReleaseInput
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Relase the compressed data to next unit
 **************************************************************************************************/
ST_ErrorCode_t  STAud_DecReleaseInput(DecoderControlBlock_t     *ControlBlock)
{
    ST_ErrorCode_t                      status = ST_NO_ERROR;
    DecoderInputCmdParam_t      *CurrentInputBufferToRelease;
    U32 i,Index;
    MemoryBlock_t       *InputBlockFromProducer=NULL;

    /* Check if there is data to be sent to output */
    if(ControlBlock->DecoderInputQueueFront != ControlBlock->DecoderInputQueueTail)
    {
        Index  = ControlBlock->DecoderInputQueueFront % STREAMING_DECODER_QUEUE_SIZE;
        CurrentInputBufferToRelease = &ControlBlock->DecoderInputQueue[Index];

        if((ControlBlock->StreamParams.StreamContent != STAUD_STREAM_CONTENT_BEEP_TONE) &&
            (ControlBlock->StreamParams.StreamContent != STAUD_STREAM_CONTENT_PINK_NOISE))
        {
            /* Set the decoded buffer size returned from Decoder */
            for (i=0; i < ControlBlock->NumInputScatterPagesPerTransform; i++)
            {
                InputBlockFromProducer = CurrentInputBufferToRelease->InputBlockFromProducer[i];

                /* Put in required info for Frame Based transformer */
                /* Copy the PTS etc values from input to output block */
                ControlBlock->PTS                       =       InputBlockFromProducer->Data[PTS_OFFSET];
                ControlBlock->PTS33Bit  =       InputBlockFromProducer->Data[PTS33_OFFSET];

                if (InputBlockFromProducer->Flags & FREQ_VALID)
                {
                    STTBX_Print(("Input Sampling Frequency %d\n", InputBlockFromProducer->Data[FREQ_OFFSET]));
                    ControlBlock->InputFrequency = InputBlockFromProducer->Data[FREQ_OFFSET];
                }

                /* Pass the Layer update, if any, to next module */
                if (InputBlockFromProducer->Flags & LAYER_VALID)
                {
                    STTBX_Print(("Layer Update in Decoder \n"));
                    ControlBlock->LayerInfo = (U32)InputBlockFromProducer->AppData_p;
                }

                /* Pass the Fade update, if any, to next module */
                if (InputBlockFromProducer->Flags & FADE_VALID)
                {
                    STTBX_Print(("Fade Update in Decoder \n"));
                    ControlBlock->Fade = (U32)InputBlockFromProducer->Data[FADE_OFFSET];
                }

                /* Pass the Pan update, if any, to next module */
                if (InputBlockFromProducer->Flags & PAN_VALID)
                {
                    STTBX_Print(("Pan Update in Decoder \n"));
                    ControlBlock->Pan = (U32)InputBlockFromProducer->Data[PAN_OFFSET];
                }

                /* Pass the Mode update, if any, to next module */
                if (InputBlockFromProducer->Flags & MODE_VALID)
                {
                    STTBX_Print(("Layer Update in Decoder \n"));
                    ControlBlock->CodingMode = (U32)InputBlockFromProducer->Data[CODING_OFFSET];
                }

                /* Copy other needed information from Input to output */
                /* Maintain old flags to be used otherwise will be overwritten
                when new input block is recieved : Case when decoded data is 0 */
                ControlBlock->Flags |= InputBlockFromProducer->Flags;

                status = MemPool_FreeIpBlk(ControlBlock->InMemoryBlockManagerHandle,
                                        (U32 *)&(CurrentInputBufferToRelease->InputBlockFromProducer[i]),
                                        ControlBlock->Handle);
                if(status != ST_NO_ERROR)
                {
                    STTBX_Print(("STAud_DecReleaseInput::MemPool_FreeIpBlk Failed\n"));
                }
            }
        }

        /* As all the scatter pages have been released set it back to 0 */
        CurrentInputBufferToRelease->DecoderBufIn.BufferIn_p->NumberOfScatterPages = 0;
        CurrentInputBufferToRelease->DecoderBufIn.BufferIn_p->TotalSize = 0;

        /* update queue front */
        ControlBlock->DecoderInputQueueFront++;
    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_DecApplyPcmParams(DecoderControlBlock_t    *ControlBlock)
{
    ST_ErrorCode_t      status = ST_NO_ERROR;
    if(ControlBlock->HoldCompressionMode.Apply)
    {
        status |= STAud_DecPPSetCompressionMode(ControlBlock->Handle,ControlBlock->HoldCompressionMode.CompressionMode);
        ControlBlock->HoldCompressionMode.Apply = FALSE;
    }

    if(ControlBlock->HoldDynamicRange.Apply)
    {
        status |= STAud_DecPPSetDynamicRange(ControlBlock->Handle,&ControlBlock->HoldDynamicRange.DyRange);
        ControlBlock->HoldDynamicRange.Apply = FALSE;
    }

    if(ControlBlock->HoldStereoMode.Apply)
    {
        status |= STAud_DecPPSetStereoMode(ControlBlock->Handle, ControlBlock->HoldStereoMode.StereoMode);
        ControlBlock->HoldStereoMode.Apply = FALSE;
    }

    if(ControlBlock->HoldDownMixParams.Apply)
    {
        status |= STAud_DecPPSetDownMix(ControlBlock->Handle, &ControlBlock->HoldDownMixParams.Downmix);
        ControlBlock->HoldDownMixParams.Apply = FALSE;
    }

    return status;
}

/******************************************************************************
 *  Function name       : STAud_DecSetAndInitDecoder
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : MME_ERROR
 *  Description         :
 ***************************************************************************** */
MME_ERROR       STAud_DecSetAndInitDecoder(DecoderControlBlock_t        *ControlBlock)
{
    MME_ERROR   Error = MME_SUCCESS;
    enum eAccDecoderId DecID = ACC_AC3_ID;
    char TransformerName[64];
    U32  j=0;

    /* Init callback data as Handle. It will help us find the decoder from list of decoders in multi instance mode */
    ControlBlock->MMEinitParams.CallbackUserData = (void *)ControlBlock->Handle;

    /*Transformer restart*/
    ControlBlock->TransformerRestart = TRUE;
    ControlBlock->streamingTransformerFirstTransfer = FALSE;
    ControlBlock->StreamingEofFound = FALSE;
    ControlBlock->EnableTranscoding = FALSE;
    ControlBlock->GeneratePattern = TRUE;

    /*Reset AD Structure.*/
    memset((void *)&ControlBlock->ADControl, 0,sizeof(ADControl_t));

    /*Reset Attenuation Structure.*/
    memset((void *)&ControlBlock->Attenuation_p, 0,sizeof(STAUD_Attenuation_t));

    /* Default values. These shall be overwritten by individual decoder type */
    ControlBlock->UseStreamingTransformer = FALSE;
    ControlBlock->DefaultDecodedSize = 1536 * 4 * ControlBlock->NumChannels;
    ControlBlock->DefaultCompressedFrameSize = 1536;

    switch(ControlBlock->StreamParams.StreamContent)
    {
        case STAUD_STREAM_CONTENT_MPEG1:
        case STAUD_STREAM_CONTENT_MPEG2:
        case STAUD_STREAM_CONTENT_MP3   :
            DecID = ACC_MP2A_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 1152 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 1152;
            break;

        case STAUD_STREAM_CONTENT_MPEG_AAC:
            DecID = ACC_MP4A_AAC_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 1024 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 1024;
#ifdef MSPP_PARSER
            ControlBlock->UseStreamingTransformer = TRUE;
#endif
            break;

        case STAUD_STREAM_CONTENT_HE_AAC:
            DecID = ACC_MP4A_AAC_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 1024 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 1024;
#ifdef MSPP_PARSER
            ControlBlock->UseStreamingTransformer = TRUE;
#endif
            break;

        case STAUD_STREAM_CONTENT_AC3:
            DecID = ACC_AC3_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 1536 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 1536;
            break;

        case STAUD_STREAM_CONTENT_DDPLUS:
            DecID = ACC_DDPLUS_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 1536 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 1536;
            ControlBlock->EnableTranscoding = TRUE;
            break;

        case STAUD_STREAM_CONTENT_CDDA:
        case STAUD_STREAM_CONTENT_PCM:
            DecID = ACC_LPCM_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = CDDA_NB_SAMPLES * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = CDDA_NB_SAMPLES;
            break;

        case STAUD_STREAM_CONTENT_LPCM:
        case STAUD_STREAM_CONTENT_LPCM_DVDA:
            DecID = ACC_LPCM_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = LPCMV_SAMPLES_PER_FRAME * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = LPCMV_SAMPLES_PER_FRAME;
            break;

        case STAUD_STREAM_CONTENT_MLP:
            DecID = ACC_MLP_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 600 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 600;
            break;

        case STAUD_STREAM_CONTENT_DTS:
            DecID = ACC_DTS_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 600 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 600;
            break;

        case STAUD_STREAM_CONTENT_WMA:
            DecID = ACC_WMA9_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 600 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 600;
            ControlBlock->UseStreamingTransformer = TRUE;
            break;

        case STAUD_STREAM_CONTENT_WMAPROLSL:
            DecID = ACC_WMAPROLSL_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 600 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 600;
            ControlBlock->UseStreamingTransformer = TRUE;
            break;

        case STAUD_STREAM_CONTENT_ADIF:
        case STAUD_STREAM_CONTENT_MP4_FILE:
        case STAUD_STREAM_CONTENT_RAW_AAC:
            DecID = ACC_MP4A_AAC_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 1024 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 1024;
            ControlBlock->UseStreamingTransformer = TRUE;
            break;

        case STAUD_STREAM_CONTENT_BEEP_TONE:
        case STAUD_STREAM_CONTENT_PINK_NOISE:
            DecID = ACC_GENERATOR_ID;
            /* Set the default decoded buffer size for this stream */
            ControlBlock->DefaultDecodedSize = 1024 * 4 * ControlBlock->NumChannels;
            ControlBlock->DefaultCompressedFrameSize = 1024;
            break;

        case STAUD_STREAM_CONTENT_DV:
        case STAUD_STREAM_CONTENT_CDDA_DTS:
        case STAUD_STREAM_CONTENT_NULL:
        default:
            STTBX_Print(("STAud_DecSetAndInitDecoder: Unsupported Decoder Type !!!\n "));
            break;
    }


    /* Set number of scatter pages per transform based on the trasnformer type*/
    if (ControlBlock->UseStreamingTransformer)
    {
        /* For stream based transformer*/
        ControlBlock->NumInputScatterPagesPerTransform = NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM;
        ControlBlock->NumOutputScatterPagesPerTransform = NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM;

        /*priority for decoding*/
        ControlBlock->MMEinitParams.Priority = MME_PRIORITY_BELOW_NORMAL;
    }
    else
    {
        /* For frame based transformer*/
        ControlBlock->NumInputScatterPagesPerTransform = 1;
        ControlBlock->NumOutputScatterPagesPerTransform = 1;

        /*priority for decoding*/
        ControlBlock->MMEinitParams.Priority = MME_PRIORITY_NORMAL;
    }

    /* Get the changed PCM params, if any. This can happen if user changes some PCM params
    between Init/stop and start of audio
    */

    /* init decoder params */
    Error |= acc_setAudioDecoderInitParams(ControlBlock->MMEinitParams.TransformerInitParams_p,
                                            sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
                                            DecID,
                                            ControlBlock->StreamParams.StreamContent,
                                            ControlBlock->DecPcmProcessing,
                                            ControlBlock->Handle);

    Error = STAud_DecApplyPcmParams(ControlBlock); //SA

    /* Register to memory block manager as a Producer */
    if(ControlBlock->OutMemoryBlockManagerHandle0)
    {
        Error |= MemPool_RegProducer(ControlBlock->OutMemoryBlockManagerHandle0, ControlBlock->ProducerParam_s);
    }

    if(ControlBlock->OutMemoryBlockManagerHandle1)
    {
        Error |= MemPool_RegProducer(ControlBlock->OutMemoryBlockManagerHandle1, ControlBlock->ProducerParam_s);
    }

    /* Checking the capability of the LX firmware for the stream */
    if (ControlBlock->DecCapability.SupportedStreamContents & ControlBlock->StreamParams.StreamContent)
    {
        /* Register to memory block manager as a consumer for input */
        if(ControlBlock->InMemoryBlockManagerHandle)
        {
        Error |= MemPool_RegConsumer(ControlBlock->InMemoryBlockManagerHandle,ControlBlock->consumerParam_s);
        }

        if(Error == MME_SUCCESS)
        {
            /* We need to try on all LX */
            for(j = 0; j < TOTAL_LX_PROCESSORS; j++)
            {
                memset(TransformerName,0,64);
                /* Generate the LX transfommer name */
                GetLxName(j,"AUDIO_DECODER",TransformerName,sizeof(TransformerName));
                Error = MME_InitTransformer(TransformerName, &ControlBlock->MMEinitParams, &ControlBlock->TranformHandleDecoder);
                if(Error == MME_SUCCESS)
                {
                    /* transformer initialized */
                    ControlBlock->Transformer_Init = TRUE;
                    break;
                }
                else
                {
                    STTBX_Print(("ERROR: Decoder Init Fail=0x%X\n", Error));
                    ControlBlock->Transformer_Init = FALSE;
                }
            }
        }
    }
    else
    {
        ControlBlock->decoderInByPassMode = TRUE;
        Error = MME_SUCCESS;
    }

    /* Init the sampling frequency to invaid value. LX Decoder ,in its callback, will inform the correct value */
    ControlBlock->SamplingFrequency = ACC_FS_reserved;
    ControlBlock->AudioCodingMode = ACC_MODE_undefined_3F;

    /* Signal the Command Completion Semaphore */
    STOS_SemaphoreSignal(ControlBlock->CmdCompletedSem_p);

    return Error;
}

/******************************************************************************
 *  Function name       : STAud_DecStopDecoder
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : MME_ERROR
 *  Description         :
 ***************************************************************************** */
MME_ERROR       STAud_DecStopDecoder(DecoderControlBlock_t      *ControlBlock)
{
    MME_ERROR   Error = MME_SUCCESS;
    U32 i,count=0, opBufWait=0, tempOutputQueueFront = 0, tempInputQueueFront = 0;
    DecoderInputCmdParam_t      *CurrentInputBufferToAbort;
    DecoderOutputCmdParam_t     *CurrentOutputBufferToAbort;
    DecoderInputCmdParam_t      *CurrentInputBufferToRelease;
    U32                     QueueFrontIndex;

    STTBX_Print(("STAud_DecStopDecoder: Called !!!\n "));

    /*Transformer restart*/
    ControlBlock->TransformerRestart = TRUE;
    ControlBlock->streamingTransformerFirstTransfer = FALSE;
    ControlBlock->StreamingEofFound = FALSE;

    /*  terminate all the transformers */
    if (ControlBlock->UseStreamingTransformer)
    {
        //Send Abort command for the output buffer first
        // Rahul Modified to abort output transform cmds in reverse order
        tempOutputQueueFront = ControlBlock->DecoderOutputQueueFront;

        //for (i = ControlBlock->DecoderOutputQueueFront; i < ControlBlock->DecoderOutputQueueTail; i++)
        for (i = (ControlBlock->DecoderOutputQueueTail - 1); ((i >= tempOutputQueueFront)/*ControlBlock->DecoderOutputQueueFront*/ && (ControlBlock->DecoderOutputQueueTail != ControlBlock->DecoderOutputQueueFront)) ; i--)
        {
            CurrentOutputBufferToAbort = &(ControlBlock->DecoderOutputQueue[i %STREAMING_DECODER_QUEUE_SIZE]);
            MME_AbortCommand(ControlBlock->TranformHandleDecoder,  CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId);
        }

        opBufWait = 0;
        while((ControlBlock->DecoderOutputQueueFront != ControlBlock->DecoderOutputQueueTail) && (opBufWait < 10))
        {
            AUD_TaskDelayMs(3);
            opBufWait++;
        }

        //send the abort command for the input buffers after all abort commands for
        //output is sent

        // Rahul Modified to abort send buffer cmds in reverse order
        //for (i = ControlBlock->DecoderInputQueueFront; i < ControlBlock->DecoderInputQueueTail; i++)
        tempInputQueueFront = ControlBlock->DecoderInputQueueFront;
        for (i = (ControlBlock->DecoderInputQueueTail - 1); ((i >= tempInputQueueFront) /*ControlBlock->DecoderInputQueueFront*/ && (ControlBlock->DecoderInputQueueTail != ControlBlock->DecoderInputQueueFront)); i--)
        {
            CurrentInputBufferToAbort = &(ControlBlock->DecoderInputQueue[i %STREAMING_DECODER_QUEUE_SIZE]);
            MME_AbortCommand(ControlBlock->TranformHandleDecoder,  CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId);
        }

        /* We should wait for both input queue and output queue to be freed before terminating the transformer*/
        while(ControlBlock->DecoderOutputQueueFront != ControlBlock->DecoderOutputQueueTail ||
            ControlBlock->DecoderInputQueueFront != ControlBlock->DecoderInputQueueTail)
        {
            AUD_TaskDelayMs(5);
            /* Make a bounded wait. */
            if((count/600))
            {
                /* We will come here only if LX is crashed so queueTails and Fronts will never be updated */
                /* Waited enough -- 5 * 600 = 3000ms = 3sec */
                STTBX_Print(("LX Not Responding.....Waiting since %d ms \n",count*5 ));
                STTBX_Print(("OutputQueueFront %d,OutputQueueTail %d,InputQueueFront %d,InputQueueTail %d \n",
                ControlBlock->DecoderOutputQueueFront,ControlBlock->DecoderOutputQueueTail,ControlBlock->DecoderInputQueueFront,ControlBlock->DecoderInputQueueTail));
                break;
            }
            count++;
        }
    }
    else
    {
        /* Frame Based decoder */
        if((ControlBlock->DecoderOutputQueueFront == ControlBlock->DecoderOutputQueueTail) &&
            (ControlBlock->DecoderInputQueueFront != ControlBlock->DecoderInputQueueTail))
        {
            /* Special Condition
            In frame based decoder, we had got input block and we were waiting for output block.
            Free the input block we were holding  */
            STTBX_Print((" Dec Stop- Special case \n"));
            while (ControlBlock->DecoderInputQueueFront != ControlBlock->DecoderInputQueueTail)
            {
                QueueFrontIndex = ControlBlock->DecoderInputQueueFront%STREAMING_DECODER_QUEUE_SIZE;
                CurrentInputBufferToRelease = &ControlBlock->DecoderInputQueue[QueueFrontIndex];

                for (i=0; i<CurrentInputBufferToRelease->DecoderBufIn.BufferIn_p->NumberOfScatterPages; i++)
                {
                    MemPool_FreeIpBlk(ControlBlock->InMemoryBlockManagerHandle,
                                    (U32 *)&(CurrentInputBufferToRelease->InputBlockFromProducer[i]),
                                    ControlBlock->Handle);
                }
                    CurrentInputBufferToRelease->DecoderBufIn.BufferIn_p->NumberOfScatterPages = 0;
                    ControlBlock->DecoderInputQueueFront++;
                }
        }

        /* We should wait for both input queue and output queue to be freed before terminating the transformer.
        GNBvd59558 - && operator ensure the special condition when multiple frrames are sent to LX for decoding and we have
        just got input block for next cmd. This means DecoderInputQueueFront never be equal to  DecoderInputQueueTail
        */
        while((ControlBlock->DecoderOutputQueueFront != ControlBlock->DecoderOutputQueueTail) &&
                (ControlBlock->DecoderInputQueueFront != ControlBlock->DecoderInputQueueTail))
        {
            AUD_TaskDelayMs(5);
            /* Make a bounded wait. */
            if((count/600))
            {
                /* We will come here only if LX is crashed so queueTails and Fronts will never be updated */
                /* Waited enough -- 5 * 600 = 3000ms = 3sec */
                STTBX_Print(("LX Not Responding.....Waiting since %d ms \n",count*5 ));
                STTBX_Print(("OutputQueueFront %d,OutputQueueTail %d,InputQueueFront %d,InputQueueTail %d \n",
                ControlBlock->DecoderOutputQueueFront,ControlBlock->DecoderOutputQueueTail,ControlBlock->DecoderInputQueueFront,ControlBlock->DecoderInputQueueTail));
                break;
            }
            count++;
        }
    }

    count=0;
    /*Wait till UpdateGlobalParamList does not become NULL --> All commands to LX are complete
    and the related memory has been de-allocated*/
    while(ControlBlock->UpdateGlobalParamList!=NULL)
    {
        if(ControlBlock->UpdateGlobalParamList)
        {
            int countpending=0;
            UpdateGlobalParamList_t *temp;

            temp=ControlBlock->UpdateGlobalParamList;
            STTBX_Print(("STAud_DecStopDecoder: UpdateGlobalParamList NOT NULL  \n"));

            while(temp)
            {
                countpending++;
                temp=temp->Next_p;
            }
            STTBX_Print(("No of commands pending in globalParamList= %d\n",countpending));
        }
        else
        {
            STTBX_Print(("STAud_DecStopDecoder: UpdateGlobalParamList=NULL \n"));
        }
#ifdef ST_OSWINCE
        Sleep(30);
#else
        AUD_TaskDelayMs(5);
#endif
        /* Make a bounded wait. */
        if((count/600))
        {
            /* We will come here only if STAud_DecPPRemCmdFrmList succeeds and no command is pending for completion*/
            /* Waited enough -- 5 * 600 = 3000ms = 3sec */
            STTBX_Print(("Callback not completed (Command List not empty).....Waiting since %d ms \n",count*5 ));
            break;
        }
        count++;
    }
    /* Terminate the Transformer */
    if(ControlBlock->Transformer_Init)
    {
        /* Try MME_TermTransformer few time as sometime LX return BUSY */
        count=0;
        do
        {
            Error = MME_TermTransformer(ControlBlock->TranformHandleDecoder);
            count++;
            AUD_TaskDelayMs(3);
        }while((Error != MME_SUCCESS) && (count < 4));
    }

    if(Error != MME_SUCCESS)
    {
        STTBX_Print(("ERROR!!!Decoder MME_TermTransformer::FAILED,Error = %x \n",Error));
    }
    else
    {
        /* We have been able to terminate the transformer properly which indicates
        that all the input buffers for which a transform was sent are released
        Only the last one (for which the transform was not send) COULD BE pending */
        //DecoderInputCmdParam_t        *CurrentInputBufferToRelease;
        //U32                     QueueFrontIndex;

        while (ControlBlock->DecoderInputQueueFront != ControlBlock->DecoderInputQueueTail)
        {
            QueueFrontIndex = ControlBlock->DecoderInputQueueFront%STREAMING_DECODER_QUEUE_SIZE;
            CurrentInputBufferToRelease = &ControlBlock->DecoderInputQueue[QueueFrontIndex];

            for (i=0; i<CurrentInputBufferToRelease->DecoderBufIn.BufferIn_p->NumberOfScatterPages; i++)
            {
                MemPool_FreeIpBlk(ControlBlock->InMemoryBlockManagerHandle,
                                    (U32 *)&(CurrentInputBufferToRelease->InputBlockFromProducer[i]),
                                    ControlBlock->Handle);
            }
            CurrentInputBufferToRelease->DecoderBufIn.BufferIn_p->NumberOfScatterPages = 0;
            ControlBlock->DecoderInputQueueFront++;
        }
    }

    /* Reset the decoder init */
    ControlBlock->Transformer_Init = FALSE;

    ControlBlock->DecInState = MME_DECODER_INPUT_STATE_IDLE;

    /* Reset Decoder queues */
    ControlBlock->DecoderOutputQueueFront = ControlBlock->DecoderOutputQueueTail=0;
    ControlBlock->DecoderInputQueueFront = ControlBlock->DecoderInputQueueTail=0;

    /* Reset stream contents */
    memset((void*)&ControlBlock->StreamParams, 0, sizeof(STAUD_StreamParams_t));

    /* UnRegister to memory block manager as a Producer */
    if(ControlBlock->OutMemoryBlockManagerHandle0)
    {
        Error |= MemPool_UnRegProducer(ControlBlock->OutMemoryBlockManagerHandle0, ControlBlock->Handle);
    }

    if(ControlBlock->OutMemoryBlockManagerHandle1)
    {
        Error |= MemPool_UnRegProducer(ControlBlock->OutMemoryBlockManagerHandle1, ControlBlock->Handle);
    }

    if (!ControlBlock->decoderInByPassMode)
    {
        /* Unregister from the block manager */
        /* UnRegister to memory block manager as a Consumer */
        if(ControlBlock->InMemoryBlockManagerHandle)
        {
            Error |= MemPool_UnRegConsumer(ControlBlock->InMemoryBlockManagerHandle,ControlBlock->Handle);
        }

    }

    /* Signal the Command Completion Semaphore - STOP Cmd*/
    STOS_SemaphoreSignal(ControlBlock->CmdCompletedSem_p);

    STTBX_Print(("STAud_DecStopDecoder: Return !!!\n "));

    return Error;
}



/**************************************************************************************************
 *  Function name       : STAud_DecInitDecoderQueueInParams
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Initalize Decoder queue params
 *                                1. Allocate and initialize MME structure for input buffers
 *                                2. Allocate and initialize MME structure for Output buffers
 **************************************************************************************************/
ST_ErrorCode_t STAud_DecInitDecoderQueueInParams(DecoderControlBlock_t  *TempDecBlock,
                                                STAVMEM_PartitionHandle_t AVMEMPartition)
{

    /* Local Variables */
    void   *VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t        VirtualMapping;
    STAVMEM_AllocBlockParams_t  AllocParams;
    U32 i, j;
    U32 BufferSize = 0 ;

    ST_ErrorCode_t Error = ST_NO_ERROR;
    /**************** Initialize Input param ********************/
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    AllocParams.PartitionHandle = AVMEMPartition;
    AllocParams.Alignment = 32;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocParams.ForbiddenRangeArray_p = NULL;
    AllocParams.ForbiddenBorderArray_p = NULL;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.NumberOfForbiddenBorders = 0;

    BufferSize = sizeof(MME_DataBuffer_t);
    AllocParams.Size = BufferSize * STREAMING_DECODER_QUEUE_SIZE;

    Error = STAVMEM_AllocBlock(&AllocParams, &TempDecBlock->BufferInHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 1 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(TempDecBlock->BufferInHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 2 %x",Error));
        return Error;
    }

    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        DecoderInputCmdParam_t  * DecIpQueue_p = &TempDecBlock->DecoderInputQueue[i];
        void * Address = (MME_DataBuffer_t *)VirtualAddress + i;

        DecIpQueue_p->DecoderBufIn.BufferIn_p = STAVMEM_VirtualToCPU(Address,&VirtualMapping);
    }

    BufferSize = sizeof(MME_ScatterPage_t);
    AllocParams.Size = BufferSize * NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM * STREAMING_DECODER_QUEUE_SIZE;
    Error = STAVMEM_AllocBlock(&AllocParams, &TempDecBlock->ScatterPageInHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 3 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(TempDecBlock->ScatterPageInHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 4 %x",Error));
        return Error;
    }
    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        for (j = 0; j < NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            DecoderInputCmdParam_t      * DecIpQueue_p = &TempDecBlock->DecoderInputQueue[i];
            void * Address = (MME_ScatterPage_t *)VirtualAddress + (j +(i * NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM));

            DecIpQueue_p->DecoderBufIn.ScatterPageIn_p[j]= STAVMEM_VirtualToCPU(Address,&VirtualMapping);
        }
    }

    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        for (j = 0; j < NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            /* We will set the Page_p to point to the compressed buffer, received from
            Parser unit, in Process_Input function */
            TempDecBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->Page_p = NULL;

            /* Data Buffers initialisation */
            TempDecBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->Size = 0;//COMPRESSED_BUFFER_SIZE;//2048;  update by the input buffer size
            TempDecBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->BytesUsed = 0;
            TempDecBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->FlagsIn = (unsigned int)MME_DATA_CACHE_COHERENT;
            TempDecBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[j]->FlagsOut = 0;
        }
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->StructSize    = sizeof (MME_DataBuffer_t);
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->UserData_p    = NULL;
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->Flags         = 0;
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->StreamNumber  = 0;
        // TBD needs to be updated
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->NumberOfScatterPages = 0;//NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM;
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->ScatterPages_p= TempDecBlock->DecoderInputQueue[i].DecoderBufIn.ScatterPageIn_p[0];
        //TempDecBlock->DecoderQueue[i].DecoderBufIn.BufferIn_p->TotalSize     = TempDecBlock->DecoderQueue[i].DecoderBufIn.BufferIn_p->ScatterPages_p->Size;
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->TotalSize                   = 0;
        TempDecBlock->DecoderInputQueue[i].DecoderBufIn.BufferIn_p->StartOffset         = 0;

        //TempDecBlock->DecoderInputQueue[i].NumberInputScatterPages = 0;
    }
    return ST_NO_ERROR;
}

/**************************************************************************************************
 *  Function name       : STAud_DecInitDecoderQueueOutParams
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Initalize Decoder queue params
 *                                1. Allocate and initialize MME structure for Output buffers
 **************************************************************************************************/
ST_ErrorCode_t STAud_DecInitDecoderQueueOutParams(DecoderControlBlock_t *TempDecBlock,
                                                STAVMEM_PartitionHandle_t AVMEMPartition)
{
    /* Local Variables */
    void                                *VirtualAddress;
    STAVMEM_SharedMemoryVirtualMapping_t        VirtualMapping;
    STAVMEM_AllocBlockParams_t                          AllocBlockParams;
    U32 i,j;
    U32 BufferSize = 0;

    ST_ErrorCode_t Error = ST_NO_ERROR;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    AllocBlockParams.PartitionHandle = AVMEMPartition;
    AllocBlockParams.Alignment = 32;
    AllocBlockParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.ForbiddenRangeArray_p = NULL;
    AllocBlockParams.ForbiddenBorderArray_p = NULL;
    AllocBlockParams.NumberOfForbiddenRanges = 0;
    AllocBlockParams.NumberOfForbiddenBorders = 0;

    BufferSize = sizeof(MME_DataBuffer_t);

    AllocBlockParams.Size = BufferSize * (STREAMING_DECODER_QUEUE_SIZE << 1); /* For output buffer 0 and 1 */

    Error = STAVMEM_AllocBlock(&AllocBlockParams, &TempDecBlock->BufferOutHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 1 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(TempDecBlock->BufferOutHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 2 %x",Error));
        return Error;
    }

    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        DecoderOutputCmdParam_t * DecOpQueue_p = &TempDecBlock->DecoderOutputQueue[i];
        void * Address0 = (MME_DataBuffer_t *)VirtualAddress + i;
        void * Address1 = (MME_DataBuffer_t *)Address0 + STREAMING_DECODER_QUEUE_SIZE;

        DecOpQueue_p->DecoderBufOut0.BufferOut_p = STAVMEM_VirtualToCPU(Address0,&VirtualMapping);
        DecOpQueue_p->DecoderBufOut1.BufferOut_p = STAVMEM_VirtualToCPU(Address1,&VirtualMapping);
    }

    /* For output scatter pages for buffer 0 & 1*/
    BufferSize = sizeof(MME_ScatterPage_t);
    AllocBlockParams.Size =  BufferSize * NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM * (STREAMING_DECODER_QUEUE_SIZE << 1); /*for buffer 0 & 1*/
    Error = STAVMEM_AllocBlock(&AllocBlockParams, &TempDecBlock->ScatterPageOutHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 3 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(TempDecBlock->ScatterPageOutHandle, &VirtualAddress);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 4 %x",Error));
        return Error;
    }

    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        for (j = 0; j < NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            DecoderOutputCmdParam_t     * DecOpQueue_p = &TempDecBlock->DecoderOutputQueue[i];
            void * Address0 = (MME_ScatterPage_t *)VirtualAddress + (j+i*NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM);
            void * Address1 = (MME_ScatterPage_t *)Address0 + NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM * STREAMING_DECODER_QUEUE_SIZE;

            DecOpQueue_p->DecoderBufOut0.ScatterPageOut_p[j]= STAVMEM_VirtualToCPU(Address0,&VirtualMapping);
            DecOpQueue_p->DecoderBufOut1.ScatterPageOut_p[j]= STAVMEM_VirtualToCPU(Address1,&VirtualMapping);
        }
    }

    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        MME_DataBuffer_t                        * BufferOut_p;
        DecoderOutputCmdParam_t * DecrOpQueue_p = &TempDecBlock->DecoderOutputQueue[i];

        for (j = 0; j < NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
        {
            /* We will set the Page_p to point to the Empty PCM buffer, received from
            Consumer unit, in Process_Input function */
            MME_ScatterPage_t           *ScatterPageOut_p = DecrOpQueue_p->DecoderBufOut0.ScatterPageOut_p[j];

            ScatterPageOut_p->Page_p = NULL;
            /* Data Buffers initialisation */
            ScatterPageOut_p->Size = 0;
            ScatterPageOut_p->BytesUsed = 0;
            ScatterPageOut_p->FlagsIn = (unsigned int)MME_DATA_CACHE_COHERENT;
            ScatterPageOut_p->FlagsOut = 0;
            ScatterPageOut_p = DecrOpQueue_p->DecoderBufOut1.ScatterPageOut_p[j];
            ScatterPageOut_p->Page_p = NULL;
            /* Data Buffers initialisation */
            ScatterPageOut_p->Size = 0;
            ScatterPageOut_p->BytesUsed = 0;
            ScatterPageOut_p->FlagsIn = (unsigned int)MME_DATA_CACHE_COHERENT;
            ScatterPageOut_p->FlagsOut = 0;
        }

        BufferOut_p = DecrOpQueue_p->DecoderBufOut0.BufferOut_p;
        BufferOut_p->StructSize    = sizeof (MME_DataBuffer_t);
        BufferOut_p->UserData_p    = NULL;
        BufferOut_p->Flags         = 0;
        BufferOut_p->StreamNumber  = 0;
        BufferOut_p->NumberOfScatterPages = 0;//NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM;
        BufferOut_p->ScatterPages_p = DecrOpQueue_p->DecoderBufOut0.ScatterPageOut_p[0];
        //BufferOut_p->TotalSize     = DecrOpQueue_p->DecoderBufOut.ScatterPageOut_p->Size;
        BufferOut_p->TotalSize     = 0;
        BufferOut_p->StartOffset   = 0;

        BufferOut_p = DecrOpQueue_p->DecoderBufOut1.BufferOut_p;

        BufferOut_p->StructSize    = sizeof (MME_DataBuffer_t);
        BufferOut_p->UserData_p    = NULL;
        BufferOut_p->Flags         = 0xCD; // For DDP transcoding //0;
        BufferOut_p->StreamNumber  = 0;
        BufferOut_p->NumberOfScatterPages = 0;//NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM;
        BufferOut_p->ScatterPages_p= DecrOpQueue_p->DecoderBufOut1.ScatterPageOut_p[0];
        //BufferOut_p->TotalSize     = DecrOpQueue_p->DecoderBufOut.ScatterPageOut_p->Size;
        BufferOut_p->TotalSize     = 0;
        BufferOut_p->StartOffset   = 0;
    }
    return ST_NO_ERROR;

}

void STAud_DecInitDecoderFrameParams(DecoderControlBlock_t *TempDecBlock)
{
    U32 i;
    for(i = 0; i < STREAMING_DECODER_QUEUE_SIZE; i++)
    {
        TempDecBlock->DecoderOutputQueue[i].FrameParams.Restart = FALSE;
        TempDecBlock->DecoderOutputQueue[i].FrameParams.Cmd           = ACC_CMD_PLAY;
        TempDecBlock->DecoderOutputQueue[i].FrameParams.PauseDuration = 0;
        TempDecBlock->DecoderOutputQueue[i].FrameParams.PTS = 0x0;
        TempDecBlock->DecoderOutputQueue[i].FrameParams.PtsFlags = 0x0;
    }
}

/******************************************************************************
 *  Function name       : STAud_DecDeInitDecoderQueueInParam
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : De allocate Decoder queue params
 *                                1. Deallocate Input buffers
 ***************************************************************************** */
ST_ErrorCode_t  STAud_DecDeInitDecoderQueueInParam(DecoderControlBlock_t *DecoderControlBlock)
{
    ST_ErrorCode_t      Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    STAVMEM_BlockHandle_t       ScatterPageInHandle,BufferInHandle;

    FreeBlockParams.PartitionHandle = DecoderControlBlock->InitParams.AVMEMPartition;
    ScatterPageInHandle = DecoderControlBlock->ScatterPageInHandle;

    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&ScatterPageInHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("4 E = %x",Error));
    }

    BufferInHandle = DecoderControlBlock->BufferInHandle;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&BufferInHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("5 E = %x",Error));
    }

    return Error;
}

/******************************************************************************
 *  Function name       : STAud_DecDeInitDecoderQueueOutParam
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : De allocate Decoder queue params
 *                                1. Deallocate output buffers
 ***************************************************************************** */
ST_ErrorCode_t  STAud_DecDeInitDecoderQueueOutParam(DecoderControlBlock_t *DecoderControlBlock)
{
    ST_ErrorCode_t      Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    STAVMEM_BlockHandle_t       ScatterPageHandle,BufferHandle;

    FreeBlockParams.PartitionHandle = DecoderControlBlock->InitParams.AVMEMPartition;
    ScatterPageHandle =         DecoderControlBlock->ScatterPageOutHandle;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&ScatterPageHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("7 E = %x",Error));
    }

    BufferHandle = DecoderControlBlock->BufferOutHandle;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&BufferHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print((" 8 E = %x",Error));
    }
    return Error;
}

/******************************************************************************
 *  Function name       : STAud_DecSendCommand
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : Send Command to decoder Task
 ***************************************************************************** */
ST_ErrorCode_t STAud_SendCommand(DecoderControlBlock_t *ControlBlock,MMEAudioDecoderStates_t    DesiredState)
{
    /* Get the current Decoder State */
    if(ControlBlock->CurDecoderState != DesiredState)
    {
        /* Send command to Decoder. Change its next state to Desired state */
        STOS_MutexLock(ControlBlock->LockControlBlockMutex_p);
        ControlBlock->NextDecoderState = DesiredState;
        STOS_MutexRelease(ControlBlock->LockControlBlockMutex_p);
        /* Signal Decoder task Semaphore */
        STOS_SemaphoreSignal(ControlBlock->DecoderTaskSem_p);
    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_DecSendCommand(MMEAudioDecoderStates_t     DesiredState,
                                    DecoderControlBlock_t *ControlBlock)
{
    switch(DesiredState)
    {
        case MME_DECODER_STATE_START:
            if((ControlBlock->CurDecoderState == MME_DECODER_STATE_IDLE) ||
                (ControlBlock->CurDecoderState == MME_DECODER_STATE_STOPPED))
            {
                /*Send Cmd */
                STAud_SendCommand(ControlBlock , DesiredState);
            }
            else
            {
                return STAUD_ERROR_INVALID_STATE;
            }
            break;

        case MME_DECODER_STATE_PLAYING:
            /* Internal State */
            if(ControlBlock->CurDecoderState == MME_DECODER_STATE_START)
            {
                /*Send Cmd */
                STAud_SendCommand(ControlBlock , DesiredState);
            }
            else
            {
                return STAUD_ERROR_INVALID_STATE;
            }
            break;

        case MME_DECODER_STATE_STOP:
            if((ControlBlock->CurDecoderState == MME_DECODER_STATE_START) ||
                (ControlBlock->CurDecoderState == MME_DECODER_STATE_PLAYING))
            {
                /*Send Cmd */
                STAud_SendCommand(ControlBlock , DesiredState);
            }
            else
            {
                return STAUD_ERROR_INVALID_STATE;
            }
            break;

        case MME_DECODER_STATE_STOPPED:
            /* Internal State */
            if(ControlBlock->CurDecoderState == MME_DECODER_STATE_STOP)
            {
                /*Send Cmd */
                STAud_SendCommand(ControlBlock , DesiredState);
            }
            else
            {
                return STAUD_ERROR_INVALID_STATE;
            }
            break;

        case MME_DECODER_STATE_TERMINATE:
            if((ControlBlock->CurDecoderState == MME_DECODER_STATE_STOPPED) ||
                (ControlBlock->CurDecoderState == MME_DECODER_STATE_IDLE))
            {
                /*Send Cmd */
                STAud_SendCommand(ControlBlock , DesiredState);
            }
            else
            {
                return STAUD_ERROR_INVALID_STATE;
            }
            break;
        case MME_DECODER_STATE_IDLE:
        default:
            break;
    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_DecMMETransformCompleted(MME_Command_t * CallbackData , void *UserData)
{
    /* Local variables */
    MME_Command_t               *command_p;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;
    STAud_DecHandle_t   Handle;

    /* Get the decoder refereance fro which this call back has come */
    Handle = (STAud_DecHandle_t)UserData;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("ERROR!!!MMETransformCompleted::NO Decoder Handle Found \n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    command_p       = (MME_Command_t *) CallbackData;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("ERROR!!!MMETransformCompleted::MME_COMMAND_FAILED \n"));
    }

    if (!(Dec_p->DecoderControlBlock.UseStreamingTransformer))
    {
        /* for frame based transformer release input block */
        STAud_DecReleaseInput(&Dec_p->DecoderControlBlock);
    }

    /*Get the Completed command and send decoded data to next unit */
    STAud_DecProcessOutput(&Dec_p->DecoderControlBlock,command_p);
    if (!(Dec_p->DecoderControlBlock.UseStreamingTransformer))
    {
        /* for frame based transformer trigger the main task */
        //STOS_SemaphoreSignal(Dec_p->DecoderControlBlock.DecoderTaskSem_p);
    }

    return ST_NO_ERROR;
}

/**************************************************************************************************
 *  Function name       : STAud_DecMMESetGlobalTransformCompleted
 *  Arguments           :
 *       IN                     :
 *       OUT                    :
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : One MME_SET_GLOBAL_TRANSFORM_PARAMS Completed. Remove the completed
 *                         command from List of update commands we sent. We will remove the first
 *                         node from the list. This is based on the assumption that decoder will
 *                         process the cmd sequentially i.e. it will return back the commands in
 *                         the order we sent them.Hence we remove the first command from list and
 *                         free it
 **************************************************************************************************/
ST_ErrorCode_t STAud_DecMMESetGlobalTransformCompleted(MME_Command_t * CallbackData , void *UserData)
{
    /* Local variables */
    MME_Command_t               *command_p;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;
    STAud_DecHandle_t   Handle;
    partition_t                                 *DriverPartition = NULL;
    TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gp_params;

    /*STTBX_Print(("MMESetGlobalTransformCompleted::Callback Received \n"));*/
    STTBX_Print(("\n MMEGblTraCom:CB\n"));
    /* Get the decoder refereance fro which this call back has come */
    Handle = (STAud_DecHandle_t)UserData;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("MMESetGlobalTransformCompleted::Decoder not found \n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    command_p       = (MME_Command_t *) CallbackData;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("MMESetGlobalTransformCompleted::MME_COMMAND_FAILED \n"));
    }

    /* Free the structures passed with command */
    gp_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) command_p->Param_p;
    if (gp_params->StructSize < sizeof(TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT))
    {
        /* This was allocated while we were building the PP command for sending to Decoder */
        DriverPartition = Dec_p->DecoderControlBlock.InitParams.DriverPartition;
        memory_deallocate(DriverPartition,gp_params);
    }

    /* Free the completed command */
    STAud_DecPPRemCmdFrmList(&Dec_p->DecoderControlBlock);

    /* Signal Decode Task Semaphore */
    //STOS_SemaphoreSignal(Dec_p->DecoderControlBlock.DecoderTaskSem_p);

    return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_DecMMESendBufferCompleted(MME_Command_t * CallbackData, void *UserData)
{
    /* Local variables */
    MME_Command_t               *command_p;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;
    STAud_DecHandle_t   Handle;

    /* Get the decoder refereance fro which this call back has come */
    Handle = (STAud_DecHandle_t)UserData;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("ERROR!!!MMETransformCompleted::NO Decoder Handle Found \n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    command_p       = (MME_Command_t *) CallbackData;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("ERROR!!!MMETransformCompleted::MME_COMMAND_FAILED \n"));
    }

    /*Get the Completed command and release the input buffer*/
    STAud_DecReleaseInput(&Dec_p->DecoderControlBlock);

    /* Signal Decode Task Semaphore */
    //STOS_SemaphoreSignal(Dec_p->DecoderControlBlock.DecoderTaskSem_p);
    // Rahul : Probably we dont need this

    return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name       : TransformerCallback_AudioDec
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Decoder Callback
 ***************************************************************************** */
void TransformerCallback_AudioDec(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{
    /* Local variables */
    MME_Command_t *command_p;
    static U32 ReceivedCount;
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
    if(ReceivedCount%500==499)
    STTBX_Print((" CB %d \n",ReceivedCount));
    ReceivedCount++;

    switch(command_p->CmdCode)
    {
        case MME_TRANSFORM:
            STAud_DecMMETransformCompleted(CallbackData,UserData);
            break;

        case MME_SET_GLOBAL_TRANSFORM_PARAMS:
            STAud_DecMMESetGlobalTransformCompleted(CallbackData,UserData);
            break;

        case MME_SEND_BUFFERS:
            STAud_DecMMESendBufferCompleted(CallbackData,UserData);
            break;

        default:
            break;
    }

}

ST_ErrorCode_t  HandleTransformUpdate(DecoderControlBlock_t *DecoderControlBlock, STAud_LpcmStreamParams_t *StreamParams, STAUD_StreamContent_t StreamType)
{
    /* Local variables */
    ST_ErrorCode_t                      error = ST_NO_ERROR;
    TARGET_AUDIODECODER_INITPARAMS_STRUCT       *init_params;
    TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
    MME_Command_t  * cmd=NULL;

    /* Decoder Config Params */
    MME_LxLpcmConfig_t * lpcm_config = NULL;

    /* Get decoder init params */
    init_params = DecoderControlBlock->MMEinitParams.TransformerInitParams_p;

    /* Get the Global Parmeters from the current decoder */
    gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;
    lpcm_config = (MME_LxLpcmConfig_t *)gbl_params->DecConfig;

    /* update the params */
    /* Common to DVD V and DVD A */
    lpcm_config->Config[LPCM_NB_CHANNELS] = StreamParams->channels;
    lpcm_config->Config[LPCM_WS_CH_GR1] = StreamParams->sampleBits ;
    lpcm_config->Config[LPCM_FS_CH_GR1] = CovertToFsLPCMCode(StreamParams->sampleRate);

    lpcm_config->Config[LPCM_CHANNEL_ASSIGNMENT]        = StreamParams->channelAssignment;
    if(StreamType == STAUD_STREAM_CONTENT_PCM)
    {
        if(((DecoderControlBlock->StreamContent == STAUD_STREAM_CONTENT_WAVE)&& (StreamParams->mode==1))||
                 (DecoderControlBlock->StreamContent == STAUD_STREAM_CONTENT_AIFF))
        {
            lpcm_config->Config[LPCM_NB_SAMPLES] = (CDDA_NB_SAMPLES << 1);
            lpcm_config->Config[LPCM_FS_CH_GR1] = CovertToFsCode(StreamParams->sampleRate);
        }
        else if((DecoderControlBlock->StreamContent == STAUD_STREAM_CONTENT_WAVE && StreamParams->mode!=1))
        {
            lpcm_config->Config[LPCM_NB_SAMPLES] = StreamParams->NbSamples;
            lpcm_config->Config[LPCM_MODE]                           = CovertToLPCMMode(StreamParams->mode);
            lpcm_config->Config[LPCM_BIT_SHIFT_CH_GR2] = StreamParams->bitShiftGroup2;
        }
        else
        {
            lpcm_config->Config[LPCM_NB_SAMPLES]    = CDDA_NB_SAMPLES;
            lpcm_config->Config[LPCM_FS_CH_GR1] = CovertToFsCode(StreamParams->sampleRate);
        }
        DecoderControlBlock->SamplingFrequency  = StreamParams->sampleRate;
    }
    else
    {
        lpcm_config->Config[LPCM_NB_SAMPLES]    = LPCMV_SAMPLES_PER_FRAME;
    }

    /* DVD A specific */
    if(StreamType == STAUD_STREAM_CONTENT_LPCM_DVDA)
    {
        lpcm_config->Config[LPCM_WS_CH_GR2]                     = StreamParams->sampleBitsGr2 ;
        lpcm_config->Config[LPCM_FS_CH_GR2]                     = StreamParams->sampleRateGr2;
        lpcm_config->Config[LPCM_BIT_SHIFT_CH_GR2]      = StreamParams->bitShiftGroup2;
        /* override the number of samples */
        lpcm_config->Config[LPCM_NB_SAMPLES]                    = LPCMA_SAMPLES_PER_FRAME;
    }

    /* Get the MME command which will be sent to Decoder */
    cmd = STAud_DecPPGetCmd(DecoderControlBlock);
    if( cmd == NULL)
    {
        /* No memory for command to be initialized */
        STTBX_Print(("HandleTransformUpdate: Error- No memory \n" ));
        return ST_ERROR_NO_MEMORY;
    }

    /* Fill and Send MME Command */
    error = acc_setAudioDecoderGlobalCmd(cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
                                        ACC_LAST_DECPCMPROCESS_ID, DecoderControlBlock->InitParams.DriverPartition);
    if(error != MME_SUCCESS)
    {
        /* Need some better error type */
        STTBX_Print(("HandleTransformUpdate: ERROR setAudioDecoderGlobalCmd: cmd = %x\n ",ACC_LAST_DECPCMPROCESS_ID));
        return ST_ERROR_NO_MEMORY;
    }

    /* Send MME_SET_GLOBAL_TRANSFORM_PARAMS Cmd */
    error = MME_SendCommand(DecoderControlBlock->TranformHandleDecoder, cmd);
    if(error != MME_SUCCESS)
        STTBX_Print(("HandleTransformUpdate: MME_SendCommand status = %x, cmd = %x \n ",error,(U32)cmd));

    return error;
}

ST_ErrorCode_t HandleDtsTransformUpdate(DecoderControlBlock_t *DecoderControlBlock, STAud_DtsStreamParams_t *StreamParams)
{
    /* Local variables */
    ST_ErrorCode_t                      error = ST_NO_ERROR;
    TARGET_AUDIODECODER_INITPARAMS_STRUCT       *init_params;
    TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
    MME_Command_t  * cmd=NULL;

    /* Decoder Config Params */
    MME_LxDtsConfig_t * dts_config = NULL;

    /* Get decoder init params */
    init_params = DecoderControlBlock->MMEinitParams.TransformerInitParams_p;

    /* Get the Global Parmeters from the current decoder */
    gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;
    dts_config = (MME_LxDtsConfig_t *)gbl_params->DecConfig;
    /* update the params */
    dts_config->FirstByteEncSamples             = StreamParams->FirstByteEncSamples;
    dts_config->Last4ByteEncSamples             = StreamParams->Last4ByteEncSamples;
    dts_config->DelayLossLess                   = StreamParams->DelayLossLess;
    dts_config->CoreSize                                = StreamParams->CoreSize;

    /* Get the MME command which will be sent to Decoder */
    cmd = STAud_DecPPGetCmd(DecoderControlBlock);
    if( cmd == NULL)
    {
        /* No memory for command to be initialized */
        STTBX_Print(("HandleDtsTransformUpdate: Error- No memory \n" ));
        return ST_ERROR_NO_MEMORY;
    }

    /* Fill and Send MME Command */
    error = acc_setAudioDecoderGlobalCmd(cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
                                        ACC_LAST_DECPCMPROCESS_ID, DecoderControlBlock->InitParams.DriverPartition);
    if(error != MME_SUCCESS)
    {
        /* Need some better error type */
        STTBX_Print(("HandleDtsTransformUpdate: ERROR setAudioDecoderGlobalCmd: cmd = %x\n ",ACC_LAST_DECPCMPROCESS_ID));
        return ST_ERROR_NO_MEMORY;
    }

    /* Send MME_SET_GLOBAL_TRANSFORM_PARAMS Cmd */
    error = MME_SendCommand(DecoderControlBlock->TranformHandleDecoder, cmd);
    if(error != MME_SUCCESS)
        STTBX_Print(("HandleDtsTransformUpdate: MME_SendCommand status = %x, cmd = %x \n ",error,(U32)cmd));

    return error;
}


ST_ErrorCode_t  HandleWMATransformUpdate(DecoderControlBlock_t *DecoderControlBlock, MemoryBlock_t *dataBufferIn)
{
    /* Local variables */
    ST_ErrorCode_t                      error = ST_NO_ERROR;
    TARGET_AUDIODECODER_INITPARAMS_STRUCT       *init_params;
    TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
    MME_Command_t  * cmd=NULL;
    /* Decoder Config Params */
    MME_LxWmaConfig_t * wma_config = NULL;

    /* Get decoder init params */
    init_params = DecoderControlBlock->MMEinitParams.TransformerInitParams_p;

    /* Get the Global Parmeters from the current decoder */
    gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;
    wma_config = (MME_LxWmaConfig_t *)gbl_params->DecConfig;

    wma_config->AudioStreamInfo.nVersion = dataBufferIn->Data[WMA_VERSION_OFFSET];
    wma_config->AudioStreamInfo.wFormatTag = dataBufferIn->Data[WMA_FORMAT_OFFSET];
    wma_config->AudioStreamInfo.nSamplesPerSec = dataBufferIn->Data[WMA_SAMPLES_P_SEC_OFFSET];
    wma_config->AudioStreamInfo.nAvgBytesPerSec = dataBufferIn->Data[WMA_AVG_BYTES_P_SEC_OFFSET];
    wma_config->AudioStreamInfo.nBlockAlign = dataBufferIn->Data[WMA_BLK_ALIGN_OFFSET];
    wma_config->AudioStreamInfo.nChannels = dataBufferIn->Data[WMA_NUMBER_CHANNELS_OFFSET];
    wma_config->AudioStreamInfo.nEncodeOpt = dataBufferIn->Data[WMA_ENCODE_OPT_OFFSET];
    wma_config->AudioStreamInfo.nSamplesPerBlock = dataBufferIn->Data[WMA_SAMPLES_P_BLK_OFFSET];
    wma_config->AudioStreamInfo.dwChannelMask = dataBufferIn->Data[WMA_CHANNEL_MASK_OFFSET];
    wma_config->AudioStreamInfo.nBitsPerSample = dataBufferIn->Data[WMA_BITS_P_SAMPLE_OFFSET];
    wma_config->AudioStreamInfo.wValidBitsPerSample = dataBufferIn->Data[WMA_VALID_BITS_P_SAMPLE_OFFSET];
    wma_config->AudioStreamInfo.wStreamId = dataBufferIn->Data[WMA_STREAM_ID_OFFSET];

    /* Get the MME command which will be sent to Decoder */
    cmd = STAud_DecPPGetCmd(DecoderControlBlock);
    if( cmd == NULL)
    {
        /* No memory for command to be initialized */
        STTBX_Print(("HandleTransformUpdate: Error- No memory \n" ));
        return ST_ERROR_NO_MEMORY;
    }

    /* Fill and Send MME Command */
    error = acc_setAudioDecoderGlobalCmd(cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
                                        ACC_LAST_DECPCMPROCESS_ID, DecoderControlBlock->InitParams.DriverPartition);
    if(error != MME_SUCCESS)
    {
        /* Need some better error type */
        STTBX_Print(("HandleWMATransformUpdate: ERROR setAudioDecoderGlobalCmd: cmd = %x\n ",ACC_LAST_DECPCMPROCESS_ID));
        return ST_ERROR_NO_MEMORY;
    }

    /* Send MME_SET_GLOBAL_TRANSFORM_PARAMS Cmd */
    error = MME_SendCommand(DecoderControlBlock->TranformHandleDecoder, cmd);
    if(error != MME_SUCCESS)
        STTBX_Print(("HandleWMATransformUpdate: MME_SendCommand status = %x, cmd = %x \n ",error,(U32)cmd));

    return error;
}

ST_ErrorCode_t  HandleWMAProLslTransformUpdate(DecoderControlBlock_t *DecoderControlBlock, MemoryBlock_t *dataBufferIn)
{
    /* Local variables */
    ST_ErrorCode_t                      error = ST_NO_ERROR;
    TARGET_AUDIODECODER_INITPARAMS_STRUCT       *init_params;
    TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
    MME_Command_t  * cmd=NULL;

    /* Decoder Config Params */
    MME_LxWmaProLslConfig_t * wmaprolsl_config = NULL;

    /* Get decoder init params */
    init_params = DecoderControlBlock->MMEinitParams.TransformerInitParams_p;

    /* Get the Global Parmeters from the current decoder */
    gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;
    wmaprolsl_config = (MME_LxWmaProLslConfig_t *)gbl_params->DecConfig;

    wmaprolsl_config->AudioStreamInfo.nVersion = dataBufferIn->Data[WMA_VERSION_OFFSET];
    wmaprolsl_config->AudioStreamInfo.wFormatTag = dataBufferIn->Data[WMA_FORMAT_OFFSET];
    wmaprolsl_config->AudioStreamInfo.nSamplesPerSec = dataBufferIn->Data[WMA_SAMPLES_P_SEC_OFFSET];
    wmaprolsl_config->AudioStreamInfo.nAvgBytesPerSec = dataBufferIn->Data[WMA_AVG_BYTES_P_SEC_OFFSET];
    wmaprolsl_config->AudioStreamInfo.nBlockAlign = dataBufferIn->Data[WMA_BLK_ALIGN_OFFSET];
    wmaprolsl_config->AudioStreamInfo.nChannels = dataBufferIn->Data[WMA_NUMBER_CHANNELS_OFFSET];
    wmaprolsl_config->AudioStreamInfo.nEncodeOpt = dataBufferIn->Data[WMA_ENCODE_OPT_OFFSET];
    wmaprolsl_config->AudioStreamInfo.nSamplesPerBlock = dataBufferIn->Data[WMA_SAMPLES_P_BLK_OFFSET];
    wmaprolsl_config->AudioStreamInfo.dwChannelMask = dataBufferIn->Data[WMA_CHANNEL_MASK_OFFSET];
    wmaprolsl_config->AudioStreamInfo.nBitsPerSample = dataBufferIn->Data[WMA_BITS_P_SAMPLE_OFFSET];
    wmaprolsl_config->AudioStreamInfo.wValidBitsPerSample = dataBufferIn->Data[WMA_VALID_BITS_P_SAMPLE_OFFSET];
    wmaprolsl_config->AudioStreamInfo.wStreamId = dataBufferIn->Data[WMA_STREAM_ID_OFFSET];

    /* Get the MME command which will be sent to Decoder */
    cmd = STAud_DecPPGetCmd(DecoderControlBlock);
    if( cmd == NULL)
    {
        /* No memory for command to be initialized */
        STTBX_Print(("HandleTransformUpdate: Error- No memory \n" ));
        return ST_ERROR_NO_MEMORY;
    }

    /* Fill and Send MME Command */
    error = acc_setAudioDecoderGlobalCmd(cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
                  ACC_LAST_DECPCMPROCESS_ID, DecoderControlBlock->InitParams.DriverPartition);
    if(error != MME_SUCCESS)
    {
        /* Need some better error type */
        STTBX_Print(("HandleWMAProLslTransformUpdate: ERROR setAudioDecoderGlobalCmd: cmd = %x\n ",ACC_LAST_DECPCMPROCESS_ID));
        return ST_ERROR_NO_MEMORY;
    }

    /* Send MME_SET_GLOBAL_TRANSFORM_PARAMS Cmd */
    error = MME_SendCommand(DecoderControlBlock->TranformHandleDecoder, cmd);
    if(error != MME_SUCCESS)
        STTBX_Print(("HandleWMAProLslTransformUpdate: MME_SendCommand status = %x, cmd = %x \n ",error,(U32)cmd));
    return error;
}


/**************************************************************************************************
 *  Function name       : STAud_DecIsInit
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : Search the list of decoder control blocks to find if "this" decoder is
 *                            already Initialized.
 **************************************************************************************************/
BOOL STAud_DecIsInit(const ST_DeviceName_t DeviceName)
{
    BOOL Initialized = FALSE;
    DecoderControlBlockList_t *queue = DecoderQueueHead_p;/* Global queue head pointer */

    while (queue)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(queue->DecoderControlBlock.DeviceName, DeviceName) != 0)
        {
            /* Next UART in the queue */
            queue = queue->Next_p;
        }
        else
        {
            /* The UART is already initialized */
            Initialized = TRUE;
            break;
        }
    }

    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;
}

/**************************************************************************************************
 *  Function name       : STAud_DecQueueAdd
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine appends an allocated Decoder control block to the
 *                                 Decoder queue.
 **************************************************************************************************/
void STAud_DecQueueAdd(DecoderControlBlockList_t *QueueItem_p)
{
    DecoderControlBlockList_t           *qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if (DecoderQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        DecoderQueueHead_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = DecoderQueueHead_p;
        while (qp != NULL && qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
} /* STAud_DecQueueAdd() */

/******************************************************************************
 *  Function name       : STAud_DecQueueRemove
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine removes a Decoder control block from the
 *                                 Decoder queue.
 ***************************************************************************** */
void *   STAud_DecQueueRemove( DecoderControlBlockList_t        *QueueItem_p)
{
    DecoderControlBlockList_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if (DecoderQueueHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = DecoderQueueHead_p;

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
                DecoderQueueHead_p = this_p->Next_p;
            }
        }
    }
    return (void*)DecoderQueueHead_p;
} /* STAud_DecQueueRemove() */
/******************************************************************************
 *  Function name       : STAud_DecGetControlBlockFromName
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine removes a Decoder control block from the
 *                                 Decoder queue.
 ***************************************************************************** */
DecoderControlBlockList_t *
   STAud_DecGetControlBlockFromName(const ST_DeviceName_t DeviceName)
{
    DecoderControlBlockList_t *qp = DecoderQueueHead_p; /* Global queue head pointer */

    while (qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->DecoderControlBlock.DeviceName, DeviceName) != 0)
        {
            /* Next UART in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The UART entry has been found */
            break;
        }
    }

    /* Return the UART control block (or NULL) to the caller */
    return qp;
} /* STAud_DecGetControlBlockFromName() */

/******************************************************************************
 *  Function name       : STAud_DecGetControlBlockFromHandle
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine returns the control block for a given handle
 ***************************************************************************** */
DecoderControlBlockList_t *
   STAud_DecGetControlBlockFromHandle(STAud_DecHandle_t Handle)
{
    DecoderControlBlockList_t *qp = DecoderQueueHead_p; /* Global queue head pointer */

    /* Iterate over the uart queue */
    while (qp != NULL)
    {
        /* Check for a matching handle */
        if (Handle == qp->DecoderControlBlock.Handle && Handle != 0)
        {
            break;  /* This is a valid handle */
        }

        /* Try the next block */
        qp = qp->Next_p;
    }

    /* Return the Dec control block (or NULL) to the caller */
    return qp;
}

/* Serve dual purpose - Ping to check if LX is initialized and Get LX firmware revision
    On successful return from this function, LX is properly initilized and LxRevision will
    hold firmware revision number
*/
ST_ErrorCode_t  STAud_DecPingLx(U8 *LxRevision, U32 StringSize)
{
    MME_TransformerCapability_t TransformerCapability;
    MME_ERROR           statusLx1 = MME_SUCCESS;
    U8    loop=0;
#if defined(ST_7200)
    MME_ERROR           statusLx2 =MME_SUCCESS;
    U8                  TransformerName0[64],TransformerName1[64];
    sprintf((char *)TransformerName0,"%s%d","PING_CPU_",LX_TRANSFORMER_NAME_SUFFIX0);
    sprintf((char *)TransformerName1,"%s%d","PING_CPU_",LX_TRANSFORMER_NAME_SUFFIX1);
#endif

    /* Fill in the capability struture */
    TransformerCapability.StructSize            = sizeof(MME_TransformerCapability_t);
    TransformerCapability.TransformerInfoSize = StringSize;
    TransformerCapability.TransformerInfo_p   = LxRevision;

    /* Request Ping Capability */
    while(1)
    {
#if (defined(ST_7109) ||defined(ST_7100))
        /* On 710x Audio CPU is at number 2 (see EMBX init structures) */
        statusLx1 = MME_GetTransformerCapability("PING_CPU_2", &TransformerCapability);
#elif defined(ST_7200)
        /* On 7200 Audio CPU id - 1 and 3 */
        statusLx1 = MME_GetTransformerCapability((char*)TransformerName0, &TransformerCapability);
        statusLx2 = MME_GetTransformerCapability((char*)TransformerName1, &TransformerCapability);
        if((statusLx1 == MME_SUCCESS) || (statusLx2 == MME_SUCCESS))
        {
            STTBX_Print(("LX Status :: LX1 = %d, LX2 = %d \n",statusLx1,statusLx2));
            /* if any one of LX is up, assume its ok...to be finalized ..*/
            statusLx1 = MME_SUCCESS;
        }
#endif
        /* Bounded wait - total wait period 100ms */
        if((statusLx1 == MME_SUCCESS) || (loop++ >= 25))
        {
            break;
        }
        else
        {
            AUD_TaskDelayMs(4); /* wait for 4ms, hoping that LX will be initialized */
        }
    }

    if(statusLx1== MME_SUCCESS)
        return ST_NO_ERROR;
    else
        return STAUD_ERROR_MME_TRANSFORM;
}

ST_ErrorCode_t  STAud_InitDecCapability(DecoderControlBlock_t *TempDecBlock)
{
    MME_TransformerCapability_t TransformerCapability;
    MME_LxAudioDecoderInfo_t    DecoderInfo;
    MME_ERROR                                   Error = MME_SUCCESS;
    U32                                                 i,j;
    char      TransformerName[64];

    /* Fill in the capability struture */
    TempDecBlock->DecCapability.DynamicRangeCapable     = TRUE;
    TempDecBlock->DecCapability.TempoCapable            = FALSE;
    TempDecBlock->DecCapability.FadingCapable           = FALSE;
    TempDecBlock->DecCapability.RunTimeSwitch           = FALSE;
    TempDecBlock->DecCapability.SupportedStopModes      = 0;

    TempDecBlock->DecCapability.SupportedStereoModes = (STAUD_STEREO_MODE_STEREO |STAUD_STEREO_MODE_PROLOGIC|
                                                        STAUD_STEREO_MODE_DUAL_LEFT |STAUD_STEREO_MODE_DUAL_RIGHT |
                                                        STAUD_STEREO_MODE_DUAL_MONO | STAUD_STEREO_MODE_AUTO);


    TempDecBlock->DecCapability.SupportedStreamTypes    = (STAUD_STREAM_TYPE_ES | STAUD_STREAM_TYPE_PES | STAUD_STREAM_TYPE_PES_DVD);
    TempDecBlock->DecCapability.SupportedTrickSpeeds[0] = 0;
    TempDecBlock->DecCapability.SupportedStreamContents = 0;
    TempDecBlock->DecCapability.SupportedEffects                = 0;
    TempDecBlock->DecCapability.DeEmphasisCapable               = FALSE;
    TempDecBlock->DecCapability.PrologicDecodingCapable = FALSE;

    /* Get capability for all LX processors */
    for(j = 0; j < TOTAL_LX_PROCESSORS; j++)
    {
        memset(&DecoderInfo, 0, sizeof(MME_LxAudioDecoderInfo_t));
        memset(&TransformerCapability, 0, sizeof(MME_TransformerCapability_t));
        TransformerCapability.StructSize                        = sizeof(MME_TransformerCapability_t);
        TransformerCapability.TransformerInfoSize       = sizeof(MME_LxAudioDecoderInfo_t);
        TransformerCapability.TransformerInfo_p         = &DecoderInfo;
        memset(TransformerName,0,64);
        /* Generate the LX transfommer name(based on ST231 number) */
        GetLxName(j,"AUDIO_DECODER",TransformerName, sizeof(TransformerName));
        Error = MME_GetTransformerCapability((char *)TransformerName, &TransformerCapability);
        if (Error == MME_SUCCESS)
        {
            STTBX_Print(("MME capability : Success for decoder\n"));
            for (i = 0; i < 32; i++)
            {
                switch (DecoderInfo.PcmProcessorCapabilityFlags[0] & (1 << i))
                {
                    case (1 << ACC_DeEMPH):
                        TempDecBlock->DecCapability.DeEmphasisCapable |= TRUE;
                        break;
                    case (1 << ACC_PLII):
                        TempDecBlock->DecCapability.PrologicDecodingCapable |= TRUE;
                        break;
                    case (1 << ACC_TSXT):
                        TempDecBlock->DecCapability.SupportedEffects |= (STAUD_EFFECT_SRS_3D |STAUD_EFFECT_TRUSURROUND |
                                                                                 STAUD_EFFECT_SRS_TRUSUR_XT | STAUD_EFFECT_SRS_TRUBASS | STAUD_EFFECT_SRS_FOCUS);
                        break;
                    case (1<<ACC_TEMPO_CONTROL) :
                        TempDecBlock->DecCapability.TempoCapable |= TRUE;
                        break;
                    default:
                        break;
                }
            }

            for (i=0; i< 32; i++)
            {
                switch (DecoderInfo.DecoderCapabilityFlags & (1 << i))
                {
#ifdef  STAUD_INSTALL_AC3
                    case (1 << ACC_AC3):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_AC3;
                    break;
#endif
#ifdef  STAUD_INSTALL_MPEG
                    case (1 << ACC_MP2a):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MPEG1 | STAUD_STREAM_CONTENT_MPEG2;
                    break;

                    case (1 << ACC_MP3):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MP3;
                    break;
#endif
#ifdef  STAUD_INSTALL_DTS
                    case (1 << ACC_DTS):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_DTS;
                    break;
#endif
#ifdef  STAUD_INSTALL_MLP
                    case (1 << ACC_MLP):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MLP;
                    break;
#endif
#if defined (STAUD_INSTALL_CDDA) || defined (STAUD_INSTALL_LPCMV) || defined(STAUD_INSTALL_LPCMA)
                    case (1 << ACC_LPCM): /* LPCM DVD */
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_PCM | STAUD_STREAM_CONTENT_CDDA | STAUD_STREAM_CONTENT_LPCM;
                    break;
#endif
#ifdef STAUD_INSTALL_WMA
                    case (1 << ACC_WMA9):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_WMA;
                    break;
#endif
#if defined (STAUD_INSTALL_HEAAC) || defined (STAUD_INSTALL_AAC)
                    case (1 << ACC_MP4a_AAC):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_MPEG_AAC | STAUD_STREAM_CONTENT_HE_AAC
                    | STAUD_STREAM_CONTENT_ADIF | STAUD_STREAM_CONTENT_MP4_FILE
                    | STAUD_STREAM_CONTENT_RAW_AAC;

                    break;
#endif
#ifdef STAUD_INSTALL_WMAPROLSL
                    case (1 << ACC_WMAPROLSL):
                    // WMA prolsl decoder can decode WMA9 streams also
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_WMAPROLSL | STAUD_STREAM_CONTENT_WMA;
                    break;
#endif
#ifdef  STAUD_INSTALL_AC3
                    case (1 << ACC_DDPLUS):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_DDPLUS;
                    break;
#endif
                    case (1 << ACC_GENERATOR):
                    TempDecBlock->DecCapability.SupportedStreamContents |= STAUD_STREAM_CONTENT_BEEP_TONE | STAUD_STREAM_CONTENT_PINK_NOISE;
                    break;

                    case (1 << ACC_PCM):
                    case (1 << ACC_SDDS):
                    case (1 << ACC_OGG):
                    case (1 << ACC_REAL_AUDIO):
                    case (1 << ACC_HDCD):
                    case (1 << ACC_AMRWB):
                    default:
                     break;
                }
            }
        }
        else
        {
            STTBX_Print(("MME capability : Failture for decoder\n"));
            return STAUD_ERROR_MME_TRANSFORM;
        }
    }
    return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name       : STAud_DecGetSamplFreq
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : Get Sampling Frequency
 ***************************************************************************** */
enum eAccFsCode STAud_DecGetSamplFreq(STAud_DecHandle_t Handle)
{

    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* return Sampling frequency for this decoder */
    return(CovertToFsCode(Dec_p->DecoderControlBlock.StreamParams.SamplingFrequency));
}

/******************************************************************************
 *  Function name       : STAud_DecGetBeepToneFreq
 *  Arguments           :
 *       IN                     :
 *       OUT                    : U32
 *       INOUT          :
 *  Return              : U32
 *  Description         : Get Sampling Frequency
 ***************************************************************************** */
U32 STAud_DecGetBeepToneFreq(STAud_DecHandle_t Handle, U32 *Freq_p)
{
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;
    U32 FreqConfig=0;
    U32 Tonefreq =0,BaseFreq=0;
    U8 ExpFreq;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* return Sampling frequency for this decoder */
    /*calculate the values to be configured */
    FreqConfig = Dec_p->DecoderControlBlock.BeepToneFrequency;
    Tonefreq = FreqConfig & 0xFF;
    BaseFreq = (Tonefreq & 0x3F) << 2;
    ExpFreq  = (U8)(Tonefreq >> 6);
    ExpFreq  = ((((100 << 16) + (10 << 8) + 1) >> (8 * ExpFreq)))&0xff;
    Tonefreq = BaseFreq * ExpFreq;
    *Freq_p = Tonefreq;

    return(Dec_p->DecoderControlBlock.BeepToneFrequency);
}

/******************************************************************************
 *  Function name       : STAud_DecGetBeepToneFreq
 *  Arguments           :
 *       IN                     :
 *       OUT                    : U32
 *       INOUT          :
 *  Return              : U32
 *  Description         : Get Sampling Frequency
 ***************************************************************************** */

ST_ErrorCode_t  STAud_DecSetBeepToneFreq(STAud_DecHandle_t              Handle,
                                        U32 BeepToneFrequency)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    U32 BaseCount = 0;
    U32 ExpCount = 0;
    U32 TempConfig;

    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;
    if(BeepToneFrequency<=252)
    {
        BeepToneFrequency = BeepToneFrequency - (BeepToneFrequency%4);
    }
    else if(BeepToneFrequency<=2520)
    {
        BeepToneFrequency = BeepToneFrequency - (BeepToneFrequency %40);
    }
    else if (BeepToneFrequency<=25200)
    {
        BeepToneFrequency = BeepToneFrequency - (BeepToneFrequency %400);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Set Beep tone Frequency  */
    for(ExpCount = 0; ExpCount < 4; ExpCount++)
    {
        for(BaseCount = 0; BaseCount < 64; BaseCount ++ )
        {
            TempConfig = ((((6556161 >> (8*ExpCount)) & 0xff)*BaseCount)<<2);
            if(TempConfig == BeepToneFrequency)
            {
                Dec_p->DecoderControlBlock.BeepToneFrequency = (ExpCount << 6 )| (BaseCount);
                return Error;
            }
        }
    }

    /*if not able to get the freq value by above eq than set the default value*/
    Dec_p->DecoderControlBlock.BeepToneFrequency = 0x4B; /*Default value */

    return Error;
}

/******************************************************************************
 *  Function name : STAud_DecSetDDPOP
 *  Arguments           :
 *       IN                     : Handle, handle of the driver
 *       IN                     : DDPOPSetting, output setting
 *  Return              : ST_ErrorCode_t
 *  Description   : Set DDPlus OutPut Setting
 ***************************************************************************** */

ST_ErrorCode_t  STAud_DecSetDDPOP(STAud_DecHandle_t     Handle, U32 DDPOPSetting)
{
    DecoderControlBlockList_t   *Dec_p = STAud_DecGetControlBlockFromHandle(Handle);

    if(Dec_p)
    {
        //only valid the frequency setting
        Dec_p->DecoderControlBlock.DDPOPSetting = (U8)DDPOPSetting;
    }
    else
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    return ST_NO_ERROR;
}

#if ENABLE_START_UP_PATTERN_GENERATION
void SetStartPattern(MemoryBlock_t      *outputBlock, U32 NUMChannels)
{
    S16 i;
    outputBlock->Size = START_UP_PATTERN_LENGTH_IN_SAMPLES * NUMChannels * 4;
    memset((U8 *)outputBlock->MemoryStart, 0, outputBlock->Size);

    if (NUMChannels == 2)
    {
        for (i=strlen(startuppattern2Channel);i>=0;i--)
        {
#ifdef ST_OS21
            *(U8 *)(outputBlock->MemoryStart +  (outputBlock->Size - (strlen(startuppattern2Channel) - i))) = toascii(startuppattern2Channel[i]);
#endif
        }
    }
    else if (NUMChannels == 10)
    {
        for (i=strlen(startuppattern10Channel);i>=0;i--)
        {
#ifdef ST_OS21
            *(U8 *)(outputBlock->MemoryStart +  (outputBlock->Size - (strlen(startuppattern10Channel) - i))) = toascii(startuppattern10Channel[i]);
#endif
        }
    }
}
#endif

#if ENABLE_EOF_PATTERN_GENERATION
void SetEOFPattern(MemoryBlock_t        *outputBlock, U32 NUMChannels)
{
    U8 i;
    memset((U8 *)(outputBlock->MemoryStart + outputBlock->Size), 0, EOF_PATTERN_LENGTH_IN_SAMPLES * NUMChannels * 4);
    for (i=0;i<=strlen(eofpattern);i++)
    {
#ifdef ST_OS21
        *(U8 *)(outputBlock->MemoryStart +  outputBlock->Size + i) = toascii(eofpattern[i]);
#endif
    }
    outputBlock->Size += EOF_PATTERN_LENGTH_IN_SAMPLES * NUMChannels * 4;
}
#endif

#if ENABLE_START_UP_PATTERN_GENERATION_FOR_TRANSCODER
void SetTranscoderStartPattern(MemoryBlock_t    *outputBlock, U32 compressedFrameSize)
{
    U32 i;
    outputBlock->Size = (compressedFrameSize - 2) * 4;
    memset((U8 *)outputBlock->MemoryStart, 0, outputBlock->Size);

    for (i=0;i<outputBlock->Size;i++)
    {
#ifdef ST_OS21
        *(U8 *)(outputBlock->MemoryStart +  i) = toascii(startuppattern2Channel[i % strlen(startuppattern2Channel)]);
#endif
    }
}
#endif

ST_ErrorCode_t  STAud_DecGetOPBMHandle(STAud_DecHandle_t        Handle, U32 OutputId, STMEMBLK_Handle_t * BMHandle_p)
{
    DecoderControlBlockList_t   *Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    ST_ErrorCode_t      Error=ST_NO_ERROR;

    if(Dec_p)
    {
        DecoderControlBlock_t   * DecCtrlBlk_p = &(Dec_p->DecoderControlBlock);
        switch(OutputId)
        {
            case 0:
                * BMHandle_p = DecCtrlBlk_p->OutMemoryBlockManagerHandle0;
                break;
            case 1:
                * BMHandle_p = DecCtrlBlk_p->OutMemoryBlockManagerHandle1;
                break;
            default:
                * BMHandle_p = 0;
                Error = ST_ERROR_BAD_PARAMETER;
            break;
        }
    }
    else
    {
        /* handle not found */
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}


ST_ErrorCode_t  STAud_DecSetIPBMHandle(STAud_DecHandle_t        Handle, U32 InputId, STMEMBLK_Handle_t BMHandle)
{
    DecoderControlBlockList_t   *Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    ST_ErrorCode_t      Error=ST_NO_ERROR;

    if(Dec_p)
    {
        DecoderControlBlock_t   * DecCtrlBlk_p = &(Dec_p->DecoderControlBlock);
        STOS_MutexLock(DecCtrlBlk_p->LockControlBlockMutex_p);
        switch(DecCtrlBlk_p->CurDecoderState)
        {
            case MME_DECODER_STATE_IDLE:
            case MME_DECODER_STATE_STOPPED:
                if(InputId == 0)
                {
                    DecCtrlBlk_p->InMemoryBlockManagerHandle = BMHandle;
                }
                else
                {
                    Error = ST_ERROR_BAD_PARAMETER;
                }
                break;
            default:
                /*We can't change the handle in running condition*/
                Error = STAUD_ERROR_INVALID_STATE;
                break;
        }
        STOS_MutexRelease(DecCtrlBlk_p->LockControlBlockMutex_p);
    }
    else
    {
    /* handle not found */
    Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}

void STAud_DecSetStreamInfo(DecoderControlBlock_t * ControlBlock, MemoryBlock_t * InputBlockFromProducer)
{
    if(InputBlockFromProducer->Flags & LAYER_VALID)
    {
        ControlBlock->TransformerRestart = TRUE;
        STTBX_Print(("LAYER UPDATE\n"));
    }
    if(InputBlockFromProducer->Flags & FADE_VALID)
    {
        ControlBlock->ADControl.Fade= InputBlockFromProducer->Data[FADE_OFFSET];
    }
    if(InputBlockFromProducer->Flags & PAN_VALID)
    {
        ControlBlock->ADControl.Pan = InputBlockFromProducer->Data[PAN_OFFSET];
        STTBX_Print(("Pan received :%d\n",ControlBlock->ADControl.Pan));
        if(ControlBlock->ADControl.Pan == 0xFF00 && ControlBlock->ADControl.PanErrorFlag!=1) /*Rampdown condition*/
        {
            ControlBlock->ADControl.Pan =  0;
            ControlBlock->ADControl.PanErrorFlag = 1;
            ControlBlock->ADControl.TargetPan = 0;

            if(ControlBlock->ADControl.CurPan != 0)// appply ramping only if curpan is not zero.
            {
                ControlBlock->ADControl.FramesLeftForRamp = ControlBlock->ADControl.NumFramesPerSecond;
                STAud_CalculateRamp(ControlBlock,ControlBlock->ADControl.NumFramesPerSecond);
            }
        }
        else if(ControlBlock->ADControl.Pan != 0xFF00)
        {
            if(ControlBlock->ADControl.PanErrorFlag==1)
            {
                ControlBlock->ADControl.FramesLeftForRamp = ControlBlock->ADControl.NumFramesPerSecond;
                STAud_CalculateRamp(ControlBlock,ControlBlock->ADControl.NumFramesPerSecond);
                ControlBlock->ADControl.TargetPan = ControlBlock->ADControl.Pan;
                ControlBlock->ADControl.PanErrorFlag = -1; //reset the error flag for error recovery
            }
            else if((ControlBlock->ADControl.TargetPan != ControlBlock->ADControl.Pan) && ControlBlock->ADControl.PanErrorFlag == -1)
            {
                STAud_CalculateRamp(ControlBlock,ControlBlock->ADControl.FramesLeftForRamp);
                ControlBlock->ADControl.TargetPan = ControlBlock->ADControl.Pan;
                ControlBlock->ADControl.PanErrorFlag = 0; //reset the error flag after recovery from error
            }
        }
    }

    STAud_ApplyPan(ControlBlock);

#ifndef MSPP_PARSER
    if((ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_WMA)
            && (InputBlockFromProducer->Flags & WMA_FLAG_MASK))
    {
        HandleWMATransformUpdate(ControlBlock,(void*)InputBlockFromProducer);
    }
    else if((ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_WMAPROLSL)
            && (InputBlockFromProducer->Flags & WMA_FLAG_MASK))
    {
        HandleWMAProLslTransformUpdate(ControlBlock,(void*)InputBlockFromProducer);
    }
#endif

}

void    STAud_ApplyPan(DecoderControlBlock_t * ControlBlock)
{
    int TempCurPan , TempTargetPan;
    if((ControlBlock->ADControl.TargetPan != ControlBlock->ADControl.CurPan))
    {
        STTBX_Print(("TAIL before applying:%d\n",ControlBlock->DecoderInputQueueTail));
        if(ControlBlock->ADControl.FrameNumber == ControlBlock->DecoderInputQueueTail)
        {
            TempTargetPan = ControlBlock->ADControl.TargetPan;
            TempCurPan = ControlBlock->ADControl.CurPan + ControlBlock->ADControl.RampFactor;
            if(ControlBlock->ADControl.RampFactor > 0)
            {
                TempCurPan = TempCurPan % 256 ;
                ControlBlock->ADControl.PanDiff = ControlBlock->ADControl.PanDiff - ControlBlock->ADControl.RampFactor;
            }
            else
            {
                TempCurPan = ((TempTargetPan)&&(TempCurPan<0))?(256 + TempCurPan):TempCurPan;
                ControlBlock->ADControl.PanDiff = ControlBlock->ADControl.PanDiff + ControlBlock->ADControl.RampFactor;
            }

            ControlBlock->ADControl.CurPan = ((ControlBlock->ADControl.PanDiff)<0)?ControlBlock->ADControl.TargetPan:(U32)TempCurPan;
            ControlBlock->ADControl.FrameNumber = ControlBlock->ADControl.FrameNumber + ControlBlock->ADControl.ApplyFrameFactor;
            ControlBlock->ADControl.FramesLeftForRamp = ControlBlock->ADControl.FramesLeftForRamp - ControlBlock->ADControl.ApplyFrameFactor;
            STTBX_Print(("TAIL after applying:%d\n",ControlBlock->DecoderInputQueueTail));
            STTBX_Print(("Frame number for Pan :%d\n",ControlBlock->ADControl.FrameNumber));
            STAud_DecPPSetPan(ControlBlock->Handle);
        }
    }
    else if((ControlBlock->ADControl.Pan != ControlBlock->ADControl.CurPan)&&(ControlBlock->ADControl.PanErrorFlag==0))
    {
        ControlBlock->ADControl.CurPan = ControlBlock->ADControl.Pan;
        ControlBlock->ADControl.TargetPan = ControlBlock->ADControl.CurPan;
        STAud_DecPPSetPan(ControlBlock->Handle);
    }
    else
    {
        ControlBlock->ADControl.ApplyFrameFactor = 0;
        if(ControlBlock->ADControl.PanErrorFlag == -1)
            ControlBlock->ADControl.PanErrorFlag = 0;
    }
}


void  STAud_CalculateRamp(DecoderControlBlock_t * ControlBlock, U32 NumberOfFrames)
{
    U32 Pan = ControlBlock->ADControl.Pan;
    U32 CurPan = ControlBlock->ADControl.CurPan;
    U32 PanDiff;
    ControlBlock->ADControl.FrameNumber = ControlBlock->DecoderInputQueueTail;

    if(CurPan > Pan)
    {
        PanDiff = CurPan - Pan;
        ControlBlock->ADControl.RampFactor = -1;
    }
    else if(CurPan < Pan)
    {
        PanDiff = Pan - CurPan;
        ControlBlock->ADControl.RampFactor = 1;
    }
    else
    {
        ControlBlock->ADControl.ApplyFrameFactor=0;
        ControlBlock->ADControl.RampFactor=0;
        return;
    }

    if(PanDiff>128)
    {
        PanDiff = 256 - PanDiff;
        ControlBlock->ADControl.RampFactor = ControlBlock->ADControl.RampFactor * -1;
    }

    ControlBlock->ADControl.PanDiff = PanDiff;
    ControlBlock->ADControl.ApplyFrameFactor = (10*NumberOfFrames + 5*PanDiff)/(PanDiff*10);
    if(!ControlBlock->ADControl.ApplyFrameFactor)
    {
        ControlBlock->ADControl.ApplyFrameFactor = 1;
        ControlBlock->ADControl.RampFactor = ControlBlock->ADControl.RampFactor * (PanDiff*10 + 5*NumberOfFrames)/(NumberOfFrames*10);
    }
}

/******************************************************************************
 *  Function name       : STAud_DecSetInitParams
 *  Arguments           :
 *       IN                     : Variable from application to set init params
 *       OUT            : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This
 ***************************************************************************** */
ST_ErrorCode_t  STAud_DecSetInitParams(STAud_DecHandle_t Handle, STAUD_DRInitParams_t * InitParams_p)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Dec_p->DecoderControlBlock.CurDecoderState!=MME_DECODER_STATE_PLAYING)&&(Dec_p->DecoderControlBlock.CurDecoderState!=MME_DECODER_STATE_START))
    {
        if(InitParams_p->SBRFlag)
            Dec_p->DecoderControlBlock.SBRFlag = ACC_MME_TRUE;
        else
            Dec_p->DecoderControlBlock.SBRFlag = ACC_MME_FALSE;
    }
    else
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    return Error;
}


/******************************************************************************
 *  Function name       : STAud_DecGetSamplingFrequency
 *  Arguments           :
 *       IN                     : Variable from application to store frequency
 *       OUT            : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This
 ***************************************************************************** */
ST_ErrorCode_t  STAud_DecGetStreamInfo(STAud_DecHandle_t                Handle, STAUD_StreamInfo_t * StreamInfo_p)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    DecoderControlBlockList_t   *Dec_p= (DecoderControlBlockList_t*)NULL;

    Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
    if(Dec_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Dec_p->DecoderControlBlock.CurDecoderState!=MME_DECODER_STATE_PLAYING)||(Dec_p->DecoderControlBlock.SamplingFrequency==ACC_FS_reserved))
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    (StreamInfo_p->StreamContent) = Dec_p->DecoderControlBlock.StreamContent;
    (StreamInfo_p->BitRate) = 0;
    (StreamInfo_p->SamplingFrequency) = CovertFsCodeToSampleRate(Dec_p->DecoderControlBlock.SamplingFrequency);
    (StreamInfo_p->LayerNumber) = 0;
    (StreamInfo_p->Emphasis) = (BOOL)Dec_p->DecoderControlBlock.Emphasis;
    (StreamInfo_p->CopyRight) = 0;
    (StreamInfo_p->Mode) = 0;
    (StreamInfo_p->Bit_Rate_Code) = 0;
    (StreamInfo_p->Bit_Stream_Mode) = 0;
    (StreamInfo_p->Audio_Coding_Mode) = (U8)Dec_p->DecoderControlBlock.InputCodingMode;

    return Error;
}
