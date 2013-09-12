/********************************************************************************
 *   Filename   	: Aud_MMEAudioDecoderPP.h											*
 *   Start       		: 02.09.2005                                                   							*
 *   By          		: Chandandeep Singh Pabla											*
 *   Contact     	: chandandeep.pabla@st.com										*
 *   Description  	: The file contains the MME based Audio Decoder APIs that will be			*
 *				  Exported to the Modules.											*
 ********************************************************************************
 */

#ifndef __MMEAUDDECPP_H__
#define __MMEAUDDECPP_H__

/* {{{ Includes */
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stlite.h"
#include "stddefs.h"

/* MME FIles */
#include "acc_mme.h"
#include "pcm_postprocessingtypes.h"
#include "audio_decodertypes.h"
#include "acc_multicom_toolbox.h"
#include "aud_decoderparams.h"
#include "acc_mmedefines.h"
#include "aud_mmeaudiostreamdecoder.h"
#include "mp2a_decodertypes.h"
#include "dolbydigital_decodertypes.h"
#include "dts_decodertypes.h"
#include "mlp_decodertypes.h"

#include "staudlx.h"
#include "blockmanager.h"

#include "stos.h"
/* }}} */

/* {{{ Defines */

/* }}} */


/* {{{ Structs */
/*
typedef struct DownmixParams_s
{
	U16				Apply;
	BOOL				stereoUpmix;
	BOOL				monoUpmix;
	BOOL				meanSurround;
	BOOL				secondStereo;
	BOOL				normalize;
	U32				normalizeTableIndex;
	BOOL				dialogEnhance;
}DownmixParams_t;
*/
/* DeEmph Struct */
typedef struct DeEmphParams_s
{
	enum eAccProcessApply  Apply;
	enum eAccDeemphMode    Mode;

}DeEmphParams_t;

/* }}} */


/* {{{ Function defines */

/* Interface Fucntions */
ST_ErrorCode_t 	STAud_DecPPSetDownMix(STAud_DecHandle_t	Handle,STAUD_DownmixParams_t	*downmix);
ST_ErrorCode_t 	STAud_DecPPGetDownMix(STAud_DecHandle_t		Handle, STAUD_DownmixParams_t	*downmix);

ST_ErrorCode_t 	STAud_DecPPSetCompressionMode(STAud_DecHandle_t	Handle,STAUD_CompressionMode_t	CompressionMode_p);
ST_ErrorCode_t 	STAud_DecPPGetCompressionMode(STAud_DecHandle_t		Handle,STAUD_CompressionMode_t	*CompressionMode_p);

ST_ErrorCode_t 	STAud_DecPPSetAudioCodingMode(STAud_DecHandle_t	Handle,STAUD_AudioCodingMode_t AudioCodingMode_p);
ST_ErrorCode_t 	STAud_DecPPGetAudioCodingMode(STAud_DecHandle_t		Handle,STAUD_AudioCodingMode_t *AudioCodingMode_p);

ST_ErrorCode_t 	STAud_DecPPSetDynamicRange(STAud_DecHandle_t Handle, STAUD_DynamicRange_t	*DyRange);
ST_ErrorCode_t 	STAud_DecPPGetDynamicRange(STAud_DecHandle_t Handle, STAUD_DynamicRange_t	*DyRange);
ST_ErrorCode_t 	STAud_DecPPSetDeEmph(STAud_DecHandle_t	Handle,	DeEmphParams_t	DeEmph);
ST_ErrorCode_t 	STAud_DecPPGetDeEmph(STAud_DecHandle_t	Handle,DeEmphParams_t    *DeEmph);

ST_ErrorCode_t 	STAud_DecPPSetDolbyDigitalEx(STAud_DecHandle_t		Handle, BOOL DolbyDigitalEx);
ST_ErrorCode_t 	STAud_DecPPGetDolbyDigitalEx(STAud_DecHandle_t		Handle, BOOL	*DolbyDigitalEx_p);

ST_ErrorCode_t 	STAud_DecPPSetProLogic(STAud_DecHandle_t		Handle, BOOL	Prologic);
ST_ErrorCode_t 	STAud_DecPPGetProLogic(STAud_DecHandle_t		Handle, BOOL	*Prologic);

ST_ErrorCode_t 	STAud_DecPPSetProLogicAdvance(STAud_DecHandle_t		Handle,STAUD_PLIIParams_t PLIIParams);
ST_ErrorCode_t 	STAud_DecPPGetProLogicAdvance(STAud_DecHandle_t		Handle,STAUD_PLIIParams_t *PLIIParams);

ST_ErrorCode_t 	STAud_DecPPSetEffect(STAud_DecHandle_t		Handle,STAUD_Effect_t	  effect);
ST_ErrorCode_t 	STAud_DecPPGetEffect(STAud_DecHandle_t		Handle,STAUD_Effect_t	* effect);

ST_ErrorCode_t 	STAud_DecPPSetOmniParams(STAud_DecHandle_t		Handle,STAUD_Omni_t   Omni);
ST_ErrorCode_t 	STAud_DecPPGetOmniParams(STAud_DecHandle_t		Handle,STAUD_Omni_t * Omni_p);

ST_ErrorCode_t	STAud_DecPPSetCircleSurrondParams(STAud_DecHandle_t		Handle,STAUD_CircleSurrondII_t   *Csii);
ST_ErrorCode_t	STAud_DecPPGetCircleSurroundParams(STAud_DecHandle_t		Handle,STAUD_CircleSurrondII_t * Csii);

ST_ErrorCode_t 	STAud_DecPPSetSRSEffectParams(STAud_DecHandle_t	Handle,STAUD_EffectSRSParams_t	ParamType, S16 Value);
ST_ErrorCode_t 	STAud_DecPPGetSRSEffectParams(STAud_DecHandle_t	Handle,STAUD_EffectSRSParams_t ParamType,S16 *Value_p);


ST_ErrorCode_t 	STAud_DecPPSetStereoMode(STAud_DecHandle_t		Handle,STAUD_StereoMode_t	StereoMode);
ST_ErrorCode_t 	STAud_DecPPGetStereoMode(STAud_DecHandle_t		Handle,STAUD_StereoMode_t	*StereoMode);
ST_ErrorCode_t 	STAud_DecPPGetInStereoMode(STAud_DecHandle_t	Handle,STAUD_StereoMode_t	*StereoMode);

ST_ErrorCode_t 	STAud_DecPPSetVol(STAud_DecHandle_t		Handle,STAUD_Attenuation_t  *Attenuation_p);
ST_ErrorCode_t 	STAud_DecPPGetVol(STAud_DecHandle_t		Handle,STAUD_Attenuation_t  *Attenuation_p);

ST_ErrorCode_t 	STAud_DecPPSetDTSNeo(STAud_DecHandle_t		Handle,STAUD_DTSNeo_t  *DTSNeo_p);
ST_ErrorCode_t 	STAud_DecPPGetDTSNeo(STAud_DecHandle_t		Handle,STAUD_DTSNeo_t  *DTSNeo_p);


MME_Command_t*	STAud_DecPPGetCmd(DecoderControlBlock_t	*DecCtrlBlock);
ST_ErrorCode_t	STAud_DecPPAddCmdToList(UpdateGlobalParamList_t *Node, DecoderControlBlock_t	*DecCtrlBlock);
ST_ErrorCode_t	STAud_DecPPRemCmdFrmList(DecoderControlBlock_t	*DecCtrlBlock);
ST_ErrorCode_t	STAud_DecPPSendCmd(DecoderControlBlockList_t *Dec_p, enum eAccDecPcmProcId PcmProc_id);
ST_ErrorCode_t 	STAud_DecPPSetSpeed(STAud_DecHandle_t	Handle, S32 Speed);

U32	STAud_DecPPGetOutMode(STAUD_PLIIOutMode_t OutMode);
U32 STAud_DecPPGetDecMode(STAUD_PLIIDecMode_t DecMode);

ST_ErrorCode_t 	STAud_DecPPSetPan(STAud_DecHandle_t		Handle);
/* }}} */
#endif /* __MMEAUDDECPP_H__ */



