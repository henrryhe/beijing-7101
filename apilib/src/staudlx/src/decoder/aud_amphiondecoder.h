/********************************************************************************
 *   Filename   	: aud_amphiondecoder.h										*
 *   Start       	: 15.06.2007                                                *
 *   By          	: Rahul Bansal											    *
 *   Contact     	: rahul.bansal@st.com										*
 *   Description  	: The file contains amphion based Audio Decoder APIs and    *
                      structures that will be used in decoder module            *
 *				                    											*
 ********************************************************************************
 */

#ifndef __AMPHIONAUDDEC_H__
#define __AMPHIONAUDDEC_H__


/* {{{ Includes */
#ifndef ST_OSLINUX
    #include "sttbx.h"
#endif
#include "stlite.h"
#include "stddefs.h"

#include "staudlx.h"
#include "blockmanager.h"
#include "aud_utils.h"
#include "aud_defines.h"
#include "stos.h"
#include "staud.h"
#ifdef MP3_SUPPORT
    #include "mp3.h"//mp3
    #include "acf_st20_audiocodec.h"//mp3
#endif
/* }}} */

/* {{{ Defines */

/* Decoder task stack size */
#define	AMPHION_AUDIO_DECODER_TASK_STACK_SIZE	(1024*2)

/* Decoder task priority */
#ifndef	AMPHION_AUDIO_DECODER_TASK_PRIORITY
    #define	AMPHION_AUDIO_DECODER_TASK_PRIORITY		6
#endif

#define BYTES_PER_CHANNEL 2
/* Maximum queue size for decoder */
#define	DECODER_QUEUE_SIZE			8
/* Maximum back buffers required for decoding*/
#define DECODER_MAX_BACKBUFFER_COUNT 2

#define DECODER_INODE_LIST_COUNT (DECODER_MAX_BACKBUFFER_COUNT + 1 + 1)
#define DECODER_ONODE_LIST_COUNT (DECODER_MAX_BACKBUFFER_COUNT + 1)

#define DEBUG_DECODING_TIME 0

/* Decoder Handle */
typedef	U32	STAud_DecHandle_t;
/* }}} */

/* {{{ Enumerations */

/* Decoder State Machine */
typedef enum
{
	DECODER_STATE_IDLE = 0,
	DECODER_STATE_START,
	DECODER_STATE_PLAYING,
	DECODER_STATE_STOP,
	DECODER_STATE_STOPPED,
	DECODER_STATE_TERMINATE
}AudioDecoderStates_t;

/*  States for processing date during playing state */
typedef enum
{

    DECODER_PROCESSING_STATE_IDLE = 0,
    DECODER_PROCESSING_STATE_GET_INPUT_DATA,
    DECODER_PROCESSING_STATE_GET_OUTPUT_BUFFER,
    DECODER_PROCESSING_STATE_UPDATE_QUEUES,
    DECODER_PROCESSING_STATE_GET_AMPHION,
    DECODER_PROCESSING_STATE_DECODE_FRAME,
    DECODER_PROCESSING_STATE_RELEASE_AMPHION,
    DECODER_PROCESSING_STATE_BYPASS

}AudioDecoderProcessingStates_t;

/*  States for processing date during playing state */
typedef enum
{
	DECODER_SINGLE = 0,
    DECODER_DUAL,
    DECODER_BYPASS,
    DECODER_CODEC
}AudioDecoderMode_t;

/* }}} */

/* {{{ Global Variables */

/* Decoder Input buffer list */
typedef struct DecoderInputBufferParams_s
{
    U32 INodeListCount;
	/* Structure to Hold received Memory Block */
	MemoryBlock_t	*InputBlockFromProducer;		/*  Compressed input Buffer */
}DecoderInputBufferParam_t;

typedef struct DecoderOutputBufferParams_s
{
    U32 ONodeListCount;
	/* Structure to Hold received Memory Block */
	MemoryBlock_t	*OutputBlockFromProducer;		/*  Deocded PCM Buffer */
}DecoderOutputBufferParam_t;

/* Decoder Init Structure.This structure should be passed along with Decoder Init call */
typedef struct STAud_DecInitParams_s
{
	/* Stream Content Types info(MPEG/AC3 etc)*/
	STAUD_StreamParams_t	    *StreamParams_p;
	/* memory partitions */
	STAVMEM_PartitionHandle_t	AVMEMPartition;
    ST_Partition_t          	*CPUPartition;
	ST_Partition_t				*DriverPartition;
	/* Buffer manager Handle */
	STMEMBLK_Handle_t		    InBufHandle;
	STMEMBLK_Handle_t		    OutBufHandle;
   	U8				            NumChannels;
   	/* Semaphore for Amphion access*/
   	semaphore_t                 *AmphionAccess;
   	STFDMA_ChannelId_t          AmphionInChannel;
   	STFDMA_ChannelId_t          AmphionOutChannel;
} STAud_DecInitParams_t;


/*	Decoder level PCM Processing parameter structure.
	It will have contain state of decoder's PCM processing state
	For example, its MUTE or UNMUTE etc
*/
typedef struct STAud_DecPCMParams_s
{
	/* Decoder Mute*/
	BOOL			Mute;
}STAud_DecPCMParams_t;


/* PCM post processing structues.
They will hold the temporary values if command is sent before audio is started */
typedef struct DynamicRange_s
{
	BOOL 					Apply;
	STAUD_DynamicRange_t	DyRange;
}DynamicRange_t;

typedef struct CompressionMode_s
{
	BOOL 					Apply;
	STAUD_CompressionMode_t	CompressionMode;
}CompressionMode_t;

typedef struct StereoMode_s
{
	BOOL 					Apply;
	STAUD_StereoMode_t	    		StereoMode;
}StereoMode_t;

typedef struct DownMix_s
{
	BOOL 					Apply;
	STAUD_DownmixParams_t	Downmix;
}DownMix_t;



typedef struct ADControl_s
{
    U32         Fade;
    U32         Pan;
    U32         CurPan;
    U32         TargetPan;
    int         PanDiff;
    U32         FrameNumber;
    U32         ApplyFrameFactor; /*Number of frames after which pan wil be applied*/
    int         RampFactor; /*rampdown or rampup factor*/
    BOOL        PanErrorFlag;
    U32         NumFramesPerSecond;
}ADControl_t;

/* Deocder Control Blocks list Structure - It will have all parms needed for one Instance of Decoder*/
typedef struct DecoderControlBlock_s
{
	/* Device Name */
	ST_DeviceName_t		            DeviceName;

	/* Device Handle */
	STAud_DecHandle_t		        Handle;

	/* Current Decoder State */
	AudioDecoderStates_t	        CurDecoderState;
    AudioDecoderProcessingStates_t  DecInState;
	/* Next decoder state. Will be given by the command to decoder. On
	receiving Next state decoder will change its current state to next state */
	AudioDecoderStates_t	        NextDecoderState;

	/* Decoder task will wait on this semaphore */
	semaphore_t		                *DecoderTaskSem_p;

	/* FDMA callbacks will wait on this semaphore */
	semaphore_t		                *FDMACallbackSem_p;

	/* Control Block Mutex */
	STOS_Mutex_t		            *LockControlBlockMutex_p;

	/* Command Completed Semaphore - For Synchronizing(block) Decoder State change commands */
	semaphore_t		                *CmdCompletedSem_p;
	/* Decoder processing states */
	AudioDecoderProcessingStates_t	DecProcesssingState;

	/* Decoder queues - for handling decoder activities */
	DecoderInputBufferParam_t	    DecoderInputQueue[DECODER_QUEUE_SIZE];

	DecoderOutputBufferParam_t	    DecoderOutputQueue[DECODER_QUEUE_SIZE];

	/* AVMem block handle to store I/P Data Buffer strucrures*/
	STAVMEM_BlockHandle_t	        BufferInHandle;
	/* AVMem block handle to store Buffer handle for MME Data buffers of output buffer 0 and 1*/
	STAVMEM_BlockHandle_t 	        BufferOutHandle;

	U32                             BackBufferCount;
    STFDMA_GenericNode_t            *INode[DECODER_INODE_LIST_COUNT];
    STFDMA_GenericNode_t            *ONode[DECODER_ONODE_LIST_COUNT];

    AudioDecoderMode_t              DecoderMode;

	/* Decoder queue management Variables */
	U32		DecoderInputQueueTail, DecoderInputQueueFront;	/* Incoming compressed buffer will be added here */
	U32		DecoderOutputQueueTail, DecoderOutputQueueFront;/* Next output buffer will be added here*/
	U32     DecoderOutputQueueNext, DecoderInputQueueNext;	/* Offset in input and output queue from which next decoding will be done*/

   	/* Semaphore for Amphion access*/
   	semaphore_t                 *AmphionAccess;
   	STFDMA_ChannelId_t          AmphionInChannel;
   	STFDMA_ChannelId_t          AmphionOutChannel;

	/* Decoder Task */
	task_t *		DecoderTask;
    #if defined (ST_OS20)
        U8			    *DecoderTaskstack;
        ST_Partition_t	*CPUPartition;
        tdesc_t         DecoderTaskDesc;
    #endif
	/* memory partitions */
	STAVMEM_PartitionHandle_t	AVMEMPartition;

	/* Stream Content Types (MPEG/AC3 etc)*/
	STAUD_StreamParams_t	    StreamParams;


	/* Decoder init parameters */
	STAud_DecInitParams_t	    InitParams;

	/* Buffer manager Handle */
	STMEMBLK_Handle_t		    InMemoryBlockManagerHandle;
	STMEMBLK_Handle_t		    OutMemoryBlockManagerHandle;


	ConsumerParams_t 		    consumerParam_s;
	ProducerParams_t  		    ProducerParam_s;

	/* Deocder PCM processing params */
	STAud_DecPCMParams_t	    PcmProcess;


	/* Default Decoded buffer size. It will be used to send proper size to next unit when
	    decoder return error. This will make next units work normally
	*/
	U32						    DefaultDecodedSize;
	/* Default Compressed frame size for transcoded output. It will be used to send proper size to next unit when
	    decoder return error. This will make next units work normally
	*/
	U32						    DefaultCompressedFrameSize;



	/*Adding AD Control Structure to store AD related values*/
	ADControl_t                 ADControl;

	STAUD_Attenuation_t         Attenuation_p;

    STAUD_DRCapability_t        DecCapability;

	BOOL	                    Decode_Zero_Size_Flag;
	BOOL                        Reuse_transcoded_buffer;

	/* Transfer Completed*/
	BOOL    InputTransferCompleted,OutputTransferCompleted;

    STFDMA_TransferGenericParams_t InputTransferParams,OutputTransferParams;
    STFDMA_TransferId_t            InputTransferId, OutputTransferId;

	/* Sampling frequency received from decoder */
	U32		            SamplingFrequency;
	U32                 OutputEmphasis;
	MPEGAudioLayer_t    OutputLayerInfo;
	U32                 OutputCodingMode;

	DataAlignment_t     TrancodedDataAlignment;
	U32		AudioCodingMode;
	/* PTS and Flags received from current input buffer : Used only for Frame Based transformer*/
	U32			        Flags;
	U32			        PTS;
	U32			        PTS33Bit;
	U32			        InputFrequency;
	BOOL			    TransformerRestart;
	MPEGAudioLayer_t    LayerInfo;
	U32			        Fade;
	U32			        Pan;
	U32			        CodingMode;
	U32			        streamingTransformerFirstTransfer;
   	U8			        NumChannels;

	BOOL			    decoderInByPassMode;
	BOOL			    SingleDecoderInput;

	/* Hold the post processing values, if called before start */
	CompressionMode_t	HoldCompressionMode;
	DynamicRange_t		HoldDynamicRange;
	StereoMode_t 		HoldStereoMode;
	DownMix_t			HoldDownMixParams;
    #ifdef MP3_SUPPORT
        tMp3Config                 *mp3Config_p;
    	ACF_ST20_tDecoderInterface *DecoderTransfer;
    	ACF_ST20_tFrameBuffer      *EStoMP3DecoderTransfer;
    	ACF_ST20_tPcmBuffer        *MP3DecodertoPCMTransfer;
    #endif
    #if DEBUG_DECODING_TIME
        U32                 frameCount;
        U32                 TimeTakeninDecoding;
        U32                 TimeTakenPerFrame;
    	STOS_Clock_t        timeBeforeDecode;
    #endif
}DecoderControlBlock_t;

/* Deocder Control Blocks list Structure*/
typedef struct DecoderControlBlockList_s
{
	DecoderControlBlock_t			DecoderControlBlock; /* Deocder Control Block */
	struct DecoderControlBlockList_s	*Next_p;
}DecoderControlBlockList_t;


/* }}} */

/* {{{ Function defines */

/* Interface Fucntions */
ST_ErrorCode_t 	STAud_DecInit(const ST_DeviceName_t DeviceName,const STAud_DecInitParams_t *InitParams);
ST_ErrorCode_t 	STAud_DecTerm(STAud_DecHandle_t		Handle);
ST_ErrorCode_t 	STAud_DecOpen(const ST_DeviceName_t DeviceName,	STAud_DecHandle_t *Handle);
ST_ErrorCode_t  STAud_DecClose(void);
ST_ErrorCode_t 	STAud_DecStart(STAud_DecHandle_t		Handle);
ST_ErrorCode_t 	STAud_DecSetStreamParams(STAud_DecHandle_t		Handle,	STAUD_StreamParams_t * StreamParams_p);
ST_ErrorCode_t 	STAud_DecStop(STAud_DecHandle_t		Handle);
ST_ErrorCode_t 	STAud_DecMute(STAud_DecHandle_t	Handle,	BOOL Mute);
ST_ErrorCode_t 	STAud_DecGetCapability(STAud_DecHandle_t Handle, STAUD_DRCapability_t * Capability_p);


/* Task Entry Function */
void                STAud_DecTask_Entry_Point(void* params);

ST_ErrorCode_t	ResetQueueParams(DecoderControlBlock_t	*ControlBlock);
ST_ErrorCode_t	STAud_DecSetAndInitDecoder(DecoderControlBlock_t	*ControlBlock);
ST_ErrorCode_t 	STAud_DecInitDecoderQueueInParams(	DecoderControlBlock_t	*TempDecBlock);
ST_ErrorCode_t 	STAud_DecInitDecoderQueueOutParams(DecoderControlBlock_t	*TempDecBlock);

ST_ErrorCode_t 	STAud_DecAllocateDecoderQueueInParams(	DecoderControlBlock_t	*TempDecBlock);
ST_ErrorCode_t 	STAud_DecAllocateDecoderQueueOutParams(DecoderControlBlock_t	*TempDecBlock);
ST_ErrorCode_t 	STAud_DecFreeDecoderQueueInParams(	DecoderControlBlock_t	*TempDecBlock);
ST_ErrorCode_t 	STAud_DecFreeDecoderQueueOutParams(DecoderControlBlock_t	*TempDecBlock);


BOOL 		    STAud_DecIsInit(const ST_DeviceName_t DeviceName);
ST_ErrorCode_t	STAud_InitDecCapability(DecoderControlBlock_t *TempDecBlock);

/* Control Block queue Updation functions */
void 		    STAud_DecQueueAdd(DecoderControlBlockList_t *QueueItem_p);
void *   		STAud_DecQueueRemove( DecoderControlBlockList_t	*QueueItem_p);
DecoderControlBlockList_t *	STAud_DecGetControlBlockFromName(const ST_DeviceName_t DeviceName);
DecoderControlBlockList_t *	STAud_DecGetControlBlockFromHandle(STAud_DecHandle_t Handle);

ST_ErrorCode_t  STAud_SendCommand(DecoderControlBlock_t *ControlBlock,AudioDecoderStates_t	DesiredState);
ST_ErrorCode_t	STAud_DecStopDecoder(DecoderControlBlock_t	*ControlBlock);

ST_ErrorCode_t  STAud_DecSendCommand(AudioDecoderStates_t	DesiredState,
									DecoderControlBlock_t *ControlBlock);
ST_ErrorCode_t	STAud_DecProcessData(DecoderControlBlock_t	*ControlBlock);

U32             STAud_DecGetSamplFreq(STAud_DecHandle_t Handle);
void        	STAud_ApplyPan(DecoderControlBlock_t	*ControlBlock);
void            STAud_CalculateRamp(DecoderControlBlock_t	*ControlBlock);
void  			STAUD_SetStereoOutputAtAmphion (STAUD_StereoMode_t StereoMode);

ST_ErrorCode_t	STAud_DecGetOPBMHandle(STAud_DecHandle_t	Handle, U32 OutputId, STMEMBLK_Handle_t *BMHandle_p);
ST_ErrorCode_t	STAud_DecSetIPBMHandle(STAud_DecHandle_t	Handle, U32 InputId, STMEMBLK_Handle_t BMHandle);
void            STAud_DecSetStreamInfo(DecoderControlBlock_t	*ControlBlock , MemoryBlock_t *InputBlockFromProducer);
ST_ErrorCode_t  STAUD_GetAmphionStereoMode(STAud_DecHandle_t Handle, STAUD_StereoMode_t	*StereoMode_p);

/* Reset function for hw Amphion decoder*/
void            ResetAudioDecoder(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  STAUD_SetAmphionPan(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  STAUD_SetAmphionAttenuation(STAud_DecHandle_t		Handle,STAUD_Attenuation_t  *Attenuation_p);
void            STAUD_SetAttenuationAtAmphion(U16 Left, U16 Right);
void	        UpdateLayerInfo(DecoderControlBlock_t * ControlBlock);
void            UpdateStereoOutputBuffer(DecoderControlBlock_t * ControlBlock,MemoryBlock_t	*OutputBlockFromProducer);
ST_ErrorCode_t  STAUD_SetAmphionStereoMode(STAud_DecHandle_t		Handle,STAUD_StereoMode_t	StereoMode);
ST_ErrorCode_t  STAUD_SetDecodingType(STAud_DecHandle_t		Handle);
ST_ErrorCode_t  CheckAndUpdateInputFDMAQueue(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  CheckAndUpdateOutputFDMAQueue(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  DecodeFrame(DecoderControlBlock_t * ControlBlock);
void            ConfigureAmphionDecoder(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  StartDecoderTransfers(DecoderControlBlock_t * ControlBlock);
void            DecoderInputFDMACallbackFunction(U32 TransferId,U32 CallbackReason,U32 *CurrentNode_p,U32 NodeBytesRemaining,BOOL Error,void *ApplicationData_p, clock_t  InterruptTime );
void            DecoderOutputFDMACallbackFunction(U32 TransferId,U32 CallbackReason,U32 *CurrentNode_p,U32 NodeBytesRemaining,BOOL Error,void *ApplicationData_p, clock_t  InterruptTime);
ST_ErrorCode_t  DeliverOutputBuffer(DecoderControlBlock_t * ControlBlock,BOOL Error);
ST_ErrorCode_t  ReleaseInputBuffer(DecoderControlBlock_t * ControlBlock, BOOL Error);
ST_ErrorCode_t  WaitForDecoderTransferToComplete(DecoderControlBlock_t * ControlBlock);
BOOL            CheckErrorConditions(DecoderControlBlock_t * ControlBlock);
void            UpdateOutputBufferParams(DecoderControlBlock_t * ControlBlock);


//mp3
#ifdef MP3_SUPPORT
    void            ConfigureMP3Decoder(DecoderControlBlock_t * ControlBlock);
    enum eErrorCode StartMP3DecoderTransfers(DecoderControlBlock_t * ControlBlock);
#endif
ST_ErrorCode_t 	EStoMP3DecoderTransferParameter(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t 	MP3DecodertoPCMTransferParameter(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t 	DecoderTransferParameter(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  ConfigurePCMByPass(DecoderControlBlock_t *ControlBlock);
void            SetVolumeandStereoMode(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  ConfigureEStoMP3DecoderInputParameter(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  ConfigureMP3DecodertoPCMOutputParameter(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t  ConfigureDecoderTransferParameter(DecoderControlBlock_t * ControlBlock);
ST_ErrorCode_t 	STAUD_GetAmphionAttenuation(STAud_DecHandle_t Handle,STAUD_Attenuation_t  *Attenuation_p);
#endif /* __AMPHIONAUDDEC_H__ */



