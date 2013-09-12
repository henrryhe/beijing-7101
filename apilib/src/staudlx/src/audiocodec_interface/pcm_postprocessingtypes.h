/// $Id: Pcm_PostProcessingTypes.h,v 1.3 2003/11/26 17:03:07 lassureg Exp $
/// @file     : ACC_Multicom/ACC_Transformers/Pcm_PostProcessingTypes.h
///
/// @brief    : Pcm Post Processing types for Multicom
///
/// @par OWNER: Gael Lassure
///
/// @author   : Gael Lassure
///
/// @par SCOPE:
///
/// @date     : 2004-11-1
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///

#ifndef _PCM_POSTPROCESSINGTYPES_H_
#define _PCM_POSTPROCESSINGTYPES_H_

#include "acc_mmedefines.h"

#ifndef _DEFINED_UNSIGNED_INT_TYPES_
#define _DEFINED_UNSIGNED_INT_TYPES_

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int   U32;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef unsigned short U16;
#endif

#ifndef DEFINED_U8
#define DEFINED_U8
typedef unsigned char  U8;
#endif
#ifndef DEFINED_I32
#define DEFINED_I32
typedef int            I32;
#endif

#ifndef DEFINED_I16
#define DEFINED_I16
typedef short          I16;
#endif

#ifndef DEFINED_I8
#define DEFINED_I8
typedef char           I8;
#endif
#endif //_DEFINED_UNSIGNED_INT_TYPES_

enum eAccPcmProcId
{
	ACC_PCM_CMC_ID,
	ACC_PCM_DeEMPH_ID,
	ACC_PCM_AC3Ex_ID,
	ACC_PCM_CUSTOM_MAIN_ID,
	ACC_PCM_PLII_ID,
	ACC_PCM_CSII_ID,
	ACC_PCM_CHSYN_ID,
	ACC_PCM_CUSTOM_22N_ID,
	ACC_PCM_TSXT_ID,
	ACC_PCM_OMNI_ID,
	ACC_PCM_STWD_ID,
	ACC_PCM_CUSTOM_N22_ID,
	ACC_PCM_DB_HEADPHONE_ID,
	ACC_PCM_ST_HEADPHONE_ID,
	ACC_PCM_CUSTOM_HEADPHONE_ID,
	ACC_PCM_DMIX_ID,
	ACC_PCM_KOKPS_ID,     /* Pitch Shift   */
	ACC_PCM_KOKVC_ID,     /* Voice Cancel  */
	ACC_PCM_TEMPO_ID,
	ACC_PCM_KOKMUSIC_ID,
	ACC_PCM_BASSMGT_ID,
	ACC_PCM_EQUALIZER_ID,
	ACC_PCM_CUSTOM_ID,
	ACC_PCM_DCREMOVE_ID,
	ACC_PCM_DELAY_ID,
	ACC_PCM_SFC_ID,
	ACC_PCM_SIGANALYSIS_ID,
	ACC_PCM_ES2PES_ENCODER_ID,

	ACC_PCM_ENCODER_ID,
	ACC_PCM_FATPIPE_ID,
	ACC_PCMPRO_30_ID,
	ACC_PCMPRO_31_ID,
	ACC_PCM_NEO_ID,
	ACC_PCMPRO_BTSC_ID,
	/* Do not edit beyond this comment */
	ACC_LAST_PCMPROCESS_ID
};

#define ACC_PCMPROC_MT(x)    (((U32) x >> 8) & 0xF)                        // Extract the MT type from Id.
#define ACC_PCMPROC_SUBID(x) (ACC_PCMPROC_MT(x) - ACC_PCMPROCESS_MAIN_MT)  // Extract the OutChain from MT type
#define ACC_PCMPROC_ID(x)    (x & 0x3F)                                    // Extract the base Id


enum eTSXTCOnfigIdx
{
	TSXT_ENABLE,
	TSXT_HEADPHONE,
	TSXT_TBSIZE,
	TSXT_MODE,

	/* Do not edit beyond this comment */
	TSXT_NB_CONFIG_ELEMENTS
};

enum eTSXTLfResponse
{
    TSXTLfResponse_40Hz,
    TSXTLfResponse_60Hz,
    TSXTLfResponse_100Hz,
    TSXTLfResponse_150Hz,
    TSXTLfResponse_200Hz,
    TSXTLfResponse_250Hz,
    TSXTLfResponse_300Hz,
    TSXTLfResponse_400Hz
};

enum eTSXTMode
{
	TSXT_Passivematrix,
	TSXT_1_0,
	TSXT_2_0,
	TSXT_3_0,
	TSXT_2_1,
	TSXT_3_1,
	TSXT_2_2,
	TSXT_3_2,
	TSXT_3_3,
	TSXT_3_2_BSDigital,
	TSXT_PL2_Music,
	TSXT_CSII,
	TSXT_AutoMode
};

typedef struct
{
	U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	enum eAccProcessApply  Apply;
	U8                     Config[TSXT_NB_CONFIG_ELEMENTS];
	//!< [0] : enum eBoolean   TsxtEnable [bit0] / FocusEnable[bit1] / TBEnable[bit2] / DownMix Enable[bit3];
	//!< [1] : enum eBoolean   TsxtHeadphone;
	//!< [2] : enum eTBSpkSize TBSize;
	//!< [3] : enum eTsxtMode  TsxtMode;

	tCoeff15            FocusElevation;        //!< Q15 Coefficient for focus
	tCoeff15            FocusTweeterElevation; //!< Q15 Coefficient for focus
	tCoeff15            TBLevel;               //!< Q15 TruBass level
	tCoeff15            InputGain;             //!< Q15 InputGain ( applied to all channels )

} MME_TsxtGlobalParams_t;

#define MASK8(x) (0xFF - BIT(x))

#define TSXT_SET_TRUSURROUND(config, enb) config[TSXT_ENABLE] = (config[TSXT_ENABLE] & MASK8(0)) | (enb << 0)
#define TSXT_SET_FOCUS(config, enb)       config[TSXT_ENABLE] = (config[TSXT_ENABLE] & MASK8(1)) | (enb << 1)
#define TSXT_SET_TRUBASS(config, enb)     config[TSXT_ENABLE] = (config[TSXT_ENABLE] & MASK8(2)) | (enb << 2)
#define TSXT_SET_DMIX(config, enb)        config[TSXT_ENABLE] = (config[TSXT_ENABLE] & MASK8(3)) | (enb << 3)

#define TSXT_GET_TRUSURROUND(config) (config[TSXT_ENABLE] >> 0) & 1
#define TSXT_GET_FOCUS(config)       (config[TSXT_ENABLE] >> 1) & 1
#define TSXT_GET_TRUBASS(config)     (config[TSXT_ENABLE] >> 2) & 1
#define TSXT_GET_DMIX(config)        (config[TSXT_ENABLE] >> 3) & 1

#define TSXT_CHECK_ENABLE(config)         (config[TSXT_ENABLE] & 0x3F)

enum eDMixConfigIdx
{
	DMIX_USER_DEFINED,
	DMIX_STEREO_UPMIX,
	DMIX_MONO_UPMIX,
	DMIX_MEAN_SURROUND,
	DMIX_SECOND_STEREO,
	DMIX_MIX_LFE,
	DMIX_NORMALIZE,
	DMIX_NORM_IDX,
	DMIX_DIALOG_ENHANCE,

	/* Do not edit beyond this comment */
	DMIX_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	enum eAccProcessApply  Apply;
	U8                     Config[DMIX_NB_CONFIG_ELEMENTS];
	//!< [0] : enum eBoolean UserDefinedTab[MIX_MAIN_AND_AUX]; / Tables attached : Main[bit 0]/Aux[bit 1]
	//!< [1] : enum eBoolean StereoUpMix;     / Upmix L/R to Centre channel  (default : no upmix)
	//!< [2] : enum eBoolean MonoUpMix;       / Upmix Mono to L/R  with 0 dB (default :  -3dB)
	//!< [3] : enum eBoolean MixMeanSurround; / Downmix (Ls + Rs)/2 to L/R   (default : Ls+Rs)
	//!< [4] : enum eBoolean MixSecondStereo; / Output second stereo on L/R
	//!< [5] : enum eBoolean MixLfe;          / Mix Lfe in L/R in case of stereo downmix
	//!< [6] : enum eBoolean Normalize;       / Enable Normalization
	//!< [7] : u8            NormIdx;         / Index to the predifined set of normalisation coefficient
	//!< [8] : enum eBoolean DialogEnhance;   / Boost Centre Channel downmix wrt L/R/Surrounds

	//!< The 2 Following tables should be sent only if the UserDefinedTab is set to ACC_TRUE
	tCoeff15               MainMixTable[8][8]; //!< Q15 Downmix Coefficient for main Output
	tCoeff15               AuxMixTable[2][8];  //!< Q15 Downmix Coefficient for aux  Output

} MME_DMixGlobalParams_t;

enum eCMConfigIdx
{
	CMC_OUTMODE_MAIN,
	CMC_OUTMODE_AUX,
	CMC_DUAL_MODE,
	CMC_PCM_DOWN_SCALED,

	/* Do not edit beyond this comment */
	CMC_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure

	U8                  Config[CMC_NB_CONFIG_ELEMENTS];
	//!< [0] : enum eAcMode   DownMix[MIX_MAIN]
	//!< [1] : enum eAcMode   DownMix[MIX_AUX]
	//!< [2] : enum eDualMode DualMode
	//!< [3] : enum eBoolean  PcmDownScaled   / Pcm Buffer is already downscaled to prevent overflow

	tCoeff15            CenterMixCoeff;
	tCoeff15            SurroundMixCoeff;

} MME_CMCGlobalParams_t;

enum eAccDeemphMode
{
	ACC_DEEMPH_50_15_us,
	ACC_DEEMPH_CCITT_J_17,

	// do not edit below this item
	ACC_LAST_DEEMPH_MODE
};
typedef struct
{
	U32                    Id;             //!< ID of this processing
	U32                    StructSize;     //!< Size of this structure

	enum eAccProcessApply  Apply;          //!< Deemphasis Enable switch
	enum eAccDeemphMode    Mode;           //!< Deemphasis Mode selector
} MME_DeEmphGlobalParams_t;

enum eEncoderPPType
{
	ACC_ENCODERPP_UNDEFINED,
	ACC_ENCODERPP_AACE,
	ACC_ENCODERPP_DDCE,
	ACC_ENCODERPP_DTSE,

	// do not edit below this item
	ACC_LAST_ENCODERPP_TYPE
};

enum eEncoderPPAACSubType
{
	ACC_ENCODERPP_AACE_ADTS,
	ACC_ENCODERPP_AACE_LOAS,

	// do not edit below this item
	ACC_LAST_ENCODERPP_AAC_SUBTYPE
};

typedef struct
{
	U32                    Id;             //!< ID of this processing
	U32                    StructSize;     //!< Size of this structure

	enum eAccProcessApply  Apply;          //!< Encoder Enable switch
	enum eEncoderPPType    Type;           //!< Encoder Type
	U32                    BitRate;        //!< BitRate
} MME_EncoderPPGlobalParams_t;

#define ACC_ENCODERPP_GET_TYPE(cfg)    (cfg->Type & 0xF)
#define ACC_ENCODERPP_GET_SUBTYPE(cfg) ((cfg->Type>>4) & 0xF)
#define ACC_ENCODERPP_SET_TYPE(cfg, type)     cfg->Type = (enum eEncoderPPType) (((U32)cfg->Type &0xF0) + (type & 0xF))
#define ACC_ENCODERPP_SET_SUBTYPE(cfg, stype) cfg->Type = (enum eEncoderPPType) (((U32)cfg->Type &0x0F) + ((stype & 0xF) <<4))


typedef struct
{
	U32                    Id;             //!< ID of this processing
	U32                    StructSize;     //!< Size of this structure

	enum eAccProcessApply  Apply;          //!< Fatpipe Enable switch
	U32                    Config[16];     //!<
} MME_FatpipeGlobalParams_t;


enum ePLIIDecMode
{
    PLII_PROLOGIC = 0,
    PLII_VIRTUAL,
    PLII_MUSIC,
    PLII_MOVIE,
    PLII_MATRIX,
    PLII_RESERVED_1,
    PLII_RESERVED_2,
    PLII_CUSTOM
};

enum ePLIIConfigIdx
{
	PLII_DEC_MODE,
	PLII_AUTO_BALANCE,
	PLII_SPEAKER_DIMENSION,
	PLII_SURROUND_FILTER,
	PLII_PANORAMA,
	PLII_RS_POLARITY,
	PLII_CENTRE_SPREAD,
	PLII_OUT_MODE,
	/* Do not edit beyond this comment */
	PLII_NB_CONFIG_ELEMENTS
};
typedef struct
{
	U32                    Id;             //!< ID of this processing
	U32                    StructSize;     //!< Size of this structure

	enum eAccProcessApply  Apply;          //!< Enable/Disable AC3Ex processing

	I8                     Config[PLII_NB_CONFIG_ELEMENTS];
	//!< [0] : enum eProliiDecMode      DecMode;       / PLII Decoder Mode / Disable ==> 0xFF
	//!< [1] : enum eProcessApply       AutoBalance;   / Enable AutoBalance mode
	//!< [2] : enum eProliiDimensionSet Dimension;     / Cntr/Srd vs LR speaker dimension
	//!< [3] : enum eProcessApply       SurFilt;       /
	//!< [4] : enum eProcessApply       Panorama;      /
	//!< [5] : enum eProcessApply       RsPolarity;    /
	//!< [6] : enum eProliiCentreSpread CentreSpread;  / from 0 to 7
	//!< [7] : enum eAcMode             OutMode;       / Output Mode :: MODE30 / 22 / 32

	tCoeff15               PcmScale;         //!< Q15 Input Gain.

} MME_PLIIGlobalParams_t;

typedef struct
{
	U32                    Id;            //!< ID of this processing
	U32                    StructSize;    //!< Size of this structure

	enum eAccProcessApply  Apply;         //!< Enable/Disable AC3Ex processing

} MME_AC3ExGlobalParams_t;

enum eKokPSConfigIdx
{
	KOKPS_KEY,
	KOKPS_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	enum eAccProcessApply  Apply;
	U8                     Config[KOKPS_NB_CONFIG_ELEMENTS];
} MME_KokPSGlobalParams_t;

enum eKokVCConfigIdx
{
	KOKVC_CANCEL_STRENGTH,
	KOKVC_FIRST_TIME,
	KOKVC_VOICE_CANCEL_SWITCH,
	KOKVC_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	enum eAccProcessApply  Apply;
	U8                     Config[KOKVC_NB_CONFIG_ELEMENTS];

} MME_KokVCGlobalParams_t;

typedef struct
{
	U32                    Id;               //!< ID of this processing
	U32                    StructSize;       //!< Size of this structure
	enum eAccProcessApply  Apply;

	int                    Ratio;            //!< Speed Change ratio in %

} MME_TempoGlobalParams_t;


enum eCSIICOnfigIdx
{
	CSII_ENABLE,
	CSII_TBSIZE,
	CSII_OUTMODE,
	CSII_MODE,

	/* Do not edit beyond this comment */
	CSII_NB_CONFIG_ELEMENTS
};

enum eCSIIEnableBitField
{
	CSII_EN_PHANTOM_BIT,
	CSII_EN_CNTR_FB_BIT,
	CSII_EN_IS525_BIT,
	CSII_EN_CNTR_REAR_BIT,
	CSII_EN_CNTR_REAR_FB_BIT,
	CSII_EN_TBSUB_BIT,
	CSII_EN_TBFRONT_BIT,
	CSII_EN_FOCUS_BIT
};

typedef struct
{
	U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	enum eAccProcessApply  Apply;
	U8                     Config[CSII_NB_CONFIG_ELEMENTS];

	//!< [0] bit 0 : Phantom
	//!< [0] bit 1 : CenterFB
	//!< [0] bit 2 : IS525Mode
	//!< [0] bit 3 : CenterRear
	//!< [0] bit 4 : RCenterFB
	//!< [0] bit 5 : TBSub
	//!< [0] bit 6 : TBFront
	//!< [0] bit 7 : FocusEnable
	//!< [2] : enum eTBSpkSize TBSize;
	//!< [3] : enum eCSiiOutputMode  OutMode;
	//!< [4] : enum eCSiiMode  Mode;

	//enum eBoolean   Phantom;	 / true for phantom center image
	//enum eBoolean   CenterFB;	 / true for full bandwidth center channel
	//enum eBoolean   IS525Mode; / true for 525 mode
	//enum eBoolean   CenterRear;
	//enum eBoolean   RCenterFB;
	//enum eBoolean   TBSub;
	//enum eBoolean   TBFront;
	//enum eBoolean   FocusEnable;

	//enum eTBSpkSize TBSize;
	//enum eCSiiOutputMode  OutMode;
	//enum eCSiiMode   Mode;

	tCoeff15         InputGain;
	tCoeff15         TBLevel;
	tCoeff15         FocusElevation;

} MME_CSIIGlobalParams_t;




enum eOMNICOnfigIdx
{
	OMNI_ENABLE,
	OMNI_INPUT_MODE,
	OMNI_SURROUND_MODE,
	OMNI_DIALOG_MODE,
	OMNI_LFE_MODE,
	OMNI_DIALOG_LEVEL,

	/* Do not edit beyond this comment */
	OMNI_NB_CONFIG_ELEMENTS
};

typedef struct
{
    U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure
	enum eAccProcessApply  Apply;
	U8                     Config[OMNI_NB_CONFIG_ELEMENTS];
} MME_OmniGlobalParams_t;


enum eSTWDCOnfigIdx
{
	STWD_ENABLE,
	STWD_MODE,
	STWD_BASS_FREQ,
	STWD_MEDM_FREQ,
	STWD_HIGH_FREQ,

	/* Do not edit beyond this comment */
	STWD_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32                    Id;                //!< ID of this processing
	U32                    StructSize;        //!< Size of this structure

	enum eAccProcessApply  Apply;

	U8                     Config[STWD_NB_CONFIG_ELEMENTS];
	tCoeff15               StwdSurroundGain;  //!< Surround Gain

} MME_StwdGlobalParams_t;



enum eCHSYNConfigIdx
{
	CHSYN_ENABLE,
	CHSYN_MODE,
	CHSYN_OUT_ACMODE,
	CHSYN_LFE_ENABLE,

	/* Do not edit beyond this comment */
	CHSYN_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32              Id;             //!< ID of this processing
	U32                       StructSize;     //!< Size of this structure
	enum eAccProcessApply  Apply;
	U8                        Config[CHSYN_NB_CONFIG_ELEMENTS];

} MME_ChsynGlobalParams_t;


enum eAccBassMgtType
{
	BASSMGT_VOLUME_CTRL,
	BASSMGT_DOLBY_1,
	BASSMGT_DOLBY_2,
	BASSMGT_DOLBY_3,
	BASSMGT_SIMPLIFIED,
	BASSMGT_DOLBY_CERT,
	BASSMGT_Philips,
	BASSMGT_CUSTOMER1,
	BASSMGT_CUSTOMER2,
	BASSMGT_CUSTOMER3
};

enum eBassMgtConfigIdx
{
	BASSMGT_TYPE,
	BASSMGT_LFE_OUT,
	BASSMGT_BOOST_OUT,
	BASSMGT_PROLOGIC_IN,
	BASSMGT_OUT_WS,
	BASSMGT_LIMITER,
	BASSMGT_INPUT_ROUTE,
	BASSMGT_OUTPUT_ROUTE,
	/* Do not edit beyond this comment */
	BASSMGT_NB_CONFIG_ELEMENTS
};

enum eVolumeConfigIdx
{
	VOLUME_LEFT,
	VOLUME_RGHT,
	VOLUME_CNTR,
	VOLUME_LFE,
	VOLUME_LSUR,
	VOLUME_RSUR,
	VOLUME_CSURL,
	VOLUME_CSURR,
	VOLUME_AUX_LEFT,
	VOLUME_AUX_RIGHT,
	VOLUME_AUX2_LEFT,
	VOLUME_AUX2_RIGHT,
	VOLUME_NB_CONFIG_ELEMENTS
};

enum eDelayConfigIdx
{
	DELAY_LEFT,
	DELAY_RGHT,
	DELAY_CNTR,
	DELAY_LFE,
	DELAY_LSUR,
	DELAY_RSUR,
	DELAY_CSURL,
	DELAY_CSURR,

	/* Do not edit beyond this comment */
	DELAY_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32              Id;   //!< ID of this processing
	U32              StructSize;
	enum eAccProcessApply  Apply;
	U8               Config[BASSMGT_NB_CONFIG_ELEMENTS];
	//!< [0] : enum eBassMgtType Type;      / Select the type of BassMgt
	//!< [1] : enum eAccBoolean  LfeOut;    / Enable LFE output
	//!< [2] : enum eAccBoolean  BoostOut;  / Digital boost to compensate input attenuation
	//!< [3] : enum eAccBoolean  PrologicIn;/ Disable Surround Filters
	//!< [4] : int               WordSize;  / 32/24/20/18/16 bit output - rounding
	//!< [5] : enum eBassMgtLimiterIdx Limiter; / Parameter of Philips bassmgt
	U8               Volume[VOLUME_NB_CONFIG_ELEMENTS];
	enum eAccBoolean DelayUpdate;
	U8               Delay[DELAY_NB_CONFIG_ELEMENTS];
} MME_BassMgtGlobalParams_t;

enum eEqualizerMgtConfigIdx
{
	EQ_BAND_0,
	EQ_BAND_1,
	EQ_BAND_2,
	EQ_BAND_3,
	EQ_BAND_4,
	EQ_BAND_5,
	EQ_BAND_6,
	EQ_BAND_7,

	/* Do not edit beyond this comment */
	EQ_NB_CONFIG_ELEMENTS
};
typedef struct
{
	U32              Id;               //!< ID of this processing
	U32              StructSize;
	enum eAccProcessApply  Apply;
	enum eAccBoolean Enable;
	U8               Config[EQ_NB_CONFIG_ELEMENTS];
	//!< [0 .. 7] / Gain in given frequency band in dB
} MME_EqualizerGlobalParams_t;


typedef struct
{
	U32                   Id;               //!< ID of this processing
	U32                   StructSize;
	enum eAccProcessApply Apply;
} MME_DCRemoveGlobalParams_t;

typedef struct
{
	U32                   Id;               //!< ID of this processing
	U32                   StructSize;
	enum eAccProcessApply Apply;
	enum eAccFsCode       FsOut;
} MME_SfcPPGlobalParams_t;


typedef struct
{
	U32                   Id;               //!< ID of this processing
	U32                   StructSize;
	enum eAccProcessApply Apply;
	enum eAccBoolean      DelayUpdate;
	U8                    Delay[DELAY_NB_CONFIG_ELEMENTS];
	//!< [0..7] / Delay for given channel in samples
} MME_DelayGlobalParams_t;

typedef struct
{
	U32                   Id;               //!< ID of this processing
	U32                   StructSize;
	enum eAccProcessApply Apply;
	tCoeff15              InputGain;
	tCoeff15              TXGain;
	enum eAccBoolean      DualSignal;

} MME_BTSCGlobalParams_t;

enum eACCWatermarkStatus
{
	ACC_WATERMARK_COPY_FREELY,
	ACC_WATERMARK_COPY_ONCE,
	ACC_WATERMARK_NO_COPY,
	ACC_WATERMARK_NOT_DETECTED,
	ACC_WATERMARK_DETECTION_IN_PROGRESS,

	// Do not edit below this line
	ACC_WATERMARK_LAST_STATUS
};

enum eACCSDMIStatus
{
	ACC_SDMI_TRIGGER_ON,
	ACC_SDMI_TRIGGER_OFF,
	ACC_SDMI_PROCESSING,

	// Do not edit below this line
	ACC_SDMI_LAST_STATUS
};

typedef struct
{
	U32              Id;
	U32              StructSize;

 	enum eAccBoolean UpdateStatus;

	U8               Config[13];

	enum eACCWatermarkStatus WatermarkStatus;
	enum eACCSDMIStatus      SDMIStatus;

} MME_WaterMStatus_t;


enum eSigAnalysisConfigIdx
{
	SIGANALYSIS_WATERMARK_ENABLE,
	SIGANALYSIS_VUMETER_ENABLE,
	SIGANALYSIS_FREQANALYSER_ENABLE,

	// do not edit below this line
	SIGANALYSIS_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32                   Id;
	U32                   StructSize;

	enum eAccProcessApply Apply;

	U8                    Config[SIGANALYSIS_NB_CONFIG_ELEMENTS];

} MME_SigAnalysisGlobalParams_t;

typedef struct
{
	U32                   Id;               //!< ID of this processing
	U32                   StructSize;
	enum eAccProcessApply Apply;
} MME_es2pesGlobalParams_t;

enum eNEOConfigIdx
{
	NEO_IO_STARTIDX,
	NEO_CENTRE_GAIN,
	NEO_SETUP_MODE,
	NEO_AUXMODE_FLAG,
	NEO_OUTPUT_CHAN,
	/* Do not edit beyond this comment */
	NEO_NB_CONFIG_ELEMENTS
};

typedef struct
{
	U32                    Id;             //!< ID of this processing
	U32                    StructSize;     //!< Size of this structure

	enum eAccProcessApply  Apply;          //!< Enable/Disable Neo processing

	U8                     Config[NEO_NB_CONFIG_ELEMENTS];
	//!< [0] : int8                bitwidth;   I/P Bitwidth (16/24)                                  
	//!< [1] : int8                cgain;      Center gain 0x0 to 0xFF representing range 0.0 to 1.0 
	//!< [2] : enum eNeoSetupMode  mode;       1 - CINEMA , 2 - MUSIC , 3 - WIDE                     
	//!< [3] : int                 auxmode;    0 - Back , 1 - Side  FUTURE VERSIONS ONLY             
	//!< [4] : int                 channels;   Desired O/P Channels 3,4,5,6,7                        
} MME_NEOGlobalParams_t;
#endif /* _PCM_POSTPROCESSINGTYPES_H_ */

