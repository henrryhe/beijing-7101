/*****************************************************************************
 *
 *  Module      : staudlx_core_exports.c
 *  Date        : 02-07-2005
 *  Author      : WALKERM
 *  Description : Implementation for exporting STAPI functions
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/version.h> /* Kernel version */

#include "staudlx.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

/* Exported STAUDLX functions */
EXPORT_SYMBOL(STAUD_Close);
EXPORT_SYMBOL(STAUD_DPSetCallback);
EXPORT_SYMBOL(STAUD_DRConnectSource);
EXPORT_SYMBOL(STAUD_PPDisableDownsampling);
EXPORT_SYMBOL(STAUD_PPEnableDownsampling);
EXPORT_SYMBOL(STAUD_IPGetBroadcastProfile);
EXPORT_SYMBOL(STAUD_DRGetCapability);
EXPORT_SYMBOL(STAUD_DRGetDynamicRangeControl);
EXPORT_SYMBOL(STAUD_IPGetSamplingFrequency);
EXPORT_SYMBOL(STAUD_DRGetSpeed);
EXPORT_SYMBOL(STAUD_IPGetStreamInfo);
EXPORT_SYMBOL(STAUD_DRGetSyncOffset);
EXPORT_SYMBOL(STAUD_DRPause);
EXPORT_SYMBOL(STAUD_DRPrepare);
EXPORT_SYMBOL(STAUD_DRResume);
EXPORT_SYMBOL(STAUD_OPGetCapability);
EXPORT_SYMBOL(STAUD_OPGetParams);
EXPORT_SYMBOL(STAUD_OPEnableSynchronization);
EXPORT_SYMBOL(STAUD_OPDisableSynchronization);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
EXPORT_SYMBOL(STAUD_DRSetClockRecoverySource);
#endif
EXPORT_SYMBOL(STAUD_OPSetDigitalMode);
EXPORT_SYMBOL(STAUD_OPGetDigitalMode);
EXPORT_SYMBOL(STAUD_OPMute);
EXPORT_SYMBOL(STAUD_OPSetParams);
EXPORT_SYMBOL(STAUD_OPUnMute);
EXPORT_SYMBOL(STAUD_DRSetDynamicRangeControl);
EXPORT_SYMBOL(STAUD_DRSetSpeed);
EXPORT_SYMBOL(STAUD_IPSetStreamID);
EXPORT_SYMBOL(STAUD_IPGetSynchroUnit);
EXPORT_SYMBOL(STAUD_IPPauseSynchro);
EXPORT_SYMBOL(STAUD_IPSkipSynchro);
EXPORT_SYMBOL(STAUD_DRSetSyncOffset);
EXPORT_SYMBOL(STAUD_DRStart);
EXPORT_SYMBOL(STAUD_DRStop);
EXPORT_SYMBOL(STAUD_GetCapability);
EXPORT_SYMBOL(STAUD_GetRevision);
EXPORT_SYMBOL(STAUD_Init);
EXPORT_SYMBOL(STAUD_IPGetCapability);
EXPORT_SYMBOL(STAUD_IPGetDataInterfaceParams);
EXPORT_SYMBOL(STAUD_IPGetParams);
EXPORT_SYMBOL(STAUD_IPGetInputBufferParams);
EXPORT_SYMBOL(STAUD_IPSetDataInputInterface);
EXPORT_SYMBOL(STAUD_IPQueuePCMBuffer);
EXPORT_SYMBOL(STAUD_IPSetLowDataLevelEvent);
EXPORT_SYMBOL(STAUD_IPSetParams);
EXPORT_SYMBOL(STAUD_MXGetCapability);
EXPORT_SYMBOL(STAUD_MXSetInputParams);
EXPORT_SYMBOL(STAUD_Open);
EXPORT_SYMBOL(STAUD_PPGetAttenuation);
EXPORT_SYMBOL(STAUD_PPGetDelay);
EXPORT_SYMBOL(STAUD_PPGetSpeakerEnable);
EXPORT_SYMBOL(STAUD_PPGetSpeakerConfig);
EXPORT_SYMBOL(STAUD_PPSetAttenuation);
EXPORT_SYMBOL(STAUD_PPSetDelay);
EXPORT_SYMBOL(STAUD_PPSetSpeakerEnable);
EXPORT_SYMBOL(STAUD_PPSetSpeakerConfig);
EXPORT_SYMBOL(STAUD_MXConnectSource); //?
EXPORT_SYMBOL(STAUD_PPGetCapability);
/* EXPORT_SYMBOL(STAUD_PPGetEffect); */
EXPORT_SYMBOL(STAUD_PPGetKaraoke);
EXPORT_SYMBOL(STAUD_GetBufferParam);

#if 0
EXPORT_SYMBOL(STAUD_PPSetEffect);
EXPORT_SYMBOL(STAUD_PPSetStereoModeNow);
#endif
EXPORT_SYMBOL(STAUD_PPSetKaraoke);
EXPORT_SYMBOL(STAUD_PPConnectSource);
EXPORT_SYMBOL(STAUD_Term);
EXPORT_SYMBOL(STAUD_DRSetDeEmphasisFilter);
EXPORT_SYMBOL(STAUD_DRGetDeEmphasisFilter);
EXPORT_SYMBOL(STAUD_DRSetEffect);
EXPORT_SYMBOL(STAUD_DRGetEffect);
EXPORT_SYMBOL(STAUD_DRSetPrologic);
EXPORT_SYMBOL(STAUD_DRGetPrologic);
EXPORT_SYMBOL(STAUD_DRSetStereoMode);
EXPORT_SYMBOL(STAUD_DRGetStereoMode);
EXPORT_SYMBOL(STAUD_DRGetInStereoMode);
EXPORT_SYMBOL(STAUD_DRMute);
EXPORT_SYMBOL(STAUD_DRUnMute);
EXPORT_SYMBOL(STAUD_PPDcRemove);
EXPORT_SYMBOL(STAUD_DRSetCompressionMode);
EXPORT_SYMBOL(STAUD_DRSetAudioCodingMode);
EXPORT_SYMBOL(STAUD_DRGetAudioCodingMode);
EXPORT_SYMBOL(STAUD_IPSetPCMParams);
EXPORT_SYMBOL(STAUD_IPGetPCMBuffer);
EXPORT_SYMBOL(STAUD_IPGetPCMBufferSize);
EXPORT_SYMBOL(STAUD_DRGetSRSEffectParams);
EXPORT_SYMBOL(STAUD_DRSetDownMix);
EXPORT_SYMBOL(STAUD_DRSetSRSEffectParams);
EXPORT_SYMBOL(STAUD_SetClockRecoverySource);
EXPORT_SYMBOL(STAUD_OPEnableHDMIOutput);
EXPORT_SYMBOL(STAUD_OPDisableHDMIOutput);
EXPORT_SYMBOL(STAUD_MXUpdatePTSStatus);
EXPORT_SYMBOL(STAUD_DRGetBeepToneFrequency);
EXPORT_SYMBOL(STAUD_DRSetBeepToneFrequency);
EXPORT_SYMBOL(STAUD_MXSetMixLevel);
EXPORT_SYMBOL(STAUD_IPSetBroadcastProfile);
EXPORT_SYMBOL(STAUD_IPGetBitBufferFreeSize);
EXPORT_SYMBOL(STAUD_IPSetPCMReaderParams);
EXPORT_SYMBOL(STAUD_DRSetDolbyDigitalEx);
EXPORT_SYMBOL(STAUD_DRGetDolbyDigitalEx);
EXPORT_SYMBOL(STAUD_PPGetEqualizationParams);
EXPORT_SYMBOL(STAUD_PPSetEqualizationParams);
EXPORT_SYMBOL(STAUD_DRSetOmniParams);
EXPORT_SYMBOL(STAUD_DRSetInitParams);
EXPORT_SYMBOL(STAUD_DRGetOmniParams);
EXPORT_SYMBOL(STAUD_ModuleStart);
EXPORT_SYMBOL(STAUD_GenericStart);
EXPORT_SYMBOL(STAUD_IPGetPCMReaderParams);
EXPORT_SYMBOL(STAUD_ModuleStop);
EXPORT_SYMBOL(STAUD_GenericStop);
EXPORT_SYMBOL(STAUD_MXUpdateMaster);
EXPORT_SYMBOL(STAUD_OPSetLatency);
EXPORT_SYMBOL(STAUD_DRGetPrologicAdvance);
EXPORT_SYMBOL(STAUD_DRSetPrologicAdvance);
EXPORT_SYMBOL(STAUD_IPHandleEOF);
EXPORT_SYMBOL(STAUD_DRSetDDPOP);
EXPORT_SYMBOL(STAUD_ENCGetCapability);
EXPORT_SYMBOL(STAUD_ENCSetOutputParams);
EXPORT_SYMBOL(STAUD_ENCGetOutputParams);
EXPORT_SYMBOL(STAUD_ConnectSrcDst);
EXPORT_SYMBOL(STAUD_DisconnectInput);
EXPORT_SYMBOL(STAUD_SetModuleAttenuation);
EXPORT_SYMBOL(STAUD_GetModuleAttenuation );
EXPORT_SYMBOL(STAUD_DRGetStreamInfo);

/* Conditional compilation (#if's) etc taken from aud_wrap.c */
#ifndef STB
#define STB
#endif

#ifndef STAUD_NO_WRAPPER_LAYER

EXPORT_SYMBOL(STAUD_DisableDeEmphasis);
#if defined (STB)
EXPORT_SYMBOL(STAUD_DisableSynchronisation);
#endif
EXPORT_SYMBOL(STAUD_EnableDeEmphasis);
#if defined (STB)
EXPORT_SYMBOL(STAUD_EnableSynchronisation);
#endif
EXPORT_SYMBOL(STAUD_GetAttenuation);
EXPORT_SYMBOL(STAUD_GetChannelDelay);
EXPORT_SYMBOL(STAUD_GetDigitalOutput);
EXPORT_SYMBOL(STAUD_GetDynamicRangeControl);
EXPORT_SYMBOL(STAUD_GetEffect);
EXPORT_SYMBOL(STAUD_GetKaraokeOutput);
EXPORT_SYMBOL(STAUD_GetPrologic);
EXPORT_SYMBOL(STAUD_GetSpeaker);
EXPORT_SYMBOL(STAUD_GetSpeakerConfiguration);
EXPORT_SYMBOL(STAUD_GetStereoOutput);
#if defined (DVD) || defined (ST_7100)
EXPORT_SYMBOL(STAUD_GetSynchroUnit);
#endif
EXPORT_SYMBOL(STAUD_Mute);
EXPORT_SYMBOL(STAUD_Pause);
#if defined (DVD)
EXPORT_SYMBOL(STAUD_PauseSynchro);
#endif
#if defined (DVD) || defined (ST_7100)
EXPORT_SYMBOL(STAUD_Play);
#endif
#if defined (DVD)
EXPORT_SYMBOL(STAUD_Prepare);
#endif
EXPORT_SYMBOL(STAUD_Resume);
EXPORT_SYMBOL(STAUD_SetAttenuation);
EXPORT_SYMBOL(STAUD_SetChannelDelay);
EXPORT_SYMBOL(STAUD_SetDigitalOutput);
EXPORT_SYMBOL(STAUD_SetDynamicRangeControl);
EXPORT_SYMBOL(STAUD_SetEffect);
EXPORT_SYMBOL(STAUD_SetKaraokeOutput);
EXPORT_SYMBOL(STAUD_SetPrologic);
EXPORT_SYMBOL(STAUD_SetSpeaker);
EXPORT_SYMBOL(STAUD_SetSpeakerConfiguration);
EXPORT_SYMBOL(STAUD_SetStereoOutput);
EXPORT_SYMBOL(STAUD_OPSetEncDigitalOutput);
EXPORT_SYMBOL(STAUD_DRSetCircleSurroundParams);
EXPORT_SYMBOL(STAUD_DRGetCircleSurroundParams);

EXPORT_SYMBOL(STAUD_DRSetDTSNeoParams);
EXPORT_SYMBOL(STAUD_DRGetDTSNeoParams);

EXPORT_SYMBOL(STAUD_PPSetBTSCParams);
EXPORT_SYMBOL(STAUD_PPGetBTSCParams);
EXPORT_SYMBOL(STAUD_SetScenario);

#if defined (DVD)
EXPORT_SYMBOL(STAUD_SkipSynchro);
#endif
#if defined (STB)
EXPORT_SYMBOL(STAUD_Start);
#endif
EXPORT_SYMBOL(STAUD_Stop);
#if 0
EXPORT_SYMBOL(STAUD_GetInputBufferParams);
#endif
EXPORT_SYMBOL(STAUD_PlayerStart);
EXPORT_SYMBOL(STAUD_PlayerStop);
#if 0
EXPORT_SYMBOL(AUD_IntHandlerInstall);
#endif
#endif
