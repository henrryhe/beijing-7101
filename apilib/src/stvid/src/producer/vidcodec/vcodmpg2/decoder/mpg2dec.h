/*******************************************************************************

File name   : mpeg2dec.h

Description : lVideo driver MPG2 CODEC API header file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
21 Jul 2004        Created                                           CL
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MPG2DECODER_H
#define __MPG2DECODER_H

/* Includes ----------------------------------------------------------------- */
#include "vidcodec.h"
#include "vcodmpg2.h"
#include "sttbx.h"

#include "vid_ctcm.h"

#include "mme.h"
#include "embx.h"

#define MULTICOM
#define MPEG2_MME_VERSION 32
#include "MPEG2_VideoTransformerTypes.h"

/* Exported Constants ------------------------------------------------------- */
#ifdef PRINT_DECODER_INPUT
#define DEBUG_FILENAME   "./vc1_decode_debug.txt"
#endif  /*PRINT_DECODER_INPUT*/

#ifndef MME_VIDEO_TRANSPORT_NAME
#define MME_VIDEO_TRANSPORT_NAME "ups"
#endif /* not MME_VIDEO_TRANSPORT_NAME */

#ifndef MME_VIDEO1_TRANSPORT_NAME
#define MME_VIDEO1_TRANSPORT_NAME "TransportVideo1"
#endif /* not MME_VIDEO1_TRANSPORT_NAME */

#ifndef MME_VIDEO2_TRANSPORT_NAME
#define MME_VIDEO2_TRANSPORT_NAME "TransportVideo2"
#endif /* not MME_VIDEO2_TRANSPORT_NAME */

#define DECODER_MAX_NUMBER_JOBS 1 /* 1 means the decoder handles only one decode job at a time. */
#ifndef DECODER_COMMANDS_BUFFER_SIZE
#define DECODER_COMMANDS_BUFFER_SIZE 10
#endif

/* Definition for f_code */
#define F_CODE_FORWARD      0
#define F_CODE_BACKWARD     1
#define F_CODE_HORIZONTAL   0
#define F_CODE_VERTICAL     1

enum
{
    MPEG2DECODER_INFORM_READ_ADDRESS_EVT_ID,
    MPEG2DECODER_JOB_COMPLETED_EVT_ID,
    MPEG2DECODER_CONFIRM_BUFFER_EVT_ID,
    MPEG2DECODER_STOP_EVT_ID,
    MPEG2DECODER_NB_REGISTERED_EVENTS_IDS
};

#ifdef STVID_DEBUG_GET_STATISTICS
typedef struct ParserStats_s
{
    U32                                 DecodePictureSkippedRequested;
    U32                                 DecodePictureSkippedNotRequested;
    U32                                 DecodeInterruptPipelineIdle;
    U32                                 DecodeInterruptDecoderIdle;                      /*++ on pipeline idle interrupt*/
    U32                                 DecodeInterruptStartDecode;                      /*++ on decoder idle interrupt*/
    U32                                 DecodePbMaxNbInterruptsSyntaxErrorPerPicture;
    U32                                 DecodePbInterruptSyntaxError;
    U32                                 DecodePbInterruptDecodeOverflowError;            /*++ on severe error or overflow error interrupt*/
    U32                                 DecodePbInterruptDecodeUnderflowError;
    U32                                 DecodePbInterruptMisalignmentError;              /*= 0 (for Omega1 decoder only)*/
    U32                                 DecodePbFirmwareWatchdogError;
} DecoderStats_t;
#endif


/* Exported types ----------------------------------------------------------- */

typedef enum
{
	MPEG2DECODER_CONTEXT_NOT_USED,      /* keep this one at the first one */
	MPEG2DECODER_CONTEXT_DECODE_FUNC,
	MPEG2DECODER_CONTEXT_CONFIRM_BUFFER_FUNC,
	MPEG2DECODER_CONTEXT_CONFIRM_BUFFER_EVT,
	MPEG2DECODER_CONTEXT_JOB_COMPLETED_EVT
} MPEG2DecoderContextState_t;

typedef struct MPEG2DecoderContext_s
{
	MPEG2DecoderContextState_t		State;
	U32							CommandIdParam;
	U32							CommandIdFmw;
    BOOL            HasBeenRejected;
	DECODER_DecodingJobResults_t DECODER_DecodingJobResults;
#if defined(VALID_TOOLS)
    void *  Address_p;  /* Allocated address of the total of the buffer */
#endif
} MPEG2DecoderContext_t;

typedef struct MPEG2CompanionData_s
{
    MME_Command_t                       CmdInfoParam; /* used for MME_SET_GLOBAL_TRANSFORM_PARAMS command */
    MME_Command_t                       CmdDecoder;   /* used for MME_TRANSFORM for Firmware commands */
    MPEG2_CommandStatus_t               MPEG2CmdStatus;
}
MPEG2CompanionData_t;

typedef struct MPG2DecoderPrivateData_s
{
    ST_Partition_t                 *CPUPartition_p;
    U32                            NextCommandId;
    U32                            CommandId;

    DECODER_Status_t               DecoderStatus;
    MPEG2CompanionData_t           *MPEG2CompanionData_p;   /* Data sent or received by Companion chip (LX) */
    DECODER_DecodingJobResults_t   DecodingJobResults;
    DECODER_InformReadAddress_t    InformReadAddress;
    BOOL                           TransformParamsConfirmed;
/*     BOOL                                   DecoderIsRunning;*/
    MPEG2DecoderContext_t            DecoderContext;

    MME_TransformerHandle_t        TranformHandleDecoder;
    STEVT_EventID_t                RegisteredEventsID[MPEG2DECODER_NB_REGISTERED_EVENTS_IDS];

    U32                                      CommandsData[DECODER_COMMANDS_BUFFER_SIZE];
    CommandsBuffer32_t             CommandsBuffer;     /* Data concerning the decode controller commands queue */
    MPEG2_SetGlobalParamSequence_t GlobalParamSeq;
    MPEG2_TransformParam_t         TransformParam;

    void                           *BitBufferAddress_p;
    U32                            BitBufferSize;

    QuantiserMatrix_t               intra_quantiser_matrix;
    QuantiserMatrix_t               non_intra_quantiser_matrix;
    QuantiserMatrix_t               chroma_intra_quantiser_matrix;
    QuantiserMatrix_t               chroma_non_intra_quantiser_matrix;

#ifdef STVID_DEBUG_GET_STATISTICS
    DecoderStats_t                 Statistics;
#endif
#ifdef STVID_DEBUG_GET_STATUS
	struct
    {
    	STVID_SequenceInfo_t * SequenceInfo_p;  /* Last sequence information, NULL if none or stopped */
    } Status;
#endif /* STVID_DEBUG_GET_STATUS */
    MPEG2_InitTransformerParam_t    MPEG2_InitTransformerParam;
    BOOL                            IsEmbxTransportValid;
    EMBX_TRANSPORT                  EmbxTransport;
    STVID_CodecMode_t               CodecMode;
} MPEG2DecoderPrivateData_t;
/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
/* mpeg2dec.c module */
ST_ErrorCode_t mpg2dec_GetState(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId, DECODER_Status_t * const DecoderStatus_p);
ST_ErrorCode_t mpg2dec_GetCodecInfo(const DECODER_Handle_t DecoderHandle, DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication, const STVID_CodecMode_t CodecMode);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t mpg2dec_GetStatistics(const DECODER_Handle_t DecoderHandle, STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t mpg2dec_ResetStatistics(const DECODER_Handle_t DecoderHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t mpg2dec_GetStatus(const DECODER_Handle_t DecoderHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
ST_ErrorCode_t mpg2dec_Init(const DECODER_Handle_t DecoderHandle, const DECODER_InitParams_t * const InitParams_p);
ST_ErrorCode_t mpg2dec_Reset(const DECODER_Handle_t DecoderHandle);
ST_ErrorCode_t mpg2dec_DecodePicture(const DECODER_Handle_t DecoderHandle, const DECODER_DecodePictureParams_t * const StartParams_p, DECODER_Status_t * const DecoderStatus_p);
ST_ErrorCode_t mpg2dec_ConfirmBuffer(const DECODER_Handle_t DecoderHandle, const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p);
ST_ErrorCode_t mpg2dec_Abort(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId);
ST_ErrorCode_t mpg2dec_Setup(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p);
ST_ErrorCode_t mpg2dec_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t mpg2dec_Term(const DECODER_Handle_t DecoderHandle);
#ifdef ST_speed
void mpg2dec_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode);
#ifdef STVID_TRICKMODE_BACKWARD
void mpg2dec_SetDecodingDirection(const DECODER_Handle_t DecoderHandle, const DecodingDirection_t Direction);
#endif
#endif
ST_ErrorCode_t mpg2dec_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize);
#if defined (DVD_SECURED_CHIP)
ST_ErrorCode_t mpg2dec_SetupForH264PreprocWA_GNB42332(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* DVD_SECURED_CHIP */


void mpg2dec_CompletedDecoding(void * const DecoderHandle_p);
void mpg2dec_AskForConfirmBufferNotification(void * const DecoderHandle_p);

ST_ErrorCode_t mpg2dec_StartdecoderTask(const DECODER_Handle_t decoderHandle);
ST_ErrorCode_t mpg2dec_StopDecoderTask(const DECODER_Handle_t DecoderHandle);

#endif /* #ifdef __MPG2DECODER_H */

/* end of mpg2dec.h */
