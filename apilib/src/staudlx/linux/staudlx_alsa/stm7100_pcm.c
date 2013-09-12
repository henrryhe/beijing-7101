/*
 *  ALSA Driver on the top of STAUDLX
 *  Copyright (c)   (c) 2006 STMicroelectronics Limited
 *
 *  *  Authors:  Udit Kumar <udit-dlh.kumar@st.com>
 *
 *   This program is interface between STAUDLX and ALSA application. 
 * For ALSA application, this will serve as ALSA driver but at the lower layer, it will map ALSA lib calls to STAULDX calls
 */

#include <linux/delay.h>
#include <linux/device.h>
#ifdef STAUDLX_ALSA_LDDE2.2
#include <linux/platform_device.h>
#endif 

				
static snd_pcm_hardware_t stb7100_pcm_hw =
{
	.info =		(SNDRV_PCM_INFO_MMAP           |
			 SNDRV_PCM_INFO_INTERLEAVED    |
			 SNDRV_PCM_INFO_BLOCK_TRANSFER |
			 SNDRV_PCM_INFO_MMAP_VALID     |
			 SNDRV_PCM_INFO_PAUSE),

	//.info =		(        
//			 SNDRV_PCM_INFO_INTERLEAVED    |
//			 SNDRV_PCM_INFO_BLOCK_TRANSFER |
			 
			    
//			 SNDRV_PCM_INFO_PAUSE),

	.formats =	(SNDRV_PCM_FMTBIT_S8 |SNDRV_PCM_FMTBIT_U8 |
				SNDRV_PCM_FMTBIT_S16_BE |SNDRV_PCM_FMTBIT_U16_BE |
				SNDRV_PCM_FMTBIT_S16_LE| SNDRV_PCM_FMTBIT_U16_LE),


 //.formats =	(SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S16_LE),
//	.formats =	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_BE |
//				SNDRV_PCM_FMTBIT_U16_BE |SNDRV_PCM_FMTBIT_S24_BE|
//				SNDRV_PCM_FMTBIT_U24_BE|SNDRV_PCM_FMTBIT_S16_LE|
//				SNDRV_PCM_FMTBIT_U16_LE| SNDRV_PCM_FMTBIT_S24_3LE),

	.rates =	(SNDRV_PCM_RATE_8000 |
			 SNDRV_PCM_RATE_16000 |
			 SNDRV_PCM_RATE_32000 |
			 SNDRV_PCM_RATE_44100 |
			 SNDRV_PCM_RATE_48000 |
			 SNDRV_PCM_RATE_96000 |
			 SNDRV_PCM_RATE_192000 ),

	.rate_min	  = 8000,
	.rate_max	  = 192000,
	.channels_min	  = 2,
	.channels_max	  =2,
	.buffer_bytes_max = FRAMES_TO_BYTES(PCM_MAX_FRAMES,2),
	.period_bytes_min = FRAMES_TO_BYTES(1,2),
	.period_bytes_max = FRAMES_TO_BYTES(PCM_MAX_FRAMES,2),
	.periods_min	  = 1,
	.periods_max	  = PCM_MAX_FRAMES
};

#if 0
static snd_pcm_hardware_t stb7100_spdif_hw =
{
	.info =		(SNDRV_PCM_INFO_INTERLEAVED |
			 SNDRV_PCM_INFO_PAUSE),

	.formats =	(SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S24_LE),

	.rates =	(SNDRV_PCM_RATE_32000 |
			 SNDRV_PCM_RATE_44100 |
			 SNDRV_PCM_RATE_48000),

	.rate_min	  = 32000,
	.rate_max	  = 48000,
	.channels_min	  = 2,
	.channels_max	  = 2,
	.buffer_bytes_max = FRAMES_TO_BYTES(PCM_MAX_FRAMES,2),
	.period_bytes_min = FRAMES_TO_BYTES(1,2),
	.period_bytes_max = FRAMES_TO_BYTES(PCM_MAX_FRAMES,2),
	.periods_min	  = 1,
	.periods_max	  = PCM_MAX_FRAMES
};
#endif 
static snd_pcm_hardware_t stb7100_capture_hw =
{
	.info =		(SNDRV_PCM_INFO_MMAP           |
			 SNDRV_PCM_INFO_INTERLEAVED    |
			 SNDRV_PCM_INFO_BLOCK_TRANSFER |
			 SNDRV_PCM_INFO_MMAP_VALID     |
			 SNDRV_PCM_INFO_PAUSE),

	 .formats =	(SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S16_LE),
//	 .formats =	(SNDRV_PCM_FMTBIT_S32_LE ),
	
	.rates =	(SNDRV_PCM_RATE_32000 |
			 SNDRV_PCM_RATE_44100 |
			 SNDRV_PCM_RATE_48000 |
			 SNDRV_PCM_RATE_96000 |
			 SNDRV_PCM_RATE_192000 ),

	.rate_min	  = 32000,
	.rate_max	  = 192000,
	.channels_min	  = 2,
	.channels_max	  =2,
	.buffer_bytes_max = FRAMES_TO_BYTES(CAPTURE_MAX_FRAMES,2),
	.period_bytes_min = FRAMES_TO_BYTES(384,2),
	.period_bytes_max = FRAMES_TO_BYTES(CAPTURE_MAX_FRAMES,2),
	.periods_min	  =1,
	.periods_max	  = CAPTURE_MAX_FRAMES
};


static int snd_generic_volume_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count =2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 63;
	return 0;
}


static int snd_generic_volume_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.integer.value[0] = chip->Volume[LEFTCHANNEL];
	ucontrol->value.integer.value[1] = chip->Volume[RIGHTCHANNEL];
	return 0;
}


static int snd_generic_volume_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);
	unsigned char old, val;
	BOOL Changed= FALSE;

	//spin_lock_irq(&chip->lock);
	old = chip->Volume[LEFTCHANNEL];

	val = ucontrol->value.integer.value[0];
	chip->Volume[LEFTCHANNEL] =val ;
	if(old!=val) 
		Changed = TRUE;

	old = chip->Volume[RIGHTCHANNEL];
	val = ucontrol->value.integer.value[1];
	chip->Volume[RIGHTCHANNEL] =val ;
	if(old!=val) 
		Changed = TRUE;

	// Set Audio Driver Attenuatition // 
	{
		ST_ErrorCode_t Error=0;
		STAUD_Attenuation_t Attenuatation; 
		memset(&Attenuatation,0,sizeof(STAUD_Attenuation_t)) ;
		Attenuatation.Left = chip->Volume[LEFTCHANNEL];
		Attenuatation.Right = chip->Volume[RIGHTCHANNEL];
		switch(chip->card_data->major)
		{
			case PCM0_DEVICE:
				Error=STAUD_PPSetAttenuation(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED0, &Attenuatation);				
			break;

			case PCM1_DEVICE:
				Error=STAUD_PPSetAttenuation(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED1, &Attenuatation);				
			break;

			case SPDIF_DEVICE:
				Error=STAUD_PPSetAttenuation(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED2, &Attenuatation);				
			break;

			default:
				break;
		}
		DEBUG_PRINT(("Mixer Put Callback for Volume l=%d, r=%d", chip->Volume[LEFTCHANNEL], chip->Volume[RIGHTCHANNEL]));
		DEBUG_PRINT(("Set Attenuatation Error is =%d, Device is =%d",Error,chip->card_data->major));
	}

	//spin_unlock_irq(&chip->lock);
	return Changed;
}


static snd_kcontrol_new_t snd_generic_volume __devinitdata = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"PCM "  "Playback "  "Volume ",
	.info =		snd_generic_volume_info,
	.get =		snd_generic_volume_get,
	.put =		snd_generic_volume_put
};


static int __devinit snd_generic_create_controls(pcm_hw_t *chip)
{
	int err;
//	snd_kcontrol_t *kctl;

		err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_generic_volume, chip));
		if (err < 0)
			return err;
		
	return 0;
}



static void stb7100_pcm_stop_playback(snd_pcm_substream_t *substream)
{
 	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	STAUD_Stop_t StopMode;
	U32 Error=0;
	STAUD_Fade_t Fade;
	StopMode = STAUD_STOP_NOW;
	Fade.FadeType = STAUD_FADE_SOFT_MUTE;

	//spin_lock(&chip->lock);
	DEBUG_PRINT((" Debug : Unlocking Stream \n "));
	snd_pcm_stream_unlock_irq(substream);
	DEBUG_PRINT((" Debug : Unlocked Stream And DR Stop \n "));
	Error = STAUD_DRStop(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED0,StopMode,&Fade);
	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_DRStop Failed Error code =%d Device =%d \n",Error,chip->card_data->major);

	}
	snd_pcm_stream_lock_irq(substream);
	
}


static void stb7100_pcm_start_playback(snd_pcm_substream_t *substream)
{
	pcm_hw_t     *chip = snd_pcm_substream_chip(substream);
	STAUD_StreamParams_t StartParams;
	U32 Error = 0;
	StartParams.StreamContent = STAUD_STREAM_CONTENT_ALSA; // STAUD_STREAM_CONTENT_MAX;  // For ALSA Added in STAUDLX // 
	StartParams.StreamType= STAUD_STREAM_TYPE_ES;
	StartParams.SamplingFrequency = substream->runtime->rate;

	//spin_lock(&chip->lock);
	if(chip->card_data->major == SPDIF_DEVICE)
	{
		STAUD_DigitalMode_t SPDIFMode=0;
		if(chip->iec60958_rawmode)
		{	
			SPDIFMode = STAUD_DIGITAL_MODE_NONCOMPRESSED;			
		}
		
		else
		{
			SPDIFMode = STAUD_DIGITAL_MODE_NONCOMPRESSED;
			switch(chip->iec_encoding_mode)
			{
			// Fill the desired Start Stream Contents
				
				case ENCODING_IEC61937_MPEG_384_FRAME:
				case ENCODING_IEC61937_MPEG_1152_FRAME:
				
				case ENCODING_IEC61937_MPEG_2304_FRAME:
				case ENCODING_IEC61937_MPEG_768_FRAME:
				case ENCODING_IEC61937_MPEG_2304_FRAME_LSF:
				case ENCODING_IEC61937_MPEG_768_FRAME_LSF:
					StartParams.StreamContent = STAUD_STREAM_CONTENT_MPEG2;
				break;
					
				case ENCODING_IEC61937_MPEG_1024_FRAME:
					StartParams.StreamContent = STAUD_STREAM_CONTENT_MPEG_AAC;
				break; 

				case ENCODING_IEC61937_AC3:
					StartParams.StreamContent = STAUD_STREAM_CONTENT_AC3;
				break;

				case ENCODING_IEC61937_DTS_1:
				case ENCODING_IEC61937_DTS_2:
				case ENCODING_IEC61937_DTS_3:
					StartParams.StreamContent = STAUD_STREAM_CONTENT_DTS;
					break;

						
				default:
					break;
			}
			
			
		}
		// Here set the SPDIF Mode 
		Error = STAUD_OPSetDigitalMode(chip->AudioHandle, STAUD_OBJECT_OUTPUT_SPDIF0,SPDIFMode);
	}	
	snd_pcm_stream_unlock_irq(substream);
	Error=STAUD_DRStart(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED0,&StartParams);
	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_DRSTART Failed Error code =%d and device is =%d \n",Error,chip->card_data->major);
	}	
	// Set the Volume After Start as indicated by Mixer Interface, if any 
	{
		STAUD_Attenuation_t Attenuatation; 
		memset(&Attenuatation,0,sizeof(STAUD_Attenuation_t)) ;
		Attenuatation.Left = chip->Volume[LEFTCHANNEL];
		Attenuatation.Right = chip->Volume[RIGHTCHANNEL];
		switch(chip->card_data->major)
		{
			case PCM0_DEVICE:
				
				Error=STAUD_PPSetAttenuation(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED0, &Attenuatation);
				
			break;

			case PCM1_DEVICE:
				
				Error=STAUD_PPSetAttenuation(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED1, &Attenuatation);
				
			break;

			case SPDIF_DEVICE:
				
				Error=STAUD_PPSetAttenuation(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED2, &Attenuatation);
				
			break;

			default:
				printk("oops in vol Set + Start \n");
				break;
		}
		DEBUG_PRINT((" Volume l=%d, r=%d", chip->Volume[LEFTCHANNEL], chip->Volume[RIGHTCHANNEL]));
		DEBUG_PRINT(("Set Attenuatation Error is =%d, Device is =%d",Error,chip->card_data->major));
	}
	snd_pcm_stream_lock_irq(substream); 
//	spin_unlock(&chip->lock);
}


static void stb7100_pcm_unpause_playback(snd_pcm_substream_t *substream)
{
 	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
//	unsigned long reg=0;
	U32 Error=0;

//    spin_lock(&chip->lock);
	switch(chip->card_data->major)
	{
		case PCM0_DEVICE:
			Error=STAUD_DRResume(chip->AudioHandle,STAUD_OBJECT_INPUT_CD0);
		break;

		case PCM1_DEVICE:
			Error=STAUD_DRResume(chip->AudioHandle,STAUD_OBJECT_INPUT_CD1);
		break;


		case SPDIF_DEVICE:
			Error=STAUD_DRResume(chip->AudioHandle,STAUD_OBJECT_INPUT_CD2);
			break;

		default:
			break;
	}
	if(Error!=0)
	{
		printk( KERN_ALERT "STAUD_DRResume Failed Error code =%d and Device is =%d \n",Error,chip->card_data->major);
	}
	//spin_unlock(&chip->lock);
}


static void stb7100_pcm_pause_playback(snd_pcm_substream_t *substream)
{
     pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	 STAUD_Fade_t Fade;
	
	unsigned int Error=0;
	Fade.FadeType = STAUD_FADE_SOFT_MUTE;

	//spin_lock(&chip->lock);

	switch(chip->card_data->major)
	{
		case PCM0_DEVICE:
			Error=STAUD_DRPause(chip->AudioHandle,STAUD_OBJECT_INPUT_CD0,&Fade);
		break;

		case PCM1_DEVICE:
			Error=STAUD_DRPause(chip->AudioHandle,STAUD_OBJECT_INPUT_CD1,&Fade);
		break;


		case SPDIF_DEVICE:
			Error=STAUD_DRPause(chip->AudioHandle,STAUD_OBJECT_INPUT_CD2,&Fade);
			break;

		default:
			break;
	}
	
	


	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_DRStop Paused Error code =%d and Device is =%d \n",Error,chip->card_data->major);
	}
//	spin_unlock(&chip->lock);
}


static snd_pcm_uframes_t stb7100_fdma_playback_pointer(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	/*
	 * Calculate our current playback position, using the number of bytes
	 * left for the DMA engine needs to transfer to complete a full
	 * iteration of the buffer. This is common to all STb7100 audio players
	 * using the FDMA (including SPDIF).
	 */
	
	unsigned int  pos=  (unsigned int)chip->ReadPointer- (unsigned int)substream->runtime->dma_addr;	
	return bytes_to_frames(substream->runtime,pos);
}




static int stb7100_pcm_program_hw(snd_pcm_substream_t *substream)
{
	
	U32 Error=0;
	STAUD_PCMInputParams_t InputInterface;
	STAUD_OutputParams_t	     OutPutParams;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);

	InputInterface.Frequency = substream->runtime->rate;
	InputInterface.NumChannels = substream->runtime->channels;
	InputInterface.DataPrecision = 0;


	DEBUG_PRINT(("ALSA ST: Num of channels is %d \n",InputInterface.NumChannels));

	OutPutParams.PCMOutParams.InvertWordClock                 = FALSE; 
	OutPutParams.PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
	OutPutParams.PCMOutParams.InvertBitClock                  = FALSE;
	OutPutParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_16BITS; 
	OutPutParams.PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT;
	OutPutParams.PCMOutParams.MemoryStorageFormat16 = TRUE; 
	OutPutParams.PCMOutParams.MSBFirst                        = TRUE;
	OutPutParams.PCMOutParams.PcmPlayerFrequencyMultiplier	= 256;





	// Udit Debug --1 Lines to remove 
 	//InputInterface.NumChannels = 2;
	
	/*if(substream->runtime->format== SNDRV_PCM_FORMAT_S16_LE)
	{
		InputInterface.DataPrecision = 65536; //0 will take 16 bit format 
		
		OutPutParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_16BITS; 
		OutPutParams.PCMOutParams.MemoryStorageFormat16 = TRUE; 
	}
	
	else if (substream->runtime->format== SNDRV_PCM_FORMAT_S32_LE)
	{

		InputInterface.DataPrecision = 0; //0 will take 16 bit format 
		
		OutPutParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_24BITS; 
		OutPutParams.PCMOutParams.MemoryStorageFormat16 = FALSE; 

	}*/

				
	switch(substream->runtime->format)
	{
		 case SNDRV_PCM_FORMAT_S8:
		 /*case SNDRV_PCM_FMTBIT_S8:*/
		 	InputInterface.DataPrecision = 8;
		 	DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_S8 \n"));
		 	break;
		
		case SNDRV_PCM_FORMAT_U8:
			InputInterface.DataPrecision = 17;
			DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_S8 \n"));
			break; 
			
	 	/*case  SNDRV_PCM_FMTBIT_S16_BE:*/
		case SNDRV_PCM_FORMAT_S16_BE:
			InputInterface.DataPrecision = 16;
			DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_S16_BE \n"));
			break;

		/*case SNDRV_PCM_FMTBIT_U16_BE:*/
		case SNDRV_PCM_FORMAT_U16_BE:
			InputInterface.DataPrecision = 131072;
			DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_U16_BE \n"));
			break;

		/*case SNDRV_PCM_FMTBIT_S24_BE:*/
		case SNDRV_PCM_FORMAT_S24_BE:
		/*case SNDRV_PCM_FORMAT_S24_3LE: */
			InputInterface.DataPrecision = 24;
			DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_S24_BE \n"));
			break;

		/*case SNDRV_PCM_FMTBIT_U24_BE:*/
		case SNDRV_PCM_FORMAT_U24_BE:
			InputInterface.DataPrecision = 24;
			DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_U24_BE \n"));
			break;

		case SNDRV_PCM_FORMAT_S16_LE:
		// case SNDRV_PCM_FMTBIT_S16_LE:
			InputInterface.DataPrecision = 65536;
			DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_S16_LE \n"));
			break;

		case SNDRV_PCM_FORMAT_U16_LE:
		/*case SNDRV_PCM_FMTBIT_U16_LE:*/
			DEBUG_PRINT((" ALSA ST: Prepare Call Format is SNDRV_PCM_FORMAT_U16_LE \n"));
			InputInterface.DataPrecision = 196608;
			break;	

		default:
			printk(KERN_ALERT " ALSA ST: Prepare Call wrong Format is %d\n", substream->runtime->format);
			return -1;

		
	}


	DEBUG_PRINT(( "PCM Prepare callback \n"));
//	spin_lock(&chip->lock);

	if(chip->card_data->major == PCM0_DEVICE)
	{
		Error = STAUD_IPSetPCMParams(chip->AudioHandle,STAUD_OBJECT_INPUT_CD0,&InputInterface);
		//Error |=  STAUD_OPSetParams(chip->AudioHandle,STAUD_OBJECT_OUTPUT_PCMP0,&OutPutParams);

	}
	else if(chip->card_data->major == PCM1_DEVICE)
	{
		Error = STAUD_IPSetPCMParams(chip->AudioHandle,STAUD_OBJECT_INPUT_CD1,&InputInterface);
		//Error |=  STAUD_OPSetParams(chip->AudioHandle,STAUD_OBJECT_OUTPUT_PCMP1,&OutPutParams);

	}
	else if(chip->card_data->major == SPDIF_DEVICE)
	{
		OutPutParams.SPDIFOutParams.AutoLatency					= TRUE;
		OutPutParams.SPDIFOutParams.AutoCategoryCode				= TRUE;
		OutPutParams.SPDIFOutParams.CategoryCode					= 0;
		OutPutParams.SPDIFOutParams.AutoDTDI						= TRUE;
		OutPutParams.SPDIFOutParams.CopyPermitted					= STAUD_COPYRIGHT_MODE_ONE_COPY;
		OutPutParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 128;
		OutPutParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode		= STAUD_SPDIF_DATA_PRECISION_24BITS;
		OutPutParams.SPDIFOutParams.Emphasis						= STAUD_SPDIF_EMPHASIS_CD_TYPE;
		switch(chip->iec_encoding_mode)
		{
		// Set the Latency and Freq Multiplier Stuff 
		// Although Freq Muti for SPDIF is 128 ,,but just 
		 default:
		 	break;
		}
		Error = STAUD_IPSetPCMParams(chip->AudioHandle,STAUD_OBJECT_INPUT_CD2,&InputInterface);
		//Error |=  STAUD_OPSetParams(chip->AudioHandle,STAUD_OBJECT_OUTPUT_SPDIF0,&OutPutParams);
	}
	if(Error!=0)
	{
		printk( " STAUD_IPSetPCMParams Failed Error Code = %d and device is =%d \n", Error, chip->card_data->major);
		return -1;
	}
	chip->are_clocks_active = 1;
	snd_pcm_lib_buffer_bytes(substream);
//	spin_unlock(&chip->lock);
	return Error;

}

static U8 ALSACommand[4][1]={{0},{0},{0},{0}};
static task_t * alsaPlayerTask_p=NULL;
static  semaphore_t * alsaPlayerTaskSem_p=NULL;
static semaphore_t * alsaOpenSem_p=NULL;
static BOOL CommandTerminate = FALSE;

#ifdef STAUDLX_ALSA_TEST
extern int SystemStartUp(void);
static BOOL SystemStarted = FALSE;
#endif 

static int stb7100_pcm_free(pcm_hw_t *card)
{

	DEBUG_PRINT(("Freeing Allocated Memory \n"));
	if(alsaOpenSem_p)
	{
		DEBUG_PRINT(("Freeing Protection Semaphore \n"));
		STOS_SemaphoreDelete(NULL,alsaOpenSem_p);
		alsaOpenSem_p = NULL;
	}
	if(alsaPlayerTask_p)
	{
		CommandTerminate = TRUE;
		DEBUG_PRINT(("Stopping Task \n"));
		STOS_SemaphoreSignal(alsaPlayerTaskSem_p);
		STOS_TaskDelayUs(1000); 
		/*STOS_TaskWait(&alsaPlayerTask_p,TIMEOUT_INFINITY);*/
	       STOS_TaskDelete(alsaPlayerTask_p,NULL,NULL,NULL);	
		  DEBUG_PRINT(("Stopped Task \n"));
		alsaPlayerTask_p= NULL;
	}
	
	/*Remove clock recovery and EVt Module here*/
#ifdef STAUDLX_ALSA_TEST
	{	
		if(SystemStarted == TRUE)
		{
			STCLKRV_TermParams_t ClockTerm;
			STEVT_TermParams_t EvtTerm;
			ST_ErrorCode_t 	Error=ST_NO_ERROR;
			ClockTerm.ForceTerminate = TRUE;
			EvtTerm.ForceTerminate   = TRUE;
			Error = STCLKRV_Term("STCLK0", &ClockTerm) ;
			if(ST_NO_ERROR!=Error)
				printk("Clock Recovery Term Error %d \n",Error );

			Error = STEVT_Term("EVT0", &EvtTerm) ; 
			if(Error != ST_NO_ERROR)
				printk("Event Term Error %d \n",Error);
			
			SystemStarted= FALSE;
		}
			
	}
#endif 	
	if(alsaPlayerTaskSem_p)
	{
		DEBUG_PRINT(("Freeing Task Semaphore \n"));
		STOS_SemaphoreDelete(NULL,alsaPlayerTaskSem_p);
		alsaPlayerTaskSem_p= NULL;
	}
	
	kfree(card);

	return 0;
}


static unsigned int stb7100_pcm_channels[] = { 2,4,6,8,10 };


static snd_pcm_hw_constraint_list_t stb7100_constraints_channels = {
		.count = ARRAY_SIZE(stb7100_pcm_channels),
		.list = stb7100_pcm_channels,
		.mask = 0
};

// Command Proto type Command[DeviceID][Command] 
// Presently only 1 command is supported 
int ALSAPlayerTask(void * Params);
int ALSAPlayerTask(void *Params)
{
	ST_ErrorCode_t Error=ST_NO_ERROR;
	U8 DeviceID=0;
	STOS_TaskEnter(NULL);
	
	while(1)
	{
		STOS_SemaphoreWait(alsaPlayerTaskSem_p);

		{
				STAUD_TermParams_t Term;
				
				if(ALSACommand[0][0]==1)
				{
					DEBUG_PRINT(("Terminating Audio 0 \n"));	
					Error=STAUD_Term ("AUDALSA", &Term);
					ALSACommand[0][0] = 0;
					DEBUG_PRINT(( "Terminated Audio 0 Error =%d\n", Error));	
					DeviceID =0;
				}
				else if (ALSACommand[1][0]==1)
				{
					DEBUG_PRINT(( KERN_ALERT "Terminating Audio 1 \n"));	
					Error=STAUD_Term ("AUDALSA1", &Term);
					ALSACommand[1][0] = 0;
					DEBUG_PRINT(("Terminated Audio 1 Error =%d \n", Error));	
					DeviceID =1;
				
				}
				else if (ALSACommand[2][0]==1)
				{
					DEBUG_PRINT(( KERN_ALERT "Terminating Audio 2 \n"));	
					Error=STAUD_Term ("AUDALSA2", &Term);
					ALSACommand[2][0] = 0;
					DEBUG_PRINT(( KERN_ALERT "Terminated Audio 2 Error =%d \n", Error));	
					DeviceID =2;
				
				}
				else if(ALSACommand[3][0]==1)
				{
					DEBUG_PRINT(( KERN_ALERT "Terminating Audio 3 \n"));	
					Error=STAUD_Term ("AUDALSA3", &Term);
					ALSACommand[3][0] = 0;
					DEBUG_PRINT(( KERN_ALERT "Terminated Audio 3 Error =%d \n", Error));	
					DeviceID =3;
				
				}
				if(Error!=0)
				{
					printk(KERN_ALERT "STAUD Term Failed Erorr =%d and Device is =%d ", Error, DeviceID);
				}
				if(CommandTerminate==TRUE)
				{
						DEBUG_PRINT(("ALSA Task Stop \n"));
						/*STOS_TaskExit(NULL);*/
						return 0;
				}

		}

	}
}
	
static int stb7100_pcm_open(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
//	void              *syscfg;
  //  	unsigned long      chiprev;
    	int                err;
	STAUD_OpenParams_t OpenParams;
	STAUD_InitParams_t	AUD_InitParams;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);

	if(alsaPlayerTask_p==NULL)
	{

		alsaPlayerTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);
		alsaOpenSem_p= STOS_SemaphoreCreateFifo(NULL,1);
		err= STOS_TaskCreate(ALSAPlayerTask,NULL,NULL,16*1024,NULL,NULL,
                                        &alsaPlayerTask_p,NULL,10, "ALSAPlayerTask", 0);
		
	}

	OpenParams.SyncDelay = 0;
	STOS_SemaphoreWait(alsaOpenSem_p);
	if(chip->card_data->major == PCM0_DEVICE)
	{
		memset(&AUD_InitParams      ,0,sizeof(STAUD_InitParams_t));
		AUD_InitParams.DeviceType                                   = STAUD_DEVICE_STI7100;
		AUD_InitParams.Configuration                                = STAUD_CONFIG_DVB_COMPACT; 
		AUD_InitParams.DACClockToFsRatio                            = 256; 

		AUD_InitParams.PCMOutParams.InvertWordClock                 = FALSE; 
		AUD_InitParams.PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
		AUD_InitParams.PCMOutParams.InvertBitClock                  = FALSE;
		AUD_InitParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_16BITS; 
		AUD_InitParams.PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT;
		AUD_InitParams.PCMOutParams.MemoryStorageFormat16 = FALSE; // Udit 
		AUD_InitParams.PCMOutParams.MSBFirst                        = TRUE;
		AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier	= 256;
		
		AUD_InitParams.SPDIFOutParams.AutoLatency					= TRUE;
		AUD_InitParams.SPDIFOutParams.AutoCategoryCode				= TRUE;
		AUD_InitParams.SPDIFOutParams.CategoryCode					= 0;
		AUD_InitParams.SPDIFOutParams.AutoDTDI						= TRUE;
		AUD_InitParams.SPDIFOutParams.CopyPermitted					= STAUD_COPYRIGHT_MODE_ONE_COPY;
		AUD_InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 128;
		AUD_InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode		= STAUD_SPDIF_DATA_PRECISION_24BITS;
		AUD_InitParams.SPDIFOutParams.Emphasis						= STAUD_SPDIF_EMPHASIS_CD_TYPE;
		AUD_InitParams.MaxOpen                                      = 1;
		/*To discussed with Chadan */
		AUD_InitParams.CPUPartition_p                               = (ST_Partition_t *)-1;

		AUD_InitParams.AVMEMPartition								= STAVMEM_GetPartitionHandle(0);;
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;

#if MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= AVMEM_PartitionHandle[1];
		AUD_InitParams.AllocateFromEMBX								= FALSE;
#else //MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;
#endif //MSPP_SUPPORT

		AUD_InitParams.DriverIndex				    		  =19 ; /*Let's 19 is kept for ALSA, Initial test*/

		AUD_InitParams.PCMMode                                      = PCM_ON;
		AUD_InitParams.SPDIFMode                                    =  STAUD_DIGITAL_MODE_NONCOMPRESSED; /* STAUD_DIGITAL_MODE_COMPRESSED; */
		AUD_InitParams.NumChannels                                  = 2;


		strcpy(AUD_InitParams.EvtHandlerName,"EVT0");
		strcpy(AUD_InitParams.ClockRecoveryName,"STCLK0");
		DEBUG_PRINT(("ST Init called  PCMP 0\n"));
		if((err = STAUD_Init( "AUDALSA", &AUD_InitParams ))>0) 
		{
			printk(KERN_ALERT ">>> Failed STAUDInit_PCMP0 %d \n", err);
		      // stb7100_pcm_free(chip);
			STOS_SemaphoreSignal(alsaOpenSem_p);
			return -1;
		}

	
    	    	DEBUG_PRINT(("STALSA Init external dac PCMP0 Success \n"));
		err=STAUD_Open("AUDALSA",&OpenParams,&chip->AudioHandle);
		

		
	}
	else if (chip->card_data->major == PCM1_DEVICE)
	{
		memset(&AUD_InitParams      ,0,sizeof(STAUD_InitParams_t));
		AUD_InitParams.DeviceType                                   = STAUD_DEVICE_STI7100;
		AUD_InitParams.Configuration                                = STAUD_CONFIG_DVB_COMPACT; 
		AUD_InitParams.DACClockToFsRatio                            = 256; 
		AUD_InitParams.PCMOutParams.InvertWordClock                 = FALSE; 

		AUD_InitParams.PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
		AUD_InitParams.PCMOutParams.InvertBitClock                  = FALSE;
		AUD_InitParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_16BITS; // Udit 
		AUD_InitParams.PCMOutParams.MemoryStorageFormat16 = FALSE; // Udit 
		AUD_InitParams.PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT;
		AUD_InitParams.PCMOutParams.MSBFirst                        = TRUE;
		AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier	= 256;

		
		AUD_InitParams.SPDIFOutParams.AutoLatency					= TRUE;
		AUD_InitParams.SPDIFOutParams.AutoCategoryCode				= TRUE;
		AUD_InitParams.SPDIFOutParams.CategoryCode					= 0;
		AUD_InitParams.SPDIFOutParams.AutoDTDI						= TRUE;
		AUD_InitParams.SPDIFOutParams.CopyPermitted					= STAUD_COPYRIGHT_MODE_ONE_COPY;
		AUD_InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 128;
		AUD_InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode		= STAUD_SPDIF_DATA_PRECISION_24BITS;
		AUD_InitParams.SPDIFOutParams.Emphasis						= STAUD_SPDIF_EMPHASIS_CD_TYPE;
		AUD_InitParams.MaxOpen                                      = 1;
		/*To discussed with Chadan */
		AUD_InitParams.CPUPartition_p                               = (ST_Partition_t *)-1;

		AUD_InitParams.AVMEMPartition								= STAVMEM_GetPartitionHandle(0);;
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;

#if MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= AVMEM_PartitionHandle[1];
		AUD_InitParams.AllocateFromEMBX								= FALSE;
#else //MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;
#endif //MSPP_SUPPORT

		AUD_InitParams.DriverIndex				    		  =20; /*Let's 14 is kept for ALSA, Initial test*/

		AUD_InitParams.PCMMode                                      = PCM_ON;
		AUD_InitParams.SPDIFMode                                    =  STAUD_DIGITAL_MODE_NONCOMPRESSED; /* STAUD_DIGITAL_MODE_COMPRESSED; */
		AUD_InitParams.NumChannels                                  = 2;


		strcpy(AUD_InitParams.EvtHandlerName,"EVT0");
		strcpy(AUD_InitParams.ClockRecoveryName,"STCLK0");
		DEBUG_PRINT(("ST Init called  PCMP 1\n"));
		if((err = STAUD_Init( "AUDALSA1", &AUD_InitParams ))>0) 
		{
			printk(KERN_ALERT ">>> Failed STAUDInit_PCMP1 %d \n", err);
		       //stb7100_pcm_free(chip);
		   	STOS_SemaphoreSignal(alsaOpenSem_p);
			return -1;
		}
		DEBUG_PRINT(("STALSA Init internal dac PCMP1 Successful \n"));
		err=STAUD_Open("AUDALSA1",&OpenParams,&chip->AudioHandle);
	}
	else if (chip->card_data->major == SPDIF_DEVICE)
	{
		memset(&AUD_InitParams      ,0,sizeof(STAUD_InitParams_t));

		//chip->hw = stb7100_spdif_hw;
		AUD_InitParams.DeviceType                                   = STAUD_DEVICE_STI7100;
		AUD_InitParams.Configuration                                = STAUD_CONFIG_DVB_COMPACT; 
		AUD_InitParams.DACClockToFsRatio                            = 256; 

		AUD_InitParams.PCMOutParams.InvertWordClock                 = FALSE; 
		AUD_InitParams.PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
		AUD_InitParams.PCMOutParams.InvertBitClock                  = FALSE;
		AUD_InitParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_16BITS; // Udit 
		AUD_InitParams.PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT;
		AUD_InitParams.PCMOutParams.MemoryStorageFormat16 = FALSE; // Udit 
		AUD_InitParams.PCMOutParams.MSBFirst                        = TRUE;
		AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier	= 256;
		
		AUD_InitParams.SPDIFOutParams.AutoLatency					= TRUE;
		AUD_InitParams.SPDIFOutParams.AutoCategoryCode				= TRUE;
		AUD_InitParams.SPDIFOutParams.CategoryCode					= 0;
		AUD_InitParams.SPDIFOutParams.AutoDTDI						= TRUE;
		AUD_InitParams.SPDIFOutParams.CopyPermitted					= STAUD_COPYRIGHT_MODE_ONE_COPY;
		AUD_InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 128;
		AUD_InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode		= STAUD_SPDIF_DATA_PRECISION_24BITS;
		AUD_InitParams.SPDIFOutParams.Emphasis						= STAUD_SPDIF_EMPHASIS_CD_TYPE;
		AUD_InitParams.MaxOpen                                      = 1;
		/*To discussed with Chadan */
		AUD_InitParams.CPUPartition_p                               = (ST_Partition_t *)-1;

		AUD_InitParams.AVMEMPartition								= STAVMEM_GetPartitionHandle(0);;
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;

#if MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= AVMEM_PartitionHandle[1];
		AUD_InitParams.AllocateFromEMBX								= FALSE;
#else //MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;
#endif //MSPP_SUPPORT

		AUD_InitParams.DriverIndex				    		  =21; /*Let's 10 is kept for ALSA, Initial test*/

		AUD_InitParams.PCMMode                                      = PCM_ON;
		AUD_InitParams.SPDIFMode                                    =  STAUD_DIGITAL_MODE_NONCOMPRESSED; /* STAUD_DIGITAL_MODE_COMPRESSED; */
		AUD_InitParams.NumChannels                                  = 2;


		strcpy(AUD_InitParams.EvtHandlerName,"EVT0");
		strcpy(AUD_InitParams.ClockRecoveryName,"STCLK0");
		DEBUG_PRINT(("ST Init called  SPDIF \n"));
		if((err = STAUD_Init( "AUDALSA2", &AUD_InitParams ))>0) 
		{
			printk(KERN_ALERT ">>> Failed STAUDInit_SPDIFP %d \n", err);
		       // stb7100_pcm_free(chip);
		       STOS_SemaphoreSignal(alsaOpenSem_p);
			return -1;
		}

		DEBUG_PRINT(("STALSA Init SPDIF Successful \n"));
		err=STAUD_Open("AUDALSA2",&OpenParams,&chip->AudioHandle);
	}

	
	if(chip->AudioHandle==0 || err !=0)
	{
		printk(KERN_ALERT "STAUD_OpenFailed Error = %d and Device is =%d ", err, chip->card_data->major);
		return -1;
	}
	
	runtime->hw.channels_min = 1; // UL Debug 

	
	err = snd_pcm_hw_constraint_list(substream->runtime, 0,
					 SNDRV_PCM_HW_PARAM_CHANNELS,
					 &stb7100_constraints_channels);
	STOS_SemaphoreSignal(alsaOpenSem_p);
	return err;
}


static stm_playback_ops_t stb7100_pcm_ops = {
	.free_device      = stb7100_pcm_free,
	.open_device      = stb7100_pcm_open,
	.program_hw       = stb7100_pcm_program_hw,
	.playback_pointer = stb7100_fdma_playback_pointer,
	.start_playback   = stb7100_pcm_start_playback,
	.stop_playback    = stb7100_pcm_stop_playback,
	.pause_playback   = stb7100_pcm_pause_playback,
	.unpause_playback = stb7100_pcm_unpause_playback
};

static void stb7100_capture_stop_playback(snd_pcm_substream_t *substream)
{
 	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	STAUD_Stop_t StopMode;
	U32 Error=0;
	STAUD_Fade_t Fade;
	StopMode = STAUD_STOP_NOW;
	Fade.FadeType = STAUD_FADE_SOFT_MUTE;

	//spin_lock(&chip->lock);
	DEBUG_PRINT((" Debug : stb7100_capture_stop_playback Unlocking Stream \n "));
	snd_pcm_stream_unlock_irq(substream);
	DEBUG_PRINT((" Debug : Unlocked Stream And DR Stop \n "));
	Error = STAUD_DRStop(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED0,StopMode,&Fade);
	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_DRStop Failed Error code =%d Device =%d \n",Error,chip->card_data->major);

	}
	snd_pcm_stream_lock_irq(substream);
	
}


static void stb7100_capture_start_playback(snd_pcm_substream_t *substream)
{
	pcm_hw_t     *chip = snd_pcm_substream_chip(substream);
	STAUD_StreamParams_t StartParams;
	U32 Error = 0;
	StartParams.StreamContent = STAUD_STREAM_CONTENT_ALSA; // STAUD_STREAM_CONTENT_MAX;  // For ALSA Added in STAUDLX // 
	StartParams.StreamType= STAUD_STREAM_TYPE_ES;
	StartParams.SamplingFrequency = substream->runtime->rate;

	snd_pcm_stream_unlock_irq(substream);
	Error=STAUD_DRStart(chip->AudioHandle,STAUD_OBJECT_DECODER_COMPRESSED0,&StartParams);
	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_DRSTART Failed Error code =%d and device is =%d \n",Error,chip->card_data->major);
	}	
	snd_pcm_stream_lock_irq(substream); 
//	spin_unlock(&chip->lock);
}


static void stb7100_capture_unpause_playback(snd_pcm_substream_t *substream)
{
 	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
//	unsigned long reg=0;
	U32 Error=0;

//    spin_lock(&chip->lock);
	if(chip->card_data->major==CAPTURE_DEVICE)
	{
		
			Error=STAUD_DRResume(chip->AudioHandle,STAUD_OBJECT_INPUT_PCMREADER0);
	}
	if(Error!=0)
	{
		printk( KERN_ALERT "STAUD_DRResume Failed Error code =%d and Device is =%d \n",Error,chip->card_data->major);
	}
	//spin_unlock(&chip->lock);
}


static void stb7100_capture_pause_playback(snd_pcm_substream_t *substream)
{
     pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	 STAUD_Fade_t Fade;
	
	unsigned int Error=0;
	Fade.FadeType = STAUD_FADE_SOFT_MUTE;

	//spin_lock(&chip->lock);

	if(chip->card_data->major==CAPTURE_DEVICE)
	{
			Error=STAUD_DRPause(chip->AudioHandle,STAUD_OBJECT_INPUT_PCMREADER0,&Fade);
	}
	
	


	if(Error!=0)
	{
		printk(KERN_ALERT "STAUD_DRStop Paused Error code =%d and Device is =%d \n",Error,chip->card_data->major);
	}
//	spin_unlock(&chip->lock);
}


static snd_pcm_uframes_t stb7100_fdma_capture_pointer(snd_pcm_substream_t * substream)
{
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	/*
	 * Calculate our current playback position, using the number of bytes
	 * left for the DMA engine needs to transfer to complete a full
	 * iteration of the buffer. This is common to all STb7100 audio players
	 * using the FDMA (including SPDIF).
	 */
	
	// Uk unsigned int  pos=  (unsigned int)chip->WritePointer- (unsigned int)substream->runtime->dma_addr;	
	unsigned int  pos=  (unsigned int)chip->WritePointer- (unsigned int)chip->BaseAddress;	
	DEBUG_PRINT(("Pointer Callback bytes =%x, ret value =%x \n", pos, bytes_to_frames(substream->runtime,pos)));
	return bytes_to_frames(substream->runtime,pos);
}





static int stb7100_capture_program_hw(snd_pcm_substream_t *substream)
{
	
	U32 Error=0;


	pcm_hw_t *chip = snd_pcm_substream_chip(substream);
	STAUD_PCMReaderConf_t PCMReaderMode;


	/* InputInterface.NumChannels = substream->runtime->channels;*/

	PCMReaderMode.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT ;

	if(substream->runtime->format ==SNDRV_PCM_FORMAT_S32_LE)
	{
		PCMReaderMode.BitsPerSubFrame =  STAUD_DAC_NBITS_SUBFRAME_32 ; 
		PCMReaderMode.Precision = STAUD_DAC_DATA_PRECISION_24BITS ; 
		PCMReaderMode.MemFormat	 = STAUD_PCMR_BITS_16_0 ;
		DEBUG_PRINT(("Reader Format 32 bits \n "));
	}
	else if(substream->runtime->format ==SNDRV_PCM_FORMAT_S16_LE)
	{
		PCMReaderMode.BitsPerSubFrame =  STAUD_DAC_NBITS_SUBFRAME_32 ; 
		PCMReaderMode.Precision = STAUD_DAC_DATA_PRECISION_16BITS ; 
		PCMReaderMode.MemFormat	 = STAUD_PCMR_BITS_16_16 ;
		DEBUG_PRINT(("Reader Format 16 bits \n "));
	}
	
	/*PCMReaderMode.FallingSCLK = TRUE;*/
	PCMReaderMode.FallingSCLK = FALSE;
	PCMReaderMode.Frequency =  substream->runtime->rate;
	/*PCMReaderMode.LeftLRHigh =  TRUE;*/
	PCMReaderMode.LeftLRHigh =  FALSE;
	PCMReaderMode.MSBFirst = TRUE;
	PCMReaderMode.Padding = TRUE; 
	PCMReaderMode.Rounding = STAUD_NO_ROUNDING_PCMR; 
	
	Error = STAUD_IPSetPCMReaderParams(chip->AudioHandle,STAUD_OBJECT_INPUT_PCMREADER0,&PCMReaderMode);
	if(Error)
	{
		printk(KERN_ALERT "STAUD_IPSetPCMReaderParams Failed Error=%d \n ",Error );
	}
	DEBUG_PRINT(("STAUD_IPSetPCMReaderParams  Error=%d \n ",Error ));
		
	return Error;



}



static int stb7100_capture_open(snd_pcm_substream_t *substream)
{
//	snd_pcm_runtime_t *runtime = substream->runtime;
//	void              *syscfg;
   // 	unsigned long      chiprev;
    	int                err;
	STAUD_OpenParams_t OpenParams;
	STAUD_InitParams_t	AUD_InitParams;
	pcm_hw_t *chip = snd_pcm_substream_chip(substream);

	if(alsaPlayerTask_p==NULL)
	{

		alsaPlayerTaskSem_p = STOS_SemaphoreCreateFifo(NULL,0);
		alsaOpenSem_p= STOS_SemaphoreCreateFifo(NULL,1);
		err= STOS_TaskCreate(ALSAPlayerTask,NULL,NULL,16*1024,NULL,NULL,
                                        &alsaPlayerTask_p,NULL,10, "ALSAPlayerTask", 0);
		
	}


	STOS_SemaphoreWait(alsaOpenSem_p);
	if(chip->card_data->major == CAPTURE_DEVICE)
	{
		memset(&AUD_InitParams      ,0,sizeof(STAUD_InitParams_t));
		AUD_InitParams.DeviceType                                   = STAUD_DEVICE_STI7100;
		AUD_InitParams.Configuration                                = STAUD_CONFIG_DVB_COMPACT; 
		AUD_InitParams.DACClockToFsRatio                            = 256; 



		AUD_InitParams.PCMReaderMode.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT ;
		AUD_InitParams.PCMReaderMode.BitsPerSubFrame =  STAUD_DAC_NBITS_SUBFRAME_32 ; 
		/*AUD_InitParams.PCMReaderMode.FallingSCLK = TRUE;*/
		AUD_InitParams.PCMReaderMode.FallingSCLK = FALSE;
		AUD_InitParams.PCMReaderMode.Frequency =  48000;
		/*AUD_InitParams.PCMReaderMode.LeftLRHigh =  TRUE;*/
		AUD_InitParams.PCMReaderMode.LeftLRHigh =  FALSE;
		AUD_InitParams.PCMReaderMode.MSBFirst = TRUE;
		AUD_InitParams.PCMReaderMode.Padding = TRUE; 
		AUD_InitParams.PCMReaderMode.Precision = STAUD_DAC_DATA_PRECISION_24BITS ; 
		AUD_InitParams.PCMReaderMode.MemFormat = STAUD_PCMR_BITS_16_0 ; 
		AUD_InitParams.PCMReaderMode.Rounding   = STAUD_NO_ROUNDING_PCMR; 

		
		AUD_InitParams.MaxOpen                                      = 1;
		/*To discussed with Chadan */
		AUD_InitParams.CPUPartition_p                               = (ST_Partition_t *)-1;

		AUD_InitParams.AVMEMPartition								= STAVMEM_GetPartitionHandle(0);;
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;

#if MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= AVMEM_PartitionHandle[1];
		AUD_InitParams.AllocateFromEMBX								= FALSE;
#else //MSPP_SUPPORT
		AUD_InitParams.BufferPartition								= 0;
		AUD_InitParams.AllocateFromEMBX								= TRUE;
#endif //MSPP_SUPPORT

		AUD_InitParams.DriverIndex				    		  =24 ; /*Let's 22, For ALSA */

		AUD_InitParams.NumChannels                                  = 2;


		strcpy(AUD_InitParams.EvtHandlerName,"EVT0");
		strcpy(AUD_InitParams.ClockRecoveryName,"STCLK0");
		DEBUG_CAPTURE_CALLS(("ST Init called  Capture \n"));
		if((err = STAUD_Init( "AUDALSA3", &AUD_InitParams ))>0) 
		{
			printk(KERN_ALERT ">>> Failed STAUDInit_Capture %d \n", err);
		      // stb7100_pcm_free(chip);
			STOS_SemaphoreSignal(alsaOpenSem_p);
			return -1;
		}

	
    	    	DEBUG_CAPTURE_CALLS(("STALSA Init Capture Success \n"));
		err=STAUD_Open("AUDALSA3",&OpenParams,&chip->AudioHandle);
		
		if(chip->AudioHandle==0 || err !=0)
		{
			printk(KERN_ALERT "STAUD_OpenFailed Error = %d and Device is =%d ", err, chip->card_data->major);
			return -1;
		}
	

		
	}
	STOS_SemaphoreSignal(alsaOpenSem_p);
	return err;
}

static stm_playback_ops_t stb7100_capture_ops = {
	.free_device      = stb7100_pcm_free,
	.open_device      = stb7100_capture_open,
	.program_hw       = stb7100_capture_program_hw,
	.playback_pointer = stb7100_fdma_capture_pointer,
	.start_playback   = stb7100_capture_start_playback,
	.stop_playback    = stb7100_capture_stop_playback,
	.pause_playback   = stb7100_capture_pause_playback,
	.unpause_playback = stb7100_capture_unpause_playback
};

static int main_device_allocate(snd_card_t *card, stm_snd_output_device_t *dev_data, pcm_hw_t *stb7100 )
{
	if(!card)
		return -EINVAL;

//	spin_lock_init(&stb7100->lock);

	stb7100->card          = card;
	stb7100->irq           = -1;
	stb7100->card_data     = dev_data;
	
	return 0;
}

static snd_device_ops_t ops = {
    .dev_free = snd_pcm_dev_free,
};

static int stb7100_create_lpcm_device(pcm_hw_t *chip,snd_card_t **this_card)
{
	int err = 0;

	chip->pcm_converter = 0;
    	chip->hw            = stb7100_pcm_hw;
	chip->oversampling_frequency = 256;

	chip->playback_ops  = &stb7100_pcm_ops;


	
	if(chip->card_data->major == SPDIF_DEVICE)
	{
		sprintf((*this_card)->shortname, "STb7100_SPDIF");
		sprintf((*this_card)->longname,  "STb7100_SPDIF");
		chip->iec60958_rawmode = TRUE;

	}
	else if (chip->card_data->major == CAPTURE_DEVICE)
	{
		sprintf((*this_card)->shortname, "STb7100_CAPTURE");
		sprintf((*this_card)->longname,  "STb7100_CAPTURE");
		chip->playback_ops  = &stb7100_capture_ops;
		chip->hw            = stb7100_capture_hw;
	}
	else 
	{
		sprintf((*this_card)->shortname, "STb7100_PCM%d",chip->card_data->major);
		sprintf((*this_card)->longname,  "STb7100_PCM%d",chip->card_data->major );
	}
	sprintf((*this_card)->driver,    "%d",chip->card_data->major);
	
#ifdef STAUDLX_ALSA_TEST
	if(SystemStarted == FALSE)
		{
		printk("Starting System \n");
		SystemStartUp();
		printk(" System Started \n");
		}

	SystemStarted = TRUE;
#endif 


    	switch(chip->card_data->major){
	        case PCM0_DEVICE:
			chip->hw = stb7100_pcm_hw;
			
			break;
		case PCM1_DEVICE:
			
			chip->hw = stb7100_pcm_hw;
			
			break;

		case SPDIF_DEVICE:
			
			iec60958_default_channel_status(chip);
			snd_iec60958_create_controls(chip);
        	    	
			break;

			case CAPTURE_DEVICE:
				chip->hw = stb7100_capture_hw;
				break;

		default:
			printk(KERN_ERR "STb7100 ALSA create dev not supported!\n ");
    	}

	if(chip->card_data->major==CAPTURE_DEVICE)
	{
		if((err = snd_card_capture_allocate(chip,chip->card_data->minor,(*this_card)->longname)) < 0)
		{
		 	printk(">>> Failed to create Capture stream \n");
			stb7100_pcm_free(chip);
	    	}
	}
	else 
	{
		snd_generic_create_controls(chip);

		if((err = snd_card_pcm_allocate(chip,chip->card_data->minor,(*this_card)->longname)) < 0)
		{
		        	printk(">>> Failed to create PCM stream \n");
			        stb7100_pcm_free(chip);
	    	}
	}
    	
	if((err = snd_device_new((*this_card), SNDRV_DEV_LOWLEVEL,chip, &ops)) < 0)
	{
		printk(">>> creating sound device :%d,%d failed\n",chip->card_data->major,chip->card_data->minor);
		stb7100_pcm_free(chip);
		return err;
	}
	if ((err = snd_card_register((*this_card))) < 0) {
		stb7100_pcm_free(chip);
		return err;
	}

	return 0;
}

static struct platform_device * pcm0_platform_device;
static struct platform_device * pcm1_platform_device;
static struct platform_device * spdif_platform_device;
static struct platform_device * capture_platform_device;
#if 0
static int stb7100_platform_alsa_probe(struct device *dev); 
#endif 
void  stb7100_platform_alsa_release(struct device  * dev);

static struct device_driver alsa_capture_driver = {
	.name  = "7100_ALSA_CAPTURE",
	.owner = THIS_MODULE,
	.bus   = &platform_bus_type,


};

static struct device_driver alsa_pcm0_driver = {
	.name  = "7100_ALSA_PCM0",
	.owner = THIS_MODULE,
	.bus   = &platform_bus_type,



};

static struct device_driver alsa_pcm1_driver = {
	.name  = "7100_ALSA_PCM1",
	.owner = THIS_MODULE,
	.bus   = &platform_bus_type,


	
};
static struct device_driver alsa_spdif_driver = {
	.name  = "7100_ALSA_SPD",
	.owner = THIS_MODULE,
	.bus   = &platform_bus_type,



};


static struct device alsa_pcm1_device = {
	.bus_id="alsa_7100_pcm1",
	.driver = &alsa_pcm1_driver,
	.parent   = &platform_bus ,
	.bus      = &platform_bus_type,
	.release  = stb7100_platform_alsa_release,
};

static struct device alsa_pcm0_device = {
	.bus_id="alsa_7100_pcm0",
	.driver = &alsa_pcm0_driver,
	.parent   = &platform_bus ,
	.bus      = &platform_bus_type,
	.release  = stb7100_platform_alsa_release,
};
static struct device alsa_spdif_device = {
	.bus_id="alsa_7100_spdif",
	.driver = &alsa_spdif_driver,
	.parent   = &platform_bus ,
	.bus      = &platform_bus_type,
	.release  = stb7100_platform_alsa_release,
};
static struct device alsa_capture_device = {
	.bus_id="alsa_7100_capture",
	.driver = &alsa_capture_driver,
	.parent   = &platform_bus ,
	.bus      = &platform_bus_type,
	.release  = stb7100_platform_alsa_release,
};



#if 0
static int __init stb7100_platform_alsa_probe(struct device *dev)
{

	printk(KERN_ALERT "stb7100_platform_alsa_probe Called \n");
	if(strcmp(dev->bus_id,alsa_pcm0_driver.name)==0)
	        pcm0_platform_device = to_platform_device(dev);

	else if(strcmp(dev->bus_id,alsa_pcm1_driver.name)==0)
	        pcm1_platform_device = to_platform_device(dev);

	else if(strcmp(dev->bus_id,alsa_spdif_driver.name)==0)
	        spdif_platform_device = to_platform_device(dev);

	else if(strcmp(dev->bus_id,alsa_capture_driver.name)==0)
	        capture_platform_device = to_platform_device(dev);

	else return -EINVAL;

        return 0;
}
#endif 
void  stb7100_platform_alsa_release(struct device  * dev)
{
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : >>> stb7100_platform_alsa_release()  \n "));
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : <<<  stb7100_platform_alsa_release()  \n "));
}

static int snd_pcm_card_unregister( int dev)
{
	int err=0;
	struct device_driver *  dev_driver;
	struct device * device;
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : >>> snd_pcm_card_unregister()  \n "));
	switch(dev){
		case PCM0_DEVICE:
			dev_driver= 	&alsa_pcm0_driver;
			device =  	&alsa_pcm0_device;
			break;
		case PCM1_DEVICE:
			dev_driver= 	&alsa_pcm1_driver;
			device =  	&alsa_pcm1_device;
			break;
		case SPDIF_DEVICE:
			dev_driver= 	&alsa_spdif_driver;
			device =  	&alsa_spdif_device;
			break;
		case CAPTURE_DEVICE:
			dev_driver= 	&alsa_capture_driver;
			device =  	&alsa_capture_device;
			break;
		default:
			printk(KERN_ALERT "STAUD ALSA : >>> EINVAL snd_pcm_card_unregister() Invalid Device \n");
			return -EINVAL;
	}
	DEBUG_PRINT((" STAUD ALSA : >>> snd_pcm_card_unregister()  device_unregister  \n"));
	device_unregister(device);
	DEBUG_PRINT((" STAUD ALSA : >>> snd_pcm_card_unregister()  device_unregister  OK \n"));
	
	driver_unregister(dev_driver);
	DEBUG_PRINT((" STAUD ALSA : >>> snd_pcm_card_unregister()  driver_unregister  OK \n"));

	
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : <<< snd_pcm_card_unregister()  \n "));
	return 0;
}
static int register_platform_driver(struct platform_device *platform_dev,pcm_hw_t *chip)
{
	static struct resource *res;
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : >>> register_platform_driver()  \n "));
	if (!platform_dev)
	{
       		printk("%s Failed. Check your kernel SoC config\n",__FUNCTION__);
       	  	return -EINVAL;
       }

	res = platform_get_resource(platform_dev, IORESOURCE_IRQ,0);    /*resource 0 */
	if(res!=NULL)
	{
		chip->min_ch = res->start;
		chip->max_ch = res->end;
	}
	else
	{
		printk(KERN_ALERT "STAUD ALSA: <<< ENOSYS register_platform_driver IRQ 0\n");
		return -ENOSYS;
	}

	res = platform_get_resource(platform_dev, IORESOURCE_IRQ,1);
	if(res==NULL)
	{
		printk(KERN_ALERT "STAUD ALSA: <<< ENOSYS register_platform_driver() IRQ  1 \n ");
		 return -ENOSYS;
	}

	DEBUG_PRINT_FN_CALL(("STAUD ALSA : <<< register_platform_driver()  \n "));
	return 0;
}



static int snd_pcm_card_generic_probe(snd_card_t ** card, pcm_hw_t * chip, int dev)
{
	int err=0;
	struct device_driver *  dev_driver;
	struct device * device;
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : >>> snd_pcm_card_generic_probe() dev=%d \n ",dev));
	switch(dev){
		case PCM0_DEVICE:
			dev_driver= 	&alsa_pcm0_driver;
			device =  	&alsa_pcm0_device;
			break;
		case PCM1_DEVICE:
			dev_driver= 	&alsa_pcm1_driver;
			device =  	&alsa_pcm1_device;
			break;
		case SPDIF_DEVICE:
			dev_driver= 	&alsa_spdif_driver;
			device =  	&alsa_spdif_device;
			break;
		case CAPTURE_DEVICE:
			dev_driver= 	&alsa_capture_driver;
			device =  	&alsa_capture_device;
			break;
		default:
			printk(KERN_ALERT  "STAUD ALSA : <<< EINVAL  snd_pcm_card_generic_probe() Invalid Device \n ");	
			return -EINVAL;
	}
	DEBUG_PRINT(("STAUD ALSA: snd_pcm_card_generic_probe () driver register \n "));
	if(driver_register(dev_driver)==0)
	{
		DEBUG_PRINT(("STAUD ALSA: snd_pcm_card_generic_probe () device  register \n "));
		if(device_register(device)!=0)
		{
			printk(KERN_ALERT  "STAUD ALSA : <<< ENOSYS  snd_pcm_card_generic_probe()  Device registration  failed \n ");	
			return -ENOSYS;
		}
	}
	else
	{
		printk(KERN_ALERT "STAUD ALSA : <<< ENOSYS  snd_pcm_card_generic_probe()  Driver registration failed \n ");	
		return -ENOSYS;
	}

/*Udit Kumar LDDE 2.2*/
	switch(dev){
		case PCM0_DEVICE:
			 pcm0_platform_device = to_platform_device(&alsa_pcm0_device);
		
			break;
		case PCM1_DEVICE:
			 pcm1_platform_device = to_platform_device(&alsa_pcm1_device);
			break;
		case SPDIF_DEVICE:
			spdif_platform_device = to_platform_device(&alsa_spdif_device);
			break;
		case CAPTURE_DEVICE:
			 capture_platform_device = to_platform_device(&alsa_capture_device);
			break;
		default:
			return -EINVAL;
	}
/*end Udit Kumar*/
	chip->card_data = &card_list[dev];

	*card = snd_card_new(index[card_list[dev].major],id[card_list[dev].major], THIS_MODULE, 0);
        if (card == NULL)
	{
      		printk(KERN_ALERT "STAUD ALSA : <<< ENOMEM snd_pcm_card_generic_probe  cant allocate new card of %d\n",card_list[dev].major);
      		return -ENOMEM;
        }
      	if((err = main_device_allocate(*card,&card_list[dev],chip)) < 0)
	{
              	printk(KERN_ALERT "STAUD ALSA : <<< err=%d  snd_pcm_card_generic_probe  main_device_allocate() failed \n",err);
               	snd_card_free(*card);
       		return err;
       }
	DEBUG_PRINT_FN_CALL(("STAUD ALSA : <<< snd_pcm_card_generic_probe()  \n "));
	return 0;
}


static int __init snd_pcm_card_probe(int dev)
{
	snd_card_t *card={0};

	int err;
        pcm_hw_t *chip;

	DEBUG_PRINT_FN_CALL(("STAUD ALSA : >>> snd_pcm_card_probe()  dev=%d\n ",dev));
	if((chip = kcalloc(1,sizeof(pcm_hw_t), GFP_KERNEL)) == NULL)
	{
		printk(KERN_ALERT "STAUD ALSA : <<< ENOMEM snd_pcm_card_probe() No memory for chip \n ");	
              return -ENOMEM;
	}
	
	snd_pcm_card_generic_probe(&card,chip,dev);
	
        switch(card_list[dev].major){
		/*	case PROTOCOL_CONVERTER_DEVICE:
				return -ENODEV;*/

	     case PCM1_DEVICE:
		 	register_platform_driver(pcm1_platform_device, chip);
		 	break;

            case PCM0_DEVICE:
			register_platform_driver(pcm0_platform_device, chip);
			break;
			
	     case SPDIF_DEVICE:
	 		register_platform_driver(spdif_platform_device, chip);
		 	break;
			
	    case CAPTURE_DEVICE:
			register_platform_driver(capture_platform_device, chip);
			break;                        
              
             default:
             	   printk(KERN_ALERT "STAUD ALSA : <<< ENODEV  snd_pcm_card_probe() Invalid Device \n ");	
                 return -ENODEV;
        }
				
          if((err = stb7100_create_lpcm_device(chip,&card)) <0)
          {
          			printk(KERN_ALERT "STAUD ALSA : <<<   snd_pcm_card_probe() stb7100_create_lpcm_device failed \n ");	
                            snd_card_free(card);
          }
		  
		card_list[dev].device = card; 
		DEBUG_PRINT_FN_CALL(("STAUD ALSA : <<<  snd_pcm_card_probe() err =%d \n ",err));	
   		return err;

}
