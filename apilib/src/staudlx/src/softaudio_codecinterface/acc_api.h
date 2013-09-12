/*$Id: acc_api.h,v 1.3 2003/09/23 11:34:13 lassureg Exp $*/

#ifndef _API_H_
#define _API_H_

#include "acc_defines.h"

#define uchar unsigned char

typedef int    tSample;
typedef int    tCoeff;
typedef int    tCoeff32;
typedef short  tCoeff16;

typedef int    tPcmProcessingConfig;
typedef tCoeff tDownMixTable [SYS_NB_MAX_CHANNELS][SYS_NB_MAX_CHANNELS];

enum ePageState
{
    PAGE_INVALID,
    PAGE_VALID,
    PAGE_IN_USE,
    PAGE_RELEASED
};


typedef struct
{
	void            * Page_p;     /* Pointer to the Datas         */
	int               Size;       /* Length of the page in bytes  */
	int               BytesUsed;  /* Number of bytes already read */
	enum ePageState   Flags;      /* */
}
tScatterPage;

typedef struct
{
	/* Frame Base decoding*/
	int               FrameSize;
	uchar           * DataPtr;
	enum eFsCode      SamplingFreq;
	int               NbSamples;
	enum eAcMode      AudioMode;
	enum eCodecCode   StreamType;


	/* Stream Base decoding : when FrameSize = 0xFFFF*/
	int               NbScatterPagesAllocated; /* Number of ScatterPages allocated  */
	int               PageSize;                /* Max Page size */
	int               NbScatterPages; /* Number of ScatterPages available      */
	tScatterPage    * Pages;          /* Pointer to an array of ScatterPages   */
	int               StartOffset;    /* Offset when to read in the first page */

	int               BufferSize;

	int               PTSflag;            /* PTS validity bits                     */
	int               PTS;                /* PTS associated to the buffer (if any) */

}
tFrameBuffer;

typedef struct
{
	tSample *          SamplesPtr[SYS_NB_MAX_CHANNELS];   /* Pointers to each Channels																*/
	int                NbSamples;                         /* Number of Sample Per Channel in the PCM buffer           */
	short              NbChannels;                        /* Number of Channels                                       */
	short              Offset;                            /* WS-based Address-Offset between samples of same channel  */
	enum eFsCode       SamplingFreq;                      /* Sampling frequency of the PCM Samples                    */
	enum eWordSizeCode WordSize;                          /* 32/16 bit word Left-aligned                              */
	enum eAcMode       AudioMode;                         /* Audio Configuration Mode.                                */
	enum eBoolean      ChannelExist[SYS_NB_MAX_CHANNELS]; /* Presence of channels                                     */
                                                          /*                                                          */
	int                BufferSize;         /* PcmBuffer Allocated Size in samples per channel                         */
	int                PTSflag;            /* PTS validity bits                                                       */
	int                PTS;                /* PTS associated to the buffer (if any)                                   */
}
tPcmBuffer;


typedef struct
{
	enum eCodecCode  DecoderSelection;
	enum ePacketCode PacketSelection;
	enum eCodecCmd   SkipMuteCmd;
	enum eBoolean    CrcOff;
}
tCommonDecoderConfig;

typedef struct
{
	enum eAcMode     DownMix[MIX_MAIN_AND_AUX];
	enum eDualMode   DualMode[MIX_MAIN_AND_AUX];
	tCoeff16         CentreMixCoeff;
	tCoeff16         SurroundMixCoeff;
	enum eBoolean    PcmDownScaled;                     /* TRUE if signal has been downscaled before downmix */
	enum eBoolean    PreDefinedTable[MIX_MAIN_AND_AUX]; /* User Pre defined table */
	tDownMixTable *  DownMixTable[MIX_MAIN_AND_AUX];
}
tCommonMixConfig;

typedef struct
{
	tCommonDecoderConfig CDC;   /* Common Decoder Informations				*/
	tCommonMixConfig *   CMC;   /* Common Mixing  Configuration       */
	uchar      *         PDC;   /* Particular decoder informations    */
}
tDecoderConfig;

typedef struct
{
	enum eCodecCode EncoderSelection;
	uchar *  PEC;               /* Particular Encoding informations*/
}
tEncoderConfig;

typedef struct
{
	tCommonDecoderConfig CDC;   /* Common Decoder Informations			*/
	tCommonMixConfig *   CMC;   /* Common Mixing  Configuration     */
	uchar *  PDC;               /* Particular Coder informations    */
}
tTranscoderConfig;

typedef struct
{
	char * Mem_p;
	int    Size;
	int    Occupied;
}tScratchBuffer;

typedef struct
{
	/* internal parameters*/
	int            * EncoderMem;
	int              EncoderMemSize;
	tScratchBuffer * ScratchBuffer;

	/* input parameters	*/
	enum eBoolean    FirstTime;
	tEncoderConfig * Config;
	tPcmBuffer     * PcmIn;

	/* Output parameters */
	tFrameBuffer   * FrameOut;
	enum eErrorCode  EncoderStatus;
	int              NbEncodedSamples;

}
tEncoderInterface;

typedef struct
{
	/* Internal parameters */
	int            * DecoderMem;
	int              DecoderMemSize;
	tScratchBuffer * ScratchBuffer;

	/* input parameters*/
	enum eBoolean    FirstTime;
	tDecoderConfig * Config;
	tFrameBuffer   * FrameIn;
	tFrameBuffer   * FrameOut;

	/* Output parameters*/
	tPcmBuffer     * PcmOut;
	tPcmBuffer     * PcmOutAux;

	int              RemainingBlocks;
	enum eBoolean    OutputPcmEnable;
	enum eErrorCode  DecoderStatus;
	int              NbNewOutSamples;
	int              BitStreamInformations[SYS_NB_MAX_BITSTREAM_INFO];
}
tDecoderInterface;

typedef struct
{
	/* Internal parameters*/
	int                  * ProcessMem;
	int                    ProcessMemSize;
	tScratchBuffer       * ScratchBuffer;

	/* input parameters*/
	enum eBoolean          FirstTime;
	int                    NbPcmIn;
	tPcmBuffer           * PcmIn[SYS_NB_MAX_PCMIN];
	tPcmProcessingConfig * Config;               /* Pointer on a PcmProcessing Configuration*/

	/* Output parameters*/
	int                    NbPcmOut;
	tPcmBuffer           * PcmOut[SYS_NB_MAX_PCMOUT];
	int                    NbNewOutSamples;
}
tPcmProcessingInterface;


typedef struct
{
	/* Internal parameters*/
	int               * TranscoderMem;
	int                 TranscoderMemSize;
	tScratchBuffer    * ScratchBuffer;

	/* input parameters*/
	enum eBoolean       FirstTime;
	tTranscoderConfig * Config;
	tFrameBuffer      * FrameIn;

	/* Output parameters*/
	tFrameBuffer      * FrameOut;
	enum eErrorCode     TranscoderStatus;
	int                 NbEncodedSamples;

}
tTranscoderInterface;

#endif /* _API_H_ */
