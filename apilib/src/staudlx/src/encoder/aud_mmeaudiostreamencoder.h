/********************************************************************************
 *   Filename       : Aud_MMEAudioStreamEncoder.h                                           *
 *   Start              : 25.01.2006                                                                            *
 *   By                 : Rahul Bansal                                          *
 *   Contact        : rahul.bansal@st.com                                       *
 *   Description    : The file contains the streaming MME based Audio Encoder APIs that will be         *
 *                Exported to the Modules.                                          *
 ********************************************************************************
 */

#ifndef __MMEAUDSTRENC_H__
#define __MMEAUDSTRENC_H__

/* {{{ Includes */
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stlite.h"
#include "stddefs.h"

/* MME FIles */
#include "acc_mme.h"
#include "pcm_postprocessingtypes.h"
#include "audio_encodertypes.h"
#include "wmae_encodertypes.h"

#include "staudlx.h"
#include "blockmanager.h"
#include "stos.h"
#include "aud_utils.h"


/* }}} */

/* {{{ Defines */

/* Encoder task stack size */
#define STREAMING_AUDIO_ENCODER_TASK_STACK_SIZE (1024*16)

/* Encoder task priority */
#ifndef STREAMING_AUDIO_ENCODER_TASK_PRIORITY
#define STREAMING_AUDIO_ENCODER_TASK_PRIORITY       6
#endif

/* Maximum queue size for encoder */
#define STREAMING_ENCODER_QUEUE_SIZE            24
#define NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM   1
#define NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM  1
#define SEND_CMD_RECOVERY_THRESHHOLD            3

/* Encoder Handle */
typedef U32 STAud_EncHandle_t;
/* }}} */


/* {{{ Enumerations */

/* Encoder State Machine */
typedef enum {
    MME_ENCODER_STATE_IDLE = 0,
    MME_ENCODER_STATE_START,
    MME_ENCODER_STATE_ENCODING,
    MME_ENCODER_STATE_STOP,
    MME_ENCODER_STATE_STOPPED,
    MME_ENCODER_STATE_TERMINATE,
}MMEAudioEncoderStates_t;

/* Input States for Sending Data to Encoder */
typedef enum {
    MME_ENCODER_INPUT_STATE_IDLE = 0,
    MME_ENCODER_INPUT_STATE_GET_INPUT_DATA,
    MME_ENCODER_INPUT_STATE_GET_OUTPUT_BUFFER
}MMEAudioEncoderInputStates_t;

/* }}} */

/* {{{ Global Variables */

/* Encoder(LX) input buffer params */
typedef struct EncoderInBufParams_s
{
    MME_DataBuffer_t        BufferIn;
    MME_DataBuffer_t        * BufferIn_p;
    MME_ScatterPage_t       ScatterPageIn[NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM];
}EncoderInBufParams_t;

/* Encoder(LX) output buffer params */
typedef struct EncoderOutBufParams_s
{
    MME_DataBuffer_t        BufferOut;
    MME_DataBuffer_t        * BufferOut_p;
    MME_ScatterPage_t       ScatterPageOut[NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM];

}EncoderOutBufParams_t;

/* Encoder command params along with I/O Buffers */
typedef struct EncoderInputCmdParams_s
{
    MME_Command_t                       Cmd;         /* MME Command */
    MME_AudioEncoderStatusParams_t      EncoderFrameStatus;
    MME_AudioEncoderTransformParams_t   EncoderTransformParam;

    BOOL                                commandCompleted;   /* Boolean to check command status */

    /* Encoder I/O Buffer params. Used for storing pointers to comprssed and empty buffers which
        will be sent to encoder */
    EncoderInBufParams_t                EncoderBufIn;

    /* Structure to Hold received Memory Block */
    MemoryBlock_t                       *InputBlockFromProducer[NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM];     /*  Compressed input Buffer */

}EncoderInputCmdParam_t;

typedef struct EncoderOutputCmdParams_s
{
    MME_Command_t                   Cmd;         /* MME Command */
    MME_AudioEncoderStatusBuffer_t  EncoderStatusBuffer;
    MME_StreamingBufferParams_t     StreamingBufferParams;
    BOOL                            commandCompleted;   /* Boolean to check command status */

    /* Encoder I/O Buffer params. Used for storing pointers to comprssed and empty buffers which
        will be sent to encoder */
    EncoderOutBufParams_t           EncoderBufOut;

    /* Structure to Hold received Memory Block */
    MemoryBlock_t                   *OutputBlockFromConsumer[NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM];       /*  Empty output Buffer */

}EncoderOutputCmdParam_t;

/* Encoder Init Structure.This structure should be passed along with Encoder Init call */
typedef struct STAud_EncInitParams_s
{
    /* Stream Content Types info(MPEG/AC3 etc)*/
    STAUD_StreamParams_t        *StreamParams_p;

    /* memory partitions */
    STAVMEM_PartitionHandle_t   AVMEMPartition;
    ST_Partition_t              *DriverPartition;

    /* Buffer manager Handle */
    STMEMBLK_Handle_t           InBufHandle;
    STMEMBLK_Handle_t           OutBufHandle;
    U8                          NumChannels;
} STAud_EncInitParams_t;


/* Encoder Input params structure. Input parameters are received with input data through BM*/
typedef struct STAud_EncInputParams_s
{
    /* Recieved from application hrough API, will be used to calculate Number of samples in input Block*/
    U32                     InputFrequency;
    U32                     WordSize;
    enum eAccWordSizeCode   WordSizeCode;   // Same as wordsize in acc code
    U8                      NumChannels;
} STAud_EncInputParams_t;

/* Deocder Control Blocks list Structure - It will have all parms needed for one Instance of Encoder*/
typedef struct EncoderControlBlock_s
{
    /* Device Name */
    ST_DeviceName_t             DeviceName;

    /* Device Handle */
    STAud_EncHandle_t           Handle;

    /* Current Encoder State */
    MMEAudioEncoderStates_t     CurEncoderState;
    /* Next encoder state. Will be given by the command to encoder. On
    receiving Next state encoder will change its current state to next state */
    MMEAudioEncoderStates_t     NextEncoderState;

    /* Encoder task will wait on this semaphore */
    semaphore_t                 *EncoderTaskSem_p;

    /* Control Block Mutex */
    STOS_Mutex_t                *LockControlBlockMutex_p;

    /* Command Completed Semaphore - For Synchronizing(block) Encoder State change commands */
    semaphore_t                 *CmdCompletedSem_p;
    /* Encoder input states */
    MMEAudioEncoderInputStates_t EncInState;

    /* Encoder command queue - used for handling encoder activities */
    EncoderInputCmdParam_t      *EncoderInputQueue[STREAMING_ENCODER_QUEUE_SIZE];
    EncoderOutputCmdParam_t     *EncoderOutputQueue[STREAMING_ENCODER_QUEUE_SIZE];

    /* AVMEM block handle to allocate Input queue elements from AVMEM*/
    STAVMEM_BlockHandle_t       AVMEMInBufferHandle;
    /* AVMEM block handle to allocate Output queue elements from AVMEM*/
    STAVMEM_BlockHandle_t       AVMEMOutBufferHandle;

    /* Current encoder parameter for one send input buffer command */
    EncoderInputCmdParam_t      *CurrentInputBufferToSend;

    /* Current encoder queue buffer we are processing for output */
    EncoderOutputCmdParam_t     *CurrentOutputBufferToSend;

    /* Encoder queue management Variables */
    U32                         EncoderInputQueueTail, EncoderOutputQueueTail;  /* Incoming compressed buffer will be added here */
    U32                         EncoderInputQueueFront, EncoderOutputQueueFront;    /* Encoded buffer will be sent from this offset */
    U32                         EncoderInputQueueFromMME, EncoderOutputQueueFromMME;    /* Next expected buffer to be returned from Encoder */

    /* Number of Input/Output Scatter Pages to be used Per transform*/
    /* Should be set to 1 for frame based encoders*/
    U32                         NumInputScatterPagesPerTransform;
    U32                         NumOutputScatterPagesPerTransform;

    /* MME encoder parameters */
    MME_TransformerInitParams_t MMEinitParams;
    STAud_EncInitParams_t       InitParams;
    MME_TransformerHandle_t     TranformHandleEncoder;

    /* Encoder Task */
    task_t *                    EncoderTask;

    /* memory partitions */
    STAVMEM_PartitionHandle_t   AVMEMPartition;

    /* Stream Content Types (MPEG/AC3 etc)*/
    STAUD_StreamContent_t       EncodedStreamContent;
    enum eAccEncoderId          EncID;
    enum eAccProcessApply       SFCApply;

    /* Audio encoder output parameters given by application*/
    /* Contains info regarding stream type , bitrate sampling frequency etc*/
    STAUD_ENCOutputParams_s     OutputParams;

    /* Audio encoder input params received through BM*/
    STAud_EncInputParams_t      InputParams;

    /* Buffer manager Handle */
    STMEMBLK_Handle_t           InMemoryBlockManagerHandle;
    STMEMBLK_Handle_t           OutMemoryBlockManagerHandle;

    /* Structure to receive current Memory Block for Buffer Management */
    MemoryBlock_t               *CurInputBlockFromProducer;     /*  Compressed input Buffer */
    MemoryBlock_t               *CurOutputBlockFromConsumer;        /*  Empty output Buffer */

    ConsumerParams_t            consumerParam_s;
    ProducerParams_t            ProducerParam_s;

    /* MME Update Global Param Command List */
    /* We are using a list here because user can send multiple update command, one after other,
        before the previously issued command is returned from Encoder */
//  UpdateGlobalParamList_t     *UpdateGlobalParamList;

    /* PP Cmd list protector */
    STOS_Mutex_t                *PPCmdMutex_p;

    /* Default Encoded buffer size. It will be used to send proper size to next unit when
        encoder return error. This will make next units work normally
    */
    U32                         DefaultEncodedSize;

    /* Default Compressed frame size for transcoded output. It will be used to send proper size to next unit when
        encoder return error. This will make next units work normally
    */
    U32                         DefaultCompressedFrameSize;

    /* Encoder Capability returned to Application*/
    STAUD_ENCCapability_t       EncCapability;

    /* Encoder Capabilities as returned by LX*/
    MME_TransformerCapability_t EncTransformeCapabilities;
    MME_AudioEncoderInfo_t      EncInfo;
    DataAlignment_t             EncodedDataAlignment;
    /* Transformer Init Bool */
    BOOL                        Transformer_Init;

    /* PTS and Flags received from current input buffer : Used only for Frame Based transformer*/
    U32                         Flags;
    U32                         PTS;
    BOOL                        TransformerRestart;

    /* Output frequency of Encoded data which is returned by LX
    Currently this will be same as output frequency configured. This should be equal to
    the actual frequency returned by LX*/

    U32                         EncodedOutputFrequency;

    S32                         SamplesPerOutputFrame;
    /* These var will hold the number of samples currently pending with LX*/
    S32                         PendingInputSamples;
    S32                         PendingOutputSamples;


    MME_WmaeStatus_t            WmaeStatus; //stores the status of the WMA encoder
    BOOL                        WmaeAsfHeaderReceived; //to check whether ASF header is received
    MemoryBlock_t               WmaeAsfHeaderBlock; //The ASF Header, will be corrected when last input buffer callback is received
    BOOL                        WmaeFirstOpBlkReceived; //checks if first op block call back received
    BOOL                        WmaeLastOpBlkDelivered; //checks if last op block call back received
    U32                         WmaeEncodedDataSize; //Size of the WMA encoded data/file delivered to the next unit

}EncoderControlBlock_t;

/* Deocder Control Blocks list Structure*/
typedef struct EncoderControlBlockList_s
{
    EncoderControlBlock_t            EncoderControlBlock; /* Deocder Control Block */
    struct EncoderControlBlockList_s *Next_p;
}EncoderControlBlockList_t;


/* }}} */


/* {{{ Function defines */

/* Interface Fucntions */
ST_ErrorCode_t  STAud_EncInit(const ST_DeviceName_t DeviceName,const STAud_EncInitParams_t *InitParams);
ST_ErrorCode_t  STAud_EncTerm(STAud_EncHandle_t     Handle);
ST_ErrorCode_t  STAud_EncOpen(const ST_DeviceName_t DeviceName, STAud_EncHandle_t *Handle);
ST_ErrorCode_t  STAud_EncClose(void);
ST_ErrorCode_t  STAud_EncStart(STAud_EncHandle_t        Handle);
ST_ErrorCode_t  STAud_EncSetStreamParams(STAud_EncHandle_t      Handle, STAUD_StreamParams_t * StreamParams_p);
ST_ErrorCode_t  STAud_EncStop(STAud_EncHandle_t     Handle);
ST_ErrorCode_t  STAud_EncoderGetCapability(STAud_EncHandle_t Handle, STAUD_ENCCapability_t * Capability_p);
ST_ErrorCode_t  STAud_EncoderGetOutputParams(STAud_EncHandle_t Obj_Handle,STAUD_ENCOutputParams_s * EncoderOPParams_p);
ST_ErrorCode_t  STAud_EncoderSetOutputParams(STAud_EncHandle_t Obj_Handle,STAUD_ENCOutputParams_s EncoderOPParams);
ST_ErrorCode_t  STAUD_GetEncCapability( STAUD_ENCCapability_t * EncCapability);

/* Task Entry Function */
void            STAud_EncTask_Entry_Point(void* params);

/* Local function */
void            ResetOutputQueueElement(EncoderOutputCmdParam_t * outputQueueElement);
void            ResetInputQueueElement(EncoderInputCmdParam_t * inputQueueElement);

ST_ErrorCode_t  ResetEncoderQueueParams(EncoderControlBlock_t   *ControlBlock);
MME_ERROR       STAud_EncSetAndInitEncoder(EncoderControlBlock_t    *ControlBlock);
ST_ErrorCode_t  EncInitEncoderQueueInParams(    EncoderControlBlock_t   *TempEncBlock,  STAVMEM_PartitionHandle_t AVMEMPartition);
ST_ErrorCode_t  EncInitEncoderQueueOutParams(EncoderControlBlock_t  *TempEncBlock,STAVMEM_PartitionHandle_t AVMEMPartition);
void            EncInitTransformerInitParams(EncoderControlBlock_t *TempEncBlock, enum eAccEncoderId EncID, BOOL EnableByteSwap);
ST_ErrorCode_t  EncDeInitEncoderQueueInParam(EncoderControlBlock_t * EncoderControlBlock);
ST_ErrorCode_t  EncDeInitEncoderQueueOutParam(EncoderControlBlock_t * EncoderControlBlock);
ST_ErrorCode_t  STAud_EncSendCommand(MMEAudioEncoderStates_t    DesiredState,EncoderControlBlock_t *ControlBlock);
ST_ErrorCode_t  STAud_EncReleaseInput(EncoderControlBlock_t *ControlBlock, MME_Command_t *command_p);
ST_ErrorCode_t  EncoderSendCommand(EncoderControlBlock_t *ControlBlock,MMEAudioEncoderStates_t  DesiredState);
ST_ErrorCode_t  EncoderParseInputBufferParams(EncoderControlBlock_t *ControlBlock,MemoryBlock_t *InputBlock);
MME_ERROR       STAud_EncInitTransformer(EncoderControlBlock_t  *ControlBlock);


//WMAe specific local function
ST_ErrorCode_t  WmaeCorrectAsfHeader(EncoderControlBlock_t  *ControlBlock);


MME_ERROR       STAud_EncStopEncoder(EncoderControlBlock_t  *ControlBlock);

/* Control Block queue Updation functions */
void            STAud_EncQueueAdd(EncoderControlBlockList_t *QueueItem_p);
void *          STAud_EncQueueRemove( EncoderControlBlockList_t *QueueItem_p);
EncoderControlBlockList_t *
                STAud_EncGetControlBlockFromName(const ST_DeviceName_t DeviceName);
EncoderControlBlockList_t *
                STAud_EncGetControlBlockFromHandle(STAud_EncHandle_t Handle);
BOOL            STAud_EncIsInit(const ST_DeviceName_t DeviceName);

/* Main processing functions */
ST_ErrorCode_t  STAud_EncProcessInput(EncoderControlBlock_t *ControlBlock);
ST_ErrorCode_t  STAud_EncProcessOutput(EncoderControlBlock_t    *ControlBlock, MME_Command_t        *command_p);

/* MME Calback functions */
ST_ErrorCode_t  STAud_EncMMETransformCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t  STAud_EncMMESendBufferCompleted(MME_Command_t * CallbackData, void *UserData);
void            TransformerCallback_AudioEnc(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);

ST_ErrorCode_t  CalculateEncOutputFrequency(EncoderControlBlock_t *ControlBlock);
void            EncSFCOutputFreq(EncoderControlBlock_t *ControlBlock);
BOOL            EncoderSupportedFrequency(EncoderControlBlock_t *ControlBlock);
BOOL            CheckEncoderUnderflow(EncoderControlBlock_t *ControlBlock);

void            UpdatePendingSamples(S32    *CurrentSamplePending, S32 FrameSample);
//U32 CovertFsCodeToSampleRate(enum eAccFsCode FsCode);
//enum eAccFsCode STAud_EncGetSamplFreq(STAud_EncHandle_t Handle);
/* }}} */
#endif /* __MMEAUDSTRENC_H__ */



