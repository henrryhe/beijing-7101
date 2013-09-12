
/* {{{ Includes */
//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include <string.h>
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_mmeaudiostreamencoder.h"
//#include "aud_mmeaudioencoderpp.h"
#include "acc_multicom_toolbox.h"

/* }}} */

/* Encoder Instance Array */
ST_DeviceName_t Encoder_DeviceName[] = {"ENC0","ENC1"};

/* {{{ Testing Params */
#define INPUT_FROM_FILE 0
#if INPUT_FROM_FILE
    #define         FRAMES  5
    #define         MPEG    1152
    #define         AC3             1536
    #define         CDDA    2352
    #define         FSIZE   CDDA

    char InBuf[FSIZE*FRAMES]; /* NO_FRAMES of MP2 audio */

#endif
#define DUMP_OUTPUT_TO_FILE 0
#if DUMP_OUTPUT_TO_FILE
        #define FRAMES  300
        char OutBuf[1152*4*2*FRAMES]; /* FRAMES of MP2 audio */
#endif

#define ENC_LATENCY_OFFSET 10
#define MILLISECOND_TO_PTS(v)           ((v) * 90)
/* }}} */

/* {{{ Global Variables */

/* Initialized Encoder list. New encoder will be added to this list on initialization. */
EncoderControlBlockList_t       *EncoderQueueHead_p = NULL;
/* }}} */


/******************************************************************************
 *  Function name       : STAud_EncInit
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will create the encoder task, initialize MME encoders's In/out
 *                                paramaters and will update main HAL structure with current values
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncInit(const ST_DeviceName_t DeviceName,
                           const STAud_EncInitParams_t *InitParams)
{
    /* Local variables */
    ST_ErrorCode_t                  error = ST_NO_ERROR;
    EncoderControlBlockList_t       *EncCtrlBlock_p = NULL;
    partition_t                             *DriverPartition = NULL;
    EncoderControlBlock_t           *TempEncBlock_p;

    /* First Check if this encoder is not already open */
    if(!STAud_EncIsInit(DeviceName))
    {
        /* Encoder not already initialized. So do the initialization */

        /* Allocate memory for new Encoder control block */
        DriverPartition = (partition_t *)InitParams->DriverPartition;

        EncCtrlBlock_p = (EncoderControlBlockList_t *)
                          memory_allocate(DriverPartition,sizeof(EncoderControlBlockList_t));

        if(EncCtrlBlock_p == NULL)
        {
            /* Error No memory available */
            return ST_ERROR_NO_MEMORY;
        }

        /* Reset the structure */
        memset((void *)EncCtrlBlock_p, 0,sizeof(EncoderControlBlockList_t));

        /* Fill in the details of our new control block */
        EncCtrlBlock_p->EncoderControlBlock.Handle    = 0;
        strncpy(EncCtrlBlock_p->EncoderControlBlock.DeviceName,
                DeviceName, ST_MAX_DEVICE_NAME_TOCOPY);


        EncCtrlBlock_p->EncoderControlBlock.InitParams = *InitParams;

        /* Initialize MME Structure */
        TempEncBlock_p = &EncCtrlBlock_p->EncoderControlBlock;

        TempEncBlock_p->MMEinitParams.Priority                  = MME_PRIORITY_NORMAL;
        TempEncBlock_p->MMEinitParams.StructSize                = sizeof (MME_TransformerInitParams_t);
        TempEncBlock_p->MMEinitParams.Callback                  = &TransformerCallback_AudioEnc;
        TempEncBlock_p->MMEinitParams.CallbackUserData          = NULL; /* To be Initialized in STAud_EncStart */
        TempEncBlock_p->MMEinitParams.TransformerInitParamsSize = sizeof(MME_AudioEncoderInitParams_t);
        TempEncBlock_p->MMEinitParams.TransformerInitParams_p   = memory_allocate(DriverPartition,
                                                                sizeof(MME_AudioEncoderInitParams_t));

        if(TempEncBlock_p->MMEinitParams.TransformerInitParams_p == NULL)
        {
            /*Free the Control Block */
            memory_deallocate(DriverPartition,EncCtrlBlock_p);
            return ST_ERROR_NO_MEMORY;
        }

        memset((void *)TempEncBlock_p->MMEinitParams.TransformerInitParams_p, 0,
               sizeof(MME_AudioEncoderInitParams_t));


        TempEncBlock_p->EncodedStreamContent          = STAUD_STREAM_CONTENT_AC3;
        TempEncBlock_p->OutputParams.AutoOutputParam  = TRUE;
        TempEncBlock_p->InputParams.NumChannels       = InitParams->NumChannels;

        /* Create the Encoder Task Semaphore */
        TempEncBlock_p->EncoderTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);
        /* Create the Control Block Mutex */
        TempEncBlock_p->LockControlBlockMutex_p = STOS_MutexCreateFifo();

        /* Create the Command Competion Semaphore */
        TempEncBlock_p->CmdCompletedSem_p = STOS_SemaphoreCreateFifo(NULL,0);

        /* Create the Post processing Command list Mutex */
        TempEncBlock_p->PPCmdMutex_p = STOS_MutexCreateFifo();

        /* Initialize and Allocate Encoder Queue's buffer pointer */
        error |=EncInitEncoderQueueInParams(TempEncBlock_p, InitParams->AVMEMPartition);
        error |=EncInitEncoderQueueOutParams(TempEncBlock_p, InitParams->AVMEMPartition);

        /* Set the Encoder Starting State */
        TempEncBlock_p->CurEncoderState = MME_ENCODER_STATE_IDLE;
        TempEncBlock_p->NextEncoderState = MME_ENCODER_STATE_IDLE;
        TempEncBlock_p->Transformer_Init = FALSE;

        /* Set Input processing state */
        TempEncBlock_p->EncInState = MME_ENCODER_INPUT_STATE_IDLE;

        /*Reset The transformer restart required*/
        TempEncBlock_p->TransformerRestart = TRUE;

        /* Reset Encoder queues */
        TempEncBlock_p->EncoderInputQueueFront = TempEncBlock_p->EncoderInputQueueTail=0;
        TempEncBlock_p->EncoderOutputQueueFront = TempEncBlock_p->EncoderOutputQueueTail=0;


        TempEncBlock_p->WmaeAsfHeaderReceived = FALSE;
        TempEncBlock_p->WmaeStatus.ErrorCode = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.nAudioDelayBuffer = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.PacketSize = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SamplesPerPack = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SamplingFreq = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SizeOf_s_asf_header_type = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SizeOf_s_data_obj = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SizeOf_s_ext_obj = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SizeOf_s_file_obj = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SizeOf_s_header_obj = 0;
        TempEncBlock_p->WmaeStatus.AsfPostProcessing.SizeOf_s_stream_obj = 0;
        memset(&TempEncBlock_p->WmaeAsfHeaderBlock,0,sizeof(MemoryBlock_t));
        TempEncBlock_p->WmaeFirstOpBlkReceived = FALSE;
        TempEncBlock_p->WmaeLastOpBlkDelivered = FALSE;
        TempEncBlock_p->WmaeEncodedDataSize = 0;

        STOS_TaskCreate(STAud_EncTask_Entry_Point,&(EncCtrlBlock_p->EncoderControlBlock),NULL,
                        STREAMING_AUDIO_ENCODER_TASK_STACK_SIZE,NULL,NULL,
                        &TempEncBlock_p->EncoderTask,NULL,STREAMING_AUDIO_ENCODER_TASK_PRIORITY,
                        DeviceName,0);


        if (TempEncBlock_p->EncoderTask == NULL)
        {
            /* Error */
            /*Free the TransformerInitParams_p */
            memory_deallocate(DriverPartition,TempEncBlock_p->MMEinitParams.TransformerInitParams_p);

            /* Terminate Encoder Semaphore */
            STOS_SemaphoreDelete(NULL,TempEncBlock_p->EncoderTaskSem_p);

            /* Delete Mutex */
            STOS_MutexDelete(TempEncBlock_p->LockControlBlockMutex_p);

            /* Delete Mutex - PP cmd*/
            STOS_MutexDelete(TempEncBlock_p->PPCmdMutex_p);

            /* Terminate Cmd Completion Semaphore */
            STOS_SemaphoreDelete(NULL,TempEncBlock_p->CmdCompletedSem_p);

            /*Free the Control Block */
            memory_deallocate(DriverPartition,EncCtrlBlock_p);
            return ST_ERROR_NO_MEMORY;
        }

        /* Get Buffer Manager Handles */
        EncCtrlBlock_p->EncoderControlBlock.InMemoryBlockManagerHandle = InitParams->InBufHandle;
        EncCtrlBlock_p->EncoderControlBlock.OutMemoryBlockManagerHandle = InitParams->OutBufHandle;

        /* Only Initialize with Buffer Manager Params. We will register/unregister with Block manager in DRStart/DRStop */
        /* As Consumer of incoming data */
        TempEncBlock_p->consumerParam_s.Handle = (U32)EncCtrlBlock_p;
        TempEncBlock_p->consumerParam_s.sem    = EncCtrlBlock_p->EncoderControlBlock.EncoderTaskSem_p;
        /* Register to memory block manager as a consumer */
        //error |= MemPool_RegConsumer(EncCtrlBlock_p->EncoderControlBlock.InMemoryBlockManagerHandle,TempEncBlock_p->consumerParam_s);

        /* As Producer of outgoing data */
        TempEncBlock_p->ProducerParam_s.Handle = (U32)EncCtrlBlock_p;
        TempEncBlock_p->ProducerParam_s.sem    = EncCtrlBlock_p->EncoderControlBlock.EncoderTaskSem_p;

        /* Initialize the Encoder Capability structure */
        STAUD_GetEncCapability(&(TempEncBlock_p->EncCapability));

        /* Update the intitialized Encoder queue with current control block */
        STAud_EncQueueAdd(EncCtrlBlock_p);
    }
    else
    {
        /* Encoder already initialized. */
        error = ST_ERROR_ALREADY_INITIALIZED;
    }
#if INPUT_FROM_FILE
    FILE *fp = NULL;
    U32 size;
    //fp = fopen("c:\\trumpet.mpg","rb");
    fp = fopen("c:\\cdda.raw","rb");
    fseek (fp,FSIZE*6,SEEK_SET);
    size=fread(InBuf,1,FSIZE*FRAMES,fp);
#endif

    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_EncInit: Failed to Init Successfully\n "));
    }
    else
    {
        STTBX_Print(("STAud_EncInit: Successful \n "));
    }

    return error;
}

/******************************************************************************
 *  Function name       : STAud_EncTerm
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will terminate the encoder task, Free MME encoders's In/out
 *                                paramaters and will update main HAL structure with current values
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncTerm(STAud_EncHandle_t Handle)
{
    /* Local Variable */
    ST_ErrorCode_t                  error = ST_NO_ERROR;
    EncoderControlBlockList_t       *Enc_p;
    partition_t                             *DriverPartition = NULL;
    task_t                                  *TempTask_p;

    STTBX_Print(("STAud_EncTerm: Entered\n "));

    /* Obtain the Encoder control block associated with the device name */
    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);

    if(Enc_p == NULL)
    {
        /* No sunch Device found */
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Terminate Encoder Task */
    TempTask_p = Enc_p->EncoderControlBlock.EncoderTask;

    error |= STAud_EncSendCommand(MME_ENCODER_STATE_TERMINATE, &Enc_p->EncoderControlBlock);
    if(error == STAUD_ERROR_INVALID_STATE)
    {
        STTBX_Print(("STAud_EncTerm: Error !!!\n "));
        return error;
    }

    STOS_TaskWait(&TempTask_p,TIMEOUT_INFINITY);
    STOS_TaskDelete(TempTask_p,NULL,NULL,NULL);


    DriverPartition = Enc_p->EncoderControlBlock.InitParams.DriverPartition;

    /* Free Encoder queue Structures */
    error |= EncDeInitEncoderQueueInParam(&Enc_p->EncoderControlBlock);
    error |= EncDeInitEncoderQueueOutParam(&Enc_p->EncoderControlBlock);
    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_EncTerm: Failed to deallocate queue\n "));
    }

    /* Free the MME Param Structure */
    memory_deallocate(DriverPartition, Enc_p->EncoderControlBlock.MMEinitParams.TransformerInitParams_p);

    /* Terminate Encoder Semaphore */
    STOS_SemaphoreDelete(NULL,Enc_p->EncoderControlBlock.EncoderTaskSem_p);
    /* Delete Mutex */
    STOS_MutexDelete(Enc_p->EncoderControlBlock.LockControlBlockMutex_p);

    /* Delete Mutex - PP cmd*/
    STOS_MutexDelete(Enc_p->EncoderControlBlock.PPCmdMutex_p);

    /* Terminate Cmd Completion Semaphore */
    STOS_SemaphoreDelete(NULL,Enc_p->EncoderControlBlock.CmdCompletedSem_p);

    /* Remove the Control Block from Queue */
    STAud_EncQueueRemove(Enc_p);

    /*Free the Control Block */
    memory_deallocate(DriverPartition,Enc_p);

    if(error != ST_NO_ERROR)
    {
        STTBX_Print(("STAud_EncTerm: Failed to Terminate Successfully\n "));
    }
    else
    {
        STTBX_Print(("STAud_EncTerm: Successfully\n "));
    }
    return error;
}


/******************************************************************************
 *  Function name       : STAud_EncOpen
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will Open the encoder
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncOpen(const ST_DeviceName_t DeviceName,
                                                                        STAud_EncHandle_t *Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    EncoderControlBlockList_t       *Enc_p;

    /* Get the desired control block */
    Enc_p = STAud_EncGetControlBlockFromName(DeviceName);

    if(Enc_p == NULL)
    {
        /* Device not initialized */
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Check nobody else has already opened it */
    if (Enc_p->EncoderControlBlock.Handle == 0)
    {
        /* Set and return Handle */
        Enc_p->EncoderControlBlock.Handle = (STAud_EncHandle_t)Enc_p;
        *Handle = Enc_p->EncoderControlBlock.Handle;
    }

    STTBX_Print(("STAud_EncOpen: Successful \n "));
    return error;
}


/******************************************************************************
 *  Function name       : STAud_EncClose
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will close the task
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncClose()
{
    return ST_NO_ERROR;
}


ST_ErrorCode_t  STAud_EncSetStreamParams(STAud_EncHandle_t Handle,
                                         STAUD_StreamParams_t * StreamParams_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    // Set encoder specidifc parameters
    Enc_p->EncoderControlBlock.EncodedStreamContent = StreamParams_p->StreamContent;

    return Error;

}

/******************************************************************************
 *  Function name       : STAud_EncStart
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will send init and play cmd to Encoder Task. This
 *                                 function should be called from DRStart() function
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncStart(STAud_EncHandle_t Handle)
{
    ST_ErrorCode_t  Error            = ST_NO_ERROR;
    EncoderControlBlockList_t *Enc_p = (EncoderControlBlockList_t*)NULL;

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Check the Encoder capabilities and then start the encoder */
    if(Enc_p->EncoderControlBlock.EncodedStreamContent &
       Enc_p->EncoderControlBlock.EncCapability.SupportedStreamContents)
    {
        /* Send the Start command to encoder. */
        Error = STAud_EncSendCommand(MME_ENCODER_STATE_START, &Enc_p->EncoderControlBlock);

        if(Error == ST_NO_ERROR)
        {
            /* Wait on sema for START completion */
            STOS_SemaphoreWait(Enc_p->EncoderControlBlock.CmdCompletedSem_p);
        }
    }
    else
    {
        /* Encoder capability missing in bin. Return Error */
        Enc_p->EncoderControlBlock.Transformer_Init = FALSE;
        Error = STAUD_ERROR_ENCODER_START;
    }

    return Error;
}

/******************************************************************************
 *  Function name       : STAud_EncStop
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : This function will Send the Stop cmd to Encoder Task. This
 *                                 function should be called from DRStop() function
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncStop(STAud_EncHandle_t Handle)
{

    ST_ErrorCode_t  Error = ST_NO_ERROR;
    EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;

    STTBX_Print(("STAud_EncStop: Called \n "));

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }
    memset(&Enc_p->EncoderControlBlock.WmaeStatus,0,sizeof(MME_WmaeStatus_t));
        memset(&Enc_p->EncoderControlBlock.WmaeAsfHeaderBlock,0,sizeof(MemoryBlock_t));
    Enc_p->EncoderControlBlock.WmaeFirstOpBlkReceived = FALSE;
    Enc_p->EncoderControlBlock.WmaeLastOpBlkDelivered = FALSE;
    Enc_p->EncoderControlBlock.WmaeEncodedDataSize = 0;

    /* Send the Stop command only if we had started the encoder. */
    Error = STAud_EncSendCommand(MME_ENCODER_STATE_STOP, &Enc_p->EncoderControlBlock);

    if(Error == ST_NO_ERROR)
    {
        /* Wait on sema for START completion */
        STOS_SemaphoreWait(Enc_p->EncoderControlBlock.CmdCompletedSem_p);
    }


    STTBX_Print(("STAud_EncStop: Successful\n "));
    return Error;
}

/******************************************************************************
 *  Function name       : STAud_EncoderGetCapability
 *  Arguments           :
 *       IN                     :       Enc Handle
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Returns the encoder capabilities
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncoderGetCapability(STAud_EncHandle_t Handle,
                                           STAUD_ENCCapability_t * Capability_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Set encoder Capability parameter */
    *Capability_p = Enc_p->EncoderControlBlock.EncCapability;

    return Error;

}


ST_ErrorCode_t  STAud_EncoderGetOutputParams(STAud_EncHandle_t Handle,
                                             STAUD_ENCOutputParams_s * EncoderOPParams_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Set encoder output params*/
    *EncoderOPParams_p = Enc_p->EncoderControlBlock.OutputParams;

    return Error;

}

ST_ErrorCode_t  STAud_EncoderSetOutputParams(STAud_EncHandle_t Handle,
                                             STAUD_ENCOutputParams_s EncoderOPParams)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    EncoderControlBlockList_t *Enc_p= (EncoderControlBlockList_t*)NULL;

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        /* handle not found */
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Get encoder output parmaeters from application*/
    Enc_p->EncoderControlBlock.OutputParams = EncoderOPParams;

    return Error;

}

/******************************************************************************
*                                               ENCODER TASK                                                                               *
******************************************************************************/
/******************************************************************************
 *  Function name       : STAud_EncTask_Entry_Point
 *  Arguments           :
 *       IN                     :
 *       OUT                    : void
 *       INOUT          :
 *  Return              : void
 *  Description         : This function is the entry point for encoder task. Ihe encoder task will be in
 *                                 different states depending of the command received from upper/lower layers
 ***************************************************************************** */
void    STAud_EncTask_Entry_Point(void* params)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    MME_ERROR       Status;

    /* Get the Encoder control Block for this task */
    EncoderControlBlock_t   *Control_p = (EncoderControlBlock_t*)params;

    STOS_TaskEnter(params);
    while(1)
    {

        /* Wait on sema for start command */
        STOS_SemaphoreWait(Control_p->EncoderTaskSem_p);

        /* Check if some command is recevied. We check this by looking for next desired state of encoder */
        if(Control_p->CurEncoderState != Control_p->NextEncoderState)
        {
            /* Encoder state need to be changed */
            STOS_MutexLock(Control_p->LockControlBlockMutex_p);
            Control_p->CurEncoderState = Control_p->NextEncoderState;
            STOS_MutexRelease(Control_p->LockControlBlockMutex_p);
        }

        switch(Control_p->CurEncoderState)
        {

        case MME_ENCODER_STATE_IDLE:
            break;

        case MME_ENCODER_STATE_START:

            Status = STAud_EncSetAndInitEncoder(Control_p);
            if(Status != MME_SUCCESS)
            {
                STTBX_Print(("Audio Deocder:Init failure\n"));
            }
            else
            {
                STTBX_Print(("Audio Deocder:Init Success\n"));
            }


            Status = ResetEncoderQueueParams(Control_p);
            /* Encoder Initialized. Now Send Play Command to Encoder Task i.e. this task itself */
            STAud_EncSendCommand(MME_ENCODER_STATE_ENCODING, Control_p);

            break;

        case MME_ENCODER_STATE_ENCODING:

            /* Process incoming Data */
            Error = STAud_EncProcessInput(Control_p);
            if(Error == ST_NO_ERROR)
            {
                /* Process outgoing Data */
                /* Currently being done in Encoder callback but shall be moved here in future */
                //STAud_EncProcessOutput();

            }
            //task_delay(65000);

            break;

        case MME_ENCODER_STATE_STOP:
            Error = STAud_EncStopEncoder(Control_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_EncStopEncoder::Failed %x \n",Error));
            }
            /* Encoder Stoped. Now Send Stopped Command to Encoder Task i.e. this task itself */
            STAud_EncSendCommand(MME_ENCODER_STATE_STOPPED, Control_p);

            break;

        case MME_ENCODER_STATE_STOPPED:
            /* Nothing to do in stopped state */
            break;

        case MME_ENCODER_STATE_TERMINATE:
            STOS_TaskExit(params);
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
*                                               LOCAL FUNCTIONS                                                                            *
******************************************************************************/
ST_ErrorCode_t  ResetEncoderQueueParams(EncoderControlBlock_t   *Control_p)
{
    U32 i;

    // Reset Input Queue params
    for(i = 0; i < STREAMING_ENCODER_QUEUE_SIZE; i++)
    {
        ResetInputQueueElement(Control_p->EncoderInputQueue[i]);
    }

    // Reset Output Queue params
    for(i = 0; i < STREAMING_ENCODER_QUEUE_SIZE; i++)
    {
        ResetOutputQueueElement(Control_p->EncoderOutputQueue[i]);
    }

    // Reset number of input/output samples
    Control_p->PendingInputSamples = 0;
    Control_p->PendingOutputSamples= 0;

    return ST_NO_ERROR;
}


/******************************************************************************
 *  Function name       : STAud_EncProcessInput
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Get the comressed input buffers, get empty PCM buffer,
                          Update MME params and send command to MME encoder
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncProcessInput(EncoderControlBlock_t     *Control_p)
{

    ST_ErrorCode_t          status = ST_NO_ERROR;
    U32                                     QueueTailIndex;
    MME_ERROR               Status;
    BOOL                            DataAvailableAtInput = TRUE;


    while (DataAvailableAtInput)
    {
        switch(Control_p->EncInState)
        {
        case MME_ENCODER_INPUT_STATE_IDLE:
            /* Intended fall through */
            Control_p->EncInState = MME_ENCODER_INPUT_STATE_GET_INPUT_DATA;


        case MME_ENCODER_INPUT_STATE_GET_INPUT_DATA:
            {
                BOOL SendMMEComamand = TRUE;

                // Geting Input for streaming transformer
                while (SendMMEComamand)
                {
                    SendMMEComamand = FALSE;

                    if (((Control_p->EncoderInputQueueTail - Control_p->EncoderInputQueueFront) < STREAMING_ENCODER_QUEUE_SIZE)
                        || (Control_p->Transformer_Init == FALSE)
                            /*&& ((Control_p->Transformer_Init == FALSE) ||
                                    ((Control_p->EncoderOutputQueueTail - Control_p->EncoderOutputQueueFront) > 4*(Control_p->EncoderInputQueueTail - Control_p->EncoderInputQueueFront + 1))
                               )*/
                       )

                    {
                        // We have space to send another command
                        QueueTailIndex = Control_p->EncoderInputQueueTail%STREAMING_ENCODER_QUEUE_SIZE;
                        Control_p->CurrentInputBufferToSend =
                            Control_p->EncoderInputQueue[QueueTailIndex];

                        while ((Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages != Control_p->NumInputScatterPagesPerTransform)
                                && (MemPool_GetIpBlk(Control_p->InMemoryBlockManagerHandle, (U32 *)&Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages],Control_p->Handle) == ST_NO_ERROR))
                        {
                            /* This functions extract metadeta from memory block*/
                            EncoderParseInputBufferParams(Control_p, Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages]);
                            if (Control_p->Transformer_Init == FALSE)
                            {
                                /* First i/p buffer after start/restart*/
                                Status = STAud_EncInitTransformer(Control_p);
                                if(Status != MME_SUCCESS)
                                {
                                    STTBX_Print(("Audio Deocder:Init failure\n"));
                                }
                                else
                                {
                                    STTBX_Print(("Audio Deocder:Init Success\n"));
                                }

                            }
                            //memset ((U8 *)Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages]->MemoryStart, 0xAB, (1536 * 8));
                            Control_p->CurrentInputBufferToSend->EncoderBufIn.ScatterPageIn[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages].Page_p = (void*)Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages]->MemoryStart;
                            //Added for testing
                            //Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages]->Size = 1536 * 4 * 2;
                            Control_p->CurrentInputBufferToSend->EncoderBufIn.ScatterPageIn[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages].Size = Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages]->Size;
                            Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.TotalSize += Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages]->Size;
                            Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages++;

                        }

                        if (Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn_p->NumberOfScatterPages
                            == Control_p->NumInputScatterPagesPerTransform)
                        {
                            if (CheckEncoderUnderflow(Control_p) == FALSE)
                            {

                                U32 numSample = 0;

                                // We have enough input buffers to send one transform command
                                //TBD MME_TRANSFORM_COMMAND
                                // Fill in number of samples required for output to avoid SFC
                                numSample = (Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.TotalSize/((Control_p->InputParams.WordSize/8) * (Control_p->InputParams.NumChannels)));
                                Control_p->CurrentInputBufferToSend->EncoderTransformParam.numberOutputSamples = numSample;
                                UpdatePendingSamples(&(Control_p->PendingInputSamples), numSample);

                                //Hardcode Last block

                                if ((Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_WMA)
                                     && (Control_p->CurrentInputBufferToSend->InputBlockFromProducer[Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.NumberOfScatterPages - 1]->Flags & EOF_VALID)
                                    )
                                {
                                    STTBX_Print(("STAud_EncProcessInput::Sending LAST WMA BLOCK to Encoder\n"));
                                    Control_p->CurrentInputBufferToSend->EncoderTransformParam.EndOfFile = 1;

                                }


                                status = MME_SendCommand(Control_p->TranformHandleEncoder, &Control_p->CurrentInputBufferToSend->Cmd);
                                Control_p->CurrentInputBufferToSend = NULL;

                                /* Update encoder tail */
                                Control_p->EncoderInputQueueTail++;

                                if (status == MME_SUCCESS)
                                {
                                        //STTBX_Print(("Transform command %d\n",Control_p->EncoderInputQueueTail));
                                }
                                else
                                {
                                        STTBX_Print(("Transform command failed %d\n",Control_p->EncoderInputQueueTail));
                                }

                                SendMMEComamand = TRUE;
                            }
                        }
                        else
                        {
                                DataAvailableAtInput = FALSE;
                        }
                    }

/*                  if ((Control_p->CurrentInputBufferToSend != NULL) &&
                        (Control_p->EncoderOutputQueueTail - Control_p->EncoderOutputQueueFront) < 4*(Control_p->EncoderInputQueueTail - Control_p->EncoderInputQueueFront + 1))
                    {
                            // We are here because probably we didn't fetched all input buffers
                            // because 4*no. input buffers were unavilable
                            DataAvailableAtInput = TRUE;
                    }
                    else
                    {
                            DataAvailableAtInput = FALSE;

                    }
*/
                            }
                    }

                    /* Intended fall through */
                    Control_p->EncInState = MME_ENCODER_INPUT_STATE_GET_OUTPUT_BUFFER;//MME_ENCODER_INPUT_STATE_GET_OUTPUT_BUFFER;

        case MME_ENCODER_INPUT_STATE_GET_OUTPUT_BUFFER:
            if ((Control_p->EncodedStreamContent != STAUD_STREAM_CONTENT_WMA) ||
                !(Control_p->WmaeAsfHeaderReceived))
            {

                /* Start sending output buffer only after transformer has initialized*/
                if (Control_p->Transformer_Init)
                {
                    BOOL SendMMEComamand = TRUE;

                    while (SendMMEComamand)
                    {
                        SendMMEComamand = FALSE;

                        if ((Control_p->EncoderOutputQueueTail - Control_p->EncoderOutputQueueFront < STREAMING_ENCODER_QUEUE_SIZE) /*&&
                                (Control_p->EncoderOutputQueueTail == Control_p->EncoderOutputQueueFront)*/
                           )

                        {
                            // We have space to send another command
                            QueueTailIndex = Control_p->EncoderOutputQueueTail%STREAMING_ENCODER_QUEUE_SIZE;
                            Control_p->CurrentOutputBufferToSend = Control_p->EncoderOutputQueue[QueueTailIndex];

                            while ((Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages != Control_p->NumOutputScatterPagesPerTransform) &&
                                   (MemPool_GetOpBlk(Control_p->OutMemoryBlockManagerHandle, (U32 *)&Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages]) == ST_NO_ERROR))
                            {
                                Control_p->CurrentOutputBufferToSend->EncoderBufOut.ScatterPageOut[Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages].Page_p = (void*)Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages]->MemoryStart;
                                // Rahul hardcoded this to get a output buffer from a input buffer
                                Control_p->CurrentOutputBufferToSend->EncoderBufOut.ScatterPageOut[Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages].Size = Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages]->MaxSize;//Size;512;//
                                Control_p->CurrentOutputBufferToSend->EncoderBufOut.ScatterPageOut[Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages].BytesUsed = 0;
                                Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.TotalSize += Control_p->CurrentOutputBufferToSend->OutputBlockFromConsumer[Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages]->MaxSize;//Size;512;//
                                Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages++;
                            }

                            if (Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut.NumberOfScatterPages == Control_p->NumOutputScatterPagesPerTransform)
                            {

                                //U32 i = 0;
                                // We have enough output buffers to send one command
                                //TBD MME_SEND_BUFFER_COMMAND
                                /*Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut_p->TotalSize = 0;
                                for (i = 0; i < Control_p->NumInputScatterPagesPerTransform; i++)
                                {
                                        Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut_p->TotalSize += Control_p->CurrentOutputBufferToSend->EncoderBufOut.BufferOut_p->ScatterPages_p[i].Size;
                                }*/
                                UpdatePendingSamples(&(Control_p->PendingOutputSamples), Control_p->SamplesPerOutputFrame);

                                /* update command params */
                                /*status = acc_setAudioEncoderTransformCmd(&Control_p->CurrentOutputBufferToSend->Cmd,
                                                                                                                        &Control_p->CurrentOutputBufferToSend->FrameParams,
                                                                                                                        sizeof (MME_LxAudioEncoderFrameParams_t),
                                                                                                                        &Control_p->CurrentOutputBufferToSend->FrameStatus,
                                                                                                                        sizeof (MME_LxAudioEncoderFrameStatus_t),
                                                                                                                        &(Control_p->CurrentOutputBufferToSend->EncoderBufOut0.BufferOut_p),
                                                                                                                        ACC_CMD_PLAY, 0, FALSE,
                                                                                                                        0, 1);*/


                                status = MME_SendCommand(Control_p->TranformHandleEncoder,
                                                        &Control_p->CurrentOutputBufferToSend->Cmd);
                                Control_p->CurrentOutputBufferToSend = NULL;
                                /* Update encoder tail */
                                Control_p->EncoderOutputQueueTail++;

                                if (status == MME_SUCCESS)
                                {
                                    //STTBX_Print(("Send Buffer command %d\n",Control_p->EncoderOutputQueueTail));
                                }
                                else
                                {
                                    STTBX_Print(("Send Buffer command failed %d\n",Control_p->EncoderOutputQueueTail));
                                }

                                SendMMEComamand = TRUE;
                            }
                        }
                    }
                }
            }

//         if ((DataAvailableAtInput == TRUE) &&
//            ((Control_p->EncoderOutputQueueTail - Control_p->EncoderOutputQueueFront) < 4*(Control_p->EncoderInputQueueTail - Control_p->EncoderInputQueueFront + 1))
//            )
            if ((DataAvailableAtInput == TRUE) && (CheckEncoderUnderflow(Control_p) == FALSE))
            {
                // if we input data and we are not in underflow then go and fetch more data
                DataAvailableAtInput = TRUE;
            }
            else
            {
                // There is no point in going back to get more input data
                // if we dont have sufficient output buffers even till now
                // or data was there at all
                DataAvailableAtInput = FALSE;

            }

            Control_p->EncInState = MME_ENCODER_INPUT_STATE_IDLE;//MME_ENCODER_INPUT_STATE_IDLE;//MME_ENCODER_INPUT_STATE_UPDATE_PARAMS_SEND_TO_MME;

            break;

        default:
            break;

        }
    }
    return status;
}

/******************************************************************************
 *  Function name       : STAud_EncProcessOutput
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Send the encoded data to next unit
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncProcessOutput(EncoderControlBlock_t    *Control_p, MME_Command_t            *command_p)
{

    ST_ErrorCode_t          status = ST_NO_ERROR;
    EncoderOutputCmdParam_t *CurrentBufferToDeliver;
    U32 i = 0;

    partition_t *DriverPartition = NULL;
    DriverPartition = (partition_t *)Control_p->InitParams.DriverPartition;

    /* Check if there is data to be sent to output */
    if(Control_p->EncoderOutputQueueFront != Control_p->EncoderOutputQueueTail)
    {
        CurrentBufferToDeliver = Control_p->EncoderOutputQueue[Control_p->EncoderOutputQueueFront % STREAMING_ENCODER_QUEUE_SIZE];

        /* Copy the PTS etc values from input to output block */

        /* TBD Copy other needed information from Input to output ...Flags and frequency need to be updated*/
        /*STTBX_Print((" Enc:: Flags = %d , PTS = %d \n",Control_p->CurrentBufferToOut->OutputBlockFromConsumer->Flags,
                                                                                                                                        Control_p->CurrentBufferToOut->OutputBlockFromConsumer->PTS ));*/

        /* Set the encoded buffer size returned from Encoder */
        for (i=0; i < Control_p->NumOutputScatterPagesPerTransform; i++)
        {
            // TBD for streaming buffer we have to pick data from DataBuffer[0] as this is were we will put output buffer
            // Instead of picking from Cmd buffers pick scatterpages from encoder queue
            if (command_p->CmdStatus.State == MME_COMMAND_FAILED)//(command_p != NULL)
            {
                    CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size = Control_p->DefaultEncodedSize;
            }
            else
            {
                    CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size = CurrentBufferToDeliver->EncoderBufOut.BufferOut.ScatterPages_p[i].BytesUsed;
            }

            // For Stream based encoders all the information should be extracted from frame status
            CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags = DATAFORMAT_VALID;
            CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[DATAFORMAT_OFFSET] = Control_p->EncodedDataAlignment;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
            if (ACC_isPTS_PRESENT(CurrentBufferToDeliver->EncoderStatusBuffer.PtsInfo.PtsFlags))
            {
                STCLKRV_ExtendedSTC_t tempPTS;
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= PTS_VALID;
                tempPTS.BaseBit32 = ACC_PTSflag_EXTRACT_B32(CurrentBufferToDeliver->EncoderStatusBuffer.PtsInfo.PtsFlags);
                tempPTS.BaseValue = CurrentBufferToDeliver->EncoderStatusBuffer.PtsInfo.PTS;
                ADDToEPTS(&tempPTS, MILLISECOND_TO_PTS(ENC_LATENCY_OFFSET));
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[PTS_OFFSET] = tempPTS.BaseValue;
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[PTS33_OFFSET] = tempPTS.BaseBit32;
            }
#endif

            if (Control_p->EncodedOutputFrequency != Control_p->OutputParams.OutputFrequency)
            {
                Control_p->EncodedOutputFrequency = Control_p->OutputParams.OutputFrequency;
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= FREQ_VALID;
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Data[FREQ_OFFSET] = Control_p->EncodedOutputFrequency;
            }


            if (Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_DTS)
            {
#ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size = 2048;
#else
                CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size = 2032;
#endif
            }

            //Check for WMA_EOF received

            if (Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_WMA)
            {
                STTBX_Print(("STAud_EncProcessOutput::LAST WMA BLOCK CALL BACK RECEIVED\n"));

                if (!Control_p->WmaeFirstOpBlkReceived)
                {
                    U32 * TempWmaAsfMemoryStart;
                    Control_p->WmaeFirstOpBlkReceived = TRUE;
                    memcpy(&Control_p->WmaeAsfHeaderBlock,CurrentBufferToDeliver->OutputBlockFromConsumer[i],sizeof(*CurrentBufferToDeliver->OutputBlockFromConsumer[i]));
                    TempWmaAsfMemoryStart = (U32 *)memory_allocate(DriverPartition,CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size);
                    Control_p->WmaeAsfHeaderBlock.MemoryStart = (U32)TempWmaAsfMemoryStart;
                    memcpy((void*)Control_p->WmaeAsfHeaderBlock.MemoryStart,(void*)CurrentBufferToDeliver->OutputBlockFromConsumer[i]->MemoryStart,CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size);

                }

                if (!Control_p->WmaeLastOpBlkDelivered && (command_p->CmdStatus.Error != MME_COMMAND_ABORTED))
                {
                    Control_p->WmaeEncodedDataSize += CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size;
                }

                if (CurrentBufferToDeliver->EncoderStatusBuffer.LastBuffer == 1)
                {
                    Control_p->WmaeLastOpBlkDelivered = TRUE;
                }
                //If the callback are for abort command issued for the output buffers
                //once last i/p block callback is received
                if (command_p->CmdStatus.Error == MME_COMMAND_ABORTED)
                {
                    CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Flags |= DUMMYBLOCK_VALID;
                    CurrentBufferToDeliver->OutputBlockFromConsumer[i]->Size = 0;
                }

            }
            status = MemPool_PutOpBlk(Control_p->OutMemoryBlockManagerHandle ,
                                      (U32 *)&(CurrentBufferToDeliver->OutputBlockFromConsumer[i]));


            if(status != ST_NO_ERROR)
            {
                    STTBX_Print(("STAud_EncProcessOutput::MemPool_PutOpBlk Failed\n"));
            }


        }

                // As all the scatter pages have been released set it back to 0
                CurrentBufferToDeliver->EncoderBufOut.BufferOut.NumberOfScatterPages = 0;
                CurrentBufferToDeliver->EncoderBufOut.BufferOut.TotalSize = 0;

                // Decrease the number of pending output samples
                UpdatePendingSamples(&(Control_p->PendingOutputSamples), ((-1)* Control_p->SamplesPerOutputFrame));

                /* update queue front */
                Control_p->EncoderOutputQueueFront++;

                //STTBX_Print(("Send buffer command completed %d\n",Control_p->EncoderOutputQueueFront));
    }


    if (Control_p->EncoderOutputQueueFront == Control_p->EncoderOutputQueueTail)
    {
        if ((Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_WMA) && Control_p->WmaeAsfHeaderReceived)
        {
            //wait for all the output buffers to be released before sending an output buffer
            //with the corrected ASF header as the last buffer
            MemoryBlock_t *memBlkToDeliver;

            U32 memoryStartAdd;

            /*
            while(Control_p->EncoderOutputQueueFront != Control_p->EncoderOutputQueueTail)
            {
                    AUD_TaskDelayMs(3);
                    STTBX_Print(("Waiting for abort on output"));
            }
            */


            //deliver the ASF Header to the next unit after correction
            //delivering first to second last o/p block


            while (MemPool_GetOpBlk(Control_p->OutMemoryBlockManagerHandle,(U32 *)&memBlkToDeliver) != ST_NO_ERROR)
            {
                    STTBX_Print(("WmaeCorrectAsfHeader::MemPool_GetOpBlk Failed\n"));
                    AUD_TaskDelayMs(3);
            }

            memoryStartAdd = memBlkToDeliver->MemoryStart;

            memcpy(memBlkToDeliver, &Control_p->WmaeAsfHeaderBlock,sizeof(Control_p->WmaeAsfHeaderBlock));
            memBlkToDeliver->MemoryStart = memoryStartAdd;
            memcpy((void*)memBlkToDeliver->MemoryStart,(void*)Control_p->WmaeAsfHeaderBlock.MemoryStart,Control_p->WmaeAsfHeaderBlock.Size);

            memory_deallocate(DriverPartition, (void *)Control_p->WmaeAsfHeaderBlock.MemoryStart);

            memBlkToDeliver->Flags |= WMAE_ASF_HEADER_VALID;

            status = MemPool_PutOpBlk(Control_p->OutMemoryBlockManagerHandle ,
                                    (U32 *)&(memBlkToDeliver));


            if(status != ST_NO_ERROR)
            {
                    STTBX_Print(("WmaeCorrectAsfHeader::MemPool_PutOpBlk Failed\n"));
            }

        }

    }

    return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name       : STAud_EncReleaseInput
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Relase the compressed data to next unit
 ***************************************************************************** */
ST_ErrorCode_t  STAud_EncReleaseInput(EncoderControlBlock_t     *Control_p, MME_Command_t            *command_p)
{

    ST_ErrorCode_t          status = ST_NO_ERROR;
    EncoderInputCmdParam_t  *CurrentInputBufferToRelease;
    U32 i;
    MME_ERROR       Error = MME_SUCCESS;

    /* Check if there is data to be sent to output */
    if(Control_p->EncoderInputQueueFront != Control_p->EncoderInputQueueTail)
    {
        CurrentInputBufferToRelease = Control_p->EncoderInputQueue[Control_p->EncoderInputQueueFront % STREAMING_ENCODER_QUEUE_SIZE];


        //WMA_EOF received for callback
        if (Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_WMA)
        {
            //check if last i/p buffer callback received
            if (CurrentInputBufferToRelease->EncoderTransformParam.EndOfFile == 1)
            {
                //int count = 0; //for bounded wait
                MME_WmaeStatus_t *wmae_status = (MME_WmaeStatus_t *)CurrentInputBufferToRelease->EncoderFrameStatus.SpecificEncoderStatusBArray;

                STTBX_Print(("STAud_EncReleaseInput::Callback for the last input buffer sent received\n"));

                Control_p->WmaeStatus = *wmae_status;
                Control_p->WmaeAsfHeaderReceived = TRUE;


                //if ASF Header is received do the correction in first saved o/p block

                status = WmaeCorrectAsfHeader(Control_p);

                //send abort for output queue elements

                for (i = Control_p->EncoderOutputQueueFront; i < Control_p->EncoderOutputQueueTail; i++)
                {
                    EncoderOutputCmdParam_t *CurrentOutputBufferToAbort = (Control_p->EncoderOutputQueue[i %STREAMING_ENCODER_QUEUE_SIZE]);
                    Error = MME_AbortCommand(Control_p->TranformHandleEncoder,  CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId);
                    if (MME_SUCCESS != Error)
                    {
                        STTBX_Print(("Error in Send buffer command cmd ID 0x%x\n", CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId));
                    }
                    else
                    {
                        STTBX_Print(("Sending abort for Send buffer command cmd ID 0x%x\n", CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId));
                    }
                }

            }

        }

        /* Copy the PTS etc values from input to output block */

        /* Copy other needed information from Input to output ...Flags and frequency need to be updated*/
        /*STTBX_Print((" Enc:: Flags = %d , PTS = %d \n",Control_p->CurrentBufferToOut->OutputBlockFromConsumer->Flags,
                                                                                                                                        Control_p->CurrentBufferToOut->OutputBlockFromConsumer->PTS ));*/
        /* Set the encoded buffer size returned from Encoder */
        STTBX_Print(("ENCODER:Input Rel:%d, Op del:,%d,Transf:%d,SYS:%d,\n",Control_p->EncoderInputQueueFront,
                                                                                                Control_p->EncoderOutputQueueFront,
                                                                                                CurrentInputBufferToRelease->EncoderFrameStatus.ElapsedTime[2],
                                                                                                time_now()));
//              STTBX_Print(("ENCODER:, PrePro:,%d, Encode:,%d, Transf:,%d, RdBuff:,%d, WrBuff:,%d, WtBuff:,%d\n",
//                                                                              CurrentInputBufferToRelease->EncoderFrameStatus.ElapsedTime[0],
//                                                                              CurrentInputBufferToRelease->EncoderFrameStatus.ElapsedTime[1],
//                                                                              CurrentInputBufferToRelease->EncoderFrameStatus.ElapsedTime[2],
//                                                                              CurrentInputBufferToRelease->EncoderFrameStatus.ElapsedTime[3],
//                                                                              CurrentInputBufferToRelease->EncoderFrameStatus.ElapsedTime[4],
//                                                                              CurrentInputBufferToRelease->EncoderFrameStatus.ElapsedTime[5]));
        for (i=0; i < Control_p->NumInputScatterPagesPerTransform; i++)
        {

            status = MemPool_FreeIpBlk(Control_p->InMemoryBlockManagerHandle,
                                            (U32 *)&(CurrentInputBufferToRelease->InputBlockFromProducer[i]),
                                            Control_p->Handle);
            if(status != ST_NO_ERROR)
            {
                STTBX_Print(("STAud_EncProcessOutput::MemPool_FreeIpBlk Failed\n"));
            }
            else
            {
                //STTBX_Print(("Released input buffer %d\n",Control_p->EncoderInputQueueFront));
            }

        }

        // As all the scatter pages have been released set it back to 0
        CurrentInputBufferToRelease->EncoderBufIn.BufferIn.NumberOfScatterPages = 0;
        CurrentInputBufferToRelease->EncoderBufIn.BufferIn.TotalSize = 0;

        // Decrease the number of pending input samples
        UpdatePendingSamples(&(Control_p->PendingInputSamples), (-1) * (CurrentInputBufferToRelease->EncoderTransformParam.numberOutputSamples));

        /* update queue front */
        Control_p->EncoderInputQueueFront++;
    }
    //STTBX_Print(("Transform command completed %d\n",Control_p->EncoderInputQueueFront));

    return ST_NO_ERROR;
}
/******************************************************************************
 *  Function name       : STAud_EncSetAndInitEncoder
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : MME_ERROR
 *  Description         :
 ***************************************************************************** */
MME_ERROR       STAud_EncSetAndInitEncoder(EncoderControlBlock_t        *Control_p)
{
    MME_ERROR   Error = MME_SUCCESS;

    /* Init callback data as Handle. It will help us find the encoder from list of encoders in multi instance mode */
    Control_p->MMEinitParams.CallbackUserData = (void *)Control_p->Handle;

    /*Transformer restart*/
    Control_p->TransformerRestart = TRUE;

    /* Register to memory block manager as a consumer for input */
    Error |= MemPool_RegConsumer(Control_p->InMemoryBlockManagerHandle,Control_p->consumerParam_s);

    /* Register to memory block manager as a Producer */
    Error |= MemPool_RegProducer(Control_p->OutMemoryBlockManagerHandle, Control_p->ProducerParam_s);


    /* Default values. These shall be overwritten by individual encoder type */
    Control_p->DefaultEncodedSize = 1536 * 4 * Control_p->InputParams.NumChannels;
    Control_p->DefaultCompressedFrameSize = 1536;                                        // Used for transcoded output



    // Number of scatter pages per transform should be set to 1 for a frame based Mode (not for encoder)
    Control_p->NumInputScatterPagesPerTransform = NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM;
    Control_p->NumOutputScatterPagesPerTransform = NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM;


    /* Signal the Command Completion Semaphore */
    STOS_SemaphoreSignal(Control_p->CmdCompletedSem_p);

    return Error;

}

MME_ERROR       STAud_EncInitTransformer(EncoderControlBlock_t  *Control_p)
{
    MME_ERROR   Error = MME_SUCCESS;
    BOOL EnableByteSwap = FALSE;
    char      TransformerName[64];
    U32 j=0;
    Control_p->EncID = ACC_DDCE_ID;
    memset(TransformerName,0,64);

    if((Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_MPEG1) ||
            (Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_MPEG2))
    {
        Control_p->EncID = ACC_MP2E_ID;

        /* Set the default encoded buffer size for this stream */
        Control_p->DefaultEncodedSize = 1152 * 4 * Control_p->InputParams.NumChannels;
        Control_p->DefaultCompressedFrameSize = 1152;
        //Control_p->EncodedDataAlignment = BE;
        Control_p->EncodedDataAlignment = LE;
        Control_p->SamplesPerOutputFrame = 1152;
        EnableByteSwap = TRUE;
    }

    if (Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_MP3)
    {
        Control_p->EncID = ACC_MP3E_ID;

        /* Set the default encoded buffer size for this stream */
        Control_p->DefaultEncodedSize = 1152 * 4 * Control_p->InputParams.NumChannels;
        Control_p->DefaultCompressedFrameSize = 1152;
        //Control_p->EncodedDataAlignment = BE;
        Control_p->EncodedDataAlignment = LE;
        Control_p->SamplesPerOutputFrame = 1152;
        EnableByteSwap = TRUE;
    }

    if(Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_MPEG_AAC)
    {

        Control_p->EncID = ACC_AACE_ID;

        /* Set the default encoded buffer size for this stream */
        Control_p->DefaultEncodedSize = 1024 * 4 * Control_p->InputParams.NumChannels;
        Control_p->DefaultCompressedFrameSize = 1024;
        Control_p->EncodedDataAlignment = BE;
        //Control_p->EncodedDataAlignment = LE;
        Control_p->SamplesPerOutputFrame = 1024;
        EnableByteSwap = FALSE;
    }

    if(Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_AC3)
    {

        if (Control_p->InputParams.NumChannels == 10)
        {
            Control_p->EncID = ACC_DDCE51_ID;
            Control_p->EncodedDataAlignment = LE;
        }
        else
        {
            Control_p->EncID = ACC_DDCE_ID;
            //Control_p->EncodedDataAlignment = BE;
            Control_p->EncodedDataAlignment = LE;
            EnableByteSwap = TRUE;
        }

        /* Set the default encoded buffer size for this stream */
        Control_p->DefaultEncodedSize = 1536 * 4 * Control_p->InputParams.NumChannels;
        Control_p->DefaultCompressedFrameSize = 1536;
        Control_p->SamplesPerOutputFrame = 1536;

    }

    if(Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_WMA)
    {
        Control_p->EncID = ACC_WMAE_ID;
        /* Set the default encoded buffer size for this stream */
        Control_p->DefaultEncodedSize = 600 * 4 * Control_p->InputParams.NumChannels;
        Control_p->DefaultCompressedFrameSize = 600;
        Control_p->EncodedDataAlignment = BE;
        Control_p->SamplesPerOutputFrame = 1024;

    }

    if(Control_p->EncodedStreamContent == STAUD_STREAM_CONTENT_DTS)
    {

        Control_p->EncID = ACC_DTSE_ID;

        /* Set the default encoded buffer size for this stream */
        Control_p->DefaultEncodedSize = 512 * 4 * Control_p->InputParams.NumChannels;
        Control_p->DefaultCompressedFrameSize = 512;
        Control_p->EncodedDataAlignment = LE;//BE;
        Control_p->SamplesPerOutputFrame = 512;

    }

/* Set encoder output params based on user config*/

    if (Control_p->OutputParams.AutoOutputParam == TRUE)
    {
        // Set output frequency equal to input frequency
        //Control_p->OutputParams.OutputFrequency   = CalculateEncOutputFrequency(Control_p);
        CalculateEncOutputFrequency(Control_p);
        Control_p->OutputParams.BitRate           = 128;
    }
    else
    {
        if (Control_p->InputParams.InputFrequency != Control_p->OutputParams.OutputFrequency)
        {
            Control_p->SFCApply = ACC_MME_ENABLED;
        }
        else
        {
            Control_p->SFCApply = ACC_MME_DISABLED;
        }

    }

    Control_p->EncodedOutputFrequency = 0xFFFFFFFF;

    /* init encoder params */
    EncInitTransformerInitParams(Control_p, Control_p->EncID, EnableByteSwap);

    if(Error == MME_SUCCESS)
    {
        /* We need to try on all LX */
        for(j = 0; j < TOTAL_LX_PROCESSORS; j++)
        {
            /* Generate the LX transfommer name(based on ST231 number */
            GetLxName(j,"MME_TRANSFORMER_TYPE_AUDIO_ENCODER",TransformerName, sizeof(TransformerName));
            Error = MME_InitTransformer(TransformerName, &Control_p->MMEinitParams, &Control_p->TranformHandleEncoder);
            if(Error == MME_SUCCESS)
            {
                /* transformer initialized */
                Control_p->Transformer_Init = TRUE;
                break;
            }
            else
            {
                Control_p->Transformer_Init = FALSE;
            }
        }
    }

    return Error;
}
/******************************************************************************
 *  Function name       : STAud_EncStopEncoder
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : MME_ERROR
 *  Description         :
 ***************************************************************************** */
MME_ERROR       STAud_EncStopEncoder(EncoderControlBlock_t      *Control_p)
{
    MME_ERROR       Error = MME_SUCCESS;
    U32 i, tempInputQueueFront, abortCmdCompleteWait = 0;
    EncoderInputCmdParam_t  *CurrentInputBufferToAbort;
    EncoderOutputCmdParam_t *CurrentOutputBufferToAbort;

    STTBX_Print(("STAud_EncStopEncoder: Called !!!\n "));

    /*Transformer restart*/

    // Add a delay of 30 ms to make sure that the buffers are returned before
    // we start sending aborts..
    AUD_TaskDelayMs(300);

    // Rahul TBD to abort all the commands
    if(Control_p->Transformer_Init)
    {

        Control_p->TransformerRestart = TRUE;
        tempInputQueueFront = Control_p->EncoderInputQueueFront;
        //for (i = Control_p->EncoderInputQueueFront; i < Control_p->EncoderInputQueueTail; i++)
        for (i = (Control_p->EncoderInputQueueTail - 1); ((i >= tempInputQueueFront) && (Control_p->EncoderInputQueueTail != Control_p->EncoderInputQueueFront)); i--)
        {
            CurrentInputBufferToAbort = (Control_p->EncoderInputQueue[i %STREAMING_ENCODER_QUEUE_SIZE]);
            Error = MME_AbortCommand(Control_p->TranformHandleEncoder,  CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId);
            if (MME_SUCCESS != Error)
            {
                STTBX_Print(("Error in Transform command cmd ID 0x%x\n", CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId));

            }
            else
            {
                STTBX_Print(("Sending abort for Transform command cmd ID 0x%x\n", CurrentInputBufferToAbort->Cmd.CmdStatus.CmdId));
            }

        }

        for (i = Control_p->EncoderOutputQueueFront; i < Control_p->EncoderOutputQueueTail; i++)
        {
            CurrentOutputBufferToAbort = (Control_p->EncoderOutputQueue[i %STREAMING_ENCODER_QUEUE_SIZE]);
            Error = MME_AbortCommand(Control_p->TranformHandleEncoder,  CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId);
            if (MME_SUCCESS != Error)
            {
                STTBX_Print(("Error in Send buffer command cmd ID 0x%x\n", CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId));
            }
            else
            {
                STTBX_Print(("Sending abort for Send buffer command cmd ID 0x%x\n", CurrentOutputBufferToAbort->Cmd.CmdStatus.CmdId));
            }
        }
    }

    /* We should wait for both input queue and output queue to be freed before terminating the transformer*/

    while(((Control_p->EncoderOutputQueueFront != Control_p->EncoderOutputQueueTail) ||
              (Control_p->EncoderInputQueueFront != Control_p->EncoderInputQueueTail)) &&
              (abortCmdCompleteWait <= 300))
    {
        AUD_TaskDelayMs(10);
        abortCmdCompleteWait++;
        STTBX_Print((" L "));
    }

    /* Terminate the Transformer */
    if(Control_p->Transformer_Init)
    {
        Error = MME_TermTransformer(Control_p->TranformHandleEncoder);
    }

    if(Error != MME_SUCCESS)
    {
        STTBX_Print(("ERROR!!!MME_TermTransformer::FAILED %d\n", Error));
        //printf("ERROR!!!MME_TermTransformer::FAILED %d\n", Error);
    }

    /* Reset the encoder init */
    Control_p->Transformer_Init = FALSE;

    /* Return the input and output buffers to BM */
    // TBD Rahul Check this part probably should be called for Input also
    // This clean up should not be required
    /*while(Control_p->EncoderOutputQueueFront != Control_p->EncoderOutputQueueTail)
    {
            // Here we will fake the STAud_EncProcessOutput function.we will ask it to put all the input and output buffers its has with it
            STAud_EncProcessOutput(Control_p, NULL);
    }*/

    Control_p->EncInState = MME_ENCODER_INPUT_STATE_IDLE;


    /* Reset Encoder queues */
    // Rahul TBD
    //Control_p->EncoderQueueFront = Control_p->EncoderQueueTail=0;
    Control_p->EncoderOutputQueueFront = Control_p->EncoderOutputQueueTail=0;
    Control_p->EncoderInputQueueFront = Control_p->EncoderInputQueueTail=0;

    /* Change encoder state */
    //STAud_EncSendCommand(MME_ENCODER_STATE_IDLE, Control_p);

    /* Unregister from the block manager */
    /* UnRegister to memory block manager as a Consumer */
    Error |= MemPool_UnRegConsumer(Control_p->InMemoryBlockManagerHandle,Control_p->consumerParam_s.Handle);

    /* UnRegister to memory block manager as a Producer */
    Error |= MemPool_UnRegProducer(Control_p->OutMemoryBlockManagerHandle, Control_p->ProducerParam_s.Handle);

    /* Signal the Command Completion Semaphore - STOP Cmd*/
    STOS_SemaphoreSignal(Control_p->CmdCompletedSem_p);

    STTBX_Print(("STAud_EncStopEncoder: Return !!!\n "));

    return Error;

}


/******************************************************************************
 *  Function name       : EncInitEncoderQueueInParams
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Initalize Encoder queue params
 *                                1. Allocate and initialize MME structure for input buffers
 *                                2. Allocate and initialize MME structure for Output buffers
 ***************************************************************************** */
ST_ErrorCode_t EncInitEncoderQueueInParams(     EncoderControlBlock_t   *TempEncBlock_p,
                                                                                                STAVMEM_PartitionHandle_t AVMEMPartition)
{

    /* Local Variables */
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_AllocBlockParams_t                      AllocParams;
    U32     i;
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
    AllocParams.Size = sizeof(EncoderInputCmdParam_t) * STREAMING_ENCODER_QUEUE_SIZE;

    Error = STAVMEM_AllocBlock(&AllocParams, &TempEncBlock_p->AVMEMInBufferHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 1 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(TempEncBlock_p->AVMEMInBufferHandle, (void **)&(TempEncBlock_p->EncoderInputQueue[0]));
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("In Error = 2 %x",Error));
        return Error;
    }


    for(i = 0; i < STREAMING_ENCODER_QUEUE_SIZE; i++)
    {
        // Get pointer for Queue element
        TempEncBlock_p->EncoderInputQueue[i] = TempEncBlock_p->EncoderInputQueue[0] + i;//*sizeof(EncoderInputCmdParam_t);
        ResetInputQueueElement(TempEncBlock_p->EncoderInputQueue[i]);
    }

    return ST_NO_ERROR;
}

void ResetInputQueueElement(EncoderInputCmdParam_t * inputQueueElement)
{
    U32 j;

    memset((void *)inputQueueElement, 0,sizeof(EncoderInputCmdParam_t));

    // Initialize all the elements of queue
    for (j = 0; j < NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
    {
        /* We will set the Page_p to point to the pcm buffer, received from
                input unit, in Process_Input function */
        inputQueueElement->EncoderBufIn.ScatterPageIn[j].Page_p    = NULL;

        /* Data Buffers initialisation */
        inputQueueElement->EncoderBufIn.ScatterPageIn[j].Size      = 0;//COMPRESSED_BUFFER_SIZE;//2048;  update by the input buffer size
        inputQueueElement->EncoderBufIn.ScatterPageIn[j].BytesUsed = 0;
        inputQueueElement->EncoderBufIn.ScatterPageIn[j].FlagsIn   = (unsigned int)MME_DATA_CACHE_COHERENT;
        inputQueueElement->EncoderBufIn.ScatterPageIn[j].FlagsOut  = 0;
    }

    inputQueueElement->EncoderBufIn.BufferIn.StructSize            = sizeof (MME_DataBuffer_t);
    inputQueueElement->EncoderBufIn.BufferIn.UserData_p            = NULL;
    inputQueueElement->EncoderBufIn.BufferIn.Flags                 = 0;
    inputQueueElement->EncoderBufIn.BufferIn.StreamNumber          = 0;
    // TBD needs to be updated
    inputQueueElement->EncoderBufIn.BufferIn.NumberOfScatterPages  = 0;//NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM;
    inputQueueElement->EncoderBufIn.BufferIn.ScatterPages_p        = &(inputQueueElement->EncoderBufIn.ScatterPageIn[0]);
    inputQueueElement->EncoderBufIn.BufferIn.TotalSize             = 0;
    inputQueueElement->EncoderBufIn.BufferIn.StartOffset           = 0;

    inputQueueElement->EncoderBufIn.BufferIn_p                     = &(inputQueueElement->EncoderBufIn.BufferIn);

    inputQueueElement->Cmd.StructSize                              = sizeof(MME_Command_t);
    inputQueueElement->Cmd.CmdCode                                 = MME_TRANSFORM;
    inputQueueElement->Cmd.CmdEnd                                  = MME_COMMAND_END_RETURN_NOTIFY;
    inputQueueElement->Cmd.DueTime                                 = 0;
    inputQueueElement->Cmd.NumberInputBuffers                      = 1;
    inputQueueElement->Cmd.NumberOutputBuffers                     = 0;
    inputQueueElement->Cmd.DataBuffers_p                           = &(inputQueueElement->EncoderBufIn.BufferIn_p);

    // Fill MME_AudioEncoderTransformParams_t and set in param_p of TRANSFORM command
    // Will be filled equal to input samples before sending the command
    inputQueueElement->EncoderTransformParam.numberOutputSamples   = 0;
    inputQueueElement->EncoderTransformParam.EndOfFile             = 0;
    inputQueueElement->EncoderTransformParam.PtsInfo.PtsFlags      = ACC_NO_PTS_DTS;

    inputQueueElement->Cmd.ParamSize                               = sizeof(MME_AudioEncoderTransformParams_t);
    inputQueueElement->Cmd.Param_p                                 = (MME_GenericParams_t)(&(inputQueueElement->EncoderTransformParam));


    // Fill MME_AudioEncoderStatusParams_t and set in AdditionalInfo_p of TRANSFORM command
    inputQueueElement->EncoderFrameStatus.StructSize               = sizeof(MME_AudioEncoderStatusParams_t);
    inputQueueElement->EncoderFrameStatus.SpecificEncoderStatusBArraySize = MAX_NB_ENCODER_TRANSFORM_RETURN_ELEMENT;

    inputQueueElement->Cmd.CmdStatus.AdditionalInfoSize            = sizeof(MME_AudioEncoderStatusParams_t);
    inputQueueElement->Cmd.CmdStatus.AdditionalInfo_p              = (MME_GenericParams_t)(&(inputQueueElement->EncoderFrameStatus));
    return;
}

/******************************************************************************
 *  Function name       : EncInitEncoderQueueOutParams
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Initalize Encoder queue params
 *                                1. Allocate and initialize MME structure for Output buffers
 ***************************************************************************** */
ST_ErrorCode_t EncInitEncoderQueueOutParams(EncoderControlBlock_t       *TempEncBlock_p,
                                                                                                STAVMEM_PartitionHandle_t AVMEMPartition)
{

    /* Local Variables */
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_AllocBlockParams_t                      AllocBlockParams;
    U32     i;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    AllocBlockParams.PartitionHandle = AVMEMPartition;
    AllocBlockParams.Alignment = 32;
    AllocBlockParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.ForbiddenRangeArray_p = NULL;
    AllocBlockParams.ForbiddenBorderArray_p = NULL;
    AllocBlockParams.NumberOfForbiddenRanges = 0;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.Size = sizeof(EncoderOutputCmdParam_t) * STREAMING_ENCODER_QUEUE_SIZE;

    /* For output buffer 0*/
    Error = STAVMEM_AllocBlock(&AllocBlockParams, &TempEncBlock_p->AVMEMOutBufferHandle);
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 1 %x",Error));
        return Error;
    }

    Error = STAVMEM_GetBlockAddress(TempEncBlock_p->AVMEMOutBufferHandle, (void **)&(TempEncBlock_p->EncoderOutputQueue[0]));
    if(Error!=ST_NO_ERROR)
    {
        STTBX_Print(("Out Error = 2 %x",Error));
        return Error;
    }

    for(i = 0; i < STREAMING_ENCODER_QUEUE_SIZE; i++)
    {
        // Get pointer for Queue element
        TempEncBlock_p->EncoderOutputQueue[i] = TempEncBlock_p->EncoderOutputQueue[0] + i;//*sizeof(EncoderOutputCmdParam_t);
        ResetOutputQueueElement(TempEncBlock_p->EncoderOutputQueue[i]);
    }

    return ST_NO_ERROR;

}

void ResetOutputQueueElement(EncoderOutputCmdParam_t * outputQueueElement)
{
    U32 j;

    memset((void *)outputQueueElement, 0,sizeof(EncoderOutputCmdParam_t));


    // Initialize all the elements of queue
    for (j = 0; j < NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM; j++)
    {
        /* We will set the Page_p to point to the empty buffer, received from
                output unit, in Process_Input function */
        outputQueueElement->EncoderBufOut.ScatterPageOut[j].Page_p              = NULL;

        /* Data Buffers initialisation */
        outputQueueElement->EncoderBufOut.ScatterPageOut[j].Size                = 0;//COMPRESSED_BUFFER_SIZE;//2048;  update by the input buffer size
        outputQueueElement->EncoderBufOut.ScatterPageOut[j].BytesUsed   = 0;
        outputQueueElement->EncoderBufOut.ScatterPageOut[j].FlagsIn             = (unsigned int)MME_DATA_CACHE_COHERENT;
        outputQueueElement->EncoderBufOut.ScatterPageOut[j].FlagsOut    = 0;
    }

    outputQueueElement->EncoderBufOut.BufferOut.StructSize                          = sizeof (MME_DataBuffer_t);
    outputQueueElement->EncoderBufOut.BufferOut.UserData_p                          = NULL;
    outputQueueElement->EncoderBufOut.BufferOut.Flags                                       = 0;
    outputQueueElement->EncoderBufOut.BufferOut.StreamNumber                        = 0;
    // TBD needs to be updated
    outputQueueElement->EncoderBufOut.BufferOut.NumberOfScatterPages        = 0;//NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM;
    outputQueueElement->EncoderBufOut.BufferOut.ScatterPages_p                      = &(outputQueueElement->EncoderBufOut.ScatterPageOut[0]);
    outputQueueElement->EncoderBufOut.BufferOut.TotalSize                           = 0;
    outputQueueElement->EncoderBufOut.BufferOut.StartOffset                         = 0;


    outputQueueElement->EncoderBufOut.BufferOut_p                                           = &(outputQueueElement->EncoderBufOut.BufferOut);

    outputQueueElement->Cmd.StructSize                                                              = sizeof(MME_Command_t);
    outputQueueElement->Cmd.CmdCode                                                                 = MME_SEND_BUFFERS;
    outputQueueElement->Cmd.CmdEnd                                                                  = MME_COMMAND_END_RETURN_NOTIFY;
    outputQueueElement->Cmd.DueTime                                                                 = 0;
    outputQueueElement->Cmd.NumberInputBuffers                                              = 0;
    outputQueueElement->Cmd.NumberOutputBuffers                                             = 1;
    outputQueueElement->Cmd.DataBuffers_p                                                   = &(outputQueueElement->EncoderBufOut.BufferOut_p);


    // Fill MME_StreamingBufferParams_t and set in param_p of SEND_BUFFER command
    // Not required currently
    outputQueueElement->Cmd.ParamSize                                                                       = 0;
    outputQueueElement->Cmd.Param_p                                                                         = NULL;


    // Fill MME_AudioEncoderStatusBuffer_t and set in AdditionalInfo_p of SEND_BUFFER command
    outputQueueElement->EncoderStatusBuffer.StructSize                                      = sizeof(MME_AudioEncoderStatusBuffer_t);

    outputQueueElement->Cmd.CmdStatus.AdditionalInfoSize                            = sizeof(MME_AudioEncoderStatusBuffer_t);
    outputQueueElement->Cmd.CmdStatus.AdditionalInfo_p                                      = (MME_GenericParams_t)(&(outputQueueElement->EncoderStatusBuffer));

    return;
}

void EncInitTransformerInitParams(EncoderControlBlock_t *TempEncBlock_p, enum eAccEncoderId EncID, BOOL EnableByteSwap)
{
    MME_AudioEncoderInitParams_t * tempEncoderInitParams = (MME_AudioEncoderInitParams_t *)TempEncBlock_p->MMEinitParams.TransformerInitParams_p;

    tempEncoderInitParams->StructSize                  = sizeof(MME_AudioEncoderInitParams_t);
    tempEncoderInitParams->BytesToSkipBeginScatterPage = 0;
    tempEncoderInitParams->maxNumberSamplesPerChannel  = 2048; // To be filled just before start

    tempEncoderInitParams->OptionFlags1                = 0x1;  // Bit 0 If set, the buffers are returned when they are full or when an encoded frame is complete

    if (EnableByteSwap == TRUE)
    {
        tempEncoderInitParams->OptionFlags1            |= 0x8;  // Bit 3 If set, the output buffer are byte swaped after encoding
    }

    tempEncoderInitParams->GlobalParams.StructSize     = sizeof(MME_AudioEncoderGlobalParams_t);

    tempEncoderInitParams->GlobalParams.InConfig.Id    = ACC_ENCODER_INPUT_CONFIG_ID;
    tempEncoderInitParams->GlobalParams.InConfig.StructSize = sizeof(MME_AudEncInConfig_t);

    // Input params set below should be configurable before transformer is initialized
    tempEncoderInitParams->GlobalParams.InConfig.ChannelConfig   = TempEncBlock_p->InputParams.NumChannels;//0x2;    // bit 7 - 0: No. of channels
                                                                                                                                              // bit 15 - 8: Encoding Mode
                                                                                                                                              // bit 31 - 16: Channel Swap
    //tempEncoderInitParams->GlobalParams.InConfig.ChannelConfig     = 0x0870a;
    tempEncoderInitParams->GlobalParams.InConfig.WordSize        = TempEncBlock_p->InputParams.WordSizeCode;//ACC_WS32;
    tempEncoderInitParams->GlobalParams.InConfig.SamplingFreq    = CovertToFsCode(TempEncBlock_p->InputParams.InputFrequency);//ACC_FS48k;

    tempEncoderInitParams->GlobalParams.PcmParams.Id                        = ACC_ENCODER_PCMPREPROCESSOR_ID;
    tempEncoderInitParams->GlobalParams.PcmParams.StructSize     = sizeof(MME_AudEncPreProcessingConfig_t);
    tempEncoderInitParams->GlobalParams.PcmParams.Sfc.Id         = ACC_ENCODER_SFC_ID;
    tempEncoderInitParams->GlobalParams.PcmParams.Sfc.StructSize = sizeof(MME_SfcGlobalParams_t);
    tempEncoderInitParams->GlobalParams.PcmParams.Sfc.Apply      = TempEncBlock_p->SFCApply;//ACC_MME_DISABLED;//ACC_MME_ENABLED;//
    tempEncoderInitParams->GlobalParams.PcmParams.Sfc.Config[SFC_MODE] = AENC_SFC_AUTO_NOUT_MODE;


    tempEncoderInitParams->GlobalParams.OutConfig.Id                        = ACC_GENERATE_ENCODERID(EncID);
    tempEncoderInitParams->GlobalParams.OutConfig.StructSize        = sizeof(MME_AudEncOutConfig_t);
    tempEncoderInitParams->GlobalParams.OutConfig.outputBitrate = TempEncBlock_p->OutputParams.BitRate;//128;
    tempEncoderInitParams->GlobalParams.OutConfig.SamplingFreq      = CovertToFsCode(TempEncBlock_p->OutputParams.OutputFrequency);//ACC_FS48k;

    /* Copy encoder specific parameters*/
    switch (EncID)
    {
    case ACC_DDCE_ID:
    case ACC_DDCE51_ID:
        memcpy(tempEncoderInitParams->GlobalParams.OutConfig.Config, &TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.DDConfig, sizeof(TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.DDConfig));
        if (TempEncBlock_p->InputParams.NumChannels == 10)
        {
            tempEncoderInitParams->GlobalParams.InConfig.ChannelConfig      = 0x870a;
            tempEncoderInitParams->GlobalParams.OutConfig.outputBitrate = 384;
        }
        break;
    case ACC_MP3E_ID:
        memcpy(tempEncoderInitParams->GlobalParams.OutConfig.Config, &TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.MP3Config, sizeof(TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.MP3Config));
        break;
    case ACC_MP2E_ID:
        memcpy(tempEncoderInitParams->GlobalParams.OutConfig.Config, &TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.MP2Config, sizeof(TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.MP2Config));
        break;
    case ACC_DTSE_ID:
        tempEncoderInitParams->GlobalParams.OutConfig.Config[0] = 0x10;//LE//0x18;BE output
        if (TempEncBlock_p->InputParams.NumChannels == 10)
        {
            tempEncoderInitParams->GlobalParams.InConfig.ChannelConfig      = 0x870a;
        }
        break;
    case ACC_AACE_ID:
        memcpy(tempEncoderInitParams->GlobalParams.OutConfig.Config, &TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.AacConfig, sizeof(TempEncBlock_p->OutputParams.EncoderStreamSpecificParams.AacConfig));
        break;
    default:
        break;
    }

}

ST_ErrorCode_t  EncoderParseInputBufferParams(EncoderControlBlock_t *Control_p,MemoryBlock_t *InputBlock)
{
    ST_ErrorCode_t  Error=ST_NO_ERROR;

    if (Control_p->Transformer_Init == FALSE)
    {
    /* These parameters can only be updated if transformer is not initialized*/
        if (InputBlock->Flags & FREQ_VALID)
        {
            Control_p->InputParams.InputFrequency = InputBlock->Data[FREQ_OFFSET];
        }

        if (InputBlock->Flags & NCH_VALID)
        {
            Control_p->InputParams.NumChannels = InputBlock->Data[NCH_OFFSET];
        }

        if (InputBlock->Flags & SAMPLESIZE_VALID)
        {
            Control_p->InputParams.WordSizeCode = InputBlock->Data[SAMPLESIZE_OFFSET];
            Control_p->InputParams.WordSize     = ConvertFromAccWSCode(Control_p->InputParams.WordSizeCode);
        }
    }

    if (InputBlock->Flags & PTS_VALID)
    {
        Control_p->CurrentInputBufferToSend->EncoderTransformParam.PtsInfo.PtsFlags = ACC_PTS_PRESENT;
        Control_p->CurrentInputBufferToSend->EncoderTransformParam.PtsInfo.PTS      = InputBlock->Data[PTS_OFFSET];
        Control_p->CurrentInputBufferToSend->EncoderTransformParam.PtsInfo.PtsFlags |= (InputBlock->Data[PTS33_OFFSET] << 16);
    }
    else
    {
        Control_p->CurrentInputBufferToSend->EncoderTransformParam.PtsInfo.PtsFlags = ACC_NO_PTS_DTS;
    }
    return Error;

}

/******************************************************************************
 *  Function name       : EncDeInitEncoderQueueInParam
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : De allocate Encoder queue params
 *                                1. Deallocate Input buffers
 ***************************************************************************** */
ST_ErrorCode_t  EncDeInitEncoderQueueInParam(EncoderControlBlock_t *EncoderControlBlock)
{

    ST_ErrorCode_t  Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;

    FreeBlockParams.PartitionHandle = EncoderControlBlock->InitParams.AVMEMPartition;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&EncoderControlBlock->AVMEMInBufferHandle);
    if(Error!=ST_NO_ERROR)
    {
            STTBX_Print(("4 E = %x",Error));
    }

    return Error;
}

/******************************************************************************
 *  Function name       : EncDeInitEncoderQueueOutParam
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : De allocate Encoder queue params
 *                                1. Deallocate output buffers
 ***************************************************************************** */
ST_ErrorCode_t  EncDeInitEncoderQueueOutParam(EncoderControlBlock_t *EncoderControlBlock)
{

    ST_ErrorCode_t  Error=ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;

    FreeBlockParams.PartitionHandle = EncoderControlBlock->InitParams.AVMEMPartition;
    Error |= STAVMEM_FreeBlock(&FreeBlockParams,&EncoderControlBlock->AVMEMOutBufferHandle);
    if(Error!=ST_NO_ERROR)
    {
            STTBX_Print(("7 E = %x",Error));
    }


    return Error;
}

/******************************************************************************
 *  Function name       : STAud_EncSendCommand
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : Send Command to encoder Task
 ***************************************************************************** */
ST_ErrorCode_t EncoderSendCommand(EncoderControlBlock_t *Control_p,MMEAudioEncoderStates_t   DesiredState)
{

    /* Get the current Encoder State */
    if(Control_p->CurEncoderState != DesiredState)
    {
        /* Send command to Encoder. Change its next state to Desired state */
        STOS_MutexLock(Control_p->LockControlBlockMutex_p);
        Control_p->NextEncoderState = DesiredState;
        STOS_MutexRelease(Control_p->LockControlBlockMutex_p);

        /* Signal Encoder task Semaphore */
        STOS_SemaphoreSignal(Control_p->EncoderTaskSem_p);
    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_EncSendCommand(MMEAudioEncoderStates_t     DesiredState,
                                    EncoderControlBlock_t *Control_p)
{

    switch(DesiredState)
    {
    case MME_ENCODER_STATE_START:
        if((Control_p->CurEncoderState == MME_ENCODER_STATE_IDLE) ||
                (Control_p->CurEncoderState == MME_ENCODER_STATE_STOPPED))
        {
                /*Send Cmd */
                EncoderSendCommand(Control_p , DesiredState);
        }
        else
        {
                return STAUD_ERROR_INVALID_STATE;
        }
        break;

    case MME_ENCODER_STATE_ENCODING:
        /* Internal State */
        if(Control_p->CurEncoderState == MME_ENCODER_STATE_START)
        {
                /*Send Cmd */
                EncoderSendCommand(Control_p , DesiredState);
        }
        else
        {
                return STAUD_ERROR_INVALID_STATE;
        }
        break;

    case MME_ENCODER_STATE_STOP:
        if((Control_p->CurEncoderState == MME_ENCODER_STATE_START) ||
                (Control_p->CurEncoderState == MME_ENCODER_STATE_ENCODING))
        {
                /*Send Cmd */
                EncoderSendCommand(Control_p , DesiredState);
        }
        else
        {
                return STAUD_ERROR_INVALID_STATE;
        }
        break;

    case MME_ENCODER_STATE_STOPPED:
        /* Internal State */
        if(Control_p->CurEncoderState == MME_ENCODER_STATE_STOP)
        {
                /*Send Cmd */
                EncoderSendCommand(Control_p , DesiredState);
        }
        else
        {
                return STAUD_ERROR_INVALID_STATE;
        }
        break;

    case MME_ENCODER_STATE_TERMINATE:
        if((Control_p->CurEncoderState == MME_ENCODER_STATE_STOPPED) ||
                (Control_p->CurEncoderState == MME_ENCODER_STATE_IDLE))
        {
                /*Send Cmd */
                EncoderSendCommand(Control_p , DesiredState);
        }
        else
        {
                return STAUD_ERROR_INVALID_STATE;
        }
        break;
    default:
        break;
    }

    return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_EncMMETransformCompleted(MME_Command_t * CallbackData , void *UserData)
{
    /* Local variables */
    MME_Command_t           *command_p;
    //U32                                   EncodedBufSize = 0;
    EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;
    STAud_EncHandle_t       Handle;


    /* Get the encoder refereance fro which this call back has come */
    Handle = (STAud_EncHandle_t)UserData;

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("ERROR!!!MMETransformCompleted::NO Encoder Handle Found \n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    command_p       = (MME_Command_t *) CallbackData;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("ERROR!!!MMETransformCompleted::MME_COMMAND_FAILED \n"));
    }

    // release input block
    STAud_EncReleaseInput(&Enc_p->EncoderControlBlock,command_p);

    /* Signal Encode Task Semaphore */
    /* Enabled only for testing*/
    /*STOS_SemaphoreSignal(Enc_p->EncoderControlBlock.EncoderTaskSem_p);*/
    // Rahul : Probably we dont need this for streaming transfomers

    return ST_NO_ERROR;

}

/******************************************************************************
 *  Function name       : STAud_EncMMESetGlobalTransformCompleted
 *  Arguments           :
 *       IN                     :
 *       OUT                    :
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : One MME_SET_GLOBAL_TRANSFORM_PARAMS Completed. Remove the completed command from
 *                                List of update commands we sent. We will remove the first node from the list. This is based on the
 * assumption that encoder will process the cmd sequentially i.e. it will return back the commands in the order we sent them.
 * Hence we remove the first command from list and free it
 ***************************************************************************** */
#if 0
ST_ErrorCode_t STAud_EncMMESetGlobalTransformCompleted(MME_Command_t * CallbackData , void *UserData)
{
        /* Local variables */
        MME_Command_t           *command_p;
        EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;
        STAud_EncHandle_t       Handle;
        partition_t                             *DriverPartition = NULL;

        TARGET_AUDIOENCODER_GLOBALPARAMS_STRUCT *gp_params;

        /*STTBX_Print(("MMESetGlobalTransformCompleted::Callback Received \n"));*/
        STTBX_Print(("\n MMEGblTraCom:CB\n"));
        /* Get the encoder refereance fro which this call back has come */
        Handle = (STAud_EncHandle_t)UserData;

        Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
        if(Enc_p == NULL)
        {
                /* handle not found - We shall not come here - ERROR*/
                STTBX_Print(("MMESetGlobalTransformCompleted::Encoder not found \n"));
        }
        command_p       = (MME_Command_t *) CallbackData;
        if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
        {
                /* Command failed */
                STTBX_Print(("MMESetGlobalTransformCompleted::MME_COMMAND_FAILED \n"));
        }
        /* Free the structures passed with command */
        gp_params = (TARGET_AUDIOENCODER_GLOBALPARAMS_STRUCT *) command_p->Param_p;
        if (gp_params->StructSize < sizeof(TARGET_AUDIOENCODER_GLOBALPARAMS_STRUCT))
        {
                /* This was allocated while we were building the PP command for sending to Encoder */
                DriverPartition = Enc_p->EncoderControlBlock.InitParams.DriverPartition;
                memory_deallocate(DriverPartition,gp_params);
                //free(gp_params);
        }

        /* Free the completed command */
        STAud_EncPPRemCmdFrmList(&Enc_p->EncoderControlBlock);

        /* Signal Encode Task Semaphore */
        STOS_SemaphoreSignal(Enc_p->EncoderControlBlock.EncoderTaskSem_p);

        return ST_NO_ERROR;
}
#endif
ST_ErrorCode_t STAud_EncMMESendBufferCompleted(MME_Command_t * CallbackData, void *UserData)
{
    /* Local variables */
    MME_Command_t           *command_p;
    //U32                                   EncodedBufSize = 0;
    EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;
    STAud_EncHandle_t       Handle;


    /* Get the encoder refereance fro which this call back has come */
    Handle = (STAud_EncHandle_t)UserData;

    Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
    if(Enc_p == NULL)
    {
        /* handle not found - We shall not come here - ERROR*/
        STTBX_Print(("ERROR!!!MMETransformCompleted::NO Encoder Handle Found \n"));
        return ST_ERROR_INVALID_HANDLE;
    }

    command_p       = (MME_Command_t *) CallbackData;
    if(command_p->CmdStatus.State == MME_COMMAND_FAILED)
    {
        /* Command failed */
        STTBX_Print(("ERROR!!!MMETransformCompleted::MME_COMMAND_FAILED \n"));
    }

    /*Get the Completed command and release the outputs buffer*/
    STAud_EncProcessOutput(&Enc_p->EncoderControlBlock,command_p);

    /* Signal Encode Task Semaphore */
    /* Rahul enabled this for testing*/
    /* STOS_SemaphoreSignal(Enc_p->EncoderControlBlock.EncoderTaskSem_p);*/
    // Rahul : Probably we dont need this

    return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name       : TransformerCallback_AudioEnc
 *  Arguments           :
 *       IN                     :
 *       OUT                    : Error status
 *       INOUT          :
 *  Return              : ST_ErrorCode_t
 *  Description         : Encoder Callback
 ***************************************************************************** */
void TransformerCallback_AudioEnc(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{
    /* Local variables */
    MME_Command_t   *command_p;
    static U32      ReceivedCount;
    command_p       = (MME_Command_t *) CallbackData;

    if (Event != MME_COMMAND_COMPLETED_EVT)
    {
        // For debugging purpose we can add a few prints here
        return;
    }
    if(ReceivedCount%100==99)
            STTBX_Print((" CB %d \n",ReceivedCount++));
    switch(command_p->CmdCode)
    {
    case MME_TRANSFORM:
        STAud_EncMMETransformCompleted(CallbackData,UserData);
        break;

    case MME_SET_GLOBAL_TRANSFORM_PARAMS:
        //STAud_EncMMESetGlobalTransformCompleted(CallbackData,UserData);
        break;

    case MME_SEND_BUFFERS:
        STAud_EncMMESendBufferCompleted(CallbackData,UserData);
        break;
    default:
        break;
    }

}

/******************************************************************************
 *  Function name       : STAud_EncIsInit
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : Search the list of encoder control blocks to find if "this" encoder is already
 *                                Initialized.
 ***************************************************************************** */
BOOL STAud_EncIsInit(const ST_DeviceName_t DeviceName)
{
    BOOL    Initialized = FALSE;
    EncoderControlBlockList_t *queue = EncoderQueueHead_p;/* Global queue head pointer */

    while (queue)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(queue->EncoderControlBlock.DeviceName, DeviceName) != 0)
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

/******************************************************************************
 *  Function name       : STAud_EncQueueAdd
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine appends an allocated Encoder control block to the
 *                                 Encoder queue.
 ***************************************************************************** */
void STAud_EncQueueAdd(EncoderControlBlockList_t *QueueItem_p)
{
    EncoderControlBlockList_t           *qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if (EncoderQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        EncoderQueueHead_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = EncoderQueueHead_p;
        while (qp != NULL && qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
} /* STAud_EncQueueAdd() */

/******************************************************************************
 *  Function name       : STAud_EncQueueRemove
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine removes a Encoder control block from the
 *                                 Encoder queue.
 ***************************************************************************** */
void *   STAud_EncQueueRemove( EncoderControlBlockList_t        *QueueItem_p)
{
    EncoderControlBlockList_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if (EncoderQueueHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = EncoderQueueHead_p;

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
                EncoderQueueHead_p = this_p->Next_p;
            }
        }
    }
    return (void*)EncoderQueueHead_p;
} /* STAud_EncQueueRemove() */
/******************************************************************************
 *  Function name       : STAud_EncGetControlBlockFromName
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine removes a Encoder control block from the
 *                                 Encoder queue.
 ***************************************************************************** */
EncoderControlBlockList_t *
   STAud_EncGetControlBlockFromName(const ST_DeviceName_t DeviceName)
{
    EncoderControlBlockList_t *qp = EncoderQueueHead_p; /* Global queue head pointer */

    while (qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->EncoderControlBlock.DeviceName, DeviceName) != 0)
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
} /* STAud_EncGetControlBlockFromName() */

/******************************************************************************
 *  Function name       : STAud_EncGetControlBlockFromHandle
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : This routine returns the control block for a given handle
 ***************************************************************************** */
EncoderControlBlockList_t *
   STAud_EncGetControlBlockFromHandle(STAud_EncHandle_t Handle)
{
    EncoderControlBlockList_t *qp = EncoderQueueHead_p; /* Global queue head pointer */

    /* Iterate over the uart queue */
    while (qp != NULL)
    {
        /* Check for a matching handle */
        if (Handle == qp->EncoderControlBlock.Handle && Handle != 0)
        {
            break;  /* This is a valid handle */
        }

        /* Try the next block */
        qp = qp->Next_p;
    }

    /* Return the Enc control block (or NULL) to the caller */
    return qp;
}

ST_ErrorCode_t  STAUD_GetEncCapability( STAUD_ENCCapability_t * EncCapability)
{

    MME_TransformerCapability_t TransformerCapability;
    MME_AudioEncoderInfo_t      EncoderInfo;
    MME_ERROR                   Error = MME_SUCCESS;
    U32                         i,j;
    char                        TransformerName[64];

    /* Fill in the capability struture */
    EncCapability->SupportedStreamTypes =STAUD_STREAM_TYPE_ES;
    EncCapability->SupportedStreamContents = 0;

    /* Get the capabilities from each LX */
    for(j = 0; j < TOTAL_LX_PROCESSORS; j++)
    {
        memset(&EncoderInfo, 0, sizeof(MME_AudioEncoderInfo_t));
        memset(&TransformerCapability, 0, sizeof(MME_TransformerCapability_t));
        TransformerCapability.StructSize                        = sizeof(MME_TransformerCapability_t);
        TransformerCapability.TransformerInfoSize       = sizeof(MME_AudioEncoderInfo_t);
        TransformerCapability.TransformerInfo_p         = &EncoderInfo;
        /* Generate the LX transfommer name(based on ST231 number */
        GetLxName(j,"MME_TRANSFORMER_TYPE_AUDIO_ENCODER",TransformerName, sizeof(TransformerName));
        Error = MME_GetTransformerCapability(TransformerName, &TransformerCapability);
        if (Error == MME_SUCCESS)
        {
            STTBX_Print(("MME capability : Success for Encoder\n"));

            for (i=0; i< 32; i++)
            {
                switch (EncoderInfo.EncoderCapabilityFlags & (1 << i))
                {
                case (1 << ACC_AC3):
                    EncCapability->SupportedStreamContents |= STAUD_STREAM_CONTENT_AC3;
                    break;
                case (1 << ACC_MP2a):
                    EncCapability->SupportedStreamContents |= STAUD_STREAM_CONTENT_MPEG2;
                    break;

                case (1 << ACC_MP3):
                    EncCapability->SupportedStreamContents |= STAUD_STREAM_CONTENT_MP3;
                    break;
                case (1 << ACC_DTS):
                    EncCapability->SupportedStreamContents |= STAUD_STREAM_CONTENT_DTS;
                    break;
                case (1 << ACC_WMA9):
                    EncCapability->SupportedStreamContents |= STAUD_STREAM_CONTENT_WMA;
                    break;
                case (1 << ACC_MP4a_AAC):
                    EncCapability->SupportedStreamContents |= STAUD_STREAM_CONTENT_MPEG_AAC ;

                    break;
                case (1 << ACC_WMAPROLSL):
                case (1 << ACC_DDPLUS):
                case (1 << ACC_GENERATOR):
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
                STTBX_Print(("MME capability : Failture for Encoder\n"));
        }
    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t CalculateEncOutputFrequency(EncoderControlBlock_t *Control_p)
{
    if (Control_p->OutputParams.AutoOutputParam)
    {
        if (EncoderSupportedFrequency(Control_p) == TRUE)
        {
            Control_p->OutputParams.OutputFrequency = Control_p->InputParams.InputFrequency;
            Control_p->SFCApply = ACC_MME_DISABLED;
        }
        else
        {
            EncSFCOutputFreq(Control_p);
        }
    }

    return ST_NO_ERROR;
}

void EncSFCOutputFreq(EncoderControlBlock_t *Control_p)
{
    switch(Control_p->EncID)
    {
    case ACC_DDCE_ID:
    case ACC_DTSE_ID:
    case ACC_WMAE_ID:
        switch (Control_p->InputParams.InputFrequency)
        {
        case 8000:
        case 11025:
        case 12000:
            Control_p->OutputParams.OutputFrequency = 4 * Control_p->InputParams.InputFrequency;
            Control_p->SFCApply = ACC_MME_ENABLED;
            break;
        case 16000:
        case 22050:
        case 24000:
            Control_p->OutputParams.OutputFrequency = 2 * Control_p->InputParams.InputFrequency;
            Control_p->SFCApply = ACC_MME_ENABLED;
            break;
        case 64000:
        case 88200:
        case 96000:
            Control_p->OutputParams.OutputFrequency = Control_p->InputParams.InputFrequency/2;
            Control_p->SFCApply = ACC_MME_ENABLED;
            break;
        case 128000:
        case 176400:
        case 192000:
            Control_p->OutputParams.OutputFrequency = Control_p->InputParams.InputFrequency/4;
            Control_p->SFCApply = ACC_MME_ENABLED;
            break;
        default:
            //This should not occur
            break;
        }
        break;

    case ACC_DDCE51_ID:
        Control_p->OutputParams.OutputFrequency = 48000;
        Control_p->SFCApply = ACC_MME_ENABLED;
        break;

    case ACC_MP3E_ID:
        switch (Control_p->InputParams.InputFrequency)
        {
        case 64000:
        case 88200:
        case 96000:
            Control_p->OutputParams.OutputFrequency = Control_p->InputParams.InputFrequency/2;
            Control_p->SFCApply = ACC_MME_ENABLED;
            break;
        case 128000:
        case 176400:
        case 192000:
            Control_p->OutputParams.OutputFrequency = Control_p->InputParams.InputFrequency/4;
            Control_p->SFCApply = ACC_MME_ENABLED;
            break;
        default:
            //This should not occur
            break;
        }
        break;

    case ACC_MP2E_ID:
    case ACC_AACE_ID:


    default:
           break;

    }

}

BOOL EncoderSupportedFrequency(EncoderControlBlock_t *Control_p)
{
    switch(Control_p->EncID)
    {
    case ACC_DDCE_ID:
    case ACC_DTSE_ID:
    case ACC_WMAE_ID:
        if ((Control_p->InputParams.InputFrequency == 32000) ||
            (Control_p->InputParams.InputFrequency == 48000) ||
            (Control_p->InputParams.InputFrequency == 44100))
        {
            return TRUE;
        }
        break;


    case ACC_DDCE51_ID:
        if (Control_p->InputParams.InputFrequency == 48000)
        {
            return TRUE;
        }
        break;

    case ACC_MP3E_ID:
        if ((Control_p->InputParams.InputFrequency == 32000) ||
            (Control_p->InputParams.InputFrequency == 44100) ||
            (Control_p->InputParams.InputFrequency == 48000) ||
            (Control_p->InputParams.InputFrequency == 8000) ||
            (Control_p->InputParams.InputFrequency == 11025) ||
            (Control_p->InputParams.InputFrequency == 12000) ||
            (Control_p->InputParams.InputFrequency == 16000) ||
            (Control_p->InputParams.InputFrequency == 22050) ||
            (Control_p->InputParams.InputFrequency == 24000))
        {
            return TRUE;
        }
        break;

    case ACC_MP2E_ID:
    case ACC_AACE_ID:


    default:
            return TRUE;

    }

    return FALSE;

}


#if 0
/******************************************************************************
 *  Function name       : STAud_EncGetSamplFreq
 *  Arguments           :
 *       IN                     :
 *       OUT                    : BOOL
 *       INOUT          :
 *  Return              : BOOL
 *  Description         : Get Sampling Frequency
 ***************************************************************************** */
enum eAccFsCode STAud_EncGetSamplFreq(STAud_EncHandle_t Handle)
{

        EncoderControlBlockList_t       *Enc_p= (EncoderControlBlockList_t*)NULL;

        Enc_p = STAud_EncGetControlBlockFromHandle(Handle);
        if(Enc_p == NULL)
        {
                /* handle not found */
                return ST_ERROR_INVALID_HANDLE;
        }

        /* return Sampling frequency for this encoder */
        return(CovertToFsCode(Enc_p->EncoderControlBlock.OutputParams.samplingFrequency));

}
#endif

BOOL CheckEncoderUnderflow(EncoderControlBlock_t        *Control_p)
{

    BOOL EncUnderFlow = FALSE;
    U32  numberPendingInputSamples = 0;

    if (Control_p->CurrentInputBufferToSend != NULL)
    {
        numberPendingInputSamples = (Control_p->CurrentInputBufferToSend->EncoderBufIn.BufferIn.TotalSize/((Control_p->InputParams.WordSize/8) * (Control_p->InputParams.NumChannels)));
    }

    if (Control_p->EncoderInputQueueTail == 0)
    {
        // Let it go till first input is send to LX
        EncUnderFlow = FALSE;
    }
    else
    {
        if ((numberPendingInputSamples + Control_p->PendingInputSamples) > Control_p->PendingOutputSamples)
        {
            EncUnderFlow = TRUE;
        }
        else
        {
            EncUnderFlow = FALSE;
        }
    }
    return (EncUnderFlow);
}

void UpdatePendingSamples(S32   *CurrentSamplePending, S32 FrameSample)
{
    if ((*CurrentSamplePending + FrameSample) < 0)
    {
        STTBX_Print(("Error in computing Pending samples\n"));
    }
    else
    {
        *CurrentSamplePending += FrameSample;
    }
}
/******************************************************************************
 *  Function name       : WmaeCorrectAsfHeader
 *  Arguments           :
 *       IN                     : void
 *       OUT            : void
 *       INOUT          :
 *  Return              : void
 *  Description         : Corrects the ASF header block with the information received
 *                                        in the last block
 ***************************************************************************** */
ST_ErrorCode_t WmaeCorrectAsfHeader(EncoderControlBlock_t       *Control_p)
{

#ifdef ST_OSWINCE
    __int64 n_send_time, nTotalPlayDuration;
    __int64 mult_const=10000L;
    __int64 nFileSize = Control_p->WmaeEncodedDataSize;
    __int64 packet_count;
#else
    long long n_send_time, nTotalPlayDuration;
    long long mult_const=10000L;
    long long nFileSize = Control_p->WmaeEncodedDataSize;
    long long packet_count;
#endif

    ST_ErrorCode_t          status = ST_NO_ERROR;

    MME_WmaeAsfPostProcessing_t *wmae_pp_p = &Control_p->WmaeStatus.AsfPostProcessing;

    U8 *BlkStart = (U8*)Control_p->WmaeAsfHeaderBlock.MemoryStart;

    //printf("In WMAeCorrectASF Header\n");

    //Compute send time from size of wavefile encoded.

    packet_count = (nFileSize - wmae_pp_p->SizeOf_s_asf_header_type) / wmae_pp_p->PacketSize;

    n_send_time = packet_count * wmae_pp_p->SamplesPerPack; //duration in samples

#ifdef ST_OSWINCE
    n_send_time = ((double)n_send_time *(double)1000)/(double)wmae_pp_p->SamplingFreq;
    n_send_time = (__int64)(n_send_time) * mult_const;
    nTotalPlayDuration = (__int64)(n_send_time) + mult_const * wmae_pp_p->nAudioDelayBuffer;
#else
    n_send_time = ((long long)n_send_time *(long long)1000)/(long long)wmae_pp_p->SamplingFreq;
    n_send_time = (long long)(n_send_time) * mult_const;
    nTotalPlayDuration = (long long)(n_send_time) + mult_const * wmae_pp_p->nAudioDelayBuffer;
#endif
    //File Properties Object

    memcpy(BlkStart+wmae_pp_p->SizeOf_s_header_obj+40,&nFileSize,8);

    memcpy(BlkStart+wmae_pp_p->SizeOf_s_header_obj+40+16,&packet_count,8);

    memcpy(BlkStart+wmae_pp_p->SizeOf_s_header_obj+40+16+8,&nTotalPlayDuration,8);

    memcpy(BlkStart+wmae_pp_p->SizeOf_s_header_obj+40+16+8+8,&n_send_time,8);

    //Data Properties Object
    nFileSize -= (wmae_pp_p->SizeOf_s_asf_header_type - wmae_pp_p->SizeOf_s_data_obj);

    memcpy(BlkStart+wmae_pp_p->SizeOf_s_header_obj
    +wmae_pp_p->SizeOf_s_file_obj
    +wmae_pp_p->SizeOf_s_stream_obj
    +wmae_pp_p->SizeOf_s_ext_obj
    +16,&nFileSize,8);


    memcpy(BlkStart+wmae_pp_p->SizeOf_s_header_obj
    +wmae_pp_p->SizeOf_s_file_obj
    +wmae_pp_p->SizeOf_s_stream_obj
    +wmae_pp_p->SizeOf_s_ext_obj
    +16 +8 + 16,&packet_count,8);

    return status;
}


