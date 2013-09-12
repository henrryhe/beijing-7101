/// $Id: mixer_multicom_heap.h,v 1.6 2003/09/24 16:51:46 lassureg Exp $
/// @file     : ACC_Multicom/ACC_MIXER/mixer_multicom_heap.h
///
/// @brief    : MIXER Multicom transformer definition
///
/// @par OWNER: Gael Lassure
///
/// @author   : Gael Lassure
///
/// @par SCOPE: Internal transformer interface file
///
/// @date     : 2004-11-30
///
/// &copy; 2004 ST Microelectronics. All Rights Reserved.
///


#ifndef _MIXER_MULTICOM_HEAP_H_
#define _MIXER_MULTICOM_HEAP_H_

/* Driver Definitions */
#include "mme_interface.h"
#include "mixer_processortypes.h"
//#include "mixer_multicom_defines.h"

//#define ACC_MIXER_MAX_NB_INPUT    2
#define ACC_NB_PCMOUT_BUFFERS     1
#define SYS_NB_MAX_CHANNELS       10


typedef struct
{
	MME_UINT            NumberInputBuffers;
	MME_UINT            NumberOutputBuffers;
	MME_DataBuffer_t ** DataBuffers_p;

} MME_BufferPassingConvention_t;


typedef struct
{
	int               NbInput;
	enum eAccFsCode   OutputSamplingFreq;
	U32               MaxNbOutSamples;
	U32               NbOutSamples;
}
tMMEMixerConfig;

typedef struct
{
	int                  *  SamplesPtr[SYS_NB_MAX_CHANNELS];   // Pointers to each Channels
	int                     NbSamples;                         // Number of Sample Per Channel in the PCM buffer
	short                   NbChannels;                        // Number of Channels
	short                   Offset;                            // WS-based Address-Offset between samples of same channel
	enum eAccFsCode         SamplingFreq;                      // Sampling frequency of the PCM Samples
	enum eAccWordSizeCode   WordSize;                          // 32/16 bit word Left-aligned
	enum eAccAcMode         AudioMode;                         // Audio Configuration Mode.
	enum eAccBoolean        ChannelExist[SYS_NB_MAX_CHANNELS]; // Presence of channels

	int                     BufferSize;         // PcmBuffer Allocated Size in samples per channel
	int                     Flags;              // PTS validity bits / PTS[32] / Emphasis
	int                     PTS;                // PTS associated to the buffer (if any)
}
tPcmBuffer;


typedef struct
{
	// Internal parameters
	int                  * ProcessMem;
	int                    ProcessMemSize;
	//tScratchBuffer       * ScratchBuffer;

	// input parameters
	enum eAccBoolean       FirstTime;
	int                    NbPcmIn;
	tPcmBuffer           * PcmIn[ACC_MIXER_MAX_NB_INPUT];
	void                 * Config;               // Pointer on a PcmProcessing Configuration

	// Output parameters
	int                    NbPcmOut;
	tPcmBuffer           * PcmOut[ACC_NB_PCMOUT_BUFFERS];
	int                    NbNewOutSamples;
}
tPcmProcessingInterface;


typedef struct
{
	U32                       StructSize;

	/* System */
	U32                       OutputNbChannels;

	/* Mixer MacroTransformer variables */
	tMMEMixerConfig           Config;
	void                    * MixerConfig;               // Pointer on a PcmProcessing Configuration
	MME_MixerInputConfig_t    InputConfig[ACC_MIXER_MAX_NB_INPUT];

	/* Main and Pcm Post Processings */
	tPcmProcessingInterface   PcmInterface;

	//     I/O Buffers
	tPcmBuffer                PcmOut[ACC_NB_PCMOUT_BUFFERS];

	// Debug info
	int        FrameNumber;
	int        CommandNumber;

	enum eAccMainChannelIdx   OutputIdx;
	short                     MixerOutNChans;
}
MME_LxMixerMem_t;

#endif /* _MIXER_MULTICOM_HEAP_H_ */
