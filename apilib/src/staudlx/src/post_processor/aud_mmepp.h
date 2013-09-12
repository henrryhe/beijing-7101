/********************************************************************************
 *   Filename   	: aud_mmepp.h											*
 *   Start       		: 13.09.2005                                                   							*
 *   By          		: udit Kumar											*
 *   Contact     	: udit-dlh.kumar@st.com										*
 *   Description  	: The file contains the MME based Audio PP APIs that will be			*
 *				  Exported to the Modules.											*
 ********************************************************************************
 */

#ifndef __MMEAUDPP_H__
#define __MMEAUDPP_H__

/* MME FIles */
#include "acc_mme.h"
#include "pcm_postprocessingtypes.h"
#include "audio_decodertypes.h"
#include "acc_mmedefines.h"
#include "mixer_processortypes.h"

/* {{{ Includes */
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdlib.h"
#include "string.h"
#endif
#include "stlite.h"
#include "stddefs.h"
#include "staudlx.h"
#include "blockmanager.h"
#include "acc_multicom_toolbox.h"
#include "staud.h"


/* }}} */

/* {{{ Defines */

/* PP task stack size */
#define	AUDIO_PP_TASK_STACK_SIZE	 (1024*16)

/* PP task priority */
#ifndef	AUDIO_PP_TASK_PRIORITY
#define	AUDIO_PP_TASK_PRIORITY        6
#endif

/* Maximum queue size for PP */
/*Increased Queue size to reduce the task switching */
#define	PP_QUEUE_SIZE                          8

/* PP Handle */
typedef	U32	STAud_PPHandle_t;
/* }}} */

typedef void * STAud_PPValue_t;
/* {{{ Enumerations */

/* PP State Machine */

#define ACC_MIXER_NB_INPUT 1
#define ACC_MIXER_OUTPUT_BUFFERS 1
#define INPUT_MAX_BLOCK_PER_TRANSFORM 4
typedef enum 
{
    MME_PP_STATE_IDLE = 0,
    MME_PP_STATE_START,
    MME_PP_STATE_PLAYING,
    MME_PP_STATE_PAUSED,
    MME_PP_STATE_STOP,
    MME_PP_STATE_STOPPED,
    MME_PP_STATE_TRICK,
    MME_PP_STATE_TERMINATE,
    MME_PP_STATE_UPDATE

}MMEAudioPPStates_t;

/* Input States for Sending Data to PP */
typedef enum 
{
    MME_PP_INPUT_STATE_IDLE = 0,
    MME_PP_INPUT_STATE_GET_INPUT_DATA,
    MME_PP_INPUT_STATE_GET_OUTPUT_BUFFER,
    MME_PP_INPUT_STATE_UPDATE_PP_PARAMS,
    MME_PP_INPUT_STATE_SEND_TO_MME,
    MME_PP_GET_BUFFER_FROM_QUEUE
}MMEAudioPPInputStates_t;

typedef enum 
{
    MME_PP_INPUT_GET_DATA_IDLE = 0,
    MME_PP_INPUT_GET_DATA_CHECK_UNUSED_BLOCK,
    MME_PP_INPUT_GET_DATA_NEW_BLOCK,

}MMEAudioPPInputGetDataStates_t;

typedef enum STAUD_PPType_s
{
    STAUD_PP_BASS,
    STAUD_PP_EQUAL,
    STAUD_PP_DCREMOVE,
    STAUD_PP_DELAY
} STAud_PPType_t;

typedef enum STAUD_PPSpeakerType_e
{
    STAUD_PPSPEAKER_TYPE_SMALL = 1,
    STAUD_PPSPEAKER_TYPE_LARGE = 2

} STAUD_PPSpeakerType_t;

typedef struct STAudPP_SpeakerEnable_s
{
    BOOL Left;
    BOOL Right;
    BOOL LeftSurround;
    BOOL RightSurround;
    BOOL Center;
    BOOL Subwoofer;
    BOOL CsLeft;
    BOOL CsRight;
    BOOL VcrLeft;
    BOOL VcrRight;

} STAudPP_SpeakerEnable_t;

typedef struct STAUD_PPSpeakerConfiguration_s
{
    STAUD_PPSpeakerType_t Front;
    STAUD_PPSpeakerType_t Center;
    STAUD_PPSpeakerType_t LeftSurround;
    STAUD_PPSpeakerType_t RightSurround;
    STAUD_PPSpeakerType_t CsLeft;
    STAUD_PPSpeakerType_t CsRight;
    STAUD_PPSpeakerType_t VcrLeft;
    STAUD_PPSpeakerType_t  VcrRight;
    BOOL		LFEPresent;

} STAud_PPSpeakerConfiguration_t;

/* }}} */

/* {{{ Global Variables */

/* PP(LX) input buffer params */
typedef struct PPInBufParams_s
{
    MME_DataBuffer_t			*BufferIn_p[ACC_MIXER_NB_INPUT];
    MME_ScatterPage_t 		*ScatterPageIn_p[ACC_MIXER_NB_INPUT];
}PPInBufParams_t;

/* PP(LX) output buffer params */
typedef struct PPOutBufParams_s
{
    MME_DataBuffer_t			*BufferOut_p[ACC_MIXER_OUTPUT_BUFFERS];
    MME_ScatterPage_t		*ScatterPageOut_p[ACC_MIXER_OUTPUT_BUFFERS];

}PPOutBufParams_t;

/*Status of Input Used */
typedef struct
{
    MemoryBlock_t	*InputBlockFromProducer;
    U32 BytesUsed;
    BOOL FullyUsed;
}PPInputBlock_t;
typedef struct
{
    PPInputBlock_t  PPInputBlock[INPUT_MAX_BLOCK_PER_TRANSFORM];    
    U32 NumFreeBlock;
    U32 NumUsed; // can be Max 4 per transform
    // as a current 0 will be first one
    U32 TotalInputBytesNeededForTransform;
    U32 TotalInputBytesRecevied; // Total Bytes We have for the current transform
} PPInputBlocks_t;
/* PP command params along with I/O Buffers */

typedef struct PPCmdParams_s
{
    MME_DataBuffer_t*				BufferPtrs[2]; /* Buffer pointers to hold Input and output buffer */
    MME_Command_t					Cmd;		 /* MME Command */
    BOOL							commandCompleted;	/* Boolean to check command status */
	
    MME_LxMixerTransformerFrameDecodeStatusParams_t	PPFrameStatus;	/* Decoed Frame Status */
    /* PP I/O Buffer params. Used for storing pointers to comprssed and empty buffers which
    will be sent to PP */
    PPInBufParams_t				PPBufIn;
    PPOutBufParams_t			PPBufOut;
    
    /* Command for the Transform */
    MME_LxMixerTransformerFrameDecodeParams_t				PPTransformParams;
    
    /* Structure to Hold received Memory Block */
    
    PPInputBlocks_t	InputBlock;
    MemoryBlock_t	*OutputBlockFromConsumer;		/*  Empty output Buffer */

}PPCmdParam_t;
/* PP Init Structure.This structure should be passed along with PP Init call */

typedef struct STAud_PPInitParams_s
{
	/* memory partitions */
    STAVMEM_PartitionHandle_t	AVMEMPartition;
    ST_Partition_t				*DriverPartition;
    /* Buffer manager Handle */
    STMEMBLK_Handle_t		InBufHandle;
    STMEMBLK_Handle_t		OutBufHandle;
    U8                    NumChannels;
} STAud_PPInitParams_t;


typedef struct
{
    U8 NumChannels;
    U32 Frequency;
    U32 OutPutFrequency;
    U32 InputFrequency;
    U32 OutPutNbSamples;
} STAud_PPStartParams_t;

/* Deocder Control Blocks list Structure - It will have all parms needed for one Instance of PP*/
typedef struct PPControlBlock_s
{
    /* Device Name */
    ST_DeviceName_t		DeviceName;
    
    /* Device Handle */
    STAud_PPHandle_t		Handle;
    
    /*downSampling flas*/
    BOOL				DownSamplingEnabled;
    
    /*In order to Serve Any Parameetre Update Request*/
    BOOL 				UpdateRequired;
    
    
    /*Used for Audio mode in pcm processing transformer*/
    BOOL		DoNotOverWriteCodingMode;
    /* Current PP State */
    MMEAudioPPStates_t		CurPPState;
    /* Next PP state. Will be given by the command to PP. On
    receiving Next state PP will change its current state to next state */
    MMEAudioPPStates_t		NextPPState;
    
    /* PP task will wait on this semaphore */
    semaphore_t		*PPTaskSem_p;
    
    semaphore_t		*CmdCompletedSem_p;
    /* Control Block Mutex */
    STOS_Mutex_t		*LockControlBlockMutex_p;
    
    /* PP input states */
    MMEAudioPPInputStates_t	PPInState;
    MMEAudioPPInputGetDataStates_t GetDataState;
    
    /* PP command queue - used for handling PP activities */
    PPCmdParam_t	PPQueue[PP_QUEUE_SIZE];
    
    /* Current PP parameter for one decode input command */
    PPCmdParam_t	*CurrentBufferToSend;
    
    /* Prev PP parameter for one decode input command */
    PPCmdParam_t	*PrevBufferSent;
    
    /* Current PP queue buffer we are processing for output */
    PPCmdParam_t	*CurrentBufferToOut;
    
    /* PP queue management Variables */
    U32		PPQueueTail;	/* Incoming compressed buffer will be added here */
    U32		PPQueueFront;	/* Decoded buffer will be sent from this offset */
    U32		PPQueueFromMME;	/* Next expected buffer to be returned from PP */
    
    /* MME PP parameters */
    MME_TransformerInitParams_t	MMEinitParams;
    MME_TransformerHandle_t    	TranformHandlePP;
    
    U32 TargetBytes;
    
    /*To work with encoder we need to set forced samples at the output */
    U32 ForcedOutputSamples;
    
    /*MME_LxMixerTransformerInitParams_t	LXInitParams;*/
    
    /* PP Task */
    task_t *		PPTask;
    
    /* memory partitions */
    STAVMEM_PartitionHandle_t	AVMEMPartition;
    STAVMEM_BlockHandle_t	BufferInHandle;
    STAVMEM_BlockHandle_t	ScatterPageInHandle;
    STAVMEM_BlockHandle_t	BufferOutHandle;
    STAVMEM_BlockHandle_t	ScatterPageOutHandle;
    
    /* PP init parameters */
    STAud_PPInitParams_t		InitParams;
    
    /* Buffer manager Handle */
    STMEMBLK_Handle_t		InMemoryBlockManagerHandle;
    STMEMBLK_Handle_t		OutMemoryBlockManagerHandle;
    
    /* Structure to receive current Memory Block for Buffer Management */
    MemoryBlock_t	*CurInputBlockFromProducer;		/*  Compressed input Buffer */
    MemoryBlock_t	*CurOutputBlockFromConsumer;		/*  Empty output Buffer */
    
    ConsumerParams_t 		consumerParam_s;
    ProducerParams_t  		ProducerParam_s;
    /* Post Processing Parameters */
    MME_BassMgtGlobalParams_t   *BassMgt;
    MME_EqualizerGlobalParams_t *Equalizer;
    MME_DCRemoveGlobalParams_t  *DcRemove;
    MME_TempoGlobalParams_t     *TempoControl;
    MME_DelayGlobalParams_t     *Delay;
    MME_LxMixerTransformerGlobalParams_t *GlobalPcmParams;
    MME_LxMixerOutConfig_t                     *OutConfig;
    MME_DMixGlobalParams_t			*DmixConfig;
    MME_CMCGlobalParams_t			*CMCConfig;
#ifdef ST_7200
    MME_BTSCGlobalParams_t			*BTSC;
#endif
    STAud_PPStartParams_t				StartParams;
    MME_Command_t						GlobalCmd;
    STAUD_Attenuation_t					Attenuation;
    STAud_PPSpeakerConfiguration_t	SpeakerConfig;
    U8								NumChannels;

}PPControlBlock_t;

/* Deocder Control Blocks list Structure*/
typedef struct PPControlBlockList_s
{
    PPControlBlock_t			PPControlBlock; /* Deocder Control Block */
    struct PPControlBlockList_s	*Next_p;
}PPControlBlockList_t;



/* }}} */


/* {{{ Function defines */

/* Interface Fucntions */
    ST_ErrorCode_t    STAud_PPInit(const ST_DeviceName_t DeviceName,const STAud_PPInitParams_t *InitParams);
    ST_ErrorCode_t    STAud_PPTerm(STAud_PPHandle_t Handle);
    ST_ErrorCode_t    STAud_PPOpen(const ST_DeviceName_t DeviceName, STAud_PPHandle_t *Handle);
    ST_ErrorCode_t    STAud_PPClose(void);
    ST_ErrorCode_t    STAud_PPStart(STAud_PPHandle_t Handle, STAud_PPStartParams_t *StartParams_p);
    ST_ErrorCode_t    STAud_PPStop(STAud_PPHandle_t Handle);
    
    
    
    ST_ErrorCode_t    STAud_PPSetOutputStreamType(STAud_PPHandle_t		Handle, STAUD_StreamContent_t  StreamContent);
    
    /*Set Coding Mode */
    ST_ErrorCode_t    STAud_PPSetCodingMode(STAud_PPHandle_t		Handle, STAUD_AudioCodingMode_t  AudioCodingMode);
    
    
    /*Attenuation Set/Get */
    ST_ErrorCode_t    STAud_PPSetVolParams(STAud_PPHandle_t		Handle, STAUD_Attenuation_t *Attenuataion);
    ST_ErrorCode_t    STAud_PPGetVolParams(STAud_PPHandle_t		Handle, STAUD_Attenuation_t *Attenuataion);
    
    
    
    /*BTSC  Set/Get */
    ST_ErrorCode_t    STAud_PPSetBTSCParams(STAud_PPHandle_t		Handle, STAUD_BTSC_t *DTSNeo_p);
    ST_ErrorCode_t    STAud_PPGetBTSCParams(STAud_PPHandle_t		Handle, STAUD_BTSC_t *DTSNeo_p);
    
    ST_ErrorCode_t    STAud_PPSetEqualizationParams(STAud_PPHandle_t		Handle, STAUD_Equalization_t *Equal);
    ST_ErrorCode_t    STAud_PPGetEqualizationParams(STAud_PPHandle_t		Handle, STAUD_Equalization_t *Equal);
    
    /*Delay Set/get */
    ST_ErrorCode_t    STAud_PPSetChannelDelay(STAud_PPHandle_t		Handle, STAUD_Delay_t *ChannelDelay);
    ST_ErrorCode_t    STAud_PPGetChannelDelay(STAud_PPHandle_t		Handle, STAUD_Delay_t *ChannelDelay);
    
    /*Down Sampling */
    ST_ErrorCode_t    STAud_PPDisableDownSampling(STAud_PPHandle_t );
    ST_ErrorCode_t    STAud_PPEnableDownSampling(STAud_PPHandle_t , U32 Frequency);
    
    /*DC Remove */
    ST_ErrorCode_t    STAud_PPDcRemove(STAud_PPHandle_t Handle, enum eAccProcessApply *Apply);
    ST_ErrorCode_t    STAud_PostProcGetCapability(STAud_PPHandle_t Handle,STAUD_PPCapability_t *Capability);
    /* Task Entry Function */
    
    
    /*Get/Set Speaker Config*/
    ST_ErrorCode_t    STAud_PPGetSpeakerConfig(STAud_PPHandle_t Handle,STAud_PPSpeakerConfiguration_t *SpeakerConfig);
    ST_ErrorCode_t    STAud_PPSetSpeakerConfig(STAud_PPHandle_t Handle,STAud_PPSpeakerConfiguration_t *SpeakerConfig, STAUD_BassMgtType_t BassType);
    
    void                        PP_Task_Entry_Point(void* params);
    
    ST_ErrorCode_t    STAud_PPGetOPBMHandle(STAud_PPHandle_t	Handle, U32 OutputId,STMEMBLK_Handle_t *BMHandle_p);
    ST_ErrorCode_t    STAud_PPSetIPBMHandle(STAud_PPHandle_t	Handle, U32 InputId, STMEMBLK_Handle_t BMHandle);

/* }}} */
#endif /* __MMEAUDPP_H__ */



