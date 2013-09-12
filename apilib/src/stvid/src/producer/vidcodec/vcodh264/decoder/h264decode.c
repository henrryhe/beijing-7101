/*!
 ************************************************************************
 * \file h264decode.c
 *
 * \brief H264 decoder
 *
 * \author
 * Laurent Delaporte \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 ******************************************************************************
 */

/* Define to add debug info and traces */
/* Note : To add traces add STVID_DECODER_TRACE_UART in environment variables */

#ifdef STVID_DEBUG_DECODER
    #define STTBX_REPORT
#endif

/* #define H264DECODE_TRACE_VERBOSE */

#if defined(TRACE_DECODER)
#define TRACE_UART
#endif /* TRACE_DECODER */

#if defined TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#include <assert.h>
#endif

#include "h264decode.h"

/* Constants ---------------------------------------------------------------- */
#define STVID_ALLOW_VARIABLE_INTERMEDIATE_BUFFER_SIZE

#define MAX_TRY_FOR_MME_TERM_TRANSFORMER 10 /* max loop number for MME_TermTransformer completion waiting loop in DECODER_Term */

#if defined(TRACE_UART) && defined(VALID_TOOLS)

#define H264DEC_TRACE_BLOCKNAME "H264DEC"

/* !!!! WARNING !!!! : Any modification of this table should be reported in emun in h264decoder.h */
static TRACE_Item_t h264dec_TraceItemList[] =
{
    {"GLOBAL",          TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"START",           TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"REJECTED_START",  TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"PP_CALLBACK",     TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"CONFIRM_BUFFER",  TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"REJECTED_CB",     TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"FMW_JOB_END",     TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"FMW_CALLBACK",    TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"FMW_ERRORS",      TRACE_H264DEC_COLOR,                        TRUE, FALSE},
    {"FB_ADDRESSES",    TRACE_H264DEC_COLOR,                        TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

const DECODER_FunctionsTable_t DECODER_H264Functions =
{
    H264DECODER_GetState,
    H264DECODER_GetCodecInfo,
#ifdef STVID_DEBUG_GET_STATISTICS
    H264DECODER_GetStatistics,
    H264DECODER_ResetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    H264DECODER_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    H264DECODER_Init,
    H264DECODER_Reset,
    H264DECODER_DecodePicture,
    H264DECODER_ConfirmBuffer,
    H264DECODER_Abort,
    H264DECODER_Setup,
    H264DECODER_FillAdditionnalDataBuffer,
    H264DECODER_Term
#ifdef ST_speed
	,H264DECODER_SetDecodingMode
#ifdef STVID_TRICKMODE_BACKWARD
    ,H264DECODER_SetDecodingDirection
#endif
#endif
    ,H264DECODER_UpdateBitBufferParams
#if defined(DVD_SECURED_CHIP)
    ,H264DECODER_SetupForH264PreprocWA_GNB42332
#endif /* DVD_SECURED_CHIP */
};

#define IsFramePicture(Picture_p) ((Picture_p)->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
#define IsSecondOfTwoFields(Picture_p) (!(IsFramePicture(Picture_p)) && !((Picture_p)->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields))

static void h264_InitData(H264DecoderPrivateData_t * PrivateData_p);
static U8 h264dec_SearchPictureDescriptorByFrameIndex(H264_RefPictListAddress_t * InitialRefPictList_p, U8 PictureDescriptorRange, U8 iFrame, H264_PictureDescriptorType_t Interlace);
static ST_ErrorCode_t h264dec_SearchFrameInFullFrameList(VIDCOM_PictureGenericData_t * PictGenData_p, U32 iRef, H264_PictureSpecificData_t * PictSpecData_p, U8 *iFrame_p);
static BOOL PictureIsMBAFF (VIDCOM_ParsedPictureInformation_t * ParsedPicture_p);
static void TransformerCallback_preprocessor (H264PreprocCommand_t *command_p, void *UserData);
static void TransformerCallback_decoder (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);
static void h264dec_CopyPPS(VIDCOM_ParsedPictureInformation_t *  ParsedPicture_p, H264CompanionData_t * H264CompanionData_p);
static void h264dec_CopySPS(VIDCOM_ParsedPictureInformation_t * ParsedPicture_p, H264CompanionData_t * H264CompanionData_p);
static void h264dec_CreatePictureDescriptorFrame(H264_PictureDescriptor_t * PictureDescriptor_p, U32 FrameIndex, H264_SpecificReferenceList_t * SpecificReferenceList_p, VIDBUFF_FrameBuffer_t * ReferenceFrameBuff_p);
static void h264dec_CreatePictureDescriptorField(H264_PictureDescriptor_t * PictureDescriptor_p, U32 FrameIndex, H264_SpecificReferenceList_t * SpecificReferenceList_p, VIDCOM_ParsedPictureInformation_t * ParsedPicture_p, U32 FieldType);
static void h264dec_CreateNonExistingPictureDescriptorField(H264_PictureDescriptor_t * PictureDescriptor_p, U32 FrameIndex, H264_SpecificReferenceList_t * SpecificReferenceList_p, U32 FieldType);
static void h264dec_CreateNonExistingPictureDescriptor(H264_PictureDescriptor_t * PictureDescriptor_p, U32 FrameIndex, H264_SpecificReferenceList_t * FullReferenceTopFieldList_p, H264_SpecificReferenceList_t * FullReferenceBottomFieldList_p);
static void h264dec_CreatePictureDescriptor(H264_PictureDescriptor_t * PictureDescriptor_p, U32 FrameIndex, H264_SpecificReferenceList_t * SpecificReferenceList_p, VIDCOM_ParsedPictureInformation_t * ParsedPicture_p);

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        :
Description :
Parameters  : Event
 *            CallbackData
 *            UserData
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_GetStatistics(const DECODER_Handle_t DecoderHandle, STVID_Statistics_t * const Statistics_p)
{
    DECODER_Properties_t *          DECODER_Data_p;
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
    H264DecoderPrivateData_t *      PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    /* TODO update statistics with actual values in the decoder */
    Statistics_p->DecodeInterruptStartDecode  = -1;
    Statistics_p->DecodeInterruptPipelineIdle = -1;
    Statistics_p->DecodeInterruptDecoderIdle  = -1;
    Statistics_p->DecodeInterruptBitBufferEmpty = -1;
    Statistics_p->DecodeInterruptBitBufferFull  = -1;
    Statistics_p->DecodePbMaxNbInterruptSyntaxErrorPerPicture  = -1;
    Statistics_p->DecodePbInterruptSyntaxError  = -1;
    Statistics_p->DecodePbInterruptDecodeOverflowError  = -1;
    Statistics_p->DecodePbInterruptDecodeUnderflowError = -1;
    Statistics_p->DecodePbInterruptMisalignmentError = -1;
    Statistics_p->DecodePbInterruptQueueOverflow = -1;
    Statistics_p->DecodeGNBvd42696Error = PrivateData_p->Statistics.DecodeGNBvd42696Error;

    return(ErrorCode);
} /* end of H264DECODER_GetStatistics() */

/*******************************************************************************
Name        :
Description :
Parameters  : Event
 *            CallbackData
 *            UserData
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_ResetStatistics(const DECODER_Handle_t DecoderHandle, const STVID_Statistics_t * const Statistics_p)
{
    DECODER_Properties_t *          DECODER_Data_p;
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
    H264DecoderPrivateData_t *      PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (Statistics_p->DecodeGNBvd42696Error != 0)
    {
        PrivateData_p->Statistics.DecodeGNBvd42696Error  = 0;
    }

    return(ErrorCode);
} /* end of H264DECODER_ResetStatistics() */
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
/*******************************************************************************
Name        :
Description :
Parameters  : Event
 *            CallbackData
 *            UserData
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_GetStatus(const DECODER_Handle_t DecoderHandle, STVID_Status_t * const Status_p)
{
    DECODER_Properties_t *          DECODER_Data_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    H264DecoderPrivateData_t *      PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    /* Update status with actual values in the decoder */
	Status_p->SequenceInfo_p                = PrivateData_p->Status.SequenceInfo_p;
	Status_p->IntermediateBuffersAddress_p  = PrivateData_p->Status.IntermediateBuffersAddress_p;
	Status_p->IntermediateBuffersSize       = PrivateData_p->Status.IntermediateBuffersSize;

    return(ErrorCode);
} /* end of H264DECODER_GetStatus() */
#endif /* STVID_DEBUG_GET_STATUS */
/*******************************************************************************
Name        :
Description :
Parameters  :
 *
 *
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_GetState(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId, DECODER_Status_t * const DecoderStatus_p)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;

    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(CommandId);
    UNUSED_PARAMETER(DecoderStatus_p);

    return(ErrorCode);
} /* end of H264DECODER_GetState() */

/*******************************************************************************
Name        :
Description :
Parameters  :
 *
 *
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_GetCodecInfo(const DECODER_Handle_t DecoderHandle, DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication, const STVID_CodecMode_t CodecMode)
{
    ST_ErrorCode_t  ErrorCode        = ST_NO_ERROR;
    U32             LevelIdc;

    UNUSED_PARAMETER(DecoderHandle);

    CodecInfo_p->MaximumDecodingLatencyInSystemClockUnit = 9000; /* System Clock = 90kHz */

    if (CodecMode == STVID_CODEC_MODE_DECODE)
    {
        /* PPB size. 160 bytes/MB for picture with Max number of motion vectors per two consecutive MBs > 16
          (level 3.0 & below) and 40 bytes/MB for pictures with Max number of motion vectors per two consecutive MBs <= 16(level 3.1 and above) */
        /* WARNING: due to BUG GNBvd37128 64 is the mimimum to choose for HD instead of 40 */

        LevelIdc = ProfileAndLevelIndication & 0xFF;
        if ((ProfileAndLevelIndication & 0xFF) < 31)
        {
            CodecInfo_p->FrameBufferAdditionalDataBytesPerMB     = 160 ;
        }
        else
        {
            CodecInfo_p->FrameBufferAdditionalDataBytesPerMB     = 64;
        }

    }
    else
    {
        /* Transcoding Mode not investigated yet */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    CodecInfo_p->DecodePictureToConfirmBufferMaxTime     = 160; /* TODO : Which unit !!! Here it is set to 4Vsync in ms unit */

    return(ErrorCode);
} /* end of H264DECODER_GetCodecInfo() */

/*******************************************************************************
Name        :
Description :
Parameters  :
 *
 *
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_Reset(const DECODER_Handle_t DecoderHandle)
{
    DECODER_Properties_t *          DECODER_Data_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    H264DecoderPrivateData_t *      PrivateData_p = NULL;
    U32                             LoopCounter;
    U32                             Command;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;

    if ((DECODER_Data_p == NULL) || (DECODER_Data_p->PrivateData_p == NULL))
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        PrivateData_p = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
        STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
        PrivateData_p->DecoderResetPending = TRUE;
        for (LoopCounter = 0; LoopCounter < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS; LoopCounter++)
        {
            if (PrivateData_p->H264DecoderData[LoopCounter].State == DECODER_CONTEXT_PREPROCESSING)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER_Reset (Call H264Preproc_AbortCommand) for JobID=%d", PrivateData_p->H264DecoderData[LoopCounter].CommandIdForProd));
                ErrorCode = H264Preproc_AbortCommand(PrivateData_p->TranformHandlePreProc, &PrivateData_p->H264DecoderData[LoopCounter].H264PreprocCommand);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264Preproc_AbortCommand) : No current job to abort"));
                }
            }
        }
        STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);

    }

    Command = DECODER_COMMAND_RESET;
    PushDecoderCommand(DECODER_Data_p, Command);
#if !defined PERF1_TASK2
    STOS_SemaphoreSignal(PrivateData_p->DecoderOrder_p);
#else
    h264dec_DecoderMainFunc(DecoderHandle);
#endif
    return(ErrorCode);
} /* end of H264DECODER_Reset() */

/*******************************************************************************
Name        :
Description :
Parameters  :
 *
 *
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_Abort(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    H264DecoderPrivateData_t *      PrivateData_p;
	U32								iBuff;
    BOOL                            IsBufferFound;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

	iBuff = 0;
	IsBufferFound = FALSE;
	do
	{
        IsBufferFound = (PrivateData_p->H264DecoderData[iBuff].CommandIdForProd == CommandId);
		if (!IsBufferFound)
		{
			iBuff++;
		}
	}
	while ((iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS) && !IsBufferFound);

    if (!IsBufferFound)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_Abort) : Unknown Command Id %d ", CommandId));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        return(ErrorCode/* TODO */);
    }

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_Abort) : DECODER_Abort %d %d-%d/%d",
            PrivateData_p->H264DecoderData[iBuff].CommandIdForProd,
            PrivateData_p->H264DecoderData[iBuff].DecodingOrderFrameID,
            PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.IDExtension,
            PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.ID));

    if (PrivateData_p->H264DecoderData[iBuff].State == DECODER_CONTEXT_PREPROCESSING)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER_Abort (Call H264Preproc_AbortCommand) for JobID=%d", PrivateData_p->H264DecoderData[iBuff].CommandIdForProd));
        ErrorCode = H264Preproc_AbortCommand(PrivateData_p->TranformHandlePreProc, &PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264Preproc_AbortCommand) : No current job to abort"));
            ErrorCode = ST_NO_ERROR; /* TODO, what to do else in this case ??? We have to finish the job... */
        }
    }

    /* TODO : protected with semaphores ?? */
    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
    if (PrivateData_p->NbBufferUsed > 0)
    {
        PrivateData_p->NbBufferUsed--;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_Abort) : PrivateData_p->NbBufferUsed was already == 0"));
        PrivateData_p->NbBufferUsed = 0;
    }
    PrivateData_p->H264DecoderData[iBuff].State = DECODER_CONTEXT_NOT_USED;
    if (PrivateData_p->H264DecoderData[iBuff].PreprocStartHasBeenRejected)
    {
        if (PrivateData_p->RejectedPreprocStartCommand > 0)
        {
            PrivateData_p->RejectedPreprocStartCommand--;
        }
    }
    PrivateData_p->H264DecoderData[iBuff].PreprocStartHasBeenRejected = FALSE;
    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
    /* TODO : Actually stop preprocessor or firmware depending on current status */
#ifdef H264DECODE_TRACE_VERBOSE
	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_Abort) : Decode aborted Command Id %d ", CommandId));
#endif
    return(ErrorCode/* TODO */);
} /* end of H264DECODER_Abort() */

/*******************************************************************************
Name        : h264dec_SearchPictureDescriptorByFrameIndex
Description : Search the picture descriptor that points to a given frame index
            : This function also looks for the correct interlace.
Parameters  : H264_RefPictListAddress_t InitialRefPictList
            : U8 PictureDescriptorRange
            : U8 iFrame
            : H264_PictureDescriptorType_t Interlace
Assumptions :
Limitations :
Returns     : Index in Picture Descriptor table
*******************************************************************************/
static U8 h264dec_SearchPictureDescriptorByFrameIndex(H264_RefPictListAddress_t * InitialRefPictList_p, U8 PictureDescriptorRange, U8 iFrame, H264_PictureDescriptorType_t Interlace)
{
    BOOL IsPictureDescriptorFound;
    U8 PictureDescriptorIndex;
    U8 PictureDescriptorIndexFound;

    IsPictureDescriptorFound = FALSE;
    PictureDescriptorIndexFound = 0;
    for(PictureDescriptorIndex = 0; PictureDescriptorIndex < PictureDescriptorRange; PictureDescriptorIndex++)
    {
        if (InitialRefPictList_p->PictureDescriptor[PictureDescriptorIndex].FrameIndex == iFrame)
        {
            if ((Interlace == H264_PIC_DESCRIPTOR_FRAME) ||(Interlace == H264_PIC_DESCRIPTOR_AFRAME))
            {
                IsPictureDescriptorFound = TRUE;
                PictureDescriptorIndexFound = PictureDescriptorIndex;
                break;
            }
            else if (InitialRefPictList_p->PictureDescriptor[PictureDescriptorIndex].HostData.DescriptorType == Interlace)
            {
                IsPictureDescriptorFound = TRUE;
                PictureDescriptorIndexFound = PictureDescriptorIndex;
                break;
            }
        }
    }
    return(PictureDescriptorIndexFound);
} /* end of h264dec_SearchPictureDescriptorByFrameIndex() */

/*******************************************************************************
Name        : h264dec_SearchFrameInFullFrameList
Description : Search a frame in the list of reference (P0, B0 or B1)
Parameters  : VIDCOM_PictureGenericData_t * PictGenData_p
            : U32 iRef
Assumptions :
Limitations :
Returns     : Frame index in full frame reference list
*******************************************************************************/
static ST_ErrorCode_t h264dec_SearchFrameInFullFrameList(VIDCOM_PictureGenericData_t * PictGenData_p,
                                      U32 iRef,
                                      H264_PictureSpecificData_t * PictSpecData_p,
                                      U8 *iFrame_p)
{
    BOOL IsDecOrderFrameIDFound;
#ifdef STVID_DEBUG_DECODER
    BOOL ExpectingAtLeastOneRefFrameButNot1stOf2fields;
#endif
    U8 iFrame;
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;

    IsDecOrderFrameIDFound = FALSE;
#ifdef STVID_DEBUG_DECODER
    ExpectingAtLeastOneRefFrameButNot1stOf2fields = FALSE;
#endif /* STVID_DEBUG_DECODER */
    for (iFrame = 0; iFrame < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES && !IsDecOrderFrameIDFound ; iFrame++)
    {
        if (PictGenData_p->IsValidIndexInReferenceFrame[iFrame] || PictSpecData_p->FullReferenceTopFieldList[iFrame].IsNonExisting)
        {
#ifdef STVID_DEBUG_DECODER
            if (PictGenData_p->FullReferenceFrameList[iFrame] != PictGenData_p->DecodingOrderFrameID)
            {
                ExpectingAtLeastOneRefFrameButNot1stOf2fields = TRUE;
            }
#endif /* STVID_DEBUG_DECODER */
            if (PictGenData_p->FullReferenceFrameList[iFrame] == iRef)
            {
                IsDecOrderFrameIDFound = TRUE;
                break;
            }
        }
    }
    if (!IsDecOrderFrameIDFound)
    {
#ifdef STVID_DEBUG_DECODER
        if (ExpectingAtLeastOneRefFrameButNot1stOf2fields)
        {
            /* Don't report error message if reference list of picture has been patched by the producer (case of I only decode for instance) */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_ConfirmBuffer) : Index not found for RefPicList"));
        }
#endif /* STVID_DEBUG_DECODER */
        *iFrame_p = 0;
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        *iFrame_p = iFrame;
    }
    return(ErrorCode);
} /* end of h264dec_SearchFrameInFullFrameList() */

/*******************************************************************************
Name        :
Description :
Parameters  :
 *
 *
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL PictureIsMBAFF (VIDCOM_ParsedPictureInformation_t * ParsedPicture_p)
{
    H264_GlobalDecodingContextSpecificData_t    * GDCSpecData_p;

    GDCSpecData_p = (H264_GlobalDecodingContextSpecificData_t *) ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    return(GDCSpecData_p->mb_adaptive_frame_field_flag == 1);
} /* end of PictureIsMBAFF() */

/*******************************************************************************
Name        :
Description :
Parameters  : Event
 *            CallbackData
 *            UserData
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void TransformerCallback_preprocessor (H264PreprocCommand_t *command_p, void *UserData)
{
    DECODER_Properties_t         *DECODER_Data_p;
    H264DecoderPrivateData_t     *PrivateData_p;
    U32                          iPP;
    U32                          Command;
    BOOL                         IsBufferFree;

    DECODER_Data_p  = (DECODER_Properties_t *) UserData;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (PrivateData_p->DecoderResetPending)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (TransformerCallback_preprocessor) : Reset pending Ignore Command Id %d", command_p->CmdId));
        /* No error handling in decoder. Producer will not receive the end of this preprocessing job and will abort the decode after its preproc timeout has triggered */
        return;
    }

	iPP = 0;
	IsBufferFree = FALSE;
	do
	{
		IsBufferFree = !PrivateData_p->PreProcContext[iPP].BufferUsed;
		if (!IsBufferFree)
		{
			iPP++;
		}
	}
	while ((iPP < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS) && !IsBufferFree);

    if (!IsBufferFree)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (TransformerCallback_preprocessor) : No room for decode context storage, Command Id %d ", command_p->CmdId));
        /* Force to use any valid entry to avoid to access out of table bounds, why not index 0 ? */
        iPP = 0;
    }

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_PP_CALLBACK, "dPP_JC%d-c%d ", command_p->CmdId, iPP));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
    PrivateData_p->PreProcContext[iPP].BufferUsed = TRUE;
    PrivateData_p->PreProcContext[iPP].HasBeenRejected = FALSE;
    PrivateData_p->PreProcContext[iPP].H264_CommandStatus_preproc = command_p->CommandStatus_preproc;
    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
 	PrivateData_p->PreProcContext[iPP].CallBackTime = time_now();
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    if (command_p->CommandStatus_preproc.IntPictBufferEnd_p - command_p->CommandStatus_preproc.IntSESBBegin_p > DECODER_INTERMEDIATE_BUFFER_SIZE ||
            command_p->CommandStatus_preproc.IntPictBufferEnd_p == 0)
	{
        TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_PP_CALLBACK, "dBUG_BUG_IPBE=%d/ISESB=%d ", command_p->CommandStatus_preproc.IntPictBufferEnd_p, command_p->CommandStatus_preproc.IntSESBBegin_p));
    }
#endif
    PrivateData_p->PreProcContext[iPP].CommandId = command_p->CmdId;
    PrivateData_p->PreProcContext[iPP].ErrorCode = command_p->Error;
    Command = DECODER_COMMAND_CONFIRM_BUFFER_EVT;
    Command += (U32)(iPP) << 8;
/*    TraceBuffer(("P0%d03 ", iPP)); */
    PushDecoderCommand(DECODER_Data_p, Command);
#if !defined PERF1_TASK2
    /* Callback may be called while Decoder is terminating, so protect usage of the semaphore as it could be already deleted */
    if (PrivateData_p->DecoderOrder_p != NULL)
    {
        STOS_SemaphoreSignal(PrivateData_p->DecoderOrder_p);
    }
#else
    h264dec_DecoderMainFunc( (DECODER_Handle_t) DECODER_Data_p);
#endif


} /* end of TransformerCallback_preprocessor() */

/*******************************************************************************
Name        : TransformerCallback_decoder
Description :
Parameters  : Event
 *            CallbackData
 *            UserData
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void TransformerCallback_decoder (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{
    MME_Command_t               *command_p;
    DECODER_Properties_t *      DECODER_Data_p;
    H264DecoderPrivateData_t *  PrivateData_p;
    H264_CommandStatus_fmw_t *  H264_CommandStatus_fmw_p;
    U32                         iFmw;
	U32							Command;
	BOOL						IsBufferFree;

    UNUSED_PARAMETER(Event);

    DECODER_Data_p  = (DECODER_Properties_t *) UserData;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    command_p       = (MME_Command_t *) CallbackData;
    H264_CommandStatus_fmw_p = (H264_CommandStatus_fmw_t *) command_p->CmdStatus.AdditionalInfo_p;

    if (PrivateData_p->DecoderResetPending)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (TransformerCallback_decoder) : Reset pending Ignore Command Id %d", command_p->CmdStatus.CmdId));
        /* No error handling in decoder. Producer will not receive the end of this preprocessing job and will abort the decode after its preproc timeout has triggered */
        return;
    }

	iFmw = 0;
	IsBufferFree = FALSE;
	do
	{
		IsBufferFree = !PrivateData_p->FmwContext[iFmw].BufferUsed;
		if (!IsBufferFree)
		{
			iFmw++;
		}
	}
	while ((iFmw < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS * 2) && !IsBufferFree);

    if (!IsBufferFree)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (TransformerCallback_decoder) : No room for decode context storage, Command Id %d ", command_p->CmdStatus.CmdId));
        /* Force to use any valid entry to avoid to access out of table bounds, why not index 0 ? */
        iFmw = 0;
    }

#if defined(TRACE_UART) && defined(VALID_TOOLS)
     TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_CALLBACK, "dFMW_JC%d-c%d ", command_p->CmdStatus.CmdId, iFmw));
#endif

    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
    PrivateData_p->FmwContext[iFmw].BufferUsed = TRUE;
    PrivateData_p->FmwContext[iFmw].CallBackTime = time_now();
    if ((H264_CommandStatus_fmw_t *) command_p->CmdStatus.AdditionalInfoSize != 0)
    {
        PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw = *H264_CommandStatus_fmw_p;
    }
    PrivateData_p->FmwContext[iFmw].CommandId = command_p->CmdStatus.CmdId;
    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);

    Command = DECODER_COMMAND_JOB_COMPLETED_EVT;
	Command += (U32)(iFmw) << 8;
/*    TraceBuffer(("D0%d04 ", iFmw)); */
	PushDecoderCommand(DECODER_Data_p, Command);
#if !defined PERF1_TASK2
    /* Callback may be called while Decoder is terminating, so protect usage of the semaphore as it could be already deleted */
    if (PrivateData_p->DecoderOrder_p != NULL)
    {
        STOS_SemaphoreSignal(PrivateData_p->DecoderOrder_p);
    }
#else
    h264dec_DecoderMainFunc( (DECODER_Handle_t) DECODER_Data_p);
#endif

} /* end of TransformerCallback_decoder() */

/*******************************************************************************
Name        : h264dec_CopyScalingLists
Description :
Parameters  : ParsedPicture_p
 *            CurrentPPS_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CopyScalingLists(VIDCOM_ParsedPictureInformation_t *  ParsedPicture_p,
                                        H264_ScalingList_t                *CurrentScalingList_p)
{
    H264_SpecificScalingMatrix_t   *ScalingMatrix_p;
    U32 ScalingListNumber;
    U32 CoefNumber;

    ScalingMatrix_p  = &(((H264_PictureDecodingContextSpecificData_t *)(ParsedPicture_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p))->ScalingMatrix);
    CurrentScalingList_p->ScalingListUpdated = 1;  /* TODO Set only if ScalingMatrix different from previous picture */

    for(ScalingListNumber = 0 ; ScalingListNumber < 6 ; ScalingListNumber++)
    {
        for(CoefNumber = 0 ; CoefNumber < 16 ; CoefNumber++)
        {
            CurrentScalingList_p->FirstSixScalingList[ScalingListNumber][CoefNumber] = ScalingMatrix_p->FirstSixScalingList[ScalingListNumber][CoefNumber];
        }
    }
    for(ScalingListNumber = 0 ; ScalingListNumber < 2 ; ScalingListNumber++)
    {
        for(CoefNumber = 0 ; CoefNumber < 64 ; CoefNumber++)
        {
            CurrentScalingList_p->NextTwoScalingList[ScalingListNumber][CoefNumber] = ScalingMatrix_p->NextTwoScalingList[ScalingListNumber][CoefNumber];
        }
    }
} /* end of h264dec_CopyScalingLists() */

/*******************************************************************************
Name        : h264dec_CopyPPS
Description :
Parameters  : ParsedPicture_p
 *            CurrentPPS_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CopyPPS(VIDCOM_ParsedPictureInformation_t *  ParsedPicture_p, H264CompanionData_t * H264CompanionData_p)
{
    H264_PictureDecodingContextSpecificData_t   *PDCSpecData_p;
    H264_SetGlobalParamPPS_t * CurrentPPS_p = &(H264CompanionData_p->h264GlobalParam.H264SetGlobalParamPPS);

    PDCSpecData_p  = ParsedPicture_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p;

    if (PDCSpecData_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CopyPPS) : Null param passed"));
    }

    H264CompanionData_p->PPS_HasChanged = PDCSpecData_p->PPS_HasChanged;
    PDCSpecData_p->PPS_HasChanged = FALSE;   /* reset it to FALSE until next PPS is parsed */
    H264CompanionData_p->PicParameterSetId = PDCSpecData_p->pic_parameter_set_id;

    CurrentPPS_p->entropy_coding_mode_flag      = PDCSpecData_p->entropy_coding_mode_flag;
    CurrentPPS_p->pic_order_present_flag        = PDCSpecData_p->pic_order_present_flag;
    CurrentPPS_p->num_ref_idx_l0_active_minus1  = PDCSpecData_p->num_ref_idx_l0_active_minus1;
    CurrentPPS_p->num_ref_idx_l1_active_minus1  = PDCSpecData_p->num_ref_idx_l1_active_minus1;
    CurrentPPS_p->weighted_pred_flag            = PDCSpecData_p->weighted_pred_flag;
    CurrentPPS_p->weighted_bipred_idc           = PDCSpecData_p->weighted_bipred_idc;
    CurrentPPS_p->pic_init_qp_minus26           = PDCSpecData_p->pic_init_qp_minus26;
    CurrentPPS_p->chroma_qp_index_offset        = PDCSpecData_p->chroma_qp_index_offset;
    CurrentPPS_p->deblocking_filter_control_present_flag = PDCSpecData_p->deblocking_filter_control_present_flag;
    CurrentPPS_p->constrained_intra_pred_flag   = PDCSpecData_p->constrained_intra_pred_flag;
    CurrentPPS_p->transform_8x8_mode_flag       = PDCSpecData_p->transform_8x8_mode_flag;
    CurrentPPS_p->second_chroma_qp_index_offset = PDCSpecData_p->second_chroma_qp_index_offset;

    h264dec_CopyScalingLists(ParsedPicture_p, &CurrentPPS_p->ScalingList);
} /* end of h264dec_CopyPPS() */

/*******************************************************************************
Name        : h264dec_CopySPS
Description :
Parameters  : ParsedPicture_p
 *            CurrentSPS_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CopySPS(VIDCOM_ParsedPictureInformation_t *  ParsedPicture_p, H264CompanionData_t * H264CompanionData_p)
{
    H264_GlobalDecodingContextSpecificData_t * GDCSpecData_p;
    STVID_SequenceInfo_t * SequenceInfo_p;
    H264_SetGlobalParamSPS_t * CurrentSPS_p = &(H264CompanionData_p->h264GlobalParam.H264SetGlobalParamSPS);

    GDCSpecData_p  = ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    SequenceInfo_p  = &(ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo);

    if ((GDCSpecData_p == NULL) || (SequenceInfo_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CopySPS) : Null param passed"));
    }

    H264CompanionData_p->SPS_HasChanged = GDCSpecData_p->SPS_HasChanged;
    GDCSpecData_p->SPS_HasChanged = FALSE; /* reset it to FALSE until next SPS is parsed */
    H264CompanionData_p->SeqParameterSetId = GDCSpecData_p->seq_parameter_set_id;

    CurrentSPS_p->level_idc                         = SequenceInfo_p->ProfileAndLevelIndication & 0xFF;
    CurrentSPS_p->log2_max_frame_num_minus4         = GDCSpecData_p->log2_max_frame_num_minus4;
    CurrentSPS_p->pic_order_cnt_type                = GDCSpecData_p->pic_order_cnt_type;
    CurrentSPS_p->log2_max_pic_order_cnt_lsb_minus4 = GDCSpecData_p->log2_max_pic_order_cnt_lsb_minus4;
    CurrentSPS_p->delta_pic_order_always_zero_flag  = GDCSpecData_p->delta_pic_order_always_zero_flag;
    CurrentSPS_p->pic_width_in_mbs_minus1           = SequenceInfo_p->Width/16 - 1;
    CurrentSPS_p->pic_height_in_map_units_minus1    = SequenceInfo_p->Height/(16 * (2 - GDCSpecData_p->frame_mbs_only_flag)) -1;
    CurrentSPS_p->frame_mbs_only_flag               = GDCSpecData_p->frame_mbs_only_flag;
    CurrentSPS_p->mb_adaptive_frame_field_flag      = GDCSpecData_p->mb_adaptive_frame_field_flag;
    CurrentSPS_p->direct_8x8_inference_flag         = GDCSpecData_p->direct_8x8_inference_flag;
#if defined(ST_7100) && defined(ST_CUT_1_X)
    CurrentSPS_p->DecoderProfileConformance         = H264_MAIN_PROFILE;
#else
    CurrentSPS_p->DecoderProfileConformance         = H264_HIGH_PROFILE;
#endif
    CurrentSPS_p->chroma_format_idc                 = GDCSpecData_p->chroma_format_idc;
} /* end of h264dec_CopySPS() */

/*******************************************************************************
Name        : h264dec_CreatePictureDescriptorFrame
Description : Create one PictureDescriptor that will be sent to the Firmware
 *            Current picture is FRAME and Reference picture is FIELD
 * 			  --> we need to access to the 2 fields to find the correct POCs
Parameters  : PictureDescriptor_p : pointer on Picture Descriptor to be created
 *            FrameIndex :  Index in FullReferenceFrameList array
 *            SpecificReferenceList_p : contains data that need to be sent to
 * 										the firmware
 *            ParsedPicture_p : pointer on the parsed picture
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CreatePictureDescriptorFrame(
		 H264_PictureDescriptor_t *     PictureDescriptor_p,
         U32                            FrameIndex,
         H264_SpecificReferenceList_t * SpecificReferenceList_p,
		 VIDBUFF_FrameBuffer_t         *ReferenceFrameBuff_p)
{
    VIDCOM_PictureGenericData_t  * PictGenData_p1;
    H264_PictureSpecificData_t * PictSpecData_p1;
    VIDCOM_PictureGenericData_t  * PictGenData_p2;
    H264_PictureSpecificData_t * PictSpecData_p2;
	VIDCOM_ParsedPictureInformation_t * ParsedPicture_p1, * ParsedPicture_p2;

	ParsedPicture_p1 = &ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation;
    PictGenData_p1    = &(ParsedPicture_p1->PictureGenericData);
    PictSpecData_p1   = ParsedPicture_p1->PictureSpecificData_p;
    if (ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p != NULL)
	{
		ParsedPicture_p2 = &ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation;
    	PictGenData_p2    = &(ParsedPicture_p2->PictureGenericData);
    	PictSpecData_p2   = ParsedPicture_p2->PictureSpecificData_p;
        if ((ParsedPicture_p2 == NULL) || (PictGenData_p2 == NULL) || (PictSpecData_p2 == NULL))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptorFrame) : Null param passed"));
        }
	}
    else
    {
        PictSpecData_p2  = NULL;
	}

    /* Error recovery: Use current picture as reference */
    if (PictSpecData_p2 == NULL)
    {
        PictSpecData_p2  = PictSpecData_p1;
    }

    PictureDescriptor_p->HostData.PictureNumber     = SpecificReferenceList_p->PictureNumber;
	if (PictGenData_p1->PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
	{
	    PictureDescriptor_p->HostData.PicOrderCntTop    = PictSpecData_p1->PicOrderCntTop;
        if (PictSpecData_p2 != NULL)
	    {
	       PictureDescriptor_p->HostData.PicOrderCntBot    = PictSpecData_p2->PicOrderCntBot;
        }
        else
	    {
	       PictureDescriptor_p->HostData.PicOrderCntBot    = 0;
        }
	}
    else
    {
        if (PictSpecData_p2 != NULL)
	    {
    	    PictureDescriptor_p->HostData.PicOrderCntTop    = PictSpecData_p2->PicOrderCntTop;
        }
        else
	    {
    	    PictureDescriptor_p->HostData.PicOrderCntTop    = 0;
        }
	    PictureDescriptor_p->HostData.PicOrderCntBot    = PictSpecData_p1->PicOrderCntBot;
	}

	if (PictureDescriptor_p->HostData.PicOrderCntTop < PictureDescriptor_p->HostData.PicOrderCntBot)
	{
		PictureDescriptor_p->HostData.PicOrderCnt = PictureDescriptor_p->HostData.PicOrderCntTop;
	}
	else
	{
		PictureDescriptor_p->HostData.PicOrderCnt = PictureDescriptor_p->HostData.PicOrderCntBot;
	}
    PictureDescriptor_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FRAME;

	/* TODO : if Field1 is not IDR and Field2 is IDR, what shall we do ??? */
	PictureDescriptor_p->HostData.IdrFlag = PictSpecData_p1->IsIDR;
    if (SpecificReferenceList_p->IsLongTerm)
    {
        PictureDescriptor_p->HostData.ReferenceType = H264_LONG_TERM_REFERENCE;
    }
    else
    {
        PictureDescriptor_p->HostData.ReferenceType = H264_SHORT_TERM_REFERENCE;
    }
    PictureDescriptor_p->HostData.NonExistingFlag = SpecificReferenceList_p->IsNonExisting;
    PictureDescriptor_p->FrameIndex = FrameIndex;
} /* end of h264dec_CreatePictureDescriptorFrame() */

/*******************************************************************************
Name        : h264dec_CreatePictureDescriptorField
Description : Create one PictureDescriptor that will be sent to the Firmware
 *            Current picture is FIELD and Reference picture is FRAME
Parameters  : PictureDescriptor_p : pointer on Picture Descriptor to be created
 *            FrameIndex :  Index in FullReferenceFrameList array
 *            SpecificReferenceList_p : contains data that need to be sent to
 * 										the firmware
 *            ParsedPicture_p : pointer on the parsed picture
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CreatePictureDescriptorField(H264_PictureDescriptor_t *     PictureDescriptor_p,
                                U32                                 FrameIndex,
                                H264_SpecificReferenceList_t *      SpecificReferenceList_p,
                                VIDCOM_ParsedPictureInformation_t * ParsedPicture_p, U32 FieldType)
{
    VIDCOM_PictureGenericData_t  * PictGenData_p;
    H264_PictureSpecificData_t * PictSpecData_p;

    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;
    if ((PictGenData_p == NULL) || (PictSpecData_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptorField) : Null param passed"));
    }

    PictureDescriptor_p->HostData.PictureNumber     = SpecificReferenceList_p->PictureNumber;
    PictureDescriptor_p->HostData.DescriptorType = FieldType;
	switch (FieldType)
	{
		case H264_PIC_DESCRIPTOR_FIELD_TOP :
    		PictureDescriptor_p->HostData.PicOrderCntTop    = PictSpecData_p->PicOrderCntTop;
    		PictureDescriptor_p->HostData.PicOrderCntBot    = POC_NOT_USED;
    		PictureDescriptor_p->HostData.PicOrderCnt       = PictSpecData_p->PicOrderCntTop;
			break;

		case H264_PIC_DESCRIPTOR_FIELD_BOTTOM:
    		PictureDescriptor_p->HostData.PicOrderCntBot    = PictSpecData_p->PicOrderCntBot;
    		PictureDescriptor_p->HostData.PicOrderCntTop    = POC_NOT_USED;
    		PictureDescriptor_p->HostData.PicOrderCnt       = PictSpecData_p->PicOrderCntBot;
			break;
		default :
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptorField) : Unknown Picture Structure %d ", FieldType));
			break;
	}
	PictureDescriptor_p->HostData.IdrFlag = PictSpecData_p->IsIDR;
    if (SpecificReferenceList_p->IsLongTerm)
    {
        PictureDescriptor_p->HostData.ReferenceType = H264_LONG_TERM_REFERENCE;
    }
    else
    {
        PictureDescriptor_p->HostData.ReferenceType = H264_SHORT_TERM_REFERENCE;
    }
    PictureDescriptor_p->HostData.NonExistingFlag = SpecificReferenceList_p->IsNonExisting;
    PictureDescriptor_p->FrameIndex = FrameIndex;
} /* end of h264dec_CreatePictureDescriptorField() */

/*******************************************************************************
Name        : h264dec_CreateNonExistingPictureDescriptorField
Description : Create one PictureDescriptor that will be sent to the Firmware
 *            Current picture is FIELD and Reference picture is FRAME
Parameters  : PictureDescriptor_p : pointer on Picture Descriptor to be created
 *            FrameIndex :  Index in FullReferenceFrameList array
 *            SpecificReferenceList_p : contains data that need to be sent to
 * 										the firmware
 *            ParsedPicture_p : pointer on the parsed picture
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CreateNonExistingPictureDescriptorField(H264_PictureDescriptor_t *     PictureDescriptor_p,
                                U32                                 FrameIndex,
                                H264_SpecificReferenceList_t *      SpecificReferenceList_p,
                                U32 FieldType)
{
    PictureDescriptor_p->HostData.PictureNumber     = SpecificReferenceList_p->PictureNumber;
    PictureDescriptor_p->HostData.DescriptorType    = FieldType;
    PictureDescriptor_p->HostData.PicOrderCnt       = SpecificReferenceList_p->NonExistingPicOrderCnt;
	switch (FieldType)
	{
		case H264_PIC_DESCRIPTOR_FIELD_TOP :
    		PictureDescriptor_p->HostData.PicOrderCntTop    = SpecificReferenceList_p->NonExistingPicOrderCnt;
    		PictureDescriptor_p->HostData.PicOrderCntBot    = POC_NOT_USED;
			break;

		case H264_PIC_DESCRIPTOR_FIELD_BOTTOM:
    		PictureDescriptor_p->HostData.PicOrderCntBot    = SpecificReferenceList_p->NonExistingPicOrderCnt;
    		PictureDescriptor_p->HostData.PicOrderCntTop    = POC_NOT_USED;
			break;
		default :
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreateNonExistingPictureDescriptorField) : Unknown Picture Structure %d ", FieldType));
			break;
	}
	PictureDescriptor_p->HostData.IdrFlag = FALSE;
    PictureDescriptor_p->HostData.ReferenceType = H264_SHORT_TERM_REFERENCE;
    PictureDescriptor_p->HostData.NonExistingFlag = SpecificReferenceList_p->IsNonExisting;
    PictureDescriptor_p->FrameIndex = FrameIndex;
} /* end of h264dec_CreateNonExistingPictureDescriptorField() */

/*******************************************************************************
Name        : h264dec_CreateNonExistingPictureDescriptor
Description : Create a PictureDescriptor for non-existing reference frame
              that will be sent to the Firmware
Parameters  : PictureDescriptor_p : pointer on Picture Descriptor to be created
 *            FrameIndex :  Index in FullReferenceFrameList array
 *            SpecificReferenceList_p : contains data that need to be sent to
 * 										the firmware
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CreateNonExistingPictureDescriptor(H264_PictureDescriptor_t *     PictureDescriptor_p,
                                U32                                 FrameIndex,
                                H264_SpecificReferenceList_t *      FullReferenceTopFieldList_p,
                                H264_SpecificReferenceList_t *      FullReferenceBottomFieldList_p)
{
    PictureDescriptor_p->HostData.PictureNumber     = FullReferenceTopFieldList_p->PictureNumber;
    PictureDescriptor_p->HostData.PicOrderCntTop    = FullReferenceTopFieldList_p->NonExistingPicOrderCnt;
    PictureDescriptor_p->HostData.PicOrderCntBot    = FullReferenceBottomFieldList_p->NonExistingPicOrderCnt;
    /* Non existing pictures are Frame. For a frame: PicOrderCnt = min (PicOrderCntTop, PicOrderCntBot) */
    if (PictureDescriptor_p->HostData.PicOrderCntTop < PictureDescriptor_p->HostData.PicOrderCntBot)
    {
        PictureDescriptor_p->HostData.PicOrderCnt       = PictureDescriptor_p->HostData.PicOrderCntTop;
    }
    else
    {
        PictureDescriptor_p->HostData.PicOrderCnt       = PictureDescriptor_p->HostData.PicOrderCntBot;
    }

    PictureDescriptor_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FRAME;

    PictureDescriptor_p->HostData.IdrFlag = FALSE;
    PictureDescriptor_p->HostData.ReferenceType = H264_SHORT_TERM_REFERENCE;
    PictureDescriptor_p->HostData.NonExistingFlag = FullReferenceTopFieldList_p->IsNonExisting;
    PictureDescriptor_p->FrameIndex = FrameIndex;
} /* end of h264dec_CreateNonExistingPictureDescriptor() */

/*******************************************************************************
Name        : h264dec_CreatePictureDescriptor
Description : Create one PictureDescriptor that will be sent to the Firmware
Parameters  : PictureDescriptor_p : pointer on Picture Descriptor to be created
 *            FrameIndex :  Index in FullReferenceFrameList array
 *            SpecificReferenceList_p : contains data that need to be sent to
 * 										the firmware
 *            ParsedPicture_p : pointer on the parsed picture
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264dec_CreatePictureDescriptor(H264_PictureDescriptor_t *     PictureDescriptor_p,
                                U32                                 FrameIndex,
                                H264_SpecificReferenceList_t *      SpecificReferenceList_p,
                                VIDCOM_ParsedPictureInformation_t * ParsedPicture_p)
{
    VIDCOM_PictureGenericData_t  * PictGenData_p;
    H264_PictureSpecificData_t * PictSpecData_p;

    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;
    if ((PictGenData_p == NULL) || (PictSpecData_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptor) : Null param passed"));
    }

    PictureDescriptor_p->HostData.PictureNumber     = SpecificReferenceList_p->PictureNumber;
    PictureDescriptor_p->HostData.PicOrderCntTop    = PictSpecData_p->PicOrderCntTop;
    PictureDescriptor_p->HostData.PicOrderCntBot    = PictSpecData_p->PicOrderCntBot;
    if (PictGenData_p->PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
    {
        PictureDescriptor_p->HostData.PicOrderCnt       = PictureDescriptor_p->HostData.PicOrderCntTop;
    }
    else if (PictGenData_p->PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
    {
        PictureDescriptor_p->HostData.PicOrderCnt       = PictureDescriptor_p->HostData.PicOrderCntBot;
    }
    else
    {
        /* For a frame: PicOrderCnt = min (PicOrderCntTop, PicOrderCntBot) */
        if (PictureDescriptor_p->HostData.PicOrderCntTop < PictureDescriptor_p->HostData.PicOrderCntBot)
        {
            PictureDescriptor_p->HostData.PicOrderCnt       = PictureDescriptor_p->HostData.PicOrderCntTop;
        }
        else
        {
            PictureDescriptor_p->HostData.PicOrderCnt       = PictureDescriptor_p->HostData.PicOrderCntBot;
        }
    }

    switch (PictGenData_p->PictureInfos.VideoParams.PictureStructure)
    {
        case  STVID_PICTURE_STRUCTURE_TOP_FIELD :
            PictureDescriptor_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FIELD_TOP;
            break;
        case  STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
            PictureDescriptor_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FIELD_BOTTOM;
            break;
        case  STVID_PICTURE_STRUCTURE_FRAME :
            if (PictureIsMBAFF (ParsedPicture_p))
                PictureDescriptor_p->HostData.DescriptorType =  H264_PIC_DESCRIPTOR_AFRAME;
            else
                PictureDescriptor_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FRAME;
            break;
        default :
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptor) : Unknown Picture Structure %d ", PictGenData_p->PictureInfos.VideoParams.PictureStructure));
            break;
    }
    PictureDescriptor_p->HostData.IdrFlag = PictSpecData_p->IsIDR;
    if (SpecificReferenceList_p->IsLongTerm)
    {
        PictureDescriptor_p->HostData.ReferenceType = H264_LONG_TERM_REFERENCE;
    }
    else
    {
        PictureDescriptor_p->HostData.ReferenceType = H264_SHORT_TERM_REFERENCE;
    }
    PictureDescriptor_p->HostData.NonExistingFlag = SpecificReferenceList_p->IsNonExisting;
    PictureDescriptor_p->FrameIndex = FrameIndex;
} /* end of h264dec_CreatePictureDescriptor() */

/*******************************************************************************
Name        : H264DECODER_DecodePicture
Description :
Parameters  : DecoderHandle
 *            DecodePictureParams_p
 *            DecoderStatus_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_DecodePicture (const DECODER_Handle_t         DecoderHandle,
                                          const DECODER_DecodePictureParams_t *  const DecodePictureParams_p,
                                          DECODER_Status_t *       const DecoderStatus_p)
{
    ST_ErrorCode_t                  ST_ErrorCode = ST_NO_ERROR;
    U32                                         iBuff;
    U32                                         Command;
    DECODER_Properties_t            *DECODER_Data_p;
    H264_TransformParam_fmw_t       *fmw_p;
    VIDCOM_ParsedPictureInformation_t *ParsedPicture_p;
    H264_PictureSpecificData_t      *PictSpecData_p;
    VIDCOM_PictureGenericData_t     *PictGenData_p;
    H264DecoderPrivateData_t        *PrivateData_p;
    H264_PictureDecodingContextSpecificData_t  *PDCSpecData_p;
    H264_GlobalDecodingContextSpecificData_t   *GDCSpecData_p;
    STVID_DecimationFactor_t        DecimationFactors;
    STVID_SequenceInfo_t            *SequenceInfo_p;
	BOOL							 IsBufferFree;
    BOOL                                        AreTherePendingStart;
#if !defined PERF1_TASK3
    BOOL                                        IsPreprocBusy;
#endif
    H264CompanionData_t                        *H264CompanionData_p;
    STVID_SetupParams_t             SetupParams;

    if ((DecoderHandle == NULL) || (DecodePictureParams_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_DecodePicture) : DecoderHandle %x  DecodePictureParams_p %x ", DecoderHandle, DecodePictureParams_p));
        return(ST_ERROR_BAD_PARAMETER);
    }

	/* Pointers initialisation */
    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    ParsedPicture_p = &(DecodePictureParams_p->PictureInformation->ParsedPictureInformation);
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;
    PDCSpecData_p   = ParsedPicture_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p;
    GDCSpecData_p   = ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);
    SequenceInfo_p  = &(ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo);

#ifdef STVID_DEBUG_GET_STATUS
	PrivateData_p->Status.SequenceInfo_p = SequenceInfo_p;
#endif /* STVID_DEBUG_GET_STATUS */

    PrivateData_p->DecoderResetPending = FALSE;

    /* check whether the intermediate buffer have been allocated */
    if (!PrivateData_p->H264CompanionDataSetupIsDone)
    {
        /* try to allocate intermediate buffers in InitParams_p->AvmemPartitionHandle partition */
        SetupParams.SetupObject = STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION;
        SetupParams.SetupSettings.AVMEMPartition = DECODER_Data_p->AvmemPartitionHandle;
        ST_ErrorCode = H264DECODER_Setup(DecoderHandle, &SetupParams);
        if (ST_ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_DecodePicture) : DecoderHandle %x  H264DECODER_Setup failed", DecoderHandle));
            return(ST_ERROR_NO_MEMORY);
        }
    }

    /* check whether the previous command has been rejected by Transformer */
    if (PrivateData_p->RejectedPreprocStartCommand != 0)
    {
        DecoderStatus_p->State = DECODER_STATE_DECODING;
        DecoderStatus_p->IsJobSubmitted = FALSE;
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = time_now();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_DecodePicture) : Previous picture waiting for Preproc hardware"));
        return(ST_ERROR_NO_MEMORY);
    }

    iBuff = 0;
    AreTherePendingStart = FALSE;
    do
    {
        AreTherePendingStart = (PrivateData_p->H264DecoderData[iBuff].State == DECODER_CONTEXT_WAITING_FOR_START);
        if (!AreTherePendingStart)
        {
            iBuff++;
        }
    }
    while (!AreTherePendingStart && (iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS));

    /* check whether preproc hardware is busy */
#if !defined PERF1_TASK3
    H264Preproc_IsHardwareBusy(PrivateData_p->TranformHandlePreProc, &IsPreprocBusy);
    if (IsPreprocBusy || AreTherePendingStart)
#else
	if (AreTherePendingStart)
#endif
    {
        DecoderStatus_p->State = DECODER_STATE_DECODING;
        DecoderStatus_p->IsJobSubmitted = FALSE;
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = time_now();
#ifdef H264DECODE_TRACE_VERBOSE
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_DecodePicture) : Preprocessor is busy"));
#endif /* H264DECODE_TRACE_VERBOSE */
        return(ST_ERROR_NO_MEMORY);
    }

    if (PrivateData_p->RejectedPreprocStartCommand != 0)
    {
        DecoderStatus_p->State = DECODER_STATE_DECODING;
        DecoderStatus_p->IsJobSubmitted = FALSE;
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = time_now();
#ifdef H264DECODE_TRACE_VERBOSE
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_DecodePicture) : Previous picture waiting for Preproc hardware"));
#endif /* H264DECODE_TRACE_VERBOSE */
        return(ST_ERROR_NO_MEMORY);
    }

    /* check whether an intermediate buffer is available */
    if (PrivateData_p->NbBufferUsed >= DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS)
    {
        DecoderStatus_p->State = DECODER_STATE_DECODING;
        DecoderStatus_p->IsJobSubmitted = FALSE;
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = time_now();
#ifdef H264DECODE_TRACE_VERBOSE
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_DecodePicture) : no intermediate buffer available (NbBufferUsed=%d)", PrivateData_p->NbBufferUsed));
#endif /* H264DECODE_TRACE_VERBOSE */
        return(ST_ERROR_NO_MEMORY);
    }

	iBuff = 0;
	IsBufferFree = FALSE;
	do
	{
        IsBufferFree = (PrivateData_p->H264DecoderData[iBuff].State == DECODER_CONTEXT_NOT_USED);
		if (!IsBufferFree)
		{
			iBuff++;
		}
	}
    while ((iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS) && !IsBufferFree);

    if (!IsBufferFree)
    {
        DecoderStatus_p->State = DECODER_STATE_DECODING;
        DecoderStatus_p->IsJobSubmitted = FALSE;
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = time_now();
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_DecodePicture) : no intermediate buffer available"));
        return(ST_ERROR_NO_MEMORY);
    }

    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
    H264CompanionData_p = PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p;

	/* TODO : protected with semaphore ??? */
    PrivateData_p->NbBufferUsed++;
    PrivateData_p->H264DecoderData[iBuff].PreprocStartHasBeenRejected = FALSE;
    PrivateData_p->H264DecoderData[iBuff].State = DECODER_CONTEXT_WAITING_FOR_START;
    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);

/* DecodingOrderFrameID */
	PrivateData_p->H264DecoderData[iBuff].DecodingOrderFrameID = PictGenData_p->DecodingOrderFrameID;
	PrivateData_p->H264DecoderData[iBuff].IsFirstOfTwoFields = PictGenData_p->IsFirstOfTwoFields;

/* ExtendedPresentationOrderPictureID (Added for Intermediate buffer dump and debug */
	PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.IDExtension = PictGenData_p->ExtendedPresentationOrderPictureID.IDExtension;
	PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.ID = PictGenData_p->ExtendedPresentationOrderPictureID.ID;
  	PrivateData_p->H264DecoderData[iBuff].PictureNumber = DecodePictureParams_p->PictureInformation->PictureNumber;

    DecoderStatus_p->DecodingJobStatus.JobSubmissionTime = time_now();

    /* PreProcessing command */
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.BitBufferBeginAddress_p      = (U32 *) PrivateData_p->BitBufferAddress_p;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.BitBufferEndAddress_p        = (U32 *) ((U32)PrivateData_p->BitBufferAddress_p + (U32)PrivateData_p->BitBufferSize - 1);
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStartAddrBitBuffer_p  = (U32 *) PictGenData_p->BitBufferPictureStartAddress;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStopAddrBitBuffer_p   = (U32 *) PictGenData_p->BitBufferPictureStopAddress;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.IntSliceErrStatusBufferBegin_p= PrivateData_p->H264DecoderData[iBuff].IntermediateBuffer_p;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.IntPictureBufferBegin_p      = (U32 *)((U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.IntSliceErrStatusBufferBegin_p + DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE);
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.IntBufferStop_p              = (U32 *)((U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.IntPictureBufferBegin_p + PrivateData_p->IntermediateBufferSize);
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.constrained_intra_pred_flag  = PDCSpecData_p->constrained_intra_pred_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.pic_init_qp                  = PDCSpecData_p->pic_init_qp_minus26+26;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.num_ref_idx_l1_active_minus1 = PDCSpecData_p->num_ref_idx_l1_active_minus1;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.num_ref_idx_l0_active_minus1 = PDCSpecData_p->num_ref_idx_l0_active_minus1;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.deblocking_filter_control_present_flag = PDCSpecData_p->deblocking_filter_control_present_flag;
    if (PDCSpecData_p->weighted_bipred_idc == 1)
     {
      PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.weighted_bipred_idc_flag = 1;
     }
    else
     {
      PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.weighted_bipred_idc_flag = 0;
     }
    /*PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.weighted_bipred_idc_flag     = PDCSpecData_p->weighted_bipred_idc;*/
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.weighted_pred_flag           = PDCSpecData_p->weighted_pred_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.delta_pic_order_always_zero_flag = GDCSpecData_p->delta_pic_order_always_zero_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.pic_order_present_flag       = PDCSpecData_p->pic_order_present_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.pic_order_cnt_type           = GDCSpecData_p->pic_order_cnt_type;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.frame_mbs_only_flag          = GDCSpecData_p->frame_mbs_only_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.entropy_coding_mode_flag     = PDCSpecData_p->entropy_coding_mode_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.mb_adaptive_frame_field_flag = GDCSpecData_p->mb_adaptive_frame_field_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.transform_8x8_mode_flag      = PDCSpecData_p->transform_8x8_mode_flag;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.direct_8x8_inference_flag    = GDCSpecData_p->direct_8x8_inference_flag;
    if (GDCSpecData_p->chroma_format_idc == 0)
    {
        PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.monochrome                   = 1;
    }
    else
    {
        PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.monochrome                   = 0;
    }
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureWidthInMbs            = SequenceInfo_p->Width/16;
    if (PictGenData_p->PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
        PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.nb_MB_in_picture_minus1      = (SequenceInfo_p->Width * SequenceInfo_p->Height)/256 - 1;
	}
    else
    {
        PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.nb_MB_in_picture_minus1      = (SequenceInfo_p->Width * SequenceInfo_p->Height / 2)/256 - 1;
	}
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.log2_max_pic_order_cnt_lsb   = GDCSpecData_p->log2_max_pic_order_cnt_lsb_minus4 + 4;
    PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.log2_max_frame_num           = GDCSpecData_p->log2_max_frame_num_minus4 + 4;

    /* Firmware Command preparation (send in ConfirmBuffer function call) */
    fmw_p = &H264CompanionData_p->h264TransformerFmwParam;

    /* H264_TransformParam_fmw_t structure preparation */
    h264dec_CopySPS(ParsedPicture_p, H264CompanionData_p);
    h264dec_CopyPPS(ParsedPicture_p, H264CompanionData_p);

    /* TODO : MainAuxEnable management */
    fmw_p->MainAuxEnable = MAINOUT_EN;

    /* TODO : DecodingMode management */
    fmw_p->DecodingMode = H264_NORMAL_DECODE;
#ifdef ST_speed
	if ((PrivateData_p->DecodingMode == DEGRADED_BSLICES) && (ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B))
	{
		fmw_p->DecodingMode = H264_DOWNGRADED_DECODE_LEVEL1;
	}
	/*if ((PrivateData_p->DecodingMode == DEGRADED_PSLICES) && (ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P))
	{
		fmw_p->DecodingMode = H264_DOWNGRADED_DECODE_LEVEL1;
	} */
#endif

    fmw_p->AdditionalFlags = 0;

    DecimationFactors = STVID_DECIMATION_FACTOR_NONE;
    if (DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p != NULL)
    {
        DecimationFactors = DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;
    }

    fmw_p->HorizontalDecimationFactor   = HDEC_1;
    if (DecimationFactors & STVID_DECIMATION_FACTOR_H2)
    {
/*        fmw_p->HorizontalDecimationFactor   = HDEC_2;*/
        fmw_p->HorizontalDecimationFactor   = HDEC_ADVANCED_2;
        DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation=TRUE;
    }
    if (DecimationFactors & STVID_DECIMATION_FACTOR_H4)
    {
/*        fmw_p->HorizontalDecimationFactor   = HDEC_4;*/
        fmw_p->HorizontalDecimationFactor   = HDEC_ADVANCED_4;
        DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation=TRUE;
    }
    fmw_p->VerticalDecimationFactor     = VDEC_1;
    if (DecimationFactors & STVID_DECIMATION_FACTOR_V2)
    {
        fmw_p->VerticalDecimationFactor   = VDEC_2_PROG; /* TODO : not clear !!!! */
        fmw_p->VerticalDecimationFactor   = VDEC_2_INT;
    }

    fmw_p->HostData.PicOrderCntTop      = PictSpecData_p->PreDecodingPicOrderCntTop;
    fmw_p->HostData.PicOrderCntBot      = PictSpecData_p->PreDecodingPicOrderCntBot;
    fmw_p->HostData.PicOrderCnt         = PictSpecData_p->PreDecodingPicOrderCnt;
    fmw_p->HostData.IdrFlag             = PictSpecData_p->IsIDR;
    switch (PictGenData_p->PictureInfos.VideoParams.PictureStructure)
    {
        case  STVID_PICTURE_STRUCTURE_TOP_FIELD :
            fmw_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FIELD_TOP;
            break;
        case  STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
            fmw_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FIELD_BOTTOM;
            break;
        case  STVID_PICTURE_STRUCTURE_FRAME :
            fmw_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FRAME;
            if (PictureIsMBAFF (ParsedPicture_p))
            {
                fmw_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_AFRAME;
            }
            else
            {
                fmw_p->HostData.DescriptorType = H264_PIC_DESCRIPTOR_FRAME;
            }
            break;
        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_DecodePicture) : Unknown Picture Structure %d", PictGenData_p->PictureInfos.VideoParams.PictureStructure));
            break;
    }
    /* not used at this stage */
	if (PictGenData_p->IsReference)
	{
		fmw_p->HostData.ReferenceType       = H264_SHORT_TERM_REFERENCE;
	}
	else
	{
		fmw_p->HostData.ReferenceType       = H264_UNUSED_FOR_REFERENCE;
	}
    fmw_p->HostData.NonExistingFlag     = FALSE;
    fmw_p->HostData.PictureNumber       = 0;
    fmw_p->PictureStartAddrBinBuffer    = NULL;
    fmw_p->PictureStopAddrBinBuffer     = NULL;
    fmw_p->MaxSESBSize                  = 0;

    PrivateData_p->H264DecoderData[iBuff].CommandIdForProd = PrivateData_p->NextCommandId++;

    /* Data used for bit buffer read/write synchronization with inject */
    PrivateData_p->H264DecoderData[iBuff].DecoderReadAddress.DecoderReadAddress_p = (U32 *) PictGenData_p->BitBufferPictureStopAddress;
    PrivateData_p->H264DecoderData[iBuff].DecoderReadAddress.CommandId            = PrivateData_p->H264DecoderData[iBuff].CommandIdForProd;

	DecoderStatus_p->State = DECODER_STATE_DECODING;
	DecoderStatus_p->IsJobSubmitted = TRUE;
    DecoderStatus_p->CommandId = PrivateData_p->H264DecoderData[iBuff].CommandIdForProd;
	DecoderStatus_p->DecodingJobStatus.JobState = DECODER_JOB_PREPROCESSING;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_START, "dDECStart%d-b%d ",DecoderStatus_p->CommandId, iBuff));
#endif

    Command = DECODER_COMMAND_START;
	Command += (U32)(iBuff) << 8;
/*    TraceBuffer(("S0%d01 ", iBuff)); */
    PushDecoderCommand(DECODER_Data_p, Command);
#ifdef H264DECODE_TRACE_VERBOSE
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_DecodePicture) : Buffer %d  State START_FUNC IDpp %d ", iBuff, DecoderStatus_p->CommandId));
#endif
#if !defined PERF1_TASK2
    STOS_SemaphoreSignal(PrivateData_p->DecoderOrder_p);
#else
    h264dec_DecoderMainFunc(DecoderHandle);
#endif

    return(ST_ErrorCode);
} /* end of H264DECODER_DecodePicture() */

/*******************************************************************************
Name        : H264DECODER_ConfirmBuffer
Description :
Parameters  : DecoderHandle
 *            ConfirmBufferParams_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_ConfirmBuffer(const DECODER_Handle_t DecoderHandle,
                                     const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p)
{
    DECODER_Properties_t *              DECODER_Data_p;
    H264DecoderPrivateData_t *          PrivateData_p;
    U32                                 i;
    U32                                 iPictDesc;
    U32                                 iRef;
    U8                                  iFrame;
    U32                                 Command;
    U32                                 iBuff;
    ST_ErrorCode_t                      ST_ErrorCode     = ST_NO_ERROR;
    ST_ErrorCode_t                      RefErrorCode;
    STVID_DecimationFactor_t            DecimationFactors;
    H264_TransformParam_fmw_t          *fmw_p;
    H264_ReferenceFrameAddress_t       *RefFrameAddress_p;
    VIDBUFF_FrameBuffer_t              *ReferenceFrameBuff_p;
    U32                                *RefPicList_p;
    BOOL                                IsBufferFound;
    BOOL                                ReferenceIsFrame;
    BOOL                                CurrentPictureIsFrame;
    VIDCOM_ParsedPictureInformation_t  *ParsedPicture_p;
    VIDBUFF_PictureBuffer_t            *PictureBuffer_p;
    H264_PictureSpecificData_t         *PictSpecData_p;
    VIDCOM_PictureGenericData_t        *PictGenData_p;
    H264CompanionData_t                *H264CompanionData_p;
    U32                                 PictureDescriptorRange;

	DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

	iBuff = 0;
	IsBufferFound = FALSE;
	do
	{
        IsBufferFound = (PrivateData_p->H264DecoderData[iBuff].CommandIdForProd == ConfirmBufferParams_p->CommandId);
		if (!IsBufferFound)
		{
			iBuff++;
		}
	}
	while ((iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS) && !IsBufferFound);

    if (!IsBufferFound)
    {
		STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_ConfirmBuffer) : Unknown Command Id %d ", ConfirmBufferParams_p->CommandId));
        return(ST_ErrorCode/* TODO */);
    }

    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
    PrivateData_p->H264DecoderData[iBuff].ConfirmBufferHasBeenRejected = FALSE;
    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);

    H264CompanionData_p = PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p;

	/*  Create MME data associated with the Frame Buffer */
	if (ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p == NULL)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_ConfirmBuffer) :  FrameBuff_p = NULL"));
        ST_ErrorCode = ST_ERROR_NO_MEMORY;
        return(ST_ErrorCode);
	}

    fmw_p = &H264CompanionData_p->h264TransformerFmwParam;

    fmw_p->DecodedBufferAddress.Luma_p              = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address_p;
    fmw_p->DecodedBufferAddress.Chroma_p            = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p;
    fmw_p->DecodedBufferAddress.MBStruct_p          = ConfirmBufferParams_p->NewPictureBuffer_p->PPB.Address_p;


    if (ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        fmw_p->DecodedBufferAddress.LumaDecimated_p     = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p;
        fmw_p->DecodedBufferAddress.ChromaDecimated_p   = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p;
        fmw_p->MainAuxEnable = AUX_MAIN_OUT_EN;
    }
    else
    {
#ifdef ST_XVP_ENABLE_FGT
        if( (VIDFGT_IsActivated(PrivateData_p->FGTHandle) == TRUE)
            && (ConfirmBufferParams_p->NewPictureBuffer_p->ParsedPictureInformation.PictureFilmGrainSpecificData.IsFilmGrainEnabled == TRUE))
        {
            fmw_p->DecodedBufferAddress.LumaDecimated_p     = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.FGTLumaAccumBuffer_p;
            fmw_p->DecodedBufferAddress.ChromaDecimated_p   = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.FGTChromaAccumBuffer_p;

            fmw_p->MainAuxEnable = AUX_MAIN_OUT_EN;
        }
        else
#endif /* ST_XVP_ENABLE_FGT */
        {
            fmw_p->DecodedBufferAddress.LumaDecimated_p     = NULL;
            fmw_p->DecodedBufferAddress.ChromaDecimated_p   = NULL;
            fmw_p->MainAuxEnable = MAINOUT_EN;
        }
    }

#ifdef NATIVE_CORE
    /* workaround for VSOC simulator in order to provide the number of reference frames to the SW decoder */
    fmw_p->MainAuxEnable |= (ConfirmBufferParams_p->NewPictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.NumberOfReferenceFrames << 8);
#endif /* NATIVE_CORE */


	PictureBuffer_p = ConfirmBufferParams_p->NewPictureBuffer_p;
	ParsedPicture_p = &(PictureBuffer_p->ParsedPictureInformation);
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;
    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);

/* Update decimation factors. TODO : add secondary reconstruction */
    DecimationFactors = STVID_DECIMATION_FACTOR_NONE;
    if (PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        DecimationFactors = PictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;
    }

    fmw_p->HorizontalDecimationFactor   = HDEC_1;
    if (DecimationFactors & STVID_DECIMATION_FACTOR_H2)
    {
        /*fmw_p->HorizontalDecimationFactor   = HDEC_2;*/
        fmw_p->HorizontalDecimationFactor   = HDEC_ADVANCED_2;
        PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation=TRUE;
    }
    if (DecimationFactors & STVID_DECIMATION_FACTOR_H4)
    {
        /*fmw_p->HorizontalDecimationFactor   = HDEC_4;*/
        fmw_p->HorizontalDecimationFactor   = HDEC_ADVANCED_4;
        PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation=TRUE;
    }
    fmw_p->VerticalDecimationFactor     = VDEC_1;
    if (DecimationFactors & STVID_DECIMATION_FACTOR_V2)
    {
        fmw_p->VerticalDecimationFactor   = VDEC_2_PROG; /* TODO : not clear !!!! */
        fmw_p->VerticalDecimationFactor   = VDEC_2_INT;
    }

#ifdef ST_XVP_ENABLE_FGT
    if (PictureBuffer_p->AssociatedDecimatedPicture_p == NULL)
    {
        if( (VIDFGT_IsActivated(PrivateData_p->FGTHandle) == TRUE)
            && (PictureBuffer_p->ParsedPictureInformation.PictureFilmGrainSpecificData.IsFilmGrainEnabled))
        {
            fmw_p->AdditionalFlags = H264_ADDITIONAL_FLAG_FGT;
        }
    }
#endif /* ST_XVP_ENABLE_FGT */

	/* Picture Descriptors and RefFrameAdress creation */
    fmw_p->InitialRefPictList.ReferenceDescriptorsBitsField = 0;
    RefFrameAddress_p = &(fmw_p->InitialRefPictList.ReferenceFrameAddress[0]);

	CurrentPictureIsFrame = (ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME);

    for (i = iPictDesc = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++, RefFrameAddress_p++)
    {
        RefFrameAddress_p->DecodedBufferAddress.Luma_p      = NULL;
        RefFrameAddress_p->DecodedBufferAddress.Chroma_p    = NULL;
        RefFrameAddress_p->DecodedBufferAddress.MBStruct_p  = NULL;
        RefFrameAddress_p->DecodedBufferAddress.LumaDecimated_p = NULL;
        RefFrameAddress_p->DecodedBufferAddress.ChromaDecimated_p = NULL;
        ReferenceFrameBuff_p = NULL;
        if (PictGenData_p->IsValidIndexInReferenceFrame[i])
        {
            ReferenceFrameBuff_p = PictureBuffer_p->ReferencesFrameBufferPointer[i];
			if (ReferenceFrameBuff_p == NULL)
			{
		        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_ConfirmBuffer) :  ReferenceFrameBuff_p = NULL Index %d", i));
			}
            else if (ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p == NULL)
			{
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_ConfirmBuffer) :  ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p = NULL Index %d", i));
			}
            else if (!IsSecondOfTwoFields(PictureBuffer_p) && (ReferenceFrameBuff_p == PictureBuffer_p->FrameBuffer_p))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (H264DECODER_ConfirmBuffer) :  non existing ref was replaced by current FB Index %d", i));
            }
        }
        if ((ReferenceFrameBuff_p != NULL) && (ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p != NULL) &&
             ((ReferenceFrameBuff_p != PictureBuffer_p->FrameBuffer_p) || (IsSecondOfTwoFields(PictureBuffer_p))))
        {
            RefFrameAddress_p->DecodedBufferAddress.Luma_p          = ReferenceFrameBuff_p->Allocation.Address_p;
            RefFrameAddress_p->DecodedBufferAddress.Chroma_p        = ReferenceFrameBuff_p->Allocation.Address2_p;
            RefFrameAddress_p->DecodedBufferAddress.LumaDecimated_p = NULL;
            RefFrameAddress_p->DecodedBufferAddress.ChromaDecimated_p = NULL;
            RefFrameAddress_p->DecodedBufferAddress.MBStruct_p      = ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->PPB.Address_p;
            ReferenceIsFrame = (ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME);
			if (CurrentPictureIsFrame)
            {
				if (ReferenceIsFrame) /* Current Picture is FRAME and Reference is FRAME */
				{
                    h264dec_CreatePictureDescriptor(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceTopFieldList[i], &ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation);
				}
				else{ /* Current Picture is FRAME and Reference is FIELD */
           			fmw_p->InitialRefPictList.ReferenceDescriptorsBitsField |= (1 << i);
                    h264dec_CreatePictureDescriptorFrame(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceTopFieldList[i], ReferenceFrameBuff_p);
				}
#ifdef H264DECODE_TRACE_VERBOSE
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_ConfirmBuffer) : Pict Desc FRAME %d Frame Id %d", iPictDesc, i));
#endif
                iPictDesc++;
			}
            else{ /* Current Picture is FIELD */
				if (ReferenceIsFrame) /* Current Picture is FIELD and Reference is FRAME */
				{
                    h264dec_CreatePictureDescriptorField(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceTopFieldList[i], &ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation, H264_PIC_DESCRIPTOR_FIELD_TOP);
#ifdef H264DECODE_TRACE_VERBOSE
					STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DEC(H264DECODER_ConfirmBuffer): PictDesc Frame Id %d POC %d type %d",
                                iPictDesc, fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.PicOrderCnt,
                                fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.DescriptorType));
#endif
                    iPictDesc++;
                    h264dec_CreatePictureDescriptorField(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceBottomFieldList[i], &ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation, H264_PIC_DESCRIPTOR_FIELD_BOTTOM);
#ifdef H264DECODE_TRACE_VERBOSE
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DEC(H264DECODER_ConfirmBuffer): PictDesc Frame Id %d POC %d type %d",
                                iPictDesc, fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.PicOrderCnt,
                                fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.DescriptorType));
#endif
                    iPictDesc++;
				}
				else /* Current Picture is FIELD and Reference is FIELD */
				{
                	/* First Field */
           			fmw_p->InitialRefPictList.ReferenceDescriptorsBitsField |= (1 << i);
                    if ((ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p != NULL) && (ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureSpecificData_p != NULL))
	                {
					    switch (ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure)
					    {
					        case  STVID_PICTURE_STRUCTURE_TOP_FIELD :
                                h264dec_CreatePictureDescriptor(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceTopFieldList[i], &ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation);
					            break;
					        case  STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
                                h264dec_CreatePictureDescriptor(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceBottomFieldList[i], &ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation);
					            break;
							default :
								STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptor) : Unknown Picture Structure %d ", PictGenData_p->PictureInfos.VideoParams.PictureStructure));
							    break;
						}
#ifdef H264DECODE_TRACE_VERBOSE
			        	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DEC(H264DECODER_ConfirmBuffer): PictDesc Frame Id %d POC %d type %d",
                                    iPictDesc, fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.PicOrderCnt,
                                    fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.DescriptorType));
#endif
	                    iPictDesc++;
	                }
                    else
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptor) : picgendata_p==NULL for 1st field ref 0x%x for 0x%x (%d-%d/%d)",
                                ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p,
                                PictureBuffer_p->FrameBuffer_p,
                                ParsedPicture_p->PictureGenericData.DecodingOrderFrameID,
                                ParsedPicture_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                ParsedPicture_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                    }

	                /* Second Field */
                    if ((ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p != NULL) && (ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureSpecificData_p != NULL))
	                {
					    switch (ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure)
					    {
					        case  STVID_PICTURE_STRUCTURE_TOP_FIELD :
                                h264dec_CreatePictureDescriptor(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceTopFieldList[i], &ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation);
					            break;
					        case  STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
                                h264dec_CreatePictureDescriptor(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceBottomFieldList[i], &ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation);
					            break;
							default :
								STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptor) : Unknown Picture Structure %d ", PictGenData_p->PictureInfos.VideoParams.PictureStructure));
							    break;
						}
#ifdef H264DECODE_TRACE_VERBOSE
			        	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DEC(H264DECODER_ConfirmBuffer): PictDesc Frame Id %d POC %d type %d",
                                    iPictDesc, fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.PicOrderCnt,
                                    fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.DescriptorType));
#endif
						iPictDesc++;
	                }
                    else
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (h264dec_CreatePictureDescriptor) : picgendata_p==NULL for 2nd field ref 0x%x for 0x%x (%d-%d/%d)",
                                ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p,
                                PictureBuffer_p->FrameBuffer_p,
                                ParsedPicture_p->PictureGenericData.DecodingOrderFrameID,
                                ParsedPicture_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                ParsedPicture_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                    }
				}
            }
        }
        /* Non existing frames special case */
        else if (PictSpecData_p->FullReferenceTopFieldList[i].IsNonExisting)
        {
			if (CurrentPictureIsFrame)
            {
				/* Current Picture is FRAME and NON existing reference is always FRAME */
                h264dec_CreateNonExistingPictureDescriptor(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceTopFieldList[i], &PictSpecData_p->FullReferenceBottomFieldList[i]);
#ifdef H264DECODE_TRACE_VERBOSE
		        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_ConfirmBuffer) : Pict Desc NON EXISTING FRAME %d Frame Id %d", iPictDesc, i));
#endif
                iPictDesc++;
			}
            else
            { /* Current Picture is FIELD */
				/* Current Picture is FIELD and NON existing reference is always FRAME */
                h264dec_CreateNonExistingPictureDescriptorField(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceTopFieldList[i], H264_PIC_DESCRIPTOR_FIELD_TOP);
#ifdef H264DECODE_TRACE_VERBOSE
				STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DEC(H264DECODER_ConfirmBuffer): PictDesc Non existing Frame Id %d POC %d type %d",
                              iPictDesc, fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.PicOrderCnt,
                              fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.DescriptorType));
#endif
                 iPictDesc++;
                 h264dec_CreateNonExistingPictureDescriptorField(&fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc], i, &PictSpecData_p->FullReferenceBottomFieldList[i], H264_PIC_DESCRIPTOR_FIELD_BOTTOM);
#ifdef H264DECODE_TRACE_VERBOSE
                 STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DEC(H264DECODER_ConfirmBuffer): PictDesc Non existing Frame Id %d POC %d type %d",
                               iPictDesc, fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.PicOrderCnt,
                               fmw_p->InitialRefPictList.PictureDescriptor[iPictDesc].HostData.DescriptorType));
#endif
                 iPictDesc++;
            }
        }
        else{
            /* TODO : RAZ PictureDescriptor & ReferenceFrameAddress ??? */
        }
	}

    PictureDescriptorRange = iPictDesc;

	/* RefPicList creation : P0 */
    fmw_p->InitialRefPictList.InitialPList0.RefPiclistSize = 0;
    RefPicList_p = &(fmw_p->InitialRefPictList.InitialPList0.RefPicList[0]);
    for (iRef = 0; iRef < PictGenData_p->UsedSizeInReferenceListP0; iRef++)
    {
        RefErrorCode = h264dec_SearchFrameInFullFrameList(PictGenData_p, PictGenData_p->ReferenceListP0[iRef], PictSpecData_p, &iFrame);
        if (RefErrorCode == ST_NO_ERROR)
        {
            if (CurrentPictureIsFrame)
            {
                /* Search for picture descriptor that has FrameIndex = iFrame */
                *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FRAME);
            }
            else /* FIELD type */
            {
                if (PictSpecData_p->IsReferenceTopFieldP0[iRef])
                {
                    /* Search for Top field picture descriptor that has FrameIndex = iFrame */
                    *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FIELD_TOP);
                }
                else
                {
                    /* Search for Bottom field picture descriptor that has FrameIndex = iFrame */
                    *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FIELD_BOTTOM);
                }
            }   /* FIELD type */
#ifdef H264DECODE_TRACE_VERBOSE
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_ConfirmBuffer) : RefPicList P0 iRef %d iFrame %d Id %d", iRef, iFrame, PictGenData_p->FullReferenceFrameList[iFrame]));
#endif
            fmw_p->InitialRefPictList.InitialPList0.RefPiclistSize++;
            RefPicList_p++;
        }
    }

	/* RefPicList creation : B0 */
    fmw_p->InitialRefPictList.InitialBList0.RefPiclistSize = 0;
    RefPicList_p = &(fmw_p->InitialRefPictList.InitialBList0.RefPicList[0]);
    for (iRef = 0; iRef < PictGenData_p->UsedSizeInReferenceListB0; iRef++)
    {
        RefErrorCode = h264dec_SearchFrameInFullFrameList(PictGenData_p, PictGenData_p->ReferenceListB0[iRef], PictSpecData_p, &iFrame);
        if (RefErrorCode == ST_NO_ERROR)
        {
            if (CurrentPictureIsFrame)
            {
                /* Search for picture descriptor that has FrameIndex = iFrame */
                *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FRAME);
            }
            else /* FIELD type */
            {
                if (PictSpecData_p->IsReferenceTopFieldB0[iRef])
                {
                    /* Search for Top field picture descriptor that has FrameIndex = iFrame */
                    *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FIELD_TOP);
                }
                else
                {
                    /* Search for Bottom field picture descriptor that has FrameIndex = iFrame */
                    *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FIELD_BOTTOM);
                }
            }   /* FIELD type */
#ifdef H264DECODE_TRACE_VERBOSE
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_ConfirmBuffer) : RefPicList B0 iRef %d iFrame %d Id %d", iRef, iFrame, PictGenData_p->FullReferenceFrameList[iFrame]));
#endif
            fmw_p->InitialRefPictList.InitialBList0.RefPiclistSize++;
            RefPicList_p++;
        }
	}

	/* RefPicList creation : B1 */
    fmw_p->InitialRefPictList.InitialBList1.RefPiclistSize = 0;
    RefPicList_p = &(fmw_p->InitialRefPictList.InitialBList1.RefPicList[0]);
    for (iRef = 0; iRef < PictGenData_p->UsedSizeInReferenceListB1; iRef++)
    {
        RefErrorCode = h264dec_SearchFrameInFullFrameList(PictGenData_p, PictGenData_p->ReferenceListB1[iRef], PictSpecData_p, &iFrame);
        if (RefErrorCode == ST_NO_ERROR)
        {
            if (CurrentPictureIsFrame)
            {
                /* Search for picture descriptor that has FrameIndex = iFrame */
                *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FRAME);
            }
            else /* FIELD type */
            {
                if (PictSpecData_p->IsReferenceTopFieldB1[iRef])
                {
                    /* Search for Top field picture descriptor that has FrameIndex = iFrame */
                    *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FIELD_TOP);
                }
                else
                {
                    /* Search for Bottom field picture descriptor that has FrameIndex = iFrame */
                    *RefPicList_p = (U32)h264dec_SearchPictureDescriptorByFrameIndex(&(fmw_p->InitialRefPictList), PictureDescriptorRange, iFrame, H264_PIC_DESCRIPTOR_FIELD_BOTTOM);
                }
            }   /* FIELD type */
#ifdef H264DECODE_TRACE_VERBOSE
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_ConfirmBuffer) : RefPicList B1 iRef %d iFrame %d Id %d", iRef, iFrame, PictGenData_p->FullReferenceFrameList[iFrame]));
#endif
            fmw_p->InitialRefPictList.InitialBList1.RefPiclistSize++;
            RefPicList_p++;
        }
	}

	/* MME_SET_GLOBAL_TRANSFORM_PARAMS command */
    H264CompanionData_p->CmdInfoParam.StructSize  = sizeof(MME_Command_t);
	/* WARNING :  CmdEnd = MME_COMMAND_END_RETURN_NO_INFO replaced by MME_COMMAND_END_RETURN_NOTIFY */
    H264CompanionData_p->CmdInfoParam.CmdEnd      = MME_COMMAND_END_RETURN_NOTIFY;
    H264CompanionData_p->CmdInfoParam.DueTime     = 0;
    H264CompanionData_p->CmdInfoParam.ParamSize   = sizeof(H264_SetGlobalParam_t);
    H264CompanionData_p->CmdInfoParam.Param_p     = &H264CompanionData_p->h264GlobalParam;
    H264CompanionData_p->CmdInfoParam.CmdCode     = MME_SET_GLOBAL_TRANSFORM_PARAMS;
    H264CompanionData_p->CmdInfoParam.NumberInputBuffers  = 0;
    H264CompanionData_p->CmdInfoParam.NumberOutputBuffers = 0;
    H264CompanionData_p->CmdInfoParam.DataBuffers_p = NULL;
    H264CompanionData_p->CmdInfoParam.CmdStatus.AdditionalInfoSize = 0;
    H264CompanionData_p->CmdInfoParam.CmdStatus.AdditionalInfo_p = NULL;

    /* MME_TRANSFORM for Firmware command */
    H264CompanionData_p->CmdInfoFmw.StructSize  = sizeof(MME_Command_t);
    H264CompanionData_p->CmdInfoFmw.CmdEnd      = MME_COMMAND_END_RETURN_NOTIFY;
    H264CompanionData_p->CmdInfoFmw.DueTime     = 0;
    H264CompanionData_p->CmdInfoFmw.ParamSize   = sizeof(H264_TransformParam_fmw_t);
    H264CompanionData_p->CmdInfoFmw.Param_p     = &H264CompanionData_p->h264TransformerFmwParam;
    H264CompanionData_p->CmdInfoFmw.CmdCode     = MME_TRANSFORM;
    H264CompanionData_p->CmdInfoFmw.NumberInputBuffers  = 0;
    H264CompanionData_p->CmdInfoFmw.NumberOutputBuffers = 0;
    H264CompanionData_p->CmdInfoFmw.DataBuffers_p = NULL;
    H264CompanionData_p->CmdInfoFmw.CmdStatus.AdditionalInfoSize = sizeof(H264_CommandStatus_fmw_t);
    H264CompanionData_p->CmdInfoFmw.CmdStatus.AdditionalInfo_p = &H264CompanionData_p->H264_CommandStatus_fmw;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_CONFIRM_BUFFER, "dDECConfirm%d-b%dpp%d ",
                PrivateData_p->H264DecoderData[iBuff].CommandIdForProd,
                iBuff, PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.CmdId));
#endif

    /*memset(STAVMEM_DeviceToCPU(fmw_p->DecodedBufferAddress.Luma_p, &PrivateData_p->VirtualMapping), 0, ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.TotalSize);*/
    Command = DECODER_COMMAND_CONFIRM_BUFFER_FUNC;
    Command += (U32)(iBuff) << 8;
/*    TraceBuffer(("C0%d02 ", iBuff)); */
    PushDecoderCommand(DECODER_Data_p, Command);
#ifdef H264DECODE_TRACE_VERBOSE
  STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER (H264DECODER_ConfirmBuffer) : Buffer %d  State CONFIRM_BUFFER_FUNC IDpp %d IDparam %d IDfmw %d",
                iBuff, PrivateData_p->H264DecoderData[iBuff].CommandIdForProd, PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId,
                PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoFmw.CmdStatus.CmdId));
#endif
#if !defined PERF1_TASK2
    STOS_SemaphoreSignal(PrivateData_p->DecoderOrder_p);
#else
    h264dec_DecoderMainFunc( DecoderHandle);
#endif

    return(ST_ErrorCode);
} /* end of H264DECODER_ConfirmBuffer() */

/*******************************************************************************
Name        : h264_InitData
Description :
Parameters  : PrivateData_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264_InitData(H264DecoderPrivateData_t * PrivateData_p)
{
    PrivateData_p->NbBufferUsed    = 0;
    memset (&(PrivateData_p->H264DecoderData[0]), 0, sizeof(PrivateData_p->H264DecoderData));
    memset (&(PrivateData_p->PreProcContext[0]), 0, sizeof(PrivateData_p->PreProcContext));
    memset (&(PrivateData_p->FmwContext[0]), 0, sizeof(PrivateData_p->FmwContext));
    PrivateData_p->RejectedPreprocStartCommand = 0;
    PrivateData_p->RejectedConfirmBufferCallback = 0;
	PrivateData_p->RejectedSendCommand = 0;
    PrivateData_p->DecoderIsRunning = FALSE;
    PrivateData_p->DecoderResetPending = FALSE;
    PrivateData_p->IntermediateBufferAddress_p = NULL;
    PrivateData_p->H264CompanionDataSetupIsDone = FALSE;
#if !defined PERF1_TASK2
	PrivateData_p->DecoderTask.IsRunning = FALSE;
#endif
    memset (&(PrivateData_p->CommandsData[0]), 0, sizeof(PrivateData_p->CommandsData));
    /* Initialise commands queue */
    PrivateData_p->CommandsBuffer.Data_p         = PrivateData_p->CommandsData;
    PrivateData_p->CommandsBuffer.TotalSize      = sizeof(PrivateData_p->CommandsData) / 4;
    PrivateData_p->CommandsBuffer.BeginPointer_p = PrivateData_p->CommandsBuffer.Data_p;
    PrivateData_p->CommandsBuffer.UsedSize       = 0;
    PrivateData_p->CommandsBuffer.MaxUsedSize    = 0;
    PrivateData_p->NextCommandId = 500;  /* Arbitrary starting value , not set to 0 for debug purpose */
#ifdef STVID_DEBUG_GET_STATISTICS
    /* Initialise here all statistics under decoder responsibility */
    PrivateData_p->Statistics.DecodeGNBvd42696Error  = 0;
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined(STVID_VALID)
    /* TDB : initialise these to defualt values here */
    U32                                     NbMMECmdDumped;
    H264CompanionData_t                     H264CompanionDataDump[MME_MAX_DUMP];
#endif
    PrivateData_p->H264PreviousGlobalParam.SeqParameterSetId = -1;
    PrivateData_p->H264PreviousGlobalParam.PicParameterSetId = -1;
    PrivateData_p->IsEmbxTransportValid = FALSE;
#ifdef ST_speed
    PrivateData_p->DecodingMode = H264_NORMAL_DECODE;
#ifdef STVID_TRICKMODE_BACKWARD
    PrivateData_p->DecodingDirection = DECODING_FORWARD;
#endif
#endif

} /* end of h264_InitData() */

/******************************************************************************/
/*! \brief Re-Initialize the bitbuffer parameters
 *
 * Re-Initialize the bitbuffer parameters
 * Parameters: Decoder handle
 *             bit buffer address
 *             bit buffer size
 *
 * \param DecoderHandle
 * \param BitBufferAddress_p
 * \param BitBufferSize
 * \return ST_ErrorCode
 */
/******************************************************************************/
ST_ErrorCode_t H264DECODER_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize)
{
    DECODER_Properties_t *              DECODER_Data_p;
    H264DecoderPrivateData_t *          PrivateData_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

	DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->BitBufferAddress_p = BitBufferAddress_p;
    PrivateData_p->BitBufferSize = BitBufferSize;
    return(Error);
}   /* End of H264DECODER_UpdateBitBufferParams */


/******************************************************************************/
/*! \brief Initialize the decoder module
 *
 * Allocate buffers, Initialize semaphores, tasks,
 * Parameters: Data Partition
 *             Handles: Events,
 *
 * \param DecoderHandle
 * \param InitParams_p
 * \return ST_ErrorCode
 */
/******************************************************************************/
ST_ErrorCode_t H264DECODER_Init(const DECODER_Handle_t DecoderHandle, const DECODER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    H264DecoderPrivateData_t *      PrivateData_p;
    U8                              iBuff, TryCount=0;
    H264Preproc_InitParams_t        H264Preproc_InitParams;
    H264_InitTransformerParam_fmw_t H264_InitTransformerParam_fmw;
    MME_TransformerInitParams_t     initParams_decoder;
    EMBX_TRANSPORT                  tp;
    EMBX_ERROR                      EMBXError;
    U32                             DeviceRegistersBaseAddress;
    char                            TransformerName[MME_MAX_TRANSFORMER_NAME];

	if ((DecoderHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;

    /* Allocate DECODER PrivateData structure */
    DECODER_Data_p->PrivateData_p = (H264DecoderPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(H264DecoderPrivateData_t));
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

	if (PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

	/* Data initialisation : h264_InitData set H264DecoderPrivateData_t to 0 :
	 * any variable set to a value different to 0 should be initialize AFTER
	 * calling this function */
    h264_InitData(PrivateData_p);

    PrivateData_p->BitBufferAddress_p = InitParams_p->BitBufferAddress_p;
    PrivateData_p->BitBufferSize = InitParams_p->BitBufferSize;
    PrivateData_p->IntermediateBufferSize = DECODER_INTERMEDIATE_BUFFER_SIZE;
#ifdef ST_XVP_ENABLE_FGT
    PrivateData_p->FGTHandle = InitParams_p->FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
    DECODER_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
    STAVMEM_GetSharedMemoryVirtualMapping(&(PrivateData_p->VirtualMapping));

    DECODER_Data_p->CodecMode = InitParams_p->CodecMode;

    /* Allocate Host/LX shared data in EMBX shared memory to allow Multicom to perform Address translation */

#if defined(ST_7100) || defined(ST_7109)
    EMBXError = EMBX_OpenTransport(MME_VIDEO_TRANSPORT_NAME, &tp);
#elif defined(ST_7200)
    EMBXError = EMBX_OpenTransport(InitParams_p->MMETransportName, &tp);
#endif /* #elif defined(ST_7200) ... */
    if (EMBXError != EMBX_SUCCESS)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        return(ST_ERROR_NO_MEMORY);
    }
    PrivateData_p->EmbxTransport = tp;
    PrivateData_p->IsEmbxTransportValid = TRUE;
#ifdef ST_speed
    PrivateData_p->DecodingMode = H264_NORMAL_DECODE;
#ifdef STVID_TRICKMODE_BACKWARD
    PrivateData_p->DecodingDirection = DECODING_FORWARD;
#endif
#endif

    for (iBuff = 0; iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS; iBuff++)
    {
        /*  H264CompanionData_t structure allocation --> contains all the data sent or received from the LX */
        EMBXError = EMBX_Alloc(tp, sizeof(H264CompanionData_t), (EMBX_VOID **)(&(PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p)));
        if (EMBXError != EMBX_SUCCESS)
        {
            EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
            if (EMBXError != EMBX_SUCCESS)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
            }
            PrivateData_p->IsEmbxTransportValid = FALSE;
            STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
            return(ST_ERROR_NO_MEMORY);
        }
    }

/*
printf("&CommandsData=0x%08x\n",&(PrivateData_p->CommandsData));
printf("&CommandsBuffer=0x%08x\n",&(PrivateData_p->CommandsBuffer));
printf("&CommandsBuffer.BeginPointer_p=0x%08x\n",&(PrivateData_p->CommandsBuffer.BeginPointer_p));
*/
    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_CONFIRM_BUFFER_EVT, &(PrivateData_p->RegisteredEventsID[DECODER_CONFIRM_BUFFER_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_JOB_COMPLETED_EVT, &(PrivateData_p->RegisteredEventsID[DECODER_JOB_COMPLETED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_INFORM_READ_ADDRESS_EVT, &(PrivateData_p->RegisteredEventsID[DECODER_INFORM_READ_ADDRESS_EVT_ID]));
    }
	if (ErrorCode != ST_NO_ERROR)
    {
        EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
        }
        PrivateData_p->IsEmbxTransportValid = FALSE;
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module H264 decoder init : could not register events !"));
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    PrivateData_p->DataProtect_p = STOS_SemaphoreCreatePriority(DECODER_Data_p->CPUPartition_p, 1);
#if !defined PERF1_TASK2
    PrivateData_p->DecoderOrder_p = STOS_SemaphoreCreateFifoTimeOut(DECODER_Data_p->CPUPartition_p, 0);
	/* Create decoder task */
	ErrorCode = h264dec_StartdecoderTask(DecoderHandle);
	if (ErrorCode != ST_NO_ERROR)
	{
        EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
        }
        PrivateData_p->IsEmbxTransportValid = FALSE;
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
		return (ST_ERROR_BAD_PARAMETER);
	}
	PrivateData_p->DecoderTask.IsRunning = TRUE;
#endif

    /* MME API initialisation */
    H264Preproc_InitParams.CPUPartition_p = InitParams_p->CPUPartition_p;
    H264Preproc_InitParams.AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
    H264Preproc_InitParams.RegistersBaseAddress_p = InitParams_p->RegistersBaseAddress_p;
    H264Preproc_InitParams.InterruptNumber = InitParams_p->SharedItParams.Number;
    H264Preproc_InitParams.Callback = &TransformerCallback_preprocessor;
    H264Preproc_InitParams.CallbackUserData = (void *) DecoderHandle; /* UserData */

    H264_InitTransformerParam_fmw.CircularBinBufferBeginAddr_p = (void*)0;
    H264_InitTransformerParam_fmw.CircularBinBufferEndAddr_p = (void*)0xFFFFFFFF;

	initParams_decoder.Priority = MME_PRIORITY_NORMAL;
	initParams_decoder.StructSize = sizeof (MME_TransformerInitParams_t);
    initParams_decoder.Callback = &TransformerCallback_decoder;
    initParams_decoder.CallbackUserData = (void *) DecoderHandle; /* UserData */
    initParams_decoder.TransformerInitParamsSize = sizeof(H264_InitTransformerParam_fmw_t);
    initParams_decoder.TransformerInitParams_p = &(H264_InitTransformerParam_fmw);

    /* Init transformer preproc. */
    ErrorCode = H264Preproc_InitTransformer (&H264Preproc_InitParams, &PrivateData_p->TranformHandlePreProc);

    if (ErrorCode != MME_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_InitTransformer(preprocessor) failed, result=$%lx", ErrorCode));
#if !defined PERF1_TASK2
        /* Terminate the decoder task */
        h264dec_StopDecoderTask(DecoderHandle);
        PrivateData_p->DecoderTask.IsRunning = FALSE;
#endif
        EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
        }
        PrivateData_p->IsEmbxTransportValid = FALSE;
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        return (ErrorCode);
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Transformer(preprocessor) initialized, handle %d", PrivateData_p->TranformHandlePreProc));

    /* Init decoder */
    #if defined(ST_7200)
    /* Register area is seen with same address between CPU and Device on 7200 */
    DeviceRegistersBaseAddress = (U32)InitParams_p->RegistersBaseAddress_p;
    #else /* defined(ST_7200) */
    DeviceRegistersBaseAddress = (U32)STAVMEM_CPUToDevice(InitParams_p->RegistersBaseAddress_p,&PrivateData_p->VirtualMapping);
    #endif /* defined(ST_7200) */

    #if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    #if defined(ST_DELTAMU_GREEN_HE)
    DeviceRegistersBaseAddress = 0xAC220000;
    #else
    DeviceRegistersBaseAddress = 0xAC211000;
    #endif /* ST_DELTAMU_GREEN_HE */
    #endif /* ST_DELTAPHI_HE || ST_DELTAMU_HE */
#ifdef NATIVE_CORE
   /* currently VSOC is using a model for deltamu with old MME_TRANSFORMER_NAME */
	sprintf(TransformerName, "%s", H264_MME_TRANSFORMER_NAME);
#else
	sprintf(TransformerName, "%s_%08X", H264_MME_TRANSFORMER_NAME, DeviceRegistersBaseAddress);
#endif
#ifdef STUB_FMW_ST40
    ErrorCode = MMESTUB_InitTransformer(TransformerName, &initParams_decoder, &PrivateData_p->TranformHandleDecoder);
#else
    do
    {
        ErrorCode = MME_InitTransformer(TransformerName, &initParams_decoder, &PrivateData_p->TranformHandleDecoder);
        if (ErrorCode!=MME_SUCCESS)
        {
            STOS_TaskDelay((S32)100); /* Wait before a new  MME Initialization */
            TryCount++;
        }
    } while ((TryCount<10)&&(ErrorCode!=MME_SUCCESS));
#endif

    if (ErrorCode != MME_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_InitTransformer(decoder) failed, result=$%lx", ErrorCode));
        /* Terminate the preproc transformer */
        H264Preproc_TermTransformer(PrivateData_p->TranformHandlePreProc);
#if !defined PERF1_TASK2
        /* Terminate the decoder task */
        h264dec_StopDecoderTask(DecoderHandle);
        PrivateData_p->DecoderTask.IsRunning = FALSE;
#endif
        EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
        }
        PrivateData_p->IsEmbxTransportValid = FALSE;
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        return (ErrorCode);
    }
#ifdef STVID_DEBUG_GET_STATUS
	PrivateData_p->Status.SequenceInfo_p = NULL;
	PrivateData_p->Status.IntermediateBuffersAddress_p = NULL;
	PrivateData_p->Status.IntermediateBuffersSize = 0;
#endif /* STVID_DEBUG_GET_STATUS */

    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Transformer(decoder) initialized, handle %d", PrivateData_p->TranformHandleDecoder));

#if defined(MME_DUMP) && defined(VALID_TOOLS)
	HDM_Init();
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */

    return (ErrorCode);
} /* End of DECODER_Init function */

/******************************************************************************/
/*! \brief Setup an object in the decoder. Only STVID_SetupObject_t STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION supported
 *
 * Allocate intermediate buffers after init, through STVID_SetupObject_t STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION (only this one supported)
 * Parameters: AVMEM Partition
 *
 * \param DecoderHandle
 * \param SetupParams_p
 * \return ST_ErrorCode
 */
/******************************************************************************/
ST_ErrorCode_t H264DECODER_Setup(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *                  DECODER_Data_p;
    H264DecoderPrivateData_t *              PrivateData_p;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    STAVMEM_FreeBlockParams_t               FreeParams;
    U8                                      iBuff;
    void                                   *VirtualAddress;
    void                                   *DeviceAddress;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (SetupParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (SetupParams_p->SetupObject == STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION)
    {
        /* if intermediate buffers have already been allocated, free them and allocate in the passed AVMEM partition */
        if (PrivateData_p->H264CompanionDataSetupIsDone)
        {
            FreeParams.PartitionHandle = PrivateData_p->IntermediateBufferAvmemPartitionHandle;
            if (PrivateData_p->IntermediateBufferAddress_p != NULL)
            {
                ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PrivateData_p->IntermediateBufferHdl));
            }
            PrivateData_p->H264CompanionDataSetupIsDone = FALSE;
        }

        /* Change Avmem partition after previous buffers have been deallocated */
        PrivateData_p->IntermediateBufferAvmemPartitionHandle = SetupParams_p->SetupSettings.AVMEMPartition;

        AllocBlockParams.PartitionHandle          = SetupParams_p->SetupSettings.AVMEMPartition;
        AllocBlockParams.Alignment                = 8;
        AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocBlockParams.NumberOfForbiddenRanges  = 0;
        AllocBlockParams.ForbiddenRangeArray_p    = NULL;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;

        /* Allocate less intermediate buffer if bit buffer is smaller, trying to dimension
           the intermediate buffers in proportion to the stream resolution.
           Don't need to allocate 1.5MB (needed for HD) if the stream is SD or CIF or smaller (mosaic)
         Need for HD : DECODER_INTERMEDIATE_BUFFER_SIZE + DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE
         Tried here : if SD or less, allocate (BBsize/HD-BBsize * DECODER_INTERMEDIATE_BUFFER_SIZE) + DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE */
#if !defined(VALID_TOOLS) && defined(STVID_ALLOW_VARIABLE_INTERMEDIATE_BUFFER_SIZE)
        PrivateData_p->IntermediateBufferSize = DECODER_INTERMEDIATE_BUFFER_SIZE;
        if (PrivateData_p->BitBufferSize < (2 * 1024 * 1024))
        {
            /* add 128 factor to avoid to loss precision in MAx Bitbuffer Sze / Actual Bitbuffer Size operation */
            PrivateData_p->IntermediateBufferSize = (DECODER_INTERMEDIATE_BUFFER_SIZE * (U32)128) / (((U32)5120768 * (U32)128) / PrivateData_p->BitBufferSize);
        }
#endif
        AllocBlockParams.Size = DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE + PrivateData_p->IntermediateBufferSize;
        AllocBlockParams.Size = ((AllocBlockParams.Size + 7) / 8) * 8;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Adjusted Intermediate buffer size = %d * %d bytes",DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS, AllocBlockParams.Size));
        AllocBlockParams.Size *= DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS;

        if (STAVMEM_AllocBlock(&AllocBlockParams, &PrivateData_p->IntermediateBufferHdl) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        if (STAVMEM_GetBlockAddress(PrivateData_p->IntermediateBufferHdl, (void **)&VirtualAddress) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        DeviceAddress = STAVMEM_VirtualToDevice(VirtualAddress,&PrivateData_p->VirtualMapping);
        PrivateData_p->IntermediateBufferAddress_p = DeviceAddress;

        /* Split intermediate buffer in fixed parts */
        for (iBuff = 0; iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS; iBuff++)
        {
            PrivateData_p->H264DecoderData[iBuff].IntermediateBuffer_p = (H264CompanionData_t *) DeviceAddress;
            DeviceAddress = (void*)((U32)DeviceAddress + (DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE + PrivateData_p->IntermediateBufferSize));
            DeviceAddress = (void *)((((U32)DeviceAddress + 7)/8) * 8);
        }

        PrivateData_p->H264CompanionDataSetupIsDone = TRUE;
#ifdef STVID_DEBUG_GET_STATUS
        PrivateData_p->Status.IntermediateBuffersAddress_p = PrivateData_p->IntermediateBufferAddress_p;
        PrivateData_p->Status.IntermediateBuffersSize = AllocBlockParams.Size;
#endif /* STVID_DEBUG_GET_STATUS */
    }
    else
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return (ErrorCode);
} /* End of H264DECODER_Setup function */

#if defined(DVD_SECURED_CHIP)
/******************************************************************************/
/*! \brief Setup WA_42332 h264 preproc data partition
 *
 * Parameters: Handle, AVMEM Partition
 *
 * \param DecoderHandle
 * \param AVMEM Partition
 * \return ST_ErrorCode
 */
/******************************************************************************/
ST_ErrorCode_t H264DECODER_SetupForH264PreprocWA_GNB42332(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *                  DECODER_Data_p;
    H264DecoderPrivateData_t *              PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    ErrorCode = H264Preproc_SetupTransformerWA_GNB42332(PrivateData_p->TranformHandlePreProc, AVMEMPartition);

    return (ErrorCode);
} /* End of H264DECODER_SetupForH264PreprocWA_GNB42332 */
#endif /* DVD_SECURED_CHIP */

/******************************************************************************/
/*! \brief Terminate the decoder module
 *
 * deAllocate buffers, Unregister Events
 *
 * \param DecoderHandle
 * \return ST_ErrorCode
 */
/******************************************************************************/
ST_ErrorCode_t H264DECODER_Term(const DECODER_Handle_t DecoderHandle)
{
    DECODER_Properties_t *          DECODER_Data_p;
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
    MME_ERROR                       MME_Error;
    ST_ErrorCode_t                  MMETermErrorCode;
    H264DecoderPrivateData_t *      PrivateData_p;
    U8                              iBuff;
    STAVMEM_FreeBlockParams_t   	FreeParams;
    EMBX_ERROR                      EMBXError;
    U32                             TermCounter;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
#if !defined PERF1_TASK2
	/* Terminate the decoder task */
	h264dec_StopDecoderTask(DecoderHandle);
    PrivateData_p->DecoderTask.IsRunning = FALSE;
#endif

    for(iBuff = 0 ; iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS ; iBuff++)
    {
        if (PrivateData_p->H264DecoderData[iBuff].State == DECODER_CONTEXT_PREPROCESSING)
        {
            ErrorCode = H264Preproc_AbortCommand(PrivateData_p->TranformHandlePreProc, &PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand);
            if (ErrorCode == ST_NO_ERROR)
            {
                STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                PrivateData_p->H264DecoderData[iBuff].State = DECODER_CONTEXT_NOT_USED;
                PrivateData_p->NbBufferUsed--;
                STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
            }
        }
    }

	/* terminate MME API */
    MMETermErrorCode = ST_NO_ERROR;
#ifdef STUB_FMW_ST40
	MMESTUB_TermTransformer(PrivateData_p->TranformHandleDecoder);
#else
    TermCounter = 0;
    do
    {
        MME_Error = MME_TermTransformer(PrivateData_p->TranformHandleDecoder);
        if (MME_Error == MME_DRIVER_NOT_INITIALIZED)
        {
            MME_Error = MME_SUCCESS;
            MMETermErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        if (MME_Error == MME_INVALID_HANDLE)
        {
            MME_Error = MME_SUCCESS;
            MMETermErrorCode = ST_ERROR_INVALID_HANDLE;
        }
        else
        if (MME_Error == MME_COMMAND_STILL_EXECUTING)
        {
            /* wait for command completion 40 ms stands for max frame decode time */
            STOS_TaskDelayUs(40000);  /* 40 ms */
        }
        TermCounter++;
    }
    while ((MME_Error != MME_SUCCESS) && (TermCounter < MAX_TRY_FOR_MME_TERM_TRANSFORMER));
    if (MME_Error == MME_COMMAND_STILL_EXECUTING)
    {
        MMETermErrorCode = ST_ERROR_DEVICE_BUSY;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Term Failed to terminate transformer (A command is still executing)"));
    }
#endif
    H264Preproc_TermTransformer(PrivateData_p->TranformHandlePreProc);

    /* delete semaphores */
    STOS_SemaphoreDelete(DECODER_Data_p->CPUPartition_p, PrivateData_p->DataProtect_p);
#if !defined PERF1_TASK2
    STOS_SemaphoreDelete(DECODER_Data_p->CPUPartition_p, PrivateData_p->DecoderOrder_p);
    PrivateData_p->DecoderOrder_p = NULL;
#endif

    /* deallocate memory  */
    if (PrivateData_p != NULL)
    {
        /* Release H264 companion allocated memory */
        if (PrivateData_p->H264CompanionDataSetupIsDone)
        {
            if (PrivateData_p->IntermediateBufferAddress_p != NULL)
            {
                FreeParams.PartitionHandle = PrivateData_p->IntermediateBufferAvmemPartitionHandle;
                ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PrivateData_p->IntermediateBufferHdl));
            }
            PrivateData_p->H264CompanionDataSetupIsDone = FALSE;
        }
        for (iBuff = 0; iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS; iBuff++)
        {
            EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p));
            if (EMBXError != EMBX_SUCCESS)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_Free() failed ErrorCode=%x", EMBXError));
            }
        }
    }

    if (PrivateData_p->IsEmbxTransportValid)
    {
        EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
        }
        PrivateData_p->IsEmbxTransportValid = FALSE;
    }

    if (PrivateData_p != NULL)
	{
        STOS_MemoryDeallocate(DECODER_Data_p->CPUPartition_p, PrivateData_p);
    }

#if defined(MME_DUMP) && defined(VALID_TOOLS)
    HDM_Term();
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = MMETermErrorCode;
    }
	return (ErrorCode);
    }




/*******************************************************************************
Name        : H264DECODER_FillAdditionnalDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    H264DecoderPrivateData_t *      PrivateData_p;
    DECODER_CodecInfo_t   CodecInfo;
    U32 ProfileAndLevelIndication, LevelIdc;
    U32 *Buf, *Dst, *Base, Size;

    /*
        WARNING !!!!!
        These definition are based on the PPB strucuture definitions as of 2007/01/26
        If the PBB structures changes, they have to be updated accordingly!
        PPB_SD is based on DeltaPhi/glob.h/MbDescription_s
        PPB_HD is based on DeltaPhi/glob.h/StoredMbDescription_s
    */
    U32 MbDescription_SD[40] = {0x00000000,0x00000000,              /* PackedMbType MB_TYPE_I_4x4, flags */
                      0x00000000,0x00000000,0x00000000,0x00000000,  /* MvPosition */
                      0xffffffff,0xffffffff,                        /* RefIdx : no ref */
                      0x00000000,0x00000000,0x00000000,0x00000000,  /* Mv's */
                      0x00000000,0x00000000,0x00000000,0x00000000,
                      0x00000000,0x00000000,0x00000000,0x00000000,
                      0x00000000,0x00000000,0x00000000,0x00000000,
                      0x00000000,0x00000000,0x00000000,0x00000000,
                      0x00000000,0x00000000,0x00000000,0x00000000,
                      0x00000000,0x00000000,0x00000000,0x00000000,
                      0x00000000,0x00000000,0x00000000,0x00000000}; /* = 160 bytes */

    U32 MbDescription_HD[10] = {0x00000000,                         /* flags */
                      0xffffffff,                                   /* RefIdx */
                      0x00000000,0x00000000,0x00000000,0x00000000,  /* Mv's */
                      0x00000000,0x00000000,0x00000000,0x00000000}; /* = 40 bytes */

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    /* fill the PPB */
    ProfileAndLevelIndication = Picture_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication;
    LevelIdc = ProfileAndLevelIndication & 0xff;
    ErrorCode = DECODER_GetCodecInfo(DecoderHandle, &CodecInfo, ProfileAndLevelIndication, DECODER_Data_p->CodecMode);
    Size = CodecInfo.FrameBufferAdditionalDataBytesPerMB;

    if (LevelIdc > 31)
    {        /*  LevelIdc > 3.1 Use the HD PPB */
          Buf = MbDescription_HD;
    }
    else
    {        /*  LevelIdc <= 3.1 Use the SD PPB */
          Buf = MbDescription_SD;
    }

    Base = STAVMEM_DeviceToVirtual(Picture_p->PPB.Address_p, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    Dst = Base;
    do
    {
        STOS_memcpy (Dst , Buf, Size);
        Dst = Dst + Size/sizeof(U32);
    }
    while( Dst < (Base + (Picture_p->PPB.Size / sizeof(U32))) );

	return (ErrorCode);
} /* End of H264DECODER_FillAdditionnalDataBuffer() function */

#ifdef VALID_TOOLS
/*******************************************************************************
Name        : h264dec_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264DECODER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#if defined(STUB_FMW_ST40)
    ErrorCode = MMESTUB_RegisterTrace();
#endif /* defined(STUB_FMW_ST40) */

#ifdef TRACE_UART
    TRACE_ConditionalRegister(H264DEC_TRACE_BLOCKID, H264DEC_TRACE_BLOCKNAME, h264dec_TraceItemList, sizeof(h264dec_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* TRACE_UART */

    return(ErrorCode);
}
#endif /* VALID_TOOLS */
#ifdef ST_speed
/*******************************************************************************
Name        : H264DECODER_SetDecodingMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void H264DECODER_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode)
{
    DECODER_Properties_t *          DECODER_Data_p;
    H264DecoderPrivateData_t *      PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

	PrivateData_p->DecodingMode = Mode;
}
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : H264DECODER_SetDecodingDirection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void H264DECODER_SetDecodingDirection(const DECODER_Handle_t DecoderHandle,const DecodingDirection_t Direction)
{
    DECODER_Properties_t *          DECODER_Data_p;
    H264DecoderPrivateData_t *      PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->DecodingDirection = Direction;
}
#endif /* STVID_TRICKMODE_BACKWARD */
#endif  /* ST_speed */
/* End of h264decode.c */
