/********************************************************************************
 *   Filename   	: Aud_MMEAudioDecoderPP.c											*
 *   Start       		: 15.09.2005                                                   							*
 *   By          		: Chandandeep Singh Pabla											*
 *   Contact     	: chandandeep.pabla@st.com										*
 *   Description  	: The file contains the Decoder level PCM Post Processing APIs that will be	*
 *				  Exported to other Modules. Functions in this file will also send command 	*
 * 				  to MME for applying the requested post processings					 	*
 ********************************************************************************
 */


/* {{{ Includes */
//#define  STTBX_PRINT
#ifndef ST_OSLINUX
#include <string.h>
#include "sttbx.h"
#endif
#include "stos.h"
#include "aud_mmeaudiostreamdecoder.h"
#include "aud_mmeaudiodecoderpp.h"
#include "aud_audiodescription.h"

/* }}} */

/* {{{ Testing Params */
/* }}} */

/* {{{ Global Variables */
static int GlobalParamsListCount=0;
ST_ErrorCode_t	STAud_DecPPCleanCmdFrmList(DecoderControlBlock_t	*DecCtrlBlock,MME_Command_t * cmd);

/* }}} */

/******************************************************************************
 *  Function name 	: STAud_DecPPSetSpeed
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the tempo control settings to a particular decoder
 ***************************************************************************** */

ST_ErrorCode_t 	STAud_DecPPSetSpeed(STAud_DecHandle_t	Handle, S32 Speed)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT    	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT	 *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	if(Dec_p->DecoderControlBlock.DecCapability.TempoCapable)
	{
		gbl_params->PcmParams.TempoCtrl.Id = ACC_TEMPO_ID;
		gbl_params->PcmParams.TempoCtrl.StructSize = sizeof(MME_TempoGlobalParams_t);
		gbl_params->PcmParams.TempoCtrl.Ratio= Speed-100;

		if(gbl_params->PcmParams.TempoCtrl.Ratio)
			gbl_params->PcmParams.TempoCtrl.Apply = TRUE;
		else
			gbl_params->PcmParams.TempoCtrl.Apply = FALSE;

		/* Keep a copy of updated params with control block */

		Dec_p->DecoderControlBlock.DecPcmProcessing->TempoCtrl.Apply =  gbl_params->PcmParams.TempoCtrl.Apply;

		if(Dec_p->DecoderControlBlock.Transformer_Init)
		{
			error = STAud_DecPPSendCmd(Dec_p,ACC_LAST_DECPCMPROCESS_ID);
		}
	}
	else
	{
		error = ST_ERROR_FEATURE_NOT_SUPPORTED;
	}

	return error;
}


/******************************************************************************
 *  Function name 	: STAud_DecPPSetDownMix
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming down mix settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetDownMix(STAud_DecHandle_t		Handle,
                           			  STAUD_DownmixParams_t	*downmix)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
	MME_LxMlpConfig_t                       *mlp_config=NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Stream Content will be zero at init time or after stop.*/
	if(Dec_p->DecoderControlBlock.StreamParams.StreamContent == 0)
	{
		/* case:: Setting Downmix params before start. Hold them, we will apply these setting at Transformer Init time */
		Dec_p->DecoderControlBlock.HoldDownMixParams.Downmix = *downmix;
		Dec_p->DecoderControlBlock.HoldDownMixParams.Apply = TRUE;
		return error;
	}

	/* Set the new params here */
	gbl_params->PcmParams.DMix.Id = ACC_DMIX_ID;
	gbl_params->PcmParams.DMix.StructSize = sizeof(MME_DMixGlobalParams_t);

	switch(Dec_p->DecoderControlBlock.StreamParams.StreamContent)
	{
		case STAUD_STREAM_CONTENT_MLP:

			gbl_params->PcmParams.DMix.Apply =  ACC_MME_DISABLED;
			gbl_params->PcmParams.DMix.Config[DMIX_MONO_UPMIX]= (U8) ACC_MME_TRUE;

			mlp_config = (MME_LxMlpConfig_t *) gbl_params->DecConfig;
			mlp_config->Config[MLP_DRC_ENABLE] = ACC_MME_FALSE;	// pour LO/RO
			if ( downmix->Apply )
			{
				MLPMME_SET_CHREDIR(mlp_config->Config, 0);  /* Channel redirection enabled for Stereo decoding */
				mlp_config->Config[MLP_DOWNMIX_2_0] = ACC_MME_TRUE;	/* decode only substream 0 for stereo only decoder */
			}
			else
			{
				MLPMME_SET_CHREDIR(mlp_config->Config, 1);/* Channel redirection disabled for MultiChannel decoding */
				mlp_config->Config[MLP_DOWNMIX_2_0] = ACC_MME_FALSE; /* decode all present channels*/
			}
		break;

		default:
			gbl_params->PcmParams.DMix.Apply =  downmix->Apply;
			gbl_params->PcmParams.DMix.Config[DMIX_USER_DEFINED]= (U8) 0x0;
			gbl_params->PcmParams.DMix.Config[DMIX_STEREO_UPMIX]= (U8) downmix->stereoUpmix;
			gbl_params->PcmParams.DMix.Config[DMIX_MONO_UPMIX]= (U8) downmix->monoUpmix;
			gbl_params->PcmParams.DMix.Config[DMIX_MEAN_SURROUND]= (U8) downmix->meanSurround;
			gbl_params->PcmParams.DMix.Config[DMIX_SECOND_STEREO]= (U8) downmix->secondStereo;
			gbl_params->PcmParams.DMix.Config[DMIX_NORMALIZE]= (U8) downmix->normalize;
			gbl_params->PcmParams.DMix.Config[DMIX_NORM_IDX]= (U8) downmix->normalizeTableIndex;
			gbl_params->PcmParams.DMix.Config[DMIX_DIALOG_ENHANCE]= (U8) downmix->dialogEnhance;

			if (downmix->Apply)
			{
				gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]= (U8) ACC_MODE20;
				gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_AUX]= (U8) ACC_MODE_ID;
			}
			else
			{
				gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]= (U8) ACC_MODE_ID;
				gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_AUX]= (U8) ACC_MODE_ID;
			}
			break;
	}

	/* Keep a copy of updated params with control block */
	/* Dec_p->DecoderControlBlock.DecPcmProcessing.CMC =  gbl_params->PcmParams.CMC; */

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_LAST_DECPCMPROCESS_ID);
	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetDownMix
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current DMix settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetDownMix(STAud_DecHandle_t		Handle,
                           			  STAUD_DownmixParams_t	*downmix)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */

	downmix->Apply = gbl_params->PcmParams.DMix.Apply;
	downmix->stereoUpmix = gbl_params->PcmParams.DMix.Config[DMIX_STEREO_UPMIX];
	downmix->monoUpmix = gbl_params->PcmParams.DMix.Config[DMIX_MONO_UPMIX];
	downmix->meanSurround = gbl_params->PcmParams.DMix.Config[DMIX_MEAN_SURROUND];
	downmix->secondStereo = gbl_params->PcmParams.DMix.Config[DMIX_SECOND_STEREO];
	downmix->normalize = gbl_params->PcmParams.DMix.Config[DMIX_NORMALIZE];
	downmix->normalizeTableIndex = gbl_params->PcmParams.DMix.Config[DMIX_NORM_IDX];
	downmix->dialogEnhance = gbl_params->PcmParams.DMix.Config[DMIX_DIALOG_ENHANCE];

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPSetDynamicRange
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the Dynamic Range settings to a
 *  particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetDynamicRange(STAud_DecHandle_t		Handle,
                           			  STAUD_DynamicRange_t	*DyRange)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
	/* Decoder Config Params */
	MME_LxAc3Config_t	*ac3_config;
	MME_LxMp2aConfig_t	*aac_config;
	MME_LxMp2aConfig_t	*mp2a_config;
	MME_LxDtsConfig_t	*dts_config;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Stream Content will be zero at init time or after stop.*/
	if(Dec_p->DecoderControlBlock.StreamParams.StreamContent == 0)
	{
		/* case:: Setting PCM params before start */
		Dec_p->DecoderControlBlock.HoldDynamicRange.DyRange = *DyRange;
		Dec_p->DecoderControlBlock.HoldDynamicRange.Apply = TRUE;
		return error;
	}

	/* switch to the Decoder type */
	switch(Dec_p->DecoderControlBlock.StreamParams.StreamContent)
	{
		case STAUD_STREAM_CONTENT_MPEG2:
			mp2a_config = (MME_LxMp2aConfig_t *)gbl_params->DecConfig;
			mp2a_config->Config[MP2a_DRC_ENABLE]       = (U8) DyRange->Enable;
			break;

		case STAUD_STREAM_CONTENT_MPEG_AAC:
			aac_config = (MME_LxMp2aConfig_t *)gbl_params->DecConfig;
			aac_config->Config[AAC_DRC_ENABLE] = (U8) DyRange->Enable;
			break;

		case STAUD_STREAM_CONTENT_HE_AAC:
			aac_config = (MME_LxMp2aConfig_t *)gbl_params->DecConfig;
        	aac_config->Config[AAC_DRC_ENABLE]  = (U8) DyRange->Enable;
            if(DyRange->Enable)
            {
    		   	aac_config->Config[AAC_DRC_CUT]     = (U8) DyRange->CutFactor;
	    	   	aac_config->Config[AAC_DRC_BOOST]   = (U8) DyRange->BoostFactor;
		    }
			break;
		case STAUD_STREAM_CONTENT_AC3:
		case STAUD_STREAM_CONTENT_DDPLUS:
			ac3_config = (MME_LxAc3Config_t *)gbl_params->DecConfig;
			if(DyRange->Enable)
			{
				ac3_config->Config[DD_HDR]           = (U8) DyRange->CutFactor;
				ac3_config->Config[DD_LDR]           = (U8)DyRange->BoostFactor;
			}
			else
			{
				ac3_config->Config[DD_HDR]           = (U8) 0x00;
				ac3_config->Config[DD_LDR]           = (U8) 0x00;
			}
			break;

		case STAUD_STREAM_CONTENT_DTS :
			dts_config = (MME_LxDtsConfig_t *)gbl_params->DecConfig;
			dts_config->Config[DTS_DRC_ENABLE] = (U8) DyRange->Enable;
			break ;

		default:
			/* Dynamic range not supported for "this" type of decoder */
			return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}
	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_LAST_DECPCMPROCESS_ID);
	}
	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetDynamicRange
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Dynamic Range settings
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetDynamicRange(STAud_DecHandle_t		Handle,
                           			  STAUD_DynamicRange_t	*DyRange)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
	/* Decoder Config Params */
	MME_LxAc3Config_t	*ac3_config;
	MME_LxMp2aConfig_t	*aac_config;
	MME_LxMp2aConfig_t	*mp2a_config;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* switch to the Decoder type */
	switch(Dec_p->DecoderControlBlock.StreamParams.StreamContent)
	{
		case STAUD_STREAM_CONTENT_MPEG2:
			mp2a_config = (MME_LxMp2aConfig_t *)gbl_params->DecConfig;
			DyRange->Enable = mp2a_config->Config[MP2a_DRC_ENABLE];
			break;

		case STAUD_STREAM_CONTENT_MPEG_AAC:
			aac_config = (MME_LxMp2aConfig_t *)gbl_params->DecConfig;
			DyRange->Enable = aac_config->Config[AAC_DRC_ENABLE];
			break;
		case STAUD_STREAM_CONTENT_AC3:
		case STAUD_STREAM_CONTENT_DDPLUS:
			ac3_config = (MME_LxAc3Config_t *)gbl_params->DecConfig;

			DyRange->CutFactor= ac3_config->Config[DD_HDR];
			DyRange->BoostFactor = ac3_config->Config[DD_LDR];

			break;
		default:
			/* Dynamic range not supported for "this" type of decoder */
			return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}
	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPSetVol
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming down mix settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetVol(STAud_DecHandle_t		Handle,
                           			 				 STAUD_Attenuation_t  *Attenuation_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

		if(Dec_p->DecoderControlBlock.UpdateGlobalParamList)
		{
			int count=0;
			UpdateGlobalParamList_t *temp;

			temp=Dec_p->DecoderControlBlock.UpdateGlobalParamList;
			STTBX_Print(("STAud_DecPPSetVol: *** UpdateGlobalParamList NOT NULL **** \n"));
			while(temp)
			{
				count++;
				temp=temp->Next_p;
			}
			STTBX_Print(( "No of commands pending in globalParamList= %d\n",count));
		}
		else
		{
			STTBX_Print(( "STAud_DecPPSetVol: UpdateGlobalParamList=NULL \n"));
		}
	Dec_p->DecoderControlBlock.Attenuation_p.Left           = Attenuation_p->Left;
	Dec_p->DecoderControlBlock.Attenuation_p.Right          = Attenuation_p->Right;
	Dec_p->DecoderControlBlock.Attenuation_p.Center         = Attenuation_p->Center;
	Dec_p->DecoderControlBlock.Attenuation_p.Subwoofer      = Attenuation_p->Subwoofer;
	Dec_p->DecoderControlBlock.Attenuation_p.LeftSurround   = Attenuation_p->LeftSurround;
	Dec_p->DecoderControlBlock.Attenuation_p.RightSurround  = Attenuation_p->RightSurround;
	error = STAud_DecPPSetPan(Handle);
	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetVol
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Volume settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetVol(STAud_DecHandle_t		Handle,
                           			  STAUD_Attenuation_t  *Attenuation_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Get the params here */
	Attenuation_p->Left = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LEFT ] ;
	Attenuation_p->Right = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RGHT ] ;
	Attenuation_p->Center = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CNTR ] ;
	Attenuation_p->Subwoofer = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LFE ] ;
	Attenuation_p->LeftSurround = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LSUR];
	Attenuation_p->RightSurround = gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RSUR] ;
		if(Dec_p->DecoderControlBlock.UpdateGlobalParamList)
		{
			int count=0;
			UpdateGlobalParamList_t *temp;

			temp=Dec_p->DecoderControlBlock.UpdateGlobalParamList;
			STTBX_Print(("STAud_DecPPGetVol: *** UpdateGlobalParamList NOT NULL **** \n"));
			while(temp)
			{
				count++;
				temp=temp->Next_p;
			}
			STTBX_Print(("No of commands pending in globalParamList= %d\n",count));
		}
		else
		{
			STTBX_Print(("STAud_DecPPGetVol: UpdateGlobalParamList=NULL \n"));
		}

	return error;
}


/******************************************************************************
 *  Function name 	: STAud_DecPPSetEffect
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming Effect settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetEffect(STAud_DecHandle_t		Handle,
                           			  STAUD_Effect_t	effect)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.TruSurXT.Id = ACC_TSXT_ID;
	gbl_params->PcmParams.TruSurXT.StructSize = sizeof(MME_TsxtGlobalParams_t);

	switch(effect)
	{
		case STAUD_EFFECT_NONE :
			gbl_params->PcmParams.TruSurXT.Apply =  ACC_MME_DISABLED;
			break;

		case STAUD_EFFECT_TRUSURROUND :
			gbl_params->PcmParams.TruSurXT.Config[TSXT_ENABLE] = (U8)1<<3|(U8)1<<0;
			break;

		case STAUD_EFFECT_SRS_FOCUS :
			gbl_params->PcmParams.TruSurXT.Config[TSXT_ENABLE] =(((U8)1<<3)|((U8)1<<1) |((U8)1<<0));
			break;

		case STAUD_EFFECT_SRS_TRUBASS :
			gbl_params->PcmParams.TruSurXT.Config[TSXT_ENABLE] = (((U8)1<<3)|((U8)1<<2)|((U8)1<<0));
			break;

		case STAUD_EFFECT_SRS_TRUSUR_XT :
			gbl_params->PcmParams.TruSurXT.Config[TSXT_ENABLE] = (((U8)1<<3)|((U8)1<<2)|((U8)1<<1)
																|((U8)1<<0));
			break;

		case STAUD_EFFECT_SRS_BYPASS:
			gbl_params->PcmParams.TruSurXT.Config[TSXT_ENABLE] = (((U8)1<<3));
			break;


		default :
			break;
	}

	if(effect != STAUD_EFFECT_NONE)
	{
		gbl_params->PcmParams.TruSurXT.Apply =  ACC_MME_ENABLED;
	}

	/* Keep a copy of updated params with control block */
//	Dec_p->DecoderControlBlock.DecPcmProcessing.TruSurXT =  gbl_params->PcmParams.TruSurXT;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_TSXT_ID);
	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetEffect
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Effect settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetEffect(STAud_DecHandle_t		Handle,
                           			  STAUD_Effect_t	*effect)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	U8	CurEffect;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	CurEffect = gbl_params->PcmParams.TruSurXT.Config[TSXT_ENABLE];

	/* Get the Effect value  */
	if(gbl_params->PcmParams.TruSurXT.Apply==ACC_MME_DISABLED)
	{
		*effect = STAUD_EFFECT_NONE;
	}
	else
	{
		switch(CurEffect & 0xFF)
		{
			case 9:
				*effect = STAUD_EFFECT_TRUSURROUND;
				break;

			case 11:
				*effect = STAUD_EFFECT_SRS_FOCUS;
				break;

			case 13:
				*effect = STAUD_EFFECT_SRS_TRUBASS;
				break;

			case 15:
				*effect = STAUD_EFFECT_SRS_TRUSUR_XT;
				break;

			case 8:
				*effect = STAUD_EFFECT_SRS_BYPASS;
				break;

			default:
				break;
		}
	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPSetOmniParams
 *  Arguments    	:
 *       IN			:
 *       OUT    	: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming Omni settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetOmniParams(STAud_DecHandle_t		Handle,
                           				 STAUD_Omni_t   Omni)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.OmniMain.Id = ACC_OMNI_ID;
	gbl_params->PcmParams.OmniMain.StructSize = sizeof(MME_OmniGlobalParams_t);
	gbl_params->PcmParams.OmniMain.Apply = ACC_MME_ENABLED;

	gbl_params->PcmParams.OmniMain.Config[OMNI_ENABLE]			= Omni.omniEnable;
	gbl_params->PcmParams.OmniMain.Config[OMNI_INPUT_MODE]		= Omni.omniInputMode;
	gbl_params->PcmParams.OmniMain.Config[OMNI_SURROUND_MODE]	= Omni.omniSurroundMode;
	gbl_params->PcmParams.OmniMain.Config[OMNI_DIALOG_MODE]		= Omni.omniDialogMode;
	gbl_params->PcmParams.OmniMain.Config[OMNI_LFE_MODE]		= Omni.omniLfeMode;
	gbl_params->PcmParams.OmniMain.Config[OMNI_DIALOG_LEVEL]	= Omni.omniDialogLevel;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_OMNI_ID);
	}

	return error;
}


/******************************************************************************
 *  Function name 	: STAud_DecPPGetOmniParams
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Omni parameters settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetOmniParams(STAud_DecHandle_t		Handle,
                           				 STAUD_Omni_t * Omni_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	Omni_p->omniEnable			= gbl_params->PcmParams.OmniMain.Config[OMNI_ENABLE];
	Omni_p->omniInputMode		= gbl_params->PcmParams.OmniMain.Config[OMNI_INPUT_MODE];
	Omni_p->omniSurroundMode	= gbl_params->PcmParams.OmniMain.Config[OMNI_SURROUND_MODE];
	Omni_p->omniDialogMode		= gbl_params->PcmParams.OmniMain.Config[OMNI_DIALOG_MODE];
	Omni_p->omniLfeMode		= gbl_params->PcmParams.OmniMain.Config[OMNI_LFE_MODE];
	Omni_p->omniDialogLevel	= gbl_params->PcmParams.OmniMain.Config[OMNI_DIALOG_LEVEL];

	return error;
}


/******************************************************************************
 *  Function name 	: STAud_DecPPSetCircleSurrondParams
 *  Arguments    	:
 *       IN			:
 *       OUT    	: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming Omni settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetCircleSurrondParams(STAud_DecHandle_t		Handle,
                           				 STAUD_CircleSurrondII_t   *Csii)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.Csii.Id = ACC_CSII_ID;
	gbl_params->PcmParams.Csii.StructSize = sizeof(MME_OmniGlobalParams_t);
	gbl_params->PcmParams.Csii.Apply = ACC_MME_ENABLED;

	gbl_params->PcmParams.Csii.Config[CSII_ENABLE]			=
		(U8)Csii->Phantom + ((U8)Csii->CenterFB <<1) + ((U8)Csii->IS525Mode <<2) +
		((U8)Csii-> CenterRear<<3) + ((U8)Csii->RCenterFB<<4) + ((U8)Csii->TBSub<<5)+
		((U8)Csii->TBFront <<6)+ ((U8)Csii->FocusEnable<<7);

	gbl_params->PcmParams.Csii.Config[CSII_TBSIZE] = Csii->TBSize;

	gbl_params->PcmParams.Csii.Config[CSII_OUTMODE] = Csii->OutMode;

	gbl_params->PcmParams.Csii.Config[CSII_MODE] = Csii->Mode;

	gbl_params->PcmParams.Csii.InputGain =  Csii->InputGain;

	gbl_params->PcmParams.Csii.TBLevel = Csii->TBLevel;

	gbl_params->PcmParams.Csii.FocusElevation = Csii->FocusElevation;


	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_OMNI_ID);
	}

	return error;
}


/******************************************************************************
 *  Function name 	: STAud_DecPPGetOmniParams
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Omni parameters settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetCircleSurroundParams(STAud_DecHandle_t		Handle,
                           				 STAUD_CircleSurrondII_t * Csii)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	U8 CircleConfig=0;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;
	CircleConfig = gbl_params->PcmParams.Csii.Config[CSII_ENABLE]	;


	Csii->Phantom = (BOOL)(CircleConfig & 0x1);
	Csii->CenterFB =(BOOL)((CircleConfig >>1) & 0x1);
	Csii->IS525Mode = (BOOL)((CircleConfig >>2) & 0x1);
	Csii->CenterRear = (BOOL)((CircleConfig >>3) & 0x1);
	Csii->RCenterFB = (BOOL)((CircleConfig >>4) & 0x1);
	Csii->TBSub  	= (BOOL)((CircleConfig >>5) & 0x1);
	Csii->TBFront 	= (BOOL)((CircleConfig >>6) & 0x1);
	Csii->FocusEnable =(BOOL)((CircleConfig >>7) & 0x1);

	Csii->TBSize= gbl_params->PcmParams.Csii.Config[CSII_TBSIZE]  ;
	Csii->FocusElevation			= gbl_params->PcmParams.Csii.FocusElevation;
	Csii->TBLevel		= gbl_params->PcmParams.Csii.TBLevel ;
	Csii->InputGain	= gbl_params->PcmParams.Csii.InputGain;

	Csii->Mode		= gbl_params->PcmParams.Csii.Config[CSII_MODE];
	Csii->OutMode = gbl_params->PcmParams.Csii.Config[CSII_OUTMODE] ;


	return error;
}



/******************************************************************************
 *  Function name 	: STAud_DecPPSetSRSEffectParams
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming Effect settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetSRSEffectParams(STAud_DecHandle_t		Handle,
                           			  STAUD_EffectSRSParams_t	ParamType, S16 Value)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.TruSurXT.Id = ACC_TSXT_ID;
	gbl_params->PcmParams.TruSurXT.StructSize = sizeof(MME_TsxtGlobalParams_t);

	switch(ParamType)
	{
		case STAUD_EFFECT_FOCUS_ELEVATION :
			gbl_params->PcmParams.TruSurXT.FocusElevation =  Value;
			break;

		case STAUD_EFFECT_FOCUS_TWEETER_ELEVATION :
			gbl_params->PcmParams.TruSurXT.FocusTweeterElevation = Value;
			break;

		case STAUD_EFFECT_TRUBASS_LEVEL :
			gbl_params->PcmParams.TruSurXT.TBLevel = Value;
			break ;

		case STAUD_EFFECT_INPUT_GAIN :
			gbl_params->PcmParams.TruSurXT.InputGain = Value;
			break;

		case STAUD_EFFECT_TSXT_HEADPHONE :
			gbl_params->PcmParams.TruSurXT.Config[TSXT_HEADPHONE] = (BOOL) Value;
			break;

		case STAUD_EFFECT_TRUBASS_SIZE :
			gbl_params->PcmParams.TruSurXT.Config[TSXT_TBSIZE] = (U8) Value;
			/* This is an enum , containing values 0 to 8 so ok to typecast into U8*/
			break;

		case STAUD_EFFECT_TXST_MODE :
			gbl_params->PcmParams.TruSurXT.Config[TSXT_MODE] = (U8) Value;
			break;

		default :
			break;
	}


	/* Keep a copy of updated params with control block */
//	Dec_p->DecoderControlBlock.DecPcmProcessing.TruSurXT =  gbl_params->PcmParams.TruSurXT;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_TSXT_ID);
	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetEffect
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Effect settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetSRSEffectParams(STAud_DecHandle_t		Handle,
											STAUD_EffectSRSParams_t ParamType,
                           					S16	*Value_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;

	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	switch(ParamType)
	{
		case STAUD_EFFECT_FOCUS_ELEVATION :
			*Value_p = gbl_params->PcmParams.TruSurXT.FocusElevation ;
			break;

		case STAUD_EFFECT_FOCUS_TWEETER_ELEVATION :
			*Value_p = gbl_params->PcmParams.TruSurXT.FocusTweeterElevation;
			break;

		case STAUD_EFFECT_TRUBASS_LEVEL :
			*Value_p = gbl_params->PcmParams.TruSurXT.TBLevel ;
			break ;

		case STAUD_EFFECT_INPUT_GAIN :
			*Value_p = gbl_params->PcmParams.TruSurXT.InputGain;
			break;

		case STAUD_EFFECT_TSXT_HEADPHONE :
			*Value_p = gbl_params->PcmParams.TruSurXT.Config[TSXT_HEADPHONE] ;
			break;

		case STAUD_EFFECT_TRUBASS_SIZE :
			*Value_p = gbl_params->PcmParams.TruSurXT.Config[TSXT_TBSIZE];
			/* This is an enum , containing values 0 to 8 so ok to typecast into U8*/
			break;

		case STAUD_EFFECT_TXST_MODE :
			*Value_p = gbl_params->PcmParams.TruSurXT.Config[TSXT_MODE];
			break;

		default :
			break;


	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPSetStereoMode
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming Stereo settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetStereoMode(STAud_DecHandle_t		Handle,
                           			  							STAUD_StereoMode_t	StereoMode)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Stream Content will be zero at init time or after stop.*/
	if(Dec_p->DecoderControlBlock.StreamParams.StreamContent == 0)
	{
		/* case:: Setting PCM params before start */
		Dec_p->DecoderControlBlock.HoldStereoMode.StereoMode = StereoMode;
		Dec_p->DecoderControlBlock.HoldStereoMode.Apply      = TRUE;
		return error;
	}


	/* Set the new params here */
	gbl_params->PcmParams.CMC.Id = ACC_CMC_ID;
	gbl_params->PcmParams.CMC.StructSize = sizeof(MME_CMCGlobalParams_t);

#if 1
	switch(StereoMode)
	{
		case STAUD_STEREO_MODE_STEREO :
			gbl_params->PcmParams.CMC.Config[CMC_DUAL_MODE] = ACC_DUAL_LR;
			gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20;
			break;

		case STAUD_STEREO_MODE_PROLOGIC :
			gbl_params->PcmParams.CMC.Config[CMC_DUAL_MODE] = ACC_DUAL_LR;
			gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20t;
			break;

		case STAUD_STEREO_MODE_DUAL_LEFT :
			gbl_params->PcmParams.CMC.Config[CMC_DUAL_MODE] =ACC_DUAL_LEFT_MONO;
			gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20;
			break;

		case STAUD_STEREO_MODE_DUAL_RIGHT :
			gbl_params->PcmParams.CMC.Config[CMC_DUAL_MODE] = ACC_DUAL_RIGHT_MONO;
			gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20;
			break;

		case STAUD_STEREO_MODE_DUAL_MONO :
			gbl_params->PcmParams.CMC.Config[CMC_DUAL_MODE] = ACC_DUAL_MIX_LR_MONO;
			gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20;
			break;

		case STAUD_STEREO_MODE_AUTO :
			gbl_params->PcmParams.CMC.Config[CMC_DUAL_MODE] = ACC_DUAL_AUTO;

 /* GNBvd53729
 Setting StereoMode to AUTO is updating OutMode form Mode20_LFE to Mode20  Added if/else condition for mode setting */
			if(gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN] >= STAUD_ACC_MODE20t_LFE &&
				gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN] <=STAUD_ACC_MODE32_LFE )

				gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20_LFE;
			else
				gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20;
			break;
		case STAUD_STEREO_MODE_SECOND_STEREO :
			/* Call Down Mixer setting here */
			break;
		default:
			break;

	}
#else  //test if PP is working
	gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]    = (U8) ACC_MODE20;

#endif

	/* Keep a copy of updated params with control block */
//	Dec_p->DecoderControlBlock.DecPcmProcessing.CMC =  gbl_params->PcmParams.CMC;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_CMC_ID);
	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetStereoMode
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Stereo settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetStereoMode(STAud_DecHandle_t		Handle,
                           			  STAUD_StereoMode_t	*StereoMode)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
//    U8  CurEffect;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	switch(gbl_params->PcmParams.CMC.Config[CMC_DUAL_MODE])
	{
		case ACC_DUAL_LR:
			/*Default value */
			*StereoMode = STAUD_STEREO_MODE_STEREO;
			if(gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN] ==ACC_MODE20 )
				*StereoMode = STAUD_STEREO_MODE_STEREO;
			else if(gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN] ==ACC_MODE20t)
				*StereoMode = STAUD_STEREO_MODE_PROLOGIC;
			break;

		case ACC_DUAL_LEFT_MONO:
			*StereoMode = STAUD_STEREO_MODE_DUAL_LEFT;
			break;

		case ACC_DUAL_RIGHT_MONO:
			*StereoMode = STAUD_STEREO_MODE_DUAL_RIGHT;
			break;

		case ACC_DUAL_MIX_LR_MONO:
			*StereoMode = STAUD_STEREO_MODE_DUAL_MONO;
			break;
		case ACC_DUAL_AUTO :
			*StereoMode = STAUD_STEREO_MODE_AUTO;
			break;
		default:
			break;
	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetInStereoMode
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current Stereo settings of a particular decoder
 ***************************************************************************** */

ST_ErrorCode_t 	STAud_DecPPGetInStereoMode(STAud_DecHandle_t		Handle,
                           			  STAUD_StereoMode_t	*StereoMode)
{

	/* Local variables */
	ST_ErrorCode_t				error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	U8							InputStereoMode;
	
	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}
	if((Dec_p->DecoderControlBlock.StreamParams.StreamContent!=STAUD_STREAM_CONTENT_MPEG1)&&
	    (Dec_p->DecoderControlBlock.StreamParams.StreamContent!=STAUD_STREAM_CONTENT_MPEG2))
	    {
	    	return ST_ERROR_FEATURE_NOT_SUPPORTED;
	    }	

	/* Get stereo mode */
	InputStereoMode = Dec_p->DecoderControlBlock.InputStereoMode;

	switch(InputStereoMode)
	{
		case MP2E_STEREO:
			/*Default value */
			*StereoMode = MP2E_STEREO;
			break;

		case MP2E_JOINTSTEREO:
			*StereoMode = MP2E_JOINTSTEREO;
			break;

		case MP2E_DUALMONO:
			*StereoMode = MP2E_DUALMONO;
			break;

		case MP2E_MONO:
			*StereoMode = MP2E_MONO;
			break;
		default:
			break;
	}

	return error;
}


/******************************************************************************
 *  Function name 	: STAud_DecPPSetDeEmph
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming down mix settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetDeEmph(STAud_DecHandle_t		Handle,
											DeEmphParams_t    DeEmph)
{
	/* Local Variable */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
//    MME_Command_t  * cmd=NULL;

	STTBX_Print(("STAud_DecPPSetDeEmph: Entered \n "));

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	gbl_params->PcmParams.DeEmph.StructSize = sizeof(MME_DeEmphGlobalParams_t);
	gbl_params->PcmParams.DeEmph.Id = ACC_DeEMPH_ID;
	gbl_params->PcmParams.DeEmph.Apply = DeEmph.Apply;
	gbl_params->PcmParams.DeEmph.Mode = DeEmph.Mode;

	/* Keep a copy of updated params with control block */
//	Dec_p->DecoderControlBlock.DecPcmProcessing.DeEmph =  gbl_params->PcmParams.DeEmph;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_DeEMPH_ID);
	}
	return error;

}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetDeEmph
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function Get the Deemph settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetDeEmph(STAud_DecHandle_t		Handle,
											DeEmphParams_t    *DeEmph)
{
	/* Local Variable */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
//    MME_Command_t  * cmd=NULL;

	STTBX_Print(("STAud_DecPPSetDeEmph: Entered \n "));

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	DeEmph->Apply = gbl_params->PcmParams.DeEmph.Apply;
	DeEmph->Mode =  gbl_params->PcmParams.DeEmph.Mode;

	return error;

}


/******************************************************************************
 *  Function name 	: STAud_DecPPSetDolbyDigitalEx
 *  Arguments    	:
 *       IN			:
 *       OUT    	: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will enable/disable AC3Ex to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetDolbyDigitalEx(STAud_DecHandle_t		Handle,
                           				 BOOL DolbyDigitalEx)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.Ac3Ex.Id = ACC_AC3Ex_ID;
	gbl_params->PcmParams.Ac3Ex.StructSize = sizeof(MME_AC3ExGlobalParams_t);

	gbl_params->PcmParams.Ac3Ex.Apply = DolbyDigitalEx ? ACC_MME_ENABLED : ACC_MME_DISABLED;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_AC3Ex_ID);
	}

	return error;
}


/******************************************************************************
 *  Function name 	: STAud_DecPPGetDolbyDigitalEx
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function return current DolbyDigitalEx settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 		STAud_DecPPGetDolbyDigitalEx(STAud_DecHandle_t		Handle,
                           				 BOOL * DolbyDigitalEx_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	*DolbyDigitalEx_p = (gbl_params->PcmParams.Ac3Ex.Apply == ACC_MME_ENABLED) ? TRUE : FALSE;

	return error;
}



/******************************************************************************
 *  Function name 	: STAud_DecPPSetProLogic
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the Prologic settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetProLogic(STAud_DecHandle_t		Handle,
                           			  BOOL	Prologic)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.ProLogicII.Id = ACC_PLII_ID;
	gbl_params->PcmParams.ProLogicII.StructSize = sizeof(MME_PLIIGlobalParams_t);

	if(Prologic)
		gbl_params->PcmParams.ProLogicII.Apply = ACC_MME_ENABLED;
	else
		gbl_params->PcmParams.ProLogicII.Apply = ACC_MME_DISABLED;

	/* Give defalut setting here */
	gbl_params->PcmParams.ProLogicII.Config[PLII_DEC_MODE]			= (I8)0;
	gbl_params->PcmParams.ProLogicII.Config[PLII_AUTO_BALANCE]		= (I8)ACC_MME_ENABLED;
	gbl_params->PcmParams.ProLogicII.Config[PLII_SPEAKER_DIMENSION]	= (I8) 0;
	gbl_params->PcmParams.ProLogicII.Config[PLII_SURROUND_FILTER]		=(I8) ACC_MME_ENABLED;
	gbl_params->PcmParams.ProLogicII.Config[PLII_PANORAMA]			= (I8)ACC_MME_ENABLED;
	gbl_params->PcmParams.ProLogicII.Config[PLII_RS_POLARITY]			= (I8)ACC_MME_ENABLED;
	gbl_params->PcmParams.ProLogicII.Config[PLII_CENTRE_SPREAD]		= (I8) 0;
	gbl_params->PcmParams.ProLogicII.Config[PLII_OUT_MODE]			= (I8) ACC_MODE32;
	gbl_params->PcmParams.ProLogicII.PcmScale 						= (U32) 0x7FFF;

	/* Keep a copy of updated params with control block */
//	Dec_p->DecoderControlBlock.DecPcmProcessing.ProLogicII=  gbl_params->PcmParams.ProLogicII;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_PLII_ID);
	}
	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetProLogic
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will Get the Prologic settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetProLogic(STAud_DecHandle_t		Handle,
                           			  BOOL	*Prologic)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	*Prologic= gbl_params->PcmParams.ProLogicII.Apply;

	return error;
}
/******************************************************************************
 *  Function name 	: STAud_DecPPSetProLogicAdvance
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the Prologic settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetProLogicAdvance(STAud_DecHandle_t		Handle,
                           			 STAUD_PLIIParams_t PLIIParams)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	U32 TempValue=0xFF;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.ProLogicII.Id = ACC_PLII_ID;
	gbl_params->PcmParams.ProLogicII.StructSize = sizeof(MME_PLIIGlobalParams_t);

	switch(PLIIParams.Apply)
	{
		case STAUD_PLII_DISABLE:
			gbl_params->PcmParams.ProLogicII.Apply = ACC_MME_DISABLED;
			break;
		case STAUD_PLII_ENABLE:
			gbl_params->PcmParams.ProLogicII.Apply = ACC_MME_ENABLED;
			break;
		case STAUD_PLII_AUTO:
			gbl_params->PcmParams.ProLogicII.Apply = ACC_MME_AUTO;
			break;
		default :
			STTBX_Print(("STAud_DecPPSetProLogicAdvance : Error : Wrong Apply value  \n "));
			return STAUD_ERROR_WRONG_CMD_PARAM;
	}

	/* 	Set the values for PLII Params. If mode is STAUD_PLII_DISABLE or Auto, then these values
		should be ignored by LX
	*/

	/* Set Dec mode */
	TempValue = STAud_DecPPGetDecMode(PLIIParams.DecMode);
	if(TempValue == STAUD_ERROR_WRONG_CMD_PARAM)
		return TempValue;
	gbl_params->PcmParams.ProLogicII.Config[PLII_DEC_MODE] =  (I8)TempValue;
	/* Set Out mode */
	TempValue = STAud_DecPPGetOutMode(PLIIParams.OutMode);
	if(TempValue == STAUD_ERROR_WRONG_CMD_PARAM)
		return TempValue;
	gbl_params->PcmParams.ProLogicII.Config[PLII_OUT_MODE]	= (I8)TempValue;
	/* Set Other params */
	gbl_params->PcmParams.ProLogicII.Config[PLII_AUTO_BALANCE]		= (I8)ACC_MME_ENABLED;
	gbl_params->PcmParams.ProLogicII.Config[PLII_SPEAKER_DIMENSION]= (I8)PLIIParams.DimensionControl;
	gbl_params->PcmParams.ProLogicII.Config[PLII_SURROUND_FILTER]	= (I8)(PLIIParams.SurroundFilter == TRUE)?ACC_MME_ENABLED:ACC_MME_DISABLED ;
	gbl_params->PcmParams.ProLogicII.Config[PLII_PANORAMA]			= (I8)(PLIIParams.Panaroma == TRUE)?ACC_MME_ENABLED:ACC_MME_DISABLED;
	gbl_params->PcmParams.ProLogicII.Config[PLII_RS_POLARITY]		= (I8)(PLIIParams.ChannelPolarity== TRUE)?ACC_MME_ENABLED:ACC_MME_DISABLED ;
	gbl_params->PcmParams.ProLogicII.Config[PLII_CENTRE_SPREAD]	= (I8)PLIIParams.CentreWidth;
	gbl_params->PcmParams.ProLogicII.PcmScale 						= (U32) 0x7FFF;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_PLII_ID);
	}
	return error;

}
/******************************************************************************
 *  Function name 	: STAud_DecPPGetProLogicAdvance
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will Get the Prologic settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetProLogicAdvance(STAud_DecHandle_t		Handle,
                           			   STAUD_PLIIParams_t *PLIIParams)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	PLIIParams->Apply = gbl_params->PcmParams.ProLogicII.Apply;

	PLIIParams->DecMode		= gbl_params->PcmParams.ProLogicII.Config[PLII_DEC_MODE];
	PLIIParams->DimensionControl 	= gbl_params->PcmParams.ProLogicII.Config[PLII_SPEAKER_DIMENSION];
	PLIIParams->SurroundFilter 	= gbl_params->PcmParams.ProLogicII.Config[PLII_SURROUND_FILTER] ;
	PLIIParams->Panaroma		= gbl_params->PcmParams.ProLogicII.Config[PLII_PANORAMA];
	PLIIParams->ChannelPolarity 	= gbl_params->PcmParams.ProLogicII.Config[PLII_RS_POLARITY];
	PLIIParams->CentreWidth 		= gbl_params->PcmParams.ProLogicII.Config[PLII_CENTRE_SPREAD];
	PLIIParams->OutMode 		= gbl_params->PcmParams.ProLogicII.Config[PLII_OUT_MODE];

	return error;
}
/******************************************************************************
 *  Function name 	: STAud_DecPPGetCmd
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will Allocate a new MME command which will be sent to decoder.
 *				   It will also add the new command to link list of update param commands
 ***************************************************************************** */
MME_Command_t*	STAud_DecPPGetCmd(DecoderControlBlock_t	*DecCtrlBlock)
{
	UpdateGlobalParamList_t*	CmdNode=NULL;
	partition_t 		*DriverPartition = NULL;

	DriverPartition = DecCtrlBlock->InitParams.DriverPartition;

	/* Allocate memory for command */
	CmdNode = (UpdateGlobalParamList_t*)memory_allocate(DriverPartition,
						sizeof(UpdateGlobalParamList_t));
	if(CmdNode==NULL)
	{
		/* memory allocation failed. Return NULL Cmd */
		return NULL;
	}

    memset(CmdNode,0,sizeof(UpdateGlobalParamList_t));
	/* Add Command to update global cmd list */
	STAud_DecPPAddCmdToList(CmdNode,DecCtrlBlock);

	return (&CmdNode->cmd);

}

/******************************************************************************
 *  Function name 	: STAud_DecPPAddCmdToList
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will Allocate a new MME command which will be sent to decoder.
 *				   It will also add the new command to link list of update param commands
 ***************************************************************************** */
ST_ErrorCode_t	STAud_DecPPAddCmdToList(UpdateGlobalParamList_t *Node, DecoderControlBlock_t	*DecCtrlBlock)
{

	UpdateGlobalParamList_t *temp;
	UpdateGlobalParamList_t **Header;

	/* Protect the simultaneous updation of list */
	STOS_MutexLock(DecCtrlBlock->PPCmdMutex_p);

	Node->Next_p = NULL;

	Header = &DecCtrlBlock->UpdateGlobalParamList;
	/* First Node */
	if(*Header == NULL)
	{
		*Header = Node;
	}
	else
	{
		/* Find the end of List */
		temp = *Header;
		while(temp->Next_p != NULL)
			temp = temp->Next_p;

		/* Add the new node to list */
		temp->Next_p = Node;
	}

	STOS_MutexRelease(DecCtrlBlock->PPCmdMutex_p);
	/* Maintaining Count of no. of added commands in list*/
	GlobalParamsListCount++;

		if(DecCtrlBlock->UpdateGlobalParamList)
		{
			int count=0;
			UpdateGlobalParamList_t *templist;

			templist=DecCtrlBlock->UpdateGlobalParamList;
			STTBX_Print(( "STAud_DecPPAddCmdToList: *** UpdateGlobalParamList NOT NULL **** \n"));

			while(templist)
			{
				count++;
				templist=templist->Next_p;
			}
			STTBX_Print(("No of commands pending in globalParamList= %d\n",count));
		}
		else
		{
			STTBX_Print(("STAud_DecPPAddCmdToList: UpdateGlobalParamList=NULL \n"));
		}

	STTBX_Print(("STAud_DecPPAddCmdToList: GlobalParamsListCount = %d\n",GlobalParamsListCount));
	return	ST_NO_ERROR;
}
/******************************************************************************
 *  Function name 	: STAud_DecPPCleanCmdFrmList
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will removes the specified command from link list of update param
 *				   commands. It will also deallocate meory for that cmd.
 ***************************************************************************** */
ST_ErrorCode_t	STAud_DecPPCleanCmdFrmList(DecoderControlBlock_t	*DecCtrlBlock,
													MME_Command_t * cmd)
{

	UpdateGlobalParamList_t 	*parent, *temp;
	partition_t 				*DriverPartition = NULL;

	STTBX_Print(("STAud_DecPPCleanCmdFrmList: CB\n "));

	/* Protect the simultaneous updation of list */
	STOS_MutexLock(DecCtrlBlock->PPCmdMutex_p);

	/* No Node */
	if(DecCtrlBlock->UpdateGlobalParamList == NULL)
	{
		STTBX_Print(("ERROR: We should not have come here. NO COMMAND TO REMOVE!!!\n"));
		/* ERROR - We should not have came here. There is no command to remove */
	}
	else
	{
		GlobalParamsListCount--;
		/* Init Node Nalues*/
		parent = DecCtrlBlock->UpdateGlobalParamList;
		STTBX_Print(("STAud_DecPPCleanCmdFrmList: removing command \n"));
		DriverPartition = DecCtrlBlock->InitParams.DriverPartition;

		if((parent->Next_p !=NULL)&&(cmd != &(parent->cmd))) /*more than one nodes or node not equal to first*/
		{
			while(parent->Next_p)
			{
				if(cmd == &(parent->Next_p->cmd))
				{
					temp=parent->Next_p;
					parent->Next_p = parent->Next_p->Next_p;
					memory_deallocate(DriverPartition,temp);
					STTBX_Print(("STAud_DecPPCleanCmdFrmList: memory deallocated \n"));
					break;
				}
				parent=parent->Next_p;
			}
		}
		else /* only one node then delete it or if first node then delete it */
		{
			DecCtrlBlock->UpdateGlobalParamList = parent->Next_p;
			memory_deallocate(DriverPartition,parent);
		}

		if(DecCtrlBlock->UpdateGlobalParamList)
		{
			int count=0;
			UpdateGlobalParamList_t *templist;

			templist=DecCtrlBlock->UpdateGlobalParamList;
			STTBX_Print(("STAud_DecPPCleanCmdFrmList: *** UpdateGlobalParamList NOT NULL **** \n"));

			while(templist)
			{
				count++;
				templist=templist->Next_p;
			}
			STTBX_Print(("No of commands pending in globalParamList AFTER REMOVAL= %d\n",count));
		}
		else
		{
			STTBX_Print(("STAud_DecPPCleanCmdFrmList: UpdateGlobalParamList=NULL \n"));
		}

		STTBX_Print(("STAud_DecPPCleanCmdFrmList: GlobalParamsListCount = %d\n",GlobalParamsListCount));

	}

	STOS_MutexRelease(DecCtrlBlock->PPCmdMutex_p);
	return	ST_NO_ERROR;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPRemCmdFrmList
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will removes the first command from link list of update param
 *				   commands. It will also deallocate meory for that cmd.
 ***************************************************************************** */
ST_ErrorCode_t	STAud_DecPPRemCmdFrmList(DecoderControlBlock_t	*DecCtrlBlock)
{

	UpdateGlobalParamList_t 	*temp;
	partition_t 				*DriverPartition = NULL;

	STTBX_Print(("DecPPRemCmdFrmList: CB\n "));

	/* Protect the simultaneous updation of list */
	STOS_MutexLock(DecCtrlBlock->PPCmdMutex_p);

	/* No Node */
	if(DecCtrlBlock->UpdateGlobalParamList == NULL)
	{
		/* ERROR - We should not have came here. There is no command to remove */
	}
	else
	{
		GlobalParamsListCount--;
		/* Get second Node */
		temp = DecCtrlBlock->UpdateGlobalParamList->Next_p;

		/* free first node */
		DriverPartition = DecCtrlBlock->InitParams.DriverPartition;
		memory_deallocate(DriverPartition,DecCtrlBlock->UpdateGlobalParamList);
		STTBX_Print(("STAud_DecPPRemCmdFrmList: *** UpdateGlobalParamList DEALLOCATED **** \n"));
		/* reinit Header */
		DecCtrlBlock->UpdateGlobalParamList = temp;
		if(DecCtrlBlock->UpdateGlobalParamList)
		{
			int count=0;
			UpdateGlobalParamList_t *templist;

			templist=DecCtrlBlock->UpdateGlobalParamList;
			STTBX_Print(("STAud_DecPPRemCmdFrmList: *** UpdateGlobalParamList NOT NULL **** \n"));

			while(templist)
			{
				count++;
				templist=templist->Next_p;
			}
			STTBX_Print(("No of commands pending in globalParamList AFTER REMOVAL= %d\n",count));
		}
		else
		{
			STTBX_Print(("STAud_DecPPRemCmdFrmList: UpdateGlobalParamList=NULL \n"));
		}

		STTBX_Print(("STAud_DecPPRemCmdFrmList: GlobalParamsListCount = %d\n",GlobalParamsListCount));
	}

	STOS_MutexRelease(DecCtrlBlock->PPCmdMutex_p);
	return	ST_NO_ERROR;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPSendCmd
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function Allocate, Initialize and send the PCM procssing cmd to Decoder
 ***************************************************************************** */
ST_ErrorCode_t	STAud_DecPPSendCmd(DecoderControlBlockList_t *Dec_p, enum eAccDecPcmProcId PcmProc_id)
{
	/* Local Variable */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	MME_Command_t  * cmd=NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;

	STTBX_Print(("DecPPSendCmd: PCM cmd Recvd = %x\n ",PcmProc_id));

	/* Params should have been updated in the command which called this function */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the MME command which will be sent to Decoder for applying PP */
	cmd = STAud_DecPPGetCmd(&Dec_p->DecoderControlBlock);
	if( cmd == NULL)
	{
		/* No memory for command to be initialized */
		STTBX_Print(("STAud_DecPPSendCmd: Error cmd = %x\n ",PcmProc_id));
		return ST_ERROR_NO_MEMORY;
	}

	/* Fill and Send MME Command */
	error = acc_setAudioDecoderGlobalCmd(cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
                              PcmProc_id, Dec_p->DecoderControlBlock.InitParams.DriverPartition);
	if(error != MME_SUCCESS)
	{
		/* Need some better error type */
		STTBX_Print(("DecPPSendCmd: ERROR setAudioDecoderGlobalCmd: cmd = %x\n ",PcmProc_id));
		return ST_ERROR_NO_MEMORY;
	}

	/* Send MME_SET_GLOBAL_TRANSFORM_PARAMS Cmd */
	error = MME_SendCommand(Dec_p->DecoderControlBlock.TranformHandleDecoder, cmd);
	if(error != MME_SUCCESS)
	{
		 int CMDErrorCode;
	        STTBX_Print(("DecPPSendCmd: MME_SendCommand status = %x, cmd = %x \n ",error,(U32)cmd));
		 STTBX_Print(("MME_SendCommand returned ERROR: Freeing the command list \n"));
		 CMDErrorCode=STAud_DecPPCleanCmdFrmList(&Dec_p->DecoderControlBlock,cmd);
		 if(CMDErrorCode == ST_NO_ERROR)
		 {
			STTBX_Print(("STAud_DecPPSendCmd:Command freed from UpdateGlobalParamList \n"));
		 }
	 	else
	 	{
			STTBX_Print(("STAud_DecPPSendCmd:Command freeing FAILED \n"));
		}
	}

	return error;

}
#if 0
ST_ErrorCode_t 	STAud_DecPPSetDeEmph(STAud_DecHandle_t		Handle,
											DeEmphParams_t    DeEmph)
{
	/* Local Variable */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
	MME_Command_t  * cmd=NULL;

	STTBX_Print(("STAud_DecPPSetDeEmph: Entered \n "));

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	gbl_params->PcmParams.DeEmph.StructSize = sizeof(MME_DeEmphGlobalParams_t);
	gbl_params->PcmParams.DeEmph.Id = ACC_DeEMPH_ID;
	gbl_params->PcmParams.DeEmph.Apply = DeEmph.Apply;
	gbl_params->PcmParams.DeEmph.Mode = DeEmph.Mode;

	/* Get the MME command which will be sent to Decoder for applying PP */
	cmd = STAud_DecPPGetCmd(&Dec_p->DecoderControlBlock);
	if( cmd == NULL)
	{
		/* No memory for command to be initialized */
		STTBX_Print(("STAud_DecPPSetDeEmph: cmd == NULL\n "));
		return ST_ERROR_NO_MEMORY;
	}

	/* Fill and Send MME Command */
	error = acc_setAudioDecoderGlobalCmd(cmd, init_params, sizeof (TARGET_AUDIODECODER_INITPARAMS_STRUCT),
                              ACC_DeEMPH_ID);
	if(error != MME_SUCCESS)
	{
		/* Need some better error type */
		STTBX_Print(("STAud_DecPPSetDeEmph: acc_setAudioDecoderGlobalCmd ERROR \n "));
		return ST_ERROR_NO_MEMORY;
	}

	/* Send MME_SET_GLOBAL_TRANSFORM_PARAMS Cmd */
	error = MME_SendCommand(Dec_p->DecoderControlBlock.TranformHandleDecoder, cmd);

	STTBX_Print(("ApplyDeEmph: MME_SendCommand status = %x \n ",error));

	return error;

}
#endif
/******************************************************************************
 *  Function name 	: STAud_DecPPSetCompressionMode
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the Compression Mode settings to the
 *  AC3 decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetCompressionMode(STAud_DecHandle_t		Handle,
                           			  STAUD_CompressionMode_t	CompressionMode_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
	/* Decoder Config Params */
	MME_LxAc3Config_t	*ac3_config;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Stream Content will be zero at init time or after stop.*/
	if(Dec_p->DecoderControlBlock.StreamParams.StreamContent == 0)
	{
		/* case:: Setting PCM params before start */
		Dec_p->DecoderControlBlock.HoldCompressionMode.CompressionMode = CompressionMode_p;
		Dec_p->DecoderControlBlock.HoldCompressionMode.Apply = TRUE;
		return error;
	}

	/* Switch to the Decoder type */
	switch(Dec_p->DecoderControlBlock.StreamParams.StreamContent)
	{
		case STAUD_STREAM_CONTENT_AC3:
		case STAUD_STREAM_CONTENT_DDPLUS:
			ac3_config = (MME_LxAc3Config_t *)gbl_params->DecConfig;
			ac3_config->Config[DD_COMPRESS_MODE] = (U8) CompressionMode_p; //DD_LINE_OUT;
			break;
		default:
			/* Dynamic range not supported for "this" type of decoder */
			return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}
	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_LAST_DECPCMPROCESS_ID);
	}

	return error;
}
/******************************************************************************
 *  Function name 	: STAud_DecPPGetCompressionMode
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will Get the Compression Mode settings from the AC3 decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetCompressionMode(STAud_DecHandle_t		Handle,
                           			  STAUD_CompressionMode_t	*CompressionMode_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;
	/* Decoder Config Params */
	MME_LxAc3Config_t	*ac3_config;

	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}
	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* switch to the Decoder type */
	switch(Dec_p->DecoderControlBlock.StreamParams.StreamContent)
	{
		case STAUD_STREAM_CONTENT_AC3:
		case STAUD_STREAM_CONTENT_DDPLUS:
			ac3_config = (MME_LxAc3Config_t *)gbl_params->DecConfig;
			*CompressionMode_p = ac3_config->Config[DD_COMPRESS_MODE]; //DD_LINE_OUT;
			break;
		default:
			/* Compression Mode not supported for "this" type of decoder */
			return ST_ERROR_FEATURE_NOT_SUPPORTED;
	}

	return error;
}
/******************************************************************************
 *  Function name 	: STAud_DecPPSetAudioCodingMode
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming Audio coding Mode
 *                    settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetAudioCodingMode(STAud_DecHandle_t		Handle,
                           			  STAUD_AudioCodingMode_t AudioCodingMode_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/*GNBvd59573 In case of Stereo channel applying the Coding Mode to 30 Results in no Audio */
	if((Dec_p->DecoderControlBlock.NumChannels==2 )&& (AudioCodingMode_p>STAUD_ACC_MODE20))
			return ST_ERROR_FEATURE_NOT_SUPPORTED;

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	/* Set the new params here */
	gbl_params->PcmParams.DMix.Id = ACC_DMIX_ID;
	gbl_params->PcmParams.DMix.StructSize = sizeof(MME_DMixGlobalParams_t);// - (sizeof(tCoeff15) * 80);

	//gbl_params->PcmParams.DMix.Apply =  downmix.Apply;

	gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]= (U8) AudioCodingMode_p; //ACC_MODE20;
//	gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_AUX]= (U8) ACC_MODE_ID;;
	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_LAST_DECPCMPROCESS_ID);
	}

	return error;
}

/******************************************************************************
 *  Function name 	: STAud_DecPPGetAudioCodingMode
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will get the incoming Audio coding Mode
 *                    settings of a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPGetAudioCodingMode(STAud_DecHandle_t		Handle,
                           			  STAUD_AudioCodingMode_t *AudioCodingMode_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	*AudioCodingMode_p = gbl_params->PcmParams.CMC.Config[CMC_OUTMODE_MAIN]; //ACC_MODE20;

	return error;
}

U32 STAud_DecPPGetDecMode(STAUD_PLIIDecMode_t DecMode)
{
	U32 tempmode = 0xFF;

	switch(DecMode)
	{
		case STAUD_DECMODE_PROLOGIC:
			tempmode = PLII_PROLOGIC ;
			break;
		case STAUD_DECMODE_VIRTUAL:
			tempmode = PLII_VIRTUAL ;
			break;
		case STAUD_DECMODE_MUSIC:
			tempmode = PLII_MUSIC ;
			break;
		case STAUD_DECMODE_MOVIE:
			tempmode = PLII_MOVIE ;
			break;
		case STAUD_DECMODE_MATRIX:
			tempmode = PLII_MATRIX ;
			break;
		default:
			STTBX_Print(("STAud_DecPPSetProLogicAdvance : Error : Wrong Dec Mode  \n "));
			tempmode = STAUD_ERROR_WRONG_CMD_PARAM;
	}
	return tempmode;
}

U32 STAud_DecPPGetOutMode(STAUD_PLIIOutMode_t OutMode)
{
	U32 tempmode = 0xFF;

	switch(OutMode)
	{
		case STAUD_PLII_MODE22:
			tempmode =  ACC_MODE22;
			break;
		case STAUD_PLII_MODE30:
			tempmode =  ACC_MODE30;
			break;
		case STAUD_PLII_MODE32:
			tempmode =  ACC_MODE32;
			break;
		default:
			STTBX_Print(("STAud_DecPPSetProLogicAdvance : Error : Wrong Out Mode  \n "));
			return STAUD_ERROR_WRONG_CMD_PARAM;
	}
	return tempmode;

}


/******************************************************************************
 *  Function name 	: STAud_DecPPSetPan
 *  Arguments    	:
 *       IN			:
 *       OUT    		: Error status
 *       INOUT		:
 *  Return       	: ST_ErrorCode_t
 *  Description   	: This function will apply the incoming PAN settings to a particular decoder
 ***************************************************************************** */
ST_ErrorCode_t 	STAud_DecPPSetPan(STAud_DecHandle_t		Handle)
{
	/* Local variables */
	ST_ErrorCode_t 			error		= ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p		= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT		*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT		*gbl_params;
	U8 LeftAttenuation,RightAttenuation;
    	U32 Pan;
    	STAUD_Attenuation_t  				TempAttenuation_p;
	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;
	Pan = Dec_p->DecoderControlBlock.ADControl.CurPan;
	TempAttenuation_p = Dec_p->DecoderControlBlock.Attenuation_p;
	LeftAttenuation  = (PanTable[Pan][0]) + TempAttenuation_p.Left;
	RightAttenuation = (PanTable[Pan][1]) + TempAttenuation_p.Right;

    	STTBX_Print(("PAN=%d\n",Pan));
	STTBX_Print(("PanTable[0][0]=%d\n",PanTable[Pan][0]));
	STTBX_Print(("PanTable[1][0]=%d\n",PanTable[Pan][1]));

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	if((LeftAttenuation != TempAttenuation_p.Left) && (RightAttenuation != TempAttenuation_p.Right))
	{
		gbl_params->PcmParams.DMix.Id = ACC_DMIX_ID;
		gbl_params->PcmParams.DMix.StructSize = sizeof(MME_DMixGlobalParams_t);

		gbl_params->PcmParams.DMix.Apply =  ACC_MME_ENABLED;

		gbl_params->PcmParams.DMix.Config[DMIX_MONO_UPMIX]= ACC_MME_ENABLED;
	}
	/* Set the new params here */
	gbl_params->PcmParams.BassMgt.Id = ACC_DECPCMPRO_BASSMGT_ID;
	gbl_params->PcmParams.BassMgt.StructSize = sizeof(MME_BassMgtGlobalParams_t);

	gbl_params->PcmParams.BassMgt.Apply =  ACC_MME_ENABLED;

	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LEFT ] = LeftAttenuation;
	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RGHT ] = RightAttenuation;
	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CNTR ] = TempAttenuation_p.Center;
	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LFE  ] = TempAttenuation_p.Subwoofer;
	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_LSUR ] = TempAttenuation_p.LeftSurround;
	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_RSUR ] = TempAttenuation_p.RightSurround;
	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CSURL] = 0;
	gbl_params->PcmParams.BassMgt.Volume[ACC_MAIN_CSURR] = 0;
	gbl_params->PcmParams.BassMgt.Volume[ACC_AUX_LEFT  ] = 0;
	gbl_params->PcmParams.BassMgt.Volume[ACC_AUX_RGHT  ] = 0;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_LAST_DECPCMPROCESS_ID);
	}

	return error;
}
ST_ErrorCode_t 	STAud_DecPPSetDTSNeo(STAud_DecHandle_t		Handle,STAUD_DTSNeo_t  *DTSNeo_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	if(( DTSNeo_p->OutputChanels>7|| DTSNeo_p->OutputChanels<3)||DTSNeo_p->NeoMode==0)
	{
		return ST_ERROR_BAD_PARAMETER;
	}

	/* Set the new params here */
	gbl_params->PcmParams.DTSNeo.Id = ACC_NEO_ID;
	gbl_params->PcmParams.DTSNeo.StructSize=sizeof(MME_NEOGlobalParams_t);

	gbl_params->PcmParams.DTSNeo.Apply = DTSNeo_p->Enable;

	gbl_params->PcmParams.DTSNeo.Config[NEO_IO_STARTIDX]=0; /*Always Zero */
	gbl_params->PcmParams.DTSNeo.Config[NEO_CENTRE_GAIN]=DTSNeo_p->CenterGain;
	gbl_params->PcmParams.DTSNeo.Config[NEO_SETUP_MODE]= DTSNeo_p->NeoMode;
	gbl_params->PcmParams.DTSNeo.Config[NEO_AUXMODE_FLAG]= DTSNeo_p->NeoAUXMode;
	gbl_params->PcmParams.DTSNeo.Config[NEO_OUTPUT_CHAN]= DTSNeo_p->OutputChanels;

	if(Dec_p->DecoderControlBlock.Transformer_Init)
	{
		error = STAud_DecPPSendCmd(Dec_p,ACC_NEO_ID);

	}

	return error;
}


ST_ErrorCode_t 	STAud_DecPPGetDTSNeo(STAud_DecHandle_t		Handle,STAUD_DTSNeo_t  *DTSNeo_p)
{
	/* Local variables */
	ST_ErrorCode_t 			error = ST_NO_ERROR;
	DecoderControlBlockList_t	*Dec_p= (DecoderControlBlockList_t*)NULL;
	TARGET_AUDIODECODER_INITPARAMS_STRUCT   	*init_params;
	TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *gbl_params;


	Dec_p = STAud_DecGetControlBlockFromHandle(Handle);
	if(Dec_p == NULL)
	{
		/* handle not found */
		return ST_ERROR_INVALID_HANDLE;
	}

	/* Get decoder init params */
	init_params = Dec_p->DecoderControlBlock.MMEinitParams.TransformerInitParams_p;

	/* Get the Global Parmeters from the current decoder */
	gbl_params = (TARGET_AUDIODECODER_GLOBALPARAMS_STRUCT *) &init_params->GlobalParams;

	DTSNeo_p->Enable=gbl_params->PcmParams.DTSNeo.Apply;

	DTSNeo_p->CenterGain= gbl_params->PcmParams.DTSNeo.Config[NEO_CENTRE_GAIN];
	DTSNeo_p->NeoAUXMode=gbl_params->PcmParams.DTSNeo.Config[NEO_AUXMODE_FLAG];
	DTSNeo_p->NeoMode=gbl_params->PcmParams.DTSNeo.Config[NEO_SETUP_MODE];
	DTSNeo_p->OutputChanels=gbl_params->PcmParams.DTSNeo.Config[NEO_OUTPUT_CHAN];


	return error;
}

