/*******************************************************************************

File name   : avsdecode.h

Description : AVS interface decoding  for MME type definitions.

COPYRIGHT (C) STMicroelectronics 2008

Date               Modification                                     Name
----               ------------                                     ----
29 January 2008    Created                                          PLE
*******************************************************************************/

#ifndef _AVSDECODE_H_
#define _AVSDECODE_H_

#include "stos.h"
#include "sttbx.h"      /* Must be placed after setting STTBX_PRINT and STTBX_REPORT */

#include "vid_com.h"
#include "vidcodec.h"
#include "vid_ctcm.h"
#include "stdevice.h"
#include "vcodavs.h"
#ifdef AVS_CODEC
#define AVS_MME_VERSION 10
#else
#define AVS_MME_VERSION 11  /* To force use of AVS MME data structures V1.1 in transformer_decoderTypes.h */
#endif /* AVS_CODEC */
#include "avs_transformer_decoderTypes.h"
#include "mme.h"
#include "embx.h"


#define NUM_MME_INPUT_BUFFERS   (MME_UINT)1  /* Compressed AVS stream, internal forward ref, internal backward ref */
/*#define NUM_MME_OUTPUT_BUFFERS    (MME_UINT)3  */ /* Outputs (Luma, Chroma) and internal reconstruction address */

#define NUM_MME_OUTPUT_BUFFERS  (MME_UINT)9   /* Outputs (Luma, Chroma) and internal reconstruction address */

#define NUM_MME_AVS_BUFFERS (MME_UINT)(NUM_MME_INPUT_BUFFERS+NUM_MME_OUTPUT_BUFFERS)    /* AVS, REF[0,1,2]  */


typedef enum
{
    AVSDECODER_CONTEXT_NOT_USED,          /* keep this one at the first one */
    AVSDECODER_CONTEXT_DECODE_FUNC,
    AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC
} AVSDecoderContextState_t;

/* Events ID used for events registration and notification  */
enum
{
    AVSDECODER_INFORM_READ_ADDRESS_EVT_ID,
    AVSDECODER_JOB_COMPLETED_EVT_ID,
    AVSDECODER_NB_REGISTERED_EVENTS_IDS   /* Keep this one as the last one */
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

typedef struct AVSDecoderContext_s
{
    AVSDecoderContextState_t    State;
    U32                             CommandIdPreproc;
    U32                             CommandIdParam;
    U32                             CommandIdFmw;
    BOOL                            HasBeenRejected;
    DECODER_DecodingJobResults_t    DECODER_DecodingJobResults;
    DECODER_DecodingJobResults_t    DECODER_PreprocJobResults;
} AVSDecoderContext_t;

typedef struct AVSDecoderPrivateData_s
{
    void                              * BitBufferAddress_p;
    U32                                 BitBufferSize;
    AVSDecoderContextState_t        DecoderState;
    AVSDecoderContext_t             DecoderContext;
    DECODER_InformReadAddress_t         DecoderReadAddress;
    U32                                 NextCommandId;
    U32                                 CommandId;
    DECODER_Status_t                    DecoderStatus;
    ST_Partition_t                    * CPUPartition_p;
    STAVMEM_BlockHandle_t               IntraMBStruct_Handle;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    MME_Command_t                       CmdDecoder;
    MME_AVSVideoDecodeParams_t         TransformParam;
    MME_TransformerHandle_t             TranformHandleDecoder;
    STEVT_EventID_t                     RegisteredEventsID[AVSDECODER_NB_REGISTERED_EVENTS_IDS];
    DECODER_DecodingJobResults_t        DECODER_DecodingJobResults;
    MME_AVSVideoDecodeReturnParams_t   ReturnParams;
    MME_AVSVideoDecodeInitParams_t     StoredInitParams;
    void                              * BitBuffer_p;
    void                              * OutputBuffer_p;
    void                              * ForwardReference_p;
    void                              * BackwardReference_p;
/*For AVS Release 2,Omeg format */
    void                              * Omeg2OutputLumaBuffer_p;
    void                              * Omeg2OutputChromaBuffer_p;
    void                              * Omeg2ForwardLumaReference_p;
    void                              * Omeg2ForwardChromaReference_p;
    void                              * Omeg2BackwardLumaReference_p;
    void                              * Omeg2BackwardChromaReference_p;
/*For Dedug, to keep the latest EndOffset */
    U32                               * LastEndOfffset;
    int                     CurrPicDistanceMsb;/**/
    int                     PicDistanceMsb;/**/
    int                     PrevPicDistanceLsb;/**/

    MME_AVSVideoDecodeInitParams_t      AvsDecoderInitParams;
    MME_TransformerInitParams_t         InitParamsDecoder;
    char                                TransformerName[MME_MAX_TRANSFORMER_NAME];
    BOOL                                IsEmbxTransportValid;
    EMBX_TRANSPORT                      EmbxTransport;
#ifdef STVID_DEBUG_GET_STATISTICS
  DecoderStats_t                 Statistics;
#endif
} AVSDecoderPrivateData_t;


void AVS_TransformerCallback_decoder (MME_Event_t       Event,
                                          MME_Command_t   * CallbackData,
                                          void            * UserData);

ST_ErrorCode_t AVSDECODER_Abort(const DECODER_Handle_t    DecoderHandle,
                                    const CommandId_t         CommandId);
ST_ErrorCode_t AVSDECODER_ConfirmBuffer(const DECODER_Handle_t DecoderHandle,
                                            const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p);
ST_ErrorCode_t AVSDECODER_DecodePicture (const DECODER_Handle_t                 DecoderHandle,
                                             const DECODER_DecodePictureParams_t *  const DecodePictureParams_p,
                                             DECODER_Status_t *                     const DecoderStatus_p);
/*ST_ErrorCode_t AVSDECODER_GetCodecInfo (const DECODER_Handle_t    DecoderHandle,
                                            DECODER_CodecInfo_t     * const CodecInfo_p,
                                            const U32 ProfileAndLevelIndication);

ST_ErrorCode_t AVSDECODER_GetCodecInfo(DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication, const STVID_CodecMode_t CodecMode);
*/
ST_ErrorCode_t AVSDECODER_GetCodecInfo (const DECODER_Handle_t    DecoderHandle,
                                            DECODER_CodecInfo_t     * const CodecInfo_p,
                                            const U32 ProfileAndLevelIndication,
                                            const STVID_CodecMode_t CodecMode);

ST_ErrorCode_t AVSDECODER_GetState(const DECODER_Handle_t  DecoderHandle,
                                       const CommandId_t       CommandId,
                                       DECODER_Status_t        * const DecoderStatus_p);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t AVSDECODER_GetStatistics(const DECODER_Handle_t     DecoderHandle,
                                          STVID_Statistics_t       * const Statistics_p);
ST_ErrorCode_t AVSDECODER_ResetStatistics(const DECODER_Handle_t       DecoderHandle,
                                            const STVID_Statistics_t   * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
ST_ErrorCode_t AVSDECODER_Init(const DECODER_Handle_t       DecoderHandle,
                                   const DECODER_InitParams_t * const InitParams_p);
ST_ErrorCode_t AVSDECODER_Reset(const DECODER_Handle_t DecoderHandle);
ST_ErrorCode_t AVSDECODER_Setup(const DECODER_Handle_t DecoderHandle,
                                    const STVID_SetupParams_t * const SetupParams_p);
ST_ErrorCode_t AVSDECODER_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t AVSDECODER_Term(const DECODER_Handle_t DecoderHandle);
ST_ErrorCode_t AVS_UpdateTransformer(const DECODER_Handle_t       DecoderHandle,
                                         U32 FrameWidth, U32 FrameHeight, U32 CodecVersion);

#ifdef STUB_AVS_FMW_ST40
/* mpg4p2 stub functions */
ST_ErrorCode_t InitStub (MME_GenericCallback_t Callback);
ST_ErrorCode_t MMESTUB_InitTransformer(ST_DeviceName_t DeviceName, MME_TransformerInitParams_t *Params_p,
                                                    MME_TransformerHandle_t *Handle_p);
void TaskStub(void);
MME_ERROR MMESTUB_SendCommand(MME_TransformerHandle_t Handle, MME_Command_t *CmdInfo_p);
ST_ErrorCode_t MMESTUB_Init(ST_DeviceName_t Name);
ST_ErrorCode_t MMESTUB_Term(ST_DeviceName_t Name);
ST_ErrorCode_t MMESTUB_TermTransformer(MME_TransformerHandle_t Handle);
ST_ErrorCode_t MMESTUB_RegisterTrace(void);
#endif /* end of STUB_AVS_FMW_ST40 */

#ifdef ST_speed
void AVSDECODER_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode);
#ifdef STVID_TRICKMODE_BACKWARD
void AVSDECODER_SetDecodingDirection(const DECODER_Handle_t DecoderHandle, const DecodingDirection_t Direction);
#endif /* STVID_TRICKMODE_BACKWARD */
#endif /* ST_speed */

ST_ErrorCode_t AVSDECODER_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize);

#endif /* _MPG4P2DECODE_H_ */


