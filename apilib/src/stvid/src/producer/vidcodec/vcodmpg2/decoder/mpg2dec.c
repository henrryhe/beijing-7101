/*******************************************************************
               MPEG Video Codec Decoder Module

            Copyright 2004 Scientific-Atlanta, Inc.

                  Subscriber Engineering
                  5030 Sugarloaf Parkway
                     P. O. Box 465447
                  Lawrenceville, GA 30042

                 Proprietary and Confidential
        Unauthorized distribution or copying is prohibited
                    All rights reserved

 No part of this computer software may be reprinted, reproduced or utilized
 in any form or by any electronic, mechanical, or other means, now known or
 hereafter invented, including photocopying and recording, or using any
 information storage and retrieval system, without permission in writing
 from Scientific Atlanta, Inc.

*******************************************************************

 File Name:    mpg2dec.c

 See Also:     mpg2dec.h

 Project:      Zeus

 Compiler:     ST40

 Author:       John Bean

 Description:  MPEG2 Decoder top level CODEC API functions

 Documents:

 Notes:

******************************************************************
 History:
 Rev Level    Date    Name    ECN#    Description
------------------------------------------------------------------
 1.00       10/04/04  JRB             First Release
******************************************************************/


/* ====== includes ======*/
/*#define MPEG2_DUMP_AFTER_DECODE*/
/* enable STTBX_Report functions for debug stage... */
#ifdef STVID_DEBUG_DECODER
    #define STTBX_REPORT
#endif

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "sttbx.h"

#include "mpg2dec.h"

/* #define TRACE_UART */

#if defined TRACE_UART
#include "../../../../tests/src/trace.h"
#define TraceDecode(x) TraceState x
#else
#define TraceDecode(x)
#endif /* TRACE_UART */

/* ====== file info ======*/


/* ====== defines ======*/
#define MAX_TRY_FOR_MME_TERM_TRANSFORMER 10 /* max loop number for MME_TermTransformer completion waiting loop in DECODER_Term */

/*#define MME_SIMULATION*/

/* ====== enums ======*/


/* ====== typedefs ======*/


/* ====== globals ======*/


const DECODER_FunctionsTable_t DECODER_Mpeg2Functions =
{
    mpg2dec_GetState,
    mpg2dec_GetCodecInfo,
#ifdef STVID_DEBUG_GET_STATISTICS
    mpg2dec_GetStatistics,
    mpg2dec_ResetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    mpg2dec_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    mpg2dec_Init,
    mpg2dec_Reset,
    mpg2dec_DecodePicture,
    mpg2dec_ConfirmBuffer,
    mpg2dec_Abort,
    mpg2dec_Setup,
    mpg2dec_FillAdditionnalDataBuffer,
    mpg2dec_Term
#ifdef ST_speed
    ,mpg2dec_SetDecodingMode
#ifdef STVID_TRICKMODE_BACKWARD
    ,mpg2dec_SetDecodingDirection
#endif
#endif
    ,mpg2dec_UpdateBitBufferParams
#if defined(DVD_SECURED_CHIP)
    ,mpg2dec_SetupForH264PreprocWA_GNB42332
#endif /* DVD_SECURED_CHIP */
};


/* ====== statics ======*/

#ifdef MPEG2_DUMP_AFTER_DECODE
static U32 MPEG2_Picture_Number=0;
#endif


/* ====== prototypes ======*/

static ST_ErrorCode_t mpg2dec_InitPrivateData(MPEG2DecoderPrivateData_t * const PrivateData_p, const DECODER_InitParams_t * const InitParams_p);
void MPEG2_TransformerCallback_decoder (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);
#ifdef MME_SIMULATION
void DumpMMECommandInfo (MPEG2_TransformParam_t *TransformParam_p,
                         MPEG2_SetGlobalParamSequence_t *GlobalParam_p,
                         U32  DecodingOrderFrameID,
                         BOOL FirstPictureEver,
                         BOOL IsBitStreamMPEG1,
                         BOOL StreamTypeChange,
                         BOOL NewSeqHdr,
                         BOOL NewQuantMtx,
                         BOOL closeFiles);
#endif


/* osclock_t                               Time_start, time_end ;*/






/*******************************************************************************
Name        : MPEG2_TransformerCallback_decoder
Description :
Parameters  : Event
 *            CallbackData
 *            UserData
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void MPEG2_TransformerCallback_decoder (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{
#ifdef   MPEG2_DUMP_AFTER_DECODE
    MPEG2_TransformParam_t         *fmw_p;
    U8                              FilenameSuffix[64];
    U8                              Filename_p[64];
    MPEG2_SetGlobalParamSequence_t    *GlobalParam_p    = &PrivateData_p->GlobalParamSeq;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    MPEG2_LumaAddress_t             DecodeLuma_adr;
    MPEG2_LumaAddress_t             Decodechroma_adr;
#endif
    MME_Command_t               *command_p;
    DECODER_Properties_t *      DECODER_Data_p;
    MPEG2DecoderPrivateData_t *   PrivateData_p;
    MPEG2_CommandStatus_t *     MPEG2_CommandStatus_fmw_p;

    DECODER_Data_p  = (DECODER_Properties_t *) UserData;
    PrivateData_p   = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    command_p       = (MME_Command_t *) CallbackData;
    MPEG2_CommandStatus_fmw_p = (MPEG2_CommandStatus_t *) command_p->CmdStatus.AdditionalInfo_p;

    UNUSED_PARAMETER(Event);
    TraceDecode(("Dec","%s","End"));

    /* inform the controler of the terminated command */
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.CommandId =  PrivateData_p->CommandId;

#ifdef STUB_FMW_ST40
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;
#else
    switch (MPEG2_CommandStatus_fmw_p->ErrorCode)
    {
        case MPEG2_DECODER_ERROR_MB_OVERFLOW :
        case MPEG2_DECODER_ERROR_RECOVERED :
        case MPEG2_DECODER_ERROR_NOT_RECOVERED :
            PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
            break;
        case MPEG2_DECODER_ERROR_TASK_TIMEOUT :
            PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
            break;

        default :
            PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;
            break;
    }
#endif /* STUB_FMW_ST40 */

    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobState = DECODER_JOB_COMPLETED;
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobCompletionTime = time_now();
#ifdef STVID_VALID_MEASURE_TIMING
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.LXDecodeTimeInMicros = MPEG2_CommandStatus_fmw_p->DecodeTimeInMicros;
#endif /* STVID_VALID_MEASURE_TIMING */

#ifdef MPEG2_DUMP_AFTER_DECODE
    fmw_p = &PrivateData_p->TransformParam;
    FilenameSuffix[0] = 0;

    sprintf(Filename_p, "pic_%03d",
                        MPEG2_Picture_Number++);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

    DecodeLuma_adr = STAVMEM_DeviceToCPU(fmw_p->DecodedBufferAddress.DecodedLuma_p,&VirtualMapping);
    Decodechroma_adr = STAVMEM_DeviceToCPU(fmw_p->DecodedBufferAddress.DecodedChroma_p,&VirtualMapping);
    vidprod_SavePicture(/*OMEGA2_420*/0x94, GlobalParam_p->horizontal_size,
                                    GlobalParam_p->vertical_size,
                                    DecodeLuma_adr /* LumaAddress */,
                                    Decodechroma_adr /* ChromaAddress */,
                                    Filename_p);
 #endif

#ifdef STVID_VALID_DEC_TIME
	       /* Store the time */
    VVID_MPEG2DEC_TimeStore_Command(PrivateData_p);
#endif

    STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[MPEG2DECODER_JOB_COMPLETED_EVT_ID], (const void *) &(PrivateData_p->DecoderContext.DECODER_DecodingJobResults));
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
    PrivateData_p->DecoderContext.State = MPEG2DECODER_CONTEXT_NOT_USED; /* The flag must be reset *after* the DECODER_JOB_COMPLETED_EVT notification */
    /* The DECODER_INFORM_READ_ADDRESS_EVT can be notified before or after the reset of PrivateData_p->DecoderContext.State
     * but if we notify it before it will delay the opportunity for the producer to start a new decode as an
     * CONTROLLER_COMMAND_DECODER_READ_ADDRESS_UPDATED command will be pushed and treated before the producer make a try for the next DecodePicture */
    STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[MPEG2DECODER_INFORM_READ_ADDRESS_EVT_ID], (const void *) &(PrivateData_p->InformReadAddress));
}

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Function:    mpg2dec_GetStatistics

Parameters:  DecoderHandle
             Statistics_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Get the Decoder statistics (TBD)

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_GetStatistics(const DECODER_Handle_t DecoderHandle, STVID_Statistics_t * const Statistics_p)
{
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
   DECODER_Properties_t      *DECODER_Data_p = (DECODER_Properties_t *) DecoderHandle;
   MPEG2DecoderPrivateData_t  *PrivateData_p  = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

   Statistics_p->DecodeInterruptDecoderIdle = PrivateData_p->Statistics.DecodeInterruptDecoderIdle;
   Statistics_p->DecodeInterruptPipelineIdle = PrivateData_p->Statistics.DecodeInterruptPipelineIdle;
   Statistics_p->DecodeInterruptStartDecode = PrivateData_p->Statistics.DecodeInterruptStartDecode;
   Statistics_p->DecodePbInterruptDecodeOverflowError = PrivateData_p->Statistics.DecodePbInterruptDecodeOverflowError;
   Statistics_p->DecodePbInterruptDecodeUnderflowError = PrivateData_p->Statistics.DecodePbInterruptDecodeUnderflowError;
   Statistics_p->DecodePbInterruptMisalignmentError = PrivateData_p->Statistics.DecodePbInterruptMisalignmentError;
   Statistics_p->DecodePbInterruptSyntaxError = PrivateData_p->Statistics.DecodePbInterruptSyntaxError;
   Statistics_p->DecodePbMaxNbInterruptSyntaxErrorPerPicture = PrivateData_p->Statistics.DecodePbMaxNbInterruptsSyntaxErrorPerPicture;
   Statistics_p->DecodePictureSkippedNotRequested = PrivateData_p->Statistics.DecodePictureSkippedNotRequested;
   Statistics_p->DecodePictureSkippedRequested = PrivateData_p->Statistics.DecodePictureSkippedRequested;

   return(ErrorCode);
}

/*******************************************************************************
Function:    mpg2dec_ResetStatistics

Parameters:  DecoderHandle
             Statistics_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Reset the Decoder statistics (TBD)

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_ResetStatistics(const DECODER_Handle_t DecoderHandle, const STVID_Statistics_t * const Statistics_p)
{
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
   DECODER_Properties_t      *DECODER_Data_p = (DECODER_Properties_t *) DecoderHandle;
   MPEG2DecoderPrivateData_t  *PrivateData_p  = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
   PrivateData_p->Statistics.DecodeInterruptDecoderIdle = 0;
   PrivateData_p->Statistics.DecodeInterruptPipelineIdle = 0;
   PrivateData_p->Statistics.DecodeInterruptStartDecode = 0;
   PrivateData_p->Statistics.DecodePbInterruptDecodeOverflowError = 0;
   PrivateData_p->Statistics.DecodePbInterruptDecodeUnderflowError = 0;
   PrivateData_p->Statistics.DecodePbInterruptMisalignmentError = 0;
   PrivateData_p->Statistics.DecodePbInterruptSyntaxError = 0;
   PrivateData_p->Statistics.DecodePbMaxNbInterruptsSyntaxErrorPerPicture = 0;
   PrivateData_p->Statistics.DecodePictureSkippedNotRequested = 0;
   PrivateData_p->Statistics.DecodePictureSkippedRequested = 0;

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
ST_ErrorCode_t mpg2dec_GetStatus(const DECODER_Handle_t DecoderHandle, STVID_Status_t * const Status_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t      *DECODER_Data_p = (DECODER_Properties_t *) DecoderHandle;
    MPEG2DecoderPrivateData_t  *PrivateData_p  = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    /* Update status with actual values in the decoder */
	Status_p->SequenceInfo_p = PrivateData_p->Status.SequenceInfo_p;

    return(ErrorCode);
} /* end ofmpg2dec_GetStatus() */
#endif /* STVID_DEBUG_GET_STATUS */


/*******************************************************************************
Function:    mpg2dec_GetState

Parameters:  DecoderHandle
             CommandId
             DecoderStatus_p

Returns:     ST_NO_ERROR
             ST_ERROR_BAD_PARAMETER - CommandId not found in job queue

Scope:       Codec API

Purpose:     Get the state of the command specified

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_GetState(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId, DECODER_Status_t * const DecoderStatus_p)
{
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
   DECODER_Properties_t      *DECODER_Data_p = (DECODER_Properties_t *) DecoderHandle;
   MPEG2DecoderPrivateData_t  *PrivateData_p  = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

   if (CommandId == PrivateData_p->DecoderStatus.CommandId)
   {
      *DecoderStatus_p = PrivateData_p->DecoderStatus;
   }
   else
   {
      ErrorCode = ST_ERROR_BAD_PARAMETER;
   }

   return(ErrorCode);
}

/*******************************************************************************
Function:    mpg2dec_GetCodecInfo

Parameters:  DecoderHandle
             CodecInfo_p
             CodecMode

Returns:     ST_NO_ERROR
             ST_ERROR_BAD_PARAMETER - CommandId not found in job queue

Scope:       Codec API

Purpose:     Get the state of the command specified

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_GetCodecInfo(const DECODER_Handle_t DecoderHandle, DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication, const STVID_CodecMode_t CodecMode)
{
/* Simplified function, FrameBufferAdditionalDataBytesPerMB useless in current MPEG2 codec */
/* TODO_CL to be verified */
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(ProfileAndLevelIndication);

    /* There is no special latency in the MPEG2 decode process
    * (only a vsync latency can be considered and it's set in the DecodeStartConditionCheck) */
    CodecInfo_p->MaximumDecodingLatencyInSystemClockUnit = 0;
    if (CodecMode == STVID_CODEC_MODE_DECODE)
    {
      CodecInfo_p->FrameBufferAdditionalDataBytesPerMB     = 0;
    }
    else if (CodecMode == STVID_CODEC_MODE_TRANSCODE)
    {
      CodecInfo_p->FrameBufferAdditionalDataBytesPerMB     = 44;
    }
    else
    {
      ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    return(ErrorCode);
}

/*******************************************************************************
Function:    mpg2dec_Reset

Parameters:  DecoderHandle

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Reset the Decoder

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_Reset(const DECODER_Handle_t DecoderHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    MPEG2DecoderPrivateData_t *      PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->DecoderContext.State = MPEG2DECODER_CONTEXT_NOT_USED;

    return(ErrorCode);
}
/*******************************************************************************
Function:    mpg2dec_Abort

Parameters:  DecoderHandle
             CommandId

Returns:     ST_NO_ERROR
             ST_ERROR_BAD_PARAMETER - CommandId not found in job queue

Scope:       Codec API

Purpose:     Get the state of the command specified

Behavior:
             In the MPEG2 implementation of this codec the decode there is
             only one command pending at a time so the DECODER_Abort has no
             effect because the decode command is treated as soon as it has
             been initiated by DECODER_DecodePicture.

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_Abort(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    MPEG2DecoderPrivateData_t *      PrivateData_p;

    UNUSED_PARAMETER(CommandId);
    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->DecoderContext.State = MPEG2DECODER_CONTEXT_NOT_USED;

    return(ErrorCode);
}


/*******************************************************************************
Function:    mpg2dec_DecodePicture

Parameters:  DecoderHandle
             StartParams_p
             DecoderStatus_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Start a frame decode

Behavior:    This function actually en-queues a formal CommandId
             decode command. The actual decode will be treated by the
             decoder at a time depending on the decoder implementation.
             In the MPEG2 implementation of this codec the decode there
             is only one command pending at a time, so the decode command
             is processed immediately.

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_DecodePicture (const DECODER_Handle_t         DecoderHandle,
                              const DECODER_DecodePictureParams_t *  const DecodePictureParams_p,
                              DECODER_Status_t *                     const DecoderStatus_p)
{
    ST_ErrorCode_t                              ErrorCode        = ST_NO_ERROR;
    DECODER_Properties_t                       *DECODER_Data_p   = (DECODER_Properties_t *) DecoderHandle;
    MPEG2DecoderPrivateData_t                  *PrivateData_p    = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    MPEG2_SetGlobalParamSequence_t             *GlobalParam_p    = &PrivateData_p->GlobalParamSeq;
    MPEG2_TransformParam_t                     *TransformParam_p = &PrivateData_p->TransformParam;
    VIDCOM_ParsedPictureInformation_t          *ParsedPicture_p  = &DecodePictureParams_p->PictureInformation->ParsedPictureInformation;
    MPEG2_GlobalDecodingContextSpecificData_t  *GDCSpecData_p;
    MPEG2_PictureSpecificData_t                *PictSpecData_p   = (MPEG2_PictureSpecificData_t *) ParsedPicture_p->PictureSpecificData_p;
    VIDCOM_PictureGenericData_t                *PictureGenericData_p = &ParsedPicture_p->PictureGenericData;
    STVID_SequenceInfo_t                       *SeqInfo_p        = &ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo;
    STVID_VideoParams_t                        *VideoParams_p    = &ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams;
    U32                                         idx;
    U32                                          GDCSpecData_Int = (U32)&DecodePictureParams_p->PictureInformation->GlobalDecodingContextSpecificData;
    GDCSpecData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) GDCSpecData_Int;

#ifdef STVID_DEBUG_GET_STATUS
	PrivateData_p->Status.SequenceInfo_p = SeqInfo_p;
#endif /* STVID_DEBUG_GET_STATUS */


/*   This decoder only supports a single picture decode at once*/
    if (PrivateData_p->DecoderContext.State != MPEG2DECODER_CONTEXT_NOT_USED)
    {
        ErrorCode = ST_ERROR_DEVICE_BUSY;
        DecoderStatus_p->State = PrivateData_p->DecoderStatus.State;
        DecoderStatus_p->CommandId = PrivateData_p->DecoderStatus.CommandId;
        DecoderStatus_p->IsJobSubmitted = FALSE;
        DecoderStatus_p->DecodingJobStatus = PrivateData_p->DecoderStatus.DecodingJobStatus;
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = STOS_time_now();
        return (ErrorCode);
    }



    PrivateData_p->DecoderContext.State = MPEG2DECODER_CONTEXT_DECODE_FUNC;

/*   Fill in parameters for the InformReadAddress message*/
    PrivateData_p->InformReadAddress.CommandId =  PrivateData_p->DecoderStatus.CommandId;
    PrivateData_p->InformReadAddress.DecoderReadAddress_p = ParsedPicture_p->PictureGenericData.BitBufferPictureStopAddress;


/*   Fill in the MPEG2_SetGlobalParamSequence_t data for this picture*/
    GlobalParam_p->StructSize = sizeof (MPEG2_SetGlobalParamSequence_t);
    GlobalParam_p->horizontal_size =  SeqInfo_p->Width;
    GlobalParam_p->vertical_size = SeqInfo_p->Height;
    GlobalParam_p->progressive_sequence = (SeqInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE) ? TRUE : FALSE;
    GlobalParam_p->chroma_format = GDCSpecData_p->chroma_format;

#if (MPEG2_MME_VERSION >= 30)
    GlobalParam_p->MatrixFlags = 0;
    if (GDCSpecData_p->load_intra_quantizer_matrix)
    {
        GlobalParam_p->MatrixFlags |= MPEG2_LOAD_INTRA_QUANTIZER_MATRIX_FLAG;
    }
    if (GDCSpecData_p->load_non_intra_quantizer_matrix)
    {
        GlobalParam_p->MatrixFlags |= MPEG2_LOAD_NON_INTRA_QUANTIZER_MATRIX_FLAG;
    }
#endif
    GlobalParam_p->MPEGStreamTypeFlag = !GDCSpecData_p->IsBitStreamMPEG1;


/*    Copy the matrices into decoder private data, as they seem to get re-used*/
    for (idx = 0; idx < MPEG2_Q_MATRIX_SIZE; idx++)
    {
        GlobalParam_p->intra_quantiser_matrix[idx] = PrivateData_p->intra_quantiser_matrix[idx] = *((U8 *)PictSpecData_p->LumaIntraQuantMatrix_p + idx);
        GlobalParam_p->non_intra_quantiser_matrix[idx] = PrivateData_p->non_intra_quantiser_matrix[idx] = *((U8 *)PictSpecData_p->LumaNonIntraQuantMatrix_p + idx);
        GlobalParam_p->chroma_intra_quantiser_matrix[idx] = PrivateData_p->chroma_intra_quantiser_matrix[idx] = *((U8 *)PictSpecData_p->ChromaIntraQuantMatrix_p + idx);
        GlobalParam_p->chroma_non_intra_quantiser_matrix[idx] = PrivateData_p->chroma_non_intra_quantiser_matrix[idx] = *((U8 *)PictSpecData_p->ChromaNonIntraQuantMatrix_p + idx);
    }

/*    Fill in the MPEG2_TransformParam_t data for this picture*/
    TransformParam_p->StructSize = sizeof (MPEG2_TransformParam_t);

#ifdef MME_SIMULATION
    TransformParam_p->PictureStartAddrCompressedBuffer_p = (U32 *)((U32)ParsedPicture_p->PictureGenericData.BitBufferPictureStartAddress - (U32)PrivateData_p->BitBufferAddress_p);
    TransformParam_p->PictureStopAddrCompressedBuffer_p  = (U32 *)((U32)ParsedPicture_p->PictureGenericData.BitBufferPictureStopAddress  - (U32)PrivateData_p->BitBufferAddress_p);
#else
    TransformParam_p->PictureStartAddrCompressedBuffer_p = ParsedPicture_p->PictureGenericData.BitBufferPictureStartAddress;
    TransformParam_p->PictureStopAddrCompressedBuffer_p  = ParsedPicture_p->PictureGenericData.BitBufferPictureStopAddress;
#endif

    /*   DecodedBufferAddress is filled in mostly by the ConfirmBuffer call*/
    TransformParam_p->DecodedBufferAddress.StructSize = sizeof (MPEG2_DecodedBufferAddress_t);

    TransformParam_p->DecodingMode = MPEG2_NORMAL_DECODE;

    TransformParam_p->AdditionalFlags = 0;
#ifdef ST_transcoder /* To be enabled all time when transcode API is OK */
    if (PrivateData_p->CodecMode == STVID_CODEC_MODE_TRANSCODE)
    {
        TransformParam_p->AdditionalFlags = MPEG2_ADDITIONAL_FLAG_TRANSCODING_H264;
    }
    else
#endif
    {
        TransformParam_p->AdditionalFlags = MPEG2_ADDITIONAL_FLAG_NONE;
    }

    /* field-based streams in case of stop/start */
    if (VideoParams_p->PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
    {
        if (PictureGenericData_p->IsFirstOfTwoFields)
        {
            TransformParam_p->AdditionalFlags |= MPEG2_ADDITIONAL_FLAG_FIRST_FIELD;
        }
        else
        {
            TransformParam_p->AdditionalFlags |= MPEG2_ADDITIONAL_FLAG_SECOND_FIELD;
        }
    }

    /*    Fill in Picture Parameters*/
    TransformParam_p->PictureParameters.StructSize = sizeof (MPEG2_ParamPicture_t);

    if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_B)
    {
        TransformParam_p->PictureParameters.picture_coding_type = MPEG2_BIDIRECTIONAL_PICTURE;
    }
    else if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P)
    {
        TransformParam_p->PictureParameters.picture_coding_type = MPEG2_PREDICTIVE_PICTURE;
    }
    else if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I)
    {
        TransformParam_p->PictureParameters.picture_coding_type = MPEG2_INTRA_PICTURE;
    }
    else
    {
        TransformParam_p->PictureParameters.picture_coding_type = MPEG2_FORBIDDEN_PICTURE;
    }

    if (SeqInfo_p->MPEGStandard == STVID_MPEG_STANDARD_ISO_IEC_13818)
    {
    /*       MPEG-2*/
        TransformParam_p->PictureParameters.forward_horizontal_f_code = PictSpecData_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL];
        TransformParam_p->PictureParameters.forward_vertical_f_code = PictSpecData_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL];
        TransformParam_p->PictureParameters.backward_horizontal_f_code = PictSpecData_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL];
        TransformParam_p->PictureParameters.backward_vertical_f_code = PictSpecData_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL];
    }
    else
    {
    /*      MPEG-1*/
        TransformParam_p->PictureParameters.forward_horizontal_f_code = PictSpecData_p->full_pel_forward_vector;
        TransformParam_p->PictureParameters.forward_vertical_f_code = PictSpecData_p->forward_f_code;
        TransformParam_p->PictureParameters.backward_horizontal_f_code = PictSpecData_p->full_pel_backward_vector;
        TransformParam_p->PictureParameters.backward_vertical_f_code = PictSpecData_p->backward_f_code;
    }

    TransformParam_p->PictureParameters.intra_dc_precision = PictSpecData_p->intra_dc_precision;

    if (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
        TransformParam_p->PictureParameters.picture_structure = MPEG2_FRAME_TYPE;
    }
    else if (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
    {
        TransformParam_p->PictureParameters.picture_structure = MPEG2_TOP_FIELD_TYPE;
    }
    else if (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
    {
        TransformParam_p->PictureParameters.picture_structure = MPEG2_BOTTOM_FIELD_TYPE;
    }
    else  /*TODO: What do about reserved types?? Anything?*/
    {
        TransformParam_p->PictureParameters.picture_structure = MPEG2_RESERVED_TYPE;
    }


    /*    MPEG Decoding Flags*/
    TransformParam_p->PictureParameters.mpeg_decoding_flags = 0;
    if (VideoParams_p->TopFieldFirst)
    {
        TransformParam_p->PictureParameters.mpeg_decoding_flags |= MPEG_DECODING_FLAGS_TOP_FIELD_FIRST;
    }

    if (PictSpecData_p->frame_pred_frame_dct)
    {
        TransformParam_p->PictureParameters.mpeg_decoding_flags |= MPEG_DECODING_FLAGS_FRAME_PRED_FRAME_DCT;
    }

    if (PictSpecData_p->concealment_motion_vectors)
    {
        TransformParam_p->PictureParameters.mpeg_decoding_flags |= MPEG_DECODING_FLAGS_CONCEALMENT_MOTION_VECTORS;
    }

    if (PictSpecData_p->q_scale_type)
    {
        TransformParam_p->PictureParameters.mpeg_decoding_flags |= MPEG_DECODING_FLAGS_Q_SCALE_TYPE;
    }

    if (PictSpecData_p->intra_vlc_format)
    {
        TransformParam_p->PictureParameters.mpeg_decoding_flags |= MPEG_DECODING_FLAGS_INTRA_VLC_FORMAT;
    }

    if (PictSpecData_p->alternate_scan)
    {
        TransformParam_p->PictureParameters.mpeg_decoding_flags |= MPEG_DECODING_FLAGS_ALTERNATE_SCAN;
    }

    /*    TODO: How is deblocking filter controlled?*/
    TransformParam_p->PictureParameters.mpeg_decoding_flags |= MPEG_DECODING_FLAGS_DEBLOCKING_FILTER_ENABLE;

    /*    Fill in the Temporal Reference for this picture and the references*/
    TransformParam_p->DecodedBufferAddress.DecodedTemporalReferenceValue = PictSpecData_p->temporal_reference;
    if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_B)
    {
        TransformParam_p->RefPicListAddress.ForwardTemporalReferenceValue  = (ParsedPicture_p->PictureGenericData.UsedSizeInReferenceListB0 == 0) ? -1 : PictSpecData_p->ForwardTemporalReferenceValue;
        TransformParam_p->RefPicListAddress.BackwardTemporalReferenceValue = (ParsedPicture_p->PictureGenericData.UsedSizeInReferenceListB1 == 0) ? -1 : PictSpecData_p->BackwardTemporalReferenceValue;

    }
    else
    {
    /*       P or I frame, so only forward references in list P0*/
        TransformParam_p->RefPicListAddress.ForwardTemporalReferenceValue = (ParsedPicture_p->PictureGenericData.UsedSizeInReferenceListP0 == 0) ? -1 : PictSpecData_p->ForwardTemporalReferenceValue;
        TransformParam_p->RefPicListAddress.BackwardTemporalReferenceValue = -1;
    }

    /*    Update the decoder status for the new job*/
    PrivateData_p->CommandId = PrivateData_p->NextCommandId;
    PrivateData_p->NextCommandId++;

    PrivateData_p->DecoderStatus.CommandId = PrivateData_p->CommandId;
    PrivateData_p->DecoderStatus.DecodingJobStatus.JobState = DECODER_JOB_WAITING_CONFIRM_BUFFER;
    PrivateData_p->DecoderStatus.DecodingJobStatus.JobSubmissionTime = STOS_time_now();
    PrivateData_p->DecoderStatus.IsJobSubmitted = TRUE;
    PrivateData_p->DecoderStatus.State = DECODER_STATE_READY_TO_DECODE;

    /*    Return the status to the producer*/
    DecoderStatus_p->State             = PrivateData_p->DecoderStatus.State;
    DecoderStatus_p->CommandId         = PrivateData_p->DecoderStatus.CommandId;
    DecoderStatus_p->IsJobSubmitted    = PrivateData_p->DecoderStatus.IsJobSubmitted;
    DecoderStatus_p->DecodingJobStatus = PrivateData_p->DecoderStatus.DecodingJobStatus;

    return(ErrorCode);
}

/*******************************************************************************
Function:    mpg2dec_ConfirmBuffer

Parameters:  DecoderHandle
             CommandId
             ConfirmBufferParams_p

Returns:     ST_NO_ERROR
             ST_ERROR_BAD_PARAMETER - CommandId not found in job queue

Scope:       Codec API

Purpose:     Get the state of the command specified

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_ConfirmBuffer(const DECODER_Handle_t DecoderHandle,
                                     const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p)
{
    ST_ErrorCode_t                              ErrorCode         = ST_NO_ERROR;
    DECODER_Properties_t                        *DECODER_Data_p   = (DECODER_Properties_t *) DecoderHandle;
    MPEG2DecoderPrivateData_t                   *PrivateData_p    = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    MPEG2_TransformParam_t                      *TransformParam_p = &PrivateData_p->TransformParam;
    MPEG2_SetGlobalParamSequence_t              *GlobalParam_p    = &PrivateData_p->GlobalParamSeq;
    VIDCOM_ParsedPictureInformation_t           *ParsedPicture_p  = &ConfirmBufferParams_p->NewPictureBuffer_p->ParsedPictureInformation;
#ifdef MME_SIMULATION
    STVID_VideoParams_t                         *VideoParams_p    = &ParsedPicture_p->PictureGenericData.PictureInfos.VideoParams;
    MPEG2_PictureSpecificData_t                 *PictSpecData_p   = (MPEG2_PictureSpecificData_t *) ParsedPicture_p->PictureSpecificData_p;
    static                                      CurrentRef        = 0;
#endif
    MPEG2_GlobalDecodingContextSpecificData_t   *GDCSpecData_p;
    STVID_DecimationFactor_t                    DecimationFactors;

    S32 forwardRefIdx, backwardRefIdx, counter;
/*    GDCSpecData_p = CAST_CONST_HANDLE_TO_DATA_TYPE(MPEG2_GlobalDecodingContextSpecificData_t * , &ConfirmBufferParams_p->NewPictureBuffer_p->GlobalDecodingContextSpecificData);*/

    U32                                          GDCSpecData_Int = (U32)&ConfirmBufferParams_p->NewPictureBuffer_p->GlobalDecodingContextSpecificData;
/*    GDCSpecData_p = CAST_CONST_HANDLE_TO_DATA_TYPE(MPEG2_GlobalDecodingContextSpecificData_t * , GDCSpecData_Int_p);*/
    GDCSpecData_p = (MPEG2_GlobalDecodingContextSpecificData_t *)GDCSpecData_Int;


    /* First of all, check the CommandId */
    if (ConfirmBufferParams_p->CommandId != PrivateData_p->DecoderStatus.CommandId)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unknown CommandId %d, ConfirmBuffer exit with ST_ERROR_BAD_PARAMETER error !",
                ConfirmBufferParams_p->CommandId));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        return (ErrorCode);
    }
    if (ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (MPEG2DECODER_ConfirmBuffer) :  FrameBuff_p = NULL"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        return (ErrorCode);
    }


    /*    Build the DecodedBufferAddress structure*/
    TransformParam_p->DecodedBufferAddress.StructSize    = sizeof (MPEG2_DecodedBufferAddress_t);

    PrivateData_p->DecoderContext.State = MPEG2DECODER_CONTEXT_CONFIRM_BUFFER_FUNC;

#ifdef MME_SIMULATION

    if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_I)
    {
        TransformParam_p->DecodedBufferAddress.DecodedLuma_p = (U32 *)((CurrentRef == 0) ? 100 : 102);
        TransformParam_p->DecodedBufferAddress.DecodedChroma_p = (U32 *)((U32)TransformParam_p->DecodedBufferAddress.DecodedLuma_p + 100);

        TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p =  NULL;
        TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p = NULL;

        TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p = NULL;
        TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p = NULL;

        CurrentRef = (CurrentRef + 1) % 2;
    }
    else if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P)
    {
        TransformParam_p->DecodedBufferAddress.DecodedLuma_p = (U32 *)((CurrentRef == 0) ? 100 : 102);
        TransformParam_p->DecodedBufferAddress.DecodedChroma_p = (U32 *)((U32)TransformParam_p->DecodedBufferAddress.DecodedLuma_p + 100);

        TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p = (U32 *)((CurrentRef == 0) ? 102 : 100);
        TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p = (U32 *)((U32)TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p + 100);

        TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p = NULL;
        TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p = NULL;

        CurrentRef = (CurrentRef + 1) % 2;
    }
    else
    {
        TransformParam_p->DecodedBufferAddress.DecodedLuma_p = (U32 *) 104;
        TransformParam_p->DecodedBufferAddress.DecodedChroma_p = (U32 *) 204;

        TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p = (U32 *)((CurrentRef == 0) ? 100 : 102);
        TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p = (U32 *)((U32)TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p + 100);

        TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p = (U32 *)((CurrentRef == 0) ? 102 : 100);
        TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p = (U32 *)((U32)TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p + 100);
    }

#else
    TransformParam_p->DecodedBufferAddress.DecodedLuma_p   = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address_p;
    TransformParam_p->DecodedBufferAddress.DecodedChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p;
    /* MBDescr_p is used for transcoding purpose */

    TransformParam_p->DecodedBufferAddress.MBDescr_p       = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->FrameOrFirstFieldPicture_p->PPB.Address_p;
#endif

    if (ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        TransformParam_p->DecodedBufferAddress.DecimatedLuma_p = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p;
        TransformParam_p->DecodedBufferAddress.DecimatedChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p;
        TransformParam_p->MainAuxEnable = MPEG2_AUX_MAIN_OUT_EN;
    }
    else
    {
        TransformParam_p->DecodedBufferAddress.DecimatedLuma_p = NULL;
        TransformParam_p->DecodedBufferAddress.DecimatedChroma_p = NULL;
        TransformParam_p->MainAuxEnable = MPEG2_MAINOUT_EN;
    }

    DecimationFactors = STVID_DECIMATION_FACTOR_NONE;
    TransformParam_p->HorizontalDecimationFactor   = MPEG2_HDEC_1;
    TransformParam_p->VerticalDecimationFactor     = MPEG2_VDEC_1;

    if (ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        DecimationFactors = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;
#if 0 /* From Valid but not yet integrated */
        if(ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Advanced_H_Decimation == TRUE)
        {
            if (DecimationFactors & STVID_DECIMATION_FACTOR_H2)
            TransformParam_p->HorizontalDecimationFactor   = MPEG2_HDEC_ADVANCED_2;
            if (DecimationFactors & STVID_DECIMATION_FACTOR_H4)
            TransformParam_p->HorizontalDecimationFactor   = MPEG2_HDEC_ADVANCED_4;
        }
        else
#endif
        {
            if (DecimationFactors & STVID_DECIMATION_FACTOR_H2)
            TransformParam_p->HorizontalDecimationFactor   = MPEG2_HDEC_2;
            if (DecimationFactors & STVID_DECIMATION_FACTOR_H4)
            TransformParam_p->HorizontalDecimationFactor   = MPEG2_HDEC_4;
        }

        if (DecimationFactors & STVID_DECIMATION_FACTOR_V2)
        {
        if((ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
            || (ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
            /* TBD : or decoded picture is frame and its contents is progressive */)
            TransformParam_p->VerticalDecimationFactor   = MPEG2_VDEC_2_INT;
        else
            TransformParam_p->VerticalDecimationFactor   = MPEG2_VDEC_2_PROG; /*TBD : Used only if decoded picture is frame and its contents is interlaced*/
        }
    }


    /*    Build the RefPicListAddress structure*/

    TransformParam_p->RefPicListAddress.StructSize = sizeof (MPEG2_RefPicListAddress_t);

#ifndef MME_SIMULATION
    /*   Find the pointers to the references for this picture*/
    forwardRefIdx = -1;
    backwardRefIdx = -1;

    for (counter = 0; ((counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) &&
                        ((forwardRefIdx == -1) || (backwardRefIdx == -1))); counter++)
    {
        if (ParsedPicture_p->PictureGenericData.IsValidIndexInReferenceFrame[counter])
        {
            if (forwardRefIdx == -1)
            {
            forwardRefIdx = counter;
            }
            else
            {
            backwardRefIdx = counter;
            }
        }
    }

    /*    Verify that the forward and backward indexes are set correctly
    if both were found*/
    if ((forwardRefIdx != -1) && (backwardRefIdx != -1))
    {
    /*       do they need to be swapped?*/
        if (ParsedPicture_p->PictureGenericData.FullReferenceFrameList[forwardRefIdx] > ParsedPicture_p->PictureGenericData.FullReferenceFrameList[backwardRefIdx])
        {
            counter = forwardRefIdx;
            forwardRefIdx = backwardRefIdx;
            backwardRefIdx = counter;
        }
    }


    /*    Set the Forward reference pointers*/


    if (forwardRefIdx != -1)
    {
        TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p = ConfirmBufferParams_p->NewPictureBuffer_p->ReferencesFrameBufferPointer[forwardRefIdx]->Allocation.Address_p;
        TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->ReferencesFrameBufferPointer[forwardRefIdx]->Allocation.Address2_p;
    }
    else if (ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p != NULL)
    {   /* No reference: refer to itself rather to old last reference set... */
        TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address_p;
        TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p;
    }
    else if ((ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p != NULL) &&
            (ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p != NULL))
    {
        TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p;
        TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p;
    }
    else
    {
        TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p = NULL;
        TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p = NULL;
    }


    /*   Set the Backward reference pointers*/

    if (backwardRefIdx != -1)
    {
        TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p = ConfirmBufferParams_p->NewPictureBuffer_p->ReferencesFrameBufferPointer[backwardRefIdx]->Allocation.Address_p;
        TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->ReferencesFrameBufferPointer[backwardRefIdx]->Allocation.Address2_p;
    }
    else if (ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p != NULL)
    {   /* No reference: refer to itself rather to old last reference set... */
        TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address_p;
        TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p;
    }
    else if ((ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p != NULL) &&
            (ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p != NULL))
    {
        TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p;
        TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p = ConfirmBufferParams_p->NewPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p;
    }
    else
    {
        TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p = NULL;
        TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p = NULL;
    }
#endif /*#ifndef MME_SIMULATION*/


#ifdef MME_SIMULATION
    DumpMMECommandInfo (TransformParam_p,GlobalParam_p,
                            ParsedPicture_p->PictureGenericData.DecodingOrderFrameID,
                            GDCSpecData_p->FirstPictureEver,
                            GDCSpecData_p->IsBitStreamMPEG1,
                            GDCSpecData_p->StreamTypeChange,
                            GDCSpecData_p->NewSeqHdr,
                            GDCSpecData_p->NewQuantMxt, FALSE);
#else
    if (GDCSpecData_p->NewSeqHdr || GDCSpecData_p->NewQuantMxt)
    {
        /* MME_SET_GLOBAL_TRANSFORM_PARAMS command */
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.Param_p     = (MME_GenericParams_t)GlobalParam_p;
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.StructSize  = sizeof(MME_Command_t);
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.CmdEnd      = MME_COMMAND_END_RETURN_NO_INFO;
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.DueTime     = 0;
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.ParamSize   = sizeof(MPEG2_SetGlobalParamSequence_t);
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.CmdCode     = MME_SET_GLOBAL_TRANSFORM_PARAMS;
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.NumberInputBuffers  = 0;
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.NumberOutputBuffers = 0;
        /* TODO: does firmware use this area */
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.CmdStatus.AdditionalInfoSize = 0;
        PrivateData_p->MPEG2CompanionData_p->CmdInfoParam.CmdStatus.AdditionalInfo_p = NULL;


#if defined(MME_DUMP) /*&& defined(VALID_TOOLS)*/
        MPEG2_HDM_DumpCommand(&PrivateData_p->MPEG2CompanionData_p->CmdInfoParam);
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */
#ifdef STUB_FMW_ST40
        ErrorCode =  MPEG2STUB_SendCommand(PrivateData_p->TranformHandleDecoder, &PrivateData_p->MPEG2CompanionData_p->CmdInfoParam);
#else
        ErrorCode =  MME_SendCommand(PrivateData_p->TranformHandleDecoder, &PrivateData_p->MPEG2CompanionData_p->CmdInfoParam);
#endif /* #ifdef STUB_FMW_ST40 */
    }

    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.Param_p = (MME_GenericParams_t)TransformParam_p;
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.StructSize  = sizeof(MME_Command_t);
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.CmdEnd      = MME_COMMAND_END_RETURN_NOTIFY;
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.DueTime     = 0;
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.ParamSize   = sizeof(MPEG2_TransformParam_t);
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.CmdCode     = MME_TRANSFORM;
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.NumberInputBuffers  = 0;
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.NumberOutputBuffers = 0;
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.CmdStatus.AdditionalInfoSize = sizeof (MPEG2_CommandStatus_t);
    PrivateData_p->MPEG2CompanionData_p->CmdDecoder.CmdStatus.AdditionalInfo_p = &PrivateData_p->MPEG2CompanionData_p->MPEG2CmdStatus;

    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime = STOS_time_now();


    /*    TODO PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime = STOS_time_now();*/
    TraceDecode(("Dec","%s","Begin"));

#if defined(MME_DUMP) /*&& defined(VALID_TOOLS)*/
    MPEG2_HDM_DumpCommand(&PrivateData_p->MPEG2CompanionData_p->CmdDecoder);
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */
#ifdef STUB_FMW_ST40
    ErrorCode =  MPEG2STUB_SendCommand(PrivateData_p->TranformHandleDecoder, &PrivateData_p->MPEG2CompanionData_p->CmdDecoder);
#else
    ErrorCode =  MME_SendCommand(PrivateData_p->TranformHandleDecoder, &PrivateData_p->MPEG2CompanionData_p->CmdDecoder);
/*    Time_start=STOS_time_now();*/
#endif /* #ifdef STUB_FMW_ST40 */

    return(ErrorCode);
#endif

    /*   Update the decoder status for the new job*/
    PrivateData_p->DecoderStatus.DecodingJobStatus.JobState = DECODER_JOB_DECODING;
    PrivateData_p->DecoderStatus.State = DECODER_STATE_DECODING;
    PrivateData_p->TransformParamsConfirmed = FALSE;

    return(ErrorCode);
} /* end of mpg2dec_ConfirmBuffer */


#ifdef MME_SIMULATION
void DumpMMECommandInfo (MPEG2_TransformParam_t *TransformParam_p,
                         MPEG2_SetGlobalParamSequence_t *GlobalParam_p,
                         U32  DecodingOrderFrameID,
                         BOOL FirstPictureEver,
                         BOOL IsBitStreamMPEG1,
                         BOOL StreamTypeChange,
                         BOOL NewSeqHdr,
                         BOOL NewQuantMtx,
                         BOOL closeFiles)
{
   static FILE *cmdfile = NULL;
   static FILE *mmefile = NULL;
   U32 j;


/*   zig-zag and alternate scan patterns for loading matrices*/

   const U8 gES_scan[2][64] =
   {
      {  /*Zig-Zag scan pattern*/
         0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
         12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
         35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
         58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
      },
      { /* Alternate scan pattern*/
         0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
         41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
         51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
         53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
      }
   };

/*   default intra quantization matrix*/
   const U8 gES_DefaultIntraQM[64] =
   {
      8, 16, 19, 22, 26, 27, 29, 34,
      16, 16, 22, 24, 27, 29, 34, 37,
      19, 22, 26, 27, 29, 34, 34, 38,
      22, 22, 26, 27, 29, 34, 37, 40,
      22, 26, 27, 29, 32, 35, 40, 48,
      26, 27, 29, 32, 35, 40, 48, 58,
      26, 27, 29, 34, 38, 46, 56, 69,
      27, 29, 35, 38, 46, 56, 69, 83
   };



   if (closeFiles != FALSE)
   {
/*      Calling to close file*/
      if (cmdfile != NULL)
      {
         fprintf(cmdfile,"0xc4\n");
         fclose (cmdfile);
         cmdfile = NULL;
      }
      if (mmefile != NULL)
      {
         fclose (mmefile);
         mmefile = NULL;
      }
      return;
   }

   if (cmdfile == NULL)
   {
	   osclock_t TOut_Now;
      char FileName[30];

      TOut_Now = STOS_time_now();

      sprintf(FileName,"../../CMD-%010u.txt", (U32)TOut_Now);

      cmdfile = fopen(FileName, "w" );

      sprintf(FileName,"../../MME-%010u.txt", (U32)TOut_Now);

      mmefile = fopen(FileName, "w" );

   }

/*   first frame, or the mpeg frame type just changed*/
   if (FirstPictureEver || StreamTypeChange)
   {
      fprintf(cmdfile,"0xc1\n");
      fprintf(mmefile,"------------------------------\n");
      fprintf(mmefile,"Transformer Initial Parameters\n");
      fprintf(mmefile,"------------------------------\n");
      fprintf(mmefile,"MPEG_Decoding_Mode = %d\n",(IsBitStreamMPEG1 == FALSE));
   }

/*    If sequence header or quant mtx ext received
    or sequence extension received  */
   if (NewSeqHdr || NewQuantMtx)
   {
      fprintf(mmefile,"------------------------\n");
      fprintf(mmefile,"Sequence Parameters #%d\n",DecodingOrderFrameID + 1);
      fprintf(mmefile,"------------------------\n");
      fprintf(mmefile,"horizontal_size = %d\n",GlobalParam_p->horizontal_size);
      fprintf(mmefile,"vertical_size = %d\n",GlobalParam_p->vertical_size);
      fprintf(mmefile,"progressive_sequence = %d\n", GlobalParam_p->progressive_sequence); /* k_added_change*/
      fprintf(mmefile,"chroma_format = %d\n", GlobalParam_p->chroma_format);
      for (j=0;j<64;j++) {
         fprintf(mmefile,"intra_quantizer_matrix[%d] = %d\n",j,GlobalParam_p->intra_quantiser_matrix[j]);
      }
      for (j=0;j<64;j++) {
         fprintf(mmefile,"non_intra_quantizer_matrix[%d] = %d\n",j,GlobalParam_p->non_intra_quantiser_matrix[j]);
      }

      /* If the chroma matrices are not specified in the stream, use the luma matrices
       Note that the MME spec says to always pass these as NULL to MME, since nothing
       but 4:2:0 is supported. I'm printing these here so that my output matches
       the output from ref_output.*/
      for (j=0;j<64;j++) {
         fprintf(mmefile,"chroma_intra_quantizer_matrix[%d] = %d\n",j,GlobalParam_p->chroma_intra_quantiser_matrix[j]);
      }
      for (j=0;j<64;j++) {
         fprintf(mmefile,"chroma_non_intra_quantizer_matrix[%d] = %d\n",j,GlobalParam_p->chroma_non_intra_quantiser_matrix[j]);
      }

      fprintf(cmdfile,"0xc2\n");
   }

   fprintf(mmefile,"-----------------------\n");
   fprintf(mmefile,"Picture Parameters #%d\n",DecodingOrderFrameID + 1);
   fprintf(mmefile,"-----------------------\n");
   fprintf(mmefile,"PictureStartAddrCompressedBuffer = 0x%x\n",TransformParam_p->PictureStartAddrCompressedBuffer_p);
   fprintf(mmefile,"PictureStopAddrCompressedBuffer = 0x%x\n",TransformParam_p->PictureStopAddrCompressedBuffer_p);
   fprintf(mmefile,"DecodedLumaAddr = %d\n",TransformParam_p->DecodedBufferAddress.DecodedLuma_p);
   fprintf(mmefile,"DecodedChromaAddr = %d\n",TransformParam_p->DecodedBufferAddress.DecodedChroma_p);
   fprintf(mmefile,"DecodedTemporalReference = %d\n",TransformParam_p->DecodedBufferAddress.DecodedTemporalReferenceValue);
   fprintf(mmefile,"DecimatedLumaAddr = %d\n",TransformParam_p->DecodedBufferAddress.DecimatedLuma_p);
   fprintf(mmefile,"DecimatedChromaAddr = %d\n",TransformParam_p->DecodedBufferAddress.DecimatedChroma_p);
   fprintf(mmefile,"ForwardReferenceLumaAddr = %d\n",TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p);
   fprintf(mmefile,"ForwardReferenceLumaAddr = %d\n",TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p);
   fprintf(mmefile,"ForwardTemporalReference = %d\n",TransformParam_p->RefPicListAddress.ForwardTemporalReferenceValue);
   fprintf(mmefile,"BackwardReferenceChromaAddr = %d\n",TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p);
   fprintf(mmefile,"BackwardReferenceChromaAddr = %d\n",TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p);
   fprintf(mmefile,"BackwardTemporalReference = %d\n",TransformParam_p->RefPicListAddress.BackwardTemporalReferenceValue);
   fprintf(mmefile,"MainAuxEnable = %d\n",TransformParam_p->MainAuxEnable);
   fprintf(mmefile,"HorizontalDecimationFactor = %d\n",TransformParam_p->HorizontalDecimationFactor);
   fprintf(mmefile,"VerticalDecimationFactor = %d\n",TransformParam_p->VerticalDecimationFactor);
   fprintf(mmefile,"DecodingMode = %d\n",TransformParam_p->DecodingMode);
   fprintf(mmefile,"AdditionalFlags = %d\n",TransformParam_p->AdditionalFlags);

   fprintf(mmefile,"picture_coding_type = %d\n",TransformParam_p->PictureParameters.picture_coding_type);

   if (IsBitStreamMPEG1 == TRUE)
   {
      fprintf(mmefile,"forward_horizontal_f_code = %d\n", TransformParam_p->PictureParameters.forward_horizontal_f_code);
      fprintf(mmefile,"forward_vertical_f_code = %d\n",TransformParam_p->PictureParameters.forward_vertical_f_code);
      fprintf(mmefile,"backward_horizontal_f_code = %d\n",TransformParam_p->PictureParameters.backward_horizontal_f_code);
      fprintf(mmefile,"backward_vertical_f_code = %d\n",TransformParam_p->PictureParameters.backward_vertical_f_code);
   }
   else
   {
      fprintf(mmefile,"forward_horizontal_f_code = %d\n", TransformParam_p->PictureParameters.forward_horizontal_f_code);
      fprintf(mmefile,"forward_vertical_f_code = %d\n",TransformParam_p->PictureParameters.forward_vertical_f_code);
      fprintf(mmefile,"backward_horizontal_f_code = %d\n",TransformParam_p->PictureParameters.backward_horizontal_f_code);
      fprintf(mmefile,"backward_vertical_f_code = %d\n",TransformParam_p->PictureParameters.backward_vertical_f_code);
   }
   fprintf(mmefile,"intra_dc_precision = %d\n",TransformParam_p->PictureParameters.intra_dc_precision);
   fprintf(mmefile,"picture_structure = %d\n",TransformParam_p->PictureParameters.picture_structure);
   fprintf(mmefile,"top_field_first = %d\n",((TransformParam_p->PictureParameters.mpeg_decoding_flags & MPEG_DECODING_FLAGS_TOP_FIELD_FIRST) != 0));
   fprintf(mmefile,"frame_pred_frame_dct = %d\n",((TransformParam_p->PictureParameters.mpeg_decoding_flags & MPEG_DECODING_FLAGS_FRAME_PRED_FRAME_DCT) != 0));
   fprintf(mmefile,"concealment_motion_vectors = %d\n",((TransformParam_p->PictureParameters.mpeg_decoding_flags & MPEG_DECODING_FLAGS_CONCEALMENT_MOTION_VECTORS)!=0));
   fprintf(mmefile,"q_scale_type = %d\n",((TransformParam_p->PictureParameters.mpeg_decoding_flags & MPEG_DECODING_FLAGS_Q_SCALE_TYPE)!=0));
   fprintf(mmefile,"intra_vlc_format = %d\n",((TransformParam_p->PictureParameters.mpeg_decoding_flags & MPEG_DECODING_FLAGS_INTRA_VLC_FORMAT)!=0));
   fprintf(mmefile,"alternate_scan = %d\n",((TransformParam_p->PictureParameters.mpeg_decoding_flags & MPEG_DECODING_FLAGS_ALTERNATE_SCAN)!=0));
   fprintf(mmefile,"deblocking_filter_disable = %d\n",((TransformParam_p->PictureParameters.mpeg_decoding_flags & MPEG_DECODING_FLAGS_DEBLOCKING_FILTER_ENABLE)!=0));
   fprintf(cmdfile,"0xc3\n");
   fflush(mmefile);
   fflush(cmdfile);

}
#endif  /* MME_SIMULATION */

/*******************************************************************************
Function:    mpg2dec_InitPrivateData

Parameters:  PrivateData_p

Returns:     None

Scope:       Decoder Internal

Purpose:     Initialize the decoder private data

Behavior:

Exceptions:  None

*******************************************************************************/
static ST_ErrorCode_t mpg2dec_InitPrivateData(MPEG2DecoderPrivateData_t * const PrivateData_p, const DECODER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;


/*   Store information from InitParams that will be needed later,
   and initialize other data to default values.
*/
    PrivateData_p->CPUPartition_p = InitParams_p->CPUPartition_p;

	PrivateData_p->NextCommandId = 500;  /* Arbitrary starting value , not set to 0 for debug purpose */


/*   Initialize the DecodingJobResults structure*/

    PrivateData_p->DecodingJobResults.DecodingJobStatus.JobSubmissionTime = 0;
    PrivateData_p->DecodingJobResults.DecodingJobStatus.JobCompletionTime = 0;
    PrivateData_p->DecodingJobResults.DecodingJobStatus.JobState = DECODER_JOB_UNDEFINED;
    PrivateData_p->DecodingJobResults.CommandId = -1;
    PrivateData_p->DecodingJobResults.ErrorCode = DECODER_NO_ERROR;


/*   Initialize the Decoder Status for the state machine*/
    PrivateData_p->DecoderStatus.CommandId = 0;
    PrivateData_p->DecoderStatus.State = DECODER_STATE_IDLE;
    PrivateData_p->DecoderStatus.IsJobSubmitted = FALSE;
    PrivateData_p->DecoderStatus.DecodingJobStatus = PrivateData_p->DecodingJobResults.DecodingJobStatus;

    PrivateData_p->TransformParamsConfirmed = FALSE;


/*   Initialise commands queue*/

    PrivateData_p->CommandsBuffer.Data_p         = PrivateData_p->CommandsData;
    PrivateData_p->CommandsBuffer.TotalSize      = sizeof(PrivateData_p->CommandsData);
    PrivateData_p->CommandsBuffer.BeginPointer_p = PrivateData_p->CommandsBuffer.Data_p;
    PrivateData_p->CommandsBuffer.UsedSize       = 0;
    PrivateData_p->CommandsBuffer.MaxUsedSize    = 0;

    PrivateData_p->BitBufferAddress_p = InitParams_p->BitBufferAddress_p;
    PrivateData_p->BitBufferSize = InitParams_p->BitBufferSize;

    PrivateData_p->DecoderContext.State = MPEG2DECODER_CONTEXT_NOT_USED;
#ifdef STVID_DEBUG_GET_STATISTICS
    memset (&(PrivateData_p->Statistics), 0, sizeof(PrivateData_p->Statistics));
#endif
    PrivateData_p->CodecMode = InitParams_p->CodecMode;

    return (ErrorCode);
}
/*******************************************************************************
Function:    mpg2dec_Init

Parameters:  DecoderHandle
             InitParams_p

Returns:     ST_NO_ERROR
             STVID_ERROR_EVENT_REGISTRATION - error during evt reg

Scope:       Codec API

Purpose:     Initialize the decoder

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_Init(const DECODER_Handle_t DecoderHandle, const DECODER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t      *DECODER_Data_p;
    MPEG2DecoderPrivateData_t  *PrivateData_p;
    EMBX_ERROR                    EMBXError;
    EMBX_TRANSPORT                tp;
    MME_TransformerInitParams_t   initParams_decoder;
    char                          TransformerName[MME_MAX_TRANSFORMER_NAME];
    U32                          DeviceRegistersBaseAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U8                          TryCount=0;

	if ((DecoderHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;

    /* Allocate DECODER PrivateData structure */
    DECODER_Data_p->PrivateData_p = (MPEG2DecoderPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(MPEG2DecoderPrivateData_t));
    PrivateData_p   = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /*Initialize Private data structure*/
    mpg2dec_InitPrivateData(PrivateData_p, InitParams_p);

    DECODER_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

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

    EMBXError = EMBX_Alloc(tp, sizeof(MPEG2CompanionData_t), (EMBX_VOID **)(&(PrivateData_p->MPEG2CompanionData_p)));
    if(EMBXError != EMBX_SUCCESS)
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
    PrivateData_p->IsEmbxTransportValid = TRUE;

    /*Register the events that will be used to communicate with the Producer*/
    ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_JOB_COMPLETED_EVT, &(PrivateData_p->RegisteredEventsID[MPEG2DECODER_JOB_COMPLETED_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_CONFIRM_BUFFER_EVT, &(PrivateData_p->RegisteredEventsID[MPEG2DECODER_CONFIRM_BUFFER_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName, DECODER_INFORM_READ_ADDRESS_EVT, &(PrivateData_p->RegisteredEventsID[MPEG2DECODER_INFORM_READ_ADDRESS_EVT_ID]));
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->MPEG2CompanionData_p));
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_Free() failed ErrorCode=%x", EMBXError));
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
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module MPEG2 decoder init : could not register events !"));
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    PrivateData_p->MPEG2_InitTransformerParam.InputBufferBegin = (U32)InitParams_p->BitBufferAddress_p;
    PrivateData_p->MPEG2_InitTransformerParam.InputBufferEnd = (U32)InitParams_p->BitBufferAddress_p + InitParams_p->BitBufferSize - 1;
    initParams_decoder.Priority = MME_PRIORITY_NORMAL;
    initParams_decoder.StructSize = sizeof (MME_TransformerInitParams_t);
    initParams_decoder.Callback = &MPEG2_TransformerCallback_decoder;
    initParams_decoder.CallbackUserData = (void *) DecoderHandle; /* UserData */
    initParams_decoder.TransformerInitParamsSize = sizeof (MPEG2_InitTransformerParam_t);
    initParams_decoder.TransformerInitParams_p = &PrivateData_p->MPEG2_InitTransformerParam;

#ifdef STUB_FMW_ST40
    ErrorCode = MPEG2STUB_InitTransformer(MPEG2_MME_TRANSFORMER_NAME, &initParams_decoder, &PrivateData_p->TranformHandleDecoder,InitParams_p->CPUPartition_p);
#else


    /* Init decoder */
#if defined(ST_7200)
    /* Register area is seen with same address between CPU and Device on 7200 */
    DeviceRegistersBaseAddress = (U32)InitParams_p->RegistersBaseAddress_p;
#else /* defined(ST_7200) */
    DeviceRegistersBaseAddress = (U32)STAVMEM_CPUToDevice(InitParams_p->RegistersBaseAddress_p,&VirtualMapping);
#endif /* defined(ST_7200) */

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
#if defined(ST_DELTAMU_GREEN_HE)
      DeviceRegistersBaseAddress = 0xAC220000;
#else
      DeviceRegistersBaseAddress = 0xAC211000;
#endif /* ST_DELTAMU_GREEN_HE */
#endif /* (ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */
    sprintf(TransformerName, "%s_%08X", MPEG2_MME_TRANSFORMER_NAME, DeviceRegistersBaseAddress);

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
        EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->MPEG2CompanionData_p));
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_Free() failed ErrorCode=%x", EMBXError));
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
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_InitTransformer(decoder) failed, result=$%lx", ErrorCode));
        return (ErrorCode);
    }

#if defined(MME_DUMP) /*&& defined(VALID_TOOLS)*/
     MPEG2_HDM_Init();
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */

#ifdef STVID_DEBUG_GET_STATUS
	PrivateData_p->Status.SequenceInfo_p = NULL;
#endif /* STVID_DEBUG_GET_STATUS */


   return (ErrorCode);
} /* End of DECODER_Init function */

/*******************************************************************************
Function:    mpg2dec_Term

Parameters:  DecoderHandle
             CommandId
             DecoderStatus_p

Returns:     ST_NO_ERROR

Scope:       Codec API

Purpose:     Terminate the current frame decode

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_Term(const DECODER_Handle_t DecoderHandle)
{
    ST_ErrorCode_t                  ErrorCode        = ST_NO_ERROR;
    ST_ErrorCode_t                  MMETermErrorCode = ST_NO_ERROR;
    DECODER_Properties_t *          DECODER_Data_p;
    MPEG2DecoderPrivateData_t *     PrivateData_p;
    U32                             TermCounter;
    EMBX_ERROR                      EMBXError;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (MPEG2DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

#ifdef PRINT_DECODER_INPUT
	fclose(PrivateData_p->DebugFile);
#endif  /*PRINT_DECODER_INPUT*/

	/* terminate MME API */
#ifdef STUB_FMW_ST40
    MPEG2STUB_TermTransformer(PrivateData_p->TranformHandleDecoder, PrivateData_p->CPUPartition_p);
#else
    TermCounter = 0;
    do
    {
        ErrorCode = MME_TermTransformer(PrivateData_p->TranformHandleDecoder);
        if (ErrorCode == MME_DRIVER_NOT_INITIALIZED)
        {
            ErrorCode = MME_SUCCESS;
            MMETermErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        if (ErrorCode == MME_INVALID_HANDLE)
        {
            ErrorCode = MME_SUCCESS;
            MMETermErrorCode = ST_ERROR_INVALID_HANDLE;
        }
        else
        if (ErrorCode == MME_COMMAND_STILL_EXECUTING)
        {
            /* wait for command completion 40 ms stands for max frame decode time */
            STOS_TaskDelayUs(40000);  /* 40 ms */
        }
        TermCounter++;
    }
    while ((ErrorCode != MME_SUCCESS) && (TermCounter < MAX_TRY_FOR_MME_TERM_TRANSFORMER));
    if (ErrorCode == MME_COMMAND_STILL_EXECUTING)
    {
        MMETermErrorCode = ST_ERROR_DEVICE_BUSY;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Term Failed to terminate transformer (A command is still executing)"));
    }
#endif

    if (ErrorCode != MME_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_TermTransformer(decoder) failed, result=$%lx", ErrorCode));
        return (ErrorCode);
    }

    /* deallocate memory  */
    if (PrivateData_p != NULL)
    {
        /* Release MPEG2 companion allocated memory */
        EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->MPEG2CompanionData_p));
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_Free() failed ErrorCode=%x", EMBXError));
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

#if defined(MME_DUMP) /*&& defined(VALID_TOOLS)*/
    MPEG2_HDM_Term();
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = MMETermErrorCode;
    }
    return (ErrorCode);
}

/*******************************************************************************
Name        : mpg2dec_FillAdditionnalDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t mpg2dec_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t * const Picture_p)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(Picture_p);

    return(ST_NO_ERROR);
} /* End of mpg2dec_FillAdditionnalDataBuffer() function */

/*******************************************************************************
Function:    mpg2dec_Setup

Parameters:  SetupParams_p

Returns:     ST_ERROR_FEATURE_NOT_SUPPORTED

Scope:       Codec API

Purpose:     Not supported in MPEG2 decoder

Behavior:

Exceptions:  None

*******************************************************************************/
ST_ErrorCode_t mpg2dec_Setup(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(SetupParams_p);
    return (ST_ERROR_FEATURE_NOT_SUPPORTED);
}

#ifdef ST_speed
/*******************************************************************************
Name        : mpg2dec_SetDecodingMode
Description : Use to set degraded decode mode. Not yet implemented
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void mpg2dec_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode)
{
	UNUSED_PARAMETER(DecoderHandle);
	UNUSED_PARAMETER(Mode);
}
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : mpg2dec_SetDecodingDirection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void mpg2dec_SetDecodingDirection(const DECODER_Handle_t DecoderHandle,const DecodingDirection_t Direction)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(Direction);
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
ST_ErrorCode_t mpg2dec_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(BitBufferAddress_p);
    UNUSED_PARAMETER(BitBufferSize);

    return (ST_ERROR_FEATURE_NOT_SUPPORTED);
}   /* End of mpg2dec_UpdateBitBufferParams */

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
ST_ErrorCode_t mpg2dec_SetupForH264PreprocWA_GNB42332(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(AVMEMPartition);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of mpg2dec_SetupForH264PreprocWA_GNB42332 */
#endif /* DVD_SECURED_CHIP */

/* End of mpg2dec.c */
