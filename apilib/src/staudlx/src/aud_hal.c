/*
******************************************************************************
AUD_HAL.C
******************************************************************************
Hardware Abstraction Layer.
******************************************************************************
(C) Copyright STMicroelectronics - 2004-
******************************************************************************

Revision History (Latest modification on top)

*/

/*
******************************************************************************
Includes
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "sttbx.h"
#include "stlite.h"
#include "aud_hal.h"
#ifdef ST_OS21
#include "os21.h"
#else
#include "debug.h"
#endif
#include "aud_drv.h"
#include "stfdma.h"
#include "audioreg.h"
extern AudioDevice_t DeviceArray[];

#define LockInterrupt		      interrupt_lock
#define UnlockInterrupt		      interrupt_unlock

#define STFDMA_REQUEST_SIGNAL_SWTS	0

static U32 ArrayLayerII[] = {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384};
static U32 ArrayLayerI[] = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 488};

#define NEW_DECODER_ARCHITECTURE 1
/******************************************************************************
Private variables (static)
******************************************************************************
*/

/* Setup parameters */

HALAUD_PTIInterface_t PTIDMAReadWritePointer ;
HALAUD_PTIInterface_t PTIDMAReadWritePointerCompressed ;
HALAUD_SetupDef_t HALSetup;

/*
******************************************************************************
Private Functions (statics)
******************************************************************************
*/




/*
******************************************************************************
Global variables
******************************************************************************
*/
/*
HALAUD_ErrorTypeDef_t GlobalError;
HWVersion_t HWVersion;
*/



/* --------------------------------------------------------------------------
 Name   : HALAUD_InitData
 Purpose: Initializes internal hal structures
-----------------------------------------------------------------------------*/
void HALAUD_InitData(HALAUD_InitParams_t *HALInitParams_p)
{

	HALSetup.PESbufferAddress			= 0;
	HALSetup.PESbufferSize				= 0;
	HALSetup.BackbufferAddress			= 0;
	HALSetup.BackbufferSize				= 0;
	HALSetup.DecodedBufferAddress			= 0;
	HALSetup.DecodedBufferSize			= 0;
	HALSetup.GlobalPCMMute				=FALSE;
	HALSetup.GlobalSPDIFMute			=FALSE;
	HALSetup.InputMode				= DecoderMPEG2;
	HALSetup.SamplingFrequency			= 48000;
	HALSetup.DigitalOutputMode.Mode = DigitalOut_PCM;
	HALSetup.DigitalOutputMode.Latency		= 0;
	HALSetup.Volume.Left				= 0;
	HALSetup.Volume.Right				= 0;
	HALSetup.CPUPartition				= HALInitParams_p->CPUPartition_p;
	HALSetup.AVMEMPartition				= HALInitParams_p->AVMEMPartition;
	HALSetup.StreamType					= HALAUD_STREAM_TYPE_PES;

	HALSetup.DRCapability.DownsamplingCapable	= FALSE;
	HALSetup.DRCapability.DTSCapable		= FALSE;
	HALSetup.DRCapability.DynamicRangeCapable	= FALSE;
	HALSetup.DRCapability.SupportedPCMSamplingFreqs[0] = 48000;
	HALSetup.DRCapability.SupportedPCMSamplingFreqs[1] = 44100;
	HALSetup.DRCapability.SupportedPCMSamplingFreqs[2] = 32000;
	/* HALSetup.DRCapability.SupportedStopModes            = HALAUD_STOP_NOW; */
	HALSetup.DRCapability.SupportedStreamContents	= (HALAUD_STREAM_CONTENT_MPEG1 |
		HALAUD_STREAM_CONTENT_MPEG2);
	HALSetup.DRCapability.SupportedStreamTypes	= HALAUD_STREAM_TYPE_PES;
	HALSetup.DRCapability.SupportedTrickSpeeds[0]		= 0;
	HALSetup.DRCapability.MaxSyncOffset		= 1000;
	HALSetup.DRCapability.MinSyncOffset		= 0;
	HALSetup.DRCapability.RunTimeSwitch		= FALSE;

	HALSetup.IPCapability.PESParserCapable		= TRUE;
	HALSetup.IPCapability.I2SInputCapable		= FALSE;
	HALSetup.IPCapability.PCMBufferPlayback		= FALSE;

	HALSetup.MXCapability.NumberOfInputs		= 2;
	HALSetup.MXCapability.MixPerInputCapable	= FALSE;

	HALSetup.OPCapability.MaxAttenuation		= 63;
	HALSetup.OPCapability.AttenuationPrecision	= 1;
	HALSetup.OPCapability.ChannelAttenuationCapable	= TRUE;
	HALSetup.OPCapability.ChannelMuteCapable	= TRUE;
	HALSetup.OPCapability.DelayCapable		= FALSE;
	HALSetup.OPCapability.MaxDelay			= 0;
	HALSetup.OPCapability.MultiChannelOutputCapable	= FALSE;
	HALSetup.OPCapability.SupportedSpeakerTypes = HALAUD_SPEAKER_TYPE_SMALL;

	HALSetup.PPCapability.DeEmphasisCapable		= FALSE;
	HALSetup.PPCapability.PrologicDecodingCapable	= FALSE;
	HALSetup.PPCapability.SupportedEffects		= HALAUD_EFFECT_NONE;
	HALSetup.PPCapability.SupportedKaraokeModes	= HALAUD_KARAOKE_NONE;
	HALSetup.PPCapability.SupportedStereoModes	= HALAUD_STEREO_MODE_STEREO | HALAUD_STEREO_JOINT_STREO | HALAUD_STEREO_MODE_DUAL_CHANNEL | HALAUD_STEREO_MODE_SINGLE_CHANNEL;
	HALSetup.InitParams.InterruptNumber		= HALInitParams_p->InterruptNumber;
	HALSetup.InitParams.InterruptLevel		= HALInitParams_p->InterruptLevel;
	HALSetup.InitParams.AVMEMPartition		= HALInitParams_p->AVMEMPartition;
	HALSetup.InitParams.CD1BufferAddress_p		= HALInitParams_p->CD1BufferAddress_p;
	HALSetup.InitParams.CD1BufferSize		= HALInitParams_p->CD1BufferSize;
	HALSetup.InitParams.CD2BufferAddress_p		= HALInitParams_p->CD2BufferAddress_p;
	HALSetup.InitParams.CD2BufferSize		= HALInitParams_p->CD2BufferSize;
	HALSetup.InitParams.ClockGeneratorBaseAddress_p	= HALInitParams_p->ClockGeneratorBaseAddress_p;
	HALSetup.InitParams.InternalPLL			= HALInitParams_p->InternalPLL;
	HALSetup.InitParams.DACClockToFsRatio		= HALInitParams_p->DACClockToFsRatio;
	HALSetup.TimerTask = NULL;
	HALSetup.AC3TimerTask = NULL;
      STTBX_Print(("HALAUD_InitData done ......... " ));

}


void HALAUD_IPGetCapability(STAUD_IPCapability_t *IPCapability_p)
{


	IPCapability_p->PESParserCapable		= HALSetup.IPCapability.PESParserCapable;
	IPCapability_p->I2SInputCapable			= HALSetup.IPCapability.I2SInputCapable;
	IPCapability_p->PCMBufferPlayback		= HALSetup.IPCapability.PCMBufferPlayback;


 STTBX_Print(("HALAUD_IPGetCapability done ......... " ));

}


void HALAUD_PPGetCapability(HALAUD_PPCapability_t *PPCapability_p)
{


	PPCapability_p->DeEmphasisCapable		= HALSetup.PPCapability.DeEmphasisCapable;
	PPCapability_p->PrologicDecodingCapable		= HALSetup.PPCapability.PrologicDecodingCapable;
	PPCapability_p->SupportedEffects		= HALSetup.PPCapability.SupportedEffects;
	PPCapability_p->SupportedKaraokeModes		= HALSetup.PPCapability.SupportedKaraokeModes;
	PPCapability_p->SupportedStereoModes		= HALSetup.PPCapability.SupportedStereoModes;
 STTBX_Print(("HALAUD_PPGetCapability calling ......... " ));
}

void HALAUD_OPGetCapability(HALAUD_OPCapability_t *OPCapability_p)
{


	OPCapability_p->MaxAttenuation			= HALSetup.OPCapability.MaxAttenuation;
	OPCapability_p->ChannelAttenuationCapable	= HALSetup.OPCapability.ChannelAttenuationCapable;
	OPCapability_p->AttenuationPrecision		= HALSetup.OPCapability.AttenuationPrecision;
	OPCapability_p->ChannelMuteCapable		= HALSetup.OPCapability.ChannelMuteCapable;
	OPCapability_p->DelayCapable			= HALSetup.OPCapability.DelayCapable;
	OPCapability_p->MaxDelay			= HALSetup.OPCapability.MaxDelay;
	OPCapability_p->MultiChannelOutputCapable	= HALSetup.OPCapability.MultiChannelOutputCapable;
	OPCapability_p->SupportedSpeakerTypes		= HALSetup.OPCapability.SupportedSpeakerTypes;

 STTBX_Print(("HALAUD_OPGetCapability calling ......... " ));

}


void HALAUD_DRGetCapability(HALAUD_DRCapability_t *DRCapability_p)
{


		DRCapability_p->DownsamplingCapable = HALSetup.DRCapability.DownsamplingCapable;
		DRCapability_p->DTSCapable		  = HALSetup.DRCapability.DTSCapable;
		DRCapability_p->DynamicRangeCapable = HALSetup.DRCapability.DynamicRangeCapable;
		DRCapability_p->FadingCapable		  = HALSetup.DRCapability.FadingCapable;
		DRCapability_p->SupportedPCMSamplingFreqs[0] = HALSetup.DRCapability.SupportedPCMSamplingFreqs[0];
		DRCapability_p->SupportedPCMSamplingFreqs[1] = HALSetup.DRCapability.SupportedPCMSamplingFreqs[1];
		DRCapability_p->SupportedPCMSamplingFreqs[2] = HALSetup.DRCapability.SupportedPCMSamplingFreqs[2];
		/*DRCapability_p->SupportedStopModes            = HALSetup.DRCapability.SupportedStopModes;*/
		DRCapability_p->SupportedStreamContents = HALSetup.DRCapability.SupportedStreamContents;
		DRCapability_p->SupportedStreamTypes = HALSetup.DRCapability.SupportedStreamTypes;
		DRCapability_p->SupportedTrickSpeeds[0] = HALSetup.DRCapability.SupportedTrickSpeeds[0];
		DRCapability_p->MaxSyncOffset		  = HALSetup.DRCapability.MaxSyncOffset;
		DRCapability_p->MinSyncOffset = HALSetup.DRCapability.MinSyncOffset;
		DRCapability_p->RunTimeSwitch = HALSetup.DRCapability.RunTimeSwitch;

STTBX_Print(("HALAUD_DRGetCapability calling ......... " ));

}



 void HALAUD_OPGetParams (STAUD_OutputParams_t * Params_p)
 {


  Params_p->PCMOutParams.InvertWordClock= FALSE;
  Params_p->PCMOutParams.InvertWordClock=FALSE;
  Params_p->PCMOutParams.Format=STAUD_DAC_DATA_FORMAT_I2S;
  Params_p->PCMOutParams.Precision=STAUD_DAC_DATA_PRECISION_24BITS;
  Params_p->PCMOutParams.Alignment=STAUD_DAC_DATA_ALIGNMENT_RIGHT;
  Params_p->PCMOutParams.MSBFirst= FALSE;

  Params_p->SPDIFOutParams.AutoLatency= FALSE;
  Params_p->SPDIFOutParams.Latency= 0x03; /*need to correct it dummy value*/
  Params_p->SPDIFOutParams.CopyPermitted=STAUD_COPYRIGHT_MODE_NO_COPY;
  Params_p->SPDIFOutParams.CategoryCode=0xff; /*dummy value need to correct*/
  Params_p->SPDIFOutParams.AutoDTDI= FALSE;
 STTBX_Print(("HALAUD_OPGetParams calling ......... " ));
 }




/* --------------------------------------------------------------------------
 Name   : HALAUD_AudioBitBufferReset(void)
 Purpose: Reset the audio bit buffer content
 Note   :
-----------------------------------------------------------------------------*/



/* --------------------------------------------------------------------------
 Name   : HALAUD_SoftReset
 Purpose: This function softresets the decoder part and sets a flag that it
	  necessary to rewrite the registers (a lot of them are cleared)
	  before a new play is started.
 In     : -
 Out    : -
 Note   : -
-----------------------------------------------------------------------------
 */
void HALAUD_SoftReset(void)
{
/* softreset =1
[1] LAYER _UNKNOWN=0 0: Layer unknown, decoder detects the layer from the input stream
LAYER: 0 */
	U32 count;
	U32 delay;
	U32 NumberOfTicksPermSecond=0;
	NumberOfTicksPermSecond = (ST_GetClocksPerSecond()/1000);
	delay = NumberOfTicksPermSecond / 6;
	STSYS_WriteAudioReg32(AUD_DEC_CONFIG, 0x00000001);
	/*Check for reset*/
	task_delay(delay);
	while(!(STSYS_ReadAudioReg32(AUD_DEC_STATUS)& 0x00000002))
	{
		count++;
		task_delay(delay);
		if(count>100)
		{
			STTBX_Print(("\n Decoder Reset Failed \n"));
			break;
		}
	}


	/**(U32 *) (0x20700000)=0x00; *//* Amphion Decoder == Soft Reset Removed */
	STSYS_WriteAudioReg32(AUD_DEC_CONFIG, 0x00000002);


}



/*
******************************************************************************
Public Functions (not static)
******************************************************************************
*/


void HALAUD_PcmPlayerConfig(void)
{
	U32  readValue;
#if defined(ST_8010)

	/* init PCM_PLAYER0 for the External DAC */
	
	STSYS_WriteRegDev32LE(PCMPLAYER0_SOFT_RESET_REG,	1);//soft reset register
	STSYS_WriteRegDev32LE(PCMPLAYER0_SOFT_RESET_REG,	0);//soft reset register

	STSYS_WriteRegDev32LE(PCMPLAYER0_CONTROL_REG, 0x79e4020);
/*	STSYS_WriteRegDev32LE(PCMPLAYER0_FORMAT_REG, 0x00004588);*/
STSYS_WriteRegDev32LE(PCMPLAYER0_FORMAT_REG, 0x88);

	
    STTBX_Print(( " PCM_PLAYER 0 CONFIG_CTRL_D=[%x]\n", (readValue = STSYS_ReadRegDev32LE(PCMPLAYER0_CONTROL_REG))));
    STTBX_Print(( " PCM_PLAYER 0 PCMP_FORMAT=[%x]\n", (readValue = STSYS_ReadRegDev32LE(PCMPLAYER0_FORMAT_REG))));


	
#elif defined(ST_7100)
	STSYS_WriteRegDev32LE(PCMPLAYER0_CONTROL_REG, 0x200020);
	STSYS_WriteRegDev32LE(PCMPLAYER0_FORMAT_REG, 0x0000a588);
	/* init PCM_PLAYER1 for the Internal DAC */
	STSYS_WriteRegDev32LE(PCMPLAYER1_CONTROL_REG, 0x200020);
	STSYS_WriteRegDev32LE(PCMPLAYER1_FORMAT_REG, 0x0000a588);

    STTBX_Print(( " PCM_PLAYER 0 CONFIG_CTRL_D=[%x]\n", (readValue = STSYS_ReadRegDev32LE(PCMPLAYER0_CONTROL_REG))));
    STTBX_Print(( " PCM_PLAYER 0 PCMP_FORMAT=[%x]\n", (readValue = STSYS_ReadRegDev32LE(PCMPLAYER0_FORMAT_REG))));
    STTBX_Print(( " PCM_PLAYER 1 CONFIG_CTRL_D=[%x]\n", (readValue = STSYS_ReadRegDev32LE(PCMPLAYER1_CONTROL_REG))));
    STTBX_Print(( " PCM_PLAYER 1 PCMP_FORMAT=[%x]\n", (readValue = STSYS_ReadRegDev32LE(PCMPLAYER1_FORMAT_REG))));
#endif

}


/*----------------------------------------------------------------------------
 Name   : HALAUD_SetupDigitalOutputMode
 Purpose: This function will store the wanted setting of the S/PDIF.
 In     : HALAUD_DigitalOutputMode  : This parameter contains the wanted
				      digital output mode. The possibilities
				      are :   DigitalOut_PCM
					      DigitalOut_IEC1937
					      DigitalOut_Mute
					      DigitalOut_Off
 Out    : -
 Note   : -
------------------------------------------------------------------------------
 */


void HALAUD_SetupDigitalOutputMode(HALAUD_DigitalOutput_t HALAUD_SetDigitalOutputMode)
{
	U32 SpdifControl;
	HALSetup.DigitalOutputMode.Mode = HALAUD_SetDigitalOutputMode.Mode;
	HALSetup.DigitalOutputMode.Latency = HALAUD_SetDigitalOutputMode.Latency;
	switch (HALSetup.DigitalOutputMode.Mode)
	{
		case DigitalOut_PCM:
			SpdifControl = STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
			SpdifControl = (SpdifControl & 0xFFFFFFF8) | (0x0000000B);
			STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, SpdifControl);

			break;
		case DigitalOut_IEC1937:
			SpdifControl = STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
			SpdifControl = (SpdifControl & 0xFFFFFFF8) | (0x0000000C);
			STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, SpdifControl);
			break;
		case DigitalOut_Mute:
			SpdifControl = STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
			SpdifControl = (SpdifControl & 0xFFFFFFF8) | (0x00000002);
			STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, SpdifControl);
			break;
		case DigitalOut_Off:
			SpdifControl = STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
			SpdifControl = (SpdifControl & 0xFFFFFFF8);
			STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, SpdifControl);
			break;
		default:
			break;

	}


}
/*----------------------------------------------------------------------------
 Name   : HALAUD_GetDigitalOutput
 Purpose: This function will store the wanted setting of the S/PDIF.
 In     : HALAUD_DigitalOutputMode  : This parameter contains the wanted
				      digital output mode. The possibilities
				      are :   DigitalOut_PCM
					      DigitalOut_IEC1937
					      DigitalOut_Mute
					      DigitalOut_Off
 Out    : -
 Note   : -
------------------------------------------------------------------------------
 */
void HALAUD_GetDigitalOutput(HALAUD_DigitalOutput_t *DigitalOutput_p)

{
U32 SpdifControl;
SpdifControl= STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
	switch (SpdifControl)
		{
		case 0:
		     DigitalOutput_p->Mode=DigitalOut_Off;
	     	break;
		case 1:
		    DigitalOutput_p->Mode=DigitalOut_PCM;
		break;
		case 2:
		     DigitalOutput_p->Mode=DigitalOut_Mute;
		break;
		case 4:
		     DigitalOutput_p->Mode=DigitalOut_IEC1937;
		break;

			default:
				break;
			}



}


/*----------------------------------------------------------------------------
 Name   : HALAUD_Play
 Purpose: This function will start the hardware playing of the decoding
	  signal.
 In     : -
 Out    : -
 Note   : -
------------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------
 Name   : HALAUD_GetAttenuation
 Purpose: This function get the loudness of the output signal
 In     : Loudest : This parameter contains the volume in -dB
 Out    : -
 Note   : The left and right signal are attenuated together.
------------------------------------------------------------------------------
 */
void HALAUD_GetAttenuation(STAUD_Attenuation_t *Attenuation_p)
{
	U16 VolumeControl, left, right;
	VolumeControl= STSYS_ReadAudioReg32(AUD_DEC_VOLUME_CONTROL);
	STTBX_Print(( " GetVolumeControl=[%x]\n",VolumeControl ));
	left=VolumeControl;
	left= (left&0x0000003F);
	right=VolumeControl;
	right= (right&0x00003F00)>>8;
	Attenuation_p->Left=HALSetup.Attenuation.Left;
	Attenuation_p->Right=HALSetup.Attenuation.Right;
	Attenuation_p->LeftSurround=HALSetup.Attenuation.LeftSurround;;
	Attenuation_p->RightSurround=HALSetup.Attenuation.RightSurround;;
	Attenuation_p->Center=HALSetup.Attenuation.Center;
	Attenuation_p->Subwoofer=HALSetup.Attenuation.Subwoofer;

}




/*----------------------------------------------------------------------------
 Name   : HALAUD_SetAttenuation
 Purpose: This function sets the loudness of the output signal
 In     : Loudest : This parameter contains the volume in -dB
 Out    : -
 Note   : The left and right signal are attenuated together.
------------------------------------------------------------------------------
 */
void HALAUD_SetAttenuation(U16 Left, U16 Right)
{
	U16 VolumeControl;

	STTBX_Print(( " set left=[%x], right=[%x]\n",Left, Right ));
#ifndef ST_7100
	{
		U16 VolumeControl;
	if (Left > 63)
		Left = 63;
	if (Right > 63)
		Right = 63;
	Right=Right<<8;
	VolumeControl = (U32)(Left + Right);
	STTBX_Print(( " set_VolumeControl_to_reg=[%x]\n",VolumeControl ));

	STSYS_WriteAudioReg32(AUD_DEC_VOLUME_CONTROL, VolumeControl);
	}
#else
	{

		ST_ErrorCode_t Error;

		MME_Command_t  * cmd = (MME_Command_t *) malloc(sizeof(MME_Command_t));

		TARGET_AUDIODECODER_INITPARAMS_STRUCT   * init_params;
		TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT * gbl_params;

		// Get context of decoder
		init_params =  DeviceArray[0].MMEinitParams.TransformerInitParams_p,
		// Set the new params
		gbl_params  = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;
		gbl_params->PcmParams.BassMgt.Apply = ACC_MME_ENABLED;

		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LEFT ]  = Left;
		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RGHT ]  = Right;
		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LSUR ]  = HALSetup.Attenuation.LeftSurround;
		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RSUR ]  = HALSetup.Attenuation.RightSurround;
		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CNTR ]  = HALSetup.Attenuation.Center;
		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LFE ]	= HALSetup.Attenuation.Subwoofer;
		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CSURL]  = 0;
		gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CSURR]  = 0;
		gbl_params->PcmParams.BassMgt.Volume[ACC_AUX_LEFT  ]  = 0;
		gbl_params->PcmParams.BassMgt.Volume[ACC_AUX_RGHT  ]  = 0;

#define ST_7100_SIMPLE_UPDATEPARAMS

#ifndef ST_7100_SIMPLE_UPDATEPARAMS // send the whole config structure.
		Error = acc_setAudioDecoderGlobalCmd( cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
		                              ACC_LAST_DECPCMPROCESS_ID);
#else   // more efficient method: only send the one postprocessing to update.
		Error = acc_setAudioDecoderGlobalCmd( cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
		                              ACC_DECPCMPRO_BASSMGT_ID);
#endif // ST_7100_SIMPLE_UPDATEPARAMS


	}

#endif // ST_7100

}


/*----------------------------------------------------------------------------
 Name   : HALAUD_Stop
 Purpose: This function will stop the hardware playing of the decoding
	  signal.
 In     : -
 Out    : -
 Note   : SoftReset added in case of CDDA for GenAudio.
------------------------------------------------------------------------------
 */







/*----------------------------------------------------------------------------
 Name   : HALAUD_SetDigitalMute
 Purpose: This function will mute/unmute the digital output
 Note   :
-----------------------------------------------------------------------------*/
void HALAUD_SetDigitalMute(BOOL On)
{
	U32 SpdifControl;


	if (On)
	{
		SpdifControl = STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
		SpdifControl = ((SpdifControl & 0xFFFFFFF8) | (0x00000002));
		STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, SpdifControl);
	}
	else
	{
		switch (HALSetup.DigitalOutputMode.Mode)
		{
			case DigitalOut_IEC1937:
				SpdifControl = STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
				SpdifControl = (SpdifControl & 0xFFFFFFF8) | (0x0000000C);
				STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, SpdifControl);
				break;
			case DigitalOut_PCM:
				SpdifControl = STSYS_ReadSpdifPlayReg32(SPDIF_CTRL);
				SpdifControl = (SpdifControl & 0xFFFFFFF8) | (0x0000000B);
				STSYS_WriteSpdifPlayerReg32(SPDIF_CTRL, SpdifControl);
				break;

			default:
				break;
		}
	}

}


/*----------------------------------------------------------------------------
 Name   : HALAUD_SetPcmlMute
 Purpose: This function will mute/unmute the analog output
 Note   :
-----------------------------------------------------------------------------*/
void HALAUD_SetPcmlMute(BOOL On)
{
	U32 PcmOutputControl;

	if (On) /*mute "mute bit 0"*/
	{
		/*printf("Muted............\n");*/
#ifndef INTERNAL_DACS
		PcmOutputControl = *((volatile unsigned int *)(PCMPLAYER0_CONTROL_REG));
		PcmOutputControl    &= (0xFFFFFFFC);
		PcmOutputControl    |= (0x1);                 							    //MUTE the PCM
		*((volatile unsigned int *)(PCMPLAYER0_CONTROL_REG)) = PcmOutputControl;
#else
		PcmOutputControl = *((volatile unsigned int *)(PCMPLAYER1_CONTROL_REG));
		PcmOutputControl    &= (0xFFFFFFFC);
		PcmOutputControl    |= (0x1);                 							    //MUTE the PCM
		*((volatile unsigned int *)(PCMPLAYER1_CONTROL_REG)) = PcmOutputControl;
#endif
	}
	else  /*unmute  " mute bit 1"*/
	{
		/*printf("UnMuted............\n");*/
#ifndef INTERNAL_DACS
		PcmOutputControl = *((volatile unsigned int *)(PCMPLAYER0_CONTROL_REG));
		PcmOutputControl    &= (0xFFFFFFFC);
		PcmOutputControl    |= (0x2);                 							  //UNMUTE the PCM
		*((volatile unsigned int *)(PCMPLAYER0_CONTROL_REG)) = PcmOutputControl;
#else
		PcmOutputControl = *((volatile unsigned int *)(PCMPLAYER1_CONTROL_REG));
		PcmOutputControl    &= (0xFFFFFFFC);
		PcmOutputControl    |= (0x2);                 							  //UNMUTE the PCM
		*((volatile unsigned int *)(PCMPLAYER1_CONTROL_REG)) = PcmOutputControl;
#endif
	}
}


BOOL HALAUD_GetDeEmphasisFilter(void)
{
	BOOL Emphasis = FALSE;
	U32 ReadAudStatus;
	ReadAudStatus = STSYS_ReadAudioReg32(AUD_DEC_STATUS);
	ReadAudStatus = (ReadAudStatus & AUD_COPY_RIGHT);
	if (ReadAudStatus == HALAUD_EMPHASIS_50_MICROSECOND)
	{
		Emphasis = TRUE;
	}
	else if (ReadAudStatus == HALAUD_EMPHASIS_CCITT_J_17)
	{
		Emphasis = TRUE;
	}
	else
	{
		Emphasis = FALSE;
	}
	return Emphasis;
}


/*----------------------------------------------------------------------------
 Name   : HALAUD_GetStereoOutput
 Purpose:
 Note   : -
------------------------------------------------------------------------------
 */

HALAUD_StereoMode_t  HALAUD_GetStereoOutput (void)
{

U32 ReadAudStatus;
HALAUD_StereoMode_t StereoMode;
ReadAudStatus = STSYS_ReadAudioReg32(AUD_DEC_STATUS);
ReadAudStatus = (ReadAudStatus & AUD_AUDIO_MODE);
	if (ReadAudStatus == HALAUD_AUDIOMODE_STEREO)
	{
		StereoMode = STAUD_STEREO_MODE_STEREO;
	}
	else if (ReadAudStatus == HALAUD_AUDIOMODE_JOINT_STEREO)
	{
		StereoMode = HALAUD_STEREO_JOINT_STREO;
	}
	else if (ReadAudStatus == HALAUD_AUDIOMODE_DUAL_CHANNEL)
	{
		StereoMode = HALAUD_STEREO_MODE_DUAL_CHANNEL;
	}
	else if (ReadAudStatus == HALAUD_AUDIOMODE_SINGLE_CHANNEL)
	{
		StereoMode= HALAUD_STEREO_MODE_SINGLE_CHANNEL;
	}
	return(StereoMode);
}



/*----------------------------------------------------------------------------
 Name   : HALAUD_SetupSamplingFrequency
 Purpose:
 Note   : -
------------------------------------------------------------------------------
 */
void HALAUD_SetupSamplingFrequency(U32 samplingfrequency)
{
	HALSetup.SamplingFrequency = samplingfrequency;
}



/*----------------------------------------------------------------------------
 Name   : HALAUD_SetFrequencySynthetiser
 Purpose:
 Note   : -
------------------------------------------------------------------------------
 */
void HALAUD_SetFrequencySynthetiser(U32 Frequency)
{

	U32 i;
/* set PCM player frequency*/
#if defined(ST_5105)
	switch (Frequency)
	{

		case 32000:
			i = 4;
			break;
		case 48000:
      /* setup 0 0xf32 and setup1=0x7146 for freq LR 48 Khz */
      STSYS_WriteAudioFreqSynthesizerReg32(CKG_PCM_PLAYER_CLK_SETUP0,0xF32);
      STSYS_WriteAudioFreqSynthesizerReg32(CKG_PCM_PLAYER_CLK_SETUP1,0x7146);
      STSYS_WriteAudioFreqSynthesizerReg32(CKG_SPDIF_PLAYER__CLK_SETUP0,0x0F71);
      STSYS_WriteAudioFreqSynthesizerReg32(CKG_SPDIF_PLAYER_CLK_SETUP1,0x377f);

			break;
		case 44100:
      STSYS_WriteAudioFreqSynthesizerReg32(CKG_PCM_PLAYER_CLK_SETUP0,0x0F71);
      STSYS_WriteAudioFreqSynthesizerReg32(CKG_PCM_PLAYER_CLK_SETUP1,0x377f);

		default:
			break;
	}

	HALAUD_SetupSamplingFrequency(Frequency);
#endif

}



/*----------------------------------------------------------------------------
 Name   : HALAUD_GetFrameTime
 Purpose: Give the time in ms to decode one frame
-----------------------------------------------------------------------------*/
U32 HALAUD_GetFrameTime(void)
{
    U32 FrameTime = 0;
    U32 ReadAudStatus;
    float Value=0;

       Value = (float ) 1000  / (float)HALAUD_ReadSamplingFrequency();
   	LockInterrupt();
	ReadAudStatus = STSYS_ReadAudioReg32(AUD_DEC_STATUS);
	ReadAudStatus = (ReadAudStatus & AUD_LAYER_ID);
	if (ReadAudStatus == HALAUD_MPEG_LAYER_I)
	{
	 FrameTime = (U32)(384 * Value); /*layer I*/
	}
	else if (ReadAudStatus == HALAUD_MPEG_LAYER_II)
	{
	FrameTime = (U32)(1152 * Value); /* Layer II */
	}
	else
	 {
	 FrameTime=0;
	 }

         UnlockInterrupt();
    return FrameTime;
}

/*
LockInterrupt();
  UnlockInterrupt();  */

/*----------------------------------------------------------------------------
 Name   : HALAUD_GetBlockTime
 Purpose: Give the time in ms to decode one block
-----------------------------------------------------------------------------*/
U32 HALAUD_GetBlockTime(void)
{
    U32 BlockTime = 0;
    float Value;

    Value = (float)1000 / (float)HALAUD_ReadSamplingFrequency();


            BlockTime = (U32)(96 * Value); /*mpeg 1 and mpeg ii*/

    return BlockTime;
}




/*----------------------------------------------------------------------------
 Name   : HALAUD_GetStreamInfo
 Purpose: This function reads the Presentation Time Stamp from the dedicated
	  register.

------------------------------------------------------------------------------
 */

void HALAUD_GetStreamInfo(HALAUD_StreamInfo_t *StreamInfo_p)
{
	BOOL LayerII = FALSE;
	BOOL LayerI = FALSE;

	U32 ReadAudStatus, temp;
	LockInterrupt();
  	ReadAudStatus = STSYS_ReadAudioReg32(AUD_DEC_STATUS);
   STTBX_Print(( " HALAUD_GetStreamInfo=[%x]\n",ReadAudStatus ));
	temp = ReadAudStatus;
	ReadAudStatus = (ReadAudStatus & 0x00000060 );
	if (ReadAudStatus == 0x60)
	{
		StreamInfo_p->Emphasis = TRUE;
	}
	else if (ReadAudStatus == 20)
	{
		StreamInfo_p->Emphasis = TRUE;
	}
	else
	{
		StreamInfo_p->Emphasis = FALSE;
	}
	ReadAudStatus = temp;
	ReadAudStatus = (ReadAudStatus & 0x00010000);
	if (ReadAudStatus == 0x00010000)
	{
		StreamInfo_p->CopyRight = TRUE;
	}
	else
	{
		StreamInfo_p->CopyRight = FALSE;
	}
	ReadAudStatus = temp;
	ReadAudStatus = (ReadAudStatus & 0x00030000);
	if (ReadAudStatus == 0x00)
	{
		StreamInfo_p->AudioMode = STAUD_STEREO_MODE_STEREO;
	}
	else if (ReadAudStatus == 0x00010000)
	{
		StreamInfo_p->AudioMode = HALAUD_STEREO_JOINT_STREO;
	}
	else if (ReadAudStatus == 0x00020000)
	{
		StreamInfo_p->AudioMode = HALAUD_STEREO_MODE_DUAL_CHANNEL;
	}
	else if (ReadAudStatus == 0x00030000)
	{
		StreamInfo_p->AudioMode = HALAUD_STEREO_MODE_SINGLE_CHANNEL;
	}

	ReadAudStatus = temp;
	ReadAudStatus = (ReadAudStatus & 0x00000300);
	if (ReadAudStatus == 0x00000300)
	{
		StreamInfo_p->LayerNumber = STAUD_MPEG_LAYER_I;
		LayerI = TRUE;
	}
	else if (ReadAudStatus == 0x00000200)
	{
		StreamInfo_p->LayerNumber = STAUD_MPEG_LAYER_II;
		LayerII = TRUE;
	}

	ReadAudStatus = temp;
	ReadAudStatus = (ReadAudStatus & 0x00003C00);
	ReadAudStatus = ReadAudStatus >> 10;
	if (LayerII)
	{
		StreamInfo_p->BitRate =	ArrayLayerII[ReadAudStatus];
	}
	else if (LayerI)
	{
		StreamInfo_p->BitRate =	ArrayLayerI[ReadAudStatus];
	}

	else
	{
		StreamInfo_p->BitRate =	0;
	}

	ReadAudStatus = temp;
	ReadAudStatus = (ReadAudStatus & 0x0000C000);

	if (ReadAudStatus == 0x00)
	{
		StreamInfo_p->SamplingFrequency	=	441000;
	}
	else if (ReadAudStatus == 0x00008000)
	{
		StreamInfo_p->SamplingFrequency	=	32000;
	}

	else if (ReadAudStatus == 0x00004000)
	{
		StreamInfo_p->SamplingFrequency	=	48000;
	}
	else
	{
		StreamInfo_p->SamplingFrequency	=	00000;
	}

  UnlockInterrupt();
}






/*----------------------------------------------------------------------------
 Name   : HALAUD_ReadSamplingFrequency
 Purpose: This function reads the sampling frequency register. Every bitset
	  mappes a frequency. Look at the switch-case for which bitset mappes
	  to which frequency
 In     :
 Out    : 32 bits value of the frequency, given in Hertz.
 Note   :
------------------------------------------------------------------------------
 */
U32 HALAUD_ReadSamplingFrequency(void)
{

	U32 ReadValue, ReturnValue;
	LockInterrupt();

	ReadValue = STSYS_ReadAudioReg32(AUD_DEC_STATUS);

	ReadValue = ReadValue & AUD_SAMPFREQ__INDEX;


	if (ReadValue == 0x00000000)
	{
		ReturnValue = 44100;
	}
	else if (ReadValue == 0x00004000)
	{
		ReturnValue = 48000;
	}
	else if ((ReadValue == 0x00008000))
	{
		ReturnValue = 32000;
	}
	else
	{
		ReturnValue = 0000;
	}

	UnlockInterrupt();
	return ReturnValue;
}





void HALAUD_GetInputBufferParams(HALAUD_Buffer_t BufferType,STAUD_BufferParams_t* DataParams_p)
{

	if(BufferType == MPEG_PES_BUFFER)
	{
  		DataParams_p->BufferBaseAddr_p= (void*)HALSetup.PESbufferAddress;
		DataParams_p->BufferSize=       HALSetup.PESbufferSize;
	}

	if(BufferType == AC3_PES_BUFFER)
	{
  		DataParams_p->BufferBaseAddr_p= (void*)HALSetup.PESAC3bufferAddress;
		DataParams_p->BufferSize=       HALSetup.PESAC3bufferSize;
	}

}



void HALAUD_SetDataInputInterface(HALAUD_Buffer_t BufferType, \
											 GetWriteAddress_t  GetWriteAddress_p,InformReadAddress_t InformReadAddress_p, void * const Handle_p)
{

	if(BufferType==MPEG_PES_BUFFER)
	{
		PTIDMAReadWritePointer.WriteAddress_p= GetWriteAddress_p;
		PTIDMAReadWritePointer.ReadAddress_p= InformReadAddress_p;
		PTIDMAReadWritePointer.Handle=Handle_p;
	}
	if(BufferType==AC3_PES_BUFFER)
	{

		PTIDMAReadWritePointerCompressed.WriteAddress_p= GetWriteAddress_p;
		PTIDMAReadWritePointerCompressed.ReadAddress_p= InformReadAddress_p;
		PTIDMAReadWritePointerCompressed.Handle=Handle_p;

	}


}




/* ----------------------------- End of file ----------------------------- */

