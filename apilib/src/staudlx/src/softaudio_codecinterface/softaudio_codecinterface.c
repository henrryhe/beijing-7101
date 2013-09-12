/*
******************************************************************************
mme_wrapper.c
******************************************************************************
MME wrapper Layer
******************************************************************************
(C) Copyright STMicroelectronics - 2007
******************************************************************************
Owner : Audio Driver Noida Team
Revision History (Latest modification on top)
Contact Point : udit-dlh.kumar@st.com

------------------+-----------+-----------------------------------------------
  Date            			! Initials  ! Comments
------------------+-----------+-----------------------------------------------
07/06/07			UK				Started
------------------+-----------+-----------------------------------------------

------------------+-----------+-----------------------------------------------*/

//#define STTBX_PRINT 1

#include "softaudio_codecinterface.h"
#include "sttbx.h"
#include "string.h"
#ifndef ST_51XX
#include "malloc.h"
#else
#include "stdlib.h"
#endif
#include "stos.h"
/*Static data */
static MME_WrapperLayer_t *MMEWrapperHead;
static STOS_Mutex_t   *ProtectionLock;

/*Local Helper functions */
static void  AddTransformerToList(MME_WrapperLayer_t *Wrapper);
static void  RemoveTransformerFromList(MME_WrapperLayer_t *ItemToRemove );


/*To test this layer ,, with codec lib*/
//#define TEST_MME_WRAPPERLAYER
#ifdef TEST_MME_WRAPPERLAYER
static U8 TestHandle=0;
#endif

/******************************************************************************
 *  Function name 	: MME_Init
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: MME_Error
 *  Description   	: This function will not do anything, as on the ST40 based driver
 				   we are not using MME in real sense, MME wrapper layer is just to
 				   map MME Calls from Driver to Codec Multicom layer
 ***************************************************************************** */

MME_ERROR MME_Init(void)
{
	ProtectionLock=STOS_MutexCreateFifo();
 	return MME_SUCCESS;
}

/******************************************************************************
 *  Function name 	: MME_RegisterTransport
 *  Arguments    	:
 *       IN			: name of transport
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: MME_Error
 *  Description   	: This function will not do anything, as on the ST40 based driver
 				   we are not using MME in real sense, MME wrapper layer is just to
 				   map MME Calls from Driver to Codec Multicom layer
 ***************************************************************************** */

MME_ERROR MME_RegisterTransport(const char *name)
{
	return MME_SUCCESS;
}

/******************************************************************************
 *  Function name 	: MME_Term
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: MME_Error
 *  Description   	: This function will not do anything, as on the ST40 based driver
 				   we are not using MME in real sense, MME wrapper layer is just to
 				   map MME Calls from Driver to Codec Multicom layer
 ***************************************************************************** */
MME_ERROR MME_Term(void)
{
	STOS_MutexDelete(ProtectionLock);
	return MME_SUCCESS;
}


/******************************************************************************
 *  Function name 	: MME_GetTransformerCapability
 *  Arguments    	:
 *       IN			: Transformer Name
 *       OUT    		: Error status
 *       INOUT		: Transformer Capability
 *  Return       	: MME_Error
 *  Description   	: This function returns the transformer capability based on the name passed
 ***************************************************************************** */

MME_ERROR MME_GetTransformerCapability(const char *TransformerName,
					       MME_TransformerCapability_t * TransformerCapability_p)
{
	/*Check the Transformer type*/
	STTBX_Print((" MME Wrapper >> MME_GetTransformerCapability for %s \n", TransformerName));
	if(strcmp("MME_TRANSFORMER_TYPE_AUDIO_ENCODER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
	{
		MME_AudioEncoderInfo_t	    *EncoderInfo;
		if(sizeof(MME_AudioEncoderInfo_t)==TransformerCapability_p->TransformerInfoSize)
		{
			EncoderInfo = (MME_AudioEncoderInfo_t *)TransformerCapability_p->TransformerInfo_p;
			EncoderInfo->EncoderCapabilityFlags=0xFFFFFFFF;
			EncoderInfo->EncoderCapabilityExtFlags = 0xFFFFFFFF;
		}
	}
#else
		//return(audioencoder_GetTransformerCapability(TransformerCapability_p));
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_DECODER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		MME_LxAudioDecoderInfo_t *DecoderInfo;
		if(sizeof(MME_LxAudioDecoderInfo_t)==TransformerCapability_p->TransformerInfoSize)
		{
			DecoderInfo = (MME_LxAudioDecoderInfo_t *)TransformerCapability_p->TransformerInfo_p;
			DecoderInfo->DecoderCapabilityFlags =0xFFFFFFFF;
			DecoderInfo->DecoderCapabilityExtFlags[0] =0xFFFFFFFF;
			DecoderInfo->DecoderCapabilityExtFlags[1] =0xFFFFFFFF;
			DecoderInfo->PcmProcessorCapabilityFlags[0] =0xFFFFFFFF;
			DecoderInfo->PcmProcessorCapabilityFlags[1] =0xFFFFFFFF;
			DecoderInfo->StreamDecoderCapabilityFlags = 0; /*No Stream Decoder Supported*/
		}
#else
#ifndef ST_51XX
		return(audiodecoder_GetTransformerCapability(TransformerCapability_p));
#endif
#endif
	}
	else if (strcmp("AUDIO_DECODER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		MME_LxAudioDecoderInfo_t *DecoderInfo;
		if(sizeof(MME_LxAudioDecoderInfo_t)==TransformerCapability_p->TransformerInfoSize)
		{
			DecoderInfo = (MME_LxAudioDecoderInfo_t *)TransformerCapability_p->TransformerInfo_p;
			DecoderInfo->DecoderCapabilityFlags =0xFFFFFFFF;
			DecoderInfo->DecoderCapabilityExtFlags[0] =0xFFFFFFFF;
			DecoderInfo->DecoderCapabilityExtFlags[1] =0xFFFFFFFF;
			DecoderInfo->PcmProcessorCapabilityFlags[0] =0xFFFFFFFF;
			DecoderInfo->PcmProcessorCapabilityFlags[1] =0xFFFFFFFF;
			DecoderInfo->StreamDecoderCapabilityFlags = 0; /*No Stream Decoder Supported*/
		}
#else
#ifndef ST_51XX
		return(audiodecoder_GetTransformerCapability(TransformerCapability_p));
#endif
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_MIXER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		MME_LxMixerTransformerInfo_t *MixerInfo;
		if(sizeof(MME_LxMixerTransformerInfo_t)==TransformerCapability_p->TransformerInfoSize)
		{
			MixerInfo = (MME_LxMixerTransformerInfo_t *)TransformerCapability_p->TransformerInfo_p;
			MixerInfo->MixerCapabilityFlags=0xFFFFFFFF;
			MixerInfo->PcmProcessorCapabilityFlags[0] =0xFFFFFFFF;
			MixerInfo->PcmProcessorCapabilityFlags[1] =0xFFFFFFFF;
		}
#else
		return(mixer_GetTransformerCapability(TransformerCapability_p));
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_PCMPROCESSOR",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		MME_LxPcmProcessingInfo_t *PcmProcessingInfo;
		if(sizeof(MME_LxPcmProcessingInfo_t)==TransformerCapability_p->TransformerInfoSize)
		{
			PcmProcessingInfo = (MME_LxPcmProcessingInfo_t *)TransformerCapability_p->TransformerInfo_p;
		}
#else
#ifndef ST_51XX
		return(pcmprocessing_GetTransformerCapability(TransformerCapability_p));
#endif
#endif
	}
	else if (strcmp("PING_CPU_2",TransformerName)==0)
	{
		STTBX_Print(("MME Wrapper << MME_GetTransformerCapability \n"));
		return MME_SUCCESS;
	}
	else
	{
		STTBX_Print(("MME Wrapper << MME_GetTransformerCapability :: MME_UNKNOWN_TRANSFORMER \n"));
		return MME_UNKNOWN_TRANSFORMER;
	}

#ifdef TEST_MME_WRAPPERLAYER
	STTBX_Print(("MME Wrapper << MME_GetTransformerCapability \n"));
#endif
		return MME_SUCCESS;
}


/******************************************************************************
 *  Function name 	: MME_InitTransformer
 *  Arguments    	:
 *       IN			: Transformer Name , Init Params,
 *       OUT    		: Error status
 *       INOUT		: Transformer handle
 *  Return       	: MME_Error
 *  Description   	: This function create one transformer, and returns the handle for that transformer
 *****************************************************************************
 */


MME_ERROR MME_InitTransformer(const char *TransformerName, MME_TransformerInitParams_t * Params_p, MME_TransformerHandle_t * Handle_p)
{
	MME_WrapperLayer_t *WrapperLayer=NULL;
	MME_Error_t MME_Error=MME_SUCCESS;

	STTBX_Print(("MME Wrapper >>  MME_InitTransformer %s \n",TransformerName));
	WrapperLayer=malloc(sizeof(MME_WrapperLayer_t));

	if(WrapperLayer==NULL)
	{
		STTBX_Print(("MME Wrapper <<  MME_InitTransformer :: MME_NO_MEMORY \n"));
		return MME_NO_MEMORY;
	}

	if(strcmp("MME_TRANSFORMER_TYPE_AUDIO_ENCODER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print((" Test MME_InitTransformer for Encoder \n"));
#else
//		MME_Error=audioencoder_InitTransformer(Params_p->TransformerInitParamsSize, Params_p->TransformerInitParams_p,(void **)Handle_p);
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_DECODER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print((" Test MME_InitTransformer for Decoder \n"));
#else
#ifndef ST_51XX
		MME_Error=audiodecoder_InitTransformer(Params_p->TransformerInitParamsSize, Params_p->TransformerInitParams_p,(void **)Handle_p);
#endif
#endif
	}
	else if (strcmp("AUDIO_DECODER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print((" Test MME_InitTransformer for Decoder \n"));
#else
#ifndef ST_51XX
		MME_Error=audiodecoder_InitTransformer(Params_p->TransformerInitParamsSize, Params_p->TransformerInitParams_p,(void **)Handle_p);
#endif
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_MIXER",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
	     STTBX_Print((" Test MME_InitTransformer for Mixer \n"));
#else

		MME_Error=mixer_InitTransformer(Params_p->TransformerInitParamsSize, Params_p->TransformerInitParams_p,(void **)Handle_p);
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_PCMPROCESSOR",TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print((" Test MME_InitTransformer for pcm process \n"));
#else
#ifndef ST_51XX
		MME_Error=pcmprocessing_InitTransformer(Params_p->TransformerInitParamsSize, Params_p->TransformerInitParams_p,(void **)Handle_p);
#endif
#endif
	}
	else
	{
		free(WrapperLayer);
		STTBX_Print(("MME Wrapper <<  MME_InitTransformer :: MME_UNKNOWN_TRANSFORMER\n"));
		return MME_UNKNOWN_TRANSFORMER;
	}
	if(MME_Error==MME_SUCCESS)
	{
		/*Store the name in transformer wrapper struct*/
		strcpy(WrapperLayer->TransformerName,TransformerName);
#ifdef  TEST_MME_WRAPPERLAYER
		TestHandle++;
		*Handle_p = TestHandle;
#endif
		WrapperLayer->TransformerHandle= *Handle_p;
		WrapperLayer->UserData       = Params_p->CallbackUserData;
		WrapperLayer->UserCallbackFunction = Params_p->Callback;
		*Handle_p=(MME_TransformerHandle_t ) WrapperLayer;
		AddTransformerToList(WrapperLayer);
	}
	else
	{

		free(WrapperLayer);
	}
	STTBX_Print(("MME Wrapper <<  MME_InitTransformer :: %d \n",MME_Error));
	return MME_Error;
}


/******************************************************************************
 *  Function name 	: AddTransformerToList
 *  Arguments    	:
 *       IN			: MME_Wrapper Layer
 *       OUT    		:
 *       INOUT		:
 *  Return       	:
 *  Description   	: This function is used to maintain linked list of open/close transformers
 *****************************************************************************
 */

static void AddTransformerToList(MME_WrapperLayer_t *Wrapper)
{
	MME_WrapperLayer_t *TempWrapper;
	STOS_MutexLock(ProtectionLock);
	TempWrapper = MMEWrapperHead;
	if(MMEWrapperHead==NULL)
	{
		MMEWrapperHead=Wrapper;
		MMEWrapperHead->Next = NULL;
	}
	else
	{
		while(TempWrapper->Next!=NULL)
		{
			TempWrapper=TempWrapper->Next;
		}
		TempWrapper->Next = Wrapper;
		Wrapper->Next = NULL;
	}
	STOS_MutexRelease(ProtectionLock);
}

/******************************************************************************
 *  Function name 	: AddTransformerToList
 *  Arguments    	:
 *       IN			: MME_Wrapper Layer
 *       OUT    		:
 *       INOUT		:
 *  Return       	:
 *  Description   	: This function is used to maintain linked list of open/close transformers
 *****************************************************************************
 */

static void  RemoveTransformerFromList(MME_WrapperLayer_t *ItemToRemove )
{
	MME_WrapperLayer_t *Temp=NULL;
	MME_WrapperLayer_t *Prev=NULL;
	STOS_MutexLock(ProtectionLock);
	Temp = MMEWrapperHead;
	while(Temp!=NULL)
	{
		if(Temp->TransformerHandle==ItemToRemove->TransformerHandle)
		{
			if(MMEWrapperHead==ItemToRemove)
			{
				MMEWrapperHead=ItemToRemove->Next;
				free(ItemToRemove);
				STOS_MutexRelease(ProtectionLock);
				return;
			}
			else
			{
				Prev->Next = Temp->Next;
				free(ItemToRemove);
				STOS_MutexRelease(ProtectionLock);
				return;
			}
		}
		else
		{
			Prev=Temp;
			Temp=Temp->Next;
		}
	}
	STOS_MutexRelease(ProtectionLock);
}

/******************************************************************************
 *  Function name 	: MME_SendCommand
 *  Arguments    	:
 *       IN			: Transformer Handle
 *       OUT    		:
 *       INOUT		: CmdInfo_p
 *  Return       	: MME_Erro
 *  Description   	: This function is used to call codec function for decoding and will call the
				    callback function registered by driver
 *****************************************************************************
 */
MME_ERROR MME_SendCommand(MME_TransformerHandle_t Handle, MME_Command_t * CmdInfo_p)
{

	MME_WrapperLayer_t *MMEWrapperLayer = (MME_WrapperLayer_t *) Handle;
	MME_Error_t MME_Error=MME_SUCCESS;

	if(MMEWrapperLayer==NULL)
	{
		STTBX_Print(("MME Wrapper << MME_SendCommand :: MME_INVALID_HANDLE \n"));
		return MME_INVALID_HANDLE;
	}
	if(strcmp("MME_TRANSFORMER_TYPE_AUDIO_ENCODER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		if(CmdInfo_p->CmdCode==MME_TRANSFORM)
		{

			MME_DataBuffer_t  *InputBuffer;
			MME_DataBuffer_t  *OutBuffer;
			if(CmdInfo_p->NumberInputBuffers==1 && CmdInfo_p->NumberOutputBuffers==1)
			{
				MME_ScatterPage_t *InputScatterPage;
				MME_ScatterPage_t *OutputScatterPage;
				InputBuffer = CmdInfo_p->DataBuffers_p[0];
				InputScatterPage = InputBuffer->ScatterPages_p;
				OutBuffer  = CmdInfo_p->DataBuffers_p[1];
				OutputScatterPage = OutBuffer->ScatterPages_p;
				OutputScatterPage->BytesUsed = 1152;
			}
		}
#else
	//	MME_Error=audioencoder_ProcessCommand(MMEWrapperLayer->TransformerHandle,CmdInfo_p);
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_DECODER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		if(CmdInfo_p->CmdCode==MME_TRANSFORM)
		{

			MME_DataBuffer_t  *InputBuffer;
			MME_DataBuffer_t  *OutBuffer;
			if(CmdInfo_p->NumberInputBuffers==1 && CmdInfo_p->NumberOutputBuffers==1)
			{
				MME_ScatterPage_t *InputScatterPage;
				MME_ScatterPage_t *OutputScatterPage;
				InputBuffer = CmdInfo_p->DataBuffers_p[0];
				InputScatterPage = InputBuffer->ScatterPages_p;
				OutBuffer  = CmdInfo_p->DataBuffers_p[1];
				OutputScatterPage = OutBuffer->ScatterPages_p;
				OutputScatterPage->BytesUsed = 1152*4*2; /*Assumg MPEG and Number of channel 2*/
				{
					/*Update Comand Field too*/
					MME_CommandStatus_t *CommandStatus= &CmdInfo_p->CmdStatus;
					CommandStatus->Error = MME_SUCCESS;
					CommandStatus->State = MME_COMMAND_COMPLETED;
					{
						MME_LxAudioDecoderFrameStatus_t *Status;
						Status = (MME_LxAudioDecoderFrameStatus_t *)CommandStatus->AdditionalInfo_p;
						Status->NbOutSamples = 1152;
					}
				}
			}
		}
#else
#ifndef ST_51XX
		MME_Error=audiodecoder_ProcessCommand((void *)MMEWrapperLayer->TransformerHandle,CmdInfo_p);
#endif
#endif
	}
	else if (strcmp("AUDIO_DECODER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		/*Unpack MME command*/
		if(CmdInfo_p->CmdCode==MME_TRANSFORM)
		{

			MME_DataBuffer_t  *InputBuffer;
			MME_DataBuffer_t  *OutBuffer;
			if(CmdInfo_p->NumberInputBuffers==1 && CmdInfo_p->NumberOutputBuffers==1)
			{
				MME_ScatterPage_t *InputScatterPage;
				MME_ScatterPage_t *OutputScatterPage;
				InputBuffer = CmdInfo_p->DataBuffers_p[0];
				InputScatterPage = InputBuffer->ScatterPages_p;
				OutBuffer  = CmdInfo_p->DataBuffers_p[1];
				OutputScatterPage = OutBuffer->ScatterPages_p;
				OutputScatterPage->BytesUsed = 1152*4*2; /*Assumg MPEG and Number of channel 2*/
				{
					/*Update Comand Field too*/
					MME_CommandStatus_t *CommandStatus= &CmdInfo_p->CmdStatus;
					CommandStatus->Error = MME_SUCCESS;
					CommandStatus->State = MME_COMMAND_COMPLETED;
					{
						MME_LxAudioDecoderFrameStatus_t *Status;
						Status = (MME_LxAudioDecoderFrameStatus_t *)CommandStatus->AdditionalInfo_p;
						Status->NbOutSamples = 1152;
					}
				}
			}
		}
#else
#ifndef ST_51XX
		MME_Error=audiodecoder_ProcessCommand((void *)MMEWrapperLayer->TransformerHandle,CmdInfo_p);
#endif
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_MIXER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
	if(CmdInfo_p->CmdCode==MME_TRANSFORM)
		{

			MME_DataBuffer_t  *InputBuffer;
			MME_DataBuffer_t  *OutBuffer;
			if(CmdInfo_p->NumberInputBuffers==1 && CmdInfo_p->NumberOutputBuffers==1)
			{
				MME_ScatterPage_t *InputScatterPage;
				MME_ScatterPage_t *OutputScatterPage;
				InputBuffer = CmdInfo_p->DataBuffers_p[0];
				InputScatterPage = InputBuffer->ScatterPages_p;
				OutBuffer  = CmdInfo_p->DataBuffers_p[1];
				OutputScatterPage = OutBuffer->ScatterPages_p;
				OutputScatterPage->BytesUsed = InputScatterPage->Size;
				{
					/*Update Comand Field too*/
					MME_CommandStatus_t *CommandStatus= &CmdInfo_p->CmdStatus;
					CommandStatus->Error = MME_SUCCESS;
					CommandStatus->State = MME_COMMAND_COMPLETED;
					{
						MME_LxMixerTransformerFrameDecodeStatusParams_t *Status;
						Status = (MME_LxMixerTransformerFrameDecodeStatusParams_t *)CommandStatus->AdditionalInfo_p;
						Status->NbOutSamples = 1152;
					}
				}
			}
		}
#else
		MME_Error=mixer_ProcessCommand((void *)MMEWrapperLayer->TransformerHandle,CmdInfo_p);
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_PCMPROCESSOR",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
	if(CmdInfo_p->CmdCode==MME_TRANSFORM)
		{

			MME_DataBuffer_t  *InputBuffer;
			MME_DataBuffer_t  *OutBuffer;
			if(CmdInfo_p->NumberInputBuffers==1 && CmdInfo_p->NumberOutputBuffers==1)
			{
				MME_ScatterPage_t *InputScatterPage;
				MME_ScatterPage_t *OutputScatterPage;
				InputBuffer = CmdInfo_p->DataBuffers_p[0];
				InputScatterPage = InputBuffer->ScatterPages_p;
				OutBuffer  = CmdInfo_p->DataBuffers_p[1];
				OutputScatterPage = OutBuffer->ScatterPages_p;
				OutputScatterPage->BytesUsed = InputScatterPage->Size;
				{
					/*Update Comand Field too*/
					MME_CommandStatus_t *CommandStatus= &CmdInfo_p->CmdStatus;
					CommandStatus->Error = MME_SUCCESS;
					CommandStatus->State = MME_COMMAND_COMPLETED;
					{
						MME_PcmProcessingFrameStatus_t *Status;
						Status = (MME_PcmProcessingFrameStatus_t *)CommandStatus->AdditionalInfo_p;
						Status->NbOutSamples = 1152;
					}
				}
			}
		}
#else
#ifndef ST_51XX
		MME_Error=pcmprocessing_ProcessCommand((void *)MMEWrapperLayer->TransformerHandle,CmdInfo_p);
#endif
#endif
	}
	else
	{
		STTBX_Print(("MME Wrapper << MME_SendCommand :: MME_INVALID_HANDLE \n"));
		MME_Error=MME_INVALID_HANDLE;
	}
	if(MME_Error==MME_SUCCESS)
	{
		if(CmdInfo_p->CmdEnd==MME_COMMAND_END_RETURN_NOTIFY)
		{
			MMEWrapperLayer->UserCallbackFunction(MME_COMMAND_COMPLETED_EVT,CmdInfo_p,MMEWrapperLayer->UserData);
		}
	}
	return MME_Error;

}

/******************************************************************************
 *  Function name 	: MME_AbortCommand
 *  Arguments    	:
 *       IN			: Transformer Handle
 *       OUT    		: CmdID
 *       INOUT		: CmdInfo_p
 *  Return       	: MME_Error
 *  Description   	: This function is used to call codec function for aborting . This will be
 				   applicable only for streaming based transformers.
 *****************************************************************************
 */

MME_ERROR MME_AbortCommand(MME_TransformerHandle_t Handle, MME_CommandId_t CmdId)
{
	MME_Error_t MME_Error=MME_SUCCESS;
	MME_WrapperLayer_t *MMEWrapperLayer = (MME_WrapperLayer_t *) Handle;

	STTBX_Print(("MME Wrapper >> MME_AbortCommand  \n"));

	if(MMEWrapperLayer==NULL)
	{
		STTBX_Print(("MME Wrapper << MME_AbortCommand :: MME_INVALID_HANDLE \n"));
		return MME_INVALID_HANDLE;
	}

	if(strcmp("MME_TRANSFORMER_TYPE_AUDIO_ENCODER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print(("Test MME_AbortCommand for Encoder cmd Id %d \n", CmdId));
#else
//		MME_Error=audioencoder_AbortCommand((void *)MMEWrapperLayer->TransformerHandle,CmdId);
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_DECODER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print(("Test MME_AbortCommand for Decoder cmd Id %d \n", CmdId));
#else
#ifndef ST_51XX
		MME_Error=audiodecoder_AbortCommand((void *)MMEWrapperLayer->TransformerHandle,CmdId);
#endif
#endif
	}
	else if (strcmp("AUDIO_DECODER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print(("Test MME_AbortCommand for Decoder cmd Id %d \n", CmdId));
#else
#ifndef ST_51XX
		MME_Error=audiodecoder_AbortCommand((void *)MMEWrapperLayer->TransformerHandle,CmdId);
#endif
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_MIXER",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print(("Test MME_AbortCommand for Mixer cmd Id %d \n", CmdId));
#else
		MME_Error=mixer_AbortCommand((void *)MMEWrapperLayer->TransformerHandle,CmdId);
#endif
	}
	else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_PCMPROCESSOR",MMEWrapperLayer->TransformerName)==0)
	{
#ifdef TEST_MME_WRAPPERLAYER
		STTBX_Print(("Test MME_AbortCommand for Pcm Process cmd Id %d \n", CmdId));
#else
#ifndef ST_51XX
		MME_Error=pcmprocessing_AbortCommand((void *)MMEWrapperLayer->TransformerHandle,CmdId);
#endif
#endif
	}
	else
	{
		MME_Error = MME_INVALID_HANDLE;
	}
	STTBX_Print(("MME Wrapper << MME_AbortCommand :: %d \n", MME_Error));
	return MME_Error;
}

/******************************************************************************
 *  Function name 	: MME_TermTransformer
 *  Arguments    	:
 *       IN			: Transformer Handle
 *       OUT    		:
 *       INOUT		:
 *  Return       	: MME_Error
 *  Description   	: This function terminate one transformer
 *****************************************************************************
 */
MME_ERROR MME_TermTransformer(MME_TransformerHandle_t handle)
{
	// Get the Handle from Existing Unit
	MME_Error_t MME_Error=MME_SUCCESS;
	MME_WrapperLayer_t *WrapperLayer = (MME_WrapperLayer_t *)handle;
	MME_WrapperLayer_t *TempLayer;
	STOS_MutexLock(ProtectionLock);
	TempLayer=MMEWrapperHead;
	STOS_MutexRelease(ProtectionLock);

	STTBX_Print(("MME Wrapper >> MME_TermTransformer \n"));
	/*Search for the Valid*/
	while(TempLayer!=NULL)
	{
		if(TempLayer->TransformerHandle==WrapperLayer->TransformerHandle)
		{
			/*Got the correct Handle
			Now call the codec function*/
			if(strcmp("MME_TRANSFORMER_TYPE_AUDIO_ENCODER",TempLayer->TransformerName)==0)
			{
#ifdef TEST_MME_WRAPPERLAYER
				STTBX_Print(("Test MME Term Transformer for Encoder \n"));
#else
//				MME_Error=audioencoder_TermTransformer((void*)TempLayer->TransformerHandle);
#endif
			}
			else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_DECODER",TempLayer->TransformerName)==0)
			{
#ifdef TEST_MME_WRAPPERLAYER
				STTBX_Print(("Test MME Term Transformer for Decoder \n"));
#else
#ifndef ST_51XX
				MME_Error=audiodecoder_TermTransformer((void*)TempLayer->TransformerHandle);
#endif
#endif
			}
			else if (strcmp("AUDIO_DECODER",TempLayer->TransformerName)==0)
			{
#ifdef TEST_MME_WRAPPERLAYER
				STTBX_Print(("Test MME Term Transformer for Decoder \n"));
#else
#ifndef ST_51XX
				MME_Error=audiodecoder_TermTransformer((void*)TempLayer->TransformerHandle);
#endif
#endif
			}
			else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_MIXER",TempLayer->TransformerName)==0)
			{
#ifdef TEST_MME_WRAPPERLAYER
				STTBX_Print(("Test MME Term Transformer for Mixer  \n"));
#else

				MME_Error=mixer_TermTransformer((void*)TempLayer->TransformerHandle);
#endif
			}
			else if (strcmp("MME_TRANSFORMER_TYPE_AUDIO_PCMPROCESSOR",TempLayer->TransformerName)==0)
			{
#ifdef TEST_MME_WRAPPERLAYER
				STTBX_Print(("Test MME Term Transformer for Pcm Process \n"));
#else
#ifndef ST_51XX
				MME_Error=pcmprocessing_TermTransformer((void*)TempLayer->TransformerHandle);
#endif
#endif
			}

			RemoveTransformerFromList(TempLayer);
			STTBX_Print(("MME Wrapper << MME_TermTransformer :: %d \n", MME_Error));
			return MME_Error;
		}
		TempLayer = TempLayer->Next;
	}
	STTBX_Print(("MME Wrapper << MME_TermTransformer :: MME_INVALID_HANDLE \n"));
	return MME_INVALID_HANDLE;

}

/*Hack for Codec From Udit Kumar */
MME_ERROR MME_NotifyHost(MME_Event_t event, MME_Command_t * commandInfo, MME_ERROR errorCode)
{
	return MME_SUCCESS;
}

