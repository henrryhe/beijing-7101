/*******************************************************

 File Name: staudlx.h

 Description: Header File for STAUDLX

 Copyright: STMicroelectronics (c) 2005

*******************************************************/

#ifndef __STAUDLXPRINTS_H
    #define __STAUDLXPRINTS_H

/* Includes ----------------------------------------- */
    #include "staudlx.h"

    #ifdef __cplusplus
        extern "C" {
    #endif

    #ifdef STAUD_VERBOSE_PRINT

        void PrintSTAUD_OPSetAttenuation        (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, STAUD_Attenuation_t * Attenuation_p);

        void PrintSTAUD_DRSetDecodingType       (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_PPSetStereoMode         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_StereoMode_t StereoMode);

        void PrintSTAUD_Close                   (const STAUD_Handle_t Handle);

        void PrintSTAUD_DRConnectSource         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_Object_t InputSource);

        void PrintSTAUD_ConnectSrcDst           (STAUD_Handle_t Handle, STAUD_Object_t DestObject, U32 DstId, STAUD_Object_t SrcObject,U32 SrcId);

        void PrintSTAUD_DisconnectInput         (STAUD_Handle_t Handle, STAUD_Object_t Object,U32 InputId);

        void PrintSTAUD_PPDisableDownsampling   (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject);


        void PrintSTAUD_PPEnableDownsampling    (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, U32 );

        void PrintSTAUD_DRSetBeepToneFrequency  (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, U32 BeepToneFrequency);

        void PrintSTAUD_DRGetBeepToneFrequency  (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, U32 *BeepToneFreq_p);


        void PrintSTAUD_IPGetBroadcastProfile   (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_BroadcastProfile_t * BroadcastProfile_p);

        void PrintSTAUD_DRGetCapability         (const ST_DeviceName_t DeviceName, STAUD_Object_t DecoderObject, STAUD_DRCapability_t *Capability_p);

        void PrintSTAUD_DRGetDynamicRangeControl(STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_DynamicRange_t * DynamicRange_p);

        void PrintSTAUD_DRSetDownMix            (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_DownmixParams_t * DownMixParam_p);

        void PrintSTAUD_DRGetDownMix            (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_DownmixParams_t * DownMixParam_p);

        void PrintSTAUD_IPGetSamplingFrequency  (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U32 *SamplingFrequency_p);

        void PrintSTAUD_DRGetStreamInfo         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_StreamInfo_t * StreamInfo_p);

        void PrintSTAUD_DRGetSpeed              (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, S32 *Speed_p);

        void PrintSTAUD_IPGetStreamInfo         (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_StreamInfo_t *StreamInfo_p);

        void PrintSTAUD_DRGetSyncOffset         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, S32 *Offset_p);

        void PrintSTAUD_DRPause                 (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_Fade_t *Fade_p);

        void PrintSTAUD_DRPrepare               (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_StreamParams_t *StreamParams_p);

        void PrintSTAUD_DRResume                (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject);


        void PrintSTAUD_OPGetCapability         (const ST_DeviceName_t DeviceName, STAUD_Object_t OutputObject,STAUD_OPCapability_t *Capability_p);

        void PrintSTAUD_OPGetParams             (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, STAUD_OutputParams_t *Params_p);

        void PrintSTAUD_OPEnableSynchronization (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_OPDisableSynchronization(STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_OPEnableHDMIOutput      (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_OPDisableHDMIOutput     (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        #ifndef STAUD_REMOVE_CLKRV_SUPPORT
            void PrintSTAUD_DRSetClockRecoverySource (STAUD_Handle_t Handle, STAUD_Object_t Object, STCLKRV_Handle_t ClkSource);
        #endif

        void PrintSTAUD_OPSetDigitalMode        (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, STAUD_DigitalMode_t OutputMode);

        void PrintSTAUD_OPSetAnalogMode         (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, STAUD_PCMMode_t OutputMode);

        void PrintSTAUD_OPSetEncDigitalOutput   (const STAUD_Handle_t Handle, STAUD_Object_t OutputObject, BOOL EnableDisableEncOutput);

        void PrintSTAUD_OPGetDigitalMode        (STAUD_Handle_t Handle, STAUD_Object_t OutputObject,STAUD_DigitalMode_t * OutputMode);

        void PrintSTAUD_OPGetAnalogMode         (STAUD_Handle_t Handle, STAUD_Object_t OutputObject,STAUD_PCMMode_t * OutputMode);

        void PrintSTAUD_OPMute                  (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_OPSetParams             (STAUD_Handle_t Handle, STAUD_Object_t OutputObject,STAUD_OutputParams_t *Params_p);

        void PrintSTAUD_OPUnMute                (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_DRSetDynamicRangeControl(STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_DynamicRange_t *DynamicRange_p);

        void PrintSTAUD_DRSetCompressionMode    (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_CompressionMode_t  CompressionMode_p);

        void   PrintSTAUD_DRSetAudioCodingMode  (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_AudioCodingMode_t AudioCodingMode_p);

        void PrintSTAUD_DRSetSpeed              (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, S32 Speed);

        void PrintSTAUD_IPSetStreamID           (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U8 StreamID);

        void PrintSTAUD_DRSetSyncOffset         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, S32 Offset);

        void PrintSTAUD_OPSetLatency            (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, U32 Latency);

        void PrintSTAUD_DRStart                 (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_StreamParams_t *StreamParams_p);

        void PrintSTAUD_DRStop                  (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_Stop_t StopMode, STAUD_Fade_t *Fade_p);

        void PrintSTAUD_GetCapability           (const ST_DeviceName_t DeviceName, STAUD_Capability_t *Capability_p);

        void PrintSTAUD_Init                    (const ST_DeviceName_t DeviceName,  STAUD_InitParams_t *InitParams_p);

        void PrintSTAUD_IPGetCapability         (const ST_DeviceName_t DeviceName, STAUD_Object_t InputObject,STAUD_IPCapability_t *Capability_p);

        void PrintSTAUD_IPGetParams             (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_InputParams_t *InputParams_p);

        void PrintSTAUD_IPGetInputBufferParams  (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_BufferParams_t* DataParams_p);

        void  PrintSTAUD_IPSetDataInputInterface(STAUD_Handle_t Handle, STAUD_Object_t InputObject, GetWriteAddress_t   GetWriteAddress_p,InformReadAddress_t InformReadAddress_p, void * const BufferHandle_p);

        void PrintSTAUD_IPQueuePCMBuffer        (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_PCMBuffer_t *PCMBuffer_p,    U32 NumBuffers, U32 *NumQueued_p);

        void PrintSTAUD_IPGetPCMBuffer          (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_PCMBuffer_t * PCMBuffer_p);

        void PrintSTAUD_IPGetPCMBufferSize      (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U32 * BufferSize);

        void PrintSTAUD_IPSetLowDataLevelEvent  (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U8 Level);

        void PrintSTAUD_IPSetParams             (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_InputParams_t *InputParams_p);

        void PrintSTAUD_IPSetPCMParams          (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_PCMInputParams_t *InputParams_p);

        void PrintSTAUD_IPSetBroadcastProfile   (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_BroadcastProfile_t    BroadcastProfile);

        void PrintSTAUD_IPSetPCMReaderParams    (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_PCMReaderConf_t *InputParams_p);

        void PrintSTAUD_IPGetPCMReaderParams    (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_PCMReaderConf_t *InputParams_p);

        void PrintSTAUD_IPGetPCMReaderCapability(STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_ReaderCapability_t *InputParams_p);

        void PrintSTAUD_IPGetBitBufferFreeSize  (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U32 * Size_p);

        void PrintSTAUD_IPGetSynchroUnit        (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_SynchroUnit_t *SynchroUnit_p);

        void PrintSTAUD_IPSkipSynchro           (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U32 Delay);

        void PrintSTAUD_IPPauseSynchro          (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U32 Delay);

        void PrintSTAUD_IPSetWMAStreamID        (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U8 WMAStreamNumber);

        void PrintSTAUD_MXConnectSource         (STAUD_Handle_t Handle, STAUD_Object_t MixerObject, STAUD_Object_t *InputSources_p,  STAUD_MixerInputParams_t *  InputParams_p, U32 NumInputs);

        void PrintSTAUD_MXGetMixLevel           (STAUD_Handle_t Handle, STAUD_Object_t MixerObject, U32 InputID,  U16 *MixLevel_p);

        void PrintSTAUD_MXGetCapability         (const ST_DeviceName_t DeviceName, STAUD_Object_t MixerObject, STAUD_MXCapability_t *Capability_p);

        void PrintSTAUD_MXSetInputParams        (STAUD_Handle_t Handle, STAUD_Object_t MixerObject, STAUD_Object_t InputSource,  STAUD_MixerInputParams_t *   InputParams_p);

        void  PrintSTAUD_MXSetMixLevel          (STAUD_Handle_t Handle, STAUD_Object_t MixerObject, U32 InputID, U16 MixLevel);

        void PrintSTAUD_Open                    (const ST_DeviceName_t DeviceName, const STAUD_OpenParams_t *Params_p, STAUD_Handle_t *Handle_p);

        void PrintSTAUD_PPGetAttenuation        (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Attenuation_t *Attenuation_p);

        void PrintSTAUD_PPGetDelay              (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, STAUD_Delay_t *Delay_p);

        void PrintSTAUD_PPGetEqualizationParams (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Equalization_t *Equalization_p);


        void PrintSTAUD_PPGetSpeakerEnable      (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_SpeakerEnable_t *SpeakerEnable_p);

        void PrintSTAUD_PPGetSpeakerConfig      (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_SpeakerConfiguration_t *    SpeakerConfig_p);

        void PrintSTAUD_PPSetAttenuation        (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Attenuation_t *Attenuation_p);

        void PrintSTAUD_PPSetDelay              (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Delay_t *Delay_p);

        void PrintSTAUD_PPSetEqualizationParams (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Equalization_t *Equalization_p);

        void PrintSTAUD_PPSetSpeakerEnable      (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, STAUD_SpeakerEnable_t *SpeakerEnable_p);

        void PrintSTAUD_PPSetSpeakerConfig      (STAUD_Handle_t Handle, STAUD_Object_t OutputObject, STAUD_SpeakerConfiguration_t *   SpeakerConfig_p,   STAUD_BassMgtType_t BassType);

        void PrintSTAUD_PPConnectSource         (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Object_t InputSource);

        void PrintSTAUD_PPGetCapability         (const ST_DeviceName_t DeviceName, STAUD_Object_t PostProcObject,STAUD_PPCapability_t *Capability_p);

        void PrintSTAUD_PPGetKaraoke            (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Karaoke_t *Karaoke_p);

        void PrintSTAUD_PPSetKaraoke            (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_Karaoke_t Karaoke);

        void PrintSTAUD_Term                    (const ST_DeviceName_t DeviceName, const STAUD_TermParams_t *TermParams_p);

        /* ++ Decoder level PP */
        void PrintSTAUD_DRSetDeEmphasisFilter   (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, BOOL Emphasis);

        void PrintSTAUD_DRGetDeEmphasisFilter   (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, BOOL *Emphasis_p);

        void PrintSTAUD_DRSetDolbyDigitalEx     (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, BOOL DolbyDigitalEx);

        void PrintSTAUD_DRGetDolbyDigitalEx     (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, BOOL * DolbyDigitalEx_p);

        void PrintSTAUD_DRSetEffect             (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_Effect_t Effect);

        void PrintSTAUD_DRGetEffect             (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_Effect_t * Effect_p);

        void PrintSTAUD_DRSetOmniParams         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_Omni_t Omni);

        void PrintSTAUD_DRGetOmniParams         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_Omni_t * Omni_p);

        void PrintSTAUD_DRSetCircleSurroundParams   (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_CircleSurrondII_t CSii);

        void PrintSTAUD_DRGetCircleSurroundParams   (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_CircleSurrondII_t * CSii_p);

        void PrintSTAUD_DRSetSRSEffectParams    (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_EffectSRSParams_t ParamType,S16 Value);

        void PrintSTAUD_DRGetSRSEffectParams    (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_EffectSRSParams_t ParamType,S16 *Value);

        void PrintSTAUD_DRSetPrologic           (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, BOOL Prologic);

        void PrintSTAUD_DRGetPrologic           (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, BOOL * Prologic_p);

        void PrintSTAUD_DRSetPrologicAdvance    (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_PLIIParams_t PLIIParams);

        void PrintSTAUD_DRGetPrologicAdvance    (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_PLIIParams_t *PLIIParams);

        void PrintSTAUD_DRSetStereoMode         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_StereoMode_t StereoMode);

        void PrintSTAUD_SelectDac(void);

        void PrintSTAUD_DRGetStereoMode         (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_StereoMode_t * StereoMode_p);

        void PrintSTAUD_DRMute                  (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_DRUnMute                (STAUD_Handle_t Handle, STAUD_Object_t OutputObject);

        void PrintSTAUD_DRSetDDPOP              (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, U32 DDPOPSetting);

        void PrintSTAUD_DRGetSamplingFrequency  (STAUD_Handle_t Handle, STAUD_Object_t InputObject, U32 * SamplingFrequency_p);

        void PrintSTAUD_DRSetInitParams         (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_DRInitParams_t * InitParams);

        void PrintSTAUD_ENCGetCapability        (const ST_DeviceName_t DeviceName, STAUD_Object_t EncoderObject, STAUD_ENCCapability_t *Capability_p);

        void PrintSTAUD_ENCGetOutputParams      (STAUD_Handle_t Handle, STAUD_Object_t EncoderObject, STAUD_ENCOutputParams_s *EncoderOPParams_p);

        void PrintSTAUD_ENCSetOutputParams      (STAUD_Handle_t Handle, STAUD_Object_t EncoderObject, STAUD_ENCOutputParams_s EncoderOPParams);

        void PrintSTAUD_ModuleStart             (STAUD_Handle_t Handle, STAUD_Object_t ModuleObject, STAUD_StreamParams_t * StreamParams_p);

        void PrintSTAUD_GenericStart            (STAUD_Handle_t Handle);

        void PrintSTAUD_ModuleStop              (STAUD_Handle_t Handle, STAUD_Object_t ModuleObject);

        void PrintSTAUD_GenericStop             (STAUD_Handle_t Handle);

        void PrintSTAUD_SetModuleAttenuation    (STAUD_Handle_t Handle, STAUD_Object_t ModuleObject, STAUD_Attenuation_t *Attenuation_p);

        void PrintSTAUD_GetModuleAttenuation    (STAUD_Handle_t Handle, STAUD_Object_t ModuleObject, STAUD_Attenuation_t * Attenuation_p);

        /* Wrapper Layer function prototypes */

        #ifndef STAUD_NO_WRAPPER_LAYER

            void PrintSTAUD_DisableDeEmphasis       (const STAUD_Handle_t Handle);

            void PrintSTAUD_DisableSynchronisation  (const STAUD_Handle_t Handle);

            void PrintSTAUD_EnableDeEmphasis        (const STAUD_Handle_t Handle);

            void PrintSTAUD_EnableSynchronisation   (const STAUD_Handle_t Handle);

            void PrintSTAUD_GetAttenuation          (const STAUD_Handle_t Handle, STAUD_Attenuation_t *Attenuation);

            void PrintSTAUD_GetChannelDelay         (const STAUD_Handle_t Handle, STAUD_Delay_t *Delay);

            void PrintSTAUD_GetDigitalOutput        (const STAUD_Handle_t Handle, STAUD_DigitalOutputConfiguration_t * Mode);

            void PrintSTAUD_GetDynamicRangeControl  (const STAUD_Handle_t Handle, U8 *Control);

            void PrintSTAUD_GetEffect               (const STAUD_Handle_t Handle, STAUD_Effect_t *Mode);

            void PrintSTAUD_GetKaraokeOutput        (const STAUD_Handle_t Handle, STAUD_Karaoke_t *Mode);

            void PrintSTAUD_GetPrologic             (const STAUD_Handle_t Handle, BOOL *PrologicState);

            void PrintSTAUD_GetSpeaker              (const STAUD_Handle_t Handle, STAUD_Speaker_t *Speaker);

            void PrintSTAUD_GetSpeakerConfiguration (const STAUD_Handle_t Handle, STAUD_SpeakerConfiguration_t * Speaker);

            void PrintSTAUD_GetStereoOutput         (const STAUD_Handle_t Handle, STAUD_Stereo_t *Mode);

            void PrintSTAUD_GetSynchroUnit          (STAUD_Handle_t Handle, STAUD_SynchroUnit_t *SynchroUnit_p);

            void PrintSTAUD_Mute                    (const STAUD_Handle_t Handle, BOOL AnalogueOutput, BOOL DigitalOutput);

            void PrintSTAUD_Pause                   (const STAUD_Handle_t Handle, STAUD_Fade_t *Fade);

            void PrintSTAUD_PauseSynchro            (const STAUD_Handle_t Handle, U32 Unit);

            void PrintSTAUD_Play                    (const STAUD_Handle_t Handle);

            void PrintSTAUD_Prepare                 (const STAUD_Handle_t Handle, STAUD_Prepare_t *Params);

            void PrintSTAUD_Resume                  (const STAUD_Handle_t Handle);

            void PrintSTAUD_SetAttenuation          (const STAUD_Handle_t Handle, STAUD_Attenuation_t Attenuation);

            void PrintSTAUD_SetChannelDelay         (const STAUD_Handle_t Handle, STAUD_Delay_t Delay);

            void PrintSTAUD_SetDigitalOutput        (const STAUD_Handle_t Handle, STAUD_DigitalOutputConfiguration_t Mode);

            void PrintSTAUD_SetDynamicRangeControl  (const STAUD_Handle_t Handle, U8 Control);

            void PrintSTAUD_SetEffect               (const STAUD_Handle_t Handle, const STAUD_Effect_t Mode);

            void PrintSTAUD_SetKaraokeOutput        (const STAUD_Handle_t Handle, const STAUD_Karaoke_t Mode);

            void PrintSTAUD_SetPrologic             (const STAUD_Handle_t Handle);

            void PrintSTAUD_SetSpeaker              (const STAUD_Handle_t Handle, const STAUD_Speaker_t Speaker);

            void PrintSTAUD_SetSpeakerConfiguration (const STAUD_Handle_t Handle, STAUD_SpeakerConfiguration_t Speaker,STAUD_BassMgtType_t BassType);

            void PrintSTAUD_SetStereoOutput         (const STAUD_Handle_t Handle, const STAUD_Stereo_t Mode);

            void PrintSTAUD_SkipSynchro             (STAUD_Handle_t Handle, STAUD_AudioPlayerID_t PlayerID,U32 Delay);

            void PrintSTAUD_Start                   (const STAUD_Handle_t Handle, STAUD_StartParams_t *Params);

            void PrintSTAUD_Stop                    (const STAUD_Handle_t Handle, const STAUD_Stop_t StopMode,STAUD_Fade_t *Fade);

            void PrintSTAUD_PlayerStart             (const STAUD_Handle_t Handle);

            void PrintSTAUD_PlayerStop              (const STAUD_Handle_t Handle);

            void PrintSTAUD_GetInputBufferParams    (STAUD_Handle_t Handle, STAUD_AudioPlayerID_t PlayerID,STAUD_BufferParams_t* DataParams_p);

            #ifndef STAUD_REMOVE_CLKRV_SUPPORT
                void PrintSTAUD_SetClockRecoverySource (STAUD_Handle_t Handle, STCLKRV_Handle_t ClkSource);
            #endif

            void PrintAUD_IntHandlerInstall         (int InterruptNumber, int InterruptLevel); /* For time being */

            void PrintSTAUD_PPDcRemove              (STAUD_Handle_t Handle, STAUD_Object_t PPObject,STAUD_DCRemove_t *Params_p);
        #endif

        #ifdef ST_OSLINUX

            void PrintSTAUD_MapBufferToUserSpace    (STAUD_BufferParams_t* DataParams_p);

            void PrintSTAUD_UnmapBufferFromUserSpace(STAUD_BufferParams_t* DataParams_p);

        #endif

        void PrintSTAUD_DPSetCallback           (STAUD_Handle_t Handle, STAUD_Object_t DPObject, FrameDeliverFunc_t Func_fp, void * clientInfo);

        void PrintSTAUD_MXUpdatePTSStatus       (STAUD_Handle_t Handle, STAUD_Object_t MixerObject, U32 InputID, BOOL PTSStatus);

        void PrintSTAUD_MXUpdateMaster          (STAUD_Handle_t Handle, STAUD_Object_t MixerObject, U32 InputID, BOOL FreeRun);

        void PrintSTAUD_DRGetCompressionMode    (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_CompressionMode_t *CompressionMode_p);

        void PrintSTAUD_DRGetAudioCodingMode    (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_AudioCodingMode_t * AudioCodingMode_p);

        void PrintSTAUD_IPHandleEOF             (STAUD_Handle_t Handle, STAUD_Object_t InputObject);

        void PrintSTAUD_IPGetDataInterfaceParams(STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_DataInterfaceParams_t *DMAParams_p);

        void PrintSTAUD_GetBufferParam          (STAUD_Handle_t Handle, STAUD_Object_t InputObject, STAUD_GetBufferParam_t *BufferParam);

        void PrintSTAUD_DRSetDTSNeoParams       (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_DTSNeo_t *DTSNeo_p);

        void PrintSTAUD_DRGetDTSNeoParams       (STAUD_Handle_t Handle, STAUD_Object_t DecoderObject, STAUD_DTSNeo_t *DTSNeo_p);

        void PrintSTAUD_PPSetBTSCParams         (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_BTSC_t *BTSC_p);

        void PrintSTAUD_PPGetBTSCParams         (STAUD_Handle_t Handle, STAUD_Object_t PostProcObject, STAUD_BTSC_t *BTSC_p);

        void PrintSTAUD_SetScenario             (STAUD_Handle_t Handle, STAUD_Scenario_t Scenario);

    #else
        #define PrintSTAUD_OPSetAttenuation (x,y,z)

        #define PrintSTAUD_DRSetDecodingType(x,y)

        #define PrintSTAUD_PPSetStereoMode (x,y,z)

        #define PrintSTAUD_Close(x)

        #define PrintSTAUD_DRConnectSource(x,y,z)

        #define PrintSTAUD_ConnectSrcDst(a,b,c,d,e)

        #define PrintSTAUD_DisconnectInput(x,y,z)

        #define PrintSTAUD_PPDisableDownsampling(x,y)

        #define PrintSTAUD_PPEnableDownsampling(x,y,z)

        #define PrintSTAUD_DRSetBeepToneFrequency(x,y,z)

        #define PrintSTAUD_DRGetBeepToneFrequency(x,y,z)

        #define PrintSTAUD_IPGetBroadcastProfile(x,y,z)

        #define PrintSTAUD_DRGetCapability(x,y,z)

        #define PrintSTAUD_DRGetDynamicRangeControl(x,y,z)

        #define PrintSTAUD_DRSetDownMix(x,y,z)

        #define PrintSTAUD_DRGetDownMix(x,y,z)

        #define PrintSTAUD_IPGetSamplingFrequency(x,y,z)

        #define PrintSTAUD_DRGetStreamInfo(x,y,z)

        #define PrintSTAUD_DRGetSpeed(x,y,z)

        #define PrintSTAUD_IPGetStreamInfo(x,y,z)

        #define PrintSTAUD_DRGetSyncOffset(x,y,z)

        #define PrintSTAUD_DRPause(x,y,z)

        #define PrintSTAUD_DRPrepare(x,y,z)

        #define PrintSTAUD_DRResume(x,y)

        #define PrintSTAUD_OPGetCapability(x,y,z)

        #define PrintSTAUD_OPGetParams(x,y,z)

        #define PrintSTAUD_OPEnableSynchronization(x,y)

        #define PrintSTAUD_OPDisableSynchronization(x,y)

        #define PrintSTAUD_OPEnableHDMIOutput(x,y)

        #define PrintSTAUD_OPDisableHDMIOutput(x,y)

        #ifndef STAUD_REMOVE_CLKRV_SUPPORT
            #define PrintSTAUD_DRSetClockRecoverySource(x,y,z)
        #endif

        #define PrintSTAUD_OPSetDigitalMode(x,y,z)

        #define PrintSTAUD_OPSetAnalogMode(x,y,z)

        #define PrintSTAUD_OPSetEncDigitalOutput(x,y,z)

        #define PrintSTAUD_OPGetDigitalMode(x,y,z)

        #define PrintSTAUD_OPGetAnalogMode(x,y,z)

        #define PrintSTAUD_OPMute(x,y)

        #define PrintSTAUD_OPSetParams(x,y,z)

        #define PrintSTAUD_OPUnMute(x,y)

        #define PrintSTAUD_DRSetDynamicRangeControl(x,y,z)

        #define PrintSTAUD_DRSetCompressionMode(x,y,z)

        #define   PrintSTAUD_DRSetAudioCodingMode(x,y,z)

        #define PrintSTAUD_DRSetSpeed(x,y,z)

        #define PrintSTAUD_IPSetStreamID(x,y,z)

        #define PrintSTAUD_DRSetSyncOffset(x,y,z)

        #define PrintSTAUD_OPSetLatency(x,y,z)

        #define PrintSTAUD_DRStart(x,y,z)

        #define PrintSTAUD_DRStop(x,y,z,a)

        #define PrintSTAUD_GetCapability(x,y)

        #define PrintSTAUD_Init(y,z)

        #define PrintSTAUD_IPGetCapability(x,y,z)

        #define PrintSTAUD_IPGetParams(x,y,z)

        #define PrintSTAUD_IPGetInputBufferParams(x,y,z)

        #define  PrintSTAUD_IPSetDataInputInterface(x,y,z,a,b)

        #define PrintSTAUD_IPQueuePCMBuffer(x,y,z,a,b)

        #define PrintSTAUD_IPGetPCMBuffer(x,y,z)

        #define PrintSTAUD_IPGetPCMBufferSize(x,y,z)

        #define PrintSTAUD_IPSetLowDataLevelEvent(x,y,z)

        #define PrintSTAUD_IPSetParams(x,y,z)

        #define PrintSTAUD_IPSetPCMParams(x,y,z)

        #define PrintSTAUD_IPSetBroadcastProfile(x,y,z)

        #define PrintSTAUD_IPSetPCMReaderParams(x,y,z)

        #define PrintSTAUD_IPGetPCMReaderParams(x,y,z)

        #define PrintSTAUD_IPGetPCMReaderCapability(x,y,z)

        #define PrintSTAUD_IPGetBitBufferFreeSize(x,y,z)

        #define PrintSTAUD_IPGetSynchroUnit(x,y,z)

        #define PrintSTAUD_IPSkipSynchro(x,y,z)

        #define PrintSTAUD_IPPauseSynchro(x,y,z)

        #define PrintSTAUD_IPSetWMAStreamID(x,y,z)

        #define PrintSTAUD_MXConnectSource(x,y,z,a,b)

        #define PrintSTAUD_MXGetMixLevel(x,y,z,a)

        #define PrintSTAUD_MXGetCapability(x,y,z)

        #define PrintSTAUD_MXSetInputParams(x,y,z,a)

        #define  PrintSTAUD_MXSetMixLevel(x,y,z,a)

        #define PrintSTAUD_Open(x,y,z)

        #define PrintSTAUD_PPGetAttenuation(x,y,z)

        #define PrintSTAUD_PPGetDelay(x,y,z)

        #define PrintSTAUD_PPGetEqualizationParams(x,y,z)

        #define PrintSTAUD_PPGetSpeakerEnable(x,y,z)

        #define PrintSTAUD_PPGetSpeakerConfig(x,y,z)

        #define PrintSTAUD_PPSetAttenuation(x,y,z)

        #define PrintSTAUD_PPSetDelay(x,y,z)

        #define PrintSTAUD_PPSetEqualizationParams(x,y,z)

        #define PrintSTAUD_PPSetSpeakerEnable(x,y,z)

        #define PrintSTAUD_PPSetSpeakerConfig(x,y,z,a)

        #define PrintSTAUD_PPConnectSource(x,y,z)

        #define PrintSTAUD_PPGetCapability(x,y,z)

        #define PrintSTAUD_PPGetKaraoke(x,y,z)

        #define PrintSTAUD_PPSetKaraoke(x,y,z)

        #define PrintSTAUD_Term(y,z)

        /* ++ Decoder level PP */
        #define PrintSTAUD_DRSetDeEmphasisFilter(x,y,z)

        #define PrintSTAUD_DRGetDeEmphasisFilter(x,y,z)

        #define PrintSTAUD_DRSetDolbyDigitalEx(x,y,z)

        #define PrintSTAUD_DRGetDolbyDigitalEx(x,y,z)

        #define PrintSTAUD_DRSetEffect(x,y,z)

        #define PrintSTAUD_DRGetEffect(x,y,z)

        #define PrintSTAUD_DRSetOmniParams(x,y,z)

        #define PrintSTAUD_DRGetOmniParams(x,y,z)

        #define PrintSTAUD_DRSetCircleSurroundParams(x,y,z)

        #define PrintSTAUD_DRGetCircleSurroundParams(x,y,z)

        #define PrintSTAUD_DRSetSRSEffectParams(x,y,z,a)

        #define PrintSTAUD_DRGetSRSEffectParams(x,y,z,a)

        #define PrintSTAUD_DRSetPrologic(x,y,z)

        #define PrintSTAUD_DRGetPrologic(x,y,z)

        #define PrintSTAUD_DRSetPrologicAdvance(x,y,z)

        #define PrintSTAUD_DRGetPrologicAdvance(x,y,z)

        #define PrintSTAUD_DRSetStereoMode(x,y,z)

        #define PrintSTAUD_SelectDac()

        #define PrintSTAUD_DRGetStereoMode(x,y,z)

        #define PrintSTAUD_DRMute(x,y)

        #define PrintSTAUD_DRUnMute(x,y)

        #define PrintSTAUD_DRSetDDPOP(x,y,z)

        #define PrintSTAUD_DRGetSamplingFrequency(x,y,z)

        #define PrintSTAUD_DRSetInitParams(x,y,z)

        #define PrintSTAUD_ENCGetCapability(x,y,z)

        #define PrintSTAUD_ENCGetOutputParams(x,y,z)

        #define PrintSTAUD_ENCSetOutputParams(x,y,z)

        #define PrintSTAUD_ModuleStart(x,y,z)

        #define PrintSTAUD_GenericStart(x);

        #define PrintSTAUD_ModuleStop(x,y);

        #define PrintSTAUD_GenericStop(x);

        #define PrintSTAUD_SetModuleAttenuation(x,y,z)

        #define PrintSTAUD_GetModuleAttenuation(x,y,z)
        /* -- Decoder Level PP */

        /* Wrapper Layer function prototypes */

        #ifndef STAUD_NO_WRAPPER_LAYER

            #define PrintSTAUD_DisableDeEmphasis(x)

            #define PrintSTAUD_DisableSynchronisation(x)

            #define PrintSTAUD_EnableDeEmphasis(x)

            #define PrintSTAUD_EnableSynchronisation(x)

            #define PrintSTAUD_GetAttenuation(x,y)

            #define PrintSTAUD_GetChannelDelay(x,y)

            #define PrintSTAUD_GetDigitalOutput(cx,y)

            #define PrintSTAUD_GetDynamicRangeControl(x,y)

            #define PrintSTAUD_GetEffect(x,y)

            #define PrintSTAUD_GetKaraokeOutput(x,y)

            #define PrintSTAUD_GetPrologic(x,y)

            #define PrintSTAUD_GetSpeaker(x,y)

            #define PrintSTAUD_GetSpeakerConfiguration(x,y)

            #define PrintSTAUD_GetStereoOutput(x,y)

            #define PrintSTAUD_GetSynchroUnit(x,y)

            #define PrintSTAUD_Mute(x,y,z)

            #define PrintSTAUD_Pause(x,y)

            #define PrintSTAUD_PauseSynchro(x,y)

            #define PrintSTAUD_Play(x);

            #define PrintSTAUD_Prepare(x,y)

            #define PrintSTAUD_Resume(x);

            #define PrintSTAUD_SetAttenuation(x,y)

            #define PrintSTAUD_SetChannelDelay(x,y)

            #define PrintSTAUD_SetDigitalOutput(x,y)

            #define PrintSTAUD_SetDynamicRangeControl(x,y)

            #define PrintSTAUD_SetEffect(a,b)

            #define PrintSTAUD_SetKaraokeOutput(a,b)

            #define PrintSTAUD_SetPrologic(c);

            #define PrintSTAUD_SetSpeaker(c,p)

            #define PrintSTAUD_SetSpeakerConfiguration(x,y,z)

            #define PrintSTAUD_SetStereoOutput(x,y);

            #define PrintSTAUD_SkipSynchro(x,y,z)

            #define PrintSTAUD_Start(x,z)

            #define PrintSTAUD_Stop(x,y,z)

            #define PrintSTAUD_PlayerStart(a)

            #define PrintSTAUD_PlayerStop(a)

            #define PrintSTAUD_GetInputBufferParams(x,y,z)

            #ifndef STAUD_REMOVE_CLKRV_SUPPORT
                #define PrintSTAUD_SetClockRecoverySource(x,y)
            #endif

            #define PrintAUD_IntHandlerInstall(InterruptNumber,InterruptLevel)

            #define PrintSTAUD_PPDcRemove(x,y,z)
        #endif

        #ifdef ST_OSLINUX

            #define PrintSTAUD_MapBufferToUserSpace(x)

            #define PrintSTAUD_UnmapBufferFromUserSpace(y)

        #endif

        #define PrintSTAUD_DPSetCallback(x,y,z,a)

        #define PrintSTAUD_MXUpdatePTSStatus(x,y,z,a)

        #define PrintSTAUD_MXUpdateMaster(x,y,z,a)

        #define PrintSTAUD_DRGetCompressionMode(x,y,z)

        #define PrintSTAUD_DRGetAudioCodingMode(x,y,z)

        #define PrintSTAUD_IPHandleEOF(a,b)

        #define PrintSTAUD_IPGetDataInterfaceParams(x,y,z)

        #define PrintSTAUD_GetBufferParam(x,y,z)

        #define PrintSTAUD_DRSetDTSNeoParams(x,y,z)

        #define PrintSTAUD_DRGetDTSNeoParams(x,y,z)

        #define PrintSTAUD_PPSetBTSCParams(x,y,z);

        #define PrintSTAUD_PPGetBTSCParams(x,y,z);

        #define PrintSTAUD_SetScenario(a,b)

    #endif

    #ifdef __cplusplus
        }
    #endif

#endif /* #ifndef __STAUDLX_H */





