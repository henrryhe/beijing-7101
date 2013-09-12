#ifndef _MPG4P2DECODE_H_
#define _MPG4P2DECODE_H_

#include "stos.h"
#include "sttbx.h"      /* Must be placed after setting STTBX_PRINT and STTBX_REPORT */

#include "vid_com.h"
#include "vidcodec.h"
#include "vid_ctcm.h"
#include "stdevice.h"
#include "vcodmpg4p2.h"
#define MPEG4P2_MME_VERSION 11  /* To force use of MPEG4P2 MME data structures V1.1 in transformer_decoderTypes.h */
#include "transformer_decoderTypes.h"
#include "mme.h"
#include "embx.h"


#define NUM_MME_INPUT_BUFFERS	(MME_UINT)3   /* Compressed MPEG4P2 stream, internal forward ref, internal backward ref */
#define NUM_MME_OUTPUT_BUFFERS	(MME_UINT)3   /* Outputs (Luma, Chroma) and internal reconstruction address */

#define NUM_MME_DIVX_BUFFERS	(MME_UINT)(NUM_MME_INPUT_BUFFERS+NUM_MME_OUTPUT_BUFFERS)	/* MPEG4P2, REF[0,1,2]  */


typedef enum
{
    MPEG4P2DECODER_CONTEXT_NOT_USED,          /* keep this one at the first one */
    MPEG4P2DECODER_CONTEXT_DECODE_FUNC,
    MPEG4P2DECODER_CONTEXT_CONFIRM_BUFFER_FUNC
} MPEG4P2DecoderContextState_t;

/* Events ID used for events registration and notification  */
enum
{
    MPEG4P2DECODER_INFORM_READ_ADDRESS_EVT_ID,
    MPEG4P2DECODER_JOB_COMPLETED_EVT_ID,
    MPEG4P2DECODER_NB_REGISTERED_EVENTS_IDS   /* Keep this one as the last one */
};

#ifndef MME_VIDEO_TRANSPORT_NAME
#define MME_VIDEO_TRANSPORT_NAME "ups"
#endif

#ifndef MME_VIDEO1_TRANSPORT_NAME
#define MME_VIDEO1_TRANSPORT_NAME "TransportVideo1"
#endif  /* #ifndef MME_VIDEO1_TRANSPORT_NAME */

#ifndef MME_VIDEO2_TRANSPORT_NAME
#define MME_VIDEO2_TRANSPORT_NAME "TransportVideo2"
#endif  /* #ifndef MME_VIDEO2_TRANSPORT_NAME */


typedef struct MPEG4P2DecoderContext_s
{
	MPEG4P2DecoderContextState_t	State;
    U32                             CommandIdPreproc;
	U32							    CommandIdParam;
	U32							    CommandIdFmw;
    BOOL                            HasBeenRejected;
	DECODER_DecodingJobResults_t    DECODER_DecodingJobResults;
    DECODER_DecodingJobResults_t    DECODER_PreprocJobResults;
} MPEG4P2DecoderContext_t;

typedef struct MPEG4P2DecoderPrivateData_s
{
    void                              * BitBufferAddress_p;
    U32                                 BitBufferSize;
    MPEG4P2DecoderContextState_t        DecoderState;
	MPEG4P2DecoderContext_t 		    DecoderContext;
	DECODER_InformReadAddress_t         DecoderReadAddress;
    U32                                 NextCommandId;
    U32                                 CommandId;
    DECODER_Status_t                    DecoderStatus;
    ST_Partition_t                    * CPUPartition_p;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    MME_Command_t                       CmdDecoder;
    MME_DivXVideoDecodeParams_t         TransformParam;
    MME_TransformerHandle_t             TranformHandleDecoder;
    STEVT_EventID_t                     RegisteredEventsID[MPEG4P2DECODER_NB_REGISTERED_EVENTS_IDS];
    DECODER_DecodingJobResults_t        DECODER_DecodingJobResults;
    MME_DivXVideoDecodeReturnParams_t   ReturnParams;
    MME_DivXVideoDecodeInitParams_t     StoredInitParams;
    void                              * BitBuffer_p;
    void                              * OutputBuffer_p;
    void                              * ForwardReference_p;
    void                              * BackwardReference_p;
    MME_DivXVideoDecodeInitParams_t     DivxDecoderInitParams;
    MME_TransformerInitParams_t         InitParamsDecoder;
	char                                TransformerName[MME_MAX_TRANSFORMER_NAME];
	BOOL                                IsEmbxTransportValid;
	EMBX_TRANSPORT                      EmbxTransport;
#ifdef STVID_DEBUG_GET_STATUS
	struct
    {
    	STVID_SequenceInfo_t * SequenceInfo_p;  /* Last sequence information, NULL if none or stopped */
    } Status;
#endif /* STVID_DEBUG_GET_STATUS */
} MPEG4P2DecoderPrivateData_t;


void MPEG4P2_TransformerCallback_decoder (MME_Event_t       Event,
                                          MME_Command_t   * CallbackData,
                                          void            * UserData);

ST_ErrorCode_t MPEG4P2DECODER_Abort(const DECODER_Handle_t    DecoderHandle,
                                    const CommandId_t         CommandId);
ST_ErrorCode_t MPEG4P2DECODER_ConfirmBuffer(const DECODER_Handle_t DecoderHandle,
                                            const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p);
ST_ErrorCode_t MPEG4P2DECODER_DecodePicture (const DECODER_Handle_t                 DecoderHandle,
                                             const DECODER_DecodePictureParams_t *  const DecodePictureParams_p,
                                             DECODER_Status_t *                     const DecoderStatus_p);
ST_ErrorCode_t MPEG4P2DECODER_GetCodecInfo (const DECODER_Handle_t    DecoderHandle,
                                            DECODER_CodecInfo_t     * const CodecInfo_p,
                                            const U32 ProfileAndLevelIndication,
                                            const STVID_CodecMode_t CodecMode);
ST_ErrorCode_t MPEG4P2DECODER_GetState(const DECODER_Handle_t  DecoderHandle,
                                       const CommandId_t       CommandId,
                                       DECODER_Status_t        * const DecoderStatus_p);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t MPEG4P2DECODER_GetStatistics(const DECODER_Handle_t     DecoderHandle,
                                          STVID_Statistics_t       * const Statistics_p);
ST_ErrorCode_t MPEG4P2DECODER_ResetStatistics(const DECODER_Handle_t       DecoderHandle,
                                            const STVID_Statistics_t   * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t MPEG4P2DECODER_GetStatus(const DECODER_Handle_t     DecoderHandle,
                                        STVID_Status_t       * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
ST_ErrorCode_t MPEG4P2DECODER_Init(const DECODER_Handle_t       DecoderHandle,
                                   const DECODER_InitParams_t * const InitParams_p);
ST_ErrorCode_t MPEG4P2DECODER_Reset(const DECODER_Handle_t DecoderHandle);
ST_ErrorCode_t MPEG4P2DECODER_Setup(const DECODER_Handle_t DecoderHandle,
                                    const STVID_SetupParams_t * const SetupParams_p);
ST_ErrorCode_t MPEG4P2DECODER_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t MPEG4P2DECODER_Term(const DECODER_Handle_t DecoderHandle);
ST_ErrorCode_t MPEG4P2_UpdateTransformer(const DECODER_Handle_t       DecoderHandle,
                                         U32 FrameWidth, U32 FrameHeight, U32 CodecVersion);

#ifdef STUB_MPEG4P2_FMW_ST40
/* mpg4p2 stub functions */
ST_ErrorCode_t InitStub (MME_GenericCallback_t Callback);
ST_ErrorCode_t MMESTUB_InitTransformer(ST_DeviceName_t DeviceName, MME_TransformerInitParams_t *Params_p,
													MME_TransformerHandle_t	*Handle_p);
void TaskStub(void);
MME_ERROR MMESTUB_SendCommand(MME_TransformerHandle_t Handle, MME_Command_t *CmdInfo_p);
ST_ErrorCode_t MMESTUB_Init(ST_DeviceName_t Name);
ST_ErrorCode_t MMESTUB_Term(ST_DeviceName_t Name);
ST_ErrorCode_t MMESTUB_TermTransformer(MME_TransformerHandle_t Handle);
ST_ErrorCode_t MMESTUB_RegisterTrace(void);
#endif /* end of STUB_MPEG4P2_FMW_ST40 */

#ifdef ST_speed
void MPEG4P2DECODER_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode);
#ifdef STVID_TRICKMODE_BACKWARD
void MPEG4P2DECODER_SetDecodingDirection(const DECODER_Handle_t DecoderHandle, const DecodingDirection_t Direction);
#endif
#endif
ST_ErrorCode_t MPEG4P2DECODER_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize);
#if defined (DVD_SECURED_CHIP)
ST_ErrorCode_t MPEG4P2DECODER_SetupForH264PreprocWA_GNB42332(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* DVD_SECURED_CHIP */

#endif /* _MPG4P2DECODE_H_ */


