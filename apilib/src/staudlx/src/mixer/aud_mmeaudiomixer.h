/********************************************************************************
 *   Filename       : aud_mmeaudiomixer.h                                               *
 *   Start              : 11.10.2005                                                                            *
 *   By                 : Chandandeep Singh Pabla                                           *
 *   Contact        : chandandeep.pabla@st.com                                      *
 *   Description    : The file contains the MME based Audio Mixer APIs that will be             *
 *                Exported to the Modules.                                          *
 ********************************************************************************
 */

#ifndef __MMEAUDMIX_H__
#define __MMEAUDMIX_H__

/* {{{ Includes */
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "stlite.h"
#include "stddefs.h"

/* MME FIles */
#include "acc_mme.h"
#include "pcm_postprocessingtypes.h"
#include "mixer_processortypes.h"
#include "audio_decodertypes.h"
#include "acc_multicom_toolbox.h"
#include "aud_decoderparams.h"
#include "staudlx.h"
#include "blockmanager.h"
#include "aud_evt.h"
/* }}} */

/* {{{ Defines */

/* Mixer task stack size */
#if defined(ST_51XX)
#define AUDIO_MIXER_TASK_STACK_SIZE (1024*2)
#else
#define AUDIO_MIXER_TASK_STACK_SIZE (1024*16)
#endif
/* Mixer task priority */
#ifndef AUDIO_MIXER_TASK_PRIORITY
#define AUDIO_MIXER_TASK_PRIORITY       2
#endif

/* Maximum queue size for decoder */
#define MIXER_QUEUE_SIZE            1

/* millisec for one "CELL" or "FRAME" of Mixer */
#ifdef ST_51XX
#define MSEC_PER_MIXER_TRANSFORM        24//32
#else
#define MSEC_PER_MIXER_TRANSFORM        16//32
#endif

/* 192k produces a bigger number but since 192k*2chs < 96k*10chs and 192k*10chs isn't supported */
#define MAX_PCM_SAMPLES_PER_TRANSFORM   MSEC_PER_MIXER_TRANSFORM * 96000 * 1/1000 /*msec samp/sec sec/msec */

/* Mixer output frequency */
#define MIXER_OUT_FREQ  48000

/*make this flag FALSE to get the main input freq at the output */
/*but we don't go beyond 48KHz as buffer allocation is as per 48KHz*/
#ifdef ST_51XX
#define MIXER_OUT_PCM_SAMPLES ((MSEC_PER_MIXER_TRANSFORM * MIXER_OUT_FREQ)/ 1000) /*extra data provided for safety*/
#define VARIABLE_MIX_OUT_FREQ   1
#else
#define MIXER_OUT_PCM_SAMPLES (((MSEC_PER_MIXER_TRANSFORM + 2) * MIXER_OUT_FREQ)/ 1000) /*extra data provided for safety*/
#define VARIABLE_MIX_OUT_FREQ   0
#endif

/* Stream ID of Master stream at mixer input */
#define MASTER_STREAM_ID    0

/* Mixer Handle */
typedef U32 STAud_MixerHandle_t;
/* }}} */


/* {{{ Enumerations */

/* Mixer State Machine */
typedef enum {

    MIXER_STATE_IDLE = 0,
    MIXER_STATE_START,
    MIXER_STATE_MIXING,
    MIXER_STATE_STOP,
    MIXER_STATE_TERMINATE

}MMEAudioMixerStates_t;

/* Input States for Sending Data to Mixer */
typedef enum {

    MIXER_IDLE = 0,
    MIXER_STARTUP_PTS_CHECK,
    MIXER_GET_BUFFER_TO_SEND,
    MIXER_GET_OUTPUT_BUFFER,
    MIXER_GET_INPUT_DATA,
    MIXER_UPDATE_MIXER_PARAMS,
    MIXER_CALCULATE_PTS,
    MIXER_SEND_TO_MME,
    MIXER_PROCESS_OUTPUT

}MMEAudioMixerInputStates_t;

/* Mixer Errors */
typedef enum {

    ST_NO_STARTUP_PTS = 0x100,
    ST_WAITING_FOR_ALL_STREAMS_DATA,
    ST_ERROR_MALLOC_FAILED,
    ST_ERROR_NO_BLOCK_TO_SEND

}MMEAudioMixerErrors_t;

/* }}} */

/* {{{ Global Variables */

/* Mixer(LX) input buffer params */
typedef struct MixerInBufParams_s
{
    MME_DataBuffer_t            *BufferIn_p;
    MME_ScatterPage_t       *ScatterPageIn_p;
}MixerInBufParams_t;

/* Mixer(LX) output buffer params */
typedef struct MixerOutBufParams_s
{
    MME_DataBuffer_t            *BufferOut_p;
    MME_ScatterPage_t           *ScatterPageOut_p;

}MixerrOutBufParams_t;

/* input blocks link list */
typedef struct MixerInStreamBlocklist_s
{

    MemoryBlock_t               *InputBlock;    /* Input Block received */
    BOOL                            BlockUsed;  /* Marks if this buffer still holds some valid data */
    U32                         StartOffset;    /* Marks the offset from where data is sent to mixer from current InputBlock */
    U32                         BytesConsumed;/* Marks the total bytes consumed from start of this input Block */
    struct MixerInStreamBlocklist_s *Next_p;

}MixerInStreamBlocklist_t;

/* Mixer structure to hold and apply FADE values */
typedef struct STAUD_MixFadeInfo_s
{
    int curFade;
    U32 lastReceivedFade;
    int fadeSteps;
}STAUD_MixFadeInfo_t;

/* Mixer Input params node. For each input stream, one such node will be created */
typedef struct MixerInputStreamNode_s
{

    BOOL                    RegisteredAtMixer;      /* If this flag is set, Mixer will take into account the input from 'this' stream else ignore  it*/
    BOOL                    StreamWithValidPTS;     /* Does the stream has valid PTS or its a raw PCM stream? */
    BOOL                    GotFirstValidPTS;       /* Received the first valid PTS i.e. stream is available for mixing */
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_ExtendedSTC_t   ExtInPTS;                   /* Extended PTS value */
#endif
    U32                     LeftOverBytes;          /* leftover bytes from last IP block */
    U32                     BytesUsed;              /* Bytes consumed from leftover block */
    U32                     sampleBits;
    /* Input Stream Information*/
    U32                     numChannels;            /* No of channel in Input Stream */
    U32                     sampleRate;             /* Sample rate of this input stream */
    U32                     bytesPerSample;         /* Sample size in bytes of this input stream, 16bit=2, 32bit=4*/
    U32                     bytesPerMSec;           /* Number of bytes per millisec */
    U32                     bytesPerSampleTime; /* Number of bytes per audio sample time or "cell" */
    U16                     MixLevel;
    U16                     NewMixLevel;
    BOOL                    UserProvidedLevel;

    /* Mixer Fade info for AD channel*/
    STAUD_MixFadeInfo_t     FadeInfo;

    MixerInBufParams_t      MixerBufIn;             /* Input buffer place holder*/

    U32                     bytesToPause;
    U32                     bytesToSkip;

    /* Buffer manager Handle */
    STMEMBLK_Handle_t       InMemoryBlockHandle;

    /* these variables be used for proper handling of Start/Stop. We will reset them*/
    U32                     TransformerInBytes;     /* Valid bytes collected from input stream */
    U32                     ScatterPageNum;         /* Scatter Page nuber for this frame*/

    MixerInStreamBlocklist_t    *InBlockStarList_p; /* List of memory blocks received from producer. pointer to start of input */

}MixerInputStreamNode_t;

/* Mixer Output params node(Characterstics of mixed output stream ) */
typedef struct MixerOutputStreamNode_s
{
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    STCLKRV_ExtendedSTC_t ExtOutPTS;                    /* Extended PTS value for output stream*/
#endif
    /* output Stream Information*/
    U32                     outputChannels;         /* No of channel in output Stream */
    U32                     outputSampleRate;       /*  Sample rate of this output stream */
    U32                     outputBytesPerSample;   /* Sample size in bytes of this output stream, 16bit=2, 32bit=4*/
    U32                     outputBytesPerSec;      /* Number of bytes per second */
    U32                     outputSamplesPerTransform;      /* Number of Samples per Transform */

    /* Buffer manager Handle */
    STMEMBLK_Handle_t       OutBufHandle;

    MemoryBlock_t           *OutputBlockFromConsumer;                   /*  Empty output Buffer */

}MixerOutputStreamNode_t;

/* Current Mixer In/Out params info */
typedef struct MixerStreamsInfo_s
{

    U16                     NoOfRegisteredInput; /* Number of registered inputs for mixing */

    //STCLKRV_ExtendedSTC_t ExtMasterPTS;   / ExtendedPTS of Master stream
    //STCLKRV_ExtendedSTC_t ExtOutputPTS;   / Extended Extrapolated/actual PTS for output stream

    U16                     CurrentInputID;/* Id of the input stream, currently under processing (For handling Start/stop.Reset it)*/

    MixerOutputStreamNode_t OutputStreamParams; /* Ouput stream parameters */

    MixerInputStreamNode_t  InputStreamsParams[ACC_MIXER_MAX_NB_INPUT]; /* Actual value of input streams is equal to NoOfRegisteredInput*/

}MixerStreamsInfo_t;

/* Mixer command params along with I/O Buffers */
typedef struct MixerCmdParams_s
{

    MME_DataBuffer_t*               BufferPtrs[ACC_MIXER_MAX_BUFFERS];   /* Buffer pointers to hold Input and output buffers */
    MME_Command_t                   Cmd;                                 /* MME Command */
    BOOL                            commandCompleted;                   /* Boolean to check command status */

    MixerInBufParams_t              MixerBufIn[ACC_MIXER_MAX_NB_INPUT];/* Mixer Input Buffer params*/
    MixerrOutBufParams_t                MixerBufOut;                        /* Mixer Output Buffer params*/

    MME_LxMixerTransformerFrameDecodeParams_t   FrameParams;            /* Mixer frame params */
    MME_LxMixerTransformerFrameDecodeStatusParams_t FrameStatus;        /* Mixer Frame Status */


    U16                     OutputBufferIndex;/* Offset of output buffer in the BufferPtrs array*/
}MixerCmdParams_t;




/* Mixer Init Structure.This structure should be passed along with Mixer Init call */
typedef struct STAud_MixInitParams_s
{
    /* Stream Content Types info(MPEG/AC3 etc)*/
    STAUD_StreamParams_t    *StreamParams_p;

    /* memory partitions */
    STAVMEM_PartitionHandle_t   AVMEMPartition;
    ST_Partition_t              *DriverPartition;
    U16                         NoOfRegisteredInput;
    U32                         InBMHandle[ACC_MIXER_MAX_NB_INPUT];
    U32                         OutBMHandle;
    U8                              NumChannels;
    ST_DeviceName_t         EvtHandlerName;
    ST_DeviceName_t         EvtGeneratorName;
    STAUD_Object_t              ObjectID;

} STAud_MixInitParams_t;


/* Mixer Control Blocks list Structure - It will have all parms needed for one Instance of Mixer*/
typedef struct MixerControlBlock_s
{
    /* Device Name */
    ST_DeviceName_t     DeviceName;

    /*Event generation name */
    ST_DeviceName_t                 EvtGeneratorName;
    STAUD_Object_t                  ObjectID;

    /* Device Handle */
    STAud_MixerHandle_t     Handle;

    /* Current Mixer State */
    MMEAudioMixerStates_t       CurMixerState;

    /* Next Mixer state. Will be given by the command to Mixer. On
    receiving Next state Mixer will change its current state to next state */
    MMEAudioMixerStates_t       NextMixerState;

    /* Info about all In/Out Streams */
    MixerStreamsInfo_t      MixerStreamsInfo;

    /* Mixer task will wait on this semaphore */
    semaphore_t     *MixerTaskSem_p;

    /* Control Block Mutex */
    STOS_Mutex_t    *LockControlBlockMutex_p;

    /* Command Completed Semaphore - For Synchronizing(block) Mixer State change commands */
    semaphore_t     *CmdCompletedSem_p;

    /* Mixer runs in blocking mode i.e. it wits for the command completeion. Mixer will wait on this semaphore */
    semaphore_t     *LxCallbackSem_p;

    /* Mixer input states */
    MMEAudioMixerInputStates_t      MixerState;

    STAVMEM_BlockHandle_t   BufferInHandle;
    STAVMEM_BlockHandle_t   ScatterPageInHandle;
    STAVMEM_BlockHandle_t   BufferOutHandle;
    STAVMEM_BlockHandle_t   ScatterPageOutHandle;

    /* Mixer command queue - used for handling decoder activities */
    MixerCmdParams_t    MixerQueue[MIXER_QUEUE_SIZE];

    /* Current Mixer parameter for one Mixer input command */
    MixerCmdParams_t    *CurrentBufferToSend;

    /* Current Mixer queue buffer we are processing for Mixer */
    MixerCmdParams_t    *CurrentBufferToOut;

    /* Mixer queue management Variables */
    U32     MixerQueueTail;         /* Incoming compressed buffer will be added here */
    U32     MixerQueueFront;        /* Mixer buffer will be sent from this offset */
    U32     MixerQueueFromMME;  /* Next expected buffer to be returned from Mixer */

    /* MME Mixer parameters */
    MME_TransformerInitParams_t MMEMixerinitParams;
    MME_TransformerHandle_t     MixerTranformHandle;

    /* Mixer Task */
    task_t *        MixerTask;
#if defined (ST_OS20)
    U8                      *MixerTaskstack;
    tdesc_t                 MixerTaskDesc;
#endif

    /* memory partitions */
    STAVMEM_PartitionHandle_t   AVMEMPartition;
    ST_Partition_t              *DriverPartition;

    /* Stream Content Types (MPEG/AC3 etc)*/
    STAUD_StreamParams_t    StreamParams;

    /* Structure to receive current Memory Block for Buffer Management */
    MemoryBlock_t           *CurOutputBlockFromConsumer;        /*  Empty output Buffer */
    MemoryBlock_t           *CurInputBlockFromProducer;     /*  input Buffer */

    ConsumerParams_t        consumerParam_s;
    ProducerParams_t        ProducerParam_s;

    /* PP Cmd list protector */
    STOS_Mutex_t                *PPCmdMutex_p;

    /* Mixer Capability structure */
    STAUD_MXCapability_t        MixerCapability;

    /* Mixer transformer update cmd */
    MME_Command_t           GlobalCmd;

    STEVT_Handle_t          EvtHandle;
    // Events
    STEVT_EventID_t     EventIDPCMUnderFlow;
    STEVT_EventID_t     EventIDPCMBufferComplete;

    BOOL                UpdateRequired[ACC_MIXER_MAX_NB_INPUT];
    U16                 MasterInput; /*Specifies the master input*/
    BOOL                UpdateFadeOnMaster;
    BOOL                FreeRun;    /*The Mixer will be free running. No Input is Master*/
    BOOL                CompleteBuffer; /* Specifies if any one input is completely available*/

    /* Transformer Init Bool */
    BOOL                Transformer_Init;
#if VARIABLE_MIX_OUT_FREQ
    BOOL                OpFreqChange;
#endif
}MixerControlBlock_t;

/* mixer Control Blocks list Structure*/
typedef struct MixerControlBlockList_s
{
    MixerControlBlock_t         MixerControlBlock; /* Mixer Control Block */
    struct MixerControlBlockList_s  *Next_p;
}MixerControlBlockList_t;


/* }}} */


/* {{{ Function defines */

/* Interface Fucntions */
ST_ErrorCode_t  STAud_MixerInit (const ST_DeviceName_t DeviceName,const STAud_MixInitParams_t *InitParams);
ST_ErrorCode_t  MixerTerm       (MixerControlBlockList_t        *Mixer_p);
ST_ErrorCode_t  STAud_MixerOpen (const ST_DeviceName_t DeviceName,  STAud_MixerHandle_t *Handle);
ST_ErrorCode_t  STAud_MixerClose(void);
ST_ErrorCode_t  STAud_MixerSetStreamParams  (STAud_MixerHandle_t Handle, STAUD_StreamParams_t * StreamParams_p);
ST_ErrorCode_t  STAud_MixerGetCapability    (STAud_MixerHandle_t Handle, STAUD_MXCapability_t* Capability_p);
ST_ErrorCode_t  STAud_MixerUpdatePTSStatus  (STAud_MixerHandle_t Handle, U32 InputID, BOOL PTSStatus);
ST_ErrorCode_t  STAud_SetMixerMixLevel      (STAud_MixerHandle_t Handle, U32 InputID, U16 MixLevel);
ST_ErrorCode_t  STAud_GetMixerMixLevel      (STAud_MixerHandle_t Handle, U32 InputID, U16 *MixLevel_p);
ST_ErrorCode_t  STAud_MixerSetCmd           (STAud_MixerHandle_t Handle,MMEAudioMixerStates_t State);
ST_ErrorCode_t  STAud_MixerUpdateMaster     (STAud_MixerHandle_t Handle, U32 InputID, BOOL FreeRun);
ST_ErrorCode_t  MixerCheckStateTransition   (MixerControlBlock_t * MixerCtrlBlock_p, MMEAudioMixerStates_t State);
ST_ErrorCode_t  STAud_MixerSetVol           (STAud_MixerHandle_t Handle, STAUD_Attenuation_t  *Attenuation_p);
ST_ErrorCode_t  STAud_MixerGetVol           (STAud_MixerHandle_t Handle, STAUD_Attenuation_t  *Attenuation_p);

/* Task Entry Function */
void                    STAud_MixerTask_Entry_Point(void* params);

/* Local function */

MixerInStreamBlocklist_t *STAud_AllocateListElement(MixerControlBlock_t *ControlBlock);

MME_ERROR   STAud_MixerInitParams(MME_GenericParams_t *init_param, MixerControlBlock_t *ControlBlock);
MME_ERROR   STAud_SetMixerTrsfrmGlobalParams(MME_LxMixerTransformerGlobalParams_t *GlobalParams,
                                MixerControlBlock_t *ControlBlock);

ST_ErrorCode_t STAud_StartUpCheck(MixerControlBlock_t   *MixerCtrlBlock_p);
ST_ErrorCode_t MMEGlobalUpdateParamsCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t STAud_GetOutputBuffer(MixerControlBlock_t *MixerCtrlBlock_p);
ST_ErrorCode_t STAud_FreeIpBlocks(MixerControlBlock_t *MixerCtrlBlock_p, BOOL Flush);
ST_ErrorCode_t STAud_SendMixedFrame(MixerControlBlock_t *MixerCtrlBlock_p, U32 DecodedBufSize, BOOL Flush);
ST_ErrorCode_t STAud_UpdateMixerTransformParams(MixerControlBlock_t *MixerCtrlBlock_p);

ST_ErrorCode_t STAud_GetInputAndAddToTransformParams(MixerControlBlock_t *MixerCtrlBlock_p);
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
void STAud_ExtrapolateExtPts(MixerControlBlock_t *MixerCtrlBlock_p , STCLKRV_ExtendedSTC_t * Pts_p, U32 BytesToExt);
#endif

void STAud_SetMixerInConfig(MME_LxMixerInConfig_t *InConfig,  MixerControlBlock_t   *MixerCtrlBlock_p);
void STAud_SetMixerOutConfig(MME_LxMixerOutConfig_t *OutConfig,  MixerControlBlock_t    *MixerCtrlBlock_p);
void STAud_AddListElement(MixerInStreamBlocklist_t *TempStreamBuf,MixerControlBlock_t *MixerCtrlBlock_p,
                                        U32 InputID, BOOL BlockUsed);
void STAud_FreebufferList(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID);
void STAud_SetTempoParams(MME_TempoGlobalParams_t *TempoControl);
void STAud_SetEncparams(MME_EncoderPPGlobalParams_t *EncControl);
void STAud_SetMixerPCMParams( MME_LxPcmPostProcessingGlobalParameters_t *PcmParams);
void STAud_SetDcrmParams(MME_DCRemoveGlobalParams_t *DcRemove);
void STAud_SetBassMgtParams(MME_BassMgtGlobalParams_t *BassMgt);
void STAud_SetDelayParams(MME_DelayGlobalParams_t *Delay);
void STAud_SetEqParams(MME_EqualizerGlobalParams_t *Equalizer);
void STAud_SetSFCparams(MME_SfcPPGlobalParams_t *SfcControl);
void STAud_SetDMixparams(MME_DMixGlobalParams_t * DMixControl);
void STAud_SetCMCparams(MME_CMCGlobalParams_t *CMCControl);
#ifdef ST_7200
void STAud_SetBTSCparams(MME_BTSCGlobalParams_t *BTSC);
#endif

MME_ERROR           STAud_MixerSetAndInit(MixerControlBlock_t   *MixerCtrlBlock_p);
MME_ERROR           STAud_StopMixer(MixerControlBlock_t *MixerCtrlBlock_p);
ST_ErrorCode_t  STAud_MixerInitInputQueueParams(    MixerControlBlock_t *MixerCtrlBlock_p,
                                        STAVMEM_PartitionHandle_t AVMEMPartition);
ST_ErrorCode_t  STAud_MixerInitOutputQueueParams(MixerControlBlock_t    *MixerCtrlBlock_p,
                                        STAVMEM_PartitionHandle_t AVMEMPartition);
void                    STAud_MixerInitFrameParams(MixerControlBlock_t *MixerCtrlBlock_p);
U32                 STAud_MixerMaxBufSize(void);
ST_ErrorCode_t      STAud_MixerDeInitInputQueueParams(MixerControlBlock_t * MixerCtrlBlock_p);
ST_ErrorCode_t      STAud_MixerDeInitOutputQueueParams(MixerControlBlock_t * MixerCtrlBlock_p);


/* Control Block queue Updation functions */
 void       STAud_MixerQueueAdd(MixerControlBlockList_t *QueueItem_p);
 void *     STAud_MixerQueueRemove( MixerControlBlockList_t *QueueItem_p);
 MixerControlBlockList_t *
                STAud_MixerGetControlBlockFromName(const ST_DeviceName_t DeviceName);
MixerControlBlockList_t *
                STAud_MixerGetControlBlockFromHandle(STAud_MixerHandle_t Handle);
 BOOL       STAud_MixerIsInit(const ST_DeviceName_t DeviceName);

/* Main processing functions */
ST_ErrorCode_t  STAud_MixerMixFrame(MixerControlBlock_t *MixerCtrlBlock_p);
ST_ErrorCode_t  STAud_MixerCollectInputBuffers(MixerControlBlock_t  *MixerCtrlBlock_p, U32  DecodedDataSize);

/* MME Calback functions */
ST_ErrorCode_t  MME_MixerTransformCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t  MixerSetGlobalTransformCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t  MixerSendBufferCompleted(MME_Command_t * CallbackData, void *UserData);
void                    Mixer_TransformerCallback(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);


ST_ErrorCode_t  STAud_InitMixerCapability(MixerControlBlock_t *MixerCtrlBlock_p);
ST_ErrorCode_t STAud_CheckInputStreamFreqChange(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID);
ST_ErrorCode_t STAud_CheckInputStreamParamsChange(MixerControlBlock_t *MixerCtrlBlock_p,
                                STAud_LpcmStreamParams_t *StreamParams,U32 InputID);
void           STAud_CheckFadeParamChange(MixerControlBlock_t *MixerCtrlBlock_p, int CurrentFade,U32 InputID);
ST_ErrorCode_t STAud_MixerUpdateGlobalParams(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID);
ST_ErrorCode_t STAud_MixerRegEvt(const ST_DeviceName_t EventHandlerName,const ST_DeviceName_t AudioDevice,
                                 MixerControlBlock_t *MixerCtrlBlock_p);
ST_ErrorCode_t STAud_MixerUnSubscribeEvt(MixerControlBlock_t * MixerCtrlBlock_p);

ST_ErrorCode_t STAud_MxGetOPBMHandle(STAud_MixerHandle_t    Handle, U32 OutputId, STMEMBLK_Handle_t * BMHandle_p);
ST_ErrorCode_t STAud_MxSetIPBMHandle(STAud_MixerHandle_t    Handle, U32 InputId, STMEMBLK_Handle_t BMHandle);

void MixerApplyStreamProperties(MixerControlBlock_t *MixerCtrlBlock_p, U32 InputID, MemoryBlock_t   * IpBlkFromProducer_p);

MME_ERROR   STAud_TermMixerTransformer(MixerControlBlock_t  *MixerCtrlBlock_p);
MME_ERROR   STAud_MixerInitTransformer(MixerControlBlock_t  *MixerCtrlBlock_p);

/* Functions for audio audio sync in mixer*/
U32             MixerApplyPause     (MixerControlBlock_t    *MixerCtrlBlock_p, U32 InputID, U32 TransformInBytes, U32 TransformTargetBytes);
ST_ErrorCode_t  MixerApplySkip      (MixerControlBlock_t    *MixerCtrlBlock_p, U32 InputID);
ST_ErrorCode_t  MixerGetNextBlock   (MixerControlBlock_t    *MixerCtrlBlock_p, U32 InputID, U32 TransformInBytes);
ST_ErrorCode_t  CheckSyncOnInputID  (MixerControlBlock_t    *MixerCtrlBlock_p, U32 InputID, U32 bytesInFront, MemoryBlock_t * CurInputBlock);
U32             ConvertBytesToPTS   (MixerInputStreamNode_t *InputStreamsParams_p, U32 InputBytes);

/* functions for AD inplementation of FADE in mixer*/
void CaluclateRamp      (MixerInputStreamNode_t * InputStreamsParams_p);
void CheckAndApplyFade  (MixerControlBlock_t    * MixerCtrlBlock_p);
void ApplyFade          (MixerControlBlock_t    *MixerCtrlBlock_p, U32 FadeValue);

/* }}} */
#endif /* __MMEAUDMIX_H__ */



