#ifndef MSPPARSER_COMMON_H_
#define MSPPARSER_COMMON_H_

#include "staudlx.h"
#include "blockmanager.h"
#include "audio_parsertypes.h"
#include "stos.h"
#include "aud_utils.h"

typedef U32 MspParser_Handle_t;
typedef ST_ErrorCode_t (* PESES_ConsumedByte_t)
(
	U32 , U32
);


/* Maximum queue size for parser */

#define BUFFER_THRESHOLD 1024
#define STREAMING_PARSER_INPUT_QUEUE_SIZE			32
#define STREAMING_PARSER_OUTPUT_QUEUE_SIZE			8
#define NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM	1
#define NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM	1
#define UNITY_INDEX		1
#define MPEG_LAYER_BIT		0x3

typedef enum eMpegLayer_s
{
    MPG_LAYER4,
    MPG_LAYER3,
    MPG_LAYER2,
    MPG_LAYER1
}eMpegLayer_t;


/* MME Update Global Param Command List */
typedef struct UpdateGlobalCmdList_s
{
	MME_Command_t			cmd; /* MME Command */
	struct UpdateGlobalCmdList_s	*Next_p;
}UpdateGlobalCmdList_t;


/* Input States for Sending Data to Parser */
typedef enum {

	MME_PARSER_INPUT_STATE_IDLE = 0,
	MME_PARSER_INPUT_STATE_GET_INTPUT_DATA,
	MME_PARSER_INPUT_STATE_GET_OUTPUT_BUFFER,
	MME_PARSER_INPUT_STATE_BYPASS_DATA
}MMEAudioParserInputStates_t;

typedef struct ParserInBufParams_s
{
	//STAVMEM_BlockHandle_t	BufferInHandle;
	MME_DataBuffer_t			*BufferIn_p;
	//STAVMEM_BlockHandle_t	ScatterPageInHandle;
	MME_ScatterPage_t		*ScatterPageIn_p[NUM_INPUT_SCATTER_PAGES_PER_TRANSFORM];
}ParserInBufParams_t;

typedef struct ParserOutBufParams_s
{
	//STAVMEM_BlockHandle_t	BufferOutHandle;
	MME_DataBuffer_t			*BufferOut_p;
	//STAVMEM_BlockHandle_t	ScatterPageOutHandle;
	MME_ScatterPage_t		*ScatterPageOut_p[NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM];
}ParserOutBufParams_t;

typedef struct ParserInputCmdParams_s
{
	MME_Command_t					Cmd;		 /* MME Command */
	MME_AudioParserBufferParams_t	 	BufferParams;
	MME_AudioParserFrameStatus_t		FrameStatus;
	BOOL							commandCompleted;	/* Boolean to check command status */
	ParserInBufParams_t				ParserBufIn;

}ParserInputCmdParam_t;

typedef struct ParserOutputCmdParam_s
{
	MME_Command_t					Cmd;		 /* MME Command */
	MME_AudioParserFrameParams_t		FrameParams;	/* Parser frame params */
	MME_AudioParserFrameStatus_t		FrameStatus;	/* Parser Frame Status */
	BOOL							commandCompleted;	/* Boolean to check command status */
	ParserOutBufParams_t				ParserBufOut;

	/* Structure to Hold received Memory Block */
	MemoryBlock_t	*OutputBlockFromConsumer[NUM_OUTPUT_SCATTER_PAGES_PER_TRANSFORM];		/*  Empty output Buffer */

}ParserOutputCmdParam_t;

typedef struct
{
	ST_Partition_t * Memory_Partition;
	MspParser_Handle_t * Handle_p;
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
	STCLKRV_Handle_t				CLKRV_HandleForSynchro;
#endif
	STMEMBLK_Handle_t	      		OpBufHandle;
	STAVMEM_PartitionHandle_t		AVMEMPartition;

	STEVT_EventID_t				EventIDNewFrame;
	STEVT_EventID_t 				EventIDFrequencyChange;
	STEVT_Handle_t				EvtHandle;
	STAUD_Object_t				ObjectID;
	void 						*PESESBytesConsumedcallback;
	U32 							PESESHandle;
}MspParserInit_t;

typedef struct
{

	U32 FrameSize;
	U16 NoBytesCopied;
	BOOL FrameComplete;
	U8 * Current_Write_Ptr1;
	MemoryBlock_t	*OpBlk_p;
	U32 Blk_Out;
	U32 Skip;
	U32 Pause;
} MspBypassParams_t;



typedef struct
{
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
	STCLKRV_Handle_t 			Clksource;	/*required for dvr*/
#endif

	STAUD_StreamContent_t 		StreamContent;
	STAUD_StreamType_t 			StreamType;
	U8							StreamID;/*TBC*/
	MMEAudioParserInputStates_t	ParseInState;
	MspParser_Handle_t  			Handle;
	MspParserInit_t				InitParams;

	/* parser capability structure */
	STAUD_DRCapability_t			ParseCapability;

	/* MME Update Global Param Command List */
	/* We are using a list here because user can send multiple update command, one after other,
	    before the previously issued command is returned from Decoder */
	UpdateGlobalCmdList_t			*UpdateGlobalParamList;

	/* PP Cmd list protector */
	STOS_Mutex_t				*GlobalCmdMutex_p;
	STEVT_EventID_t				EventIDNewFrame;
	STEVT_EventID_t 				EventIDFrequencyChange;
	STEVT_Handle_t				EvtHandle;
	STAUD_Object_t				ObjectID;


	/* Parser command queue - used for handling decoder activities */

	ParserInputCmdParam_t	ParserInputQueue[STREAMING_PARSER_INPUT_QUEUE_SIZE + 1];/*last element for EOF in case input queue is full*/
	ParserOutputCmdParam_t	ParserOutputQueue[STREAMING_PARSER_OUTPUT_QUEUE_SIZE];


	/* AVMem block handle to store Buffer handle for MME Data buffers of output buffer 0*/
	STAVMEM_BlockHandle_t	BufferInHandle0;
	STAVMEM_BlockHandle_t	ScatterPageInHandle;
	STAVMEM_BlockHandle_t	BufferOutHandle0;
	STAVMEM_BlockHandle_t	ScatterPageOutHandle;

	/* Current Parser queue buffer we are processing for output */
	ParserInputCmdParam_t		*CurrentInputBufferToSend;
	ParserOutputCmdParam_t		*CurrentOutputBufferToSend;

	/* Decoder queue management Variables */
	U32		ParserInputQueueTail, ParserOutputQueueTail;	/* Incoming compressed buffer will be added here */
	U32		ParserInputQueueFront, ParserOutputQueueFront;	/* Decoded buffer will be sent from this offset */


	/* Number of Input/Output Scatter Pages to be used Per transform*/
	U32		NumInputScatterPagesPerTransform;
	U32		NumOutputScatterPagesPerTransform;

	/* MME parser parameters */
	MME_TransformerInitParams_t	MMEinitParams;
	MME_TransformerHandle_t    	TranformHandleParser;

	/* Buffer manager Handle */
	STMEMBLK_Handle_t		OutMemoryBlockManagerHandle;

	/* Structure to receive current Memory Block for Buffer Management */
	MemoryBlock_t			*CurOutputBlockFromConsumer;		/*  Empty output Buffer */
	ProducerParams_t  		ProducerParam_s;

    /*Structure to be used when Bypass mode is true.*/
    MspBypassParams_t *BypassParams;

	eMpegLayer_t	MpegLayer;
	U16			DecoderId;
	U32			Emphasis;
	U32			NbOutSamples;
	U32			PTS;
	U32			PTSflag;
	U32			AudioCodingMode;
	U32			SamplingFrequency;
	U32			FrameSize;
	S32			Speed;
	U32 			PTS_DIVISION;
	PESES_ConsumedByte_t		PESESBytesConsumedcallback;

	U32 			PESESHandle;
	BOOL		EOFBlockUsed;
	BOOL		Transformer_Init;
	BOOL		TransformerRestart;
	U32			Last_Parsed_Size;
	BOOL		Mpeg1_LayerSet;
	BOOL		MarkSentEof;
	BOOL        ByPass; /*flag to denote if the parser will be in bypass mode.*/

}MsppControlBlock_t;


/* MSP Parser functions prototype definition*/

ST_ErrorCode_t 	Mspp_Parser_Init(MspParserInit_t *MspParserInit_p, MspParser_Handle_t *Handle_p);
ST_ErrorCode_t 	Mspp_Parser_SetStreamType(MspParser_Handle_t  Handle,STAUD_StreamType_t StreamType,STAUD_StreamContent_t StreamContents);
ST_ErrorCode_t 	Mspp_Parser_SetBroadcastProfile(MspParser_Handle_t  Handle,STAUD_BroadcastProfile_t BroadcastProfile);
ST_ErrorCode_t 	Mspp_Parser_SetStreamID(MspParser_Handle_t  Handle, U32 StreamID);
ST_ErrorCode_t 	Mspp_Parser_SetPCMParams(MspParser_Handle_t  Handle,U32 Frequency, U32 SampleSize, U32 Nch);
ST_ErrorCode_t 	Mspp_Parser_GetInfo(MspParser_Handle_t  Handle,STAUD_StreamInfo_t * StreamInfo_p);
ST_ErrorCode_t 	Mspp_Parser_GetFreq(MspParser_Handle_t  Handle,U32 *SamplingFrequency_p);
ST_ErrorCode_t 	Mspp_Parser_HandleEOF(MspParser_Handle_t  Handle);
ST_ErrorCode_t 	Mspp_Parser_SetSpeed(MspParser_Handle_t  Handle,S32 Speed);

ST_ErrorCode_t 	STAud_MsppInitParserQueueInParams(MsppControlBlock_t   *Msp_ParserControlBlock_p, STAVMEM_PartitionHandle_t AVMEMPartition);
ST_ErrorCode_t 	STAud_MsppInitParserQueueOutParams(MsppControlBlock_t *Msp_ParserControlBlock_p, STAVMEM_PartitionHandle_t AVMEMPartition);
ST_ErrorCode_t	STAud_MsppDeInitParserQueueInParam(MsppControlBlock_t  *Msp_ParserControlBlock_p);
ST_ErrorCode_t	STAud_MsppDeInitParserQueueOutParam(MsppControlBlock_t *Msp_ParserControlBlock_p);
void				STAud_MsppInitParserFrameParams (MsppControlBlock_t *Msp_ParserControlBlock_p);
ST_ErrorCode_t	ResetMsppQueueParams(MsppControlBlock_t *Msp_ParserControlBlock_p);


/*MSPP Lx parser callback registered function*/

void 			TransformerCallback_AudioParser(MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);

ST_ErrorCode_t	STAud_ParseReleaseInput(MsppControlBlock_t	*Msp_ParserControlBlock_p);
ST_ErrorCode_t	STAud_ParseProcessOutput(MsppControlBlock_t 	*Msp_ParserControlBlock_p, MME_Command_t		*command_p);
ST_ErrorCode_t	STAud_InitParserCapability(MsppControlBlock_t *parserBlock);


MME_Command_t*		STAud_ParseGetCmd(MsppControlBlock_t	*Msp_ParserControlBlock_p);
ST_ErrorCode_t	ParseTransformUpdate(MsppControlBlock_t *Msp_ParserControlBlock_p, STAUD_StreamContent_t	StreamContent);
ST_ErrorCode_t	STAud_ParseAddCmdToList(UpdateGlobalCmdList_t *Node, MsppControlBlock_t	*Msp_ParserControlBlock_p);
ST_ErrorCode_t	STAud_ParseRemCmdFrmList(MsppControlBlock_t	*Msp_ParserControlBlock_p);

ST_ErrorCode_t 	STAud_ParseMMETransformCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t 	STAud_ParseMMESetGlobalTransformCompleted(MME_Command_t * CallbackData , void *UserData);
ST_ErrorCode_t 	STAud_ParseMMESendBufferCompleted(MME_Command_t * CallbackData, void *UserData);
MME_ERROR		STAud_SetAndInitParser(MsppControlBlock_t*Msp_ParserControlBlock_p);


#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t Mspp_Parser_SetClkSynchro(MspParser_Handle_t  Handle,STCLKRV_Handle_t Clksource);

#endif // STAUD_REMOVE_CLKRV_SUPPORT

#endif //PARSER_COMMON_H_

