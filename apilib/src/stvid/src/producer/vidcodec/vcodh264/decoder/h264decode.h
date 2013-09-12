/*!
 ************************************************************************
 * \file h264decode.h
 *
 * \brief H264 decoder data structures and functions prototypes
 *
 * \author
 * Laurent Delaporte \n
 * CMG/STB \n
 * Copyright (C) 2005 STMicroelectronics
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __H264DECODER_H
#define __H264DECODER_H

/* Includes ----------------------------------------------------------------- */
#include "stos.h"
#include "sttbx.h"

#include "vcodh264.h"

#ifdef NATIVE_CORE
    #define H264_MME_VERSION 40
#else
    #define H264_MME_VERSION 42 /* To force use of H264 MME data structures V4.2 in H264_VideoTransformerTypes_Fmw.h */
#endif

#define MULTICOM    /* To force use of MULTICOM in H264_VideoTransformerTypes_Fmw.h */

#include "H264_VideoTransformerTypes_Fmw.h"
#include "H264_VideoTransformerTypes_Preproc.h"
#include "preprocessor.h"
#include "h264dumpmme.h"
#include "vid_com.h"
#include "vid_ctcm.h"
#include "stdevice.h"

#ifdef ST_XVP_ENABLE_FGT
#include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

#ifndef STUB_FMW_ST40
#include "mme.h"
#include "embx.h"
#endif /* STUB_FMW_ST40 */


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#if defined(STVID_VALID)
#define MME_MAX_DUMP 100
#endif

#if defined(TRACE_UART) && defined(VALID_TOOLS)
#define TRACE_H264DEC_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_CYAN | TRC_ATTR_BRIGHT)

#define H264DEC_TRACE_BLOCKID   (0x4015)    /* 0x4000 | STVID_DRIVER_ID */

/* !!!! WARNING !!!! : Any modification of this emun should be reported in emun in h264decoder.c */
enum
{
    H264DEC_TRACE_GLOBAL = 0,
    H264DEC_TRACE_START,
    H264DEC_TRACE_REJECTED_START,
    H264DEC_TRACE_PP_CALLBACK,
    H264DEC_TRACE_CONFIRM_BUFFER,
    H264DEC_TRACE_REJECTED_CB,
    H264DEC_TRACE_FMW_JOB_END,
    H264DEC_TRACE_FMW_CALLBACK,
    H264DEC_TRACE_FMW_ERRORS,
    H264DEC_TRACE_FB_ADDRESSES
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

/*! Decoder state */
#define DECODER_COMMANDS_BUFFER_SIZE 10

#ifdef SUPPORT_ONLY_SD_PROFILES
#define DECODER_INTERMEDIATE_BUFFER_SIZE                 572256
#else
#define DECODER_INTERMEDIATE_BUFFER_SIZE                1500000 /* TODO : need a GetCapability from the MME API, set from deltaphi top level spec 2.5 page 14  */
#endif
#define DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE       1096 /* TODO : need a GetCapability from the MME API, set from deltaphi top level spec 2.5 page 14  */

#ifdef STVID_NON_STD_INT_BUFFER_NUMBER /* To be used for debug or on Hardware emulator to save memory */
#define DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS               STVID_NON_STD_INT_BUFFER_NUMBER
#else /* STVID_NON_STD_INT_BUFFER_NUMBER */
#if (defined (ST_DELTAMU_HE) && defined(ST_DELTAMU_GREEN_HE))
#define DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS               5
#else
#define DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS               4 /* TODO : need a GetCapability from the MME API */
#endif /* not deltamu green */
#endif /* not STVID_ONE_INT_BUFFER */

#define MAX_SESB_SIZE                                      1096 /* TODO : found in Top Level Spec 2.5. This value is for low delay streams, for not low delay, max is 1096 */
                                                                /* It should be changed when FDMA firmware will return number of slices found in a picture */
#define POC_NOT_USED    0xABCDABCD

#ifndef MME_VIDEO_TRANSPORT_NAME
#define MME_VIDEO_TRANSPORT_NAME "ups"
#endif /* not MME_VIDEO_TRANSPORT_NAME */

#ifndef MME_VIDEO1_TRANSPORT_NAME
#define MME_VIDEO1_TRANSPORT_NAME "TransportVideo1"
#endif /* not MME_VIDEO1_TRANSPORT_NAME */

#ifndef MME_VIDEO2_TRANSPORT_NAME
#define MME_VIDEO2_TRANSPORT_NAME "TransportVideo2"
#endif /* not MME_VIDEO2_TRANSPORT_NAME */

/* events ID used for events registration and notification */
enum
{
    DECODER_INFORM_READ_ADDRESS_EVT_ID,
    DECODER_JOB_COMPLETED_EVT_ID,
    DECODER_CONFIRM_BUFFER_EVT_ID,
    H264DECODER_NB_REGISTERED_EVENTS_IDS  /* Keep this one as the last one */
};

/* Exported types ----------------------------------------------------------- */
typedef enum
{
    DECODER_COMMAND_NOT_USED,      /* keep this one at the first one */
    DECODER_COMMAND_START,
    DECODER_COMMAND_RESET,
    DECODER_COMMAND_CONFIRM_BUFFER_EVT,
    DECODER_COMMAND_CONFIRM_BUFFER_FUNC,
    DECODER_COMMAND_JOB_COMPLETED_EVT
} DecoderCommands_t;

typedef enum
{
    DECODER_CONTEXT_NOT_USED,      /* keep this one at the first one */
    DECODER_CONTEXT_WAITING_FOR_START,
    DECODER_CONTEXT_PREPROCESSING,
    DECODER_CONTEXT_CONFIRM_BUFFER_EVT,
    DECODER_CONTEXT_DECODING
} DecoderContextState_t;

typedef struct PreProcContext_s
{
    BOOL                            BufferUsed;
    U32                             CommandId;
    H264_CommandStatus_preproc_t    H264_CommandStatus_preproc;
    osclock_t                       CallBackTime;
    BOOL                            HasBeenRejected;
    U32                             ErrorCode;
} PreProcContext_t;

typedef struct FmwContext_s
{
    BOOL                            BufferUsed;
    U32                             CommandId;
    H264_CommandStatus_fmw_t        H264_CommandStatus_fmw;
    osclock_t                       CallBackTime;
} FmwContext_t;

typedef struct H264CompanionData_s
{
    MME_Command_t                       CmdInfoParam; /* used for MME_SET_GLOBAL_TRANSFORM_PARAMS command */
    BOOL                                GlobalParamSent; /* Used to avoid looking for SetGlobalParams CmdId at callback */
    H264_SetGlobalParam_t               h264GlobalParam;
    MME_Command_t                       CmdInfoFmw;   /* used for MME_TRANSFORM for Firmware commands */
    H264_TransformParam_fmw_t           h264TransformerFmwParam;
    H264_CommandStatus_fmw_t            H264_CommandStatus_fmw;
    BOOL SPS_HasChanged;        /* used to check if PPS&SPS have to be re-sent to the decoder */
    BOOL PPS_HasChanged;
    U32 PicParameterSetId;
    U32 SeqParameterSetId;

}
H264CompanionData_t;

typedef struct H264PreviousGlobalParam_s
{
    U32 PicParameterSetId;
    U32 SeqParameterSetId;
}
H264PreviousGlobalParam_t;

typedef struct H264DecoderData_s
{
    DecoderContextState_t           State;

    U32                             PictureNumber;          /* Added for Int. Buffers dump and debug */
    U32                             DecodingOrderFrameID;
    VIDCOM_PictureID_t              ExtendedPresentationOrderPictureID; /* Added for Int. Buffers dump and debug */
    BOOL                            IsFirstOfTwoFields;

    MME_CommandId_t                 CommandIdForProd;       /* CommandId sent back to producer */

    BOOL                            PreprocStartHasBeenRejected;

    GENERIC_Time_t                  PreprocJobSubmissionTime;
    H264PreprocCommand_t            H264PreprocCommand;     /* Data sent or received by Preproc transformer */
    DECODER_DecodingJobResults_t    DECODER_PreprocJobResults;

    BOOL                            ConfirmBufferHasBeenRejected;

    GENERIC_Time_t                  DecodeJobSubmissionTime;
    H264CompanionData_t             *H264CompanionData_p;   /* Data sent or received by Companion chip (LX) */
    DECODER_DecodingJobResults_t    DECODER_DecodingJobResults;
    DECODER_InformReadAddress_t     DecoderReadAddress;

    void *                          IntermediateBuffer_p;
} H264DecoderData_t;

typedef struct H264DecoderPrivateData_s
{
    void *                                  BitBufferAddress_p;
    U32                                     BitBufferSize;
    U32                                     NbBufferUsed;
    U32                                     IntermediateBufferSize;
    H264DecoderData_t                       H264DecoderData[DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS]; /* State and infos for on-going pictures */
    PreProcContext_t                        PreProcContext[DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS];  /* Info from Preproc callback received but not yet processed */
    FmwContext_t                            FmwContext[DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS * 2];  /* Info from FMW callback received but not yet processed */
    U32                                     RejectedPreprocStartCommand;
    U32                                     RejectedConfirmBufferCallback;
    U32                                     RejectedSendCommand;
    BOOL                                    DecoderIsRunning;
    BOOL                                    DecoderResetPending;
    H264PreprocHandle_t                     TranformHandlePreProc;
    MME_TransformerHandle_t                 TranformHandleDecoder;
    STEVT_EventID_t                         RegisteredEventsID[H264DECODER_NB_REGISTERED_EVENTS_IDS];
    STAVMEM_PartitionHandle_t               IntermediateBufferAvmemPartitionHandle;
    STAVMEM_BlockHandle_t                   IntermediateBufferHdl;
    void *                                  IntermediateBufferAddress_p;
    BOOL                                    H264CompanionDataSetupIsDone;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
#if !defined PERF1_TASK2
    VIDCOM_Task_t                           DecoderTask;
    semaphore_t *                           DecoderOrder_p;
#endif
    U32                                     CommandsData[DECODER_COMMANDS_BUFFER_SIZE];
    CommandsBuffer32_t                      CommandsBuffer;                 /* Data concerning the decode controller commands queue */
    semaphore_t *                           DataProtect_p;
    U32                                     NextCommandId;
#ifdef STVID_DEBUG_GET_STATISTICS
    STVID_Statistics_t                      Statistics;
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
	struct
    {
    	STVID_SequenceInfo_t * SequenceInfo_p;  /* Last sequence information, NULL if none or stopped */
		void * IntermediateBuffersAddress_p;
		U32    IntermediateBuffersSize;          /* in byte */
    } Status;
#endif /* STVID_DEBUG_GET_STATUS */
#if defined(STVID_VALID)
    U32                                     NbMMECmdDumped;
    H264CompanionData_t                     H264CompanionDataDump[MME_MAX_DUMP];
#endif
    H264PreviousGlobalParam_t               H264PreviousGlobalParam;
    BOOL                                    IsEmbxTransportValid;
    EMBX_TRANSPORT                          EmbxTransport;
#ifdef ST_speed
	U8			   							DecodingMode;
#ifdef STVID_TRICKMODE_BACKWARD
    DecodingDirection_t                     DecodingDirection;
#endif
#endif
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t                         FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */

} H264DecoderPrivateData_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define PushDecoderCommand(DECODER_Data_p, U32Var) vidcom_PushCommand32((CommandsBuffer32_t * )&(((H264DecoderPrivateData_t *)(DECODER_Data_p->PrivateData_p))->CommandsBuffer), (U32)U32Var)
#define PopDecoderCommand(DECODER_Data_p, U32Var_p) vidcom_PopCommand32((CommandsBuffer32_t * )&(((H264DecoderPrivateData_t *)(DECODER_Data_p->PrivateData_p))->CommandsBuffer), ((U32 *) (U32Var_p)))

/* Exported Functions ------------------------------------------------------- */
/* H264decode.c module */
ST_ErrorCode_t H264DECODER_GetState(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId, DECODER_Status_t * const DecoderStatus_p);
ST_ErrorCode_t H264DECODER_GetCodecInfo(const DECODER_Handle_t DecoderHandle, DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication, const STVID_CodecMode_t CodecMode);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t H264DECODER_GetStatistics(const DECODER_Handle_t DecoderHandle, STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t H264DECODER_ResetStatistics(const DECODER_Handle_t DecoderHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t H264DECODER_GetStatus(const DECODER_Handle_t DecoderHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
ST_ErrorCode_t H264DECODER_Init(const DECODER_Handle_t DecoderHandle, const DECODER_InitParams_t * const InitParams_p);
ST_ErrorCode_t H264DECODER_Reset(const DECODER_Handle_t DecoderHandle);
ST_ErrorCode_t H264DECODER_DecodePicture(const DECODER_Handle_t DecoderHandle, const DECODER_DecodePictureParams_t * const DecodePictureParams_p, DECODER_Status_t * const DecoderStatus_p);
ST_ErrorCode_t H264DECODER_ConfirmBuffer(const DECODER_Handle_t DecoderHandle, const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p);
ST_ErrorCode_t H264DECODER_Abort(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId);
ST_ErrorCode_t H264DECODER_Setup(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p);
ST_ErrorCode_t H264DECODER_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t H264DECODER_Term(const DECODER_Handle_t DecoderHandle);
#ifdef VALID_TOOLS
ST_ErrorCode_t h264dec_RegisterTrace(void);
#endif /* VALID_TOOLS */
#ifdef ST_speed
void H264DECODER_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode);
#ifdef STVID_TRICKMODE_BACKWARD
void H264DECODER_SetDecodingDirection(const DECODER_Handle_t DecoderHandle, const DecodingDirection_t Direction);
#endif
#endif
ST_ErrorCode_t H264DECODER_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize);
#if defined (DVD_SECURED_CHIP)
ST_ErrorCode_t H264DECODER_SetupForH264PreprocWA_GNB42332(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* DVD_SECURED_CHIP */

#if !defined PERF1_TASK2
/* task.c module */
ST_ErrorCode_t h264dec_StartdecoderTask(const DECODER_Handle_t decoderHandle);
ST_ErrorCode_t h264dec_StopDecoderTask(const DECODER_Handle_t DecoderHandle);
void h264dec_DecoderTaskFunc(const DECODER_Handle_t DecoderHandle);
#else
void h264dec_DecoderMainFunc(const DECODER_Handle_t DecoderHandle);
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifdef __H264DECODER_H */

/* end of h264decode.h */

