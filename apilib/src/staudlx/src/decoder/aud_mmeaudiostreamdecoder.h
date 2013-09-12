/********************************************************************************
 *   Filename           : aud_mmeaudiostreamdecoder.h                                                                                   *
 *   Start              : 25.01.2006                                                                                                    *
 *   By                 : Rahul Bansal                                                                                  *
 *   Contact            : rahul.bansal@st.com                                                                           *
 *   Description        : The file contains the streaming MME based Audio Decoder
 *                        APIs that will be Exported to the Modules.                                                                                      *
 *********************************************************************************/

#ifndef __MMEAUDSTRDEC_H__
#define __MMEAUDSTRDEC_H__

/* {{{ Includes */
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stlite.h"
#include "stddefs.h"

/* MME FIles */
#include "acc_mme.h"
#include "pcm_postprocessingtypes.h"
#include "audio_decodertypes.h"
#include "aud_utils.h"
#include "aud_decoderparams.h"
#include "staudlx.h"
#include "blockmanager.h"
#include "lpcm_decodertypes.h"
#include "aud_decoderparams.h"
#include "wma_decodertypes.h"
#include "wmaprolsl_decodertypes.h"
#include "stos.h"
/* }}} */

/* {{{ Defines */

/* Decoder task stack size */
#define STREAMING_AUDIO_DECODER_TASK_STACK_SIZE (1024*16)

/* Decoder task priority */
#ifndef STREAMING_AUDIO_DECODER_TASK_PRIORITY
#define STREAMING_AUDIO_DECODER_TASK_PRIORITY           6
#endif

/* Maximum queue size for decoder */
#define STREAMING_DECODER_QUEUE_SIZE                    8
#define NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM   1
#define NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM  1

#define SIZE_MPEG_AUDIO_FRAME_BYTES     (10*4)* 1152
#define SIZE_AC3_AUDIO_FRAME_BYTES      (10*4)* 1536

#define ENABLE_START_UP_PATTERN_GENERATION 0
#define ENABLE_EOF_PATTERN_GENERATION      0
#define ENABLE_START_UP_PATTERN_GENERATION_FOR_TRANSCODER 0

/* Decoder Handle */
typedef U32     STAud_DecHandle_t;
/* }}} */


/* {{{ Enumerations */

/* Decoder State Machine */
typedef enum {
    MME_DECODER_STATE_IDLE = 0,
    MME_DECODER_STATE_START,
    MME_DECODER_STATE_PLAYING,
    MME_DECODER_STATE_STOP,
    MME_DECODER_STATE_STOPPED,
    MME_DECODER_STATE_TERMINATE,
}MMEAudioDecoderStates_t;

/* Input States for Sending Data to Decoder */
typedef enum {
    MME_DECODER_INPUT_STATE_IDLE = 0,
    MME_DECODER_INPUT_STATE_GET_INPUT_DATA,
    MME_DECODER_INPUT_STATE_GET_OUTPUT_BUFFER0,
    MME_DECODER_INPUT_STATE_GET_OUTPUT_BUFFER1,
    MME_DECODER_INPUT_STATE_UPDATE_PARAMS_SEND_TO_MME,
    MME_DECODER_INPUT_STATE_BYPASS,
}MMEAudioDecoderInputStates_t;

/* }}} */

/* {{{ Global Variables */

/* Decoder(LX) input buffer params */
typedef struct DecoderInBufParams_s
{
    MME_DataBuffer_t                *BufferIn_p;
    MME_ScatterPage_t               *ScatterPageIn_p[NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM];
}DecoderInBufParams_t;

/* Decoder(LX) output buffer params */
typedef struct DecoderOutBufParams_s
{
    MME_DataBuffer_t                *BufferOut_p;
    MME_ScatterPage_t               *ScatterPageOut_p[NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM];

}DecoderOutBufParams_t;

/* Decoder command params along with I/O Buffers */
typedef struct DecoderInputCmdParams_s
{
    MME_Command_t                       Cmd;             /* MME Command */
    MME_LxAudioDecoderFrameParams_t     FrameParams;    /* Decoder frame params */
    MME_LxAudioDecoderBufferParams_t    BufferParams;
    MME_LxAudioDecoderFrameStatus_t     FrameStatus;    /* Decoed Frame Status */
    BOOL                                commandCompleted;       /* Boolean to check command status */

    /* Decoder I/O Buffer params. Used for storing pointers to comprssed and empty buffers which
        will be sent to decoder */
    DecoderInBufParams_t                DecoderBufIn;

    /* Structure to Hold received Memory Block */
    MemoryBlock_t   *InputBlockFromProducer[NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM];/*  Compressed input Buffer */
}DecoderInputCmdParam_t;

typedef struct DecoderOutputCmdParams_s
{
    MME_DataBuffer_t*               BufferPtrs[3]; /* Buffer pointers to hold Input and output buffer */
    MME_Command_t                   Cmd;           /* MME Command */
    MME_LxAudioDecoderFrameParams_t FrameParams;   /* Decoder frame params */
    MME_LxAudioDecoderFrameStatus_t FrameStatus;   /* Decoed Frame Status */
    BOOL                            commandCompleted; /* Boolean to check command status */

    /* Decoder I/O Buffer params. Used for storing pointers to comprssed and empty buffers which
        will be sent to decoder */
    DecoderOutBufParams_t           DecoderBufOut0;
    DecoderOutBufParams_t           DecoderBufOut1;

    /* Structure to Hold received Memory Block */
    MemoryBlock_t                   *OutputBlockFromConsumer0[NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM]; /*  Empty output Buffer */
    MemoryBlock_t                   *OutputBlockFromConsumer1[NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM]; /*  Empty output Buffer */

}DecoderOutputCmdParam_t;

/* Decoder Init Structure.This structure should be passed along with Decoder Init call */
typedef struct STAud_DecInitParams_s
{
    /* Stream Content Types info(MPEG/AC3 etc)*/
    STAUD_StreamParams_t            *StreamParams_p;

    /* memory partitions */
    STAVMEM_PartitionHandle_t       AVMEMPartition;
    ST_Partition_t                  *DriverPartition;

    /* Buffer manager Handle */
    STMEMBLK_Handle_t               InBufHandle;
    STMEMBLK_Handle_t               OutBufHandle0;
    STMEMBLK_Handle_t               OutBufHandle1;
    U8                              NumChannels;
} STAud_DecInitParams_t;


typedef struct STAud_LpcmStreamParams_s
{
    /* Mark if there is some change in stream params */
    BOOL    ChangeSet;

    /* Possible streams params */
    U32     channels;
    U32     sampleBits;
    U32     sampleBitsGr2;
    U32     sampleRate;
    U32     sampleRateGr2;
    U32     bitShiftGroup2;
    U32     channelAssignment;
    U32     NbSamples;
    U8      mode;
} STAud_LpcmStreamParams_t;

typedef struct STAud_DtsStreamParams_s
{
    /* Mark if there is some change in stream params */
    BOOL    ChangeSet;
    /* Possible streams params */
    U8      FirstByteEncSamples;
    U32     Last4ByteEncSamples;
    U16     DelayLossLess;
    U16     CoreSize;
} STAud_DtsStreamParams_t;


/*Decoder level PCM Processing parameter structure.
  It will have contain state of decoder's PCM processing state
  For example, its MUTE or UNMUTE etc
*/
typedef struct STAud_DecPCMParams_s
{
    /* Decoder Mute*/
    BOOL    Mute;

    /* Expand here */
} STAud_DecPCMParams_t;

/* MME Update Global Param Command List */
typedef struct UpdateGlobalParamList_s
{
    MME_Command_t                   cmd; /* MME Command */
    struct UpdateGlobalParamList_s  *Next_p;
}UpdateGlobalParamList_t;

/* PCM post processing structues.
They will hold the temporary values if command is sent before audio is started */
typedef struct DynamicRange_s
{
    BOOL                    Apply;
    STAUD_DynamicRange_t    DyRange;
}DynamicRange_t;

typedef struct CompressionMode_s
{
    BOOL                    Apply;
    STAUD_CompressionMode_t CompressionMode;
}CompressionMode_t;

typedef struct StereoMode_s
{
    BOOL                Apply;
    STAUD_StereoMode_t  StereoMode;
}StereoMode_t;

typedef struct DownMix_s
{
    BOOL                    Apply;
    STAUD_DownmixParams_t   Downmix;
}DownMix_t;

typedef struct ADControl_s
{
    U32     Fade;
    U32     Pan;
    U32     CurPan;
    U32     TargetPan;
    int     PanDiff;
    U32     FrameNumber;
    U32     ApplyFrameFactor; /*Number of frames after which pan wil be applied*/
    int     RampFactor;       /*rampdown or rampup factor*/
    int     PanErrorFlag;
    U32     NumFramesPerSecond;
    int     FramesLeftForRamp;
}ADControl_t;

/* Deocder Control Blocks list Structure - It will have all parms needed for one Instance of Decoder*/
typedef struct DecoderControlBlock_s
{
    /* Device Name */
    ST_DeviceName_t         DeviceName;

    /* Device Handle */
    STAud_DecHandle_t               Handle;

    /* Current Decoder State */
    MMEAudioDecoderStates_t         CurDecoderState;
    /* Next decoder state. Will be given by the command to decoder. On
    receiving Next state decoder will change its current state to next state */
    MMEAudioDecoderStates_t         NextDecoderState;

    /* Decoder task will wait on this semaphore */
    semaphore_t             *DecoderTaskSem_p;

    /* Control Block Mutex */
    STOS_Mutex_t            *LockControlBlockMutex_p;

    /* Command Completed Semaphore - For Synchronizing(block) Decoder State change commands */
    semaphore_t             *CmdCompletedSem_p;
    /* Decoder input states */
    MMEAudioDecoderInputStates_t    DecInState;

    /* Decoder command queue - used for handling decoder activities */
    DecoderInputCmdParam_t  DecoderInputQueue[STREAMING_DECODER_QUEUE_SIZE];

    DecoderOutputCmdParam_t DecoderOutputQueue[STREAMING_DECODER_QUEUE_SIZE];

    /* AVMem block handle to store I/P Data Buffer handle*/
    STAVMEM_BlockHandle_t   BufferInHandle;
    /* AVMem block handle to store I/P scatterpage handle*/
    STAVMEM_BlockHandle_t   ScatterPageInHandle;
    /* AVMem block handle to store O/P scatterpage handle of output buffer 0 and 1*/
    STAVMEM_BlockHandle_t   ScatterPageOutHandle;
    /* AVMem block handle to store Buffer handle for MME Data buffers of output buffer 0 and 1*/
    STAVMEM_BlockHandle_t   BufferOutHandle;

    /* Current decoder parameter for one send input buffer command */
    DecoderInputCmdParam_t  *CurrentInputBufferToSend;

    /* Current decoder queue buffer we are processing for output */
    DecoderOutputCmdParam_t *CurrentOutputBufferToSend;

    /* Decoder queue management Variables */
    U32     DecoderInputQueueTail, DecoderOutputQueueTail;      /* Incoming compressed buffer will be added here */
    U32     DecoderInputQueueFront, DecoderOutputQueueFront;    /* Decoded buffer will be sent from this offset */
    U32     DecoderInputQueueFromMME, DecoderOutputQueueFromMME;/* Next expected buffer to be returned from Decoder */

    BOOL    UseStreamingTransformer;
    BOOL    EnableTranscoding;              // To enable transcoding for DDPLUS

    /* Number of Input/Output Scatter Pages to be used Per transform*/
    /* Should be set to 1 for frame based decoders*/
    U32     NumInputScatterPagesPerTransform;
    U32     NumOutputScatterPagesPerTransform;

    /* MME decoder parameters */
    MME_TransformerInitParams_t     MMEinitParams;
    MME_TransformerHandle_t         TranformHandleDecoder;

    /* Decoder Task */
    task_t *                DecoderTask;

    /* memory partitions */
    STAVMEM_PartitionHandle_t       AVMEMPartition;

    /* Stream Content Types (MPEG/AC3 etc)*/
    STAUD_StreamParams_t    StreamParams;

    /*   Beep Tone Frequency */
    U32                 BeepToneFrequency;

    /* Decoder init parameters */
    STAud_DecInitParams_t           InitParams;

    /* Buffer manager Handle */
    STMEMBLK_Handle_t               InMemoryBlockManagerHandle;
    STMEMBLK_Handle_t               OutMemoryBlockManagerHandle0;
    STMEMBLK_Handle_t               OutMemoryBlockManagerHandle1;

    /* Structure to receive current Memory Block for Buffer Management */
    MemoryBlock_t   *CurInputBlockFromProducer;             /*  Compressed input Buffer */
    MemoryBlock_t   *CurOutputBlockFromConsumer;            /*  Empty output Buffer */

    ConsumerParams_t                consumerParam_s;
    ProducerParams_t                ProducerParam_s;

    /* Deocder PCM processing params */
    STAud_DecPCMParams_t    PcmProcess;
    /* MME Update Global Param Command List */
    /* We are using a list here because user can send multiple update command, one after other,
        before the previously issued command is returned from Decoder */
    UpdateGlobalParamList_t         *UpdateGlobalParamList;

    /* PP Cmd list protector */
    STOS_Mutex_t                            *PPCmdMutex_p;

    /* Default Decoded buffer size. It will be used to send proper size to next unit when
        decoder return error. This will make next units work normally
    */
    U32                                             DefaultDecodedSize;
    /* Default Compressed frame size for transcoded output. It will be used to send proper size to next unit when
        decoder return error. This will make next units work normally
    */
    U32                                             DefaultCompressedFrameSize;

    /* Decoder Capability structure */
    STAUD_DRCapability_t            DecCapability;

    /* PCM Processing setting for decoder. We will preserve them in b/w decoder start/stop sequences */
    MME_STm7100PcmProcessingGlobalParams_t  *DecPcmProcessing;

    /*Adding AD Control Structure to store AD related values.*/
    ADControl_t         ADControl;

    STAUD_Attenuation_t  Attenuation_p;

    /* Transformer Init Bool */
    BOOL    Transformer_Init;

    /* Sampling frequency received from decoder */
    enum eAccFsCode         SamplingFrequency;
    enum eAccFsCode         TranscodedSamplingFrequency;

    DataAlignment_t     TrancodedDataAlignment;
    enum eAccAcMode         AudioCodingMode;
    U8                                      DMixTableParams;
    /* PTS and Flags received from current input buffer : Used only for Frame Based transformer*/
    U32                     Flags;
    U32                     PTS;
    U32                     PTS33Bit;
    U32                     InputFrequency;
    BOOL                    TransformerRestart;
    U32                     LayerInfo;
    U32                     Fade;
    U32                     Pan;
    U32                     CodingMode;
    U32                     streamingTransformerFirstTransfer;
    BOOL                    GeneratePattern;
    U8                      NumChannels;

    BOOL                    decoderInByPassMode;

    /* Hold the post processing values, if called before start */
    CompressionMode_t       HoldCompressionMode;
    DynamicRange_t          HoldDynamicRange;
    StereoMode_t            HoldStereoMode;
    DownMix_t                       HoldDownMixParams;

#if ENABLE_START_UP_PATTERN_GENERATION
    U32                             StartPatternBufferAddr;
#endif

    BOOL        StreamingEofFound;
    U8                      DDPOPSetting;
    U8			InputStereoMode;
    STAUD_StreamContent_t StreamContent;
    U8                      SBRFlag;
    U32                     InputCodingMode;
    U32             Emphasis;
}DecoderControlBlock_t;

/* Deocder Control Blocks list Structure*/
typedef struct DecoderControlBlockList_s
{
        DecoderControlBlock_t                   DecoderControlBlock; /* Deocder Control Block */
        struct DecoderControlBlockList_s        *Next_p;
}DecoderControlBlockList_t;


/* }}} */


/* {{{ Function defines */

/* Interface Fucntions */
ST_ErrorCode_t  STAud_DecInit(const ST_DeviceName_t DeviceName,const STAud_DecInitParams_t *InitParams);
ST_ErrorCode_t  STAud_DecTerm(STAud_DecHandle_t         Handle);
ST_ErrorCode_t  STAud_DecOpen(const ST_DeviceName_t DeviceName, STAud_DecHandle_t *Handle);
ST_ErrorCode_t  STAud_DecClose(void);
ST_ErrorCode_t  STAud_DecStart(STAud_DecHandle_t                Handle);
ST_ErrorCode_t  STAud_DecSetStreamParams(STAud_DecHandle_t              Handle, STAUD_StreamParams_t * StreamParams_p);
ST_ErrorCode_t  STAud_DecStop(STAud_DecHandle_t         Handle);
ST_ErrorCode_t  STAud_DecMute(STAud_DecHandle_t Handle, BOOL Mute);
ST_ErrorCode_t  STAud_DecGetCapability(STAud_DecHandle_t Handle, STAUD_DRCapability_t * Capability_p);
ST_ErrorCode_t  STAud_DecGetSamplingFrequency(STAud_DecHandle_t Handle, U32* SamplingFrequency_p);
ST_ErrorCode_t  STAud_DecSetInitParams(STAud_DecHandle_t Handle, STAUD_DRInitParams_t *InitParams_p);
ST_ErrorCode_t  STAud_DecGetStreamInfo(STAud_DecHandle_t Handle, STAUD_StreamInfo_t * StreamInfo_p);


/* Task Entry Function */
void                STAud_DecTask_Entry_Point(void* params);

/* Local function */
ST_ErrorCode_t  ResetQueueParams(DecoderControlBlock_t  *ControlBlock);
ST_ErrorCode_t  STAud_DecInitDecoderQueueInParams( DecoderControlBlock_t *TempDecBlock,STAVMEM_PartitionHandle_t AVMEMPartition);
ST_ErrorCode_t  STAud_DecInitDecoderQueueOutParams(DecoderControlBlock_t *TempDecBlock,STAVMEM_PartitionHandle_t AVMEMPartition);
ST_ErrorCode_t  STAud_DecDeInitDecoderQueueInParam(DecoderControlBlock_t * DecoderControlBlock);
ST_ErrorCode_t  STAud_DecDeInitDecoderQueueOutParam(DecoderControlBlock_t * DecoderControlBlock);
ST_ErrorCode_t  STAud_DecSendCommand(MMEAudioDecoderStates_t    DesiredState,DecoderControlBlock_t *ControlBlock);
MME_ERROR       STAud_DecStopDecoder(DecoderControlBlock_t      *ControlBlock);
MME_ERROR       STAud_DecSetAndInitDecoder(DecoderControlBlock_t        *ControlBlock);
void            STAud_DecInitDecoderFrameParams(DecoderControlBlock_t *TempDecBlock);

/* Control Block queue Updation functions */
void            STAud_DecQueueAdd(DecoderControlBlockList_t *QueueItem_p);
void *          STAud_DecQueueRemove( DecoderControlBlockList_t *QueueItem_p);
DecoderControlBlockList_t * STAud_DecGetControlBlockFromName(const ST_DeviceName_t DeviceName);
DecoderControlBlockList_t * STAud_DecGetControlBlockFromHandle(STAud_DecHandle_t Handle);
BOOL    STAud_DecIsInit(const ST_DeviceName_t DeviceName);

/* Main processing functions */
ST_ErrorCode_t  STAud_DecProcessInput(DecoderControlBlock_t     *ControlBlock);
ST_ErrorCode_t  STAud_DecProcessOutput(DecoderControlBlock_t    *ControlBlock, MME_Command_t *command_p);
ST_ErrorCode_t  STAud_DecReleaseInput(DecoderControlBlock_t     *ControlBlock);

/* MME Calback functions */
ST_ErrorCode_t  STAud_DecMMETransformCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t  STAud_DecMMESetGlobalTransformCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t  STAud_DecMMESendBufferCompleted(MME_Command_t * CallbackData, void *UserData);
void TransformerCallback_AudioDec(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);

ST_ErrorCode_t  HandleTransformUpdate(DecoderControlBlock_t *DecoderControlBlock,
                                      STAud_LpcmStreamParams_t *StreamParams, STAUD_StreamContent_t StreamType);
ST_ErrorCode_t  HandleDtsTransformUpdate(DecoderControlBlock_t *DecoderControlBlock, STAud_DtsStreamParams_t *StreamParams);
ST_ErrorCode_t  HandleWMATransformUpdate(DecoderControlBlock_t *DecoderControlBlock, MemoryBlock_t *dataBufferIn);
ST_ErrorCode_t  HandleWMAProLslTransformUpdate(DecoderControlBlock_t *DecoderControlBlock, MemoryBlock_t *dataBufferIn);
ST_ErrorCode_t  STAud_InitDecCapability(DecoderControlBlock_t *TempDecBlock);
ST_ErrorCode_t  STAud_DecApplyPcmParams(DecoderControlBlock_t   *ControlBlock);
ST_ErrorCode_t  STAud_SendCommand(DecoderControlBlock_t *ControlBlock,MMEAudioDecoderStates_t   DesiredState);
ST_ErrorCode_t  STAud_DecPingLx(U8 *LxRevision, U32 StringSize);
ST_ErrorCode_t  STAud_DecHandleInput(DecoderControlBlock_t      *ControlBlock);
ST_ErrorCode_t  STAud_DecGetOutBuf0(DecoderControlBlock_t       *ControlBlock);
ST_ErrorCode_t  STAud_DecGetOutBuf1(DecoderControlBlock_t       *ControlBlock);
ST_ErrorCode_t  STAud_DecSendCmd(DecoderControlBlock_t  *ControlBlock);
void            STAud_DecSetStreamInfo(DecoderControlBlock_t    *ControlBlock , MemoryBlock_t *InputBlockFromProducer);
void            STAud_ApplyPan(DecoderControlBlock_t    *ControlBlock);
void            STAud_CalculateRamp(DecoderControlBlock_t       *ControlBlock , U32 NumberOfFrames);


#ifdef ENABLE_START_UP_PATTERN_GENERATION
void            SetStartPattern(MemoryBlock_t   *outputBlock, U32 NumChannels);
#endif

#ifdef ENABLE_EOF_PATTERN_GENERATION
void            SetEOFPattern(MemoryBlock_t     *outputBlock, U32 NUMChannels);
#endif

#ifdef ENABLE_START_UP_PATTERN_GENERATION_FOR_TRANSCODER
void            SetTranscoderStartPattern(MemoryBlock_t    *outputBlock, U32 compressedFrameSize);
#endif

enum eAccFsCode STAud_DecGetSamplFreq(STAud_DecHandle_t Handle);
U32 STAud_DecGetBeepToneFreq(STAud_DecHandle_t Handle, U32 *Freq_p);
ST_ErrorCode_t  STAud_DecSetBeepToneFreq(STAud_DecHandle_t      Handle,U32 BeepToneFrequency);
ST_ErrorCode_t  STAud_DecSetDDPOP(STAud_DecHandle_t     Handle, U32 DDPOPSetting);

ST_ErrorCode_t  STAud_DecGetOPBMHandle(STAud_DecHandle_t Handle, U32 OutputId, STMEMBLK_Handle_t *BMHandle_p);
ST_ErrorCode_t  STAud_DecSetIPBMHandle(STAud_DecHandle_t Handle, U32 InputId, STMEMBLK_Handle_t BMHandle);
/* }}} */
#endif /* __MMEAUDSTRDEC_H__ */



