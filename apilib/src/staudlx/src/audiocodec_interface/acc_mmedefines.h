/// $Id: acc_mmedefines.h,v 1.6 2003/11/26 17:03:09 lassureg Exp $
/// @file     : ACC_Multicom/ACC_Transformers/acc_mmedefines.h
///
/// @brief    : Type Definitions shared between Host and MME
///
/// @par OWNER: Gael Lassure
///
/// @author   : Gael Lassure
///
/// @par SCOPE:
///
/// @date     : 2004-12-15
///
/// &copy; 2004 ST Microelectronics. All Rights Reserved.
///

#ifndef _ACC_MME_DEFINES_H_
#define _ACC_MME_DEFINES_H_

//#include "mme.h"

#define AUDIO_DECODER_NB_PCMCAPABILITY_FIELD 2

// Add missing types from old MMEplus definition

typedef enum
{
	MME_TRANSFORMER_TYPE_JPEG_DECODER = 1,
	MME_TRANSFORMER_TYPE_MPEG_VIDEO_DECODER,
	MME_TRANSFORMER_TYPE_WMV_VIDEO_DECODER,
	MME_TRANSFORMER_TYPE_REAL_VIDEO_DECODER,
	MME_TRANSFORMER_TYPE_MPEG_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_AC3_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_WMA_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_REAL_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_MPEG_VIDEO_ENCODER,
	MME_TRANSFORMER_TYPE_AC3_AUDIO_ENCODER,
	MME_TRANSFORMER_TYPE_MP1L2_AUDIO_ENCODER,
	MME_TRANSFORMER_TYPE_MP3_AUDIO_ENCODER,
	MME_TRANSFORMER_TYPE_LPCM_AUDIO_RESAMPLER,
	MME_TRANSFORMER_TYPE_SUBPICTURE_DECODER,
	MME_TRANSFORMER_TYPE_DV_DECODER,
	MME_TRANSFORMER_TYPE_MP2_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_MP3_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_MLP_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_DTS_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_PCM_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_LPCM_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_AUDIO_KARAOKE_VOICE_LINE,
	MME_TRANSFORMER_TYPE_AUDIO_MIXER,
	MME_TRANSFORMER_TYPE_AUDIO_DECODER,
	MME_TRANSFORMER_TYPE_TEST_TRANSFORMER,     /* Placeholder for trying out new or "test" transformers */
	MME_TRANSFORMER_TYPE_DV_AUDIO_TRANSCODER,
	MME_TRANSFORMER_TYPE_HVF_FORMAT_CONVERTER,
	MME_TRANSFORMER_TYPE_DIVX_VIDEO_DECODER,
	MME_TRANSFORMER_TYPE_DV_NTSC_FORMAT_CONVERTER,
	MME_TRANSFORMER_TYPE_AUDIO_ENCODER,
	MME_TRANSFORMER_TYPE_AUDIO_PARSER,
	/* TBC... */
	/* Add new IDs only at the end (before MME_TRANSFORMER_TYPE_MAX) */
	/* or you will break compatibility with previously flashed code! */
	/* The recommended way is to not add them yourself, but request  */
	/* them from the owner of this file                              */
	MME_TRANSFORMER_TYPE_MAX
} MME_TransformerType_t;

#define MME_TRANSFORMER_TYPE_AUDIO_DECODER_POST_PROCESS MME_TRANSFORMER_TYPE_PCM_AUDIO_DECODER

typedef enum
{
	MME_TRANSFORMER_SUB_TYPE_AUTO_DETECT,
	MME_TRANSFORMER_SUB_TYPE_MPEG_I,
	MME_TRANSFORMER_SUB_TYPE_MPEG_II,
	MME_TRANSFORMER_SUB_TYPE_MPEG_4,
	MME_TRANSFORMER_SUB_TYPE_WMV1,
	MME_TRANSFORMER_SUB_TYPE_WMV2,
	MME_TRANSFORMER_SUB_TYPE_WMV3,
	/* TBC... */
	MME_TRANSFORMER_SUB_TYPE_MAX
} MME_TransformerSubType_t;


typedef short tCoeff15; /* Q15 coefficients */
typedef long  tCoeff31; /* Q31 coefficients */

enum eAccSetConfig
{
	ACC_SET_CONFIG_INIT,
	ACC_SET_CONFIG_UPDATE_GLOBALS,
	ACC_SET_CONFIG_END
};

enum eAccGlobalParams
{
	ACC_GP_ID,
	ACC_GP_SIZE,
	ACC_GP_CONFIG
};

enum eAccMacroTransformerType
{
	ACC_DECODER_MT,
	ACC_DEC_POSTPROCESS_MT,
	ACC_RENDERER_PROCESS_MT,
	ACC_PCMPROCESS_MAIN_MT,
	ACC_PCMPROCESS_AUX_MT,
	ACC_PCMPROCESS_DIGOUT_MT,
	ACC_KOK_VOICELINE_MT,
	ACC_SIGNAL_PROCESSING_MT,
	ACC_AUDIOGENERATOR_MT,
	ACC_ENCODER_MT,
	ACC_AC3E_MT,
	ACC_MP3E_MT,
	ACC_MP2E_MT,
	ACC_PARSER_MT,
	// do not edit below this line
	ACC_NB_MACROTRANSFORMER_TYPES
};
#define PCMPROCESS_SET_ID(id, sub_chain) (((ACC_PCMPROCESS_MAIN_MT+sub_chain)<<8)+(id&0xFF))

enum eAccMainChannelIdx
{
	ACC_MAIN_LEFT,
	ACC_MAIN_RGHT,
	ACC_MAIN_CNTR,
	ACC_MAIN_LFE,
	ACC_MAIN_LSUR,
	ACC_MAIN_RSUR,
	ACC_MAIN_CSURL,
	ACC_MAIN_CSURR,
	ACC_AUX_LEFT,
	ACC_AUX_RGHT

};

enum eAccEmphasis
{
	ACC_NO_EMPHASIS,
	ACC_EMPHASIS_50_15_US,
	ACC_EMPHASIS_RESERVED,
	ACC_EMPHASIS_CCITT,
	ACC_EMPHASIS_PROCESSED_DISABLED = 4,
	ACC_EMPHASIS_PROCESSED_50_15_US,
	ACC_EMPHASIS_PROCESSED_RESERVED,
	ACC_EMPHASIS_PROCESSED_CCITT
};

#define ACC_EMPHASIS_DISABLED ACC_NO_EMPHASIS

enum eAccKokVLProcId
{
	ACC_ECHO_ID = ( ACC_KOK_VOICELINE_MT << 8 ),
	ACC_REVERB_ID,
	ACC_CHORUS_ID,
	ACC_HARMONY_ID,
	ACC_KOKVL_PROCESS1_ID = ( ACC_KOK_VOICELINE_MT << 8 ),

	/* Do not edit beyond this comment */
	ACC_LAST_KOKVLPROCESS_ID
};

enum eAccSignalProcessId
{
	ACC_WATERMARKING_ID = ( ACC_SIGNAL_PROCESSING_MT << 8 ),
	ACC_VUMETER_ID,
	ACC_FREQ_ANALYZER_ID,
	ACC_SCORING_ID,
	ACC_MIX_METADATA_ID,

	/* Do not edit beyond this comment */
	ACC_LAST_SIGNALPROCESSING_ID
};

enum eAccFsCode
{
	/* Range : 2^0  */
	ACC_FS48k,
	ACC_FS44k,
	ACC_FS32k,
	ACC_FS_reserved_3,
	/* Range : 2^1  */
	ACC_FS96k,
	ACC_FS88k,
	ACC_FS64k,
	ACC_FS_reserved_7,
	/* Range : 2^2  */
	ACC_FS192k,
	ACC_FS176k,
	ACC_FS128k,
	ACC_FS_reserved_11,
	/* Range : 2^3 */
	ACC_FS384k,
	ACC_FS352k,
	ACC_FS256k,
	ACC_FS_reserved_15,
	/* Range : 2^-2 */
	ACC_FS12k,
	ACC_FS11k,
	ACC_FS8k,
	ACC_FS_reserved_19,
	/* Range : 2^-1 */
	ACC_FS24k,
	ACC_FS22k,
	ACC_FS16k,
	ACC_FS_reserved_23,

	ACC_FS_reserved,  // Undefined
	ACC_FS_ID=0xFF  // Used by Mixer : if FS_ID then OutSFreq = InSFreq
};

#define BYTES_PER_32BIT_WORD	(4)
#define BYTES_PER_16BIT_WORD	(2)

enum eAccWordSizeCode
{
	ACC_WS32,
	ACC_WS16,
	ACC_WS8
};

enum eAccAcMode
{
	ACC_MODE20t,           /*  0 */
	ACC_MODE10,            /*  1 */
	ACC_MODE20,            /*  2 */
	ACC_MODE30,            /*  3 */
	ACC_MODE21,            /*  4 */
	ACC_MODE31,            /*  5 */
	ACC_MODE22,            /*  6 */
	ACC_MODE32,            /*  7 */
	ACC_MODE23,            /*  8 */
	ACC_MODE33,            /*  9 */
	ACC_MODE24,            /*  A */
	ACC_MODE34,            /*  B */
	ACC_MODE42,            /*  C */
	ACC_MODE44,            /*  D */
	ACC_MODE52,            /*  E */
	ACC_MODE53,            /*  F */

	ACC_MODEk10_V1V2OFF,   /* 10  */
	ACC_MODEk10_V1ON,      /* 11 */
	ACC_MODEk10_V2ON,      /* 12 */
	ACC_MODEk10_V1V2ON,    /* 13 */

	ACC_MODE_undefined_14, /* 14 */
	ACC_MODE_undefined_15, /* 15 */
	ACC_MODE_undefined_16, /* 16 */
	ACC_MODE_undefined_17, /* 17 */
	ACC_MODE_undefined_18, /* 18 */
	ACC_MODE_undefined_19, /* 19 */

	ACC_MODEk_AWARE,       /* 1A */ /* Default 2/0 */
	ACC_MODEk_AWARE10,     /* 1B */
	ACC_MODEk_AWARE20,     /* 1C */
	ACC_MODEk_AWARE30,     /* 1D */

	ACC_MODE_undefined_1E, /* 1E */
	ACC_MODE_undefined_1F, /* 1F */

	ACC_MODEk20_V1V2OFF,   /* 20 */
	ACC_MODEk20_V1ON,      /* 21 */
	ACC_MODEk20_V2ON,      /* 22 */
	ACC_MODEk20_V1V2ON,    /* 23 */
	ACC_MODEk20_V1Left,    /* 24 */
	ACC_MODEk20_V2Right,   /* 25 */

	ACC_MODE_undefined_26,
	ACC_MODE_undefined_27,
	ACC_MODE_undefined_28,
	ACC_MODE_undefined_29,
	ACC_MODE_undefined_2A,
	ACC_MODE_undefined_2B,
	ACC_MODE_undefined_2C,
	ACC_MODE_undefined_2D,
	ACC_MODE_undefined_2E,
	ACC_MODE_undefined_2F,

	ACC_MODEk30_V1V2OFF,   /* 30 */
	ACC_MODEk30_V1ON,      /* 31 */
	ACC_MODEk30_V2ON,      /* 32 */
	ACC_MODEk30_V1V2ON,    /* 33 */
	ACC_MODEk30_V1Left,    /* 34 */
	ACC_MODEk30_V2Right,   /* 35 */

	ACC_MODE_undefined_36,
	ACC_MODE_undefined_37,
	ACC_MODE_undefined_38,
	ACC_MODE_undefined_39,
	ACC_MODE_undefined_3A,
	ACC_MODE_undefined_3B,
	ACC_MODE_undefined_3C,
	ACC_MODE_undefined_3D,
	ACC_MODE_undefined_3E,
	ACC_MODE_undefined_3F,

	ACC_MODE20LPCMA = 0x40,/* 40 */
	ACC_MODE11p20   = 0x50,/* 50 */
    ACC_MODE10p20,         /* 51 */
    ACC_MODE20p20,         /* 52 */
    ACC_MODE30p20,         /* 53 */

	ACC_MODE_1p1    = 0x60,/* 60 */
	
	ACC_MODE32_T00  = 0x70,/* 70 */
	ACC_MODE32_T10,        /* 71 */
	ACC_MODE32_T01,        /* 72 */
	ACC_MODE32_T20,        /* 73 */
	ACC_MODE20t_LFE = 0x80,/* 80 */
	ACC_MODE10_LFE,        /* 81 */
	ACC_MODE20_LFE,        /* 82 */
	ACC_MODE30_LFE,        /* 83 */
	ACC_MODE21_LFE,        /* 84 */
	ACC_MODE31_LFE,        /* 85 */
	ACC_MODE22_LFE,        /* 86 */
	ACC_MODE32_LFE,        /* 87 */
	ACC_MODE23_LFE,        /* 88 */
	ACC_MODE33_LFE,        /* 89 */
	ACC_MODE24_LFE,        /* 8A */
	ACC_MODE34_LFE,        /* 8B */
	ACC_MODE11p20_LFE,     /* 8C */
	ACC_MODE10p20_LFE,     /* 8D */
	ACC_MODE20p20_LFE,     /* 8E */
	ACC_MODE30p20_LFE,     /* 8F */
	ACC_MODE_ALL = 0xA0,
	ACC_MODE_ALL1,
	ACC_MODE_ALL2,
	ACC_MODE_ALL3,
	ACC_MODE_ALL4,
	ACC_MODE_ALL5,
	ACC_MODE_ALL6,
	ACC_MODE_ALL7,
	ACC_MODE_ALL8,
	ACC_MODE_ALL9,
	ACC_MODE_ALL10,
	ACC_MODE_ID  = 0xFF    /* FF */
};

#define ACC_MODE_LFE 0x0080
#define ACC_MODE_AUX 0x8000


/* dual modes definitions */
enum eAccDualMode
{
	ACC_DUAL_LR,
	ACC_DUAL_LEFT_MONO,
	ACC_DUAL_RIGHT_MONO,
	ACC_DUAL_MIX_LR_MONO,
	ACC_DUAL_AUTO
};

enum eAccAttenuation
{
	ACC_UNITY   = 0x7FFF, /* Q15(1.0)         */
	ACC_M3DB    = 0x5A82, /* Q15(0.707106781) */
	ACC_M6DB    = 0x4000, /* Q15(0.5)         */
	ACC_M9DB    = 0x2D41, /* Q15(0.35)        */
	ACC_M4_5DB  = 0x4C1C, /* Q15(0.594603557) */
	ACC_M1_5DB  = 0x68F6, /* Q15(0.82)        */
	ACC_M4_75DB = 0x4A3D  /* Q15(0.58)        */
};

enum eAccBoolean
{
	ACC_MME_FALSE,
	ACC_MME_TRUE
};

enum eAccProcessApply
{
	ACC_MME_DISABLED,
	ACC_MME_ENABLED,
	ACC_MME_AUTO,

	// Do not edit below this line
	ACC_MME_END_OF_PROCESS_APPLY
};

enum eDecoderCapabilityFlags
{
	ACC_PCM,
	ACC_AC3,
	ACC_MP2a,
	ACC_MP3,
	ACC_DTS,
	ACC_MLP,
	ACC_LPCM, /* LPCM DVD */
	ACC_SDDS,
	ACC_WMA9,
	ACC_OGG,
	ACC_MP4a_AAC,
	ACC_REAL_AUDIO,
	ACC_HDCD,
	ACC_AMRWB,
	ACC_WMAPROLSL,
	ACC_DDPLUS,
	ACC_GENERATOR
};

enum ePcmProcessingCapability
{
	ACC_DMIX,
	ACC_DeEMPH,
	ACC_TSXT,
	ACC_PLII,
	ACC_CSII,
	ACC_ST_OMNISURROUND,
	ACC_KOKVC, /* Voice Cancel  */
	ACC_KOKPS, /* Pitch Shift   */
	ACC_TEMPO_CONTROL,
	ACC_KOKVL, /* Voice Line : Echo/Reverb/Chorus */
	ACC_SCORING,
	ACC_MIXING,
	ACC_BASS_MGT,
	ACC_VOLUME_CONTROL,
	ACC_DELAY,
	ACC_DCREMOVE,
	ACC_EQUALIZER,
	ACC_VUMETER,
	ACC_FREQ_ANALYSER,
	ACC_WATERMARKING,
	ACC_AC3Ex, /* _20_ */
	ACC_ST_WIDESURROUND,
	ACC_ST_CHANNEL_SYNTHESIS,
	ACC_DOLBY_HEADPHONE,
	ACC_ST_HEADPHONE,
	ACC_CUSTOM_VCR,
	ACC_CUSTOM_MAIN,
	ACC_NEO,
	ACC_CUSTOM_N22,
	ACC_CUSTOM_HEADPHONE,
	ACC_CUSTOM_KOKMUSIC,
	ACC_ES2PES,
	ACC_ENCODERPP,
	ACC_AACE_PCMPRO,
	ACC_DDCE_PCMPRO,
	ACC_DTSE_PCMPRO,
	ACC_FATPIPE_PCMPRO,
	ACC_SFC,
	ACC_BTSC, 
	ACC_CUSTOM_DECPCMPRO
};


#define ACC_SIGANALYSIS ACC_WATERMARKING

enum eAccMixOutput
{
	ACC_MIX_MAIN,
	ACC_MIX_AUX,
	ACC_MIX_MAIN_AND_AUX,
	ACC_MIX_DIGITALOUT = ACC_MIX_MAIN_AND_AUX
};

#define SIGNED_FSRANGE(fsr)  ((fsr >= ACC_FSRANGE_12k) ? (fsr - ACC_FSRANGE_UNDEFINED):fsr)

enum eFsRange
{
	ACC_FSRANGE_48k,
	ACC_FSRANGE_96k,
	ACC_FSRANGE_192k,
	ACC_FSRANGE_384k,
	ACC_FSRANGE_12k,
	ACC_FSRANGE_24k,
	ACC_FSRANGE_UNDEFINED,
	// Do not edit beyond
	ACC_FSRANGE_LAST_ELEMENT
};

#define AUDIO_STREAMING_NUM_BUFFERS_PER_SENDBUFFER  1
#define AUDIO_MAX_BUFFER_PARAMS                     2

typedef struct
{
    unsigned int StructSize;                            //!< Size of the BufferParams array.
    unsigned int BufferParams[AUDIO_MAX_BUFFER_PARAMS];          //!< Array of Params : Transformer Specific Parameters
} MME_StreamingBufferParams_t;


enum eAccPtsDts
{
    ACC_NO_PTS_DTS,
    ACC_PTS_INTERPOLATED, // ACC_PTS_DTS_FLAG_FORBIDDEN,
    ACC_PTS_PRESENT,
    ACC_PTS_DTS_PRESENT
};


#define ACC_PTSflag_EXTRACT_VAL(x) (x & 3)
#define ACC_isPTS_PRESENT(x)       (ACC_PTSflag_EXTRACT_VAL(x) != ACC_NO_PTS_DTS)
#define ACC_PTSflag_EXTRACT_B32(x) ((x >> 16) & 1)
#define ACC_PTSflag_EXTRACT(x)     (x & 0x10003)
#define ACC_PTSflag_MASKOUT(x)     (x & 0xFFFEFFFC)

#define ACC_EMPHASIS_EXTRACT(x)    ((x >> 8) & 3)
#define ACC_EMPHASIS_MASKOUT(x)    ( x & 0xFFFFFCFF)
#define ACC_LAYER_EXTRACT(x)       ((x >>10) & 3)
#define ACC_LAYER_MASKOUT(x)       ( x & 0xFFFFF3FF)

#define ACC_EMPHASIS_SET(Flag, Emphasis) Flag = ((ACC_EMPHASIS_MASKOUT(Flag)) | (Emphasis << 8))

#define ACC_LAYER_SET(Flag, Layer) Flag = ((ACC_LAYER_MASKOUT(Flag)) | (Layer << 10))

#define ACC_PTSflag_SET(Flag, val, bit32) Flag = ((ACC_PTSflag_MASKOUT(Flag)) | (val&3) | ((bit32)<<16))

#endif /* _ACC_MME_DEFINES_H_ */
