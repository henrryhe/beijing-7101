/// $Id: PcmProcessing_DecoderTypes.h,v 1.9 2003/11/26 17:03:09 lassureg Exp $
/// @file     : ACC_Multicom/ACC_Transformers/PcmProcessing_DecoderTypes.h
///
/// @brief    : Decoder PostProcessing specific types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author   : Gael Lassure
///
/// @par SCOPE:
///
/// @date     : 2004-11-11
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///


#ifndef _PCMPROCESSING_DECODERTYPES_H_
#define _PCMPROCESSING_DECODERTYPES_H_

#ifdef _ACC_AMP_SYSTEM_
#define USE_NEW_PP
#endif

#include "acc_mmedefines.h"
#include "acc_mme.h"

#include "pcm_postprocessingtypes.h"

#define DRV_PCMPROCESSING_MULTICOM_VERSION 0x070718

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM    1
#define NUM_MAX_PAGES_PER_TRANSFORM 16

//! Additional decoder capability structure
typedef struct
{
	U32                 StructSize;        //!< Size of this structure
	U32                 DecoderCapabilityFlags;
	U32                 StreamDecoderCapabilityFlags;   //!< 1 bit per decoder 
	U32                 DecoderCapabilityExtFlags[2];
	U32                 PcmProcessorCapabilityFlags[2];
} MME_LxPcmProcessingInfo_t;


typedef struct
{
	U16                         StructSize;        //!< Size of this structure
	U8                          NbPcmProcessings;  //!< NbPcmProcessings on main[0..3] and aux[4..7]
	U8                          AuxSplit;          //!< Point of split between Main output and Aux output
	MME_CMCGlobalParams_t       CMC;         //!< Common DMix Config : could be used by decoder or by generic downmix
	MME_TsxtGlobalParams_t	    TsxtVcr;     //!< Trusurround XT Configuration for VCR output
	MME_DMixGlobalParams_t      DMix;        //!< Generic DMix : valid for main and vcr output
	MME_AC3ExGlobalParams_t     Ac3Ex;       //!< Ac3EX post - processing ( can be applied to any input whether AC3 or not)
	MME_DeEmphGlobalParams_t    DeEmph;      //!< DeEmphasis Filtering
	MME_PLIIGlobalParams_t      Plii;        //!< Dolby PrologicII
	MME_TsxtGlobalParams_t      TsxtMain;    //!< TruSurround XT Configuration for Main Output

#ifdef USE_NEW_PP
	MME_OmniGlobalParams_t      OmniVcr;     //!< ST OmniSurround for VCR output 
	MME_StwdGlobalParams_t      StWdVcr;     //!< ST WideSurround for VCR output
	MME_CSIIGlobalParams_t      Csii;        //!< SRS CircleSurround II
	MME_ChsynGlobalParams_t     ChSyn;       //!< ST Channel Synthesis 
	MME_OmniGlobalParams_t      OmniMain;    //!< ST OmniSurround for Main output 
	MME_StwdGlobalParams_t      StWdMain;    //!< ST WideSurround for Main output

	MME_BassMgtGlobalParams_t   BassMgt;
	MME_EqualizerGlobalParams_t Equalizer;
	MME_DCRemoveGlobalParams_t  DcRemove;
	MME_DelayGlobalParams_t     Delay;

	MME_SigAnalysisGlobalParams_t SigAnalysis;
	MME_EncoderPPGlobalParams_t      Encoder;
#endif

	MME_KokPSGlobalParams_t     PitchShift;  //!< Kok Pitch Shift  control
	MME_KokVCGlobalParams_t     VoiceCancel; //!< Kok Voice Cancel control
	MME_TempoGlobalParams_t     TempoCtrl;   //!< Tempo Control
	MME_es2pesGlobalParams_t    Es2Pes;    //ES2pes 
	MME_SfcPPGlobalParams_t     Sfc;
} MME_LxPcmProcessingGlobalParams_t;



typedef struct 
{
	U32 Id;
	U32 StructSize;
	U8  Config[1];
} MME_PcmProcessingStatusTemplate_t;

typedef struct
{
	U32                               BytesUsed;  // Amount of this structure already filled
	MME_PcmProcessingStatusTemplate_t PcmStatus;  // To be replaced with MME_WaterMStatus_t like structures
} MME_PcmProcessingFrameExtStatus_t;

typedef struct
{
	U32                 BytesUsed;  // Amount of this structure already filled
	MME_WaterMStatus_t  PcmStatus;  // To be replaced with MME_WaterMStatus_t like structures
} MME_WMFrameStatus_t;

#include "audio_decodertypes.h"

typedef struct
{
	U32                 StructSize;

	int                 PcmProcessStatus;
	int                 NbInSamples; 
	enum eAccAcMode     InputAudioMode;
	enum eAccFsCode     InputSamplingFreq;

	int                 NbOutSamples;     //<! Report the actual number of samples that have been output 
	enum eAccAcMode     AudioMode;        //<! Channel Output configuration 
	enum eAccFsCode     SamplingFreq;     //<! Sampling Frequency of Output PcmBuffer 
	enum eAccBoolean    Emphasis;         //<! Buffer has emphasis
	U32                 ElapsedTime;      //<! elapsed time to do the transform in microseconds
	U32                 PTSflag;          //<! PTSflag[b0..1] = PTS - Validity / PTSflag[b15] =  PTS[b32] 
	U32                 PTS;              //<! PTS[b0..b31]
	
	U16                 FrameStatus[ACC_MAX_DEC_FRAME_STATUS]; /* Decoder Specific Parameters */
} MME_PcmProcessingFrameStatus_t;





typedef struct
{
	enum eAccDecoderId      DecoderId;
	U32                     StructSize;

	U32                     NbChannels;       //!< Interleaving of the input pcm buffers
	enum eAccWordSizeCode   WordSize;         //!< Input WordSize : ACC_WS32 / ACC_WS16
	enum eAccAcMode         AudioMode;        //!< Channel Configuration
	enum eAccFsCode         SamplingFreq;     //!< Sampling Frequency
	U32                     Config;           //!< For Future use 
} MME_PcmInputConfig_t;



typedef struct
{
	U32                               StructSize;  //!< Size of this structure
	MME_PcmInputConfig_t              PcmConfig ;  //!< Private Config of Audio Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams ;  //!< PcmPostProcessings Params
} MME_PcmProcessorGlobalParams_t ;


typedef struct
{
	U32                   StructSize; //!< Size of this structure

	//! System Init 
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached data aren't sent back to DDR
	U32                   BlockWise;  //!< Perform the decoding blockwise instead of framewise 
	enum eFsRange         SfreqRange;

	U8                    NChans[ACC_MIX_MAIN_AND_AUX];
	U8                    ChanPos[ACC_MIX_MAIN_AND_AUX];

	//! Decoder specific parameters
	MME_PcmProcessorGlobalParams_t	GlobalParams;

} MME_LxPcmProcessingInitParams_t;

//! This structure must be passed when sending the TRANSFORM command to the decoder


typedef struct
{
	enum eAccBoolean    Restart;
	U16                 Cmd;              //!< (enum eAccCmdCode) Play/Skip/Mute/Pause
	U16                 Duration;         //!< Duration of cmd in sample
	                                      //!< if (Duration==0) then process whole input
} MME_PcmProcessingFrameParams_t;


#endif // _PCMPROCESSING_DECODERTYPES_H_

