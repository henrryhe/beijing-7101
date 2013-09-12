/*!
 ************************************************************************
 * \file vc1decode.h
 *
 * \brief VC1 decoder data structures and functions prototypes
 *
 * \author
 *  \n
 * CMG/STB \n
 * Copyright (C) 2005 STMicroelectronics
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __VC1DECODER_H
#define __VC1DECODER_H

/* JSG, If this is defined, then the */
/* input commands from the producer */
/* are printed to a file. */
/* #define PRINT_DECODER_INPUT */

#ifdef PRINT_DECODER_INPUT
#include <stdio.h>
#endif /* PRINT_DECODER_INPUT */

/* Includes ----------------------------------------------------------------- */
#include "stos.h"
#include "sttbx.h"

#include "vcodvc1.h"
#include "vid_com.h"
#include "vid_ctcm.h"
#include "stdevice.h"

#include "mme.h"
#include "embx.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#if defined(STVID_VALID)
#define MME_MAX_DUMP 100
#endif

#if defined(TRACE_UART) && defined(VALID_TOOLS)
#define TRACE_VC1DEC_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_CYAN | TRC_ATTR_BRIGHT)

#define VC1DEC_TRACE_BLOCKID   (0x5015)    /* 0x4000 | STVID_DRIVER_ID */

/* !!!! WARNING !!!! : Any modification of this emun should be reported in emun in vc1decoder.c */
enum
{
    VC1DEC_TRACE_GLOBAL = 0,
    VC1DEC_TRACE_START,
    VC1DEC_TRACE_CONFIRM_BUFFER,
    VC1DEC_TRACE_REJECTED_CB,
    VC1DEC_TRACE_FMW_JOB_END,
    VC1DEC_TRACE_FMW_CALLBACK,
    VC1DEC_TRACE_FMW_ERRORS,
    VC1DEC_TRACE_FB_ADDRESSES
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

/*! Decoder state */
#define DECODER_COMMANDS_BUFFER_SIZE 10

/* events ID used for events registration and notification */
enum
{
    VC1DECODER_INFORM_READ_ADDRESS_EVT_ID,
    VC1DECODER_JOB_COMPLETED_EVT_ID,
    VC1DECODER_NB_REGISTERED_EVENTS_IDS  /* Keep this one as the last one */
};

#ifdef PRINT_DECODER_INPUT
#define DEBUG_FILENAME   "./vc1_decode_debug.txt"
#endif  /*PRINT_DECODER_INPUT */

#ifndef MME_VIDEO_TRANSPORT_NAME
#define MME_VIDEO_TRANSPORT_NAME "ups"
#endif /* not MME_VIDEO_TRANSPORT_NAME */

#ifndef MME_VIDEO1_TRANSPORT_NAME
#define MME_VIDEO1_TRANSPORT_NAME "TransportVideo1"
#endif /* not MME_VIDEO1_TRANSPORT_NAME */

#ifndef MME_VIDEO2_TRANSPORT_NAME
#define MME_VIDEO2_TRANSPORT_NAME "TransportVideo2"
#endif /* not MME_VIDEO2_TRANSPORT_NAME */

/* Exported types ----------------------------------------------------------- */
typedef enum
{
	VC1DECODER_CONTEXT_NOT_USED,      /* keep this one at the first one */
	VC1DECODER_CONTEXT_DECODE_FUNC,
/*    DECODER_CONTEXT_WAIT, */
	VC1DECODER_CONTEXT_CONFIRM_BUFFER_FUNC,
	VC1DECODER_CONTEXT_CONFIRM_BUFFER_EVT,
	VC1DECODER_CONTEXT_JOB_COMPLETED_EVT
} VC1DecoderContextState_t;

typedef struct VC1DecoderContext_s
{
	VC1DecoderContextState_t		State;
    U32                         CommandIdPreproc;
	U32							CommandIdParam;
	U32							CommandIdFmw;
    BOOL                        HasBeenRejected;
	DECODER_DecodingJobResults_t DECODER_DecodingJobResults;
    DECODER_DecodingJobResults_t DECODER_PreprocJobResults;
#if defined(VALID_TOOLS)
    void *  Address_p;  /* Allocated address of the total of the buffer */
#endif
} VC1DecoderContext_t;

typedef struct VC1DecoderPrivateData_s
{
    void *                  BitBufferAddress_p;
    U32                     BitBufferSize;
    VC1DecoderContextState_t    DecoderState;
	VC1DecoderContext_t 		DecoderContext/*[DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS]*/;
	DECODER_InformReadAddress_t DecoderReadAddress/*[DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS]*/;
	BOOL					DecoderIsRunning;
    MME_TransformerHandle_t TranformHandleDecoder;
    STEVT_EventID_t         RegisteredEventsID[VC1DECODER_NB_REGISTERED_EVENTS_IDS];
    DECODER_DecodingJobResults_t DECODER_DecodingJobResults;
    ST_Partition_t *        CPUPartition_p;
	VIDCOM_Task_t           DecoderTask;
    semaphore_t *           DecoderOrder_p;
    U32                     NextCommandId;
    U32                     CommandId;
    U32						CommandsData[DECODER_COMMANDS_BUFFER_SIZE];
    CommandsBuffer32_t    	CommandsBuffer;                 /* Data concerning the decode controller commands queue */
    MME_Command_t           CmdDecoder;
    VC9_CommandStatus_t     VC1CmdStatus;
    VC9_TransformParam_t    TransformParam;
#ifdef PRINT_DECODER_INPUT
	/* These structures are for avoiding the printing of*/
	/* the same GDC or PDC twice.*/
	VIDCOM_GlobalDecodingContextGenericData_t  PrevGenericGDC;
	VC1_GlobalDecodingContextSpecificData_t    PrevSpecificGDC;
	VC1_PictureDecodingContextSpecificData_t   PrevSpecificPDC;

	FILE *                  DebugFile;
#endif  /*PRINT_DECODER_INPUT */
    BOOL                                    IsEmbxTransportValid;
    EMBX_TRANSPORT                          EmbxTransport;
#ifdef STVID_DEBUG_GET_STATUS
	struct
    {
    	STVID_SequenceInfo_t * SequenceInfo_p;  /* Last sequence information, NULL if none or stopped */
    } Status;
#endif /* STVID_DEBUG_GET_STATUS */
} VC1DecoderPrivateData_t;

/* Exported Variables ------------------------------------------------------- */



/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
/* vc1decode.c module */
ST_ErrorCode_t VC1DECODER_GetState(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId, DECODER_Status_t * const DecoderStatus_p);
ST_ErrorCode_t VC1DECODER_GetCodecInfo(const DECODER_Handle_t DecoderHandle, DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication,  const STVID_CodecMode_t CodecMode);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VC1DECODER_GetStatistics(const DECODER_Handle_t DecoderHandle, STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t VC1DECODER_ResetStatistics(const DECODER_Handle_t DecoderHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t VC1DECODER_GetStatus(const DECODER_Handle_t DecoderHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
ST_ErrorCode_t VC1DECODER_Init(const DECODER_Handle_t DecoderHandle, const DECODER_InitParams_t * const InitParams_p);
ST_ErrorCode_t VC1DECODER_Reset(const DECODER_Handle_t DecoderHandle);
ST_ErrorCode_t VC1DECODER_DecodePicture(const DECODER_Handle_t DecoderHandle, const DECODER_DecodePictureParams_t * const DecodePictureParams_p, DECODER_Status_t * const DecoderStatus_p);
ST_ErrorCode_t VC1DECODER_ConfirmBuffer(const DECODER_Handle_t DecoderHandle, const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p);
ST_ErrorCode_t VC1DECODER_Abort(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId);
ST_ErrorCode_t VC1DECODER_Setup(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p);
ST_ErrorCode_t VC1DECODER_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t VC1DECODER_Term(const DECODER_Handle_t DecoderHandle);
#ifdef VALID_TOOLS
ST_ErrorCode_t vc1dec_RegisterTrace(void);
#endif /* VALID_TOOLS */
#ifdef ST_speed
void VC1DECODER_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode);
#ifdef STVID_TRICKMODE_BACKWARD
void VC1DECODER_SetDecodingDirection(const DECODER_Handle_t DecoderHandle, const DecodingDirection_t Direction);
#endif
#endif
ST_ErrorCode_t VC1DECODER_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize);
#if defined (DVD_SECURED_CHIP)
ST_ErrorCode_t VC1DECODER_SetupForH264PreprocWA_GNB42332(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* DVD_SECURED_CHIP */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifdef __VC1DECODER_H */

/* end of vc1decode.h */
