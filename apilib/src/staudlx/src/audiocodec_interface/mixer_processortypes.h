/// $Id: Mixer_ProcessorTypes.h,v 1.3 2003/11/26 17:03:09 lassureg Exp $
/// @file     : ACC_Multicom/ACC_Transformers/Mixer_ProcessorTypes.h
///
/// @brief    : Mixer Processor specific types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author   : Gael Lassure
///
/// @par SCOPE:
///
/// @date     : 2004-11-30
///
/// &copy; 2004 ST Microelectronics. All Rights Reserved.
///


#ifndef _MIXER_PROCESSORTYPES_H_
#define _MIXER_PROCESSORTYPES_H_

#define DRV_MULTICOM_MIXER_VERSION       0x061020

#include "acc_mme.h"
#include "pcm_postprocessingtypes.h"
#include "acc_mmedefines.h"


// Mixer state
enum eMixerState
{
	MIXER_INPUT_NOT_RUNNING,
	MIXER_INPUT_RUNNING,
	MIXER_INPUT_FADING_OUT,
	MIXER_INPUT_MUTED,
	MIXER_INPUT_PAUSED,
	MIXER_INPUT_FADING_IN
};

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM    1

#define ACC_MIXER_MAX_NB_INPUT      4

#define ACC_MIXER_MAX_NB_OUTPUT     1

#define ACC_MIXER_MAX_INPUT_BUFFERS  (ACC_MIXER_MAX_NB_INPUT)

#define ACC_MIXER_MAX_OUTPUT_BUFFERS (ACC_MIXER_MAX_NB_OUTPUT)

#define ACC_MIXER_MAX_BUFFERS        (ACC_MIXER_MAX_INPUT_BUFFERS + ACC_MIXER_MAX_OUTPUT_BUFFERS)

#define ACC_MIXER_MAX_INPUT_PAGES_PER_STREAM  16 // max nb of pages we want to send per input stream for each transform

#define ACC_MIXER_MAX_INPUT_PAGES    (ACC_MIXER_MAX_NB_INPUT * ACC_MIXER_MAX_INPUT_PAGES_PER_STREAM)

#define ACC_MIXER_MAX_OUTPUT_PAGES_PER_STREAM 1 // max nb of pages we want to send per output stream for each transform

#define ACC_MIXER_MAX_OUTPUT_PAGES   (ACC_MIXER_MAX_NB_INPUT * ACC_MIXER_MAX_OUTPUT_PAGES_PER_STREAM)

#define  MAX_ACC_VOL    0xFFFF
#define  MIN_ACC_VOL    0x0

#define  MAX_CHANS_ST20 2

#define ACC_MIXER_BEEPTONE_MAX_NTONE 4
#define BEEPTONE_GET_FREQ(x) (((x & 0x3f) * 4) * (((((100 << 16) + (10 << 8) + 1) >> (8 * (x>>6))))&0xff))
#define BEEPTONE_SET_FREQ(x) ((x>=2560) ? (((x / 400) & 0x3F) + 0x80) : ((x>=256) ? (((x/40) & 0x3F) + 0x40) : (((x/4)&0x3F))))

//Mixer Configuration Type IDs :
enum eAccMixerId
{
	ACC_RENDERER_MIXER_ID,
	// ?? if any other mixing technology  ??

	// All postprocessing type
	ACC_RENDERER_MIXER_POSTPROCESSING_ID,

	// Generic Output Config
	ACC_RENDERER_MIXER_OUTPUT_CONFIG_ID,

	// Do not edit below this line
	ACC_RENDERER_LAST_MIXER_ID
};

enum eAccRendererProcId
{
	ACC_RENDERER_BASSMGT_ID = ( ACC_RENDERER_PROCESS_MT << 8 ), // BassMgt + Volume Control + Delay
	ACC_RENDERER_EQUALIZER_ID,
	ACC_RENDERER_TEMPO_ID,
	ACC_RENDERER_DCREMOVE_ID,
	ACC_RENDERER_DOLBYHEADPHONE_ID,
	ACC_RENDERER_STHEADPHONE_ID,
	ACC_RENDERER_CUSTOMHEADPHONE_ID,
	ACC_RENDERER_CUSTOMMIXERPOSTPROC_ID,
	ACC_RENDERER_DELAY_ID,
	ACC_RENDERER_SFC_ID,
	ACC_RENDERER_ENCODERPP_ID,
	ACC_RENDERER_FATPIPE_ID,
	ACC_RENDERER_CMC_ID,
	ACC_RENDERER_DMIX_ID,
	ACC_RENDERER_BTSC_ID,
	ACC_LAST_RENDERERPROCESS_ID
};

enum eAccMixerCapabilityFlags
{
	ACC_MIXER_PREPROCESSING,
	ACC_MIXER_PROCESSING,
	ACC_MIXER_POSTPROCESSING
};

//! Additional decoder capability structure
typedef struct
{
	U32                     StructSize;       //!< Size of this structure
	U32                     MixerCapabilityFlags;
	U32                     PcmProcessorCapabilityFlags[2];
} MME_LxMixerTransformerInfo_t;

enum eMixerInputType
{
	ACC_MIXER_LINEARISER,
	ACC_MIXER_BEEPTONE_GENERATOR,
	ACC_MIXER_PINKNOISE_GENERATOR,

	// do not edit further
	ACC_NB_MIXER_INPUT_TYPE,
	ACC_MIXER_INVALID_INPUT
};

typedef struct
{
	U8                      InputId;          //!< b[7..4] :: Input Type | b[3..0] :: Input Number
	U8                      NbChannels;       //!< Interleaving of the input pcm buffers
	U16                     Alpha;            //!< Mixing Coefficient for Each Input
	U16                     Mono2Stereo;      //!< [enum eAccBoolean] Mono 2 Stereo upmix of a mono input
	enum eAccWordSizeCode   WordSize;         //!< Input WordSize : ACC_WS32 / ACC_WS16
	enum eAccAcMode         AudioMode;        //!<  Channel Configuration
	enum eAccFsCode         SamplingFreq;     //!< Sampling Frequency
	enum eAccMainChannelIdx FirstOutputChan;  //!< To which output channel is mixer the 1st channel of this input stream
	enum eAccBoolean        AutoFade;
	U32                     Config;           //!< Special config (applicable to generators);
} MME_MixerInputConfig_t;

typedef struct
{
	enum eAccMixerId       Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	MME_MixerInputConfig_t Config[ACC_MIXER_MAX_NB_INPUT];
} MME_LxMixerInConfig_t;

typedef struct {
	enum eAccMixerId       Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	U32                    NbOutputSamplesPerTransform; //!< Number of Output samples per transform
} MME_LxMixerOutConfig_t;

typedef struct
{
	enum eAccMixerId            Id;            //!< Id of the PostProcessing structure.
	U32                         StructSize;    //!< Size of this structure

	MME_BassMgtGlobalParams_t   BassMgt;
	MME_EqualizerGlobalParams_t Equalizer;
	MME_TempoGlobalParams_t     TempoControl;
	MME_DCRemoveGlobalParams_t  DCRemove;
	MME_DelayGlobalParams_t     Delay;
	MME_EncoderPPGlobalParams_t Encoder;
	MME_SfcPPGlobalParams_t     Sfc;
	MME_CMCGlobalParams_t       CMC;
	MME_DMixGlobalParams_t      Dmix;
#ifdef ST_7200	
	MME_BTSCGlobalParams_t      Btsc;
#endif 

} MME_LxPcmPostProcessingGlobalParameters_t; //!< PcmPostProcessings Params

typedef struct
{
	U32                                        StructSize;   //!< Size of this structure
	MME_LxMixerInConfig_t                      InConfig;     //!< Specific Configuration for Mixer
	MME_LxMixerOutConfig_t                     OutConfig;    //!< output specific configuration information
	MME_LxPcmPostProcessingGlobalParameters_t  PcmParams;    //!< PcmPostProcessings Params
#ifdef ST_51XX
	U32                                        Volume[MAX_CHANS_ST20];
	U32                                        Swap[2];
#endif
} MME_LxMixerTransformerGlobalParams_t;

typedef struct
{
	U32                   StructSize; //!< Size of this structure

	//! System Init
	enum eAccProcessApply CacheFlush; //!< If ACC_DISABLED then the cached
                                      //!< data aren't sent back at the end of the command

	//! Mixer Init Params
	U32                   NbInput;
	U32                   MaxNbOutputSamplesPerTransform;

	U32                   OutputNbChannels;
	enum eAccFsCode       OutputSamplingFreq;

	//! Mixer specific global parameters
	MME_LxMixerTransformerGlobalParams_t	GlobalParams;

} MME_LxMixerTransformerInitParams_t;

enum eMixerCommand
{
	MIXER_STOP,
	MIXER_PLAY,
	MIXER_FADE_OUT,
	MIXER_MUTE,
	MIXER_PAUSE,
	MIXER_FADE_IN
};

typedef struct
{
	U16 Command;     //!< enum eMixerCommand : Play / Mute / Pause / Fade in - out ...
	U16 StartOffset; //!< start offset of the given input [when exiting of Pause / Underflow ]
	U32 PTSflags;
	U32 PTS;
} tMixerFrameParams;

//! This structure must be passed when sending the TRANSFORM command to the decoder
typedef struct
{
	tMixerFrameParams   InputParam[ACC_MIXER_MAX_NB_INPUT]; //!< Input Params attached to given input buffers
} MME_LxMixerTransformerFrameDecodeParams_t;

typedef struct
{
	enum eMixerState State;
	U32              BytesUsed;
	U32              NbInSplNextTransform;
} MME_MixerInputStatus_t;

typedef struct
{
	U32 StructSize;
	// System report
	U32                    ElapsedTime;     //<! elapsed time to do the transform in microseconds
	MME_MixerInputStatus_t InputStreamStatus[ACC_MIXER_MAX_NB_INPUT];
} MME_LxMixerTransformerSetGlobalStatusParams_t;

typedef struct
{
	U32                    StructSize;
	// System report

	// Mixed signal Properties
	enum eAccAcMode        MixerAudioMode;   //<! Channel Output configuration
	enum eAccFsCode        MixerSamplingFreq;//<! Sampling Frequency of Output PcmBuffer

	int                    NbOutSamples;     //<! Report the actual number of samples that have been output
	enum eAccAcMode        AudioMode;        //<! Channel Output configuration
	enum eAccFsCode        SamplingFreq;     //<! Sampling Frequency of Output PcmBuffer

	enum eAccBoolean       Emphasis;         //<! Buffer has emphasis
	U32                    ElapsedTime;      //<! elapsed time to do the transform in microseconds

	U32                    PTSflag;          //<! PTSflag[b0..1] = PTS - Validity / PTSflag[b16] =  PTS[b32]
	U32                    PTS;              //<! PTS[b0..b31]

	// Input consumption feedback
	MME_MixerInputStatus_t InputStreamStatus[ACC_MIXER_MAX_NB_INPUT];

} MME_LxMixerTransformerFrameDecodeStatusParams_t;


#endif /* _MIXER_PROCESSORTYPES_H_ */
