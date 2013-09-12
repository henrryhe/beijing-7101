/// $Id: DolbyDigital_DecoderTypes.h,v 1.5 2003/11/26 17:03:08 lassureg Exp $
/// @file     : MMEPlus/Interface/Transformers/MP2a_DecoderTypes.h
///
/// @brief    : MP2 Audio Decoder specific types for MME
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


#ifndef _DOLBY_DIGITAL_DECODERTYPES_H_
#define _DOLBY_DIGITAL_DECODERTYPES_H_

#define DRV_MULTICOM_DOLBY_DIGITAL_DECODER_VERSION 061004

#include "acc_mme.h"
#include "audio_decodertypes.h"
#include "pcmprocessing_decodertypes.h"
#include "acc_mmedefines.h"

// Number of frames to decode per MME transform operation
#define NUM_FRAMES_PER_TRANSFORM	1

enum eAccAc3Status
{
	ACC_AC3_OK,
	ACC_AC3_ERROR_BSI
	// ...
};

enum eAc3CapabilityFlags
{
	ACC_DOLBY_DIGITAL_Ex,
	ACC_DOLBY_DIGITAL_PLUS
};

enum eDDConfigIdx
{
	DD_CRC_ENABLE,
	DD_LFE_ENABLE,
	DD_COMPRESS_MODE,
	DD_HDR,
	DD_LDR,
	DD_HIGH_COST_VCR,
	DD_VCR_REQUESTED,
  
	DDP_FRAMEBASED_ENABLE,
	DDP_OUTPUT_SETTING, // Bit[0..1] :: UPSAMPLE_PCMOUT_ENABLE ; BIT[2] :: LITTLE_ENDIAN_DDout 

	/* Do not edit beyond this comment */
	DD_NB_CONFIG_ELEMENTS
};

enum eDDCompMode
{
	DD_CUSTOM_MODE_0,
	DD_CUSTOM_MODE_1,
	DD_LINE_OUT,
	DD_RF_MODE,
	
	/* Do not edit beyond this element */
	DD_LAST_COMP_MODE
};

typedef struct 
{
	enum eAccDecoderId      DecoderId;
	U32                     StructSize;
  
	U8                      Config[DD_NB_CONFIG_ELEMENTS];
	//config[0]:enum eAccBoolean  CrcOff;      //!< Disable the CRC Checking
	//config[1]:enum eAccBoolean  LfeEnable;   //!< Enable the Lfe Processing
	//config[2]:enum eDDCompMode  CompMode;    //!< Compression Mode (LineOut / Custom 0,1/ RF) 
	//config[3]:U8                HDR;         //!< Q8 HDR Coefficient : 1.0 <=> 0xFF
	//config[4]:U8                LDR;         //!< Q8 LDR Coefficient : 1.0 <=> 0xFF   
	//config[5]:enum eAccBoolean  HighCostVcr; //!< Dual Decoding Simulatneaous Multichannel + Stereo 
	//config[6]:enum eAccBoolean  VcrRequested;//!< If ACC_MME_TRUE then process VCR downmix internally
	tCoeff15                PcmScale;          //!< Q15 scaling coeff (applied in freq domain) 1.0 <=> 0x7FFF     
} MME_LxAc3Config_t;


typedef struct
{
	U32                    StructSize;    //!< Size of this structure

	MME_CMCGlobalParams_t  CMC;  //!< Common DMix Config : could be used internally by decoder or by generic downmix
	MME_TsxtGlobalParams_t TSXT; //!< TsXT Configuration
	MME_OmniGlobalParams_t OMNI; //!< Omni Configuration
	MME_StwdGlobalParams_t STWD; //!< WideSurround Configuration
	MME_DMixGlobalParams_t DMix; //!< Generic DMix : valid for main and vcr output

} MME_LxAc3PcmProcessingGlobalParams_t;


typedef struct
{
	U32                               StructSize;//!< Size of this structure
	MME_LxAc3Config_t                 Ac3Config; //!< Private Config of MP2 Decoder
	MME_LxPcmProcessingGlobalParams_t PcmParams; //!< PcmPostProcessings Params

} MME_LxAc3TransformerGlobalParams_t;

typedef struct 
{
	U16            DecoderStatus;
	U16            TranscoderStatus;
	U16            ConvSyncStatus;
	U16			   TranscoderFreq;
} tMMEDDPlusStatus;

#endif /* _DOLBY_DIGITAL_DECODERTYPES_H_ */
