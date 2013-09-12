/// $Id: DTS_DecoderTypes.h,v 1.2 2003/11/26 17:03:08 lassureg Exp $
/// @file     : MMEPlus/Interface/Transformers/Dts_DecoderTypes.h
///
/// @brief    : DTS Audio Decoder specific types for MME
///
/// @par OWNER: Gael Lassure
///
/// @author   : Gael Lassure
///
/// @par SCOPE:
///
/// @date     : 2003-07-04 
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///


#ifndef _DTS_DECODERTYPES_H_
#define _DTS_DECODERTYPES_H_

#define DRV_MME_DTS_DECODER_VERSION 0x060220 //0x051112

#include "acc_mme.h"
#include "pcmprocessing_decodertypes.h"
#include "acc_mmedefines.h"
#include "audio_decodertypes.h"


#define DTS_DEFAULT_NBSAMPLES_PER_TRANSFORM 2048

enum eDTSCapabilityFlags
{
	ACC_DTS_96K,
	ACC_DTS_ES,
	ACC_DTS_HD,
	ACC_DTS_LBR
};

#define DTS_GET_NBBLOCKS(n)        (n >> 8)
#define DTS_GET_NBSAMPLES(n)       (n << 8)

enum eDtsConfigIdx
{
	DTS_CRC_ENABLE,
	DTS_LFE_ENABLE,
	DTS_DRC_ENABLE,
	DTS_ES_ENABLE,
	DTS_96k_ENABLE,
	DTS_NBBLOCKS_PER_TRANSFORM,
	/* Do not edit below this comment */
	DTS_NB_CONFIG_ELEMENTS
};

enum eDtshdConfigIdx
{
	DTSHD_CRC_ENABLE,
	DTSHD_LFE_ENABLE,
	DTSHD_DRC_ENABLE,
	DTSHD_XCH_ENABLE,
	DTSHD_96K_ENABLE,
	DTSHD_NBBLOCKS_PER_TRANSFORM,
	DTSHD_XBR_ENABLE,
	DTSHD_XLL_ENABLE,
	DTSHD_INTERPOLATE_2X_ENABLE,
	DTSHD_MIX_LFE,
	DTSHD_LBR_ENABLE,
	DTSHD_24BITENABLE,
	DTSHD_OUTSR_CHNGFACTOR,
	/* Do not edit below this comment */
	DTSHD_NB_CONFIG_ELEMENTS
};

typedef struct 
{
	enum eAccDecoderId      DecoderId;
	U32                     StructSize;

	U8                      Config[DTSHD_NB_CONFIG_ELEMENTS];

// #ifdef _INSTALLED_dts_
	//config[0]: enum eAccBoolean CrcEnable; //!< Enable the CRC Checking
	//config[1]: enum eAccBoolean LfeEnable; //!< Enable the Lfe Processing
	//config[2]: enum eAccBoolean DrcEnable; //!< Enable the Dynamic Range Compression processing
	//config[3]: enum eAccBoolean EsEnable;  //!< Enable the 6.1 Decoding
	//config[4]: enum eAccBoolean 96kEnable; //!< Enable the 96kHz decoding

// #elif defined  _INSTALLED_dtshd_
	//config[0]: enum eAccBoolean CrcEnable; //!< Enable the CRC Checking
	//config[1]: enum eAccBoolean BlockWise; //!< Enable the BlockWise Processing
	//config[2]: enum eAccBoolean XchEnable; //!< Enable the 6.1 Decoding
	//config[3]: enum eAccBoolean XBrEnable; //!< Enable the XBr Decoding
	//config[4]: enum eAccBoolean XLLEnable; //!< Enable the Lossless decoding
	//config[5]: enum eAccBoolean X96Enable; //!< Enable the 96k decoding
	//config[6]: enum eAccBoolean Interpolate2xEnable; //!< Enable the 2x Interpolation.
	//config[7]: enum eAccBoolean MixLfe; //!< Enable the Lossless decoding
	//config[8]: enum eAccBoolean SubStreamEnable; //!< Enable Substream decoding.
	//config[9]: enum eAccBoolean LBrEnable; //!< Enable the LBr decoding
	//config[10]: enum eAccBoolean HDMIDecode; //!< Enable the HDMI decoding
// #endif
	U8		FirstByteEncSamples;
	U32		Last4ByteEncSamples;
	U16		DelayLossLess;
	U16     CoreSize;
} MME_LxDtsConfig_t;

typedef struct
{
	U32                                StructSize; //!< Size of this structure
	MME_LxDtsConfig_t                  DtsConfig;  //!< Private Config of MP2 Decoder
	MME_LxPcmProcessingGlobalParams_t  PcmParams;  //!< PcmPostProcessings Params
} MME_LxDtsTransformerGlobalParams_t;


#endif /* _DTS_DECODERTYPES_H_ */
