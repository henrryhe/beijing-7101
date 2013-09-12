/*!
 ************************************************************************
 * \file vc1decode.c
 *
 * \brief VC1 decoder
 *
 * \author
 *  \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 ******************************************************************************
 */

/* Define to add debug info and traces */
/* Note : To add traces add STVID_DECODER_TRACE_UART in environment variables */



#ifndef STTBX_PRINT
#define STTBX_PRINT
#endif /* STTBX_PRINT */
#ifdef STVID_DEBUG_DECODER
    #define STTBX_REPORT
#endif


/*#define STUB_VC1_DECODER*/

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

#include "vc1decode.h"
#include "stavmem.h"

#ifdef STVID_VALID
#include "testtool.h"
#endif /* STVID_VALID */

/* Functions ---------------------------------------------------------------- */
void VC1_TransformerCallback_decoder (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);
void vc1_InitData(VC1DecoderPrivateData_t * PrivateData_p);

/* Constants ---------------------------------------------------------------- */
#define MAX_TRY_FOR_MME_TERM_TRANSFORMER 10 /* max loop number for MME_TermTransformer completion waiting loop in DECODER_Term */

#if defined(TRACE_UART) && defined(VALID_TOOLS)

#define VC1DEC_TRACE_BLOCKNAME "VC1DEC"

/* !!!! WARNING !!!! : Any modification of this table should be reported in emun in vc1decoder.h */
static TRACE_Item_t vc1dec_TraceItemList[] =
{
    {"GLOBAL",          TRACE_VC1DEC_COLOR,                        TRUE, FALSE},
    {"START",           TRACE_VC1DEC_COLOR,                        TRUE, FALSE},
    {"CONFIRM_BUFFER",  TRACE_VC1DEC_COLOR,                        TRUE, FALSE},
    {"REJECTED_CB",     TRACE_VC1DEC_COLOR,                        TRUE, FALSE},
    {"FMW_JOB_END",     TRACE_VC1DEC_COLOR,                        TRUE, FALSE},
    {"FMW_CALLBACK",    TRACE_VC1DEC_COLOR,                        TRUE, FALSE},
    {"FMW_ERRORS",      TRACE_VC1DEC_COLOR,                        TRUE, FALSE},
    {"FB_ADDRESSES",    TRACE_VC1DEC_COLOR,                        TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

const DECODER_FunctionsTable_t DECODER_VC1Functions =
{
    VC1DECODER_GetState,
    VC1DECODER_GetCodecInfo,
#ifdef STVID_DEBUG_GET_STATISTICS
    VC1DECODER_GetStatistics,
    VC1DECODER_ResetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    VC1DECODER_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    VC1DECODER_Init,
    VC1DECODER_Reset,
    VC1DECODER_DecodePicture,
    VC1DECODER_ConfirmBuffer,
    VC1DECODER_Abort,
    VC1DECODER_Setup,
    VC1DECODER_FillAdditionnalDataBuffer,
    VC1DECODER_Term
#ifdef ST_speed
    ,VC1DECODER_SetDecodingMode
#ifdef STVID_TRICKMODE_BACKWARD
    ,VC1DECODER_SetDecodingDirection
#endif
#endif
    ,VC1DECODER_UpdateBitBufferParams
#if defined(DVD_SECURED_CHIP)
    ,VC1DECODER_SetupForH264PreprocWA_GNB42332
#endif /* DVD_SECURED_CHIP */
};
/*#define VC1_DUMP_AFTER_DECODE*/
#ifdef   VC1_DUMP_AFTER_DECODE
static unsigned int VC1_Picture_Number = 0;
#endif

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
ST_ErrorCode_t VC1DECODER_GetStatistics(const DECODER_Handle_t DecoderHandle, STVID_Statistics_t * const Statistics_p)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
	UNUSED_PARAMETER(DecoderHandle);

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
    return(ErrorCode);
}

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
ST_ErrorCode_t VC1DECODER_ResetStatistics(const DECODER_Handle_t DecoderHandle, const STVID_Statistics_t * const Statistics_p)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
	UNUSED_PARAMETER(DecoderHandle);
	UNUSED_PARAMETER(Statistics_p);
    return(ErrorCode);
}
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
ST_ErrorCode_t VC1DECODER_GetStatus(const DECODER_Handle_t DecoderHandle, STVID_Status_t * const Status_p)
{
    DECODER_Properties_t *          DECODER_Data_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    VC1DecoderPrivateData_t *       PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    /* Update status with actual values in the decoder */
	Status_p->SequenceInfo_p = PrivateData_p->Status.SequenceInfo_p;

    return(ErrorCode);
}
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
ST_ErrorCode_t VC1DECODER_GetState(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId, DECODER_Status_t * const DecoderStatus_p)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
	UNUSED_PARAMETER(DecoderHandle);
	UNUSED_PARAMETER(CommandId);
	UNUSED_PARAMETER(DecoderStatus_p);
    return(ErrorCode);
}

/*******************************************************************************
Name        : VC1DECODER_GetCodecInfo
Description :
Parameters  :
 *
 *
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VC1DECODER_GetCodecInfo(const DECODER_Handle_t DecoderHandle, DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication, const STVID_CodecMode_t CodecMode)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
	UNUSED_PARAMETER(DecoderHandle);
	UNUSED_PARAMETER(ProfileAndLevelIndication);

    CodecInfo_p->MaximumDecodingLatencyInSystemClockUnit = 9000; /* System Clock = 90kHz */

    if (CodecMode == STVID_CODEC_MODE_DECODE)
    {
      CodecInfo_p->FrameBufferAdditionalDataBytesPerMB     = 16 ; /* 16 cf MME API */
    }
    else
    {
      ErrorCode = ST_ERROR_BAD_PARAMETER; /* Transcode not investigated yet */
    }

    CodecInfo_p->DecodePictureToConfirmBufferMaxTime     = 160; /* TODO : Which unit !!! Here it is set to 4Vsync in ms unit */

    return(ErrorCode);
}

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
ST_ErrorCode_t VC1DECODER_Reset(const DECODER_Handle_t DecoderHandle)
{

    ST_ErrorCode_t                          ErrorCode        = ST_NO_ERROR;
    DECODER_Properties_t                    * DECODER_Data_p;
    VC1DecoderPrivateData_t                 * PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *)        DecoderHandle;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->DecoderState = VC1DECODER_CONTEXT_NOT_USED;

    return(ErrorCode);
}

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
ST_ErrorCode_t VC1DECODER_Abort(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId)
{

    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t                    * DECODER_Data_p;
    VC1DecoderPrivateData_t                 * PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *)        DecoderHandle;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

	UNUSED_PARAMETER(CommandId);

    PrivateData_p->DecoderState = VC1DECODER_CONTEXT_NOT_USED;

    return(ErrorCode);
}

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
void VC1_TransformerCallback_decoder (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{
#ifdef   VC1_DUMP_AFTER_DECODE
    VC9_TransformParam_t            *fmw_p;
    U8                              FilenameSuffix[64];
    U8                              Filename_p[64];
#endif
    MME_Command_t               *command_p;
	/* ST_ErrorCode_t ST_ErrorCode = ST_NO_ERROR; */
    DECODER_Properties_t *      DECODER_Data_p;
    VC1DecoderPrivateData_t *   PrivateData_p;
    VC9_CommandStatus_t *       VC1_CommandStatus_fmw_p;
	UNUSED_PARAMETER(Event);

    DECODER_Data_p  = (DECODER_Properties_t *) UserData;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    command_p       = (MME_Command_t *) CallbackData;
    VC1_CommandStatus_fmw_p = (VC9_CommandStatus_t *) command_p->CmdStatus.AdditionalInfo_p;
    if ((VC9_CommandStatus_t *) command_p->CmdStatus.AdditionalInfoSize != 0)
    {
         PrivateData_p->VC1CmdStatus = *VC1_CommandStatus_fmw_p;
    }

    PrivateData_p->DecoderIsRunning = FALSE;

    /* inform the controler of the terminated command */
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.CommandId =  PrivateData_p->CommandId;

#ifdef STUB_FMW_ST40

    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;

#else /* NOT Stubbed FMW ST40 */
if(((VC1_CommandStatus_fmw_p->ErrorCode) & (VC9_DECODER_ERROR_MB_OVERFLOW   |
                                	             VC9_DECODER_ERROR_RECOVERED     |
                                	             VC9_DECODER_ERROR_NOT_RECOVERED )) != 0)
    {

        PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
        if (VC1_CommandStatus_fmw_p->ErrorCode & VC9_DECODER_ERROR_MB_OVERFLOW)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports VC9_DECODER_ERROR_MB_OVERFLOW\n"));
        }
        if (VC1_CommandStatus_fmw_p->ErrorCode & VC9_DECODER_ERROR_RECOVERED)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports VC9_DECODER_ERROR_RECOVERED\n"));
        }
        if (VC1_CommandStatus_fmw_p->ErrorCode & VC9_DECODER_ERROR_NOT_RECOVERED)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports VC9_DECODER_ERROR_NOT_RECOVERED\n"));
        }
    }
    else
    {
        PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;
    }
#endif

    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobState = DECODER_JOB_COMPLETED;
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobCompletionTime = time_now();

#ifdef STVID_VALID_DEC_TIME
	/* Store the time */
	VVID_VC1DEC_TimeStore_Command(PrivateData_p);
#endif

/*
    ST_ErrorCode =  MME_SendCommand(PrivateData_p->TranformHandleDecoder, &PrivateData_p->CmdDecoder);
*/

 /*   task_delay(10000000)*/;
#ifdef VC1_DUMP_AFTER_DECODE
    fmw_p = &PrivateData_p->TransformParam;
    FilenameSuffix[0] = 0;

    sprintf(Filename_p, "../../decoded_frames/frameafterdecode%03d_%03d_%03d_%03d%s",
                        0,
                        0,
                        0,
                        VC1_Picture_Number++,
                        FilenameSuffix);
    vidprod_SavePicture(/*OMEGA2_420*/0x94, fmw_p->FrameHorizSize,
                                    fmw_p->FrameVertSize,
                                    fmw_p->DecodedBufferAddress.Luma_p /* LumaAddress */,
                                    fmw_p->DecodedBufferAddress.Chroma_p /* ChromaAddress */,
                                    Filename_p);
 #endif

    STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[VC1DECODER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PrivateData_p->DecoderContext.DECODER_DecodingJobResults));
    /* Decoder State should be cleared after the event notify to avoid producer task to launch a new decode before all the data have been processed */
    /* If decoder state is cleared before the DECODER_JOB_COMPLETED_EVT notification and if the multicom task is low priority (lower than producer) the following scenario may appear :
     * vid_prod.c calls DecodePicture(pic1)
     * vid_prod.c calls ConfirmBuffer(pic1)
     * TransformerCallback_decoder (pic1) => PrivateData_p->DecoderContext.State = xxxDECODER_CONTEXT_NOT_USED
     * vid_prod.c calls DecodePicture(pic2)
     * vid_prod.c calls ConfirmBuffer(pic2)
     * vid_prod.c handles the DecoderJobCompletedCB(for pic1) callback but the PrivateData_p->DecoderContext content has been modified by ConfirmBuffer(pic2) !!!
     * this results in producer will be notified twice with pic2 end of decode and miss pic1 end of decode.
     */
    PrivateData_p->DecoderState = VC1DECODER_CONTEXT_NOT_USED; /* The flag must be reset *after* the DECODER_JOB_COMPLETED_EVT notification */
    /* The DECODER_INFORM_READ_ADDRESS_EVT can be notified before or after the reset of PrivateData_p->DecoderContext.State
     * but if we notify it before it will delay the opportunity for the producer to start a new decode as an
     * CONTROLLER_COMMAND_DECODER_READ_ADDRESS_UPDATED command will be pushed and treated before the producer make a try for the next DecodePicture */
    STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[VC1DECODER_INFORM_READ_ADDRESS_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PrivateData_p->DecoderReadAddress));
}

#ifdef PRINT_DECODER_INPUT

/* debug function to print out information coming in from */
/* the decode picture command.*/
void vc1_PrintDecodePictureCommand(FILE* DebugFile, const DECODER_DecodePictureParams_t *  const DecodePictureParams_p,
								   const DECODER_Handle_t         DecoderHandle)
{
    #if 0
	/*STTBX_Print(("vc1_PrintDecodePictureCommand\n\n"));*/

	int i;
	VIDCOM_GlobalDecodingContextGenericData_t *GenGDC_p;
	VC1_GlobalDecodingContextSpecificData_t   *SpecGDC_p;
	VC1_PictureDecodingContextSpecificData_t  *SpecPDC_p;
	VIDBUFF_PictureBuffer_t					  *PicBuf_p;
	VIDCOM_PictureGenericData_t				  *PicGen_p;/* Picture generic data */
	VC1_PictureSpecificData_t				  *PicSpec_p;/*Picture specific data */


	U8 PicScanTypeDesc[STGXOBJ_INTERLACED_SCAN + 1][20] = {"PROGRESSIVE_SCAN","INTERLACED_SCAN"};
	U8 MPEGFrameTypeDesc[STVID_MPEG_FRAME_B + 1][10] =    {"UNDEFINED","I","P","UNDEFINED","B"};
	U8 PictStructureDesc[STVID_PICTURE_STRUCTURE_FRAME + 1][20] =    {"TOP FIELD","BOTTOM FIELD","FRAME"};

	U8 PicTypeDesc[VC9_PICTURE_TYPE_SKIP + 1][10] = {"UNDEFINED","I","P","B","BI","SKIP"};


    VC1DecoderPrivateData_t        *PrivateData_p;

    PrivateData_p   = (VC1DecoderPrivateData_t *) (((DECODER_Properties_t *) DecoderHandle)->PrivateData_p);


	PicBuf_p  = DecodePictureParams_p->PictureInformation;

	GenGDC_p  = &(PicBuf_p->GlobalDecodingContext.GlobalDecodingContextGenericData);
	SpecGDC_p = (VC1_GlobalDecodingContextSpecificData_t *)(PicBuf_p->GlobalDecodingContextSpecificData);
	SpecPDC_p = (VC1_PictureDecodingContextSpecificData_t *)(PicBuf_p->PictureDecodingContextSpecificData);
	PicGen_p  = &(PicBuf_p->ParsedPictureInformation.PictureGenericData);

	if(PicBuf_p->ParsedPictureInformation.SizeOfPictureSpecificDataInByte !=
		sizeof(VC1_PictureSpecificData_t))
	{
		STTBX_Print(("vc1_PrintDecodePictureCommand: ERROR wrong size of picture specific data.\n"
			         "expecting %d, received %d\n",sizeof(VC1_PictureSpecificData_t),
							PicBuf_p->ParsedPictureInformation.SizeOfPictureSpecificDataInByte));
	}

	PicSpec_p = (VC1_PictureSpecificData_t *)(PicBuf_p->ParsedPictureInformation.PictureSpecificData_p);

	fprintf(DebugFile, "\n **********************   Picture %d   ************************************",PicBuf_p->PictureNumber);

	if(memcmp(&PrivateData_p->PrevGenericGDC, GenGDC_p, sizeof(VIDCOM_GlobalDecodingContextGenericData_t)) != 0)
	{
		fprintf(DebugFile, "\n\n>>>>>>>>>>>>>>>>>  Generic Global  Decoding Context Data  <<<<<<<<<<<<<<<<<<<<\n\n");
		fprintf(DebugFile, "SequenceInfo.Height                    = %d\n",GenGDC_p->SequenceInfo.Height);
		fprintf(DebugFile, "SequenceInfo.Width                     = %d\n",GenGDC_p->SequenceInfo.Width);
		fprintf(DebugFile, "SequenceInfo.Aspect                    = %d\n",GenGDC_p->SequenceInfo.Aspect);
		fprintf(DebugFile, "SequenceInfo.ScanType                  = %d\n",GenGDC_p->SequenceInfo.ScanType);
		fprintf(DebugFile, "SequenceInfo.FrameRate                 = %d\n",GenGDC_p->SequenceInfo.FrameRate);
		fprintf(DebugFile, "SequenceInfo.BitRate                   = %d\n",GenGDC_p->SequenceInfo.BitRate);
		fprintf(DebugFile, "SequenceInfo.MPEGStandard              = %d\n",GenGDC_p->SequenceInfo.MPEGStandard);
		fprintf(DebugFile, "SequenceInfo.IsLowDelay                = %s\n",GenGDC_p->SequenceInfo.IsLowDelay==TRUE?"TRUE":"FALSE");
		fprintf(DebugFile, "SequenceInfo.VBVBufferSize             = %d\n",GenGDC_p->SequenceInfo.VBVBufferSize);
		fprintf(DebugFile, "SequenceInfo.StreamID                  = %d\n",GenGDC_p->SequenceInfo.StreamID);
		fprintf(DebugFile, "SequenceInfo.ProfileAndLevelIndication = %d\n",GenGDC_p->SequenceInfo.ProfileAndLevelIndication);
		fprintf(DebugFile, "SequenceInfo.VideoFormat               = %d\n",GenGDC_p->SequenceInfo.VideoFormat);
		fprintf(DebugFile, "ColourPrimaries                        = %d\n",GenGDC_p->ColourPrimaries);
		fprintf(DebugFile, "TransferCharacteristics                = %d\n",GenGDC_p->TransferCharacteristics);
		fprintf(DebugFile, "MatrixCoefficients                     = %d\n",GenGDC_p->MatrixCoefficients);
		fprintf(DebugFile, "FrameCropInPixel.LeftOffset            = %d\n",GenGDC_p->FrameCropInPixel.LeftOffset);
		fprintf(DebugFile, "FrameCropInPixel.RightOffset           = %d\n",GenGDC_p->FrameCropInPixel.RightOffset);
		fprintf(DebugFile, "FrameCropInPixel.TopOffset             = %d\n",GenGDC_p->FrameCropInPixel.TopOffset);
		fprintf(DebugFile, "FrameCropInPixel.BottomOffset          = %d\n",GenGDC_p->FrameCropInPixel.BottomOffset);
		fprintf(DebugFile, "NumberOfReferenceFrames                = %d\n",GenGDC_p->NumberOfReferenceFrames);
		fprintf(DebugFile, "MaxDecFrameBuffering                   = %d\n",GenGDC_p->MaxDecFrameBuffering);

		PrivateData_p->PrevGenericGDC = *GenGDC_p;
	}

	if(memcmp(&PrivateData_p->PrevSpecificGDC, SpecGDC_p, sizeof(VC1_GlobalDecodingContextSpecificData_t)) != 0)
	{
		fprintf(DebugFile, "\n>>>>>>>>>>>>>>>>>  Specific  Global  Decoding Context Data  <<<<<<<<<<<<<<<<<<<<\n\n");
		fprintf(DebugFile, "QuantizedFrameRateForPostProcessing = %d\n",SpecGDC_p->QuantizedFrameRateForPostProcessing);
		fprintf(DebugFile, "QuantizedBitRateForPostProcessing   = %d\n",SpecGDC_p->QuantizedBitRateForPostProcessing);
		fprintf(DebugFile, "PostProcessingFlag                  = %s\n",SpecGDC_p->PostProcessingFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "PulldownFlag						= %s\n",SpecGDC_p->PulldownFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "InterlaceFlag						= %s\n",SpecGDC_p->InterlaceFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "TfcntrFlag                          = %s\n",SpecGDC_p->TfcntrFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "FinterpFlag                         = %s\n",SpecGDC_p->FinterpFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "ProgressiveSegmentedFrame           = %s\n",SpecGDC_p->ProgressiveSegmentedFrame == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "AspectNumerator                     = %d\n",SpecGDC_p->AspectNumerator);
		fprintf(DebugFile, "AspectDenominator                   = %d\n",SpecGDC_p->AspectDenominator);
		fprintf(DebugFile, "MultiresFlag                        = %d\n",SpecGDC_p->MultiresFlag);
		fprintf(DebugFile, "AebrFlag                            = %d\n",SpecGDC_p->AebrFlag);
		fprintf(DebugFile, "SyncmarkerFlag                      = %d\n",SpecGDC_p->SyncmarkerFlag);
		fprintf(DebugFile, "RangeredFlag                        = %d\n",SpecGDC_p->RangeredFlag);
		fprintf(DebugFile, "Maxbframes                          = %d\n",SpecGDC_p->Maxbframes);
		fprintf(DebugFile, "ABSHrdBuffer                        = %d\n",SpecGDC_p->ABSHrdBuffer);
		fprintf(DebugFile, "ABSHrdRate                          = %d\n",SpecGDC_p->ABSHrdRate);
		fprintf(DebugFile, "NumLeakyBuckets                     = %d\n",SpecGDC_p->NumLeakyBuckets);
		fprintf(DebugFile, "BitRateExponent						= %d\n",SpecGDC_p->BitRateExponent);
		fprintf(DebugFile, "BufferSizeExponent                  = %d\n",SpecGDC_p->BufferSizeExponent);

		fprintf(DebugFile, "\n         ---------HRD PARAMS--------\n");

		if((SpecGDC_p->NumLeakyBuckets > 0) && (SpecGDC_p->NumLeakyBuckets <= MAX_NUM_LEAKY_BUCKETS))
		{
			for(i = 0; i < SpecGDC_p->NumLeakyBuckets; i++)
			{
				fprintf(DebugFile, "HrdRate    %d					    = %d\n", i, SpecGDC_p->HrdBufferParams[i].HrdRate);
				fprintf(DebugFile, "HrdBuffer  %d					    = %d\n", i, SpecGDC_p->HrdBufferParams[i].HrdBuffer);
			}
		}

		PrivateData_p->PrevSpecificGDC = *SpecGDC_p;
	}/* end if(memcmp(&PrivateData_p->PrevSpecificGDC, SpecGDC_p, sizeof(VC1_GlobalDecodingContextSpecificData_t)) != 0)*/

	if(memcmp(&PrivateData_p->PrevSpecificPDC, SpecPDC_p, sizeof(VC1_PictureDecodingContextSpecificData_t)) != 0)
	{
		fprintf(DebugFile, "\n>>>>>>>>>>>>>>>>>  Specific Picture Decoding Context Data  <<<<<<<<<<<<<<<<<<<<\n\n");
		fprintf(DebugFile, "PanScanFlag     = %s\n",SpecPDC_p->PanScanFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "RefDistFlag     = %s\n",SpecPDC_p->RefDistFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "LoopFilterFlag  = %s\n",SpecPDC_p->LoopFilterFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "Fastuvmc        = %s\n",SpecPDC_p->Fastuvmc == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "ExtendedMVFlag  = %s\n",SpecPDC_p->ExtendedMVFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "Dquant          = %d\n",SpecPDC_p->Dquant);
		fprintf(DebugFile, "VstransformFlag = %s\n",SpecPDC_p->VstransformFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "OverlapFlag     = %s\n",SpecPDC_p->OverlapFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "Quantizer       = %d\n",SpecPDC_p->Quantizer);
		fprintf(DebugFile, "CodedWidth      = %d\n",SpecPDC_p->CodedWidth);
		fprintf(DebugFile, "CodedHeight     = %d\n",SpecPDC_p->CodedHeight);
		fprintf(DebugFile, "ExtendedDMVFlag = %s\n",SpecPDC_p->ExtendedDMVFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "Range_MAPYFlag  = %s\n",SpecPDC_p->Range_MAPYFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "Range_MAPY      = %d\n",SpecPDC_p->Range_MAPY);
		fprintf(DebugFile, "Range_MAPUVFlag = %s\n",SpecPDC_p->Range_MAPUVFlag == TRUE ? "TRUE":"FALSE");
		fprintf(DebugFile, "Range_MAPUV     = %d\n",SpecPDC_p->Range_MAPUV);

		if((SpecGDC_p->NumLeakyBuckets > 0) && (SpecGDC_p->NumLeakyBuckets <= MAX_NUM_LEAKY_BUCKETS))
		{
			fprintf(DebugFile, "\n         ---------HRDFullness--------\n");

			for(i=0; i < SpecGDC_p->NumLeakyBuckets; i++)
			{
				fprintf(DebugFile,"          HRDFullness %d = %d\n", i, SpecPDC_p->HRDFullness[i]);
			}
		}

		PrivateData_p->PrevSpecificPDC = *SpecPDC_p;
	}/* end if(memcmp(&PrivateData_p->PrevSpecificGDC, SpecGDC_p, sizeof(VC1_GlobalDecodingContextSpecificData_t)) != 0)*/

	fprintf(DebugFile, "\n>>>>>>>>>>>>>>>>>  Generic  Picture Data  <<<<<<<<<<<<<<<<<<<<\n\n");
	fflush(DebugFile);
	fprintf(DebugFile, "PictureInfos.VideoParams.FrameRate             = %d\n",PicGen_p->PictureInfos.VideoParams.FrameRate);
	fflush(DebugFile);

	if(PicGen_p->PictureInfos.VideoParams.ScanType <= STGXOBJ_INTERLACED_SCAN)
		fprintf(DebugFile, "PictureInfos.VideoParams.ScanType              = %s\n",PicScanTypeDesc[PicGen_p->PictureInfos.VideoParams.ScanType]);
	else
		fprintf(DebugFile, "INVALID PictureInfos.VideoParams.ScanType              = %u\n",PicGen_p->PictureInfos.VideoParams.ScanType);

	fflush(DebugFile);

	if(PicGen_p->PictureInfos.VideoParams.MPEGFrame <= STVID_MPEG_FRAME_B)
		fprintf(DebugFile, "PictureInfos.VideoParams.MPEGFrame             = %s\n",MPEGFrameTypeDesc[PicGen_p->PictureInfos.VideoParams.MPEGFrame]);
	else
		fprintf(DebugFile, "INVALID PictureInfos.VideoParams.MPEGFrame             = %u\n",PicGen_p->PictureInfos.VideoParams.MPEGFrame);

	fflush(DebugFile);

	if(PicGen_p->PictureInfos.VideoParams.PictureStructure <= STVID_PICTURE_STRUCTURE_FRAME)
		fprintf(DebugFile, "PictureInfos.VideoParams.PictureStructure      = %s\n",PictStructureDesc[PicGen_p->PictureInfos.VideoParams.PictureStructure]);
	else
		fprintf(DebugFile, "INVALID PictureInfos.VideoParams.PictureStructure      = %u\n",PicGen_p->PictureInfos.VideoParams.PictureStructure);

	fprintf(DebugFile, "PictureInfos.VideoParams.TopFieldFirst         = %s\n",PicGen_p->PictureInfos.VideoParams.TopFieldFirst == TRUE ? "TRUE" : "FALSE");
	fprintf(DebugFile, "PictureInfos.VideoParams.TimeCode.Hours        = %d\n",PicGen_p->PictureInfos.VideoParams.TimeCode.Hours);
	fprintf(DebugFile, "PictureInfos.VideoParams.TimeCode.Minutes      = %d\n",PicGen_p->PictureInfos.VideoParams.TimeCode.Minutes);
	fprintf(DebugFile, "PictureInfos.VideoParams.TimeCode.Seconds      = %d\n",PicGen_p->PictureInfos.VideoParams.TimeCode.Seconds);
	fprintf(DebugFile, "PictureInfos.VideoParams.TimeCode.Frames       = %d\n",PicGen_p->PictureInfos.VideoParams.TimeCode.Frames);
	fprintf(DebugFile, "PictureInfos.VideoParams.TimeCode.Interpolated = %s\n",PicGen_p->PictureInfos.VideoParams.TimeCode.Interpolated == TRUE ? "TRUE" : "FALSE");

    fprintf(DebugFile, "PictureInfos.VideoParams.PTS.IsValid           = %s\n",PicGen_p->PictureInfos.VideoParams.PTS.IsValid == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "PictureInfos.VideoParams.PTS.PTS               = %d\n",PicGen_p->PictureInfos.VideoParams.PTS.PTS);
	fprintf(DebugFile, "PictureInfos.VideoParams.PTS.PTS33             = %s\n",
					PicGen_p->PictureInfos.VideoParams.PTS.PTS33 == TRUE ? "TRUE" : "FALSE");
	fprintf(DebugFile, "PictureInfos.VideoParams.PTS.Interpolated      = %s\n",
					PicGen_p->PictureInfos.VideoParams.PTS.Interpolated == TRUE ? "TRUE" : "FALSE");

	fprintf(DebugFile, "PictureInfos.VideoParams.CompressionLevel      = %d\n",PicGen_p->PictureInfos.VideoParams.CompressionLevel);
	fprintf(DebugFile, "PictureInfos.VideoParams.DecimationFactors     = %d\n",PicGen_p->PictureInfos.VideoParams.DecimationFactors);
	fprintf(DebugFile, "RepeatFirstField                               = %s\n",PicGen_p->RepeatFirstField == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "RepeatProgressiveCounter                       = %d\n",PicGen_p->RepeatProgressiveCounter);
	fprintf(DebugFile, "DecodingOrderFrameID                           = %d\n",PicGen_p->DecodingOrderFrameID);
	fprintf(DebugFile, "ExtendedPresentationOrderPictureID.ID          = %d\n",PicGen_p->ExtendedPresentationOrderPictureID.ID);
	fprintf(DebugFile, "ExtendedPresentationOrderPictureID.IDExtension = %d\n",PicGen_p->ExtendedPresentationOrderPictureID.IDExtension);
	fprintf(DebugFile, "IsDisplayBoundPictureIDValid                   = %s\n",PicGen_p->IsDisplayBoundPictureIDValid == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "DisplayBoundPictureID.ID                       = %d\n",PicGen_p->DisplayBoundPictureID.ID);
	fprintf(DebugFile, "DisplayBoundPictureID.IDExtension              = %d\n",PicGen_p->DisplayBoundPictureID.IDExtension);
	fprintf(DebugFile, "IsFirstOfTwoFields                             = %s\n",PicGen_p->IsFirstOfTwoFields == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "IsReference                                    = %s\n",PicGen_p->IsReference == TRUE ? "TRUE":"FALSE");

	fprintf(DebugFile, "NumberOfPanAndScan                             = %d\n",PicGen_p->NumberOfPanAndScan);

	if((PicGen_p->NumberOfPanAndScan > 0) && (PicGen_p->NumberOfPanAndScan <= VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN))
	{
		fprintf(DebugFile, "\n     -------------Pan and Scan Info Array--------\n");

		for(i=0; i < PicGen_p->NumberOfPanAndScan; i++)
		{
			fprintf(DebugFile, "\n         ------------Pan & Scan Info %d----------\n",i);
			fprintf(DebugFile, "             FrameCentreHorizontalOffset  = %d\n",PicGen_p->PanAndScanIn16thPixel[i].FrameCentreHorizontalOffset);
			fprintf(DebugFile, "             FrameCentreVerticalOffset    = %d\n",PicGen_p->PanAndScanIn16thPixel[i].FrameCentreVerticalOffset);
			fprintf(DebugFile, "             DisplayHorizontalSize        = %u\n",PicGen_p->PanAndScanIn16thPixel[i].DisplayHorizontalSize);
			fprintf(DebugFile, "             DisplayVerticalSize          = %u\n",PicGen_p->PanAndScanIn16thPixel[i].DisplayVerticalSize);
			fprintf(DebugFile, "             HasDisplaySizeRecommendation = %s\n",PicGen_p->PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation == TRUE ? "TRUE":"FALSE");
		}
	}


	fprintf(DebugFile, "\n     -------------Full Reference Frame List --------\n");
	for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
	{
		if(PicGen_p->IsValidIndexInReferenceFrame[i] == TRUE)
		{
			fprintf(DebugFile, "\n         ------------Reference Frame %d----------\n",i);
			fprintf(DebugFile, "             FRAME %s\n",PicGen_p->IsValidIndexInReferenceFrame[i] == TRUE ? "VALID":"NOT VALID");
			fprintf(DebugFile, "             %s\n",PicGen_p->IsBrokenLinkIndexInReferenceFrame[i] == TRUE ? "BROKEN LINK":"NOT BROKEN LINK");
			fprintf(DebugFile, "             DecodingOrderFrameID    = %d\n",PicGen_p->FullReferenceFrameList[i]);
		}/* end if(PicGen_p->IsValidIndexInReferenceFrame[i] == TRUE)*/
	}

	fprintf(DebugFile, "\nUsedSizeInReferenceListP0 = %d\n",PicGen_p->UsedSizeInReferenceListP0);

	fprintf(DebugFile, "Reference List P0:   ");
	for(i = 0; i < PicGen_p->UsedSizeInReferenceListP0; i++)
	{
		fprintf(DebugFile, "%d  ",PicGen_p->ReferenceListP0[i]);
	}
	fprintf(DebugFile, "\n\n");

	fprintf(DebugFile, "UsedSizeInReferenceListB0 = %d\n",PicGen_p->UsedSizeInReferenceListB0);

	fprintf(DebugFile, "Reference List B0:   ");
	for(i = 0; i < PicGen_p->UsedSizeInReferenceListB0; i++)
	{
		fprintf(DebugFile, "%d  ",PicGen_p->ReferenceListB0[i]);
	}
	fprintf(DebugFile, "\n\n");

	fprintf(DebugFile, "UsedSizeInReferenceListB1 = %d\n",PicGen_p->UsedSizeInReferenceListB1);

	fprintf(DebugFile, "Reference List B1:   ");
	for(i = 0; i < PicGen_p->UsedSizeInReferenceListB1; i++)
	{
		fprintf(DebugFile, "%d  ",PicGen_p->ReferenceListB1[i]);
	}
	fprintf(DebugFile, "\n\n");

	fprintf(DebugFile, "BitBufferPictureStartAddress                       = %8x\n",PicGen_p->BitBufferPictureStartAddress);
	fprintf(DebugFile, "BitBufferPictureStopAddress                        = %8x\n",PicGen_p->BitBufferPictureStopAddress);

	fprintf(DebugFile, "AsynchronousCommands.FlushPresentationQueue        = %s\n",
						PicGen_p->AsynchronousCommands.FlushPresentationQueue == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "AsynchronousCommands.FreezePresentationOnThisFrame = %s\n",
						PicGen_p->AsynchronousCommands.FreezePresentationOnThisFrame == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "AsynchronousCommands.ResumePresentationOnThisFrame = %s\n",
						PicGen_p->AsynchronousCommands.ResumePresentationOnThisFrame == TRUE ? "TRUE":"FALSE");

	fprintf(DebugFile, "IsDecodeStartTimeValid                             = %s\n",PicGen_p->IsDecodeStartTimeValid == TRUE ? "TRUE":"FALSE");
    /*print  PicGen_p->DecodeStartTime;(osclock_t) here */
	fprintf(DebugFile, "IsPresentationStartTimeValid                       = %s\n",PicGen_p->IsPresentationStartTimeValid == TRUE ? "TRUE":"FALSE");
	/*print  PicGen_p->PresentationStartTime;(osclock_t) here */


	fprintf(DebugFile, "\n>>>>>>>>>>>>>>>>>  Specific Picture Data  <<<<<<<<<<<<<<<<<<<<\n\n");
	fprintf(DebugFile, "RndCtrl                = %d\n",PicSpec_p->RndCtrl);
	fprintf(DebugFile, "BackwardRefDist        = %d\n",PicSpec_p->BackwardRefDist);

	if(PicSpec_p->PictureType <= VC9_PICTURE_TYPE_SKIP)
		fprintf(DebugFile, "PictureType            = %s\n",PicTypeDesc[PicSpec_p->PictureType]);
	else
		fprintf(DebugFile, "INVALID PictureType            = %d\n",PicSpec_p->PictureType);

	fprintf(DebugFile, "ForwardRangeredfrmFlag = %s\n",PicSpec_p->ForwardRangeredfrmFlag == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "HalfWidthFlag          = %s\n",PicSpec_p->HalfWidthFlag  == TRUE ? "TRUE":"FALSE");
	fprintf(DebugFile, "HalfHeightFlag         = %s\n",PicSpec_p->HalfHeightFlag == TRUE ? "TRUE":"FALSE");


	fprintf(DebugFile, "IntComp.ForwTop_1      = %08X\n",PicSpec_p->IntComp.ForwTop_1);
	fprintf(DebugFile, "IntComp.ForwTop_2      = %08X\n",PicSpec_p->IntComp.ForwTop_2);
	fprintf(DebugFile, "IntComp.ForwBot_1      = %08X\n",PicSpec_p->IntComp.ForwBot_1);
	fprintf(DebugFile, "IntComp.ForwBot_2      = %08X\n",PicSpec_p->IntComp.ForwBot_2);
	fprintf(DebugFile, "IntComp.BackTop_1      = %08X\n",PicSpec_p->IntComp.BackTop_1);
	fprintf(DebugFile, "IntComp.BackTop_2      = %08X\n",PicSpec_p->IntComp.BackTop_2);
	fprintf(DebugFile, "IntComp.BackBot_1      = %08X\n",PicSpec_p->IntComp.BackBot_1);
	fprintf(DebugFile, "IntComp.BackBot_2      = %08X\n",PicSpec_p->IntComp.BackBot_2);

	fprintf(DebugFile, "NumSlices              = %d\n",PicSpec_p->NumSlices);

	if((PicSpec_p->NumSlices > 0) && (PicSpec_p->NumSlices <= MAX_SLICE))
	{
		fprintf(DebugFile, "\n         ---------Slice Info Array--------\n");

		for(i=0; i < PicSpec_p->NumSlices; i++)
		{
			fprintf(DebugFile, "\n         ------------Slice %d----------\n",i);
			fprintf(DebugFile, "             SliceStartAddrCompressedBuffer_p = %8x\n",PicSpec_p->SliceArray[i].SliceStartAddrCompressedBuffer_p);
			fprintf(DebugFile, "             SliceAddress                     = %8x\n",PicSpec_p->SliceArray[i].SliceAddress);
		}
	}

	fprintf(DebugFile, "\nIsReferenceTopFieldP0 Array:  ");
	for(i=0; i < PicGen_p->UsedSizeInReferenceListP0; i++)
	{
		fprintf(DebugFile, "  %s",PicSpec_p->IsReferenceTopFieldP0[i] == TRUE ? "TOP FIELD" : "BOTTOM FIELD");
	}

	fprintf(DebugFile, "\nIsReferenceTopFieldB0 Array:  ");
	for(i=0; i < PicGen_p->UsedSizeInReferenceListB0; i++)
	{
		fprintf(DebugFile, "  %s",PicSpec_p->IsReferenceTopFieldB0[i] == TRUE ? "TOP FIELD" : "BOTTOM FIELD");
	}

	fprintf(DebugFile, "\nIsReferenceTopFieldB1 Array:  ");
	for(i=0; i < PicGen_p->UsedSizeInReferenceListB1; i++)
	{
		fprintf(DebugFile, "  %s",PicSpec_p->IsReferenceTopFieldB1[i] == TRUE ? "TOP FIELD" : "BOTTOM FIELD");
	}

	fprintf(DebugFile, "\n\n");
	fflush(DebugFile);
#endif

}
#endif /*PRINT_DECODER_INPUT*/



/*******************************************************************************
Name        : VC1DECODER_DecodePicture
Description :
Parameters  : DecoderHandle
 *            DecodePictureParams_p
 *            DecoderStatus_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VC1DECODER_DecodePicture (const DECODER_Handle_t         DecoderHandle,
                                          const DECODER_DecodePictureParams_t *  const DecodePictureParams_p,
                                          DECODER_Status_t *       const DecoderStatus_p)
{
    ST_ErrorCode_t                  ST_ErrorCode = ST_NO_ERROR;

/*TD    U32                             iBuff, Command;*/
    DECODER_Properties_t            *DECODER_Data_p;
    VC9_TransformParam_t            *fmw_p;
    VIDCOM_ParsedPictureInformation_t *ParsedPicture_p;
    VC1_PictureSpecificData_t      *PictSpecData_p;
    VIDCOM_PictureGenericData_t     *PictGenData_p;
    VC1DecoderPrivateData_t        *PrivateData_p;
    VC1_PictureDecodingContextSpecificData_t  * PDCSpecData_p;
    VC1_GlobalDecodingContextSpecificData_t * GDCSpecData_p;
    STVID_DecimationFactor_t        DecimationFactors;
    STVID_SequenceInfo_t            *SequenceInfo_p;
    unsigned int                    i;
/*TD    STVID_SetupParams_t             SetupParams;*/

#ifndef STUB_VC1_DECODER


    if ((DecoderHandle == NULL) || (DecodePictureParams_p == NULL))
    {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_DecodePicture) : DecoderHandle %x  DecodePictureParams_p %x ", DecoderHandle, DecodePictureParams_p));
        return(ST_ERROR_BAD_PARAMETER);
    }

/*TD    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);*/

        /* Pointers initialisation */
    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (PrivateData_p->DecoderState != VC1DECODER_CONTEXT_NOT_USED)
    {
        DecoderStatus_p->State = DECODER_STATE_DECODING;
        DecoderStatus_p->IsJobSubmitted = FALSE;
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = time_now();
        return(ST_ERROR_NO_MEMORY);
    }
    PrivateData_p->DecoderState = VC1DECODER_CONTEXT_DECODE_FUNC;
    ParsedPicture_p = &(DecodePictureParams_p->PictureInformation->ParsedPictureInformation);
    fmw_p = &PrivateData_p->TransformParam;
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;

    PDCSpecData_p   = ParsedPicture_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p;
    GDCSpecData_p   = ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);
    SequenceInfo_p  = &(ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo);

#ifdef STVID_DEBUG_GET_STATUS
	PrivateData_p->Status.SequenceInfo_p = SequenceInfo_p;
#endif /* STVID_DEBUG_GET_STATUS */

#ifdef PRINT_DECODER_INPUT
        vc1_PrintDecodePictureCommand(PrivateData_p->DebugFile, DecodePictureParams_p, DecoderHandle);
#endif  /*PRINT_DECODER_INPUT*/

    fmw_p->CircularBufferBeginAddr_p = (U32 *) PrivateData_p->BitBufferAddress_p;
    fmw_p->CircularBufferEndAddr_p = (U32 *) ((U32)PrivateData_p->BitBufferAddress_p + (U32)PrivateData_p->BitBufferSize - 1);
    fmw_p->PictureStartAddrCompressedBuffer_p = (U32 *) PictGenData_p->BitBufferPictureStartAddress;
    fmw_p->PictureStopAddrCompressedBuffer_p = (U32 *) PictGenData_p->BitBufferPictureStopAddress;

    /* TODO : MainAuxEnable management */
    fmw_p->MainAuxEnable = VC9_MAINOUT_EN;

    /* TODO */
    fmw_p->DecodingMode = VC9_NORMAL_DECODE;
    fmw_p->AebrFlag = GDCSpecData_p->AebrFlag;
    /*TODO 0,4,12,3 ???what are the values for the profile for VC1 simple, main and Advanced*/
    fmw_p->Profile = ((SequenceInfo_p->ProfileAndLevelIndication >> 8) & 0xFF);
    fmw_p->AdditionalFlags = 0;

    fmw_p->HorizontalDecimationFactor   = VC9_HDEC_1;
    fmw_p->VerticalDecimationFactor     = VC9_VDEC_1;
    DecimationFactors = STVID_DECIMATION_FACTOR_NONE;

    if (DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p != NULL)
    {
        DecimationFactors = DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;

        if (DecimationFactors & STVID_DECIMATION_FACTOR_H2)
        {
            fmw_p->HorizontalDecimationFactor   = VC9_HDEC_2;
            DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation = FALSE;
        }
        if (DecimationFactors & STVID_DECIMATION_FACTOR_H4)
        {
            fmw_p->HorizontalDecimationFactor   = VC9_HDEC_4;
            DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation = FALSE;
        }

        if (DecimationFactors & STVID_DECIMATION_FACTOR_V2)
        {
            if((DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
            || (DecodePictureParams_p->PictureInformation->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD))
            {
                fmw_p->VerticalDecimationFactor   =  VC9_VDEC_2_INT;
            }
            else
            {
                fmw_p->VerticalDecimationFactor   = VC9_VDEC_2_PROG;
            }
        }
    }

    #if 0
    if (ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        fmw_p->PictureSyntax = VC9_PICTURE_SYNTAX_PROGRESSIVE;
    else if ((ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
            && (ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams.ScanType == STGXOBJ_INTERLACED_SCAN))
        fmw_p->PictureSyntax = VC9_PICTURE_SYNTAX_INTERLACED_FRAME;
    else if (ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
        fmw_p->PictureSyntax = VC9_PICTURE_SYNTAX_INTERLACED_FIELD;
    #endif
    fmw_p->PictureSyntax = PictSpecData_p->PictureSyntax;

    fmw_p->PictureType = PictSpecData_p->PictureType;
    fmw_p->FinterpFlag = GDCSpecData_p->FinterpFlag;
    fmw_p->FrameHorizSize = SequenceInfo_p->Width;
    fmw_p->FrameVertSize = SequenceInfo_p->Height;

    fmw_p->IntensityComp = PictSpecData_p->IntComp;
    fmw_p->MultiresFlag = GDCSpecData_p->MultiresFlag;
    fmw_p->HalfWidthFlag = PictSpecData_p->HalfWidthFlag;
    fmw_p->HalfHeightFlag = PictSpecData_p->HalfHeightFlag;
    fmw_p->SyncmarkerFlag = GDCSpecData_p->SyncmarkerFlag;
    fmw_p->RangeredFlag = GDCSpecData_p->RangeredFlag;
    fmw_p->Maxbframes = GDCSpecData_p->Maxbframes;
    fmw_p->ForwardRangeredfrmFlag = PictSpecData_p->ForwardRangeredfrmFlag;
    fmw_p->RndCtrl = PictSpecData_p->RndCtrl;
    fmw_p->NumeratorBFraction = PictSpecData_p->NumeratorBFraction;
    fmw_p->DenominatorBFraction = PictSpecData_p->DenominatorBFraction;

    fmw_p->PostprocFlag = GDCSpecData_p->PostProcessingFlag;
    fmw_p->PulldownFlag = GDCSpecData_p->PulldownFlag;
    fmw_p->InterlaceFlag = GDCSpecData_p->InterlaceFlag;
    fmw_p->PsfFlag = GDCSpecData_p->ProgressiveSegmentedFrame;
    fmw_p->TfcntrFlag = GDCSpecData_p->TfcntrFlag;
    fmw_p->BackwardRefdist = PictSpecData_p->BackwardRefDist;

    fmw_p->EntryPoint.LoopfilterFlag = PDCSpecData_p->LoopFilterFlag;
    fmw_p->EntryPoint.FastuvmcFlag = PDCSpecData_p->Fastuvmc;
    fmw_p->EntryPoint.ExtendedmvFlag = PDCSpecData_p->ExtendedMVFlag;
    fmw_p->EntryPoint.Dquant = PDCSpecData_p->Dquant;
    fmw_p->EntryPoint.VstransformFlag = PDCSpecData_p->VstransformFlag;
    fmw_p->EntryPoint.OverlapFlag = PDCSpecData_p->OverlapFlag;
    fmw_p->EntryPoint.Quantizer = PDCSpecData_p->Quantizer;
    fmw_p->EntryPoint.ExtendedDmvFlag = PDCSpecData_p->ExtendedDMVFlag;
    fmw_p->EntryPoint.PanScanFlag = PDCSpecData_p->PanScanFlag;
    fmw_p->EntryPoint.RefdistFlag = PDCSpecData_p->RefDistFlag;

    fmw_p->StartCodes.SliceCount = PictSpecData_p->NumSlices;
    for (i=0; (i<MAX_SLICE)&&(i<PictSpecData_p->NumSlices); i++)
    {
        fmw_p->StartCodes.SliceArray[i] = PictSpecData_p->SliceArray[i];
    }
    fmw_p->FramePictureLayerSize = PictSpecData_p->FramePictureLayerSize;
    fmw_p->TffFlag = PictSpecData_p->TffFlag;
    fmw_p->SecondFieldFlag = PictSpecData_p->SecondFieldFlag;
    fmw_p->RefDist = PictSpecData_p->RefDist;
#endif

    PrivateData_p->DecoderReadAddress.DecoderReadAddress_p = (U32 *) PictGenData_p->BitBufferPictureStopAddress;

    PrivateData_p->CommandId = PrivateData_p->NextCommandId;
    PrivateData_p->NextCommandId++;

    DecoderStatus_p->DecodingJobStatus.JobSubmissionTime = STOS_time_now();
    DecoderStatus_p->State = DECODER_STATE_READY_TO_DECODE;
    DecoderStatus_p->IsJobSubmitted = TRUE;
    DecoderStatus_p->CommandId = PrivateData_p->CommandId;
    DecoderStatus_p->DecodingJobStatus.JobState = DECODER_JOB_WAITING_CONFIRM_BUFFER;
    return(ST_ErrorCode);
}

/*******************************************************************************
Name        : VC1DECODER_ConfirmBuffer
Description :
Parameters  : DecoderHandle
 *            ConfirmBufferParams_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VC1DECODER_ConfirmBuffer(const DECODER_Handle_t DecoderHandle,
                                     const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p)
{
    ST_ErrorCode_t ST_ErrorCode     = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    VC1DecoderPrivateData_t *       PrivateData_p;
    U32                             iField, iFrame;
    STVID_DecimationFactor_t        DecimationFactors;
    VC9_TransformParam_t            *fmw_p;
    VIDBUFF_FrameBuffer_t           *ReferenceFrameBuff_p;
    BOOL                            IsDecOrderFrameIDFound;
    VIDCOM_ParsedPictureInformation_t *ParsedPicture_p;
    VIDBUFF_PictureBuffer_t         *PictureBuffer_p;
    VC1_PictureSpecificData_t       *PictSpecData_p;
    VIDCOM_PictureGenericData_t     *PictGenData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    ParsedPicture_p = &(ConfirmBufferParams_p->NewPictureBuffer_p->ParsedPictureInformation);
    fmw_p = &PrivateData_p->TransformParam;
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;
    PictureBuffer_p = ConfirmBufferParams_p->NewPictureBuffer_p;
    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);


    if (ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_ConfirmBuffer) :  FrameBuff_p = NULL"));
        ST_ErrorCode = ST_ERROR_NO_MEMORY;
        return(ST_ErrorCode);

    }
    PrivateData_p->DecoderState = VC1DECODER_CONTEXT_CONFIRM_BUFFER_FUNC;

    fmw_p->DecodedBufferAddress.Luma_p              = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address_p;
    fmw_p->DecodedBufferAddress.Chroma_p            = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p;
    fmw_p->DecodedBufferAddress.MBStruct_p          = ConfirmBufferParams_p->NewPictureBuffer_p->PPB.Address_p;


    if (ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        fmw_p->DecodedBufferAddress.LumaDecimated_p     = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p;
        fmw_p->DecodedBufferAddress.ChromaDecimated_p   = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p;
        fmw_p->MainAuxEnable = VC9_AUX_MAIN_OUT_EN;
    }
    else
    {
        fmw_p->DecodedBufferAddress.LumaDecimated_p     = NULL;
        fmw_p->DecodedBufferAddress.ChromaDecimated_p   = NULL;
        fmw_p->MainAuxEnable = VC9_MAINOUT_EN;
    }

	PictureBuffer_p = ConfirmBufferParams_p->NewPictureBuffer_p;
	ParsedPicture_p = &(PictureBuffer_p->ParsedPictureInformation);
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;
    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);

    fmw_p->HorizontalDecimationFactor   = VC9_HDEC_1;
    fmw_p->VerticalDecimationFactor     = VC9_VDEC_1;
    DecimationFactors = STVID_DECIMATION_FACTOR_NONE;

    if (PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        DecimationFactors = PictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;

        if (DecimationFactors & STVID_DECIMATION_FACTOR_H2)
        {
            fmw_p->HorizontalDecimationFactor   = VC9_HDEC_2;
            PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation = FALSE;
        }
        if (DecimationFactors & STVID_DECIMATION_FACTOR_H4)
        {
            fmw_p->HorizontalDecimationFactor   = VC9_HDEC_4;
            PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->AdvancedHDecimation = FALSE;
        }

        if (DecimationFactors & STVID_DECIMATION_FACTOR_V2)
        {
            if((PictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
            || (PictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD))
            {
                fmw_p->VerticalDecimationFactor   =  VC9_VDEC_2_INT;
            }
            else
            {
                fmw_p->VerticalDecimationFactor   = VC9_VDEC_2_PROG;
            }
        }
    }

    fmw_p->RefPicListAddress.ForwardReferencePictureSyntax   = PictSpecData_p->ForwardReferencePictureSyntax;
    fmw_p->RefPicListAddress.BackwardReferencePictureSyntax   = PictSpecData_p->BackwardReferencePictureSyntax;

    switch (PictSpecData_p->PictureType)
    {
        case VC9_PICTURE_TYPE_P :
        case VC9_PICTURE_TYPE_SKIP :
            IsDecOrderFrameIDFound = FALSE;
            /* RefPicList creation for P0 (Forward) */
            for (iField = iFrame = 0; iFrame < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES && !IsDecOrderFrameIDFound ; iFrame++, iField += 2)
            {
                if (PictGenData_p->IsValidIndexInReferenceFrame[iFrame] && (PictGenData_p->FullReferenceFrameList[iFrame] == PictGenData_p->ReferenceListP0[0]))
                 {
                    ReferenceFrameBuff_p = PictureBuffer_p->ReferencesFrameBufferPointer[iFrame];
                    if (ReferenceFrameBuff_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_ConfirmBuffer) :  ReferenceFrameBuff_p = NULL"));
                    }
                    fmw_p->RefPicListAddress.ForwardReferenceLuma_p          = ReferenceFrameBuff_p->Allocation.Address_p;
                    fmw_p->RefPicListAddress.ForwardReferenceChroma_p        = ReferenceFrameBuff_p->Allocation.Address2_p;
                    /*fmw_p->RefPicListAddress.ForwardReferencePictureSyntax   = PictSpecData_p->ForwardReferencePictureSyntax;*/

                    IsDecOrderFrameIDFound = TRUE;
                }
             }
            if (!IsDecOrderFrameIDFound)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_ConfirmBuffer) : DOFID %d not found for RefPicList P0", PictGenData_p->ReferenceListP0[0]));
                /* Force refs with current frame buffer */
                fmw_p->RefPicListAddress.ForwardReferencePictureSyntax   = PictSpecData_p->PictureSyntax;
                fmw_p->RefPicListAddress.ForwardReferenceLuma_p          = fmw_p->DecodedBufferAddress.Luma_p;
                fmw_p->RefPicListAddress.ForwardReferenceChroma_p        = fmw_p->DecodedBufferAddress.Chroma_p;
            }
            break;
            case VC9_PICTURE_TYPE_B :
            IsDecOrderFrameIDFound = FALSE;
            /* RefPicList creation for B0 (Forward) */
            for (iField = iFrame = 0; iFrame < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES && !IsDecOrderFrameIDFound ; iFrame++, iField += 2)
            {
                if (PictGenData_p->IsValidIndexInReferenceFrame[iFrame] && (PictGenData_p->FullReferenceFrameList[iFrame] == PictGenData_p->ReferenceListB0[0]))
                 {
                    ReferenceFrameBuff_p = PictureBuffer_p->ReferencesFrameBufferPointer[iFrame];
                    if (ReferenceFrameBuff_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_ConfirmBuffer) :  ReferenceFrameBuff_p = NULL"));
                    }
                    fmw_p->RefPicListAddress.ForwardReferenceLuma_p          = ReferenceFrameBuff_p->Allocation.Address_p;
                    fmw_p->RefPicListAddress.ForwardReferenceChroma_p        = ReferenceFrameBuff_p->Allocation.Address2_p;
                    /*fmw_p->RefPicListAddress.ForwardReferencePictureSyntax   = PictSpecData_p->ForwardReferencePictureSyntax;*/

                    IsDecOrderFrameIDFound = TRUE;
                 }
             }
            if (!IsDecOrderFrameIDFound)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_ConfirmBuffer) : DOFID %d not found for RefPicList P0", PictGenData_p->ReferenceListP0[0]));
                /* Force refs with current frame buffer */
                fmw_p->RefPicListAddress.ForwardReferencePictureSyntax   = PictSpecData_p->PictureSyntax;
                fmw_p->RefPicListAddress.ForwardReferenceLuma_p          = fmw_p->DecodedBufferAddress.Luma_p;
                fmw_p->RefPicListAddress.ForwardReferenceChroma_p        = fmw_p->DecodedBufferAddress.Chroma_p;
            }

            /* RefPicList creation for B1 (Backward) */
            IsDecOrderFrameIDFound = FALSE;
            for (iField = iFrame = 0; iFrame < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES && !IsDecOrderFrameIDFound ; iFrame++, iField += 2)
            {
                if (PictGenData_p->IsValidIndexInReferenceFrame[iFrame] && (PictGenData_p->FullReferenceFrameList[iFrame] == PictGenData_p->ReferenceListB1[0]))
                 {
                    ReferenceFrameBuff_p = PictureBuffer_p->ReferencesFrameBufferPointer[iFrame];
                    if (ReferenceFrameBuff_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_ConfirmBuffer) :  ReferenceFrameBuff_p = NULL"));
                    }
                    fmw_p->RefPicListAddress.BackwardReferenceLuma_p          = ReferenceFrameBuff_p->Allocation.Address_p;
                    fmw_p->RefPicListAddress.BackwardReferenceChroma_p        = ReferenceFrameBuff_p->Allocation.Address2_p;
                    fmw_p->RefPicListAddress.BackwardReferenceMBStruct_p      = ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p->PPB.Address_p;
                    /*fmw_p->RefPicListAddress.BackwardReferencePictureSyntax   = PictSpecData_p->BackwardReferencePictureSyntax;*/

                    IsDecOrderFrameIDFound = TRUE;
                 }
             }
            if (!IsDecOrderFrameIDFound)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (VC1DECODER_ConfirmBuffer) : DOFID %d not found for RefPicList P0", PictGenData_p->ReferenceListP0[0]));
                /* Force refs with current frame buffer */
                fmw_p->RefPicListAddress.BackwardReferencePictureSyntax   = PictSpecData_p->PictureSyntax;
                fmw_p->RefPicListAddress.BackwardReferenceLuma_p          = fmw_p->DecodedBufferAddress.Luma_p;
                fmw_p->RefPicListAddress.BackwardReferenceChroma_p        = fmw_p->DecodedBufferAddress.Chroma_p;
                fmw_p->RefPicListAddress.BackwardReferenceMBStruct_p      = fmw_p->DecodedBufferAddress.MBStruct_p;
            }
        break;
        default :
        break;
    }

    PrivateData_p->CmdDecoder.Param_p = (MME_GenericParams_t)fmw_p;
    PrivateData_p->CmdDecoder.StructSize  = sizeof(MME_Command_t);
    PrivateData_p->CmdDecoder.CmdEnd      = MME_COMMAND_END_RETURN_NOTIFY;
    PrivateData_p->CmdDecoder.DueTime     = 0;
    PrivateData_p->CmdDecoder.ParamSize   = sizeof(VC9_TransformParam_t);
    PrivateData_p->CmdDecoder.CmdCode     = MME_TRANSFORM;
    PrivateData_p->CmdDecoder.NumberInputBuffers  = 0;
    PrivateData_p->CmdDecoder.NumberOutputBuffers = 0;
    PrivateData_p->CmdDecoder.CmdStatus.AdditionalInfoSize = sizeof (VC9_CommandStatus_t);
    PrivateData_p->CmdDecoder.CmdStatus.AdditionalInfo_p = &PrivateData_p->VC1CmdStatus;

    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime = time_now();

#if defined(MME_DUMP) && defined(VALID_TOOLS)
    VC1_HDM_DumpCommand(&PrivateData_p->CmdDecoder);
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */
#ifdef STUB_FMW_ST40
    ST_ErrorCode =  MMESTUB_SendCommand(PrivateData_p->TranformHandleDecoder, &PrivateData_p->CmdDecoder);
#else
    ST_ErrorCode =  MME_SendCommand(PrivateData_p->TranformHandleDecoder, &PrivateData_p->CmdDecoder);
#endif

    return(ST_ErrorCode);
}

/*******************************************************************************
Name        : vc1_InitData
Description :
Parameters  : PrivateData_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void vc1_InitData(VC1DecoderPrivateData_t * PrivateData_p)
{
	PrivateData_p->NextCommandId = 500;  /* Arbitrary starting value , not set to 0 for debug purpose */
#if 0 /* TO BE DONE */
	PrivateData_p->NbBufferUsed    = 0;
    PrivateData_p->RejectedConfirmBuffer = 0;
	PrivateData_p->DecoderIsRunning = FALSE;
	PrivateData_p->RejectedSendCommand = 0;
#if defined(STVID_VALID)
    PrivateData_p->NbMMECmdDumped = 0;
#endif /* defined(STVID_VALID) */

    memset (&(PrivateData_p->H264DecoderData[0]), 0, sizeof(PrivateData_p->H264DecoderData));
    memset (&(PrivateData_p->DecoderContext[0]), 0, sizeof(PrivateData_p->DecoderContext));
    memset (&(PrivateData_p->PreProcContext[0]), 0, sizeof(PrivateData_p->PreProcContext));
    memset (&(PrivateData_p->FmwContext[0]), 0, sizeof(PrivateData_p->FmwContext));
#if defined(STVID_VALID)
    memset (&(PrivateData_p->H264CompanionDataDump[0]),                0, sizeof(H264CompanionData_t) * MME_MAX_DUMP);
    memset (&(PrivateData_p->H264_ReferenceFrameAddressDump[0][0]),    0, sizeof(H264_ReferenceFrameAddress_t) * MME_MAX_DUMP * VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS);
    memset (&(PrivateData_p->InitialPList0Dump[0][0]),                 0, sizeof(U32) * MME_MAX_DUMP * VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS);
    memset (&(PrivateData_p->InitialBList0Dump[0][0]),                 0, sizeof(U32) * MME_MAX_DUMP * VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS);
    memset (&(PrivateData_p->InitialBList1Dump[0][0]),                 0, sizeof(U32) * MME_MAX_DUMP * VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS);
#endif /* defined(STVID_VALID) */
    memset (&(PrivateData_p->CommandsData[0]), 0, sizeof(PrivateData_p->CommandsData));
    PrivateData_p->H264CompanionDataSetupIsDone = FALSE;
    PrivateData_p->H264CompanionData_p = NULL;
#endif /* 0 TO BE DONE */
}

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
ST_ErrorCode_t VC1DECODER_Init(const DECODER_Handle_t DecoderHandle, const DECODER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *                  DECODER_Data_p;
    VC1DecoderPrivateData_t *               PrivateData_p;
    MME_TransformerInitParams_t             initParams_decoder;
    EMBX_TRANSPORT                          tp;
    EMBX_ERROR                              EMBXError;
    U32                                     DeviceRegistersBaseAddress;
    char                                    TransformerName[MME_MAX_TRANSFORMER_NAME];
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U8                                      TryCount=0;
#if 0 /* TO BE DONE */
    U8                                      iBuff;
#endif /* 0 TO BE DONE */

	if ((DecoderHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;

    /* Allocate DECODER PrivateData structure */
    DECODER_Data_p->PrivateData_p = (VC1DecoderPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VC1DecoderPrivateData_t));
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
	if (PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    PrivateData_p->IsEmbxTransportValid = FALSE;

#ifndef STUB_FMW_ST40

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

    /* Allocate Host/LX shared data in EMBX shared memory to allow Multicom to perform Address translation */
#if defined(ST_7100) || defined(ST_7109)
    EMBXError = EMBX_OpenTransport(MME_VIDEO_TRANSPORT_NAME, &tp);
#elif defined(ST_7200)
    EMBXError = EMBX_OpenTransport(InitParams_p->MMETransportName, &tp);
#endif /* #elif defined(ST_7200) ... */
    if(EMBXError != EMBX_SUCCESS)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        return(ST_ERROR_NO_MEMORY);
    }
    PrivateData_p->EmbxTransport = tp;
    PrivateData_p->IsEmbxTransportValid = TRUE;

#endif /*STUB_FMW_ST40*/

	/* Data initialisation : vc1_InitData set VC1DecoderPrivateData_t to 0 :
	 * any variable set to a value different to 0 should be initialize AFTER
	 * calling this function */
    vc1_InitData(PrivateData_p);
    PrivateData_p->DecoderState = VC1DECODER_CONTEXT_NOT_USED;
    PrivateData_p->BitBufferAddress_p = InitParams_p->BitBufferAddress_p;
    PrivateData_p->BitBufferSize = InitParams_p->BitBufferSize;

    /* Avmem Partition used for for Mailbox allocation */
    DECODER_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

	/* save CPU partition (used for deallocation) */
    PrivateData_p->CPUPartition_p = InitParams_p->CPUPartition_p;

/*TD    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_CONFIRM_BUFFER_EVT, &(PrivateData_p->RegisteredEventsID[DECODER_CONFIRM_BUFFER_EVT]));
    }
*/
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_JOB_COMPLETED_EVT, &(PrivateData_p->RegisteredEventsID[VC1DECODER_JOB_COMPLETED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_INFORM_READ_ADDRESS_EVT, &(PrivateData_p->RegisteredEventsID[VC1DECODER_INFORM_READ_ADDRESS_EVT_ID]));
    }

	initParams_decoder.Priority = MME_PRIORITY_NORMAL;
	initParams_decoder.StructSize = sizeof (MME_TransformerInitParams_t);
    initParams_decoder.Callback = &VC1_TransformerCallback_decoder;
    initParams_decoder.CallbackUserData = (void *) DecoderHandle; /* UserData */
    initParams_decoder.TransformerInitParamsSize = 0;
    initParams_decoder.TransformerInitParams_p = NULL;

    #if defined(ST_7200)
    /* Register area is seen with same address between CPU and Device on 7200 */
    DeviceRegistersBaseAddress = (U32)InitParams_p->RegistersBaseAddress_p;
    #else  /* #if defined(ST_7200) */
    DeviceRegistersBaseAddress = (U32)STAVMEM_CPUToDevice(InitParams_p->RegistersBaseAddress_p,&VirtualMapping);
    #endif /* #if defined(ST_7200) */

    #if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    #if defined(ST_DELTAMU_GREEN_HE)
    DeviceRegistersBaseAddress = 0xAC220000;
    #else
    DeviceRegistersBaseAddress = 0xAC211000;
    #endif /* ST_DELTAMU_GREEN_HE */
    #endif /* (ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */
#ifdef NATIVE_CORE
	 /* currently VSOC is using a model for deltamu with old MME_TRANSFORMER_NAME */
    sprintf(TransformerName, "%s", VC9_MME_TRANSFORMER_NAME);
#else
	 sprintf(TransformerName, "%s_%08X", VC9_MME_TRANSFORMER_NAME, DeviceRegistersBaseAddress);
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
#endif /* STVID_DEBUG_GET_STATUS */

    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Transformer(decoder) initialized, handle %d", PrivateData_p->TranformHandleDecoder));

#if defined(MME_DUMP) && defined(VALID_TOOLS)
	VC1_HDM_Init();
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */

#ifdef PRINT_DECODER_INPUT
	if((PrivateData_p->DebugFile = fopen(DEBUG_FILENAME, "w")) == NULL)
	{
		STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"ERROR, UNABLE TO OPEN DECODER DEBUG FILE %s", DEBUG_FILENAME));
	}
#endif  /*PRINT_DECODER_INPUT*/

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
ST_ErrorCode_t VC1DECODER_Setup(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
	UNUSED_PARAMETER(DecoderHandle);
	UNUSED_PARAMETER(SetupParams_p);
    return (ErrorCode);
} /* End of VC1DECODER_Setup function */

/*******************************************************************************
Name        : VC1DECODER_FillAdditionnalDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VC1DECODER_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 *Dst;

    UNUSED_PARAMETER(DecoderHandle);

    /* fill the PPB */
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    Dst = STAVMEM_DeviceToVirtual(Picture_p->PPB.Address_p, &VirtualMapping);
    STOS_memsetUncached (Dst , 0, Picture_p->PPB.Size);

    return(ErrorCode);
} /* End of VC1DECODER_FillAdditionnalDataBuffer() function */



/******************************************************************************/
/*! \brief Terminate the decoder module
 *
 * deAllocate buffers, Unregister Events
 *
 * \param DecoderHandle
 * \return ST_ErrorCode
 */
/******************************************************************************/
ST_ErrorCode_t VC1DECODER_Term(const DECODER_Handle_t DecoderHandle)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    VC1DecoderPrivateData_t *      PrivateData_p;
    EMBX_ERROR                      EMBXError;
    /* U8                              iBuff; */
    /* STAVMEM_FreeBlockParams_t   	FreeParams; */
    /* STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping; */
    MME_ERROR                       MME_Error;
    ST_ErrorCode_t                  MMETermErrorCode;
    U32                             TermCounter;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

#ifdef PRINT_DECODER_INPUT

	fclose(PrivateData_p->DebugFile);

#endif /* PRINT_DECODER_INPUT*/

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
#endif /* STUB_FMW_ST40 */
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
    VC1_HDM_Term();
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = MMETermErrorCode;
    }
	return (ErrorCode);
}

#ifdef VALID_TOOLS
/*******************************************************************************
Name        : vc1dec_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VC1DECODER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART
    TRACE_ConditionalRegister(VC1DEC_TRACE_BLOCKID, VC1DEC_TRACE_BLOCKNAME, vc1dec_TraceItemList, sizeof(vc1dec_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* TRACE_UART */

    return(ErrorCode);
}
#endif /* VALID_TOOLS */
#ifdef ST_speed
/*******************************************************************************
Name        : VC1DECODER_SetDecodingMode
Description : Use to set degraded decode mode. Not yet implemented
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VC1DECODER_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode)
{
	UNUSED_PARAMETER(DecoderHandle);
	UNUSED_PARAMETER(Mode);
	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Degraded mode: Feature not available "));
}
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : VC1DECODER_SetDecodingDirection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VC1DECODER_SetDecodingDirection(const DECODER_Handle_t DecoderHandle,const DecodingDirection_t Direction)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(Direction);
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"SetDecodingDirection function is not implemented yet"));
}
#endif /* STVID_TRICKMODE_BACKWARD */
#endif

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
ST_ErrorCode_t VC1DECODER_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize)
{
    DECODER_Properties_t *      DECODER_Data_p;
    VC1DecoderPrivateData_t *   PrivateData_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

	DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (VC1DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->BitBufferAddress_p = BitBufferAddress_p;
    PrivateData_p->BitBufferSize = BitBufferSize;
    return(Error);
}   /* End of VC1DECODER_UpdateBitBufferParams */

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
ST_ErrorCode_t VC1DECODER_SetupForH264PreprocWA_GNB42332(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(AVMEMPartition);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of VC1DECODER_SetupForH264PreprocWA_GNB42332 */
#endif /* DVD_SECURED_CHIP */

/* End of vc1decode.c */
