/*!
 ************************************************************************
 * \file task.c
 *
 * \brief decoder task
 *
 * \author
 * Laurent Delaporte \n
 * CMG/STB \n
 * Copyright (C) 2005 STMicroelectronics
 *
 ******************************************************************************
 */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

/* Defines to add debug info and traces */
/* Note : To add traces add STVID_DECODER_TRACE_UART in environment variables */

#ifdef STVID_DEBUG_DECODER
    #define STTBX_REPORT
#endif

/*#define H264TASK_TRACE_VERBOSE*/

#include "stos.h"

#include "sttbx.h"

#if defined(ST_7100) && defined(STVID_VALID_SBAG)
    #include "sbag.h"
#endif

#if defined(TRACE_DECODER)
#define TRACE_UART
#endif /* TRACE_DECODER */

#if defined TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */

#include "h264decode.h"

#ifdef STVID_VALID_SBAG
/*sbag definitions*/
U32     ValidSbag_Counter = 0;
BOOL    ValidSBagIsFirstField = TRUE;
#endif

#ifdef STVID_CHECK_FB_OVERWRITE
extern void * LumaCheck_p;
extern void * ChromaCheck_p;
extern void * LumaCheck2_p;
extern void * ChromaCheck2_p;
#endif /* STVID_CHECK_FB_OVERWRITE */

#ifdef STVID_VALID_MULTICOM_PROFILING
osclock_t IT_Host_Send;
osclock_t IT_Host_Receive;
#endif /* STVID_VALID_MULTICOM_PROFILING */

static void h264_ClearAllPendingControllerCommands(const DECODER_Properties_t *DECODER_Data_p);
static void h264_ResetData(H264DecoderPrivateData_t * PrivateData_p);
static BOOL IsGlobalParamNeededToTransmit(H264PreviousGlobalParam_t * H264PreviousGlobalParam_p, H264CompanionData_t * H264CompanionData_p);


/*******************************************************************************
Name        : h264_ClearAllPendingControllerCommands
Description : Clears commands pending in queue of controller commands
              To be used at SoftReset, so that eventual previous commandsdon't perturb new start
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void h264_ClearAllPendingControllerCommands(const DECODER_Properties_t *DECODER_Data_p)
{
    U32 Command;
    while (PopDecoderCommand(DECODER_Data_p, &Command) == ST_NO_ERROR)
    {
        ; /* Nothing to do, just pop all start code pushed... */
    }
} /* End of h264_ClearAllPendingControllerCommands() function */


/*******************************************************************************
Name        : h264_ResetData
Description :
Parameters  : PrivateData_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void h264_ResetData(H264DecoderPrivateData_t * PrivateData_p)
{
    U32                     LoopCounter;
    H264CompanionData_t     *SaveH264CompanionData_p;   /* Data sent or received by Companion chip (LX) */
    void *                  SaveIntermediateBuffer_p;

    PrivateData_p->NextCommandId = 500;  /* Arbitrary starting value , not set to 0 for debug purpose */
	PrivateData_p->NbBufferUsed    = 0;
    PrivateData_p->RejectedConfirmBufferCallback = 0;
	PrivateData_p->RejectedSendCommand = 0;
	PrivateData_p->DecoderIsRunning = FALSE;

    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
    PrivateData_p->RejectedPreprocStartCommand = 0;
    for (LoopCounter = 0; LoopCounter < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS; LoopCounter++)
    {
        /* CL NOTE: don't reset whole H264CompanionData_p as on going Firmware actions may finish after this reset and context is needed in callback functions */
        SaveH264CompanionData_p  = PrivateData_p->H264DecoderData[LoopCounter].H264CompanionData_p;
        SaveIntermediateBuffer_p = PrivateData_p->H264DecoderData[LoopCounter].IntermediateBuffer_p;
        memset (&(PrivateData_p->H264DecoderData[LoopCounter]), 0, sizeof(PrivateData_p->H264DecoderData[LoopCounter]));
        PrivateData_p->H264DecoderData[LoopCounter].H264CompanionData_p = SaveH264CompanionData_p;
        PrivateData_p->H264DecoderData[LoopCounter].H264CompanionData_p->GlobalParamSent = FALSE;
        PrivateData_p->H264DecoderData[LoopCounter].IntermediateBuffer_p = SaveIntermediateBuffer_p;
    }

    memset (&(PrivateData_p->PreProcContext[0]), 0, sizeof(PrivateData_p->PreProcContext));
    memset (&(PrivateData_p->FmwContext[0]), 0, sizeof(PrivateData_p->FmwContext));
    memset (&(PrivateData_p->CommandsData[0]), 0, sizeof(PrivateData_p->CommandsData));
    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);

    PrivateData_p->H264PreviousGlobalParam.PicParameterSetId = -1;
    PrivateData_p->H264PreviousGlobalParam.SeqParameterSetId = -1;

}

/*******************************************************************************/
/*! \brief Compare two global param data structure and return TRUE if differents
 *
 * \param H264PreviousGlobalParam, H264CurrentGlobalParam_p
 * \return BOOL
 */
/*******************************************************************************/
static BOOL IsGlobalParamNeededToTransmit(H264PreviousGlobalParam_t * H264PreviousGlobalParam_p, H264CompanionData_t * H264CompanionData_p)
{
    /* 1st check the previous seqParameterSetId and PicParameterSetId, if one has changed, send the new ones to the MME */
    if ( (H264PreviousGlobalParam_p->SeqParameterSetId != H264CompanionData_p->SeqParameterSetId) ||
         (H264PreviousGlobalParam_p->PicParameterSetId != H264CompanionData_p->PicParameterSetId) )
    {
        H264PreviousGlobalParam_p->SeqParameterSetId = H264CompanionData_p->SeqParameterSetId;
        H264PreviousGlobalParam_p->PicParameterSetId = H264CompanionData_p->PicParameterSetId;
        return TRUE;
    }

    /* then we use the same IDs, check that their content haven't changed */
    if ( (H264CompanionData_p->PPS_HasChanged) || (H264CompanionData_p->SPS_HasChanged) )
    {
        return TRUE;
    }

    /* else it's OK not to resend the SPS/PPS */
    return FALSE;

}

#if !defined PERF1_TASK2
/*******************************************************************************/
/*! \brief Start the decoder task
 *
 * \param decoderHandle
 * \return ErrorCode ST_NO_ERROR if success,
 *                   ST_ERROR_ALREADY_INITIALIZED if task already running
 *                   ST_ERROR_BAD_PARAMETER if problem of creation
 */
/*******************************************************************************/
ST_ErrorCode_t h264dec_StartdecoderTask(const DECODER_Handle_t decoderHandle)
{
    /* dereference decoderHandle to a local variable to ease debug */
    H264DecoderPrivateData_t * decoder_Data_p = ((H264DecoderPrivateData_t *)((DECODER_Properties_t *) decoderHandle)->PrivateData_p);
    VIDCOM_Task_t * const Task_p = &(decoder_Data_p->DecoderTask);
    char TaskName[25] = "STVID[?].H264DecoderTask";

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((DECODER_Properties_t *) decoderHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

    /* TODO: define decoder own priority in vid_com.h */
    /* TODO : change task priority ??? */
    if (STOS_TaskCreate((void (*) (void*)) h264dec_DecoderTaskFunc,
                              (void *) decoderHandle,
                              ((DECODER_Properties_t *) decoderHandle)->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(Task_p->TaskStack),
                              ((DECODER_Properties_t *) decoderHandle)->CPUPartition_p,
                              &(Task_p->Task_p),
                              &(Task_p->TaskDesc),
                              STVID_TASK_PRIORITY_DECODE,
                              TaskName,
                              task_flags_no_min_stack_size) != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Task_p->IsRunning  = TRUE;
#ifdef H264TASK_TRACE_VERBOSE
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"decoder task created."));
#endif
    return(ST_NO_ERROR);
} /* End of StartdecoderTask() function */

/*******************************************************************************/
/*! \brief Stop the decoder task
 *
 * \param decoderHandle
 * \return ErrorCode ST_NO_ERROR if no problem, ST_ERROR_ALREADY_INITIALIZED if task not running
 */
/*******************************************************************************/
ST_ErrorCode_t h264dec_StopDecoderTask(const DECODER_Handle_t DecoderHandle)
{
    /* dereference decoderHandle to a local variable to ease debug */
    H264DecoderPrivateData_t * decoder_Data_p = ((H264DecoderPrivateData_t *)((DECODER_Properties_t *) DecoderHandle)->PrivateData_p);
    VIDCOM_Task_t * const Task_p = &(decoder_Data_p->DecoderTask);
    task_t * TaskList_p = Task_p->Task_p;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* Signal semaphore to release task that may wait infinitely if stopped */
    STOS_SemaphoreSignal(decoder_Data_p->DecoderOrder_p);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                      ((DECODER_Properties_t *) DecoderHandle)->CPUPartition_p,
                      &Task_p->TaskStack,
                      ((DECODER_Properties_t *) DecoderHandle)->CPUPartition_p);

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopdecoderTask() function */
#endif /* if !defined PERF1_TASK2 */
/*******************************************************************************/
/*! \brief Decoder main loop
 *
 * \param DecoderHandle
 *
 *
 *                       State            DecoderOrder
 *   Current
 *
 *   State order :
 *       1 : DECODER_CONTEXT_CONFIRM_BUFFER_EVT
 *       2 : DECODER_CONTEXT_START_FUNC
 *       --> Case (1)
 *
 *
 *   Start X
 *               Start X+1
 *               ConfirmBuffer X+1  --> Case (2)
 *   ConfirmBuffer X --> Case (3)
 *
 */
/*******************************************************************************/
#if !defined PERF1_TASK2
void h264dec_DecoderTaskFunc(const DECODER_Handle_t DecoderHandle)
#else
void h264dec_DecoderMainFunc(const DECODER_Handle_t DecoderHandle)
#endif
{
    /* dereference DecoderHandle to a local variable to ease debug */
    DECODER_Properties_t           *DECODER_Data_p;
    H264DecoderPrivateData_t *      PrivateData_p;
    U32                             Command;
    U32                             State;
    U32                             iBuff;
    U32                             i;
    U32                             iPP;
    U32                             iFmw;
    S32                             iCmd;
    U32                             MinDOFID;
#ifdef ST_speed
    U32                             MaxDOFID;
#endif
    U32                             CurDOFID;
    BOOL                            IsDecOrderRespected;
    BOOL                            IsCommandIdPP;
    BOOL                            IsCommandIdFmw;
    BOOL                            IsCommandIdPar;
    MME_ERROR                       MME_Error;
    ST_ErrorCode_t                  ST_Error;
    H264CompanionData_t            *H264CompanionData_p;
#ifdef STVID_INT_BUFFER_DUMP
    FILE                           *IntBuffer_fp;
    char                            IntBufferFileName[256];
    U8                             *IntBufferAddress;
    U32                             IntBufferSize;
    H264_PictureDescriptorType_t    DescriptorType;
    char                            PictureTypeStr[3];
#endif /* STVID_INT_BUFFER_DUMP */
#ifdef STVID_CHECK_FB_OVERWRITE
    static U32                      CountBUG = 0;
#endif /* STVID_CHECK_FB_OVERWRITE */

#if !defined PERF1_TASK2
    STOS_TaskEnter(NULL);
#endif


    DECODER_Data_p  = (DECODER_Properties_t *) DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

#if !defined PERF1_TASK2
    /* Big loop executed until ToBeDeleted is set --------------------------- */
    do
    {
        STOS_SemaphoreWait(PrivateData_p->DecoderOrder_p);

        if (!PrivateData_p->DecoderTask.ToBeDeleted)
        {
#endif
    /* Re-push commands for CONFIRM_BUFFER_EVTs that have been previously rejected because they were in wrong order */
    iPP = 0;
    while((PrivateData_p->RejectedConfirmBufferCallback != 0) && (iPP < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS))
    {
        if(PrivateData_p->PreProcContext[iPP].HasBeenRejected)
        {
            Command = DECODER_COMMAND_CONFIRM_BUFFER_EVT;
            Command += (U32)(iPP) << 8;
            /* TraceBuffer(("Pr0%d03 ", iPP)); */
            PushDecoderCommand(DECODER_Data_p, Command);
            STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
            PrivateData_p->PreProcContext[iPP].HasBeenRejected = FALSE;
            PrivateData_p->RejectedConfirmBufferCallback--;
            STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dDECRetryCB-c%dp%d ",
                        iPP, PrivateData_p->PreProcContext[iPP].CommandId));
#endif
        }
        iPP++;
    }
    /* Re-push commands for DECODER_COMMAND_CONFIRM_BUFFER_FUNCs that have been previously rejected because
    * the Previous decode is not terminated : known limitation due to the LX context switch that could provokes
    * an Instruction Cache flush when it executes the interrupt callback */
    if (PrivateData_p->RejectedSendCommand > 0)
    {
        iBuff = 0;
        MinDOFID = 0xFFFFFFFF;
#ifdef ST_speed
        MaxDOFID = 0;
#endif
        iCmd = -1;
        while(iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS)
        {
            if(PrivateData_p->H264DecoderData[iBuff].ConfirmBufferHasBeenRejected)
            {
                CurDOFID = PrivateData_p->H264DecoderData[iBuff].DecodingOrderFrameID;
#ifdef STVID_TRICKMODE_BACKWARD
                if( PrivateData_p->DecodingDirection == DECODING_FORWARD)
#endif
                {
                    /* TODO : field management */
                    if (CurDOFID < MinDOFID)
                    {
                        MinDOFID = CurDOFID;
                        iCmd = iBuff;
                    }
                    else if (CurDOFID == MinDOFID) /* Case of Field picture */
                    {
                        if (PrivateData_p->H264DecoderData[iBuff].IsFirstOfTwoFields)
                        {
                            MinDOFID = CurDOFID;
                            iCmd = iBuff;
                        }
                    }
                }
#ifdef STVID_TRICKMODE_BACKWARD
                else /* DECODING_BACKWARD */
                {
                    /* TODO : field management */
                    if (CurDOFID > MaxDOFID)
                    {
                        MaxDOFID = CurDOFID;
                        iCmd = iBuff;
                    }
                    else if (CurDOFID == MaxDOFID) /* Case of Field picture */
                    {
                        if (PrivateData_p->H264DecoderData[iBuff].IsFirstOfTwoFields)
                        {
                            MaxDOFID = CurDOFID;
                            iCmd = iBuff;
                        }
                    }
                }
#endif
            }
            iBuff++;
        }

        if (iCmd >= 0)
        {
            Command = DECODER_COMMAND_CONFIRM_BUFFER_FUNC;
            Command += (U32)(iCmd) << 8;
    /*                    TraceBuffer(("Cr0%d02 ", iCmd)); */
            PushDecoderCommand(DECODER_Data_p, Command);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dDECRetrySC(%d)-c%dp%d ",
                                        PrivateData_p->RejectedSendCommand, iCmd, PrivateData_p->H264DecoderData[iCmd].H264PreprocCommand.CmdId));
#endif
        }
    }

    /* Re-push Rejected DECODER_COMMAND_START command */
    iBuff = 0;
    while((PrivateData_p->RejectedPreprocStartCommand != 0) && (iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS))
    {
        if(PrivateData_p->H264DecoderData[iBuff].PreprocStartHasBeenRejected)
        {
            Command = DECODER_COMMAND_START;
            Command += (U32)(iBuff) << 8;
            /* TraceBuffer(("Ps0%d01 ", iBuff)); */
            PushDecoderCommand(DECODER_Data_p, Command);
            STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
            PrivateData_p->H264DecoderData[iBuff].PreprocStartHasBeenRejected = FALSE;
            PrivateData_p->RejectedPreprocStartCommand--;
            STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_START, "dDECRetryStart%d-b%d ",
                        PrivateData_p->H264DecoderData[iBuff].CommandIdForProd, iBuff));
#endif
        }
        iBuff++;
    }


    /* Get Controller commands first */
    while (PopDecoderCommand(DECODER_Data_p, &Command) == ST_NO_ERROR)
    {
        /* TraceBuffer(("O%04x ", Command));*/
        State = Command & 0x00FF;
        iBuff = (Command & 0xFF00) >> 8;
        switch (State)
        {
            case DECODER_COMMAND_RESET :
                h264_ResetData(PrivateData_p);
                h264_ClearAllPendingControllerCommands(DECODER_Data_p);
                break;

            case DECODER_COMMAND_START :
                PrivateData_p->H264DecoderData[iBuff].PreprocJobSubmissionTime = time_now();
#ifdef STVID_VALID_MEASURE_TIMING
//                        STSYS_WriteRegDev32LE((void*)0xB8025004, 4); /* Set PIO 5-2 */
#endif
                ST_Error = H264Preproc_ProcessCommand(PrivateData_p->TranformHandlePreProc, &PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand);
#ifdef H264TASK_TRACE_VERBOSE
                STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (1) iBuff %d Id %d ", iBuff, PrivateData_p->H264DecoderData[iBuff].CommandIdForProd));
#endif
                if(ST_Error != ST_NO_ERROR)
                {
                    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                    PrivateData_p->H264DecoderData[iBuff].PreprocStartHasBeenRejected = TRUE;
                    PrivateData_p->RejectedPreprocStartCommand++;
                    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_START, "dDECRejectStart%d-b%d ",
                                                PrivateData_p->H264DecoderData[iBuff].CommandIdForProd, iBuff));
#endif
                }
                else
                {
                    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                    PrivateData_p->H264DecoderData[iBuff].State = DECODER_CONTEXT_PREPROCESSING;
                    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_START, "dDECStartPP%d-b%d ",PrivateData_p->H264DecoderData[iBuff].CommandIdForProd, iBuff));
#endif
                }
                break;

            case DECODER_COMMAND_CONFIRM_BUFFER_EVT :
                /* check that we have already received DECODER_CONTEXT_PREPROCESSING --> we can send the event */
                /* else --> case (1) */
                iPP = iBuff;
#ifdef H264TASK_TRACE_VERBOSE
                STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (2) iPP %d Id %d ", iPP, PrivateData_p->PreProcContext[iPP].CommandId));
#endif
                IsCommandIdPP = FALSE;
                iBuff = 0;
                do
                {
                    IsCommandIdPP = (PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.CmdId == PrivateData_p->PreProcContext[iPP].CommandId &&
                                    PrivateData_p->PreProcContext[iPP].BufferUsed);
                    if (!IsCommandIdPP)
                    {
                        iBuff++;
                    }
                }
                while ((iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS) && !IsCommandIdPP);

                if (IsCommandIdPP && (PrivateData_p->H264DecoderData[iBuff].State == DECODER_CONTEXT_PREPROCESSING))
                {
#ifdef H264TASK_TRACE_VERBOSE
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (2) iBuff %d IdPP %d", iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif
                    /*  check that ConfirmBuffer order is respected : case (2) */
                    IsDecOrderRespected = TRUE;
                    for (i = 0; i < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS && IsDecOrderRespected; i++)
                    {
                        if ((i != iBuff) && (PrivateData_p->H264DecoderData[i].State == DECODER_CONTEXT_PREPROCESSING))
                        {
                            /* TODO : manage CommandId when looping */
                            if (PrivateData_p->H264DecoderData[iBuff].CommandIdForProd > PrivateData_p->H264DecoderData[i].CommandIdForProd)
                            {
                                /* case (2) */
                                IsDecOrderRespected = FALSE;
                            }
                        }
                    }
                    if (IsDecOrderRespected)
                    {
                        H264CompanionData_p = PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p;
                        H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer = PrivateData_p->PreProcContext[iPP].H264_CommandStatus_preproc.IntSESBBegin_p;
                        H264CompanionData_p->h264TransformerFmwParam.PictureStopAddrBinBuffer  = PrivateData_p->PreProcContext[iPP].H264_CommandStatus_preproc.IntPictBufferEnd_p;
                        H264CompanionData_p->h264TransformerFmwParam.MaxSESBSize                 = MAX_SESB_SIZE;

#ifdef STVID_INT_BUFFER_DUMP
                        DescriptorType = H264CompanionData_p->h264TransformerFmwParam.HostData.DescriptorType;
                        PictureTypeStr[2] = 0;
                        if(PrivateData_p->H264DecoderData[iBuff].IsFirstOfTwoFields)
                        {
                            PictureTypeStr[0] = '0';
                        }
                        else
                        {
                            PictureTypeStr[0] = '1';
                        }
                        if(DescriptorType == H264_PIC_DESCRIPTOR_FIELD_TOP)
                        {
                            PictureTypeStr[1] = 'T';
                        }
                        else if(DescriptorType == H264_PIC_DESCRIPTOR_FIELD_BOTTOM)
                        {
                            PictureTypeStr[1] = 'B';
                        }
                        else
                        {
                            PictureTypeStr[0] = 'F';
                            PictureTypeStr[1] = 0;
                        }
                        /* Save Slice error buffer */
                        sprintf(IntBufferFileName, "../../int_buffers/ses_buffer%03d_%03d_%03d_%03d_%s.bin",
                                PrivateData_p->H264DecoderData[iBuff].DecodingOrderFrameID,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.IDExtension,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.ID,
                                PrivateData_p->H264DecoderData[iBuff].PictureNumber,
                                PictureTypeStr);
                        IntBuffer_fp = fopen(IntBufferFileName, "wb");
                        if(IntBuffer_fp != NULL)
                        {
                            IntBufferAddress = STAVMEM_DeviceToCPU(H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer, &PrivateData_p->VirtualMapping);
                            IntBufferSize = DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE;
                            fwrite(IntBufferAddress, 4, (IntBufferSize+3)/4, IntBuffer_fp);
                        }
                        fclose(IntBuffer_fp);
                        /* Save intermediate buffer */
                        sprintf(IntBufferFileName, "../../int_buffers/int_buffer%03d_%03d_%03d_%03d_%s.bin",
                                PrivateData_p->H264DecoderData[iBuff].DecodingOrderFrameID,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.IDExtension,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.ID,
                                PrivateData_p->H264DecoderData[iBuff].PictureNumber,
                                PictureTypeStr);
                        IntBuffer_fp = fopen(IntBufferFileName, "wb");
                        if(IntBuffer_fp != NULL)
                        {
                            IntBufferAddress = STAVMEM_DeviceToCPU((U8 *)H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer + DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE, &PrivateData_p->VirtualMapping);
                            IntBufferSize = (U32)PrivateData_p->PreProcContext[iPP].H264_CommandStatus_preproc.IntPictBufferEnd_p - (U32)H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer - DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE;
                            fwrite(IntBufferAddress, 4, (IntBufferSize+3)/4, IntBuffer_fp);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                            if (IntBufferSize > DECODER_INTERMEDIATE_BUFFER_SIZE || PrivateData_p->PreProcContext[iPP].H264_CommandStatus_preproc.IntPictBufferEnd_p == 0)
                            {
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_PP_CALLBACK, "dBUG_BUG_buff%d/IPBE=%d/PSABB=%d ", iBuff, PrivateData_p->PreProcContext[iPP].H264_CommandStatus_preproc.IntPictBufferEnd_p, H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer));
                            }
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                        }
                        fclose(IntBuffer_fp);
#endif /* STVID_INT_BUFFER_DUMP */

                        STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                        PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.CommandId = PrivateData_p->H264DecoderData[iBuff].CommandIdForProd;
                        STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
                        PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_NO_ERROR;
                        if ((PrivateData_p->PreProcContext[iPP].ErrorCode != H264_PREPROCESSOR_BASE) && (PrivateData_p->PreProcContext[iPP].ErrorCode))
                        {
                            if((PrivateData_p->PreProcContext[iPP].ErrorCode & H264_PREPROCESSOR_ERROR_WRITE_ERROR) == H264_PREPROCESSOR_ERROR_WRITE_ERROR)
                            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dBUG_BUG_BUG_H264_PREPROCESSOR_ERROR_WRITE_ERROR-c%dp%d ",
                                                                iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports H264_PREPROCESSOR_ERROR_WRITE_ERROR\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                            }
                            if((PrivateData_p->PreProcContext[iPP].ErrorCode & H264_PREPROCESSOR_ERROR_READ_ERROR) == H264_PREPROCESSOR_ERROR_READ_ERROR)
                            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dBUG_BUG_BUG_H264_PREPROCESSOR_ERROR_READ_ERROR-c%dp%d ",
                                                                iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports H264_PREPROCESSOR_ERROR_READ_ERROR\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                            }
                            if((PrivateData_p->PreProcContext[iPP].ErrorCode & H264_PREPROCESSOR_ERROR_BIT_BUFFER_OVERFLOW) == H264_PREPROCESSOR_ERROR_BIT_BUFFER_OVERFLOW)
                            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dBUG_BUG_BUG_H264_PREPROCESSOR_ERROR_BIT_BUFFER_OVERFLOW-c%dp%d ",
                                                                iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
#if !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200)
                                /* On deltaphi and deltamu blue, this message is not relevant, as the preprocessor reports even if no error */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports H264_PREPROCESSOR_ERROR_BIT_BUFFER_OVERFLOW\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
#endif /* !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200) */
                            }
                            if((PrivateData_p->PreProcContext[iPP].ErrorCode & H264_PREPROCESSOR_ERROR_BIT_BUFFER_UNDERFLOW) == H264_PREPROCESSOR_ERROR_BIT_BUFFER_UNDERFLOW)
                            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dBUG_BUG_BUG_H264_PREPROCESSOR_ERROR_BIT_BUFFER_UNDERFLOW-c%dp%d ",
                                                                iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports H264_PREPROCESSOR_ERROR_BIT_BUFFER_UNDERFLOW\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                            }
                            if((PrivateData_p->PreProcContext[iPP].ErrorCode & H264_PREPROCESSOR_ERROR_INT_BUFFER_OVERFLOW) == H264_PREPROCESSOR_ERROR_INT_BUFFER_OVERFLOW)
                            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dBUG_BUG_BUG_H264_PREPROCESSOR_ERROR_INT_BUFFER_OVERFLOW-c%dp%d ",
                                                                iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports H264_PREPROCESSOR_ERROR_INT_BUFFER_OVERFLOW\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                            }
                            if((PrivateData_p->PreProcContext[iPP].ErrorCode & H264_PREPROCESSOR_ERROR_ERR_BIT_INSERTED) == H264_PREPROCESSOR_ERROR_ERR_BIT_INSERTED)
                            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dBUG_BUG_BUG_H264_PREPROCESSOR_ERROR_ERR_BIT_INSERTED-c%dp%d ",
                                                                iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports H264_PREPROCESSOR_ERROR_ERR_BIT_INSERTED\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                            }
                            if((PrivateData_p->PreProcContext[iPP].ErrorCode & H264_PREPROCESSOR_ERROR_ERR_SC_DETECTED) == H264_PREPROCESSOR_ERROR_ERR_SC_DETECTED)
                            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dBUG_BUG_BUG_H264_PREPROCESSOR_ERROR_ERR_SC_DETECTED-c%dp%d ",
                                                                iBuff, PrivateData_p->PreProcContext[iPP].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports H264_PREPROCESSOR_ERROR_ERR_SC_DETECTED\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                            }
                            if(((PrivateData_p->PreProcContext[iPP].ErrorCode) & ~(H264_PREPROCESSOR_ERROR_ERR_SC_DETECTED        |
                                                    	                           H264_PREPROCESSOR_ERROR_ERR_BIT_INSERTED       |
                                                    	                           H264_PREPROCESSOR_ERROR_INT_BUFFER_OVERFLOW    |
                                                    	                           H264_PREPROCESSOR_ERROR_BIT_BUFFER_UNDERFLOW   |
                                                    	                           H264_PREPROCESSOR_ERROR_BIT_BUFFER_OVERFLOW    |
                                                    	                           H264_PREPROCESSOR_ERROR_READ_ERROR             |
                                                    	                           H264_PREPROCESSOR_ERROR_WRITE_ERROR)) != 0)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_CONFIRM_BUFFER_EVT): Preprocessor reports UNDEFINED ERROR  0x%08x  BUG! BUG! BUG!\n",
                                                                        (PrivateData_p->PreProcContext[iPP].ErrorCode & ~H264_PREPROCESSOR_BASE)));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                                break;
                            }
                        } /* if ((PrivateData_p->PreProcContext[iPP].ErrorCode != H264_PREPROCESSOR_BASE) && (PrivateData_p->PreProcContext[iPP].ErrorCode)) */
                        STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                        PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.DecodingJobStatus.JobState = DECODER_JOB_WAITING_CONFIRM_BUFFER;  /* TODO : TRUE in TransformerCallback_decoder callback ??*/
                        PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.DecodingJobStatus.JobSubmissionTime = PrivateData_p->H264DecoderData[iBuff].PreprocJobSubmissionTime;
                        PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.DecodingJobStatus.JobCompletionTime = PrivateData_p->PreProcContext[iPP].CallBackTime;
#ifdef STVID_VALID_MEASURE_TIMING
//                                STSYS_WriteRegDev32LE((void*)0xB8025008, 4); /* Clr PIO 5-2 */
#endif
                        PrivateData_p->H264DecoderData[iBuff].State = DECODER_CONTEXT_CONFIRM_BUFFER_EVT;
                        PrivateData_p->PreProcContext[iPP].HasBeenRejected = FALSE;
                        PrivateData_p->PreProcContext[iPP].BufferUsed = FALSE;
                        STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[DECODER_CONFIRM_BUFFER_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults));
                        STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[DECODER_INFORM_READ_ADDRESS_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PrivateData_p->H264DecoderData[iBuff].DecoderReadAddress));
                        STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
                    }
                    else
                    {
                        /* Rejected because preproc callback of earlier picture not yet */
                        /* received                                                     */
                        /*  retry later                                                 */
                        STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                        PrivateData_p->RejectedConfirmBufferCallback++;
                        PrivateData_p->PreProcContext[iPP].HasBeenRejected = TRUE;
                        STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
#ifdef H264TASK_TRACE_VERBOSE
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (2) Decoder Id %d dec order not respected", PrivateData_p->PreProcContext[iPP].CommandId));
#endif
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dDECRejectOrderCB-c%dp%d ",
                                                     iPP, PrivateData_p->PreProcContext[iPP].CommandId));
#endif
                    }
                }
                else
                {
                    /* Rejected because CommandId not found in current picture list */
                    /* This is due to callback received before end of processing of */
                    /* DecodePicture command.                                       */
                    /*  retry later                                                 */
                    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                    PrivateData_p->RejectedConfirmBufferCallback++;
                    PrivateData_p->PreProcContext[iPP].HasBeenRejected = TRUE;
                    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dDECRejectTooEarlyCB-c%dp%d ",
                                iPP, PrivateData_p->PreProcContext[iPP].CommandId));
#endif
                }
                break;

            case DECODER_COMMAND_CONFIRM_BUFFER_FUNC :

                if (!PrivateData_p->DecoderIsRunning &&
                    ((PrivateData_p->RejectedSendCommand == 0) ||
                    (PrivateData_p->H264DecoderData[iBuff].ConfirmBufferHasBeenRejected)))
                {
                    PrivateData_p->DecoderIsRunning = TRUE;
                    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                    if(PrivateData_p->H264DecoderData[iBuff].ConfirmBufferHasBeenRejected)
                    {
                        PrivateData_p->H264DecoderData[iBuff].ConfirmBufferHasBeenRejected = FALSE;
                        PrivateData_p->RejectedSendCommand--;
                    }
                    PrivateData_p->H264DecoderData[iBuff].State = DECODER_CONTEXT_DECODING;
                    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);

                    H264CompanionData_p = PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p;

#ifdef H264TASK_TRACE_VERBOSE
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (3) iBuff %d    Id Fmw %d Id Param %d", iBuff,
                                PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoFmw.CmdStatus.CmdId, PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId));
#endif
#ifdef STUB_FMW_ST40
                    if (IsGlobalParamNeededToTransmit(&(PrivateData_p->H264PreviousGlobalParam), H264CompanionData_p))
                    {
                    MME_Error =  MMESTUB_SendCommand(PrivateData_p->TranformHandleDecoder, MME_SET_GLOBAL_TRANSFORM_PARAMS, &H264CompanionData_p->CmdInfoParam);
                    }
#else /* not STUB_FMW_ST40 */
#if defined(STVID_VALID)
                    if (PrivateData_p->NbMMECmdDumped < MME_MAX_DUMP)
                    {
                        PrivateData_p->H264CompanionDataDump[PrivateData_p->NbMMECmdDumped] = *H264CompanionData_p;
                        PrivateData_p->NbMMECmdDumped++;
                    }
#endif /* defined(STVID_VALID) */
#if defined(MME_DUMP) && defined(VALID_TOOLS)
                    HDM_DumpCommand(MME_SET_GLOBAL_TRANSFORM_PARAMS, &H264CompanionData_p->CmdInfoParam);
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */
                    if (IsGlobalParamNeededToTransmit(&(PrivateData_p->H264PreviousGlobalParam), H264CompanionData_p))
                    {
                        MME_Error =  MME_SendCommand(PrivateData_p->TranformHandleDecoder, &H264CompanionData_p->CmdInfoParam);
                        PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->GlobalParamSent = TRUE;
                    }
                    else
                    {
                        PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId = 0;
                        PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->GlobalParamSent = FALSE;
                    }
#endif /* STUB_FMW_ST40 */
                    PrivateData_p->H264DecoderData[iBuff].DecodeJobSubmissionTime = time_now();
#ifdef STUB_FMW_ST40
                    MME_Error =  MMESTUB_SendCommand(PrivateData_p->TranformHandleDecoder, MME_TRANSFORM, &H264CompanionData_p->CmdInfoFmw);
#else
#if defined(MME_DUMP) && defined(VALID_TOOLS)
                    HDM_DumpCommand(MME_TRANSFORM, &H264CompanionData_p->CmdInfoFmw);
#endif /*  defined(MME_DUMP) && defined(VALID_TOOLS) */

#ifdef STVID_VALID_MEASURE_TIMING
    /* STSYS_WriteRegDev32LE((void*)0xB8025004, 4); *//* Set PIO 5-2 */
#endif

#ifdef STVID_VALID_SBAG
                    /*sbag triggers definitions*/
                    ValidSbag_Counter+=1;
                    if (PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->h264TransformerFmwParam.HostData.IdrFlag)
                    {
                        STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, (SBAG_H264_DECODE_I_START + (ValidSbag_Counter & 0x3fff))); /* SBAG trigger */
                    }
                    else if (PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->h264TransformerFmwParam.HostData.ReferenceType != H264_UNUSED_FOR_REFERENCE)
                    {
                        STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, (SBAG_H264_DECODE_P_START + (ValidSbag_Counter & 0x3fff))); /* SBAG trigger */
                    }
                    else
                    {
                        STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, (SBAG_H264_DECODE_B_START + (ValidSbag_Counter & 0x3fff))); /* SBAG trigger */
                    }
                    if ((PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->h264TransformerFmwParam.HostData.DescriptorType == H264_PIC_DESCRIPTOR_FRAME) ||
                    (PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->h264TransformerFmwParam.HostData.DescriptorType == H264_PIC_DESCRIPTOR_AFRAME)
                    )
                    {
                        STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, SBAG_H264_DECODE_FRAME); /* SBAG trigger */
                    }
                    else
                    {
                        if (ValidSBagIsFirstField)
                        {
                            STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, SBAG_H264_DECODE_1ST_FIELD); /* SBAG trigger */
                            ValidSBagIsFirstField = FALSE;
                        }
                        else
                        {
                            STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, SBAG_H264_DECODE_2ND_FIELD); /*  trigger */
                            ValidSBagIsFirstField = TRUE;
                        }
                    }
#endif /*end of STVID_VALID_SBAG*/
                    MME_Error =     MME_SendCommand(PrivateData_p->TranformHandleDecoder, &H264CompanionData_p->CmdInfoFmw);
#ifdef STVID_VALID_MEASURE_TIMING
/*
        interrupt_unlock();
        task_unlock();
*/
#endif
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    {
                        int LoopRef;
                        char RefString[(16*((10*3)+4)) + 1]; /* 10 char by address, 3 address per ref, 4 separator per ref, 16 refs, 1 ending Zero */
                        H264_ReferenceFrameAddress_t *ReferenceFrameAddress_p;

                        ReferenceFrameAddress_p = &(H264CompanionData_p->h264TransformerFmwParam.InitialRefPictList.ReferenceFrameAddress[0]);
                        RefString[0] = 0;
                        for(LoopRef=0 ; LoopRef<16 ; LoopRef++)
                        {
                            sprintf(RefString, "%s(%08x/%08x/%08x)",
                                    RefString,
                                    ReferenceFrameAddress_p[LoopRef].DecodedBufferAddress.Luma_p,
                                    ReferenceFrameAddress_p[LoopRef].DecodedBufferAddress.Chroma_p,
                                    ReferenceFrameAddress_p[LoopRef].DecodedBufferAddress.MBStruct_p);
                        }
                        TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FB_ADDRESSES, "dADDR%d-(%08x/%08x/%08x)Refs%s ",
                                    PrivateData_p->H264DecoderData[iBuff].CommandIdForProd,
                                    H264CompanionData_p->h264TransformerFmwParam.DecodedBufferAddress.Luma_p,
                                    H264CompanionData_p->h264TransformerFmwParam.DecodedBufferAddress.Chroma_p,
                                    H264CompanionData_p->h264TransformerFmwParam.DecodedBufferAddress.MBStruct_p,
                                    RefString));
                    }
#endif

#ifdef STVID_CHECK_FB_OVERWRITE
                    if ((H264CompanionData_p->h264TransformerFmwParam.DecodedBufferAddress.Luma_p == LumaCheck2_p) ||
                        (H264CompanionData_p->h264TransformerFmwParam.DecodedBufferAddress.Chroma_p == ChromaCheck2_p))
                    {
                        CountBUG++;
                        TraceBuffer(("!!BUG!! No %d (IdParam=%d)@%08x/%08x \r\n",CountBUG, PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId, LumaCheck2_p,ChromaCheck2_p));
/*                                printf(" BUG !@%08x'%08x ",LumaCheck2_p,ChromaCheck2_p); */
#if 0
                        while(TRUE) /* For debug, too allow trace output before breakpoint */
                        {
                            STOS_TaskDelayUs(2000000);  /* 2s */
                            MME_Error = 0;
                        }
#endif
                    }
#endif /* STVID_CHECK_FB_OVERWRITE */
#endif /* STUB_FMW_ST40 */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_CONFIRM_BUFFER, "dDEC_CB%d-b%dp%ds%df%d ",
                                    PrivateData_p->H264DecoderData[iBuff].CommandIdForProd,
                                    iBuff, PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.CmdId,
                                    PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId,PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoFmw.CmdStatus.CmdId));
#endif
                }
                else
                {
                    /* Reject ConfirmBuffer because decoder is busy or  */
                    /* already rejected pictures have to be processed   */
                    /* before this one */
                    if(!PrivateData_p->H264DecoderData[iBuff].ConfirmBufferHasBeenRejected)
                    {
                        STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                        PrivateData_p->RejectedSendCommand++;
                        PrivateData_p->H264DecoderData[iBuff].ConfirmBufferHasBeenRejected = TRUE;
                        STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
                    }
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_REJECTED_CB, "dDECRejectSC(%d)-c%dp%d ",
                                                PrivateData_p->RejectedSendCommand, iBuff, PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.CmdId));
#endif
                }
                    break;

            case DECODER_COMMAND_JOB_COMPLETED_EVT :
                iFmw = iBuff;
#ifdef H264TASK_TRACE_VERBOSE
                STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (4) iFmw %d Id %d", iFmw, PrivateData_p->FmwContext[iFmw].CommandId));
#endif
                IsCommandIdFmw = FALSE;
                IsCommandIdPar = FALSE;
                iBuff = 0;
                do
                {
                    IsCommandIdFmw = (PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoFmw.CmdStatus.CmdId == PrivateData_p->FmwContext[iFmw].CommandId &&
                                        PrivateData_p->FmwContext[iFmw].BufferUsed);
                    IsCommandIdPar = (PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId == PrivateData_p->FmwContext[iFmw].CommandId &&
                                        PrivateData_p->FmwContext[iFmw].BufferUsed);
                    if (!IsCommandIdFmw && !IsCommandIdPar)
                    {
                        iBuff++;
                    }
                }
                while ((iBuff < DECODER_MAX_NUMBER_INTERMEDIATE_BUFFERS) && !IsCommandIdFmw && !IsCommandIdPar);

                if (IsCommandIdFmw)
                {
#ifdef STVID_VALID_MEASURE_TIMING
                    /* STSYS_WriteRegDev32LE((void*)0xB8025008, 4); *//* Clr PIO 5-2 */
#endif
#ifdef STVID_VALID_SBAG
                    STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, SBAG_H264_DECODE_STOP); /*  trigger */
                    STSYS_WriteRegDev32LE((void*)0xB8025008, 4); /* Clr PIO 5-2 */
#endif

#ifdef STVID_INT_BUFFER_DUMP
#if STVID_INT_BUFFER_DUMP == 2
                        H264CompanionData_p = PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p;
                        DescriptorType = H264CompanionData_p->h264TransformerFmwParam.HostData.DescriptorType;
                        PictureTypeStr[2] = 0;
                        if(PrivateData_p->H264DecoderData[iBuff].IsFirstOfTwoFields)
                        {
                            PictureTypeStr[0] = '0';
                        }
                        else
                        {
                            PictureTypeStr[0] = '1';
                        }
                        if(DescriptorType == H264_PIC_DESCRIPTOR_FIELD_TOP)
                        {
                            PictureTypeStr[1] = 'T';
                        }
                        else if(DescriptorType == H264_PIC_DESCRIPTOR_FIELD_BOTTOM)
                        {
                            PictureTypeStr[1] = 'B';
                        }
                        else
                        {
                            PictureTypeStr[0] = 'F';
                            PictureTypeStr[1] = 0;
                        }
                        /* Save Slice error buffer */
                        sprintf(IntBufferFileName, "../../int_buffers/ses_buffer_post_decode%03d_%03d_%03d_%03d_%s.bin",
                                PrivateData_p->H264DecoderData[iBuff].DecodingOrderFrameID,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.IDExtension,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.ID,
                                PrivateData_p->H264DecoderData[iBuff].PictureNumber,
                                PictureTypeStr);
                        IntBuffer_fp = fopen(IntBufferFileName, "wb");
                        if(IntBuffer_fp != NULL)
                        {
                            IntBufferAddress = STAVMEM_DeviceToCPU(H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer, &PrivateData_p->VirtualMapping);
                            IntBufferSize = DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE;
                            fwrite(IntBufferAddress, 4, (IntBufferSize+3)/4, IntBuffer_fp);
                        }
                        fclose(IntBuffer_fp);
                        /* Save intermediate buffer */
                        sprintf(IntBufferFileName, "../../int_buffers/int_buffer_post_decode%03d_%03d_%03d_%03d_%s.bin",
                                PrivateData_p->H264DecoderData[iBuff].DecodingOrderFrameID,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.IDExtension,
                                PrivateData_p->H264DecoderData[iBuff].ExtendedPresentationOrderPictureID.ID,
                                PrivateData_p->H264DecoderData[iBuff].PictureNumber,
                                PictureTypeStr);
                        IntBuffer_fp = fopen(IntBufferFileName, "wb");
                        if(IntBuffer_fp != NULL)
                        {
                            IntBufferAddress = STAVMEM_DeviceToCPU((U8 *)H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer + DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE, &PrivateData_p->VirtualMapping);
                            IntBufferSize = (U32)H264CompanionData_p->h264TransformerFmwParam.PictureStopAddrBinBuffer - (U32)H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer - DECODER_INTERMEDIATE_SLICE_ERROR_BUFFER_SIZE;
                            fwrite(IntBufferAddress, 4, (IntBufferSize+3)/4, IntBuffer_fp);
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                            if (IntBufferSize > DECODER_INTERMEDIATE_BUFFER_SIZE || H264CompanionData_p->h264TransformerFmwParam.PictureStopAddrBinBuffer == 0)
                            {
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_PP_CALLBACK, "dBUG_BUG_buff%d/IPBE=%d/PSABB=%d ", iBuff, H264CompanionData_p->h264TransformerFmwParam.PictureStopAddrBinBuffer, H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer));
                            }
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                        }
                        fclose(IntBuffer_fp);
#endif /* STVID_INT_BUFFER_DUMP == 2 */
#endif /* STVID_INT_BUFFER_DUMP */
#ifdef H264TASK_TRACE_VERBOSE
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (4) iBuff %d IdFmw %d", iBuff, PrivateData_p->FmwContext[iFmw].CommandId));
#endif
                    /* Decoder is ready for a new picture to be decoded */
                    PrivateData_p->DecoderIsRunning = FALSE;

                    /* inform the controler of the terminated command */
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.CommandId =  PrivateData_p->H264DecoderData[iBuff].CommandIdForProd;
                    if ((PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode != (U32)H264_DECODER_BASE) &&
                        (PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode)){
                        if ((PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode & (U32)H264_DECODER_ERROR_MB_OVERFLOW) == (U32)H264_DECODER_ERROR_MB_OVERFLOW)
                        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_ERRORS, "dH264_DECODER_ERROR_MB_OVERFLOW-c%dp%d ",
                                                                iBuff, PrivateData_p->FmwContext[iFmw].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
#if !(defined(ST_7100) && defined(ST_CUT_1_X))
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports H264_DECODER_ERROR_MB_OVERFLOW\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
#else /* !(defined(ST_7100) && defined(ST_CUT_1_X)) */
                                PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;
#endif /* not (!(defined(ST_7100) && defined(ST_CUT_1_X)) */
                        }
                        if ((PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode & (U32)H264_DECODER_ERROR_RECOVERED) == (U32)H264_DECODER_ERROR_RECOVERED)
                        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_ERRORS, "dH264_DECODER_ERROR_RECOVERED-c%dp%d ",
                                                                iBuff, PrivateData_p->FmwContext[iFmw].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports H264_DECODER_ERROR_RECOVERED\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                        }
                        if ((PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode & (U32)H264_DECODER_ERROR_NOT_RECOVERED) == (U32)H264_DECODER_ERROR_NOT_RECOVERED)
                        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                                TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_ERRORS, "dH264_DECODER_ERROR_NOT_RECOVERED-c%dp%d ",
                                                                iBuff, PrivateData_p->FmwContext[iFmw].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports H264_DECODER_ERROR_NOT_RECOVERED\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                        }
                        if ((PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode & (U32)H264_DECODER_ERROR_GNBvd42696) == (U32)H264_DECODER_ERROR_GNBvd42696)
                        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                         TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_ERRORS, "dH264_DECODER_ERROR_GNBvd42696-c%dp%d ",
                                                      iBuff, PrivateData_p->FmwContext[iFmw].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports H264_DECODER_ERROR_GNBvd42696\n"));
#ifdef STVID_DEBUG_GET_STATISTICS
                         PrivateData_p->Statistics.DecodeGNBvd42696Error++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                         PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                        }
                        if ((PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode & (U32)H264_DECODER_ERROR_TASK_TIMEOUT) == (U32)H264_DECODER_ERROR_TASK_TIMEOUT)
                        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                         TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_ERRORS, "dH264_DECODER_ERROR_TASK_TIMEOUT-c%dp%d ",
                                                      iBuff, PrivateData_p->FmwContext[iFmw].CommandId));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Decoder reports H264_DECODER_ERROR_TASK_TIMEOUT\n"));
                         PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                        }

                        if(((PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.ErrorCode) & ~(H264_DECODER_ERROR_MB_OVERFLOW        |
                                                    	                           H264_DECODER_ERROR_RECOVERED       |
                                                    	                           H264_DECODER_ERROR_NOT_RECOVERED |
                                                                                   H264_DECODER_ERROR_GNBvd42696 |
                                                                                   H264_DECODER_ERROR_TASK_TIMEOUT)) != 0)
                        {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "(DECODER_COMMAND_JOB_COMPLETED_EVT): Firmware reports UNDEFINED ERROR! BUG! BUG! BUG!\n"));
                                PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_ERROR_CORRUPTED;
                        }
                    }
                    else
                    {
                        PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.ErrorCode = DECODER_NO_ERROR;
                    }
                    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.JobState = DECODER_JOB_COMPLETED;
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime = PrivateData_p->H264DecoderData[iBuff].DecodeJobSubmissionTime;
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.JobCompletionTime = PrivateData_p->FmwContext[iFmw].CallBackTime;
#ifdef STVID_VALID_MEASURE_TIMING
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.LXDecodeTimeInMicros = PrivateData_p->FmwContext[iFmw].H264_CommandStatus_fmw.DecodeTimeInMicros;
#ifdef STVID_VALID_MULTICOM_PROFILING
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.HostSideLXSubmissionTime = IT_Host_Send;
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.HostSideLXCompletionTime = IT_Host_Receive;
                    PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.LXSideLXDecodeTime = * (unsigned int *) 0xB9214104; /* gp1 */
#endif /* STVID_VALID_MULTICOM_PROFILING */
#endif
#ifdef STVID_VALID_DEC_TIME
                    VVID_DEC_TimeStore_Command(PrivateData_p, iBuff);
#endif

#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_JOB_END, "dFMWdec_JC%d-b%dp%ds%df%d ",
                                                 PrivateData_p->H264DecoderData[iBuff].CommandIdForProd,
                                                 iBuff, PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.CmdId,
                                                 PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId, PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoFmw.CmdStatus.CmdId));
#endif

                    STEVT_Notify(DECODER_Data_p->EventsHandle, PrivateData_p->RegisteredEventsID[DECODER_JOB_COMPLETED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults));
                    /* Decoder State should be cleared after the event notify to avoid producer task to launch a new decode before all the data have been processed */
                    PrivateData_p->FmwContext[iFmw].BufferUsed = FALSE;
                    PrivateData_p->H264DecoderData[iBuff].State = DECODER_CONTEXT_NOT_USED;
                    if (PrivateData_p->NbBufferUsed > 0)
                    {
                        PrivateData_p->NbBufferUsed--;
                    }
                    else
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER Task : PrivateData_p->NbBufferUsed was already == 0"));
                        PrivateData_p->NbBufferUsed = 0;
                    }
                    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);

                }
                else if (IsCommandIdPar)
                {
#ifdef H264TASK_TRACE_VERBOSE
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (4) iBuff %d IdParam %d", iBuff, PrivateData_p->FmwContext[iFmw].CommandId));
#endif
                    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                    PrivateData_p->FmwContext[iFmw].BufferUsed = FALSE;
                    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
                    /* we received a  MME_SET_GLOBAL_TRANSFORM_PARAMS command --> nothing to do */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_JOB_END, "dFMWset_JC%d-b%dp%ds%d ",
                                                    PrivateData_p->H264DecoderData[iBuff].CommandIdForProd,
                                                    iBuff, PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.CmdId,
                                                    PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p->CmdInfoParam.CmdStatus.CmdId));
#endif
                }
                else
                {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((H264DEC_TRACE_BLOCKID, H264DEC_TRACE_FMW_ERRORS, "dFMWjcErr1-c%df%d ", iFmw, PrivateData_p->FmwContext[iFmw].CommandId));
#endif
#ifdef H264TASK_TRACE_VERBOSE
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"DECODER (h264dec_DecoderTaskFunc) (4) Unknown CmdId %d", PrivateData_p->FmwContext[iFmw].CommandId));
#endif
                    STOS_SemaphoreWait(PrivateData_p->DataProtect_p);
                    PrivateData_p->FmwContext[iFmw].BufferUsed = FALSE;
                    STOS_SemaphoreSignal(PrivateData_p->DataProtect_p);
                }
                break;

            default :
                break;
        }
    }
#if !defined PERF1_TASK2
        }
    } while (!(PrivateData_p->DecoderTask.ToBeDeleted));

    STOS_TaskExit(NULL);
#endif
} /* End of DecoderTaskFunc() function */



/* end of h264task.c */
