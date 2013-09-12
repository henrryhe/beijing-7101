/*******************************************************************************

File name   : avsdecode.c

Description : AVS interface decoding functions for MME.

COPYRIGHT (C) STMicroelectronics 2008

Date               Modification                                     Name
----               ------------                                     ----
29 January 2008    First STAPI version                              PLE
*******************************************************************************/


/* The following flag avoids a crash in the delta when we use the buffer handler
 * to get a picture which size is equal or less than 9 bytes */
#define WA_GNBvd44003
/* Includes ----------------------------------------------------------------- */
#include "vidcodec.h"
#include "avsdecode.h"

#define MAX_TRY_FOR_MME_TERM_TRANSFORMER 10 /* max loop number for MME_TermTransformer completion waiting loop in DECODER_Term */

#define AVS_DEBUG_DECODER        0
#define AVS_DEBUG_DECODER_TIME   0
#define AVS_DEBUG_DECODER_SWITCH 0
#define AVS_OUTPUT_PERFORMANCES  0

#if AVS_DEBUG_DECODER

#include "testtool.h"
#include "vid_ctcm.h"

GENERIC_Time_t DecodeStartTime=0, DecodeEndTime=0, DecodeTime1=0, DecodeTime2=0;
GENERIC_Time_t ReinitStartTime=0, ReinitEndTime=0, ReinitTime1=0;
GENERIC_Time_t TermStartTime=0, TermEndTime=0, TermTime1=0,TotalTime1=0;
U32 CptFrame =0, FrameType=0, DecodeTimeMin=20000, DecodeTimeMax=0, TotalTime=0;
GENERIC_Time_t AbortTime=0, DecodeTime=0;
U16 AbortDone = false;
U32 CptAbort =0;

#endif /* AVS_DEBUG_DECODER */



#if AVS_OUTPUT_PERFORMANCES
static char VID_Msg[1024];
#endif /* AVS_OUTPUT_PERFORMANCES */

const DECODER_FunctionsTable_t DECODER_AVSFunctions =
{
    AVSDECODER_GetState,
    AVSDECODER_GetCodecInfo,
#ifdef STVID_DEBUG_GET_STATISTICS
    AVSDECODER_GetStatistics,
    AVSDECODER_ResetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
    AVSDECODER_Init,
    AVSDECODER_Reset,
    AVSDECODER_DecodePicture,
    AVSDECODER_ConfirmBuffer,
    AVSDECODER_Abort,
    AVSDECODER_Setup,
    AVSDECODER_FillAdditionnalDataBuffer,
    AVSDECODER_Term
#ifdef ST_speed
    ,AVSDECODER_SetDecodingMode
#ifdef STVID_TRICKMODE_BACKWARD
    ,AVSDECODER_SetDecodingDirection
#endif /* STVID_TRICKMODE_BACKWARD */
#endif /* ST_SPEED */
    ,AVSDECODER_UpdateBitBufferParams
};

/* Macro ---------------------------------------------------------------- */
/*#define TRACE_AVSDECODER_STATUS*/
/*#define  TRACE_AVS_BIT_BUFFER*/
/* #define TRACE_AVSDECODER_JOBS */
/* #define TRACE_AVSFIRMWARE_ERRORS */
/* #define TRACE_AVSDECODE_START */
/* #define TRACE_MME_CALLS */
/* #define TRACE_DECODED_PICTURE_TYPE */
/* #define TRACE_DECODE_DURATION */

/* The following flag will introduce decode errors in the stream. This allows
 * testing the error recovery */
/* #define DEBUG_REPORT_ERRORS_IN_THE_STREAM */

#ifdef TRACE_DECODE_DURATION
#define TraceDecodeDuration(x) TraceState x
#else
#define TraceDecodeDuration(x)
#endif  /* TRACE_DECODE_DURATION */

#ifdef TRACE_DECODED_PICTURE_TYPE
#define TraceDecodedPictureType(x) TraceState x
#else
#define TraceDecodedPictureType(x)
#endif  /* TRACE_DECODED_PICTURE_TYPE */

#ifdef TRACE_AVSDECODER_STATUS
#define TraceAvsDecoderStatus(x) TraceState x
#else
#define TraceAvsDecoderStatus(x)
#endif  /* TRACE_AVSDECODER_STATUS */

#ifdef TRACE_MME_CALLS
#define TraceMMECall(x) TraceState x
#else
#define TraceMMECall(x)
#endif  /* TRACE_MME_CALLS */

#ifdef TRACE_AVS_BIT_BUFFER
#define TraceBitBuffer(x) TraceVal x
#else
#define TraceBitBuffer(x)
#endif /* TRACE_DISPLAY_FRAME_BUFFER */

#ifdef TRACE_AVSDECODER_JOBS
#define TraceAvsDecoderJobs(x) TraceState x
#else
#define TraceAvsDecoderJobs(x)
#endif  /* TRACE_AVSDECODER_STATUS */

#ifdef TRACE_AVSFIRMWARE_ERRORS
#define TraceAvsFirmwareErrors(x) TraceState x
#else
#define TraceAvsFirmwareErrors(x)
#endif  /* TRACE_AVSFIRMWARE_ERRORS */

#define JobResultToString(x) ( ((x) == AVS_NO_ERROR) ? "AVS_NO_ERROR" : \
                               ((x) == AVS_ILLEGAL_MB_NUM_PCKT) ? "AVS_ILLEGAL_MB_NUM_PCKT" : \
                               ((x) == AVS_CBPY_DAMAGED) ? "AVS_CBPY_DAMAGED" : \
                               ((x) == AVS_CBPC_DAMAGED) ? "AVS_CBPC_DAMAGED" : \
                               ((x) == AVS_ILLEGAL_MB_TYPE) ? "AVS_ILLEGAL_MB_TYPE" : \
                               ((x) == AVS_DC_DAMAGED) ? "AVS_DC_DAMAGED" : \
                               ((x) == AVS_AC_DAMAGED) ? "AVS_AC_DAMAGED" : \
                               ((x) == AVS_MISSING_MARKER_BIT) ? "AVS_MISSING_MARKER_BIT" : \
                               ((x) == AVS_HEADER_DAMAGED) ? "AVS_HEADER_DAMAGED" : \
                               ((x) == AVS_INVALID_PICTURE_SIZE) ? "AVS_INVALID_PICTURE_SIZE" : \
                               ((x) == AVS_GMC_NOT_SUPPORTED) ? "AVS_GMC_NOT_SUPPORTED" : \
                               ((x) == AVS_QPEL_NOT_SUPPORTED) ? "AVS_QPEL_NOT_SUPPORTED" : \
                               ((x) == AVS_FIRMWARE_TIME_OUT_ENCOUNTERED) ? "AVS_FIRMWARE_TIME_OUT_ENCOUNTERED" : "?")

/* ---------------------------------------------------------------------- */

/******************************************************************************
* \name  AVSDECODER_Init
* \brief Called by the producer, to initialize the decoder module
*
* \param DecoderHandle
* \param InitParams_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_Init(const DECODER_Handle_t       DecoderHandle,
                                   const DECODER_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t                        ST_ErrorCode = ST_NO_ERROR;

    DECODER_Properties_t                * DECODER_Data_p;
    AVSDecoderPrivateData_t             * PrivateData_p;
    /* STEVT_OpenParams_t                    STEVT_OpenParams; */
    /* STAVMEM_AllocBlockParams_t            AllocBlockParams; */
    U32                                   FrameWidth, FrameHeight;
    U32                                   Version;
    /* MME_DataBuffer_t                    **DataBuffers; */
    /* void                                * VirtualAddress; */
    /* void                                * DeviceAddress; */
    /* void                                * CPUAddress; */
    U32                                   i ,j;

    EMBX_TRANSPORT                        transport;
    EMBX_ERROR                            EMBXError;

    U32                                   DeviceRegistersBaseAddress;

    /* Check if input paramaters are valid */
    if ((DecoderHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set handle pointer */
    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;

    /* Allocate DECODER PrivateData structure */
    DECODER_Data_p->PrivateData_p = (AVSDecoderPrivateData_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(AVSDecoderPrivateData_t));
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    if (PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    PrivateData_p->IsEmbxTransportValid = FALSE;

    /* DataBuffers pointer allocation */
#if   defined(ST_7100) || defined(ST_7109)
    EMBXError = EMBX_OpenTransport(MME_VIDEO_TRANSPORT_NAME, &transport);
#elif defined(ST_7200)
    EMBXError = EMBX_OpenTransport(InitParams_p->MMETransportName, &transport);
#endif /* #elif defined(ST_7200) ... */
    if(EMBXError != EMBX_SUCCESS)
    {
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        return(ST_ERROR_NO_MEMORY);
    }
    PrivateData_p->EmbxTransport = transport;
    PrivateData_p->IsEmbxTransportValid = TRUE;

    /* Avmem Partition used for Mailbox allocation */
    DECODER_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

    /* Save CPU partition (used for deallocation) */
    PrivateData_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    /* DataBuffers allocation */
    EMBXError = EMBX_Alloc(transport, NUM_MME_AVS_BUFFERS * sizeof(MME_DataBuffer_t *),
                           (EMBX_VOID **)(&(PrivateData_p->CmdDecoder.DataBuffers_p)));
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

    for(i=0; i< NUM_MME_AVS_BUFFERS; i++)
    {
        EMBXError = EMBX_Alloc(transport, sizeof(MME_DataBuffer_t),
                               (EMBX_VOID **)(&(PrivateData_p->CmdDecoder.DataBuffers_p[i])));
        if(EMBXError != EMBX_SUCCESS)
        {
            if (i > 0)
            {
                for(j=0; j< i; j++)
                {
                    EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[j]));
                }
            }
            EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p));
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

    /* ScatterPages allocation */
    for(i=0; i< NUM_MME_AVS_BUFFERS; i++)
    {
        EMBXError = EMBX_Alloc(transport, sizeof(MME_ScatterPage_t),
                               (EMBX_VOID **)(&(PrivateData_p->CmdDecoder.DataBuffers_p[i]->ScatterPages_p)));
        if(EMBXError != EMBX_SUCCESS)
        {
            if (i > 0)
            {
                for(j=0; j< i; j++)
                {
                    EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[j]->ScatterPages_p));
                }
            }
            for(j=0; j< NUM_MME_AVS_BUFFERS; j++)
            {
                EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[j]));
            }
            EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p));
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

    /* Get virtual mapping for addresses translations */
    STAVMEM_GetSharedMemoryVirtualMapping(&(PrivateData_p->VirtualMapping));

    /*
     Pb: The decoder FW requires width and height for the initialize transformer call,
         but these informations are given by the parser when the producer calls xxxDECODER_DecodePicture
         via the DECODER_DecodePictureParams_t structure.
    */

    /* Default values, Width and Height in MB even number */
    FrameWidth      = 720;
    FrameHeight     = 576;
    Version         = 1;

    /* Index of the decode job, just incremented by one on each new job */
    /* Used by the producer to manage decode jobs. */
    /* Arbitrary starting value, (not set to 0 to see the first change in debug) */
    PrivateData_p->NextCommandId = 1;

    /* Initialize DecoderState */
    PrivateData_p->DecoderState = AVSDECODER_CONTEXT_NOT_USED;

    /* Get and store bit buffer address and size */
    PrivateData_p->BitBufferAddress_p   = InitParams_p->BitBufferAddress_p;
    PrivateData_p->BitBufferSize        = InitParams_p->BitBufferSize;

     /* Reference address pointer initialized to NULL */
    PrivateData_p->BitBuffer_p            = NULL;
    PrivateData_p->OutputBuffer_p        = NULL;
    PrivateData_p->ForwardReference_p   = NULL;
    PrivateData_p->BackwardReference_p  = NULL;

    PrivateData_p->Omeg2OutputLumaBuffer_p = NULL;
    PrivateData_p->Omeg2OutputChromaBuffer_p = NULL;
    PrivateData_p->Omeg2ForwardLumaReference_p = NULL;
    PrivateData_p->Omeg2ForwardChromaReference_p = NULL;
    PrivateData_p->Omeg2BackwardLumaReference_p = NULL;
    PrivateData_p->Omeg2BackwardChromaReference_p = NULL;

    PrivateData_p->CurrPicDistanceMsb = 0;
    PrivateData_p->PrevPicDistanceLsb = 0;
    PrivateData_p->PicDistanceMsb = 0;

    PrivateData_p->LastEndOfffset = 0;

    /* Register the two events, sent by the AVS callback function when firmware has finished to decode */
    ST_ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName,
                                             DECODER_JOB_COMPLETED_EVT,
                                             &(PrivateData_p->RegisteredEventsID[AVSDECODER_JOB_COMPLETED_EVT_ID]));

    if (ST_ErrorCode == ST_NO_ERROR)
    {
        ST_ErrorCode = STEVT_RegisterDeviceEvent(DECODER_Data_p->EventsHandle, InitParams_p->VideoName,
                                                 DECODER_INFORM_READ_ADDRESS_EVT,
                                                 &(PrivateData_p->RegisteredEventsID[AVSDECODER_INFORM_READ_ADDRESS_EVT_ID]));
    }

    ST_ErrorCode = AVS_UpdateTransformer(DecoderHandle, FrameWidth, FrameHeight, Version);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"AVS_UpdateTransformer(decoder) failed, result=$%lx", ST_ErrorCode));
        for(j=0; j< NUM_MME_AVS_BUFFERS; j++)
        {
            EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[j]->ScatterPages_p));
        }
        for(j=0; j< NUM_MME_AVS_BUFFERS; j++)
        {
            EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[j]));
        }
        EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p));
        EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
        }
        PrivateData_p->IsEmbxTransportValid = FALSE;
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        return (ST_ErrorCode);
    }

#if AVS_DEBUG_DECODER_TIME
     ReinitStartTime = time_now();
#endif /*AVS_DEBUG_DECODER_TIME*/

    #if defined(ST_7200)
    /* Register area is seen with same address between CPU and Device on 7200 */
    DeviceRegistersBaseAddress = (U32)InitParams_p->RegistersBaseAddress_p;
    #else /* defined(ST_7200) */
    DeviceRegistersBaseAddress = (U32)STAVMEM_CPUToDevice(InitParams_p->RegistersBaseAddress_p,&PrivateData_p->VirtualMapping);
    #endif /* defined(ST_7200) */

    sprintf(PrivateData_p->TransformerName, "%s_%08X", AVS_MME_TRANSFORMER_NAME, DeviceRegistersBaseAddress);
    /*sprintf(PrivateData_p->TransformerName, "%s", AVS_MME_TRANSFORMER_NAME);*/

    /* Call MME_InitTransformer specific to this AVS Avs decoder */
    TraceMMECall(("MME","INIT"));
    ST_ErrorCode = MME_InitTransformer(PrivateData_p->TransformerName, &PrivateData_p->InitParamsDecoder, &PrivateData_p->TranformHandleDecoder);

#if AVS_DEBUG_DECODER_TIME
    ReinitEndTime = time_now();
#endif

    if (ST_ErrorCode == MME_SUCCESS)
    {
        /* Store the picture parameters to check if they change along the stream */
        PrivateData_p->StoredInitParams.width           = FrameWidth;
        PrivateData_p->StoredInitParams.height          = FrameHeight;
        PrivateData_p->StoredInitParams.Progressive_sequence = Version;
    }
    else if (ST_ErrorCode != MME_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_InitTransformer(decoder) failed, result=$%lx", ST_ErrorCode));
        for(j=0; j< NUM_MME_AVS_BUFFERS; j++)
        {
            EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[j]->ScatterPages_p));
        }
        for(j=0; j< NUM_MME_AVS_BUFFERS; j++)
        {
            EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[j]));
        }
        EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p));
        EMBXError = EMBX_CloseTransport(PrivateData_p->EmbxTransport);
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_CloseTransport() failed ErrorCode=%x", EMBXError));
        }
        PrivateData_p->IsEmbxTransportValid = FALSE;
        STOS_MemoryDeallocate(InitParams_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
        return (ST_ErrorCode);
    }
 #ifdef TRACE_AVSDECODER_STATUS
    TraceAvsDecoderStatus(("DecoderContxtStatus", "%s", (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_NOT_USED)  ? "NOT_USED" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_DECODE_FUNC)             ? "DECODE_FUNC" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC)      ? "CONFIRM_BUFFER" :"INVALID"
                    ));
 #endif /* TRACE_AVSDECODER_STATUS */

    return (ST_ErrorCode);
} /* End of DECODER_Init function */

/******************************************************************************
* \name  AVSDECODER_DecodePicture
* \brief Called by the producer, to know the state of the decoder module, and to
*        initialize it.
*
* \param DecoderHandle
* \param DecodePictureParams_p
* \param DecoderStatus_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_DecodePicture (const DECODER_Handle_t                 DecoderHandle,
                                             const DECODER_DecodePictureParams_t *  const DecodePictureParams_p,
                                             DECODER_Status_t *                     const DecoderStatus_p)
{
    ST_ErrorCode_t                        ST_ErrorCode = ST_NO_ERROR;
    MME_AVSVideoDecodeParams_t         * fmw_p;
    DECODER_Properties_t                * DECODER_Data_p;
    AVSDecoderPrivateData_t         * PrivateData_p;
    VIDCOM_PictureGenericData_t         * PictGenData_p;
    VIDCOM_ParsedPictureInformation_t   * ParsedPicture_p;
    AVS_PictureSpecificData_t       * PictSpecData_p;
    STVID_SequenceInfo_t                * SequenceInfo_p;
    U32                                   temp;
#if AVS_DEBUG_DECODER
    U32                                   change;
#endif

    /* DECODER_Data pointer check */
    if ((DecoderHandle == NULL) || (DecodePictureParams_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                     "DECODER (AVSDECODER_DecodePicture) : DecoderHandle %x  DecodePictureParams_p %x ",
                      DecoderHandle, DecodePictureParams_p));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* DECODER_Data pointer initialisation */
    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;

    /* PrivateData pointer initialisation */
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;
    fmw_p = &PrivateData_p->TransformParam;
    if (PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* DecoderState set to AVSDECODER_CONTEXT_NOT_USED in AVSDECODER_Init() */
    /* If decoding active, return error (it comes from H264 wich requires N decoding in parallel) */
    if (PrivateData_p->DecoderState != AVSDECODER_CONTEXT_NOT_USED)
    {
        /* Decoder is working */
        DecoderStatus_p->State = DECODER_STATE_DECODING;
        /* Store decoder status for the GetState function */
        PrivateData_p->DecoderStatus.State = DecoderStatus_p->State;
        /* This job is rejected */
        DecoderStatus_p->IsJobSubmitted = FALSE;
        /* Time stamp */
        DecoderStatus_p->DecodingJobStatus.JobCompletionTime = time_now();

        return(ST_ERROR_NO_MEMORY);
    }

#if AVS_DEBUG_DECODER_TIME
    else
    {

        DecodeTime2 = (time_minus (DecodeEndTime, DecodeStartTime) * 10000) / (osclock_t)ST_GetClocksPerSecond();
        if (DecodeTime2)
            if (DecodeTime2 < DecodeTimeMin)
                DecodeTimeMin = DecodeTime2;
        if (DecodeTime2 > DecodeTimeMax)
            DecodeTimeMax = DecodeTime2;
        TotalTime += DecodeTime2;

        /*
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Frame:%d, Type:%d \n",CptFrame,FrameType));
        */

        if (CptFrame%16 == 0)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Time:%d, ",(U32)DecodeTime2));
            if (CptFrame != 0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Average x10:%d \n",TotalTime/CptFrame));
                STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Min:%d, Max:%d\n",DecodeTimeMin,DecodeTimeMax));
                /* STTBX_Print(("Average x10:%d ",TotalTime/CptFrame)); */
                /* STTBX_Print(("Min:%d, Max:%d\n",DecodeTimeMin,DecodeTimeMax)); */
            }
        }
    }
    change = false;
#endif /* AVS_DEBUG_DECODER_TIME */

    /* Used to know if decoder is soon decoding or ready to decode */
    PrivateData_p->DecoderState = AVSDECODER_CONTEXT_DECODE_FUNC;

    /* Set pointer on picture informations given by the parser */
    ParsedPicture_p = &(DecodePictureParams_p->PictureInformation->ParsedPictureInformation);
    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;

    /* Check if frame size or codec version have changed: if so, reinit the firmware */
    if (PictGenData_p->PictureInfos.BitmapParams.Width != PrivateData_p->StoredInitParams.width ||
       PictGenData_p->PictureInfos.BitmapParams.Height != PrivateData_p->StoredInitParams.height ||
       PictSpecData_p->ProgressiveSequence != PrivateData_p->StoredInitParams.Progressive_sequence)
    {
#if AVS_DEBUG_DECODER_TIME
        TermStartTime = time_now();
#endif /*AVS_DEBUG_DECODER_TIME*/

        /* Picture parameters have changed, so close transformer and then re-init it */
        TraceMMECall(("MME","TERM"));
        ST_ErrorCode = MME_TermTransformer(PrivateData_p->TranformHandleDecoder);
        if (ST_ErrorCode != MME_SUCCESS)
        {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_TermTransformer(decoder) failed, result=$%lx", ST_ErrorCode));
           return (ST_ErrorCode);
        }
        else
        {
#if AVS_DEBUG_DECODER_TIME
            TermEndTime = time_now();
#endif
            /*Reinitialize the firmware */
            ST_ErrorCode = AVS_UpdateTransformer(DecoderHandle, PictGenData_p->PictureInfos.BitmapParams.Width,
                                                      PictGenData_p->PictureInfos.BitmapParams.Height, PictSpecData_p->ProgressiveSequence);
            if (ST_ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"AVS_UpdateTransformer(decoder) failed, result=$%lx", ST_ErrorCode));
                return (ST_ErrorCode);
            }
#if AVS_DEBUG_DECODER_TIME
            ReinitStartTime = time_now();
#endif /*AVS_DEBUG_DECODER_TIME*/
          /* Call MME_InitTransformer specific to this AVS Avs decoder */
            TraceMMECall(("MME","INIT"));
            ST_ErrorCode = MME_InitTransformer(PrivateData_p->TransformerName, &PrivateData_p->InitParamsDecoder, &PrivateData_p->TranformHandleDecoder);
#if AVS_DEBUG_DECODER_TIME
          ReinitEndTime = time_now();
#endif /*AVS_DEBUG_DECODER_TIME*/

          if (ST_ErrorCode != MME_SUCCESS)
          {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_InitTransformer(decoder) failed, result=$%lx", ST_ErrorCode));
            return (ST_ErrorCode);
          }
          else
          {
             /* Store the new picture parameters */
             PrivateData_p->StoredInitParams.width = PictGenData_p->PictureInfos.BitmapParams.Width;
             PrivateData_p->StoredInitParams.height = PictGenData_p->PictureInfos.BitmapParams.Height;
             PrivateData_p->StoredInitParams.Progressive_sequence= PictSpecData_p->ProgressiveSequence;

 #if AVS_DEBUG_DECODER_TIME
             change = true;
#endif /*AVS_DEBUG_DECODER_TIME*/
          }
      }
    }

 #if AVS_DEBUG_DECODER_TIME

    if (change == true)
    {
        TermTime1 = (time_minus (TermEndTime, TermStartTime) *1000000) / (osclock_t)ST_GetClocksPerSecond();
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"TermTime1:%d in us\n",TermTime1));

        ReinitTime1 = (time_minus (ReinitEndTime, ReinitStartTime) *1000000) / (osclock_t)ST_GetClocksPerSecond();
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"ReinitTime1:%d in us\n",ReinitTime1));

        TotalTime1 = (time_minus (ReinitEndTime, TermStartTime) *1000000) / (osclock_t)ST_GetClocksPerSecond();
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"TotalTime1:%d in us\n",TotalTime1));
    }
#endif /*AVS_DEBUG_DECODER_TIME*/

    /* Get pointer on sequence informations */
    SequenceInfo_p  = &(ParsedPicture_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo);

    /* calculate Picture start offset from start of BitBuffer */
    temp  = (U32)(U32 *)PictGenData_p->BitBufferPictureStartAddress;
    temp -= (U32)(U32 *)PrivateData_p->BitBufferAddress_p;
    fmw_p->StartOffset = temp;
#ifdef  TRACE_AVS_BIT_BUFFER
    TraceBitBuffer(("PictStartOffset", fmw_p->StartOffset));
#endif /*TRACE_AVS_BIT_BUFFER*/
    /* calculate Picture end offset from start of BitBuffer */
    temp  = (U32)(U32 *)PictGenData_p->BitBufferPictureStopAddress;
    temp -= (U32)(U32 *)PrivateData_p->BitBufferAddress_p;
    fmw_p->EndOffset = temp;
/*Added by LQ to support AVS FW R2*/
    fmw_p->alpha_offset = (int)PictSpecData_p->AlphaCOffset;
    fmw_p->beta_offset = (int)PictSpecData_p->BetaOffset;
    fmw_p->Fixed_picture_qp= (MME_UINT)PictSpecData_p->FixedPictureQP;
    fmw_p->FrameType = (MME_UINT)PictSpecData_p->PictureCodingType;
    fmw_p->Loop_filter_disable = (MME_UINT)PictSpecData_p->LoopFilterDisable;
    fmw_p->Picture_qp = (MME_UINT)PictSpecData_p->PictureQP;
    fmw_p->Picture_ref_flag =(int)(PictSpecData_p->PictureReferenceFlag);
    if(fmw_p->Picture_ref_flag == 165)
        fmw_p->Picture_ref_flag = 1;

    fmw_p->Picture_structure = (MME_UINT)PictSpecData_p->PictureStructure;
    fmw_p->Skip_mode_flag = (MME_UINT)PictSpecData_p->SkipModeFlag;
    fmw_p->topfield_pos = (unsigned int)(fmw_p->StartOffset);/* ?????PictSpecData_p->topfield_pos*/;
    fmw_p->tr = (int)(PictSpecData_p->PictureDistance);
/*    fmw_p->Progressive_frame = (MME_UINT)(PictSpecData_p->Progressive_frame);*/
    if((MME_UINT)(PictSpecData_p->Progressive_frame) == 1)
        fmw_p->botfield_pos = (unsigned int) 0;


/*  fmw_p->StartOffset = 0;//As  linear buffer mode
fmw_p->StartOffset = (U32)(U32 *)PictGenData_p->BitBufferPictureStartAddress;*/
/*End*/
#ifdef WA_GNBvd44003
    {
        /* Avoid crashing the delta, if the picture's size is equal or less than
         * 9 bytes. */
        U32 PictureSize;

        /* Compute the picture's size */
        if(fmw_p->EndOffset >= fmw_p->StartOffset)
        {
            PictureSize = fmw_p->EndOffset - fmw_p->StartOffset + 1;
        }
        else
        {
            PictureSize = PrivateData_p->BitBufferSize - (fmw_p->StartOffset - fmw_p->EndOffset - 1);
        }

        /* Increase picture size to 9 bytes if it is less, and update again
         * the EndOffset of the picture */
        if(PictureSize < 9)
        {
            PictureSize = 9;
            fmw_p->EndOffset = fmw_p->StartOffset + PictureSize;
            if(fmw_p->EndOffset >= PrivateData_p->BitBufferSize)
            {
                fmw_p->EndOffset -= PrivateData_p->BitBufferSize;
            }
        }
    }
#endif /* WA_GNBvd44003 */

#ifdef  TRACE_AVS_BIT_BUFFER
    TraceBitBuffer(("PictEndOffset", fmw_p->EndOffset));
#endif /*TRACE_AVS_BIT_BUFFER*/

    /* for test
    fmw_p->EndOffset = 0;  Linear buffer mode
    fmw_p->EndOffset =(U32)(U32 *)PictGenData_p->BitBufferPictureStopAddress;*/

    /* Use circle buffer mode */
    if((U32)PrivateData_p->LastEndOfffset > (U32)fmw_p->EndOffset)  /* PLE_TO_DO : check the pertinence of this added cast */
    {
        if(fmw_p->StartOffset > fmw_p->topfield_pos )
        {
            fmw_p->topfield_pos = (unsigned int)(fmw_p->topfield_pos +(PrivateData_p->BitBufferSize - fmw_p->StartOffset));
        }
        else
        {
            fmw_p->topfield_pos = (unsigned int)(fmw_p->topfield_pos - fmw_p->StartOffset);
        }
    }
    else
    {
        if(PrivateData_p->LastEndOfffset == 0)
        {
            fmw_p->StartOffset = 0;
        }
        else
        {
            fmw_p->StartOffset = (MME_UINT) PrivateData_p->LastEndOfffset; /* PLE_TO_DO : check the pertinence of this added cast */
            fmw_p->topfield_pos = (unsigned int)(fmw_p->topfield_pos - fmw_p->StartOffset);
            fmw_p->botfield_pos = (unsigned int) fmw_p->topfield_pos;

        }
    }
    PrivateData_p->LastEndOfffset =  (U32 *)fmw_p->EndOffset;

    /* Store picture stop address, used later by the producer via the callback */
    PrivateData_p->DecoderReadAddress.DecoderReadAddress_p = (U32 *) PictGenData_p->BitBufferPictureStopAddress;

    /* Used to store the command index, send to the producer to identify this decode job     */
    PrivateData_p->CommandId = PrivateData_p->NextCommandId;
    PrivateData_p->NextCommandId++;

    /* Used to managed time-out */
    DecoderStatus_p->DecodingJobStatus.JobSubmissionTime = time_now();
    /* Decoder is waiting for a frame buffer send by the producer, so it is ready. */
    DecoderStatus_p->State = DECODER_STATE_READY_TO_DECODE;
    /* Store decoder status for the GetState function */
    PrivateData_p->DecoderStatus.State = DecoderStatus_p->State;
    /* The decode job is accepted, return Ok */
    DecoderStatus_p->IsJobSubmitted = TRUE;
    /* Decode job identifier returned to the producer */
    DecoderStatus_p->CommandId = PrivateData_p->CommandId;
    /* Codec doesn't need any pre-processing step. So ask to the producer to send the frame buffer */
    DecoderStatus_p->DecodingJobStatus.JobState = DECODER_JOB_WAITING_CONFIRM_BUFFER;
#ifdef TRACE_AVSDECODER_JOBS
    TraceAvsDecoderJobs(("DecoderJobs", "%s", "WAITING_BUFF"));
#endif /*TRACE_AVSDECODER_JOBS*/
#ifdef TRACE_AVSDECODER_STATUS
    TraceAvsDecoderStatus(("DecoderContxtStatus", "%s", (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_NOT_USED)  ? "NOT_USED" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_DECODE_FUNC)             ? "DECODE_FUNC" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC)      ? "CONFIRM_BUFFER" :"INVALID"
                    ));
#endif/*TRACE_AVSDECODER_STATUS*/
    /* Error code is Ok (no error). Then the producer will call the confirm_buffer() function */
    return(ST_ErrorCode);

} /* End of AVSDECODER_DecodePicture function */


/******************************************************************************
* \name  AVSDECODER_ConfirmBuffer
* \brief Initialize the decoder module
*
* \param DecoderHandle
* \param ConfirmBufferParams_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_ConfirmBuffer(const DECODER_Handle_t DecoderHandle,
                                          const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p)
{
    ST_ErrorCode_t                        ST_ErrorCode = ST_NO_ERROR;
    MME_AVSVideoDecodeParams_t          * fmw_p;
    DECODER_Properties_t                * DECODER_Data_p;
    AVSDecoderPrivateData_t             * PrivateData_p;
    VIDCOM_PictureGenericData_t         * PictGenData_p;
    AVS_PictureSpecificData_t           * PictSpecData_p;
    VIDCOM_ParsedPictureInformation_t   * ParsedPicture_p;
    /* VIDBUFF_FrameBuffer_t               * ReferenceFrameBuff_p; */
    VIDBUFF_PictureBuffer_t             * PictureBuffer_p;
    /* void                                * ForwardReferenceLuma_p = NULL; */
    /* void                                * BackwardReferenceLuma_p = NULL; */
    /* MME_Command_t                       * Command; */
    U32                                   i /*,rows,cols*/;
    MME_DataBuffer_t                   ** DataBuffers;
    /* void                                * temp; */
    /* void                                * OutputFramebufferCPUAddress; */
    int                                   PrevTR;
    /* void                                * VirtualAddress; */
    /* void                                * DeviceAddress; */
    /* void                                * CPUAddress; */
    /* void                                * OutputBitBufferCPUAddress; */
    /* S32                                   forwardRefIdx, backwardRefIdx, counter; */
    /* U32                                   even_MB_width, even_MB_height, size; */
    unsigned int                           MaxPicDistanceLsb;

    ParsedPicture_p = &(ConfirmBufferParams_p->NewPictureBuffer_p->ParsedPictureInformation);
    PictureBuffer_p = ConfirmBufferParams_p->NewPictureBuffer_p;

    PictGenData_p   = &(ParsedPicture_p->PictureGenericData);
    PictSpecData_p  = ParsedPicture_p->PictureSpecificData_p;

    DECODER_Data_p  = (DECODER_Properties_t *)      DecoderHandle;

    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    fmw_p = &PrivateData_p->TransformParam;

    /* Modified tr for FW, from FW Demo codes,To be confirmed */
    MaxPicDistanceLsb = (1 << 8);

    /* Calculate the MSBs of current picture */
    if (fmw_p->tr < PrivateData_p->PrevPicDistanceLsb && (PrivateData_p->PrevPicDistanceLsb - fmw_p->tr) >= (int)(MaxPicDistanceLsb / 2))
        PrivateData_p->CurrPicDistanceMsb = PrivateData_p->PicDistanceMsb + MaxPicDistanceLsb;
    else if (fmw_p->tr > PrivateData_p->PrevPicDistanceLsb && (fmw_p->tr - PrivateData_p->PrevPicDistanceLsb) > (int)(MaxPicDistanceLsb / 2))
        PrivateData_p->CurrPicDistanceMsb = PrivateData_p->PicDistanceMsb - MaxPicDistanceLsb;
    else
        PrivateData_p->CurrPicDistanceMsb = PrivateData_p->PicDistanceMsb;

    PrevTR = fmw_p->tr;

    fmw_p->tr = PrivateData_p->CurrPicDistanceMsb + fmw_p->tr;
    /* End */


    /* FrameBuffer_p = address where the decoded picture will be stored */
    if (ConfirmBufferParams_p->NewPictureBuffer_p->FrameBuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER (AVSDECODER_ConfirmBuffer) :  FrameBuff_p = NULL"));
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Convert devices addresses to CPU addresses for the firmware */
    if (PrivateData_p->ForwardReference_p == NULL)
        PrivateData_p->ForwardReference_p = STAVMEM_DeviceToCPU(PictureBuffer_p->FrameBuffer_p->Allocation.Address_p, &PrivateData_p->VirtualMapping);
    if (PrivateData_p->BackwardReference_p == NULL)
        PrivateData_p->BackwardReference_p = STAVMEM_DeviceToCPU(PictureBuffer_p->FrameBuffer_p->Allocation.Address_p, &PrivateData_p->VirtualMapping);

    if (PrivateData_p->Omeg2ForwardLumaReference_p == NULL)
        PrivateData_p->Omeg2ForwardLumaReference_p = STAVMEM_DeviceToCPU(PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p, &PrivateData_p->VirtualMapping);
    if (PrivateData_p->Omeg2ForwardChromaReference_p == NULL)
        PrivateData_p->Omeg2ForwardChromaReference_p = STAVMEM_DeviceToCPU(PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p, &PrivateData_p->VirtualMapping);
    if (PrivateData_p->Omeg2BackwardLumaReference_p == NULL)
        PrivateData_p->Omeg2BackwardLumaReference_p = STAVMEM_DeviceToCPU(PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p,&PrivateData_p->VirtualMapping);
    if (PrivateData_p->Omeg2BackwardChromaReference_p == NULL)
        PrivateData_p->Omeg2BackwardChromaReference_p = STAVMEM_DeviceToCPU(PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p,&PrivateData_p->VirtualMapping);

    PrivateData_p->BitBuffer_p = STAVMEM_DeviceToCPU(PrivateData_p->BitBufferAddress_p,&PrivateData_p->VirtualMapping);

    PrivateData_p->OutputBuffer_p = STAVMEM_DeviceToCPU(PictureBuffer_p->FrameBuffer_p->Allocation.Address_p,&PrivateData_p->VirtualMapping);
    PrivateData_p->Omeg2OutputLumaBuffer_p = STAVMEM_DeviceToCPU(PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p,&PrivateData_p->VirtualMapping);
    PrivateData_p->Omeg2OutputChromaBuffer_p = STAVMEM_DeviceToCPU(PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p,&PrivateData_p->VirtualMapping);
    /* Pointer on the array of input and output buffers    */
    DataBuffers = PrivateData_p->CmdDecoder.DataBuffers_p;

    /* Init DataBuffer structure */
    for(i = 0; i < (NUM_MME_AVS_BUFFERS); i++)
    {
        DataBuffers[i]->StructSize = sizeof (MME_DataBuffer_t);
        DataBuffers[i]->UserData_p = NULL;
        DataBuffers[i]->Flags = 0;
        DataBuffers[i]->StreamNumber = 0;
        DataBuffers[i]->NumberOfScatterPages = 1;
        DataBuffers[i]->StartOffset = 0;
        DataBuffers[i]->TotalSize = 0;
    }

    /* Input compressed MPEG4 stream  */
    /* Input buffer address */
    DataBuffers[0]->ScatterPages_p->Page_p =PrivateData_p->BitBuffer_p;
    DataBuffers[0]->TotalSize = PrivateData_p->BitBufferSize;

    /* Reconstruction buffer (raster)  */
    DataBuffers[1]->ScatterPages_p->Page_p = PrivateData_p->OutputBuffer_p;
    DataBuffers[1]->TotalSize = PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize;
    /*DataBuffers[1]->ScatterPages_p->Size = PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize;*/
    /* Forward reference buffer */
    DataBuffers[2]->ScatterPages_p->Page_p = PrivateData_p->ForwardReference_p;    /* Forward Ref Frame */
    DataBuffers[2]->TotalSize = PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize;
    /*DataBuffers[2]->ScatterPages_p->Size = PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize;*/
    /* Backward reference buffer */
    DataBuffers[3]->ScatterPages_p->Page_p = PrivateData_p->BackwardReference_p;    /* Backward Ref Frame */
    DataBuffers[3]->TotalSize =PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize;
    /*DataBuffers[3]->ScatterPages_p->Size = PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize;*/

    /*OutputFramebufferCPUAddress = STAVMEM_DeviceToCPU(PictureBuffer_p->FrameBuffer_p->Allocation.Address_p,&PrivateData_p->VirtualMapping);*/
    /*rows = ((576 + 31) >> 5) << 1;
    cols = (720 + 15) >> 4;*/

    DataBuffers[4]->ScatterPages_p->Page_p = PrivateData_p->Omeg2OutputLumaBuffer_p;
    DataBuffers[4]->TotalSize = PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.TotalSize - PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;

    DataBuffers[5]->ScatterPages_p->Page_p = PrivateData_p->Omeg2OutputChromaBuffer_p;
    DataBuffers[5]->TotalSize = PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;

    DataBuffers[6]->ScatterPages_p->Page_p = PrivateData_p->Omeg2ForwardLumaReference_p;
    DataBuffers[6]->TotalSize = PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.TotalSize - PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;

    DataBuffers[7]->ScatterPages_p->Page_p = PrivateData_p->Omeg2ForwardChromaReference_p;
    DataBuffers[7]->TotalSize = PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;

    DataBuffers[8]->ScatterPages_p->Page_p = PrivateData_p->Omeg2BackwardLumaReference_p;
    DataBuffers[8]->TotalSize = PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.TotalSize - PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;

    DataBuffers[9]->ScatterPages_p->Page_p = PrivateData_p->Omeg2BackwardChromaReference_p;
    DataBuffers[9]->TotalSize = PictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;

    /* Setup the scatterbuffer pages page */
    for(i = 0; i < (NUM_MME_AVS_BUFFERS); i++)
    {
        /* Only one scatterpage, so size = totalsize  */

        DataBuffers[i]->ScatterPages_p->BytesUsed  = 0;
        DataBuffers[i]->ScatterPages_p->FlagsIn    = 0;
        DataBuffers[i]->ScatterPages_p->FlagsOut   = 0;
        DataBuffers[i]->ScatterPages_p->Size   = DataBuffers[i]->TotalSize;
    }

    DataBuffers[0]->ScatterPages_p->Size = PrivateData_p->BitBufferSize;

    /* Size of the structure */
    PrivateData_p->CmdDecoder.StructSize            = sizeof(MME_Command_t);
    /* Ask to perform the decode */
    PrivateData_p->CmdDecoder.CmdCode               = MME_TRANSFORM;
    /* Send an event when decode complete */
    PrivateData_p->CmdDecoder.CmdEnd                = MME_COMMAND_END_RETURN_NOTIFY;
    /* DueTime to zero */
    PrivateData_p->CmdDecoder.DueTime               = (MME_Time_t)0;
    /* The AVS decoder need one input buffer */
    PrivateData_p->CmdDecoder.NumberInputBuffers    = NUM_MME_INPUT_BUFFERS;
    /* The AVS decoder need 3 output buffers */
    PrivateData_p->CmdDecoder.NumberOutputBuffers   = NUM_MME_OUTPUT_BUFFERS;

    /* Returned data filled by the firmware AVS decoder */
    PrivateData_p->CmdDecoder.CmdStatus.AdditionalInfoSize = sizeof(MME_AVSVideoDecodeReturnParams_t);
    PrivateData_p->CmdDecoder.CmdStatus.AdditionalInfo_p   = (MME_GenericParams_t *)&(PrivateData_p->ReturnParams);

    /* No more parmaters required by the decoder. All is contained in the structure */
    PrivateData_p->CmdDecoder.ParamSize             = sizeof(MME_AVSVideoDecodeParams_t);
    PrivateData_p->CmdDecoder.Param_p               = (MME_GenericParams_t *)fmw_p;

    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime = STOS_time_now();

    /* Decoder is working */
    PrivateData_p->DecoderState = AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC;
    /* Send command to the firmware to start decode */
    TraceMMECall(("MME","SEND"));

    ST_ErrorCode =  MME_SendCommand(PrivateData_p->TranformHandleDecoder,&PrivateData_p->CmdDecoder);

    TraceDecodeDuration(("DUR","START"));
#ifdef TRACE_AVSDECODER_JOBS
    TraceAvsDecoderJobs(("DecoderJobs", "%s", "SEND_FW"));
#endif /*TRACE_AVSDECODER_JOBS*/
    if (PictGenData_p->PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
    {
        PrivateData_p->BackwardReference_p = PrivateData_p->ForwardReference_p ;
        PrivateData_p->ForwardReference_p = PrivateData_p->OutputBuffer_p;

        PrivateData_p->Omeg2BackwardLumaReference_p = PrivateData_p->Omeg2ForwardLumaReference_p;
        PrivateData_p->Omeg2BackwardChromaReference_p = PrivateData_p->Omeg2ForwardChromaReference_p;
        PrivateData_p->Omeg2ForwardLumaReference_p = PrivateData_p->Omeg2OutputLumaBuffer_p;
        PrivateData_p->Omeg2ForwardChromaReference_p = PrivateData_p->Omeg2OutputChromaBuffer_p;

        PrivateData_p->PrevPicDistanceLsb = PrevTR;
        PrivateData_p->PicDistanceMsb = PrivateData_p->CurrPicDistanceMsb;

    }

    TraceDecodedPictureType(("DECTYPE",
                             (PictGenData_p->PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B) ? "B" :
                             (PictGenData_p->PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P) ? "P" :
                             (PictGenData_p->PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I) ? "I" : "?"));

#if AVS_DEBUG_DECODER_TIME
    CptFrame++;
    DecodeStartTime = time_now();
#endif /*AVS_DEBUG_DECODER_TIME*/
#ifdef TRACE_AVSDECODER_STATUS
   TraceAvsDecoderStatus(("DecoderContxtStatus", "%s", (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_NOT_USED)  ? "NOT_USED" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_DECODE_FUNC)             ? "DECODE_FUNC" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC)      ? "CONFIRM_BUFFER" :"INVALID"
                    ));
#endif /*TRACE_AVSDECODER_STATUS*/

/*wait(3000);*/

    return(ST_ErrorCode);

} /* End of AVSDECODER_ConfirmBuffer function */

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
ST_ErrorCode_t AVSDECODER_UpdateBitBufferParams(const DECODER_Handle_t DecoderHandle, void * const BitBufferAddress_p, const U32 BitBufferSize)
{
    DECODER_Properties_t *      DECODER_Data_p;
    AVSDecoderPrivateData_t *   PrivateData_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->BitBufferAddress_p = BitBufferAddress_p;
    PrivateData_p->BitBufferSize = BitBufferSize;
    return(Error);
}   /* End of AVSDECODER_UpdateBitBufferParams */


/******************************************************************************
* \name  AVS_TransformerCallback_decoder
* \brief Initialize the decoder module
*
* \param Event
* \param CallbackData
* \param UserData
* \return ST_ErrorCode
*
******************************************************************************/
void AVS_TransformerCallback_decoder (MME_Event_t       Event,
                                          MME_Command_t   * CallbackData,
                                          void            * UserData)
{
    /* ST_ErrorCode_t                    ST_ErrorCode = ST_NO_ERROR; */
    DECODER_Properties_t            * DECODER_Data_p;
    AVSDecoderPrivateData_t     * PrivateData_p;
#if defined(DEBUG_REPORT_ERRORS_IN_THE_STREAM)
    static U32 ReportErrorsInTheStream = 0;
#endif /* DEBUG_REPORT_ERRORS_IN_THE_STREAM */

    UNUSED_PARAMETER(Event);

#if AVS_DEBUG_DECODER_TIME
    DecodeEndTime = time_now();
#endif

    TraceDecodeDuration(("DUR","END"));

    DECODER_Data_p  = (DECODER_Properties_t *) UserData;
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (CallbackData->CmdStatus.State == MME_COMMAND_COMPLETED)
    {
        PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobState = DECODER_JOB_COMPLETED;
        PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;

#ifdef TRACE_AVSDECODER_JOBS
        TraceAvsDecoderJobs(("DecoderJobs", "%s", "COMPLETED"));
#endif /*TRACE_AVSDECODER_JOBS*/
    }
    else
    {
        /* Send error, and unchange DecodingJobStatus.JobState */
        PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;

#if AVS_DEBUG_DECODER
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Error in decode: Error returned by the transformer in processing the command\n"));
#endif /*AVS_DEBUG_DECODER*/
    }

    if (PrivateData_p->ReturnParams.Errortype==AVS_FIRMWARE_TIME_OUT_ENCOUNTERED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports AVS_DECODER_ERROR_TASK_TIMEOUT\n"));
        PrivateData_p->Statistics.DecodePbFirmwareWatchdogError++;
        PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
    }
    else
        PrivateData_p->DecoderContext.DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;

#if AVS_OUTPUT_PERFORMANCES
    {
        U32 picturetype    = PrivateData_p->ReturnParams.picturetype;
        U32 pm_cycles      = PrivateData_p->ReturnParams.pm_cycles;
        U32 decode_time    = (U32)( ((float)pm_cycles / (double)400000) + 0.5 );
        U32 pm_dmiss       = PrivateData_p->ReturnParams.pm_dmiss;
        U32 pm_imiss       = PrivateData_p->ReturnParams.pm_imiss;
        U32 pm_bundles     = PrivateData_p->ReturnParams.pm_bundles;
        U32 pm_pft         = PrivateData_p->ReturnParams.pm_pft;

        sprintf(VID_Msg, "TYPE:%s ",   (picturetype==0) ? "I" :
                                      ((picturetype==1) ? "P" :
                                      ((picturetype==2) ? "B" :"S" )));
        STTBX_Print((VID_Msg));
        sprintf(VID_Msg, "Time:%d ",decode_time);
        STTBX_Print((VID_Msg));
        sprintf(VID_Msg, "cycles:%d ",pm_cycles);
        STTBX_Print((VID_Msg));
        sprintf(VID_Msg, "dmiss:%d ",pm_dmiss);
        STTBX_Print((VID_Msg));
        sprintf(VID_Msg, "imiss:%d ",pm_imiss);
        STTBX_Print((VID_Msg));
        sprintf(VID_Msg, "dmiss nb:%d ",pm_bundles);
        STTBX_Print((VID_Msg));
        sprintf(VID_Msg, "pft:%d\n ",pm_pft);
        STTBX_Print((VID_Msg));
    }
#endif /*AVS_OUTPUT_PERFORMANCES*/


    /* Inform the controler of the terminated command */
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.CommandId =  PrivateData_p->CommandId;
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobCompletionTime = time_now();
#ifdef STVID_VALID_MEASURE_TIMING
    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.LXDecodeTimeInMicros = (U32)(((float)PrivateData_p->ReturnParams.pm_cycles / (double)400));
#endif /* STVID_VALID_MEASURE_TIMING */

    /* The order of the 3 following actions is mandatory for performance reasons AND to avoid conflicts on DECODER_DecodingJobResults struct */
         /* 1. AVSDECODER_JOB_COMPLETED_EVT_ID */
         /* 2. AVSDECODER_CONTEXT_NOT_USED */
         /* 3. AVSDECODER_INFORM_READ_ADDRESS_EVT_ID */
    /* Performance reason : It is much preferable to have AVSDECODER_JOB_COMPLETED_EVT_ID sent after AVSDECODER_JOB_COMPLETED_EVT_ID */
    /* Conflict reason : DecoderState to bet set to AVSDECODER_CONTEXT_NOT_USED AFTER AVSDECODER_JOB_COMPLETED_EVT_ID is sent */
    STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[AVSDECODER_JOB_COMPLETED_EVT_ID],
                 (const void *) &(PrivateData_p->DecoderContext.DECODER_DecodingJobResults));
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
    PrivateData_p->DecoderState = AVSDECODER_CONTEXT_NOT_USED;  /* The flag must be reset *after* the DECODER_JOB_COMPLETED_EVT notification */
    /* The DECODER_INFORM_READ_ADDRESS_EVT can be notified before or after the reset of PrivateData_p->DecoderContext.State
     * but if we notify it before it will delay the opportunity for the producer to start a new decode as an
     * CONTROLLER_COMMAND_DECODER_READ_ADDRESS_UPDATED command will be pushed and treated before the producer make a try for the next DecodePicture */
    STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[AVSDECODER_INFORM_READ_ADDRESS_EVT_ID],
                 (const void *) &(PrivateData_p->DecoderReadAddress));

#ifdef TRACE_AVSDECODER_STATUS
        TraceAvsDecoderStatus(("DecoderContxtStatus", "%s", (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_NOT_USED)  ? "NOT_USED" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_DECODE_FUNC)             ? "DECODE_FUNC" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC)      ? "CONFIRM_BUFFER" :"INVALID"
                    ));
#endif /*TRACE_AVSDECODER_STATUS*/

}

/******************************************************************************
* \name  AVSDECODER_GetState
* \brief Read the decoder module state
*
* \param DecoderHandle
* \param CommandId
* \param DecoderStatus_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_GetState(const DECODER_Handle_t  DecoderHandle,
                                       const CommandId_t       CommandId,
                                       DECODER_Status_t      * const DecoderStatus_p)
{
    ST_ErrorCode_t                            ST_ErrorCode= ST_NO_ERROR;
    DECODER_Properties_t                    * DECODER_Data_p;
    AVSDecoderPrivateData_t             * PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *)        DecoderHandle;
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

   if (CommandId == PrivateData_p->CommandId)
   {
      *DecoderStatus_p = PrivateData_p->DecoderStatus;
   }
   else
   {
      ST_ErrorCode = ST_ERROR_BAD_PARAMETER;
   }

   return(ST_ErrorCode);
}

/******************************************************************************
* \name  AVSDECODER_GetCodecInfo
* \brief Get codec informations
*
* \param DecoderHandle
* \param CodecInfo_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_GetCodecInfo (const DECODER_Handle_t    DecoderHandle,
                                            DECODER_CodecInfo_t     * const CodecInfo_p,
                                            const U32 ProfileAndLevelIndication,
                                            const STVID_CodecMode_t CodecMode)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(ProfileAndLevelIndication);

    CodecInfo_p->MaximumDecodingLatencyInSystemClockUnit = 90000; /* System Clock = 90kHz */
    if (CodecMode == STVID_CODEC_MODE_DECODE)
    {
        CodecInfo_p->FrameBufferAdditionalDataBytesPerMB     = 0;
    }
    return(ErrorCode);
}

#ifdef STVID_DEBUG_GET_STATISTICS
/******************************************************************************
* \name  AVSDECODER_GetStatistics
* \brief Get statistics on the decoder
*
* \param DecoderHandle
* \param Statistics_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_GetStatistics(const DECODER_Handle_t     DecoderHandle,
                                          STVID_Statistics_t       * const Statistics_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t      *DECODER_Data_p = (DECODER_Properties_t *) DecoderHandle;
    AVSDecoderPrivateData_t  *PrivateData_p  = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

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
    /* Statistics_p->DecodePbFirmwareWatchdogError = PrivateData_p->Statistics.DecodePbFirmwareWatchdogError; */

    return(ErrorCode);

}

/******************************************************************************
* \name  AVSDECODER_ResetStatistics
* \brief Initialize the statistics
*
* \param DecoderHandle
* \param Statistics_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_ResetStatistics(const DECODER_Handle_t       DecoderHandle,
                                            const STVID_Statistics_t   * const Statistics_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t      *DECODER_Data_p = (DECODER_Properties_t *) DecoderHandle;
    AVSDecoderPrivateData_t  *PrivateData_p  = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

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

    /* if (Statistics_p->DecodePbFirmwareWatchdogError != 0) */
    /* { */
        /* PrivateData_p->Statistics.DecodePbFirmwareWatchdogError  = 0; */
    /* } */

    return(ErrorCode);

}

#endif /* STVID_DEBUG_GET_STATISTICS */

/******************************************************************************
* \name  AVSDECODER_Reset
* \brief Initialize the decoder module
*
* \param DecoderHandle
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_Reset(const DECODER_Handle_t DecoderHandle)
{
    ST_ErrorCode_t ST_ErrorCode        = ST_NO_ERROR;
    DECODER_Properties_t                    * DECODER_Data_p;
    AVSDecoderPrivateData_t             * PrivateData_p;

    DECODER_Data_p  = (DECODER_Properties_t *)        DecoderHandle;
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->DecoderState = AVSDECODER_CONTEXT_NOT_USED;
#ifdef TRACE_AVSDECODER_STATUS
    TraceAvsDecoderStatus(("DecoderContxtStatus", "%s", (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_NOT_USED)  ? "NOT_USED" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_DECODE_FUNC)             ? "DECODE_FUNC" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC)      ? "CONFIRM_BUFFER" :"INVALID"
                    ));
#endif /*TRACE_AVSDECODER_STATUS*/
    return(ST_ErrorCode);
}

/******************************************************************************
* \name  AVSDECODER_Abort
* \brief Abort decoder
*
* \param DecoderHandle
* \param CommandId
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_Abort(const DECODER_Handle_t    DecoderHandle,
                                    const CommandId_t         CommandId)
{
    ST_ErrorCode_t                          ST_ErrorCode = ST_NO_ERROR;
    DECODER_Properties_t                    * DECODER_Data_p;
    AVSDecoderPrivateData_t             * PrivateData_p;

    UNUSED_PARAMETER(CommandId);

    DECODER_Data_p  = (DECODER_Properties_t *)        DecoderHandle;
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    PrivateData_p->DecoderState = AVSDECODER_CONTEXT_NOT_USED;
#ifdef TRACE_AVSDECODER_STATUS
     TraceAvsDecoderStatus(("DecoderContxtStatus", "%s", (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_NOT_USED)  ? "NOT_USED" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_DECODE_FUNC)             ? "DECODE_FUNC" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC)      ? "CONFIRM_BUFFER" :"INVALID"
                    ));
#endif
    return(ST_ErrorCode);
}

/******************************************************************************
* \name  AVSDECODER_Setup
* \brief Setup an object in the decoder.
* \AVS decoder doesn't support any buffer partition
* \param DecoderHandle
* \param SetupParams_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_Setup(const DECODER_Handle_t DecoderHandle,
                                    const STVID_SetupParams_t * const SetupParams_p)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(SetupParams_p);
    return (ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/******************************************************************************
* \name  AVSDECODER_FillAdditionnalDataBuffer
* \brief .
* \param DecoderHandle
* \param Picture_p
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_FillAdditionnalDataBuffer(const DECODER_Handle_t DecoderHandle,
                                    const VIDBUFF_PictureBuffer_t * const Picture_p)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(Picture_p);
    return (ST_NO_ERROR);
}

/******************************************************************************
* \name  AVSDECODER_Term
* \brief Terminate the decoder module
*
* \param DecoderHandle
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVSDECODER_Term(const DECODER_Handle_t DecoderHandle)
{
    ST_ErrorCode_t                            ST_ErrorCode= ST_NO_ERROR;
    DECODER_Properties_t                    * DECODER_Data_p;
    AVSDecoderPrivateData_t             * PrivateData_p;
    EMBX_ERROR                                EMBXError;
    U32                                       i;
    MME_ERROR                               MME_Error;
    ST_ErrorCode_t                          MMETermErrorCode;
    U32                                     TermCounter;

    DECODER_Data_p  = (DECODER_Properties_t *)      DecoderHandle;
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    /* Terminate MME API */
    MMETermErrorCode = ST_NO_ERROR;
    TermCounter = 0;
    do
    {
        TraceMMECall(("MME","INIT"));
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

    for(i=0; i < NUM_MME_AVS_BUFFERS; i++)
    {
        EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[i]->ScatterPages_p));
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_Free(DataBuffers_p[%d]->ScatterPages_p) failed ErrorCode=%x", i, EMBXError));
        }
    }

    for(i=0; i< NUM_MME_AVS_BUFFERS; i++)
    {
        EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p[i]));
        if (EMBXError != EMBX_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_Free(DataBuffers_p[%d]) failed ErrorCode=%x", i, EMBXError));
        }
    }

    EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->CmdDecoder.DataBuffers_p));
    if (EMBXError != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"EMBX_Free(DataBuffers_p) failed ErrorCode=%x", EMBXError));
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

    STOS_MemoryDeallocate(DECODER_Data_p->CPUPartition_p, DECODER_Data_p->PrivateData_p);
#ifdef TRACE_AVSDECODER_STATUS
    TraceAvsDecoderStatus(("DecoderContxtStatus", "%s", (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_NOT_USED)  ? "NOT_USED" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_DECODE_FUNC)             ? "DECODE_FUNC" :
                    (PrivateData_p->DecoderState == AVSDECODER_CONTEXT_CONFIRM_BUFFER_FUNC)      ? "CONFIRM_BUFFER" :"INVALID"
                    ));
#endif
    if (ST_ErrorCode == ST_NO_ERROR)
    {
        ST_ErrorCode = MMETermErrorCode;
    }
    return (ST_ErrorCode);
}

/******************************************************************************
* \name  AVS_UpdateTransformer
* \brief Called by the ConfirmBuffer and Init, to initialize the transformer module
*
* \param DecoderHandle
* \return ST_ErrorCode
*
******************************************************************************/
ST_ErrorCode_t AVS_UpdateTransformer(const DECODER_Handle_t DecoderHandle,
                                         U32 FrameWidth, U32 FrameHeight, U32 CodecVersion)
{
   ST_ErrorCode_t        ST_ErrorCode = ST_NO_ERROR;

    DECODER_Properties_t                * DECODER_Data_p;
    AVSDecoderPrivateData_t            * PrivateData_p;
    STAVMEM_FreeBlockParams_t           FreeParams;
    /* U32 address; */
    /* STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping; */
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    /* STAVMEM_FreeBlockParams_t AvmemFreeParams; */


    /* Check if input paramaters are valid */
    if (DecoderHandle == NULL)
        return(ST_ERROR_BAD_PARAMETER);

    /* Set handle pointer */
    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;

    /* Save pointer on private datas */
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;


    if (PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* 1.Set the frame parameters and codec version for the firmware */
    PrivateData_p->AvsDecoderInitParams.width         = (MME_UINT)FrameWidth;
    PrivateData_p->AvsDecoderInitParams.height        = (MME_UINT)FrameHeight;
    PrivateData_p->AvsDecoderInitParams.Progressive_sequence= (MME_UINT)CodecVersion;

    /* PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr = (IntraMBstruct_t*)STOS_MemoryAllocate(PrivateData_p->CPUPartition_p, ((FrameWidth + 15/16) * (FrameHeight + 15 /16) * sizeof(IntraMBstruct_t))); */
    /* SAFE_MEMORY_ALLOCATE(PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr, IntraMBstruct_t *, PrivateData_p->CPUPartition_p, ((FrameWidth + 15/16) * (FrameHeight + 15 /16) * sizeof(IntraMBstruct_t))); */

    if (PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr != NULL)
    {
        FreeParams.PartitionHandle = DECODER_Data_p->AvmemPartitionHandle;
        ST_ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(PrivateData_p->IntraMBStruct_Handle));
        PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr = NULL;
    }
    if (PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr == NULL)
    {
        /* SAFE_MEMORY_ALLOCATE(PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr, IntraMBstruct_t *, PrivateData_p->CPUPartition_p, ((((FrameWidth + 15)/16) * ((FrameHeight + 15) / 16)) * sizeof(IntraMBstruct_t))); */

        PrivateData_p->IntraMBStruct_Handle = STAVMEM_INVALID_BLOCK_HANDLE;
        AvmemAllocParams.Alignment = 128;
        AvmemAllocParams.PartitionHandle = DECODER_Data_p->AvmemPartitionHandle;
        AvmemAllocParams.Size = ((FrameWidth /16) * (FrameHeight / 16) * sizeof(IntraMBstruct_t));

        AvmemAllocParams.NumberOfForbiddenRanges = 0;
        AvmemAllocParams.ForbiddenRangeArray_p = NULL;
        AvmemAllocParams.NumberOfForbiddenBorders = 0;
        AvmemAllocParams.ForbiddenBorderArray_p = NULL;
        AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        ST_ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &(PrivateData_p->IntraMBStruct_Handle));
        if (ST_ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate MPG2DecoderLXParams_t structure !"));
            ST_ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            ST_ErrorCode = STAVMEM_GetBlockAddress(PrivateData_p->IntraMBStruct_Handle, (void **)(&(PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr)));
            /* PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr = STAVMEM_CPUToDevice(PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr ,&(PrivateData_p->VirtualMapping)); */
            PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr = STAVMEM_CPUToDevice(PrivateData_p->AvsDecoderInitParams.IntraMB_struct_ptr ,&(PrivateData_p->VirtualMapping));
        }
    }

    /* 2.This is to initialize the Avs firmware decoder */
    PrivateData_p->InitParamsDecoder.Priority          = MME_PRIORITY_NORMAL;
    PrivateData_p->InitParamsDecoder.StructSize        = sizeof (MME_TransformerInitParams_t);
    PrivateData_p->InitParamsDecoder.Callback          = &AVS_TransformerCallback_decoder;
    PrivateData_p->InitParamsDecoder.CallbackUserData  = (void *) DecoderHandle;

    /* 3.Specific parameters send to the firmware */
    PrivateData_p->InitParamsDecoder.TransformerInitParamsSize = sizeof(MME_AVSVideoDecodeInitParams_t);
    PrivateData_p->InitParamsDecoder.TransformerInitParams_p   = (void *)&PrivateData_p->AvsDecoderInitParams;

    return (ST_ErrorCode);
} /* End of AVS_UpdateTransformer function */
#ifdef ST_speed
/*******************************************************************************
Name        : AVSDECODER_SetDecodingMode
Description : Use to set degraded decode mode. Not yet implemented
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void AVSDECODER_SetDecodingMode(const DECODER_Handle_t DecoderHandle, const U8 Mode)
{
    DECODER_Properties_t *          DECODER_Data_p;
    AVSDecoderPrivateData_t *      PrivateData_p;
    UNUSED_PARAMETER(Mode);

    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (AVSDecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Degraded mode: Feature not available "));

}

#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : AVSDECODER_SetDecodingDirection
Description :
Parameters  :
Assumptions : Not implemented yet.
Limitations :
Returns     :
*******************************************************************************/
void AVSDECODER_SetDecodingDirection(const DECODER_Handle_t DecoderHandle, const DecodingDirection_t Direction)
{
    UNUSED_PARAMETER(DecoderHandle);
    UNUSED_PARAMETER(Direction);
}

#endif /* STVID_TRICKMODE_BACKWARD */
#endif /* ST_speed */

#ifdef VALID_TOOLS
/*******************************************************************************
Name        : AVSDECODER_RegisterTrace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t AVSDECODER_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    /* DUMMY RegisterTrace function, to satisfy conpilation with VALID_TOOLS */

    return(ErrorCode);
}
#endif /* VALID_TOOLS */


