/*
******************************************************************************
mixer_multicom.c
******************************************************************************
Mixer Multicom Control Layer
******************************************************************************
(C) Copyright STMicroelectronics - 2007
******************************************************************************
Owner : Audio Driver Noida Team
Revision History (Latest modification on top)
Contact Point : udit-dlh.kumar@st.com

------------------+-----------+-----------------------------------------------
  Date            ! Initials  ! Comments
------------------+-----------+-----------------------------------------------
27/06/07			SP				Started
------------------+-----------+-----------------------------------------------

------------------+-----------+-----------------------------------------------*/

#define DEBUG_FRAME_N 3
//#define STTBX_PRINT 1

#include "mixer_multicom.h"
#include "sttbx.h"
#include "string.h"
#ifndef ST_51XX
#include "malloc.h"
#else
#include "stdlib.h"
#endif
#include "stos.h"

extern MME_ERROR mixer_process ( tPcmProcessingInterface * pcm_if );

MME_ERROR mixer_GetTransformerCapability(MME_TransformerCapability_t* TransformerCaps_p)
{
	STTBX_Print(("Mixer GetTransformerCapability. Nothing to be Done !!\n"));
	return MME_SUCCESS;
}

#define MIXER_MME_ALLOWED_INPUT_JITTER 16
int mixer_alloc_pcmin(tPcmBuffer * pcmin, int nspl)
{
	int       ch;
	int       buf_size  = pcmin->BufferSize;
	int       mem_alloc = sizeof(int) * buf_size * pcmin->NbChannels;
	int     * buffer    = NULL;

	// Take some margin
	nspl += MIXER_MME_ALLOWED_INPUT_JITTER;

	if ((nspl > buf_size) && (buf_size != 0))
	{
		// Reallocate buffer
		free(pcmin->SamplesPtr[ACC_MAIN_LEFT]);
	}

	if (nspl > buf_size)
	{
		buf_size  = sizeof(int) * nspl * pcmin->NbChannels;
		buffer    = (int *) malloc(buf_size);
		mem_alloc = buf_size - mem_alloc; // only return the delta between previous and current allocation

		// if we allocate an input buffer then we will use the input stride for offset
		pcmin->BufferSize = nspl;
		pcmin->WordSize   = ACC_WS32;

		for (ch = 0;ch < pcmin->NbChannels;ch++)
		{
			pcmin->SamplesPtr[ch] = buffer;
			buffer++; // Interlaced buffer
		}
	}

	return (mem_alloc);
}

MME_ERROR      mixer_setConfig(MME_LxMixerMem_t   * mixer, MME_LxMixerInConfig_t * mixer_params,
                               enum eAccSetConfig   setConfig)
{
	MME_ERROR                  status = MME_SUCCESS;

	int                        mem_alloc = 0;
	int                        input_id, config_id;
	enum eMixerInputType       input_type;

	int                        mmeconfig_structsize;
	MME_MixerInputConfig_t  *  mmemixer_input_config;
	tPcmProcessingInterface *  mixer_if        = &mixer->PcmInterface;
	//tMMEMixerConfig         *  mmemixer_config = & mixer->Config;
	tMixerConfig            *  lxamixer_config = (tMixerConfig *) mixer->MixerConfig;
	tPcmBuffer              ** pcmin           = mixer_if->PcmIn;
	tPcmBuffer              *  pcmbuf;
	U32                        coeff, i;

	STTBX_Print(("\t mixer_setConfig(): %d input\n", mixer->Config.NbInput));

	if (mixer->Config.NbInput != 0)
	{
		lxamixer_config->Fade         = ACC_MME_FALSE;

		// Parse all input updates
		mmeconfig_structsize  = (mixer_params->StructSize - ((int)&mixer_params->Config - (int)mixer_params));
		mmemixer_input_config = mixer_params->Config;
		config_id = 0;

		while(mmeconfig_structsize)
		{
			input_id   = (U32                 )(mmemixer_input_config[config_id].InputId & 0xF) ;
			input_type = (enum eMixerInputType)(mmemixer_input_config[config_id].InputId >>  4) ;

			if (input_id > ACC_MIXER_MAX_NB_INPUT)
			{
				STTBX_Print(("\t Input 0x%02X [Skipped] \n", input_id));

				//return (ST_ERROR_BAD_PARAMETER);
			}
			else if (input_id < mixer->Config.NbInput)
			{
				STTBX_Print(("\t Input 0x%02X [Update] \n", input_id));


				// Set mixer specific configuration
				coeff      = (U32)mmemixer_input_config[config_id].Alpha;
				coeff      = coeff << 15;  // convert 0-0xffff -> 0-0x7fff8000

				lxamixer_config->Alpha[input_id]        = (unsigned int)coeff;
				lxamixer_config->FirstOutChan[input_id] = mmemixer_input_config[config_id].FirstOutputChan;

				// Update input setting
				pcmbuf = pcmin[input_id];

				if (pcmbuf == NULL)
				{
					// allocate each Input PCM Buffer
					pcmbuf     = (tPcmBuffer *) malloc( sizeof(tPcmBuffer));

					if (pcmbuf)
					{
						// Initialise
						mem_alloc += sizeof(tPcmBuffer);
						pcmbuf->SamplesPtr[ACC_MAIN_LEFT] = NULL;
						pcmbuf->BufferSize                = 0;
						pcmbuf->NbChannels                = 0;
					}
				}

				if (pcmbuf)
				{
					//int                   nbchan_old = pcmbuf->NbChannels;
					//enum eAccWordSizeCode ws_old     = pcmbuf->WordSize;
					enum eAccAcMode       true_mode;

					pcmbuf->Offset       = mmemixer_input_config[config_id].NbChannels;
					pcmbuf->NbChannels   = mmemixer_input_config[config_id].NbChannels;
					pcmbuf->SamplingFreq = mmemixer_input_config[config_id].SamplingFreq;
//					pcmbuf->SamplingFreq = ACC_FS_ID;                          // Will be set when applying SFC.
					pcmbuf->WordSize     = mmemixer_input_config[config_id].WordSize;
					pcmbuf->AudioMode    = mmemixer_input_config[config_id].AudioMode;

					// Set AcMode
					true_mode = ((pcmbuf->NbChannels <= 2) & (pcmbuf->AudioMode == ACC_MODE10)) ? ACC_MODE_ALL1 : pcmbuf->AudioMode;

					for(i=0;i<SYS_NB_MAX_CHANNELS;i++)
					{
						pcmbuf->ChannelExist[i]  = ACC_MME_FALSE;
					}

					pcmbuf->ChannelExist[ACC_MAIN_LEFT]  = ACC_MME_TRUE;
					pcmbuf->ChannelExist[ACC_MAIN_RGHT]  = (pcmbuf->NbChannels == 2) ? ACC_MME_TRUE : ACC_MME_FALSE;

					// Check property change
/*					if (  (nbchan_old  != mmemixer_input_config[config_id].NbChannels)
					    ||(ws_old      != mmemixer_input_config[config_id].WordSize  ))
					{
						// Reallocate with new property
						STTBX_Print(("\t>> Reallocate PcmBuf with a different number of channels \n"));

						// Allocate buffer
						pcmbuf->Offset = mmemixer_input_config[config_id].NbChannels;
						mem_alloc += mixer_alloc_pcmin(pcmbuf, mmemixer_config->MaxNbOutSamples);

						status = (pcmbuf->SamplesPtr[0]) ? MME_SUCCESS:MME_NOMEM;
					}
*/
					pcmin[input_id] = pcmbuf;
				}
				else
				{
					return(MME_NOMEM);
				}
			}
			config_id++;
			mmeconfig_structsize -= sizeof(MME_MixerInputConfig_t);
		}   // End of While
	}

	return (status);

}

MME_ERROR      mixer_setGlobals(MME_LxMixerMem_t                     * mixer,
                                MME_LxMixerTransformerGlobalParams_t * globalParameters,
                                enum eAccSetConfig                     setConfig)
{
	enum eAccMixerId    mixer_id;
	MME_ERROR           status       = MME_SUCCESS;

	U32   param_size   = globalParameters->StructSize - sizeof(U32); // take into account the first structsize field
	U32 * mixer_params = (U32 *) & globalParameters->InConfig;  // by default

	// Set Mixer Params
	while (param_size >= (2 * sizeof(U32))) // at least StructSize + Id
	{
		mixer_id   = mixer_params[ACC_GP_ID];

		if (mixer_id < ACC_RENDERER_MIXER_POSTPROCESSING_ID)
		{
			int i;
			STTBX_Print((" mixer_mme_%s(): Configuration of the input streams given\n",
				 (setConfig == ACC_SET_CONFIG_INIT) ? "init" : "set_globals" ));

			// Run mixer_setConfig()
			status = mixer_setConfig(mixer, (MME_LxMixerInConfig_t * )mixer_params, setConfig );
			for(i=0; i<MAX_CHANS_ST20;i++)
			{
				((tMixerConfig *)mixer->MixerConfig)->Volume[i] = globalParameters->Volume[i];
				param_size   -= sizeof(U32);
			}
			((tMixerConfig *)mixer->MixerConfig)->Swap[0] = globalParameters->Swap[0];
			param_size   -= sizeof(U32);
			((tMixerConfig *)mixer->MixerConfig)->Swap[1] = globalParameters->Swap[1];
			param_size   -= sizeof(U32);
		}
		else
		{
			STTBX_Print((" Warning !! Only ACC_RENDERER_MIXER_ID supported.\n"));
		}

		// update param parsing
		param_size   -= mixer_params[ACC_GP_SIZE];
		mixer_params  = (U32 *) ((U32) mixer_params + mixer_params[ACC_GP_SIZE]);
	}

	return (status);

}

MME_ERROR mixer_InitTransformer(MME_UINT initParamsLength,MME_GenericParams_t Params_p, void **Handle)
{
	MME_LxMixerTransformerInitParams_t * InitParams;
	MME_LxMixerMem_t                   * mixer;
	MME_ERROR                            status;
	U32                                  mem_alloc = 0;
	tMixerConfig                       * mixer_cfg;

	InitParams = (MME_LxMixerTransformerInitParams_t*) Params_p;

	// Initialize mixer
	mixer      =  (MME_LxMixerMem_t *) malloc(sizeof(MME_LxMixerMem_t));

	STTBX_Print(("mixer_initTransformer( %08X ): init Mixer \n", mixer));
#ifdef MIXER_DISPLAY
	display_mixer_init(initParamsLength, InitParams);
#endif

	// Memory for mixer config
	mixer_cfg  = ( tMixerConfig * ) malloc( sizeof(tMixerConfig )) ;

	status = (( mixer_cfg  == NULL ) || ( mixer == NULL )) ? MME_NOMEM : MME_SUCCESS;

	if ( status == MME_SUCCESS )
	{
		tPcmProcessingInterface   * mixer_if = &mixer->PcmInterface;
		int                         i;

		mem_alloc += sizeof ( tMixerConfig ) + sizeof ( MME_LxMixerMem_t );

		// reset Input config
		for (i = 0; i < ACC_MIXER_MAX_NB_INPUT; i++)
		{
			mixer->InputConfig[i].SamplingFreq = ACC_FS_reserved;
			mixer->InputConfig[i].AudioMode    = ACC_MODE_ID;

			mixer_if->PcmIn[i]   = NULL;
		}

		mixer->StructSize        = sizeof(MME_LxMixerMem_t);
		mixer->OutputNbChannels  = InitParams->OutputNbChannels;
		mixer->Config.NbInput    = InitParams->NbInput;
		mixer->OutputIdx         = ACC_MIX_MAIN; // Ouptuts on Main by default..
		mixer->MixerConfig       = (void *)mixer_cfg;

		mixer_cfg->NbInput       = InitParams->NbInput;

		mixer_if->ProcessMem     = NULL;
		mixer_if->ProcessMemSize = 0;
		mixer_if->FirstTime      = ACC_MME_TRUE;
		mixer_if->NbPcmIn        = InitParams->NbInput;
		mixer_if->Config         = (void *)mixer_cfg;
		mixer_if->NbPcmOut       = InitParams->OutputNbChannels;
		mixer_if->PcmOut[0]      = &mixer->PcmOut[0];

		// Pre-Set the output PCM Buffers
		mixer_if->PcmOut[ACC_MIX_MAIN]->Offset       = mixer->PcmOut[ACC_MIX_MAIN].Offset       = InitParams->OutputNbChannels;
		mixer_if->PcmOut[ACC_MIX_MAIN]->NbChannels   = mixer->PcmOut[ACC_MIX_MAIN].NbChannels   = InitParams->OutputNbChannels;
		mixer_if->PcmOut[ACC_MIX_MAIN]->AudioMode    = mixer->PcmOut[ACC_MIX_MAIN].AudioMode    = ACC_MODE_ID;

		// Initialise mixer config
		STTBX_Print(("\t Init Output max %d samples @ %d kHz\n", InitParams->MaxNbOutputSamplesPerTransform,
			InitParams->OutputSamplingFreq));
		mixer->Config.OutputSamplingFreq = InitParams->OutputSamplingFreq;
		mixer->Config.MaxNbOutSamples    = InitParams->MaxNbOutputSamplesPerTransform;
		mixer->Config.NbOutSamples       = InitParams->MaxNbOutputSamplesPerTransform;// set to max by default

		mixer_setGlobals(mixer, &InitParams->GlobalParams, ACC_SET_CONFIG_INIT);

		// 0th input is used to load the SamplingFreq & WordSize.
		mixer_if->PcmOut[ACC_MIX_MAIN]->SamplingFreq = mixer->PcmOut[ACC_MIX_MAIN].SamplingFreq = mixer->PcmInterface.PcmIn[0]->SamplingFreq;
		// Word size will be fixed to WS16
		mixer_if->PcmOut[ACC_MIX_MAIN]->WordSize     = mixer->PcmOut[ACC_MIX_MAIN].WordSize     = ACC_WS16 ;//mixer->PcmInterface.PcmIn[0]->WordSize;


	}
	else
	{
		if ( mixer != NULL )
		{
			free ( mixer );
		}

		if ( mixer_cfg != NULL )
		{
			free( mixer_cfg );
		}
		status = MME_NOMEM;
	}

	// Set the handle to the address of the created mixer
	*Handle = (void *)mixer;

	return ( status );

}


MME_ERROR mixer_SetGlobalTransformParameters(MME_LxMixerMem_t* mixer,MME_Command_t * Command )
{
	MME_ERROR                              status;
	MME_LxMixerTransformerGlobalParams_t * globalParameters;


	// Set Mixer params
	globalParameters = (MME_LxMixerTransformerGlobalParams_t *) Command->Param_p;

	status = mixer_setGlobals(mixer, globalParameters, ACC_SET_CONFIG_UPDATE_GLOBALS);

	// 0th input is used to load the SamplingFreq & WordSize.
	mixer->PcmInterface.PcmOut[ACC_MIX_MAIN]->SamplingFreq = mixer->PcmOut[ACC_MIX_MAIN].SamplingFreq = mixer->PcmInterface.PcmIn[0]->SamplingFreq;
	mixer->PcmInterface.PcmOut[ACC_MIX_MAIN]->WordSize     = mixer->PcmOut[ACC_MIX_MAIN].WordSize     = mixer->PcmInterface.PcmIn[0]->WordSize;


	return status;
}

MME_ERROR mixer_GetPointerAndSizeFromMMEDataBuffer(MME_DataBuffer_t * buffer, void ** data, int * size, int page_idx)
{
	if (buffer->StructSize != sizeof(MME_DataBuffer_t))
	{
		STTBX_Print(("Mixer Transform(): MME_DataBuffer_t.StructSize(%i) !=  sizeof(MME_DataBuffer_t): %d!\n",
		   buffer->StructSize, sizeof(MME_DataBuffer_t)));
		return MME_INVALID_ARGUMENT;
	}

	if (buffer->ScatterPages_p == NULL)
	{
		STTBX_Print(("Mixer Transform(): MME_DataBuffer_t.ScatterPages_p == NULL!\n"));
		return MME_INVALID_ARGUMENT;
	}

	page_idx = (page_idx == -1) ? buffer->NumberOfScatterPages - 1 : page_idx;

	*data = (char *)buffer->ScatterPages_p[page_idx].Page_p;
	*size = buffer->ScatterPages_p[page_idx].Size;

	return MME_SUCCESS;
}


#define BYTES_PER_WORD(x) ( 4 >> (x)) // WARNING :: not valid for WS24!!

void multicom_clean_pcmbuffer(tPcmBuffer * pcmbuf, enum eAccWordSizeCode ws)
{
	enum eAccMainChannelIdx   chan_out;
	unsigned int              mask = 0, mask_ch, spl_idx, nspl;
	int                       nch;
	int                     * ptr_main, *ptr_aux, *ptr;

	nch = pcmbuf->Offset; // take into account interleaving to determine real number of channels
	for (chan_out = 0; chan_out < nch; chan_out++)
	{
		mask |= (pcmbuf->ChannelExist[chan_out] << chan_out);
	}

	// Get Ptr of very first channel in buffer (depending on order of MAIN versus AUX)
	ptr_main = (int *) pcmbuf->SamplesPtr[ACC_MAIN_LEFT];
	ptr_aux  = (int *) pcmbuf->SamplesPtr[ACC_AUX_LEFT];
	ptr = (nch > ACC_AUX_LEFT) ? ((ptr_aux < ptr_main) ? ptr_aux : ptr_main) : ptr_main;

	// Correct mask depending on order of MAIN versus AUX
	mask = ((nch > ACC_AUX_LEFT) & (ptr_aux < ptr_main)) ? (((mask >> (nch-2)) & 0x3) + (mask << 2)): mask;

	nspl= pcmbuf->NbSamples * nch;

	if (ws == ACC_WS32)
	{
		chan_out = ACC_MAIN_LEFT;
		for (spl_idx = 0; spl_idx < nspl; spl_idx ++)
		{
			mask_ch = (mask >> chan_out) & 1;

			//ptr[spl_idx] = (mask_ch == 0) ? 0 : ptr[spl_idx];
			if(mask_ch == 0)
				ptr[spl_idx] = 0;
			chan_out = ((chan_out+1) == nch) ? 0 : chan_out+1;
		}
	}
	else if (ws == ACC_WS16)
	{
		short   * ptr_out = (short *) ptr;
		int       spl_in;

		chan_out = ACC_MAIN_LEFT;
		for (spl_idx = 0; spl_idx < nspl; spl_idx ++)
		{
			mask_ch = (mask >> chan_out) & 1;
			// round
			spl_in  = (ptr[spl_idx] >> 1) + 0x4000;

			// saturate
			spl_in  = (spl_in >= (int) 0x40000000) ? 0x7FFF : spl_in >> 15;
			spl_in  = (spl_in  < (int) 0xC0000000) ? 0x8000 : spl_in;

			// write back
			ptr_out[spl_idx] = (mask_ch == 0) ? 0 : (short) spl_in;
			// update channel counter
			chan_out = ((chan_out+1) == nch) ? 0 : chan_out+1;
		}
	}
}



MME_ERROR mixer_DecodeFrame(MME_LxMixerMem_t     * mixer,MME_Command_t * Command)
{
	MME_BufferPassingConvention_t             * bufferParams;
	MME_DataBuffer_t                         ** inputBuffers;
	MME_DataBuffer_t                         ** outputBuffers;
    MME_CommandStatus_t                       * commandStatus   = & Command->CmdStatus;

	MME_ERROR             result, mixerError;
	int                   processedFrameSize,i;

	MME_LxMixerTransformerFrameDecodeStatusParams_t * mixer_status = NULL;

	tPcmProcessingInterface * mixer_interface = &mixer->PcmInterface;
	tPcmBuffer              * mix_out         = mixer_interface->PcmOut[ACC_MIX_MAIN];
	tPcmBuffer              * pcm_out         = &mixer->PcmOut[ACC_MIX_MAIN];
	tPcmBuffer              ** pcm_in         = mixer_interface->PcmIn;
//	tPcmBuffer              * pcm_in1         =   mixer_interface->PcmIn[1];// need to chenge


	int                       dest_size;
	enum eAccMainChannelIdx   ch;
	U16                     * pcmptr;

	if (commandStatus->AdditionalInfoSize >= sizeof(MME_MixerInputStatus_t))
	{
		mixer_status = (MME_LxMixerTransformerFrameDecodeStatusParams_t *) commandStatus->AdditionalInfo_p;
	}

	bufferParams  = (MME_BufferPassingConvention_t *) & Command->NumberInputBuffers;
	inputBuffers  = (MME_DataBuffer_t**)(&(Command->DataBuffers_p[0]));
	outputBuffers = (MME_DataBuffer_t**)(&(Command->DataBuffers_p[Command->NumberInputBuffers]));

	// Output FrameSize = nb_out_samples * nb_out_channels * 4 bytes
	processedFrameSize = mixer->Config.NbOutSamples * mixer->OutputNbChannels * sizeof(U16);

	result = mixer_GetPointerAndSizeFromMMEDataBuffer(outputBuffers[0], ( void ** )  &pcmptr, &dest_size, 0 );
	if (result != MME_SUCCESS)
	{
		return result;
	}

	if (dest_size < processedFrameSize)
	{
		STTBX_Print(("mixer_ProcessCommand(): Output buffer no.%i too small (%i bytes), need at least %i bytes!\n",
		   ACC_MIX_MAIN, dest_size, processedFrameSize));
		return MME_INVALID_ARGUMENT;
	}

	// Reset output buffer
	pcm_out->NbChannels   = mixer->OutputNbChannels;
	pcm_out->WordSize     = mixer_interface->PcmIn[0]->WordSize;
	pcm_out->NbSamples    = 0;
	pcm_out->SamplingFreq = mixer_interface->PcmIn[0]->SamplingFreq;
	pcm_out->BufferSize   = dest_size / (pcm_out->Offset * BYTES_PER_WORD(pcm_out->WordSize));

	// Set output pointers
	for (ch = ACC_MAIN_LEFT ; ch < pcm_out->NbChannels; ch++)
	{
		// In - place mixer of main input to output
		pcm_out[ACC_MIX_MAIN].SamplesPtr[ch] = (int *)pcmptr;
		pcmptr++;
	}
	for(i=0;i<mixer_interface->NbPcmIn;i++)
	{
		result = mixer_GetPointerAndSizeFromMMEDataBuffer(inputBuffers[i], ( void ** )  &pcmptr, &dest_size, 0 );
		if (result != MME_SUCCESS)
		{
			STTBX_Print(("mixer_ProcessCommand(): input buffer no.%i too small (%i bytes), need at least %i bytes!\n",
			   ACC_MIX_MAIN, dest_size, processedFrameSize));
			return MME_INVALID_ARGUMENT;
		}

		// Reset output buffer
		pcm_in[i]->NbSamples    = dest_size/(pcm_in[i]->NbChannels*BYTES_PER_WORD(pcm_in[i]->WordSize));
		// Set output pointers
		for (ch = ACC_MAIN_LEFT ; ch < pcm_in[i]->NbChannels; ch++)
		{
			// In - place mixer of main input to output
			pcm_in[i]->SamplesPtr[ch] = (int *)pcmptr;
			pcmptr++;
		}
	}

	// Launch the decoding
	mixerError = mixer_process(mixer_interface);

	if (mixerError != MME_SUCCESS)
	{
		STTBX_Print(("mixer_ProcessCommand(): Mixer returned 0x%08x on call to mixer_process()!\n", mixerError));

		outputBuffers[ACC_MIX_MAIN]->ScatterPages_p->BytesUsed = 0;

		// Reset status
		mixer_status->NbOutSamples = 0;
		mixer_status->PTSflag      = 0;

		return MME_INVALID_ARGUMENT;
	}
	else
	{
		// set mixer status
		if ( mixer_status )
		{
			mixer_status->MixerAudioMode       = mix_out->AudioMode;
			mixer_status->MixerSamplingFreq    = mix_out->SamplingFreq;

			mixer_status->AudioMode    = pcm_out->AudioMode;
			mixer_status->SamplingFreq = pcm_out->SamplingFreq;
			mixer_status->NbOutSamples = pcm_out->NbSamples;
			mixer_status->Emphasis     = ACC_EMPHASIS_EXTRACT(pcm_out->Flags);
			mixer_status->PTSflag      = ACC_PTSflag_EXTRACT(pcm_out->Flags);
			mixer_status->PTS          = pcm_out->PTS;
			for(i=0;i<mixer_interface->NbPcmIn;i++)
			{
				mixer_status->InputStreamStatus[i].State	 			= MIXER_INPUT_RUNNING;
				mixer_status->InputStreamStatus[i].BytesUsed 			= pcm_in[i]->NbSamples * pcm_in[i]->NbChannels * BYTES_PER_WORD(pcm_in[i]->WordSize);
				mixer_status->InputStreamStatus[i].NbInSplNextTransform = 0;
			}
		}

		// Clean the channels   // -- To be validated.
		//multicom_clean_pcmbuffer ( pcm_out, pcm_out->WordSize );

		// Set "BytesUsed" to the size of a decoded Frame
		processedFrameSize = pcm_out->NbSamples * pcm_out->NbChannels * BYTES_PER_WORD(pcm_out->WordSize);
		//processedFrameSize = pcm_out->NbSamples * pcm_out->NbChannels * 2;
		outputBuffers[ACC_MIX_MAIN]->ScatterPages_p->BytesUsed = processedFrameSize;

	}

	return MME_SUCCESS;

}

MME_ERROR mixer_ProcessCommand(void * Handle, MME_Command_t * CmdInfo_p)
{
	MME_LxMixerTransformerSetGlobalStatusParams_t * cmd_status;

	MME_LxMixerMem_t        * mixer;
	MME_MixerInputStatus_t  * mixer_status;
	MME_ERROR                 status           = MME_SUCCESS;
	MME_CommandCode_t         CmdCode          = CmdInfo_p->CmdCode;
	enum eAccBoolean          mixing_transform = ACC_MME_FALSE;

	// Check parameters
	if (!Handle)
	{
		STTBX_Print(("mixer_ProcessCommand(): Handle is NULL!\n"));
		return MME_INVALID_HANDLE;
	}

	if (!CmdInfo_p)
	{
		STTBX_Print(("mixer_ProcessCommand(): CmdInfo_p is NULL!\n"));
		return MME_INVALID_ARGUMENT;
	}

	// Cast the handle into a pointer to our mixer
	mixer      = (MME_LxMixerMem_t*)Handle;
	cmd_status = (MME_LxMixerTransformerSetGlobalStatusParams_t *) CmdInfo_p->CmdStatus.AdditionalInfo_p;

	// Update state of command execution
	CmdInfo_p->CmdStatus.State = MME_COMMAND_EXECUTING;

	switch (CmdCode)
	{
	case MME_SET_GLOBAL_TRANSFORM_PARAMS:
		{
			MME_LxMixerTransformerSetGlobalStatusParams_t * setgp_status = cmd_status;
			MME_LxMixerTransformerGlobalParams_t          * param_p;

			param_p = (MME_LxMixerTransformerGlobalParams_t * ) CmdInfo_p->Param_p;
			status  = mixer_SetGlobalTransformParameters(mixer, CmdInfo_p);

			if (CmdInfo_p->CmdStatus.AdditionalInfoSize >= sizeof(MME_MixerInputStatus_t))
			{
				mixer_status              = &setgp_status->InputStreamStatus[0];
				setgp_status->ElapsedTime = 0;
			}
		}
	break;

	case MME_TRANSFORM:
		{
			MME_LxMixerTransformerFrameDecodeStatusParams_t * frame_status;

			// Process given frames
			status = mixer_DecodeFrame(mixer, CmdInfo_p);

			// Get pointer to input status.
			frame_status = (MME_LxMixerTransformerFrameDecodeStatusParams_t * )
			                CmdInfo_p->CmdStatus.AdditionalInfo_p;
			if (CmdInfo_p->CmdStatus.AdditionalInfoSize >= sizeof(MME_MixerInputStatus_t))
			{
				mixer_status              = &frame_status->InputStreamStatus[0];
				frame_status->ElapsedTime = 0;
			}
			mixing_transform = ACC_MME_TRUE;
		}
		break;

	default:
		{
			return MME_INVALID_ARGUMENT;
		}
	}

	//// Check for status availability
	//if (CmdInfo_p->CmdStatus.AdditionalInfoSize >= sizeof(MME_MixerInputStatus_t))
	//{
	//	cmd_status->StructSize  = CmdInfo_p->CmdStatus.AdditionalInfoSize;
	//	mixer_updatestatus(mixer, mixer_status, mixing_transform);
	//}

	// Update state of command execution
	CmdInfo_p->CmdStatus.State = MME_COMMAND_COMPLETED;
	CmdInfo_p->CmdStatus.Error = status;

	return (status);;
}

MME_ERROR mixer_AbortCommand(void * Handle, MME_CommandId_t commandId)
{
	STTBX_Print(("Mixer Abort Command. Nothing to be Done !!\n"));
	return MME_SUCCESS;
}

MME_ERROR mixer_TermTransformer(void * Handle)
{
	MME_LxMixerMem_t   * mixer   = (MME_LxMixerMem_t *) Handle;


	// Check parameters
	if (Handle == NULL)
	{
		STTBX_Print(("mixer_TermTransformer(): Handle is NULL!\n"));
		return MME_INVALID_HANDLE;
	}

	STTBX_Print(("mixer_mme.c: mixer_TermTransformer(%08X) >> Terminate Mixer \n", (unsigned int) Handle));

	if ( mixer != NULL )
	{
		int i;

		for(i=0; i<	ACC_MIXER_MAX_NB_INPUT; i++)
		{
			if(mixer->PcmInterface.PcmIn[i] != NULL )
			{
				/*if ( mixer->PcmInterface.PcmIn[i]->SamplesPtr[ACC_MAIN_LEFT] != NULL )
				{
					free(mixer->PcmInterface.PcmIn[i]->SamplesPtr[ACC_MAIN_LEFT]);
				}*/ //No need to free PCM Pointers these are assainged by driver
				free(mixer->PcmInterface.PcmIn[i]);
			}
		}

		if ( mixer->MixerConfig != NULL )
		{
			free( mixer->MixerConfig );
		}

		free ( mixer );
	}

	return MME_SUCCESS;
}
