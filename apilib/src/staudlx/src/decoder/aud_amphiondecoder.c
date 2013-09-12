/********************************************************************************
 *   Filename   	: aud_amphiondecoder.c										*
 *   Start       	: 16.06.2007                                                *
 *   By          	: Rahul Bansal											    *
 *   Contact     	: rahul.bansal@st.com										*
 *   Description  	: The file contains the amphion based Audio Decoder APIs    *
 *                    that will be exported to the Modules. It will also have   *
 *                    decoder task to perform actual decoding					*
 ********************************************************************************/

//#define  STTBX_PRINT
#ifndef ST_OSLINUX
    #define __OS_20TO21_MAP_H /* prevent os_20to21_map.h from being included*/
    #include "sttbx.h"
#endif
#include "stdio.h"
#include "string.h"
#include "aud_amphionregister.h"
#include "aud_amphiondecoder.h"
#include "aud_audiodescription.h"
#include "acc_multicom_toolbox.h"


/* Decoder Instance Array */
ST_DeviceName_t Decoder_DeviceName[] = {"DEC0", "DEC1"};



/* Initialized Decoder list. New decoder will be added to this list on initialization. */
DecoderControlBlockList_t	*DecoderQueueHead_p = NULL;


#define ENABLE_DECODER_122            0
#define DUMP_ENABLE_INPUT             0
#define DUMP_ENABLE_OUTPUT            0
U32 Mp3SampleMap[13] = {1152, 1152, 1152, 0, 576, 576, 576, 0, 576, 576, 576, 0, 0};

#if DUMP_ENABLE_INPUT
    U32 *DumpInput;
    U32 *DumpInputSaved;
    STAVMEM_BlockHandle_t	        DumpInHandle;
    ST_ErrorCode_t 	STAud_DecAllocateDecoderDumpInParams(DecoderControlBlock_t	*);
#endif
#if DUMP_ENABLE_OUTPUT
    U32 *DumpOutput;
    U32 *DumpOutputSaved;
    STAVMEM_BlockHandle_t	        DumpOutHandle;
    ST_ErrorCode_t 	STAud_DecAllocateDecoderDumpOutParams(DecoderControlBlock_t	*);
#endif

/******************************************************************************
 *  Function name 	: STAud_DecInit
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will create the decoder task, initialize MME decoders's In/out
 * 				  paramaters and will update main HAL structure with current values
 ***************************************************************************** */

ST_ErrorCode_t 	STAud_DecInit(const ST_DeviceName_t DeviceName,
                           const STAud_DecInitParams_t *InitParams)
{
	/* Local variables */
	ST_ErrorCode_t error                         = ST_NO_ERROR;
	DecoderControlBlockList_t	*DecCtrlBlock_p  = NULL;
	partition_t 				*DriverPartition = NULL;
	DecoderControlBlock_t		*TempDecBlock;

	/* First Check if this decoder is not already open */
	if(!STAud_DecIsInit(DeviceName))
	{
		/* Decoder not already initialized. So do the initialization */

		/* Allocate memory for new Decoder control block */
		DriverPartition = (partition_t *)InitParams->DriverPartition;

		DecCtrlBlock_p  = (DecoderControlBlockList_t *)memory_allocate(
												DriverPartition,
												sizeof(DecoderControlBlockList_t));
		if(DecCtrlBlock_p == NULL)
		{
			/* Error No memory available */
			return ST_ERROR_NO_MEMORY;
		}

		/* Reset the structure */
		memset((void *)DecCtrlBlock_p, 0, sizeof(DecoderControlBlockList_t));

		/* Fill in the details of our new control block */
		DecCtrlBlock_p->DecoderControlBlock.Handle     = 0;
		strncpy(DecCtrlBlock_p->DecoderControlBlock.DeviceName, DeviceName, ST_MAX_DEVICE_NAME);

		DecCtrlBlock_p->DecoderControlBlock.InitParams = *InitParams;

		/* Initialize MME Structure */
		TempDecBlock = &DecCtrlBlock_p->DecoderControlBlock;

		/* Create the Decoder Task Semaphore */
		TempDecBlock->DecoderTaskSem_p  = STOS_SemaphoreCreateFifo(NULL, 0);
		TempDecBlock->FDMACallbackSem_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

		/* Create the Control Block Mutex */
		TempDecBlock->LockControlBlockMutex_p = STOS_MutexCreateFifo();

		/* Create the Command Competion Semaphore */
		TempDecBlock->CmdCompletedSem_p  = STOS_SemaphoreCreateFifo(NULL, 0);
		TempDecBlock->NumChannels        = InitParams->NumChannels;
		TempDecBlock->AVMEMPartition     = InitParams->AVMEMPartition;
        TempDecBlock->CPUPartition       = InitParams->CPUPartition;
		TempDecBlock->AmphionAccess      = InitParams->AmphionAccess;
		TempDecBlock->AmphionInChannel   = InitParams->AmphionInChannel;
		TempDecBlock->AmphionOutChannel  = InitParams->AmphionOutChannel;
		TempDecBlock->SingleDecoderInput = FALSE;
        #ifdef MP3_SUPPORT
    		TempDecBlock->DecoderTransfer			   = ACF_ST20_MemAllocate(2, InitParams->CPUPartition);
            TempDecBlock->EStoMP3DecoderTransfer	   = TempDecBlock->DecoderTransfer->FrameIn;
            TempDecBlock->MP3DecodertoPCMTransfer	   =	TempDecBlock->DecoderTransfer->PcmOut;
            TempDecBlock->mp3Config_p 				   = 	(tMp3Config *)TempDecBlock->DecoderTransfer->PDC;
           	TempDecBlock->mp3Config_p->FreeFormat	   =	ACF_ST20_FALSE;
           	TempDecBlock->mp3Config_p->DecodeLastFrame =	ACF_ST20_FALSE;
           	TempDecBlock->mp3Config_p->MuteIfError	   =	ACF_ST20_TRUE;
    		TempDecBlock->mp3Config_p->mp3_left_vol    =	0; //-6dB
    		TempDecBlock->mp3Config_p->mp3_right_vol   = 	0; //-3db
    	    EStoMP3DecoderTransferParameter(TempDecBlock);
    	    MP3DecodertoPCMTransferParameter(TempDecBlock);
    	    DecoderTransferParameter(TempDecBlock);
        #endif
		/* Initialize and Allocate Decoder Queue's buffer pointer */
        error |= STAud_DecAllocateDecoderQueueOutParams(TempDecBlock);
        error |= STAud_DecAllocateDecoderQueueInParams(TempDecBlock);
        error |= STAud_DecInitDecoderQueueInParams(TempDecBlock);
		error |= STAud_DecInitDecoderQueueOutParams(TempDecBlock);

        #if DUMP_ENABLE_INPUT
          error |= STAud_DecAllocateDecoderDumpInParams(TempDecBlock);
        #endif
        #if DUMP_ENABLE_OUTPUT
           error |= STAud_DecAllocateDecoderDumpOutParams(TempDecBlock);
        #endif

		/* Init Mute param */
		TempDecBlock->PcmProcess.Mute  = FALSE;

		/* Set the Decoder Starting State */
		TempDecBlock->CurDecoderState  = DECODER_STATE_IDLE;
		TempDecBlock->NextDecoderState = DECODER_STATE_IDLE;

		/* Set Input processing state */
		TempDecBlock->DecInState       = DECODER_PROCESSING_STATE_IDLE;

        /* Force default values of PP params*/
        TempDecBlock->HoldCompressionMode.Apply = FALSE;
        TempDecBlock->HoldDynamicRange.Apply    = FALSE;
        TempDecBlock->HoldStereoMode.Apply      = FALSE;
        TempDecBlock->HoldDownMixParams.Apply   = FALSE;

		/*Reset The transformer restart required*/
		TempDecBlock->decoderInByPassMode       = FALSE;

        #if DEBUG_DECODING_TIME
            TempDecBlock->frameCount          = 0;
            TempDecBlock->TimeTakeninDecoding = 0;
            TempDecBlock->TimeTakenPerFrame   = 0;
        #endif

		/* Reset Decoder queues */
		TempDecBlock->DecoderInputQueueFront  = TempDecBlock->DecoderInputQueueTail  = 0;
		TempDecBlock->DecoderOutputQueueFront = TempDecBlock->DecoderOutputQueueTail = 0;
		TempDecBlock->DecoderOutputQueueNext  = TempDecBlock->DecoderInputQueueNext  = 0;

        // Default decoding mode for the time being
		STOS_TaskCreate(STAud_DecTask_Entry_Point, &DecCtrlBlock_p->DecoderControlBlock, TempDecBlock->CPUPartition,  AMPHION_AUDIO_DECODER_TASK_STACK_SIZE,
                                        (void **)&TempDecBlock->DecoderTaskstack, TempDecBlock->CPUPartition, &TempDecBlock->DecoderTask, &TempDecBlock->DecoderTaskDesc, AMPHION_AUDIO_DECODER_TASK_PRIORITY,
                                        "DecoderTask",
                                        0);
		if (TempDecBlock->DecoderTask == NULL)
		{
			/* Error */
			return ST_ERROR_NO_MEMORY;
		}


		/* Reuse trancoded buffer in case 0 size transcoded buffer is returned*/
		/* Should be used only for certification*/
		TempDecBlock->Reuse_transcoded_buffer = FALSE;

		/* Get Buffer Manager Handles */
		DecCtrlBlock_p->DecoderControlBlock.InMemoryBlockManagerHandle  = InitParams->InBufHandle;
		DecCtrlBlock_p->DecoderControlBlock.OutMemoryBlockManagerHandle = InitParams->OutBufHandle;

		/* Only Initialize with Buffer Manager Params. We will register/unregister with Block manager in DRStart/DRStop */
		/* As Consumer of incoming data */
		TempDecBlock->consumerParam_s.Handle = (U32)DecCtrlBlock_p;
		TempDecBlock->consumerParam_s.sem    = DecCtrlBlock_p->DecoderControlBlock.DecoderTaskSem_p;
		/* Register to memory block manager as a consumer */
		//error |= MemPool_RegConsumer(DecCtrlBlock_p->DecoderControlBlock.InMemoryBlockManagerHandle,TempDecBlock->consumerParam_s);

		/* As Producer of outgoing data */
		TempDecBlock->ProducerParam_s.Handle = (U32)DecCtrlBlock_p;
		TempDecBlock->ProducerParam_s.sem    = DecCtrlBlock_p->DecoderControlBlock.DecoderTaskSem_p;
		/* Register to memory block manager as a Producer */
		//error |= MemPool_RegProducer(DecCtrlBlock_p->DecoderControlBlock.OutMemoryBlockManagerHandle, TempDecBlock->ProducerParam_s);

		/* Initialize the Decoder Capability structure */
		STAud_InitDecCapability(TempDecBlock);

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

/******************************************************************************
 *  Function name 	: STAud_DecTerm
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will terminate the decoder task, Free MME decoders's In/out
 * 				  paramaters and will update main HAL structure with current values
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecTerm(STAud_DecHandle_t		Handle)
{
	/* Local Variable */
	ST_ErrorCode_t error         = ST_NO_ERROR;
	DecoderControlBlockList_t *Dec_p;
	partition_t *DriverPartition = NULL;
	task_t *TempTask_p;

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

	error |= STAud_DecSendCommand(DECODER_STATE_TERMINATE, &Dec_p->DecoderControlBlock);
	if(error == STAUD_ERROR_INVALID_STATE)
	{
		STTBX_Print(("STAud_DecTerm: Error !!!\n "));
		return error;
	}

	STOS_TaskWait(&TempTask_p, TIMEOUT_INFINITY);

    #ifdef MP3_SUPPORT
    	ACF_ST20_MemFree(Dec_p->DecoderControlBlock.DecoderTransfer, 2, Dec_p->DecoderControlBlock.CPUPartition);//mp3
    #endif

    #if defined (ST_OS20)
    	STOS_TaskDelete(TempTask_p, Dec_p->DecoderControlBlock.CPUPartition, (void *)Dec_p->DecoderControlBlock.DecoderTaskstack, Dec_p->DecoderControlBlock.CPUPartition);
    #else
    	STOS_TaskDelete(TempTask_p, NULL, NULL, NULL);
    #endif
	DriverPartition = Dec_p->DecoderControlBlock.InitParams.DriverPartition;

	if(error != ST_NO_ERROR)
	{
		STTBX_Print(("STAud_DecTerm: Failed to deallocate queue\n "));
	}

    STAud_DecFreeDecoderQueueInParams(&Dec_p->DecoderControlBlock);
    STAud_DecFreeDecoderQueueOutParams(&Dec_p->DecoderControlBlock);

	/* Terminate Decoder Semaphore */
	STOS_SemaphoreDelete(NULL, Dec_p->DecoderControlBlock.DecoderTaskSem_p);
	STOS_SemaphoreDelete(NULL, Dec_p->DecoderControlBlock.FDMACallbackSem_p);
	/* Delete Mutex */
	STOS_MutexDelete(Dec_p->DecoderControlBlock.LockControlBlockMutex_p);

	/* Terminate Cmd Completion Semaphore */
	STOS_SemaphoreDelete(NULL, Dec_p->DecoderControlBlock.CmdCompletedSem_p);

	/* Remove the Control Block from Queue */
	STAud_DecQueueRemove(Dec_p);

	/*Free the Control Block */
	memory_deallocate(DriverPartition, Dec_p);

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
 *  Function name 	: STAud_DecOpen
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will Open the decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecOpen(const ST_DeviceName_t DeviceName,
									STAud_DecHandle_t *Handle)
{
	ST_ErrorCode_t error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p;

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
 *  Function name 	: STAud_DecClose
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will close the task
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecClose()
{
	return ST_NO_ERROR;
}

ST_ErrorCode_t 	STAud_DecSetStreamParams(STAud_DecHandle_t		Handle,
								STAUD_StreamParams_t * StreamParams_p)
{
	ST_ErrorCode_t	Error              = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;
	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Set decoder specidifc parameters */
	Dec_p->DecoderControlBlock.StreamParams.StreamContent     = StreamParams_p->StreamContent;
	Dec_p->DecoderControlBlock.StreamParams.StreamType        = StreamParams_p->StreamType;
	Dec_p->DecoderControlBlock.StreamParams.SamplingFrequency = StreamParams_p->SamplingFrequency;
	if(Dec_p->DecoderControlBlock.StreamParams.StreamContent == STAUD_STREAM_CONTENT_MP3 \
	||Dec_p->DecoderControlBlock.StreamParams.StreamContent == STAUD_STREAM_CONTENT_PCM \
	||Dec_p->DecoderControlBlock.StreamParams.StreamContent == STAUD_STREAM_CONTENT_CDDA)
	{
 		Dec_p->DecoderControlBlock.DecoderMode = DECODER_CODEC;
	}
	else
	{
		Dec_p->DecoderControlBlock.DecoderMode = DECODER_DUAL;
	}
	return Error;

}

/******************************************************************************
 *  Function name 	: STAud_DecStart
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will send init and play cmd to Decoder Task. This
 *  				   function should be called from DRStart() function
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecStart(STAud_DecHandle_t		Handle)
{
	ST_ErrorCode_t	Error              = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Send the Start command to decoder. */
	Error = STAud_DecSendCommand(DECODER_STATE_START, &Dec_p->DecoderControlBlock);

	if(Error == ST_NO_ERROR)
	{
		/* Wait on sema for START completion */
		STOS_SemaphoreWait(Dec_p->DecoderControlBlock.CmdCompletedSem_p);
	}

	return Error;
}

/******************************************************************************
 *  Function name 	: STAud_DecStop
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will Send the Stop cmd to Decoder Task. This
 *  				   function should be called from DRStop() function
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecStop(STAud_DecHandle_t		Handle)
{

	ST_ErrorCode_t	Error = ST_NO_ERROR;

	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;

	//STTBX_Print(("STAud_DecStop: Called \n "));

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}


	/* Send the Stop command to decoder. */
	Error = STAud_DecSendCommand(DECODER_STATE_STOP, &Dec_p->DecoderControlBlock);

	Dec_p->DecoderControlBlock.decoderInByPassMode = FALSE;

	if(Error == ST_NO_ERROR)
	{
		/* Wait on sema for START completion */
		STOS_SemaphoreWait(Dec_p->DecoderControlBlock.CmdCompletedSem_p);
	}

	STTBX_Print(("STAud_DecStop: Successful\n "));
	return Error;
}


/******************************************************************************
 *  Function name 	: STAud_DecGetCapability
 *  Arguments    	:
 *       IN			:	Dec Handle
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: Returns the decoder capabilities
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecGetCapability(STAud_DecHandle_t	Handle,
											STAUD_DRCapability_t * Capability_p)
{
	ST_ErrorCode_t	Error              = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;

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
/******************************************************************************
*						DECODER TASK										   *
******************************************************************************/
/******************************************************************************
 *  Function name 	: STAud_DecTask_Entry_Point
 *  Arguments    	:
 *       IN			:
 *       OUT    		: void
 *       INOUT		:
 *  Return       	: void
 *  Description   	: This function is the entry point for decoder task. Ihe decoder task will be in
 *  				   different states depending of the command received from upper/lower layers
 ***************************************************************************** */
void    STAud_DecTask_Entry_Point(void* params)
{

	ST_ErrorCode_t Error  = ST_NO_ERROR;
	ST_ErrorCode_t Status = ST_NO_ERROR;

	/* Get the Decoder control Block for this task */
	DecoderControlBlock_t	*ControlBlock = (DecoderControlBlock_t*)params;

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

			case DECODER_STATE_IDLE:
				break;

			case DECODER_STATE_START:

				Status = STAud_DecSetAndInitDecoder(ControlBlock);
				if(Status != ST_NO_ERROR)
				{
					STTBX_Print(("Audio Deocder:Init failure\n"));
				}
				else
				{
					STTBX_Print(("Audio Deocder:Init Success\n"));
				}

				Status |= ResetQueueParams(ControlBlock);
				/* Decoder Initialized. Now Send Play Command to Decoder Task i.e. this task itself */
				if(Status == ST_NO_ERROR)
				{
					STAud_DecSendCommand(DECODER_STATE_PLAYING, ControlBlock);
				}
				else
				{
					STAud_DecSendCommand(DECODER_STATE_IDLE, ControlBlock);
				}

				break;

			case DECODER_STATE_PLAYING:

				if (ControlBlock->decoderInByPassMode)
				{
					/* 	Bypass mode - If the companion bin doesn't support the requested decoder.
						Eg - if AC3 is removed from bin, we go to bypass mode. User wil still get output on SPDIF in compressed mode
					*/
					ControlBlock->DecInState = DECODER_PROCESSING_STATE_BYPASS;
				}
				else
				{
       				Error = STAud_DecProcessData(ControlBlock);
				}
				break;

			case DECODER_STATE_STOP:
				Error = STAud_DecStopDecoder(ControlBlock);
				if(Error != ST_NO_ERROR)
				{
					STTBX_Print(("STAud_DecStopDecoder::Failed %x \n", Error));
				}
				/* Decoder Stoped. Now Send Stopped Command to Decoder Task i.e. this task itself */
				STAud_DecSendCommand(DECODER_STATE_STOPPED,  ControlBlock);
				break;

			case DECODER_STATE_STOPPED:
				break;

			case DECODER_STATE_TERMINATE:
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
*						LOCAL FUNCTIONS										   *
******************************************************************************/
ST_ErrorCode_t	ResetQueueParams(DecoderControlBlock_t	*ControlBlock)
{
	// Reset Input Queue params
	STAud_DecInitDecoderQueueOutParams(ControlBlock);
	STAud_DecInitDecoderQueueInParams(ControlBlock);

	ControlBlock->SamplingFrequency = 0xFF;
	ControlBlock->AudioCodingMode   = 0xFF;
	ControlBlock->OutputCodingMode  = 0xFF;
	ControlBlock->OutputEmphasis    = 0xFF;
	ControlBlock->OutputLayerInfo   = 0xFF;

	return ST_NO_ERROR;
}

ST_ErrorCode_t	STAud_DecHandleInput(DecoderControlBlock_t	*ControlBlock)
{
	ST_ErrorCode_t	status                       = ST_NO_ERROR;
	U32 QueueTailIndex                           = 0;
	DecoderInputBufferParam_t *InputBufferToSend = NULL;

	/* Geting Input buffers */

    while (status == ST_NO_ERROR)
    {
        if (ControlBlock->DecoderInputQueueTail - ControlBlock->DecoderInputQueueFront < DECODER_QUEUE_SIZE)
        {
    		// We have space to get another output buffer
    		QueueTailIndex     = ControlBlock->DecoderInputQueueTail%DECODER_QUEUE_SIZE;
    		InputBufferToSend  = &ControlBlock->DecoderInputQueue[QueueTailIndex];
    		status = MemPool_GetIpBlk(ControlBlock->InMemoryBlockManagerHandle,  (U32 *)&InputBufferToSend->InputBlockFromProducer, ControlBlock->Handle);
    		if (status == ST_NO_ERROR)
    		{
    		    STAud_DecSetStreamInfo(ControlBlock, InputBufferToSend->InputBlockFromProducer);
    		    InputBufferToSend->INodeListCount = 0;
    		    ControlBlock->DecoderInputQueueTail++;
    		}

        }
    }

    if ((ControlBlock->DecoderInputQueueTail - ControlBlock->DecoderInputQueueFront) > ControlBlock->BackBufferCount)
    {
        // We have enough buffers to start decoding
        status = ST_NO_ERROR;
    }

	return status;
}

ST_ErrorCode_t	STAud_DecGetOutBuf(DecoderControlBlock_t	*ControlBlock)
{
	ST_ErrorCode_t status                          = ST_NO_ERROR;
	U32 QueueTailIndex                             = 0;
	DecoderOutputBufferParam_t *OutputBufferToSend = NULL;

    while (status == ST_NO_ERROR)
    {
        if (ControlBlock->DecoderOutputQueueTail - ControlBlock->DecoderOutputQueueFront < DECODER_QUEUE_SIZE)
        {
    		// We have space to get another output buffer
    		QueueTailIndex      = ControlBlock->DecoderOutputQueueTail%DECODER_QUEUE_SIZE;
    		OutputBufferToSend  = &ControlBlock->DecoderOutputQueue[QueueTailIndex];
    		status              = MemPool_GetOpBlk(ControlBlock->OutMemoryBlockManagerHandle, (U32 *)&OutputBufferToSend->OutputBlockFromProducer);
    		if (status == ST_NO_ERROR)
    		{
    		    OutputBufferToSend->OutputBlockFromProducer->Flags = 0;
    		    OutputBufferToSend->ONodeListCount                 = 0;
    		    ControlBlock->DecoderOutputQueueTail++;

    		}

        }
    }

    if ((ControlBlock->DecoderOutputQueueTail - ControlBlock->DecoderOutputQueueFront) >= 1)
    {
        // We have enough buffers to start decoding
        status = ST_NO_ERROR;
    }


	return status;
}

/******************************************************************************
 *  Function name 	: STAud_DecProcessData
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: Get the comressed input buffers, get empty PCM buffer,Update MME params
 *				   and send command to MME decoder
 ***************************************************************************** */
ST_ErrorCode_t	STAud_DecProcessData(DecoderControlBlock_t	*ControlBlock)
{

	ST_ErrorCode_t status = ST_NO_ERROR;

    while(status == ST_NO_ERROR)
    {
    	switch(ControlBlock->DecInState)
    	{

    		case DECODER_PROCESSING_STATE_IDLE:
                #if DEBUG_DECODING_TIME
        		    if (ControlBlock->frameCount % 100 == 0)
        		    {
        		        ControlBlock->TimeTakenPerFrame = ((ControlBlock->TimeTakeninDecoding/ControlBlock->frameCount)*1000)/ST_GetClocksPerSecond();
        		        STTBX_Print(("Decoding time for Dec : FrCount %d:Time per frame (ms) %d:\n", ControlBlock->frameCount, ControlBlock->TimeTakenPerFrame));
        		    }
                #endif
    			/* Intended fall through */
    			ControlBlock->DecInState = DECODER_PROCESSING_STATE_GET_OUTPUT_BUFFER;

    		case DECODER_PROCESSING_STATE_GET_OUTPUT_BUFFER:

    			/* Get the first output buffer. It will hold decoded data */
    			status = STAud_DecGetOutBuf(ControlBlock);

    			if(status != ST_NO_ERROR)
    			{
    				return status;
    			}

    			/* Intended fall through */
    			ControlBlock->DecInState = DECODER_PROCESSING_STATE_GET_INPUT_DATA;

    		case DECODER_PROCESSING_STATE_GET_INPUT_DATA:

    			/* Get the input buffers from parser */
       			status = STAud_DecHandleInput(ControlBlock);

    			if(status != ST_NO_ERROR)
    			{
    				return status;
    			}

    			/* Intended fall through */
    			ControlBlock->DecInState = DECODER_PROCESSING_STATE_UPDATE_QUEUES;

    		case DECODER_PROCESSING_STATE_UPDATE_QUEUES:
    		    switch (ControlBlock->DecoderMode)
    		    {
    		        case DECODER_CODEC:
            			ControlBlock->DecInState = DECODER_PROCESSING_STATE_DECODE_FRAME;
    		            break;
    		        case DECODER_DUAL:
    		        default:
            		    if (CheckAndUpdateInputFDMAQueue(ControlBlock) == STAUD_ERROR_NOT_ENOUGH_BUFFERS ||
            		        CheckAndUpdateOutputFDMAQueue(ControlBlock)== STAUD_ERROR_NOT_ENOUGH_BUFFERS)
            		    {
                            return (STAUD_ERROR_NOT_ENOUGH_BUFFERS);
            		    }
            			ControlBlock->DecInState = DECODER_PROCESSING_STATE_GET_AMPHION;
    		            break;
    		    }
    		    break;

    		case DECODER_PROCESSING_STATE_GET_AMPHION:

                if (ControlBlock->InitParams.AmphionAccess != NULL)
                {
                    STOS_SemaphoreWait(ControlBlock->InitParams.AmphionAccess);
                }

    			/* Intended fall through */
    			ControlBlock->DecInState = DECODER_PROCESSING_STATE_DECODE_FRAME;

    		case DECODER_PROCESSING_STATE_DECODE_FRAME:
                #if DEBUG_DECODING_TIME
        		    ControlBlock->frameCount++;
        		    ControlBlock->timeBeforeDecode = STOS_time_now();
                #endif
    		    DecodeFrame(ControlBlock);

                #if DEBUG_DECODING_TIME
        		    ControlBlock->TimeTakeninDecoding += STOS_time_minus(STOS_time_now(), ControlBlock->timeBeforeDecode);
                #endif

    			/* Intended fall through */
    			ControlBlock->DecInState = DECODER_PROCESSING_STATE_RELEASE_AMPHION;

    		case DECODER_PROCESSING_STATE_RELEASE_AMPHION:
                if (ControlBlock->InitParams.AmphionAccess != NULL)
                {
                    STOS_SemaphoreSignal(ControlBlock->InitParams.AmphionAccess);
                }

                // Moved here after release of amphion to allow thread descheduling
                //ReleaseInputBuffer(ControlBlock, FALSE);

    			/* Intended fall through */
    			ControlBlock->DecInState = DECODER_PROCESSING_STATE_IDLE;
    			break;

    		case DECODER_PROCESSING_STATE_BYPASS:
    			/* Special state -  For handling AC3 playback without support from companion bin */
    			break;

    		default:
    			break;

    	}
    }
	return status;
}





ST_ErrorCode_t STAud_DecApplyPcmParams(DecoderControlBlock_t	*ControlBlock)
{
	ST_ErrorCode_t	status = ST_NO_ERROR;

	if(ControlBlock->HoldCompressionMode.Apply)
	{
		ControlBlock->HoldCompressionMode.Apply = FALSE;
	}

	if(ControlBlock->HoldDynamicRange.Apply)
	{
		ControlBlock->HoldDynamicRange.Apply = FALSE;
	}

	if(ControlBlock->HoldStereoMode.Apply)
	{
	    ControlBlock->HoldStereoMode.Apply = FALSE;
	}

	if(ControlBlock->HoldDownMixParams.Apply)
	{
	    ControlBlock->HoldDownMixParams.Apply = FALSE;
	}

	return status;
}

/******************************************************************************
 *  Function name 	: STAud_DecSetAndInitDecoder
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: MME_ERROR
 *  Description   	:
 ***************************************************************************** */
ST_ErrorCode_t	STAud_DecSetAndInitDecoder(DecoderControlBlock_t	*ControlBlock)
{
	ST_ErrorCode_t   Error = ST_NO_ERROR;

	/*Transformer restart*/
	ControlBlock->Reuse_transcoded_buffer= FALSE;

	/*Reset AD Structure.*/
	ControlBlock->ADControl.Fade				= 0;
	ControlBlock->ADControl.Pan					= 0;
	ControlBlock->ADControl.CurPan				= 0;
	ControlBlock->ADControl.TargetPan			= 0;
	ControlBlock->ADControl.PanDiff				= 0;
	ControlBlock->ADControl.FrameNumber			= 0;
	ControlBlock->ADControl.ApplyFrameFactor	= 0;
	ControlBlock->ADControl.RampFactor			= 0;
	ControlBlock->ADControl.PanErrorFlag		= FALSE;
	ControlBlock->ADControl.NumFramesPerSecond	= 0;
	ControlBlock->Attenuation_p.Left            = 0;
	ControlBlock->Attenuation_p.Right           = 0;
	ControlBlock->Attenuation_p.Center          = 0;
	ControlBlock->Attenuation_p.Subwoofer       = 0;
	ControlBlock->Attenuation_p.LeftSurround    = 0;
	ControlBlock->Attenuation_p.RightSurround   = 0;

	/* Default values. These shall be overwritten by individual decoder type */
	ControlBlock->DefaultDecodedSize            = 1536 * 4 * ControlBlock->NumChannels;
	ControlBlock->DefaultCompressedFrameSize    = 1536;

	switch(ControlBlock->StreamParams.StreamContent)
	{
		case STAUD_STREAM_CONTENT_MPEG1:
		case STAUD_STREAM_CONTENT_MPEG2:
		case STAUD_STREAM_CONTENT_MP3	:
			/* Set the default decoded buffer size for this stream */
			ControlBlock->DefaultDecodedSize         = 1152 * BYTES_PER_CHANNEL * ControlBlock->NumChannels;
			ControlBlock->DefaultCompressedFrameSize = 1152;

			/* Default configuration */
			if(!ControlBlock->SingleDecoderInput)
			{
				ControlBlock->BackBufferCount            = 1;
			}
			else
			{
				ControlBlock->BackBufferCount            = 0;
			}
			ControlBlock->LayerInfo                      = LAYER_2;
			break;

		case STAUD_STREAM_CONTENT_CDDA:
		case STAUD_STREAM_CONTENT_PCM:
			/* Set the default decoded buffer size for this stream */
			ControlBlock->DefaultDecodedSize         = CDDA_NB_SAMPLES * 4 * ControlBlock->NumChannels;
			ControlBlock->DefaultCompressedFrameSize = CDDA_NB_SAMPLES;
			break;

		case STAUD_STREAM_CONTENT_MPEG_AAC:
		case STAUD_STREAM_CONTENT_HE_AAC:
		case STAUD_STREAM_CONTENT_DDPLUS:
		case STAUD_STREAM_CONTENT_LPCM:
		case STAUD_STREAM_CONTENT_LPCM_DVDA:
		case STAUD_STREAM_CONTENT_MLP:
		case STAUD_STREAM_CONTENT_DTS:
		case STAUD_STREAM_CONTENT_WMA:
		case STAUD_STREAM_CONTENT_WMAPROLSL:
		case STAUD_STREAM_CONTENT_ADIF:
		case STAUD_STREAM_CONTENT_MP4_FILE:
		case STAUD_STREAM_CONTENT_RAW_AAC:
		case STAUD_STREAM_CONTENT_BEEP_TONE:
		case STAUD_STREAM_CONTENT_PINK_NOISE:
		case STAUD_STREAM_CONTENT_DV:
		case STAUD_STREAM_CONTENT_CDDA_DTS:
		case STAUD_STREAM_CONTENT_NULL:
		default:
			STTBX_Print(("STAud_DecSetAndInitDecoder: Unsupported Decoder Type !!!\n "));
			break;
	}



	Error = STAud_DecApplyPcmParams(ControlBlock); //SA
	if(ControlBlock->SingleDecoderInput)
	{
    	ResetAudioDecoder(ControlBlock);
    	SetVolumeandStereoMode(ControlBlock);
	}
	/* Register to memory block manager as a Producer */
	if(ControlBlock->OutMemoryBlockManagerHandle)
	{
		Error |= MemPool_RegProducer(ControlBlock->OutMemoryBlockManagerHandle, ControlBlock->ProducerParam_s);
	}


	//Checking the capability of the LX firmware for the stream

	if (ControlBlock->DecCapability.SupportedStreamContents & ControlBlock->StreamParams.StreamContent)
	{
		/* Register to memory block manager as a consumer for input */
		if(ControlBlock->InMemoryBlockManagerHandle)
		{
			Error |= MemPool_RegConsumer(ControlBlock->InMemoryBlockManagerHandle, ControlBlock->consumerParam_s);
		}
	}
	else
	{
		ControlBlock->decoderInByPassMode = TRUE;
		Error = ST_NO_ERROR;

	}

	/* Signal the Command Completion Semaphore */
	STOS_SemaphoreSignal(ControlBlock->CmdCompletedSem_p);

	return Error;

}

/******************************************************************************
 *  Function name 	: STAud_DecStopDecoder
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_NO_ERROR
 *  Description   	:
 ***************************************************************************** */
ST_ErrorCode_t	STAud_DecStopDecoder(DecoderControlBlock_t	*ControlBlock)
{
	ST_ErrorCode_t   Error = ST_NO_ERROR;

	STTBX_Print(("STAud_DecStopDecoder: Called !!!\n "));

	/*  terminate all the transformers */
	// Wait for FDMA to finish
	// free all remaining buffers


	ControlBlock->DecInState = DECODER_PROCESSING_STATE_IDLE;

	/* Reset Decoder queues */
	ControlBlock->DecoderOutputQueueFront = ControlBlock->DecoderOutputQueueTail=0;
	ControlBlock->DecoderInputQueueFront  = ControlBlock->DecoderInputQueueTail=0;
	ControlBlock->DecoderOutputQueueNext  = ControlBlock->DecoderInputQueueNext=0;

	/* Reset stream contents */
	memset((void*)&ControlBlock->StreamParams, 0, sizeof(STAUD_StreamParams_t));

	/* UnRegister to memory block manager as a Producer */
	if(ControlBlock->OutMemoryBlockManagerHandle)
	{
		Error |= MemPool_UnRegProducer(ControlBlock->OutMemoryBlockManagerHandle, ControlBlock->Handle);
	}

	if (!ControlBlock->decoderInByPassMode)
	{
		/* Unregister from the block manager */
		/* UnRegister to memory block manager as a Consumer */
		if(ControlBlock->InMemoryBlockManagerHandle)
		{
			Error |= MemPool_UnRegConsumer(ControlBlock->InMemoryBlockManagerHandle, ControlBlock->Handle);
		}

	}

	/* Signal the Command Completion Semaphore - STOP Cmd*/
	STOS_SemaphoreSignal(ControlBlock->CmdCompletedSem_p);

	//STTBX_Print(("STAud_DecStopDecoder: Return !!!\n "));

	return Error;

}


#if DUMP_ENABLE_INPUT
    ST_ErrorCode_t 	STAud_DecAllocateDecoderDumpInParams(	DecoderControlBlock_t	*TempDecBlock)
    {
    	ST_ErrorCode_t   Error = ST_NO_ERROR;
    	void * AllocAddress;
    	STAVMEM_AllocBlockParams_t  AllocParams;
    	U32 i;

    	AllocParams.PartitionHandle          = TempDecBlock->AVMEMPartition;
    	AllocParams.Alignment                = 32;
    	AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    	AllocParams.ForbiddenRangeArray_p    = NULL;
    	AllocParams.ForbiddenBorderArray_p   = NULL;
    	AllocParams.NumberOfForbiddenRanges  = 0;
    	AllocParams.NumberOfForbiddenBorders = 0;
    	AllocParams.Size = 1024*1024;

    	DumpInHandle = 0;

    	Error = STAVMEM_AllocBlock(&AllocParams, &DumpInHandle);
        if (Error != ST_NO_ERROR)
        {
            return Error;
        }
        Error |= STAVMEM_GetBlockAddress(DumpInHandle, &AllocAddress);
        if (Error != ST_NO_ERROR)
        {
            return Error;
        }
        DumpInput = (U32 *)AllocAddress;
        DumpInputSaved = DumpInput;
        return Error;
    }
#endif


#if DUMP_ENABLE_OUTPUT
    ST_ErrorCode_t 	STAud_DecAllocateDecoderDumpOutParams(DecoderControlBlock_t	*TempDecBlock)
    {
    	ST_ErrorCode_t   Error = ST_NO_ERROR;
    	void * AllocAddress;
    	STAVMEM_AllocBlockParams_t  AllocParams;
        U32 i;

    	AllocParams.PartitionHandle          = TempDecBlock->AVMEMPartition;
    	AllocParams.Alignment                = 32;
    	AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    	AllocParams.ForbiddenRangeArray_p    = NULL;
    	AllocParams.ForbiddenBorderArray_p   = NULL;
    	AllocParams.NumberOfForbiddenRanges  = 0;
    	AllocParams.NumberOfForbiddenBorders = 0;
    	AllocParams.Size                     = 2*1024*1024;
    	DumpOutHandle                        = 0;

        Error = STAVMEM_AllocBlock(&AllocParams, &DumpOutHandle);
        if (Error != ST_NO_ERROR)
        {
            return Error;
        }
        Error |= STAVMEM_GetBlockAddress(DumpOutHandle, &AllocAddress);
        if (Error != ST_NO_ERROR)
        {
            return Error;
        }
        DumpOutput = (U32 *)AllocAddress;
        DumpOutputSaved = DumpOutput;
        return Error;
    }
#endif


ST_ErrorCode_t 	STAud_DecAllocateDecoderQueueInParams(	DecoderControlBlock_t	*TempDecBlock)
{
	ST_ErrorCode_t   Error = ST_NO_ERROR;
	void * AllocAddress;
	STAVMEM_AllocBlockParams_t  AllocParams;
	U32 i;

	AllocParams.PartitionHandle          = TempDecBlock->AVMEMPartition;
	AllocParams.Alignment                = 32;
	AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
	AllocParams.ForbiddenRangeArray_p    = NULL;
	AllocParams.ForbiddenBorderArray_p   = NULL;
	AllocParams.NumberOfForbiddenRanges  = 0;
	AllocParams.NumberOfForbiddenBorders = 0;
	AllocParams.Size                     = (sizeof(STFDMA_GenericNode_t) * DECODER_INODE_LIST_COUNT);
	TempDecBlock->BufferInHandle         = 0;

	Error = STAVMEM_AllocBlock(&AllocParams, &TempDecBlock->BufferInHandle);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }
    Error |= STAVMEM_GetBlockAddress(TempDecBlock->BufferInHandle, &AllocAddress);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }
	for( i=0; i<DECODER_INODE_LIST_COUNT; i++)
	{
		if(Error == ST_NO_ERROR)
		{
			if(Error == ST_NO_ERROR)
			{
				TempDecBlock->INode[i] = (STFDMA_GenericNode_t *)((U32)AllocAddress + i*sizeof(STFDMA_GenericNode_t));
			}
			else
			{
				return Error;
			}
		}
	}

    return Error;
}

ST_ErrorCode_t 	STAud_DecAllocateDecoderQueueOutParams(DecoderControlBlock_t	*TempDecBlock)
{
	ST_ErrorCode_t   Error = ST_NO_ERROR;
	void * AllocAddress;
	STAVMEM_AllocBlockParams_t  AllocParams;
    U32 i;

	AllocParams.PartitionHandle          = TempDecBlock->AVMEMPartition;
	AllocParams.Alignment                = 32;
	AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
	AllocParams.ForbiddenRangeArray_p    = NULL;
	AllocParams.ForbiddenBorderArray_p   = NULL;
	AllocParams.NumberOfForbiddenRanges  = 0;
	AllocParams.NumberOfForbiddenBorders = 0;
	AllocParams.Size                     = (sizeof(STFDMA_GenericNode_t) * DECODER_ONODE_LIST_COUNT);

	TempDecBlock->BufferOutHandle        = 0;

    Error = STAVMEM_AllocBlock(&AllocParams, &TempDecBlock->BufferOutHandle);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }
    Error |= STAVMEM_GetBlockAddress(TempDecBlock->BufferOutHandle, &AllocAddress);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }
	for( i=0; i<DECODER_ONODE_LIST_COUNT; i++)
	{
		if(Error == ST_NO_ERROR)
		{
			if(Error == ST_NO_ERROR)
			{
				TempDecBlock->ONode[i]=(STFDMA_GenericNode_t *)((U32)AllocAddress + i*sizeof(STFDMA_GenericNode_t));
			}
			else
			{
				return Error;
			}
		}
	}
    return Error;
}

ST_ErrorCode_t 	STAud_DecFreeDecoderQueueInParams(	DecoderControlBlock_t	*TempDecBlock)
{
    ST_ErrorCode_t   Error = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;

    if (TempDecBlock->BufferInHandle != 0)
    {
        FreeBlockParams.PartitionHandle = TempDecBlock->AVMEMPartition;
        Error |= STAVMEM_FreeBlock(&FreeBlockParams, &TempDecBlock->BufferInHandle);
    }

    return Error;

}

ST_ErrorCode_t 	STAud_DecFreeDecoderQueueOutParams(DecoderControlBlock_t	*TempDecBlock)
{
    ST_ErrorCode_t   Error = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t FreeBlockParams;

    if (TempDecBlock->BufferOutHandle != 0)
    {
        FreeBlockParams.PartitionHandle = TempDecBlock->AVMEMPartition;
        Error |= STAVMEM_FreeBlock(&FreeBlockParams, &TempDecBlock->BufferOutHandle);
    }

    return Error;


}

/******************************************************************************
 *  Function name 	: STAud_DecInitDecoderQueueInParams
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: Initalize Decoder queue params
 ***************************************************************************** */
ST_ErrorCode_t STAud_DecInitDecoderQueueInParams(DecoderControlBlock_t	*TempDecBlock)
{

	/* Local Variables */
	U32	i;
    STFDMA_GenericNode_t *Node_p;


    for (i = 0; i < DECODER_QUEUE_SIZE; i++)
    {
        TempDecBlock->DecoderInputQueue[i].InputBlockFromProducer = NULL;
        TempDecBlock->DecoderInputQueue[i].INodeListCount         = 0;
    }

    for (i = 0; i < DECODER_INODE_LIST_COUNT; i++)
    {

        Node_p = TempDecBlock->INode[i];
        memset((char *)Node_p, 0, sizeof(STFDMA_GenericNode_t));

	    Node_p->Node.DestinationAddress_p             = (void *)((volatile U32 *) (AUDIO_BASE_ADDRESS+AUD_DEC_CD_IN));
		Node_p->Node.NodeControl.PaceSignal           = STFDMA_REQUEST_SIGNAL_CD_AUDIO;
		Node_p->Node.NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
		Node_p->Node.NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
		Node_p->Node.NodeControl.NodeCompleteNotify   = FALSE;
		Node_p->Node.NodeControl.NodeCompletePause    = FALSE;
		Node_p->Node.NodeControl.Reserved             = 0;

		if (i < (DECODER_INODE_LIST_COUNT - 1))
		{
		    Node_p->Node.Next_p = (STFDMA_Node_t *)TempDecBlock->INode[i+1];
		}
		else
		{
		    Node_p->Node.Next_p = NULL;
		}
	}

    TempDecBlock->InputTransferParams.ChannelId         = TempDecBlock->AmphionInChannel;
    TempDecBlock->InputTransferParams.Pool              = STFDMA_DEFAULT_POOL;
    TempDecBlock->InputTransferParams.NodeAddress_p     = TempDecBlock->INode[0];
    TempDecBlock->InputTransferParams.BlockingTimeout   = 0;
    TempDecBlock->InputTransferParams.FDMABlock         = STFDMA_1;
    TempDecBlock->InputTransferParams.CallbackFunction  = DecoderInputFDMACallbackFunction;
    TempDecBlock->InputTransferParams.ApplicationData_p = (void *)TempDecBlock;


	return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name 	: STAud_DecInitDecoderQueueOutParams
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: Initalize Decoder queue params
 ***************************************************************************** */
ST_ErrorCode_t STAud_DecInitDecoderQueueOutParams(DecoderControlBlock_t	*TempDecBlock)
{

	/* Local Variables */
	U32	i;
    STFDMA_GenericNode_t *Node_p;

    for (i = 0; i < DECODER_QUEUE_SIZE; i++)
    {
        TempDecBlock->DecoderOutputQueue[i].OutputBlockFromProducer = NULL;
        TempDecBlock->DecoderOutputQueue[i].ONodeListCount          = 0;
    }

    for (i = 0; i < DECODER_ONODE_LIST_COUNT; i++)
    {
        Node_p = TempDecBlock->ONode[i];
        memset((char *)Node_p, 0, sizeof(STFDMA_GenericNode_t));

	    Node_p->Node.SourceAddress_p                  = (void *)((volatile U32 *) (AUDIO_BASE_ADDRESS+AUD_DEC_PCM_OUT));
		Node_p->Node.NodeControl.PaceSignal           = STFDMA_REQUEST_SIGNAL_AUDIO_DECODER;
		Node_p->Node.NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
		Node_p->Node.NodeControl.SourceDirection      = STFDMA_DIRECTION_STATIC;
		Node_p->Node.NodeControl.NodeCompleteNotify   = FALSE;
		Node_p->Node.NodeControl.NodeCompletePause    = FALSE;
		Node_p->Node.NodeControl.Reserved             = 0;

		if (i < (DECODER_ONODE_LIST_COUNT - 1))
		{
		    Node_p->Node.Next_p = (STFDMA_Node_t *)TempDecBlock->ONode[i+1];
		}
		else
		{
		    Node_p->Node.Next_p = NULL;
		}
	}

    TempDecBlock->OutputTransferParams.ChannelId         = TempDecBlock->AmphionOutChannel;
    TempDecBlock->OutputTransferParams.Pool              = STFDMA_DEFAULT_POOL;
    TempDecBlock->OutputTransferParams.NodeAddress_p     = TempDecBlock->ONode[0];
    TempDecBlock->OutputTransferParams.BlockingTimeout   = 0;
    TempDecBlock->OutputTransferParams.FDMABlock         = STFDMA_1;
    TempDecBlock->OutputTransferParams.CallbackFunction  = DecoderOutputFDMACallbackFunction;
    TempDecBlock->OutputTransferParams.ApplicationData_p = (void *)TempDecBlock;


	return ST_NO_ERROR;
}



/******************************************************************************
 *  Function name 	: STAud_DecSendCommand
 *  Arguments    	:
 *       IN			:
 *       OUT    		: BOOL
 *       INOUT		:
 *  Return       	: BOOL
 *  Description   	: Send Command to decoder Task
 ***************************************************************************** */
ST_ErrorCode_t STAud_SendCommand(DecoderControlBlock_t *ControlBlock, AudioDecoderStates_t	DesiredState)
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
	ControlBlock->streamingTransformerFirstTransfer = 1;//mp3
	return ST_NO_ERROR;
}

ST_ErrorCode_t STAud_DecSendCommand(AudioDecoderStates_t	DesiredState,
									DecoderControlBlock_t *ControlBlock)
{

	switch(DesiredState)
	{
		case DECODER_STATE_START:
			if((ControlBlock->CurDecoderState == DECODER_STATE_IDLE) ||
				(ControlBlock->CurDecoderState == DECODER_STATE_STOPPED))
			{
				/*Send Cmd */
				STAud_SendCommand(ControlBlock , DesiredState);
			}
			else
			{
				return STAUD_ERROR_INVALID_STATE;
			}
			break;

		case DECODER_STATE_PLAYING:
			/* Internal State */
			if(ControlBlock->CurDecoderState == DECODER_STATE_START)
			{
				/*Send Cmd */
				STAud_SendCommand(ControlBlock , DesiredState);
			}
			else
			{
				return STAUD_ERROR_INVALID_STATE;
			}
			break;

		case DECODER_STATE_STOP:
			if((ControlBlock->CurDecoderState == DECODER_STATE_START) ||
				(ControlBlock->CurDecoderState == DECODER_STATE_PLAYING))
			{
				/*Send Cmd */
				STAud_SendCommand(ControlBlock , DesiredState);
			}
			else
			{
				return STAUD_ERROR_INVALID_STATE;
			}
			break;

		case DECODER_STATE_STOPPED:
			/* Internal State */
			if(ControlBlock->CurDecoderState == DECODER_STATE_STOP)
			{
				/*Send Cmd */
				STAud_SendCommand(ControlBlock , DesiredState);
			}
			else
			{
				return STAUD_ERROR_INVALID_STATE;
			}
			break;

		case DECODER_STATE_TERMINATE:
			if((ControlBlock->CurDecoderState == DECODER_STATE_STOPPED) ||
				(ControlBlock->CurDecoderState == DECODER_STATE_IDLE))
			{
				/*Send Cmd */
				STAud_SendCommand(ControlBlock , DesiredState);
			}
			else
			{
				return STAUD_ERROR_INVALID_STATE;
			}
			break;
		case DECODER_STATE_IDLE:
		default:
			break;
	}

	return ST_NO_ERROR;
}

void DecoderInputFDMACallbackFunction(U32 TransferId, U32 CallbackReason, U32 *CurrentNode_p, U32 NodeBytesRemaining, BOOL Error, void *ApplicationData_p,  clock_t  InterruptTime )
{
    DecoderControlBlock_t *ControlBlock = (DecoderControlBlock_t *)ApplicationData_p;

    switch (CallbackReason)
    {
        case STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING:
            STTBX_Print((" Input Continue \n"));
            break;

        case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
            STTBX_Print((" Input Paused \n"));
            break;

        case STFDMA_NOTIFY_TRANSFER_ABORTED:
            #if !ENABLE_DECODER_122
                STTBX_Print((" Input Aborted \n"));
            #endif
            break;

        case STFDMA_NOTIFY_TRANSFER_COMPLETE:
            ControlBlock->InputTransferCompleted = TRUE;
            STOS_SemaphoreSignal(ControlBlock->FDMACallbackSem_p);
            break;

    }

}

void DecoderOutputFDMACallbackFunction(U32 TransferId, U32 CallbackReason, U32 *CurrentNode_p, U32 NodeBytesRemaining, BOOL Error, void *ApplicationData_p, clock_t  InterruptTime )
{
    DecoderControlBlock_t *ControlBlock = (DecoderControlBlock_t *)ApplicationData_p;

    switch (CallbackReason)
    {
        case STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING:
            STTBX_Print((" Output Continue \n"));
            break;

        case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
            STTBX_Print((" Output Paused \n"));
            break;

        case STFDMA_NOTIFY_TRANSFER_ABORTED:
           STTBX_Print((" Output Aborted \n"));
            break;

        case STFDMA_NOTIFY_TRANSFER_COMPLETE:
            ControlBlock->OutputTransferCompleted = TRUE;
            STOS_SemaphoreSignal(ControlBlock->FDMACallbackSem_p);
            break;

    }
}



/******************************************************************************
 *  Function name 	: STAud_DecIsInit
 *  Arguments    	:
 *       IN			:
 *       OUT    		: BOOL
 *       INOUT		:
 *  Return       	: BOOL
 *  Description   	: Search the list of decoder control blocks to find if "this" decoder is already
 *				  Initialized.
 ***************************************************************************** */
BOOL STAud_DecIsInit(const ST_DeviceName_t DeviceName)
{
	BOOL	Initialized = FALSE;
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

/******************************************************************************
 *  Function name 	: STAud_DecQueueAdd
 *  Arguments    	:
 *       IN			:
 *       OUT    		: BOOL
 *       INOUT		:
 *  Return       	: BOOL
 *  Description   	: This routine appends an allocated Decoder control block to the
 * 				   Decoder queue.
 ***************************************************************************** */
void STAud_DecQueueAdd(DecoderControlBlockList_t *QueueItem_p)
{
    DecoderControlBlockList_t		*qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if (DecoderQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        DecoderQueueHead_p  = QueueItem_p;
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
        qp->Next_p          = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
} /* STAud_DecQueueAdd() */

/******************************************************************************
 *  Function name 	: STAud_DecQueueRemove
 *  Arguments    	:
 *       IN			:
 *       OUT    		: BOOL
 *       INOUT		:
 *  Return       	: BOOL
 *  Description   	: This routine removes a Decoder control block from the
 * 				   Decoder queue.
 ***************************************************************************** */
void *   STAud_DecQueueRemove( DecoderControlBlockList_t	*QueueItem_p)
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
 *  Function name 	: STAud_DecGetControlBlockFromName
 *  Arguments    	:
 *       IN			:
 *       OUT    		: BOOL
 *       INOUT		:
 *  Return       	: BOOL
 *  Description   	: This routine removes a Decoder control block from the
 * 				   Decoder queue.
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
 *  Function name 	: STAud_DecGetControlBlockFromHandle
 *  Arguments    	:
 *       IN			:
 *       OUT    		: BOOL
 *       INOUT		:
 *  Return       	: BOOL
 *  Description   	: This routine returns the control block for a given handle
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


ST_ErrorCode_t	STAud_InitDecCapability(DecoderControlBlock_t *TempDecBlock)
{

	/* Fill in the capability struture */
	TempDecBlock->DecCapability.DynamicRangeCapable	    = TRUE;
	TempDecBlock->DecCapability.FadingCapable		    = FALSE;
	TempDecBlock->DecCapability.RunTimeSwitch		    = FALSE;
	TempDecBlock->DecCapability.SupportedStopModes	    = 0;

	TempDecBlock->DecCapability.SupportedStereoModes    = (STAUD_STEREO_MODE_STEREO |
    													    STAUD_STEREO_MODE_DUAL_LEFT |STAUD_STEREO_MODE_DUAL_RIGHT |
    													    STAUD_STEREO_MODE_DUAL_MONO);


	TempDecBlock->DecCapability.SupportedStreamTypes	= (STAUD_STREAM_TYPE_ES | STAUD_STREAM_TYPE_PES | STAUD_STREAM_TYPE_PES_DVD);
	TempDecBlock->DecCapability.SupportedTrickSpeeds[0] = 0;

	TempDecBlock->DecCapability.SupportedStreamContents = (STAUD_STREAM_CONTENT_MPEG1 | STAUD_STREAM_CONTENT_MPEG2 |
	                                                       STAUD_STREAM_CONTENT_MP3 | STAUD_STREAM_CONTENT_PCM | STAUD_STREAM_CONTENT_CDDA);


	return ST_NO_ERROR;
}

/******************************************************************************
 *  Function name 	: STAud_DecGetSamplFreq
 *  Arguments    	:
 *       IN			:
 *       OUT    		: BOOL
 *       INOUT		:
 *  Return       	: BOOL
 *  Description   	: Get Sampling Frequency
 ***************************************************************************** */
U32 STAud_DecGetSamplFreq(STAud_DecHandle_t Handle)
{

	DecoderControlBlockList_t *Dec_p = (DecoderControlBlockList_t*)NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* return Sampling frequency for this decoder */
	return(Dec_p->DecoderControlBlock.StreamParams.SamplingFrequency);

}


ST_ErrorCode_t	STAud_DecGetOPBMHandle(STAud_DecHandle_t	Handle, U32 OutputId, STMEMBLK_Handle_t * BMHandle_p)
{
	DecoderControlBlockList_t	*Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	ST_ErrorCode_t 	Error = ST_NO_ERROR;

	if(Dec_p)
	{
		DecoderControlBlock_t *DecCtrlBlk_p = &(Dec_p->DecoderControlBlock);
		switch(OutputId)
		{
		case 0:
			* BMHandle_p = DecCtrlBlk_p->OutMemoryBlockManagerHandle;
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


ST_ErrorCode_t	STAud_DecSetIPBMHandle(STAud_DecHandle_t	Handle, U32 InputId, STMEMBLK_Handle_t BMHandle)
{
	DecoderControlBlockList_t *Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	ST_ErrorCode_t 	Error            =ST_NO_ERROR;

	if(Dec_p)
	{
		DecoderControlBlock_t	* DecCtrlBlk_p = &(Dec_p->DecoderControlBlock);
		STOS_MutexLock(DecCtrlBlk_p->LockControlBlockMutex_p);
		switch(DecCtrlBlk_p->CurDecoderState)
		{
		case DECODER_STATE_IDLE:
		case DECODER_STATE_STOPPED:
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
	if((InputBlockFromProducer->Flags & LAYER_VALID) && (ControlBlock->LayerInfo != InputBlockFromProducer->Data[LAYER_OFFSET]))
	{
		ControlBlock->LayerInfo = (enum MPEG)((U32)InputBlockFromProducer->Data[LAYER_OFFSET]);
		UpdateLayerInfo(ControlBlock);
		STTBX_Print(("LAYER UPDATE\n"));
	}
	if(InputBlockFromProducer->Flags & FADE_VALID)
	{
		ControlBlock->ADControl.Fade = InputBlockFromProducer->Data[FADE_OFFSET];
	}
	if(InputBlockFromProducer->Flags & PAN_VALID)
	{
		ControlBlock->ADControl.Pan = InputBlockFromProducer->Data[PAN_OFFSET];
		//STTBX_Print(("Pan received :%d\n",ControlBlock->ADControl.Pan));
		if(ControlBlock->ADControl.Pan == 0xFF00 && ControlBlock->ADControl.PanErrorFlag!=TRUE) /*Rampdown condition*/
		{
			ControlBlock->ADControl.Pan          =  0;
			ControlBlock->ADControl.PanErrorFlag = TRUE;
			ControlBlock->ADControl.TargetPan    = 0;

			if(ControlBlock->ADControl.CurPan != 0)// appply ramping only if curpan is not zero.
			{
			    STAud_CalculateRamp(ControlBlock);
			}
		}
		else if(ControlBlock->ADControl.PanErrorFlag && ControlBlock->ADControl.Pan != 0xFF00)
		{
			STAud_CalculateRamp(ControlBlock);
			ControlBlock->ADControl.TargetPan    = ControlBlock->ADControl.Pan;
			ControlBlock->ADControl.PanErrorFlag = 0; //reset the error flag after recovery from error
		}
	}

	STAud_ApplyPan(ControlBlock);

}

void    STAud_ApplyPan(DecoderControlBlock_t * ControlBlock)
{

	int TempCurPan , TempTargetPan;
	if(ControlBlock->ADControl.ApplyFrameFactor != 0)
	{
		if((ControlBlock->ADControl.TargetPan != ControlBlock->ADControl.CurPan)&&
		    (ControlBlock->ADControl.FrameNumber == ControlBlock->DecoderInputQueueTail))
		{
		    TempTargetPan = ControlBlock->ADControl.TargetPan;
		    TempCurPan    = ControlBlock->ADControl.CurPan + ControlBlock->ADControl.RampFactor;
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

		    ControlBlock->ADControl.CurPan      = ((ControlBlock->ADControl.PanDiff)<0)?ControlBlock->ADControl.TargetPan:(U32)TempCurPan;
		    ControlBlock->ADControl.FrameNumber = ControlBlock->ADControl.FrameNumber + ControlBlock->ADControl.ApplyFrameFactor;
		    STAUD_SetAmphionPan(ControlBlock);

		}
		else
		{
		    ControlBlock->ADControl.ApplyFrameFactor = 0;
        }
	}
	else if((ControlBlock->ADControl.Pan != ControlBlock->ADControl.CurPan) && (!ControlBlock->ADControl.PanErrorFlag))
	{
		ControlBlock->ADControl.TargetPan = ControlBlock->ADControl.CurPan;
		ControlBlock->ADControl.CurPan    = ControlBlock->ADControl.Pan;
    	//STTBX_Print(( " Pan Applied=[%d]\n",ControlBlock->ADControl.CurPan ));
		STAUD_SetAmphionPan(ControlBlock);


	}
}


void	STAud_CalculateRamp(DecoderControlBlock_t * ControlBlock)
{
	U32 Pan    = ControlBlock->ADControl.Pan;
	U32 CurPan = ControlBlock->ADControl.CurPan;
	U32 NumFramesPerSecond,PanDiff;
	NumFramesPerSecond = ControlBlock->ADControl.NumFramesPerSecond;
	ControlBlock->ADControl.FrameNumber = ControlBlock->DecoderInputQueueTail;
	if(CurPan >= Pan)
	{
		PanDiff = CurPan - Pan;
		ControlBlock->ADControl.RampFactor = -1;
	}
	else
	{
		PanDiff = Pan - CurPan;
		ControlBlock->ADControl.RampFactor = 1;
	}

	if(PanDiff>128)
	{
		PanDiff = 256 - PanDiff;
		ControlBlock->ADControl.RampFactor = ControlBlock->ADControl.RampFactor * -1;
	}
	ControlBlock->ADControl.PanDiff          = PanDiff;
	ControlBlock->ADControl.ApplyFrameFactor = NumFramesPerSecond/PanDiff;
	if(!ControlBlock->ADControl.ApplyFrameFactor)
	{
		ControlBlock->ADControl.ApplyFrameFactor = 1;
		ControlBlock->ADControl.RampFactor       = ControlBlock->ADControl.RampFactor * (PanDiff/NumFramesPerSecond);
	}


}

void ResetAudioDecoder(DecoderControlBlock_t * ControlBlock)
{
	U32 count = 0;

	STSYS_WriteAudioReg32(AUD_DEC_CONFIG, 0x00000001);
	/*Check for reset*/
	STOS_TaskDelay(ST_GetClocksPerSecond() / 100000);
	while(!(STSYS_ReadAudioReg32(AUD_DEC_STATUS)& AUD_SW_RESET))
	{
		count++;
    	STOS_TaskDelay(ST_GetClocksPerSecond() / 100000);
		if(count > 100)
		{
			STTBX_Print(("\n Decoder Reset Failed \n"));
			break;
		}
	}


	/**(U32 *) (0x20700000)=0x00; *//* Amphion Decoder == Soft Reset Removed */
	STSYS_WriteAudioReg32(AUD_DEC_CONFIG, 0x00000000);
}

void SetVolumeandStereoMode(DecoderControlBlock_t * ControlBlock)
{
	STAUD_SetAttenuationAtAmphion(ControlBlock->Attenuation_p.Left, ControlBlock->Attenuation_p.Right);
	if(ControlBlock->HoldStereoMode.Apply)
	{
	    STAUD_SetStereoOutputAtAmphion(ControlBlock->HoldStereoMode.StereoMode);
	}
}

void	UpdateLayerInfo(DecoderControlBlock_t * ControlBlock)
{
    switch(ControlBlock->LayerInfo)
    {
        case LAYER_1:
			if(!ControlBlock->SingleDecoderInput)
			{
	            ControlBlock->BackBufferCount = 2;
			}
			else
			{
	            ControlBlock->BackBufferCount = 0;
			}
			ControlBlock->DefaultDecodedSize  = 384 * 2 * ControlBlock->NumChannels;
            break;
        case LAYER_2:
        case LAYER_3:
        default:

			if(!ControlBlock->SingleDecoderInput)
			{
	            ControlBlock->BackBufferCount = 1;
			}
			else
			{
	            ControlBlock->BackBufferCount = 0;
			}
			ControlBlock->DefaultDecodedSize    = 1152 * 2 * ControlBlock->NumChannels;
            break;
    }
}

ST_ErrorCode_t CheckAndUpdateInputFDMAQueue(DecoderControlBlock_t * ControlBlock)
{
    U32 i, j;
	ST_ErrorCode_t 	Error = ST_NO_ERROR;
	DecoderInputBufferParam_t *NextBufferToDecode, *CurrentBufferToFill;

    if ((ControlBlock->DecoderInputQueueFront + ControlBlock->BackBufferCount) < ControlBlock->DecoderInputQueueTail)
    {
        // We have an input frame and enough back buffers to decode
        // Update Next frame to decode
        ControlBlock->DecoderInputQueueNext = ControlBlock->DecoderInputQueueFront + ControlBlock->BackBufferCount;
        NextBufferToDecode = &ControlBlock->DecoderInputQueue[ControlBlock->DecoderInputQueueNext%DECODER_QUEUE_SIZE];

        // Check if queue is not already updated
        if (NextBufferToDecode->INodeListCount == 0)
        {
            // This loop will fill node list with back buffers and current buffer
            for (i = ControlBlock->DecoderInputQueueFront, j = 0;
                 i <= (ControlBlock->DecoderInputQueueFront + ControlBlock->BackBufferCount); i++, j++)
            {
                CurrentBufferToFill = &ControlBlock->DecoderInputQueue[i%DECODER_QUEUE_SIZE];
                ControlBlock->INode[j]->Node.SourceAddress_p = (void *)CurrentBufferToFill->InputBlockFromProducer->MemoryStart;
                ControlBlock->INode[j]->Node.NumberBytes     = CurrentBufferToFill->InputBlockFromProducer->Size;
                ControlBlock->INode[j]->Node.Next_p          = (STFDMA_Node_t *)(&ControlBlock->INode[j+1]->Node);
                ControlBlock->INode[j]->Node.Length          = ControlBlock->INode[j]->Node.NumberBytes;
            }

#if ENABLE_DECODER_122
            // This will fill an additional node in front of current node to force output buffer
            // generation
            CurrentBufferToFill = &ControlBlock->DecoderInputQueue[(i-1)%DECODER_QUEUE_SIZE];
            ControlBlock->INode[j]->Node.SourceAddress_p = (void *)CurrentBufferToFill->InputBlockFromProducer->MemoryStart;
            ControlBlock->INode[j]->Node.NumberBytes     = CurrentBufferToFill->InputBlockFromProducer->Size;
            ControlBlock->INode[j]->Node.Next_p          = NULL;
            ControlBlock->INode[j]->Node.Length          = ControlBlock->INode[j]->Node.NumberBytes;
            NextBufferToDecode->INodeListCount           = (ControlBlock->BackBufferCount + 2);
#else
            ControlBlock->INode[j-1]->Node.Next_p        = NULL;
            NextBufferToDecode->INodeListCount           = (ControlBlock->BackBufferCount + 1);

#endif
        }


    }
    else
    {
        Error = STAUD_ERROR_NOT_ENOUGH_BUFFERS;
    }

    return Error;
}

ST_ErrorCode_t CheckAndUpdateOutputFDMAQueue(DecoderControlBlock_t * ControlBlock)
{
    U32 i;
	ST_ErrorCode_t 	Error = ST_NO_ERROR;
	DecoderOutputBufferParam_t *NextOutputBuffer;

    if (ControlBlock->DecoderOutputQueueFront < ControlBlock->DecoderOutputQueueTail)
    {
        NextOutputBuffer = &(ControlBlock->DecoderOutputQueue[ControlBlock->DecoderOutputQueueFront%DECODER_QUEUE_SIZE]);
        if (NextOutputBuffer->ONodeListCount == 0)
        {
            ControlBlock->DecoderOutputQueueNext = ControlBlock->DecoderOutputQueueFront;
            for (i = 0; i < (ControlBlock->BackBufferCount + 1); i++)
            {
                ControlBlock->ONode[i]->Node.DestinationAddress_p = (void *)NextOutputBuffer->OutputBlockFromProducer->MemoryStart;
                ControlBlock->ONode[i]->Node.NumberBytes          = ControlBlock->DefaultDecodedSize;
                ControlBlock->ONode[i]->Node.Length               = ControlBlock->DefaultDecodedSize;
                ControlBlock->ONode[i]->Node.Next_p               = &ControlBlock->ONode[i+1]->Node;
            }
            ControlBlock->ONode[i-1]->Node.Next_p = NULL;
            NextOutputBuffer->ONodeListCount = (ControlBlock->BackBufferCount + 1);
        }

    }
    else
    {
        Error = STAUD_ERROR_NOT_ENOUGH_BUFFERS;
    }

    return Error;
}

ST_ErrorCode_t DecodeFrame(DecoderControlBlock_t * ControlBlock)
{
    switch (ControlBlock->DecoderMode)
    {
        case DECODER_CODEC:
			if(ControlBlock->StreamParams.StreamContent == STAUD_STREAM_CONTENT_MP3)
			{
#ifdef MP3_SUPPORT
                ConfigureMP3Decoder(ControlBlock);
                StartMP3DecoderTransfers(ControlBlock);
#else
                // Release both buffers if mp3 is not supported
                DeliverOutputBuffer(ControlBlock, TRUE);
                ReleaseInputBuffer(ControlBlock, TRUE);
#endif
			}
			else
			{
				ConfigurePCMByPass(ControlBlock);
			}
            break;
        case DECODER_DUAL:
			if(!ControlBlock->SingleDecoderInput)
			{
	            ConfigureAmphionDecoder(ControlBlock);
	            SetVolumeandStereoMode(ControlBlock);
			}
            StartDecoderTransfers(ControlBlock);
            WaitForDecoderTransferToComplete(ControlBlock);
            break;
        default:
            break;
    }
    return ST_NO_ERROR;

}

void ConfigureAmphionDecoder(DecoderControlBlock_t * ControlBlock)
{
    ResetAudioDecoder(ControlBlock);
}

ST_ErrorCode_t StartDecoderTransfers(DecoderControlBlock_t * ControlBlock)
{
   	ST_ErrorCode_t 	Error = ST_NO_ERROR;


    ControlBlock->InputTransferCompleted  = FALSE;
    ControlBlock->OutputTransferCompleted = FALSE;

    Error = STFDMA_StartGenericTransfer(&ControlBlock->OutputTransferParams, &ControlBlock->OutputTransferId);
    Error |= STFDMA_StartGenericTransfer(&ControlBlock->InputTransferParams, &ControlBlock->InputTransferId);

    return Error;

}

ST_ErrorCode_t WaitForDecoderTransferToComplete(DecoderControlBlock_t * ControlBlock)
{
    #if !ENABLE_DECODER_122
        STFDMA_TransferStatus_t OutputStatus;
        STFDMA_TransferStatus_t InputStatus;
        int TimeoutError;
        U32 DecStatus;
    #endif
    U32 TimeOut = ST_GetClocksPerSecond();
    STOS_SemaphoreWait(ControlBlock->FDMACallbackSem_p);
    TimeOut = time_now()+(TimeOut * 5 / 1000);

    if (CheckErrorConditions(ControlBlock) == TRUE)
    {
    // TBD
    // if there are errors in decoding or input transfer got completed
    // release all input and output buffers involved in this transfer including
    // back buffers
        if (ControlBlock->OutputTransferCompleted == FALSE)
        {
            STFDMA_AbortTransfer(ControlBlock->OutputTransferId);
        }
        DeliverOutputBuffer(ControlBlock, TRUE);
        ReleaseInputBuffer(ControlBlock, TRUE);


    }
    else
    {
        #if !ENABLE_DECODER_122
            // Now wait for output transfer to complete

            TimeoutError = STOS_SemaphoreWaitTimeOut(ControlBlock->FDMACallbackSem_p, (STOS_Clock_t *)&TimeOut);
            if(TimeoutError != 0)
            {
                if (ControlBlock->InputTransferCompleted == FALSE)
                {
                    STFDMA_GetTransferStatus(ControlBlock->InputTransferId, &InputStatus);
                    if(InputStatus.NodeBytesRemaining != 0)
                    {
                        STTBX_Print(("Aborting input Xfer %d\n", InputStatus.NodeBytesRemaining));
                        STFDMA_AbortTransfer(ControlBlock->InputTransferId);
                    }
                }

                if (ControlBlock->OutputTransferCompleted == FALSE)
                {
                    STFDMA_GetTransferStatus(ControlBlock->OutputTransferId, &OutputStatus);
                    if(OutputStatus.NodeBytesRemaining != 0)
                    {
                        DecStatus = STSYS_ReadAudioReg32(AUD_DEC_STATUS);
                        STTBX_Print(("Aborting output Xfer %d: bytes in PCM fifo %d: bytes in input fifo %d\n", OutputStatus.NodeBytesRemaining, (DecStatus>>28), ((DecStatus>>24)&0xF)));
                        STFDMA_AbortTransfer(ControlBlock->OutputTransferId);
                    }
                }

                DeliverOutputBuffer(ControlBlock, TRUE);
                ReleaseInputBuffer(ControlBlock, TRUE);

            }
            else
            {
                DeliverOutputBuffer(ControlBlock, FALSE);
                ReleaseInputBuffer(ControlBlock, FALSE);
            }

        #else
            // No error and we got output transfer completion callback
            // This abort is required for 122 implementation
            STFDMA_AbortTransfer(ControlBlock->InputTransferId);

            //STOS_SemaphoreWait(ControlBlock->FDMACallbackSem_p);
            DeliverOutputBuffer(ControlBlock, FALSE);
            ReleaseInputBuffer(ControlBlock, FALSE);
        #endif

    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t DeliverOutputBuffer(DecoderControlBlock_t * ControlBlock, BOOL Error)
{
	U32                             QueueFrontIndex;
	DecoderOutputBufferParam_t      * OutputBufferToSend;
	ST_ErrorCode_t                  Status = ST_NO_ERROR;

	QueueFrontIndex    = ControlBlock->DecoderOutputQueueFront%DECODER_QUEUE_SIZE;
	OutputBufferToSend = &ControlBlock->DecoderOutputQueue[QueueFrontIndex];

    if (ControlBlock->DecoderOutputQueueFront < ControlBlock->DecoderOutputQueueTail)
    {
	    if(ControlBlock->DecoderMode == DECODER_CODEC)
	    {
            #ifdef MP3_SUPPORT
	            OutputBufferToSend->OutputBlockFromProducer->Size = ControlBlock->MP3DecodertoPCMTransfer->NbSamples*4;
            #endif
		}
		else
		{
			OutputBufferToSend->OutputBlockFromProducer->Size = ControlBlock->DefaultDecodedSize;
		}
        if (Error == FALSE)
        {
            UpdateOutputBufferParams(ControlBlock);
            #if DUMP_ENABLE_OUTPUT
                memcpy((U8 *)DumpOutput, (U8 *)OutputBufferToSend->OutputBlockFromProducer->MemoryStart, OutputBufferToSend->OutputBlockFromProducer->Size);
                DumpOutput = DumpOutput+(OutputBufferToSend->OutputBlockFromProducer->Size / 4);
                if(((U8 *)DumpOutput - (U8 *)DumpOutputSaved) > 1024*1024*2)
                {
                printf("hi");
                STTBX_Print(("TAKING DUMP"));
                }
            #endif
            Status = MemPool_PutOpBlk(ControlBlock->OutMemoryBlockManagerHandle, (U32 *)&OutputBufferToSend->OutputBlockFromProducer);
        }
        else
        {
            Status = MemPool_ReturnOpBlk(ControlBlock->OutMemoryBlockManagerHandle, (U32 *)&OutputBufferToSend->OutputBlockFromProducer);
        }

        ControlBlock->DecoderOutputQueueFront++;
    }
    return Status;
}
ST_ErrorCode_t ReleaseInputBuffer(DecoderControlBlock_t * ControlBlock, BOOL Error)
{
	U32 QueueFrontIndex                          = 0;
	DecoderInputBufferParam_t *InputBufferToFree = NULL;
	ST_ErrorCode_t Status                        = ST_NO_ERROR;

	QueueFrontIndex   = ControlBlock->DecoderInputQueueFront%DECODER_QUEUE_SIZE;
	InputBufferToFree = &ControlBlock->DecoderInputQueue[QueueFrontIndex];

    if (ControlBlock->DecoderInputQueueFront < ControlBlock->DecoderInputQueueTail)
    {
        #if DUMP_ENABLE_INPUT
            memcpy((U8 *)DumpInput, (U8 *)InputBufferToFree->InputBlockFromProducer->MemoryStart, InputBufferToFree->InputBlockFromProducer->Size);
            DumpInput = DumpInput+(InputBufferToFree->InputBlockFromProducer->Size / 4);
            if(((U8 *)DumpInput - (U8 *)DumpInputSaved) > 900*1000)
            {
                printf("hi");
                STTBX_Print(("TAKING DUMP"));
            }
        #endif
        Status = MemPool_FreeIpBlk(ControlBlock->InMemoryBlockManagerHandle, (U32 *)&(InputBufferToFree->InputBlockFromProducer), ControlBlock->Handle);
        ControlBlock->DecoderInputQueueFront++;
    }

    return Status;
}

BOOL CheckErrorConditions(DecoderControlBlock_t * ControlBlock)
{
    BOOL Error    = FALSE;
    U32 DecStatus = STSYS_ReadAudioReg32(AUD_DEC_STATUS);

    if(
            #if ENABLE_DECODER_122
                (ControlBlock->InputTransferCompleted == TRUE) ||
            #endif
        (((DecStatus & AUD_FRAME_LOCKED) >> 2) == 0x0) || (((DecStatus & AUD_CRC_ERROR) >> 3)== 0x1))
    {
        Error = TRUE;
    }

    return Error;
}

void UpdateOutputBufferParams(DecoderControlBlock_t * ControlBlock)
{
	U32 QueueFrontIndex, InputQueueFrontIndex, DecoderStatus;
	DecoderOutputBufferParam_t * OutputBufferToSend;
	DecoderInputBufferParam_t * InputBufferDecoded;
	U32 TempSamplingFrequency     = 0;
	BOOL DeliverSamplingFrequency = FALSE;

	QueueFrontIndex      = ControlBlock->DecoderOutputQueueFront%DECODER_QUEUE_SIZE;
	OutputBufferToSend   = &ControlBlock->DecoderOutputQueue[QueueFrontIndex];
	InputQueueFrontIndex = (ControlBlock->DecoderInputQueueFront + ControlBlock->BackBufferCount)%DECODER_QUEUE_SIZE;
	InputBufferDecoded   = &ControlBlock->DecoderInputQueue[InputQueueFrontIndex];

    if (ControlBlock->DecoderOutputQueueFront < ControlBlock->DecoderOutputQueueTail)
    {

        if(ControlBlock->HoldStereoMode.Apply)
        {
            UpdateStereoOutputBuffer(ControlBlock, OutputBufferToSend->OutputBlockFromProducer);
        }

        DecoderStatus = STSYS_ReadAudioReg32(AUD_DEC_STATUS);

        if (DecoderStatus & AUD_FRAME_LOCKED)
        {
            switch ((DecoderStatus & AUD_SAMPFREQ_INDEX) >> 14)
            {
                case 0:
                    TempSamplingFrequency    = 44100;
                    DeliverSamplingFrequency = TRUE;
                    break;
                case 1:
                    TempSamplingFrequency    = 48000;
                    DeliverSamplingFrequency = TRUE;
                    break;
                case 2:
                    TempSamplingFrequency    = 32000;
                    DeliverSamplingFrequency = TRUE;
                    break;
                case 3:
                default:
                    TempSamplingFrequency    = 0xff;
                    break;
            }

            switch (DecoderStatus & AUD_VERSION_ID)
            {
                case 0:
                    TempSamplingFrequency = TempSamplingFrequency/2;
                    break;
                default:
                    break;
            }

            ControlBlock->OutputEmphasis    = (DecoderStatus & AUD_EMPHASIS) >> 5;
            ControlBlock->OutputCodingMode  = (DecoderStatus & AUD_AUDIO_MODE) >> 16;
            ControlBlock->OutputLayerInfo   = (DecoderStatus & AUD_LAYER_ID) >> 8;
        }

        if ((ControlBlock->SamplingFrequency != TempSamplingFrequency) && (DeliverSamplingFrequency == TRUE))
        {
            ControlBlock->SamplingFrequency = TempSamplingFrequency;
            OutputBufferToSend->OutputBlockFromProducer->Flags             |= FREQ_VALID;
            OutputBufferToSend->OutputBlockFromProducer->Data[FREQ_OFFSET] = TempSamplingFrequency;
        }

        if (InputBufferDecoded->InputBlockFromProducer->Flags & PTS_VALID)
        {
            OutputBufferToSend->OutputBlockFromProducer->Flags             |= PTS_VALID;
            OutputBufferToSend->OutputBlockFromProducer->Data[PTS_OFFSET]  = InputBufferDecoded->InputBlockFromProducer->Data[PTS_OFFSET];
            OutputBufferToSend->OutputBlockFromProducer->Data[PTS33_OFFSET]= InputBufferDecoded->InputBlockFromProducer->Data[PTS33_OFFSET];

        }

        if (InputBufferDecoded->InputBlockFromProducer->Flags & FADE_VALID)
        {
            OutputBufferToSend->OutputBlockFromProducer->Flags            |= FADE_VALID;
            OutputBufferToSend->OutputBlockFromProducer->Data[FADE_OFFSET]= InputBufferDecoded->InputBlockFromProducer->Data[FADE_OFFSET];
        }



    }

}
ST_ErrorCode_t 	STAUD_GetAmphionAttenuation(STAud_DecHandle_t Handle, STAUD_Attenuation_t  *Attenuation_p)
{
	ST_ErrorCode_t error               = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);

	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}
	/* Get the params here */
	Attenuation_p->Left          = Dec_p->DecoderControlBlock.Attenuation_p.Left;
	Attenuation_p->Right         = Dec_p->DecoderControlBlock.Attenuation_p.Right;
	Attenuation_p->Center        = Dec_p->DecoderControlBlock.Attenuation_p.Center;
	Attenuation_p->Subwoofer     = Dec_p->DecoderControlBlock.Attenuation_p.Subwoofer ;
	Attenuation_p->LeftSurround  = Dec_p->DecoderControlBlock.Attenuation_p.LeftSurround;
	Attenuation_p->RightSurround = Dec_p->DecoderControlBlock.Attenuation_p.RightSurround;

	return error;
}

ST_ErrorCode_t STAUD_SetAmphionAttenuation(STAud_DecHandle_t		Handle, STAUD_Attenuation_t  *Attenuation_p)
{
	ST_ErrorCode_t error               = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);

	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	if(Dec_p->DecoderControlBlock.DecoderMode == DECODER_CODEC)
	{
        #ifdef MP3_SUPPORT
    		Dec_p->DecoderControlBlock.mp3Config_p->mp3_left_vol 	=  Attenuation_p->Left;  //-6dB
    		Dec_p->DecoderControlBlock.mp3Config_p->mp3_right_vol 	=  Attenuation_p->Right; //-3db
        #endif
	}
	else
	{
		Dec_p->DecoderControlBlock.Attenuation_p.Left           = Attenuation_p->Left;
		Dec_p->DecoderControlBlock.Attenuation_p.Right          = Attenuation_p->Right;
		Dec_p->DecoderControlBlock.Attenuation_p.Center         = Attenuation_p->Center;
		Dec_p->DecoderControlBlock.Attenuation_p.Subwoofer      = Attenuation_p->Subwoofer;
		Dec_p->DecoderControlBlock.Attenuation_p.LeftSurround   = Attenuation_p->LeftSurround;
		Dec_p->DecoderControlBlock.Attenuation_p.RightSurround  = Attenuation_p->RightSurround;
	}

	if(Dec_p->DecoderControlBlock.SingleDecoderInput)
	{
		SetVolumeandStereoMode(&Dec_p->DecoderControlBlock);
	}
    return error;
}
ST_ErrorCode_t STAUD_SetAmphionPan(DecoderControlBlock_t * ControlBlock)
{
	ST_ErrorCode_t error = ST_NO_ERROR;
    U32 Pan, PPan;

    PPan = ControlBlock->ADControl.TargetPan;
	Pan  = ControlBlock->ADControl.CurPan;
	ControlBlock->Attenuation_p.Left  = ControlBlock->Attenuation_p.Left  - PanTable[PPan][0];
	ControlBlock->Attenuation_p.Right = ControlBlock->Attenuation_p.Right - PanTable[PPan][1];
	ControlBlock->Attenuation_p.Left  = PanTable[Pan][0] + ControlBlock->Attenuation_p.Left;
	ControlBlock->Attenuation_p.Right = PanTable[Pan][1] + ControlBlock->Attenuation_p.Right;
    if(ControlBlock->Attenuation_p.Left > 63)
    {
        ControlBlock->Attenuation_p.Left = 63;
    }
    if(ControlBlock->Attenuation_p.Right > 63)
    {
        ControlBlock->Attenuation_p.Right = 63;
    }
    return error;
}

ST_ErrorCode_t STAUD_SetAmphionStereoMode(STAud_DecHandle_t		Handle, STAUD_StereoMode_t	StereoMode)
{
	ST_ErrorCode_t error               = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);

	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

    switch (StereoMode)
    {
        case STAUD_STEREO_MODE_DUAL_LEFT:
            Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode   = STAUD_STEREO_MODE_DUAL_LEFT;
            break;

        case STAUD_STEREO_MODE_DUAL_RIGHT:
            Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode   = STAUD_STEREO_MODE_DUAL_RIGHT;
            break;

		case STAUD_STEREO_MODE_STEREO:
            Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode   = STAUD_STEREO_MODE_STEREO;
            break;

		case STAUD_STEREO_MODE_LR_MIX:
            Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode   = STAUD_STEREO_MODE_LR_MIX;
            break;
        case STAUD_STEREO_MODE_MONO:
        case STAUD_STEREO_MODE_DUAL_MONO:
        case STAUD_STEREO_MODE_SECOND_STEREO:
			  Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode = STAUD_STEREO_MODE_STEREO;
			  break;

		case STAUD_STEREO_MODE_PROLOGIC:
		default:
			Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode   = STAUD_STEREO_MODE_STEREO;
			error = ST_ERROR_FEATURE_NOT_SUPPORTED;
        break;

    }

	Dec_p->DecoderControlBlock.HoldStereoMode.Apply                = TRUE;

    return error;
}

ST_ErrorCode_t STAUD_GetAmphionStereoMode(STAud_DecHandle_t		Handle, STAUD_StereoMode_t	*StereoMode_p)
{
	ST_ErrorCode_t error               = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);

	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}
    *StereoMode_p = Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode;
    return error;
}

void STAUD_SetAttenuationAtAmphion(U16 Left, U16 Right)
{
	U32 VolumeControl;
	VolumeControl =  STSYS_ReadAudioReg32(AUD_DEC_VOLUME_CONTROL);
	VolumeControl = VolumeControl & 0xFFFF0000; /*Preserve all the bit except volume*/
	//STTBX_Print(( " set left=[%x], right=[%x]\n",Left, Right ));
	if (Left > 63)
	{
		Left = 63;
    }
	if (Right > 63)
    {
		Right = 63;
    }
	Right = Right<<8;
	VolumeControl += (U32)(Left + Right);
	//STTBX_Print(( " set_VolumeControl_to_reg=[%x]\n",VolumeControl ));
	STSYS_WriteAudioReg32(AUD_DEC_VOLUME_CONTROL, VolumeControl);

}
void  STAUD_SetStereoOutputAtAmphion (STAUD_StereoMode_t StereoMode)
{
	BOOL MixBit16 = FALSE;
	U32 VolumeControl;
	switch(StereoMode)
	{
		 case STAUD_STEREO_MODE_DUAL_LEFT:
            MixBit16 = FALSE;
            break;
        case STAUD_STEREO_MODE_DUAL_RIGHT:
            MixBit16 = FALSE;
            break;
		case STAUD_STEREO_MODE_STEREO:
            MixBit16 = FALSE;
            break;
		case STAUD_STEREO_MODE_LR_MIX:
            MixBit16 = TRUE;
            break;
		default:
			MixBit16 = FALSE;
			break;
	}
	VolumeControl = STSYS_ReadAudioReg32(AUD_DEC_VOLUME_CONTROL);
	if(MixBit16)
	{
		VolumeControl = VolumeControl | 0x10000;
    }
	else
	{
		VolumeControl = VolumeControl & 0xFFFEFFFF;
    }
	STSYS_WriteAudioReg32(AUD_DEC_VOLUME_CONTROL, VolumeControl);

}
void UpdateStereoOutputBuffer(DecoderControlBlock_t * ControlBlock, MemoryBlock_t	*OutputBlockFromProducer)
{
	U16 *src, *dst;
	U32 count, numcount;

	numcount = ControlBlock->DefaultDecodedSize>>2;

    switch (ControlBlock->HoldStereoMode.StereoMode)
	{
    	case STAUD_STEREO_MODE_DUAL_LEFT:
    		src = (U16 *)OutputBlockFromProducer->MemoryStart;
    		dst = src + 1;
    		break;
    	case STAUD_STEREO_MODE_DUAL_RIGHT:
    		dst = (U16 *)OutputBlockFromProducer->MemoryStart;
    		src = dst + 1;
    		break;
    	default:
    		numcount = 0;
    		break;
	}
	for(count = 0;count < numcount; count++)
	{
		*dst = *src;
		src  += 2;
		dst  += 2;
	}

}

#ifdef MP3_SUPPORT
void ConfigureMP3Decoder(DecoderControlBlock_t * ControlBlock)
{
    ConfigureEStoMP3DecoderInputParameter(ControlBlock);
    ConfigureMP3DecodertoPCMOutputParameter(ControlBlock);
    ConfigureDecoderTransferParameter(ControlBlock);
}

enum eErrorCode StartMP3DecoderTransfers(DecoderControlBlock_t * ControlBlock)
{
    enum eErrorCode Error;
	Error = ACF_ST20_AudioCodec(ControlBlock->DecoderTransfer);
	if(Error == ACF_ST20_EC_OK)
	{
		ControlBlock->MP3DecodertoPCMTransfer->NbSamples = Mp3SampleMap[ControlBlock->DecoderTransfer->PcmOut->SamplingFreq];
        DeliverOutputBuffer(ControlBlock, FALSE);
        ReleaseInputBuffer(ControlBlock, FALSE);
	}
	else
	{
        DeliverOutputBuffer(ControlBlock, TRUE);
        ReleaseInputBuffer(ControlBlock, TRUE);
	}
    return Error;

}

ST_ErrorCode_t ConfigureEStoMP3DecoderInputParameter(DecoderControlBlock_t * ControlBlock)
{
	DecoderInputBufferParam_t *CurrentBufferToFill;

	CurrentBufferToFill                             = &ControlBlock->DecoderInputQueue[ControlBlock->DecoderInputQueueFront%DECODER_QUEUE_SIZE];

    ControlBlock->EStoMP3DecoderTransfer->DataPtr   = (void *)CurrentBufferToFill->InputBlockFromProducer->MemoryStart;
	ControlBlock->EStoMP3DecoderTransfer->FrameSize	= CurrentBufferToFill->InputBlockFromProducer->Size;
    return ST_NO_ERROR;
}
ST_ErrorCode_t ConfigureMP3DecodertoPCMOutputParameter(DecoderControlBlock_t * ControlBlock)
{

	DecoderOutputBufferParam_t *NextOutputBuffer;

    NextOutputBuffer = &(ControlBlock->DecoderOutputQueue[ControlBlock->DecoderOutputQueueFront%DECODER_QUEUE_SIZE]);

	ControlBlock->MP3DecodertoPCMTransfer->SamplesPtr[0] = (tSample * )((char *)NextOutputBuffer->OutputBlockFromProducer->MemoryStart);
	ControlBlock->MP3DecodertoPCMTransfer->SamplesPtr[1] = (tSample * )((char *)NextOutputBuffer->OutputBlockFromProducer->MemoryStart+2);
	return ST_NO_ERROR;
}

ST_ErrorCode_t ConfigureDecoderTransferParameter(DecoderControlBlock_t * ControlBlock)
{

	if(ControlBlock->streamingTransformerFirstTransfer)
	{
		ControlBlock->DecoderTransfer->FirstTime		= TRUE;
		ControlBlock->streamingTransformerFirstTransfer	= 0;
	}
	else
	{
		ControlBlock->DecoderTransfer->FirstTime		=	FALSE;
	}
	return ST_NO_ERROR;
}

ST_ErrorCode_t EStoMP3DecoderTransferParameter(DecoderControlBlock_t * ControlBlock)
{
	ControlBlock->EStoMP3DecoderTransfer->AudioMode    = 2;
	ControlBlock->EStoMP3DecoderTransfer->NbSamples	   = 0; // defaul sample of mp3
	ControlBlock->EStoMP3DecoderTransfer->PTS		   = 0;
	ControlBlock->EStoMP3DecoderTransfer->PTSflag	   = 0;
	ControlBlock->EStoMP3DecoderTransfer->SamplingFreq = ACF_ST20_FS44k; // default Fs is 44.1 Khz for MP3
	ControlBlock->EStoMP3DecoderTransfer->StreamType   = ACF_ST20_MP3;  // Set stream type as Mp3 stream;.
    return ST_NO_ERROR;
}
ST_ErrorCode_t MP3DecodertoPCMTransferParameter(DecoderControlBlock_t * ControlBlock)
{

	ControlBlock->MP3DecodertoPCMTransfer->NbSamples	   = 0;//ACF_ST20_MP3_NbSample;
	ControlBlock->MP3DecodertoPCMTransfer->NbChannels	   = 2;
	ControlBlock->MP3DecodertoPCMTransfer->Offset		   = 2; // Interlaced output
	ControlBlock->MP3DecodertoPCMTransfer->SamplingFreq	   = ACF_ST20_FS48k;
	ControlBlock->MP3DecodertoPCMTransfer->WordSize		   = 0;
	ControlBlock->MP3DecodertoPCMTransfer->AudioMode	   = 2;
	ControlBlock->MP3DecodertoPCMTransfer->ChannelExist[0] = 1;
	ControlBlock->MP3DecodertoPCMTransfer->ChannelExist[1] = 1;
	ControlBlock->MP3DecodertoPCMTransfer->BufferSize	   = 1152;
	ControlBlock->MP3DecodertoPCMTransfer->PTSflag		   = 0;
	ControlBlock->MP3DecodertoPCMTransfer->PTS			   = 0;
	return ST_NO_ERROR;
}

ST_ErrorCode_t DecoderTransferParameter(DecoderControlBlock_t * ControlBlock)
{

	ControlBlock->DecoderTransfer->DecoderSelection	= ACF_ST20_MP3;
	ControlBlock->DecoderTransfer->SkipMuteCmd		= 0;
	ControlBlock->DecoderTransfer->CrcOff			= 0;
	ControlBlock->DecoderTransfer->RemainingBlocks	= 0;
	ControlBlock->DecoderTransfer->OutputPcmEnable	= ACF_ST20_TRUE;
	ControlBlock->DecoderTransfer->NbNewOutSamples	= 0;
	ControlBlock->DecoderTransfer->PcmProcessCount	= 0;
	return ST_NO_ERROR;
}
#endif
ST_ErrorCode_t ConfigurePCMByPass(DecoderControlBlock_t *ControlBlock_p)
{
	ST_ErrorCode_t status = ST_NO_ERROR;
	DecoderInputBufferParam_t *CurrentBufferToFill;
	DecoderOutputBufferParam_t *NextOutputBuffer;
	STAud_LpcmStreamParams_t	*StreamParams_p;

	CurrentBufferToFill = &ControlBlock_p->DecoderInputQueue[ControlBlock_p->DecoderInputQueueFront%DECODER_QUEUE_SIZE];

    NextOutputBuffer    = &(ControlBlock_p->DecoderOutputQueue[ControlBlock_p->DecoderOutputQueueFront%DECODER_QUEUE_SIZE]);

	StreamParams_p      = (STAud_LpcmStreamParams_t*)CurrentBufferToFill->InputBlockFromProducer->AppData_p;

	NextOutputBuffer->OutputBlockFromProducer->Flags                 |= BUFFER_DIRTY;
	NextOutputBuffer->OutputBlockFromProducer->Data[CONSUMER_HANDLE] = ControlBlock_p->InMemoryBlockManagerHandle;
	NextOutputBuffer->OutputBlockFromProducer->AppData_p             = CurrentBufferToFill->InputBlockFromProducer;
#if DUMP_ENABLE_OUTPUT
    memcpy((U8 *)DumpOutput, (U8 *)CurrentBufferToFill->InputBlockFromProducer->MemoryStart, CurrentBufferToFill->InputBlockFromProducer->Size);
    DumpOutput=DumpOutput+(CurrentBufferToFill->InputBlockFromProducer->Size/4);
    if(((U8 *)DumpOutput - (U8 *)DumpOutputSaved)>800*1024)
    {
    	printf("hi");
        STTBX_Print(("TAKING DUMP"));
    }
#endif

	status = MemPool_PutOpBlk(ControlBlock_p->OutMemoryBlockManagerHandle, (U32 *)&NextOutputBuffer->OutputBlockFromProducer);
    ControlBlock_p->DecoderOutputQueueFront++;
    ControlBlock_p->DecoderInputQueueFront++;

	return status;
}

ST_ErrorCode_t STAUD_SetDecodingType(STAud_DecHandle_t		Handle)
{
	ST_ErrorCode_t error               = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p = (DecoderControlBlockList_t*)NULL;
	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}
	Dec_p->DecoderControlBlock.SingleDecoderInput = TRUE;
    return error;
}

