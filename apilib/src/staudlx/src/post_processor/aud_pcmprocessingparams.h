/// $Id: STm7100_AudioDecoder_Types.h,v 1.3 2003/11/26 17:03:07 lassureg Exp $
/// @file     : ACC_Multicom/ACC_Transformers/Audio_DecoderTypes.h
///
/// @brief    : Generic Audio Decoder types for Multicom
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


#ifndef _STM7100_AUD_PCMPROCESSINGPARAMS_H_
#define _STM7100_AUD_PCMPROCESSINGPARAMS_H_
#ifndef ST_5197
#include "mme.h"
#else
#include "mme_interface.h"
#endif 
#include "acc_mmedefines.h"
#include "pcmprocessing_decodertypes.h"
typedef struct
{
  U16                         StructSize;        //!< Size of this structure
  U8                          NbPcmProcessings;  //!< NbPcmProcessings on main[0..3] and aux[4..7]
  U8                          AuxSplit;          //!< Point of split between Main output and Aux output
  MME_BassMgtGlobalParams_t   BassMgt;
  MME_EqualizerGlobalParams_t Equalizer;
  MME_DCRemoveGlobalParams_t  DcRemove;
  MME_TempoGlobalParams_t     TempoControl;
  MME_DelayGlobalParams_t     Delay;
  
  MME_CMCGlobalParams_t       CMC;         //!< Common DMix Config : could be used by decoder or by generic downmix */
  /* 	MME_DeEmphGlobalParams_t    DeEmph;      //!< DeEmphasis Filtering */
  /* 	MME_BassMgtGlobalParams_t   BassMgt; */
  /* 	MME_DCRemoveGlobalParams_t  DcRemove; */
  MME_DMixGlobalParams_t		DMix;	//Down Mix */
  /* 	MME_PLIIGlobalParams_t		ProLogicII; */
  /* 	MME_TsxtGlobalParams_t		TruSurXT; */
} MME_STm7100LxPcmProcessingGlobalParams_t;


typedef struct
{
  U32                               StructSize;  //!< Size of this structure
  MME_PcmInputConfig_t              PcmConfig ;  //!< Private Config of Audio Decoder
  MME_STm7100LxPcmProcessingGlobalParams_t PcmParams ;  //!< PcmPostProcessings Params
} MME_STm7100PcmProcessorGlobalParams_t ;


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
  MME_STm7100PcmProcessorGlobalParams_t	GlobalParams;
  
} MME_STm7100LxPcmProcessingInitParams_t;


#endif /* _STM7100_AUD_PCMPROCESSINGPARAMS_H_ */
