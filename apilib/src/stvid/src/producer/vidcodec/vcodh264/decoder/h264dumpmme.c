/*******************************************************************************

File name   : dump_mme.c

Description : ????

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
16/12/2003         Creation                                         Didier SIRON

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>  /* va_start */
#include <stdarg.h> /* va_start */
#include <string.h> /* memset */
#include "stavmem.h" /* Virtual memory mapping */
#include "h264dumpmme.h"
#include "h264decode.h"
#include "testtool.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Public variables --------------------------------------------------------- */

/* Private Defines ---------------------------------------------------------- */

#define DUMP_FILE_NAME  "../../dump_mme.txt"

#define DEC_TIME_DUMP_FILE_NAME  "../../dump_dec_time.txt"
#ifdef ST_OS21
    #define FORMAT_SPEC_OSCLOCK "ll"
    #define OSCLOCK_T_MILLE     1000LL
    #define OSCLOCK_T_MILLION   1000000LL
#else
    #define FORMAT_SPEC_OSCLOCK ""
    #define OSCLOCK_T_MILLE     1000
    #define OSCLOCK_T_MILLION   1000000
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Variables (static) ----------------------------------------------- */

static FILE* DumpFile_p = NULL;
static int Counter = 0;

static H264_SetGlobalParamSPS_t SPS_s;
static H264_SetGlobalParamPPS_t PPS_s;
static H264_ScalingList_t       ScalingList_s;

#ifdef STVID_VALID_DEC_TIME
#define MAX_DEC_TIME 15000
static FILE* DEC_Time_DumpFile_p = NULL;
static int DEC_Time_Counter = 0;
typedef struct DEC_Time_Table_s
{
  osclock_t                        PP_Time;
  osclock_t                        DEC_Time;
  H264_PictureDescriptorType_t     Interlace;
  BOOL                             IsIDR;
  U32                              BitBufferSize;
  U32                              IntBufferSize;
#ifdef STVID_VALID_MEASURE_TIMING
  U32                              LX_DEC_Time;
  U32                              HostSideLX_DEC_Time;
  U32                              LXSideLX_DEC_Time;  
#endif
} DEC_Time_Table_t;
DEC_Time_Table_t DEC_Time_Table[MAX_DEC_TIME];

U32 ValidDisplayLockedCounter[MAX_DEC_TIME];

/* DEC_Time_Table is NOT static because used by vid_disp to report display queue lock */

#endif

#ifdef MME_DUMP_WITH_SCALING_LISTS
static char ScalingMatrixName[8][25] = {"ScalingList_4x4_Intra_Y ",
                                        "ScalingList_4x4_Intra_Cb",
                                        "ScalingList_4x4_Intra_Cr",
                                        "ScalingList_4x4_Inter_Y ",
                                        "ScalingList_4x4_Inter_Cb",
                                        "ScalingList_4x4_Inter_Cr",
                                        "ScalingList_8x8_Intra_Y ",
                                        "ScalingList_8x8_Inter_Y "};
#endif /* MME_DUMP_WITH_SCALING_LISTS */

/* Private Macros ----------------------------------------------------------- */

/* Private Function Prototypes ---------------------------------------------- */

static int myfprintf(FILE *stream, const char *format, ...);
static void DumpSPS(
    H264_SetGlobalParamSPS_t* SPS_p);
static void DumpPPS(
    H264_SetGlobalParamPPS_t* PPS_p);
static void DumpOneList(
    H264_RefPictListAddress_t* Lists_p,
    H264_RefPictList_t* List_p,
    char* ListName);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static int myfprintf(FILE *stream, const char *format, ...)
{
    int ret = 0;

    if (stream != NULL)
    {
        va_list argptr;

        va_start(argptr, format);     /* Initialize variable arguments. */

        ret = vfprintf(stream, format, argptr);

        va_end(argptr);

        fflush(stream);
    }

    return ret;
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t HDM_Init(void)
{
    ST_ErrorCode_t Error;

    Error = ST_NO_ERROR;
    Counter = 0;

    DumpFile_p = fopen(DUMP_FILE_NAME, "w");
    if (DumpFile_p == NULL)
    {
        printf("*** HDM_Init_stub: fopen error on file '%s' ***\n", DUMP_FILE_NAME);
        Error = ST_ERROR_OPEN_HANDLE;
    }

    myfprintf(DumpFile_p, "H264_MME_API_VERSION = %s\n", H264_MME_API_VERSION);

    memset(&SPS_s, -1, sizeof(SPS_s));
    memset(&PPS_s, -1, sizeof(PPS_s));

    return Error;
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t HDM_Term(void)
{
    if (DumpFile_p != NULL)
    {
        (void)fclose(DumpFile_p);
        DumpFile_p = NULL;
    }

    return ST_NO_ERROR;
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void DumpSPS(
    H264_SetGlobalParamSPS_t* SPS_p)
{
    myfprintf(DumpFile_p, "\n");
    myfprintf(DumpFile_p, "   DecoderProfileConformance = %d\n", SPS_p->DecoderProfileConformance);
    myfprintf(DumpFile_p, "   level_idc = %d\n", SPS_p->level_idc);
    myfprintf(DumpFile_p, "   log2_max_frame_num_minus4 = %d\n", SPS_p->log2_max_frame_num_minus4);
    myfprintf(DumpFile_p, "   pic_order_cnt_type = %d\n", SPS_p->pic_order_cnt_type);
    myfprintf(DumpFile_p, "   log2_max_pic_order_cnt_lsb_minus4 = %d\n", SPS_p->log2_max_pic_order_cnt_lsb_minus4);
    myfprintf(DumpFile_p, "   delta_pic_order_always_zero_flag = %d\n", SPS_p->delta_pic_order_always_zero_flag);
    myfprintf(DumpFile_p, "   pic_width_in_mbs_minus1 = %d\n", SPS_p->pic_width_in_mbs_minus1);
    myfprintf(DumpFile_p, "   pic_height_in_map_units_minus1 = %d\n", SPS_p->pic_height_in_map_units_minus1);
    myfprintf(DumpFile_p, "   frame_mbs_only_flag = %d\n", SPS_p->frame_mbs_only_flag);
    myfprintf(DumpFile_p, "   mb_adaptive_frame_field_flag = %d\n", SPS_p->mb_adaptive_frame_field_flag);
    myfprintf(DumpFile_p, "   direct_8x8_inference_flag = %d\n", SPS_p->direct_8x8_inference_flag);
    myfprintf(DumpFile_p, "   chroma_format_idc = %d\n", SPS_p->chroma_format_idc);
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void DumpPPS(
    H264_SetGlobalParamPPS_t* PPS_p)
{
#ifdef MME_DUMP_WITH_SCALING_LISTS
    U32 ScalingListCounter;
    U32 CoefCounter;
#endif /* MME_DUMP_WITH_SCALING_LISTS */

    myfprintf(DumpFile_p, "\n");
    myfprintf(DumpFile_p, "   entropy_coding_mode_flag = %d\n", PPS_p->entropy_coding_mode_flag);
    myfprintf(DumpFile_p, "   pic_order_present_flag = %d\n", PPS_p->pic_order_present_flag);
    myfprintf(DumpFile_p, "   num_ref_idx_l0_active_minus1 = %d\n", PPS_p->num_ref_idx_l0_active_minus1);
    myfprintf(DumpFile_p, "   num_ref_idx_l1_active_minus1 = %d\n", PPS_p->num_ref_idx_l1_active_minus1);
    myfprintf(DumpFile_p, "   weighted_pred_flag = %d\n", PPS_p->weighted_pred_flag);
    myfprintf(DumpFile_p, "   weighted_bipred_idc = %d\n", PPS_p->weighted_bipred_idc);
    myfprintf(DumpFile_p, "   pic_init_qp_minus26 = %d\n", PPS_p->pic_init_qp_minus26);
    myfprintf(DumpFile_p, "   chroma_qp_index_offset = %d\n", PPS_p->chroma_qp_index_offset);
    myfprintf(DumpFile_p, "   deblocking_filter_control_present_flag = %d\n", PPS_p->deblocking_filter_control_present_flag);
    myfprintf(DumpFile_p, "   constrained_intra_pred_flag = %d\n", PPS_p->constrained_intra_pred_flag);
    myfprintf(DumpFile_p, "   transform_8x8_mode_flag = %d\n", PPS_p->transform_8x8_mode_flag);
    myfprintf(DumpFile_p, "   second_chroma_qp_index_offset = %d\n", PPS_p->second_chroma_qp_index_offset);
#ifdef MME_DUMP_WITH_SCALING_LISTS
    myfprintf(DumpFile_p, "   scaling_list_updated = %d\n", PPS_p->ScalingList.ScalingListUpdated);
    if(PPS_p->ScalingList.ScalingListUpdated)
    {
        for(ScalingListCounter = 0 ; ScalingListCounter < 8 ; ScalingListCounter++)
        {
            if(ScalingListCounter < 6)
            {
                for(CoefCounter = 0 ; CoefCounter < 16 ; CoefCounter++)
                {
                    myfprintf(DumpFile_p, "   %s[%2d] = %d\n", ScalingMatrixName[ScalingListCounter], CoefCounter, PPS_p->ScalingList.FirstSixScalingList[ScalingListCounter][CoefCounter]);
                }
            }
            else
            {
                for(CoefCounter = 0 ; CoefCounter < 64 ; CoefCounter++)
                {
                    myfprintf(DumpFile_p, "   %s[%2d] = %d\n", ScalingMatrixName[ScalingListCounter], CoefCounter, PPS_p->ScalingList.NextTwoScalingList[ScalingListCounter-6][CoefCounter]);
                }
            }
        }
    }
#endif /* MME_DUMP_WITH_SCALING_LISTS */
}
/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void DumpBuffersAddress(H264_DecodedBufferAddress_t BufferAddresses)
{
    myfprintf(DumpFile_p, "   Frame buffer Adresses\n");
    myfprintf(DumpFile_p, "     Luma             = 0x%08x\n", BufferAddresses.Luma_p);
    myfprintf(DumpFile_p, "     Chroma           = 0x%08x\n", BufferAddresses.Chroma_p);
    myfprintf(DumpFile_p, "     Luma Decimated   = 0x%08x\n", BufferAddresses.LumaDecimated_p);
    myfprintf(DumpFile_p, "     Chroma Decimated = 0x%08x\n", BufferAddresses.ChromaDecimated_p);
    myfprintf(DumpFile_p, "     MB Struct        = 0x%08x\n", BufferAddresses.MBStruct_p);
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void DumpHostData(
    H264_HostData_t* HostData_p, int ForceIdrFlagToZero)
{
    myfprintf(DumpFile_p, "\n");
    myfprintf(DumpFile_p, "   PictureNumber = %d\n", HostData_p->PictureNumber);
    if (HostData_p->DescriptorType != H264_PIC_DESCRIPTOR_FIELD_BOTTOM)
    {
        myfprintf(DumpFile_p, "   PicOrderCntTop = %d\n", HostData_p->PicOrderCntTop);
    }
    if (HostData_p->DescriptorType != H264_PIC_DESCRIPTOR_FIELD_TOP)
    {
        myfprintf(DumpFile_p, "   PicOrderCntBot = %d\n", HostData_p->PicOrderCntBot);
    }
    myfprintf(DumpFile_p, "   PicOrderCnt = %d\n", HostData_p->PicOrderCnt);
    myfprintf(DumpFile_p, "   DescriptorType = %d\n", HostData_p->DescriptorType);
    myfprintf(DumpFile_p, "   ReferenceType = %d\n", HostData_p->ReferenceType);
    if (ForceIdrFlagToZero)
    {
        myfprintf(DumpFile_p, "   IdrFlag = %d\n", 0 /* To align on Firmware output, prev: HostData_p->IdrFlag */);
    }
    else
    {
        myfprintf(DumpFile_p, "   IdrFlag = %d\n", HostData_p->IdrFlag);
    }
    myfprintf(DumpFile_p, "   NonExistingFlag = %d\n", HostData_p->NonExistingFlag);
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void DumpOneList(
    H264_RefPictListAddress_t* Lists_p,
    H264_RefPictList_t* List_p,
    char* ListName)
{
    U32                                     i;
    U32                                     Index;
    H264_PictureDescriptor_t                *PictureDescriptor_p;
    U32                                     *RefPicList;
    H264_ReferenceFrameAddress_t            *ReferenceFrameAddress_p;

    PictureDescriptor_p = &(Lists_p->PictureDescriptor[0]);

    myfprintf(DumpFile_p, "\n");
    myfprintf(DumpFile_p, "   %s\n", ListName);

    RefPicList = &(List_p->RefPicList[0]);
    ReferenceFrameAddress_p = &(Lists_p->ReferenceFrameAddress[0]);

    for ( i = 0 ; i < List_p->RefPiclistSize ; i++)
    {
        myfprintf(DumpFile_p, "   element #%d", i);
        Index = RefPicList[i];
        DumpHostData(&(PictureDescriptor_p[Index].HostData), 1); /* Force IdrFlag to Zero to align on firmware output */
#ifdef MME_DUMP_WITH_ADDR
        DumpBuffersAddress(ReferenceFrameAddress_p[PictureDescriptor_p[Index].FrameIndex].DecodedBufferAddress);
#endif
    }
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t HDM_DumpCommand(MME_CommandCode_t CmdCode, MME_Command_t* CmdInfo_p)
{
    ST_ErrorCode_t Error;
    static H264_SetGlobalParam_t           *GlobalParam_p;
    static H264_TransformParam_fmw_t       *TransformParamFmw_p;
    static H264_HostData_t                 *HostData_p;
    static H264_RefPictListAddress_t       *Lists_p;

    Error = ST_NO_ERROR;

    switch (CmdCode)
    {
        case MME_SET_GLOBAL_TRANSFORM_PARAMS:
            GlobalParam_p = (H264_SetGlobalParam_t *)CmdInfo_p->Param_p;
            
            /* Memorize the SPS values */
            memcpy(&SPS_s, &(GlobalParam_p->H264SetGlobalParamSPS), sizeof(SPS_s));
            memcpy(&PPS_s, &(GlobalParam_p->H264SetGlobalParamPPS), sizeof(PPS_s));
            break;

        case MME_TRANSFORM:
            if(DumpFile_p != NULL)
            {
                TransformParamFmw_p = (H264_TransformParam_fmw_t *)CmdInfo_p->Param_p;
                HostData_p = &(TransformParamFmw_p->HostData);
                Lists_p = &(TransformParamFmw_p->InitialRefPictList);

                Counter += 1;
                myfprintf(DumpFile_p, "\nMME_TRANSFORM #%d\n", Counter);
                myfprintf(DumpFile_p, "   StructSize = %d\n", 0 /* To align on Firmware output, prev: CmdInfo_p->StructSize */);
                myfprintf(DumpFile_p, "   ParamSize = %d\n", 38 /* To align on Firmware output, prev: CmdInfo_p->ParamSize */);

                DumpSPS(&SPS_s);
                DumpPPS(&PPS_s);

                /* Note: we don't dump PictureStartAddrBinBuffer/PictureStopAddrBinBuffer
                        because it is implementation dependant */

                myfprintf(DumpFile_p, "\n");
                myfprintf(DumpFile_p, "   HorizontalDecimationFactor = %d\n", TransformParamFmw_p->HorizontalDecimationFactor);
                myfprintf(DumpFile_p, "   VerticalDecimationFactor = %d\n", TransformParamFmw_p->VerticalDecimationFactor);

                DumpHostData(HostData_p, 0); /* keep IdrFlag as this one is managed by the firmware output */
#ifdef MME_DUMP_WITH_ADDR
                myfprintf(DumpFile_p, "   Picture Start Addr = 0x%08x\n", TransformParamFmw_p->PictureStartAddrBinBuffer);
                myfprintf(DumpFile_p, "   Picture Stop Addr  = 0x%08x\n", TransformParamFmw_p->PictureStopAddrBinBuffer);
                myfprintf(DumpFile_p, "   Picture Data Size  = %d bytes\n", (U32)TransformParamFmw_p->PictureStopAddrBinBuffer-(U32)TransformParamFmw_p->PictureStartAddrBinBuffer);
                DumpBuffersAddress(TransformParamFmw_p->DecodedBufferAddress);
#endif

                /* Note: we don't dump DecodedBufferAddress/ReferenceFrameAddress_p
                        because it is implementation dependant */

                myfprintf(DumpFile_p, "\n");
                myfprintf(DumpFile_p, "   ReferenceDescriptorsBitsField = 0x%08x\n", Lists_p->ReferenceDescriptorsBitsField);
                DumpOneList(Lists_p, &(Lists_p->InitialPList0), "InitialPList0");
                DumpOneList(Lists_p, &(Lists_p->InitialBList0), "InitialBList0");
                DumpOneList(Lists_p, &(Lists_p->InitialBList1), "InitialBList1");
            }
            break;

        default :
            myfprintf(DumpFile_p, "*** HDM_DumpCommand: unknown CmdCode '%d' ***\n", CmdCode);
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return Error;
}

#ifdef STVID_VALID_DEC_TIME
/*******************************************************************************
 * Name        : VVID_DEC_TimeDump_Init
 * Description : initialize the decoder time dump process
 * Parameters  : filename
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
ST_ErrorCode_t VVID_DEC_TimeDump_Init(char * filename)
{
    ST_ErrorCode_t Error;

    Error = ST_NO_ERROR;

    DEC_Time_DumpFile_p = fopen(filename, "w");
    if (DEC_Time_DumpFile_p == NULL)
    {
        printf("*** DEC_Time_Dump: fopen error on file '%s' ***\n", DEC_TIME_DUMP_FILE_NAME);
        Error = ST_ERROR_OPEN_HANDLE;
    }

    return Error;
}

/*******************************************************************************
 * Name        : VVID_DEC_TimeDump_Term
 * Description : terminates the preprocessor time dump process
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
ST_ErrorCode_t VVID_DEC_TimeDump_Term(void)
{

    if (DEC_Time_DumpFile_p != NULL)
    {
        (void)fclose(DEC_Time_DumpFile_p);
        DEC_Time_DumpFile_p = NULL;
    }

    return ST_NO_ERROR;
}

/*******************************************************************************
 * Name        : VVID_DEC_TimeStore_Command
 * Description : terminates the decoder time dump process
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
ST_ErrorCode_t VVID_DEC_TimeStore_Command(H264DecoderPrivateData_t * PrivateData_p, U32 iBuff)
{
    ST_ErrorCode_t Error;
    U32 BitBufferSize;    
    H264CompanionData_t * H264CompanionData_p;

    Error = ST_NO_ERROR;

    H264CompanionData_p = PrivateData_p->H264DecoderData[iBuff].H264CompanionData_p;

    if (DEC_Time_Counter < MAX_DEC_TIME - 1)
    {
      DEC_Time_Counter += 1;
      DEC_Time_Table[DEC_Time_Counter].PP_Time = (PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.DecodingJobStatus.JobCompletionTime -
                                                  PrivateData_p->H264DecoderData[iBuff].DECODER_PreprocJobResults.DecodingJobStatus.JobSubmissionTime
                                                 );
      DEC_Time_Table[DEC_Time_Counter].Interlace = H264CompanionData_p->h264TransformerFmwParam.HostData.DescriptorType;
     	if ((U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStopAddrBitBuffer_p < (U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStartAddrBitBuffer_p)
      {
         /* Write pointer has done a loop, not the Read pointer */
         BitBufferSize = (U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.BitBufferEndAddress_p
                         - (U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStartAddrBitBuffer_p
                         + (U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStopAddrBitBuffer_p
                         - (U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.BitBufferBeginAddress_p
                         + 1;
      }
      else
      {
        /* Normal: start <= read <= write <= top */
        BitBufferSize = (U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStopAddrBitBuffer_p
                        - (U32)PrivateData_p->H264DecoderData[iBuff].H264PreprocCommand.TransformerPreprocParam.PictureStartAddrBitBuffer_p;
      }
      DEC_Time_Table[DEC_Time_Counter].BitBufferSize = BitBufferSize;
  
      DEC_Time_Table[DEC_Time_Counter].IntBufferSize = (U32)H264CompanionData_p->h264TransformerFmwParam.PictureStopAddrBinBuffer - (U32)H264CompanionData_p->h264TransformerFmwParam.PictureStartAddrBinBuffer;

      /* Decoder */
      DEC_Time_Table[DEC_Time_Counter].DEC_Time = time_minus(PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.JobCompletionTime,
                                                   PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime
                                                  );
#ifdef STVID_VALID_MEASURE_TIMING
      DEC_Time_Table[DEC_Time_Counter].LX_DEC_Time = PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.LXDecodeTimeInMicros;
#ifdef STVID_VALID_MULTICOM_PROFILING
      DEC_Time_Table[DEC_Time_Counter].HostSideLX_DEC_Time = (U32) time_minus(PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.HostSideLXCompletionTime, 
                                                                              PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.HostSideLXSubmissionTime)
                                                                   *OSCLOCK_T_MILLION/ST_GetClocksPerSecond();
      DEC_Time_Table[DEC_Time_Counter].LXSideLX_DEC_Time = PrivateData_p->H264DecoderData[iBuff].DECODER_DecodingJobResults.DecodingJobStatus.LXSideLXDecodeTime;
#endif /* STVID_VALID_MULTICOM_PROFILING */
#endif /* STVID_VALID_MEASURE_TIMING */

      DEC_Time_Table[DEC_Time_Counter].IsIDR = H264CompanionData_p->h264TransformerFmwParam.HostData.IdrFlag;
    
      ValidDisplayLockedCounter[DEC_Time_Counter] = 0;
    }  
    return Error;
}

/*******************************************************************************
 * Name        : VVID_DEC_TimeDump_Command
 * Description : terminates the decoder time dump process
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
static BOOL VVID_DEC_TimeDump_Command(STTST_Parse_t *pars_p, char *result_sym_p)
{        
    BOOL    RetErr;
    U32 NbPicToDump;
    
    RetErr = STTST_GetInteger(pars_p, 10 , (S32*)&NbPicToDump );

    if (DEC_Time_Counter < NbPicToDump)
    {
        NbPicToDump = DEC_Time_Counter;
    }
    if (VVID_DEC_TimeDump_Command_function(DEC_TIME_DUMP_FILE_NAME, DEC_Time_Counter))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL VVID_DEC_TimeDump_Command_function(char * filename, U32 DEC_Time_Counter)
{
    osclock_t TOut_ClockTicks;
    U32 i;
    char Interlace;
    U32 NbDisplayQueue = 0;
    U32 MaxDecodingTime = 0 ;
    U32 NbPicturesOverBudget = 0;
        
    TOut_ClockTicks = (osclock_t) ST_GetClocksPerSecond();

    VVID_DEC_TimeDump_Init(filename);
    
    if(DEC_Time_DumpFile_p != NULL)
    {
      for(i=1;i<= DEC_Time_Counter; i++)
      {
        if ((DEC_Time_Table[i].Interlace == H264_PIC_DESCRIPTOR_FRAME) ||
            (DEC_Time_Table[i].Interlace == H264_PIC_DESCRIPTOR_AFRAME))
        {
            if (DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks > 33333)
            {
                NbPicturesOverBudget++;
            }
        }
        else
        {
            if (DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks > 16666)
            {
                NbPicturesOverBudget++;
            }           
        }
        if (DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks > MaxDecodingTime)
        {
           MaxDecodingTime = DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks; 
        }
        NbDisplayQueue += ValidDisplayLockedCounter[i];
      }
#ifdef STVID_VALID_MULTICOM_PROFILING
      myfprintf(DEC_Time_DumpFile_p, "DEC_TRANSFORM IsIDR Interlace PPTime BBSize IBSize ST40-DECTime LX-DECTime HostSideLX-DECTime LXSideLX-DECTime %d %d %d %d\n", NbDisplayQueue, DEC_Time_Counter, MaxDecodingTime, NbPicturesOverBudget);
#else
      myfprintf(DEC_Time_DumpFile_p, "DEC_TRANSFORM IsIDR Interlace PPTime BBSize IBSize ST40-DECTime LX-DECTime %d %d %d %d\n", NbDisplayQueue, DEC_Time_Counter, MaxDecodingTime, NbPicturesOverBudget);
#endif

      for(i=1; i <= DEC_Time_Counter; i++)
      {
        if ((DEC_Time_Table[i].Interlace == H264_PIC_DESCRIPTOR_FRAME) ||
            (DEC_Time_Table[i].Interlace == H264_PIC_DESCRIPTOR_AFRAME))       
        {
          Interlace = 'F'; /* Frame */
        }
        else if (DEC_Time_Table[i].Interlace == H264_PIC_DESCRIPTOR_FIELD_TOP)
        {
          Interlace = 'T'; /* Top Field */
        }
        else
        {
          Interlace = 'B'; /* Bottom field */
        }
#ifdef STVID_VALID_MEASURE_TIMING
#ifdef STVID_VALID_MULTICOM_PROFILING
        myfprintf(DEC_Time_DumpFile_p, "%d %d %c %10"FORMAT_SPEC_OSCLOCK"d %d %d %10"FORMAT_SPEC_OSCLOCK"d %d %d %d %d\n",
                                        i /* DEC_TRANSFORM */,
                                        DEC_Time_Table[i].IsIDR,
                                        Interlace,
                                        DEC_Time_Table[i].PP_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        DEC_Time_Table[i].BitBufferSize,
                                        DEC_Time_Table[i].IntBufferSize,
                                        DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        DEC_Time_Table[i].LX_DEC_Time,
                                        DEC_Time_Table[i].HostSideLX_DEC_Time,
                                        DEC_Time_Table[i].LXSideLX_DEC_Time,
                                        ValidDisplayLockedCounter[i]                                       
                                        );

#else
        myfprintf(DEC_Time_DumpFile_p, "%d %d %c %10"FORMAT_SPEC_OSCLOCK"d %d %d %10"FORMAT_SPEC_OSCLOCK"d %d %d\n",
                                        i /* DEC_TRANSFORM */,
                                        DEC_Time_Table[i].IsIDR,
                                        Interlace,
                                        DEC_Time_Table[i].PP_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        DEC_Time_Table[i].BitBufferSize,
                                        DEC_Time_Table[i].IntBufferSize,
                                        DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        DEC_Time_Table[i].LX_DEC_Time,
                                        ValidDisplayLockedCounter[i]                                       
                                        );
#endif /* STVID_VALID_MULTICOM_PROFILING */
#else /* not STVID_VALID_MEASURE_TIMING */
        myfprintf(DEC_Time_DumpFile_p, "%d %d %c %10"FORMAT_SPEC_OSCLOCK"d %d %d %10"FORMAT_SPEC_OSCLOCK"d\n",
                                        i /* DEC_TRANSFORM */,
                                        DEC_Time_Table[i].IsIDR,
                                        Interlace,
                                        DEC_Time_Table[i].PP_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        DEC_Time_Table[i].BitBufferSize,
                                        DEC_Time_Table[i].IntBufferSize,
                                        DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks);
#endif                                        
      }
      VVID_DEC_TimeDump_Term();
      return FALSE;
    }
    else
    {
      return TRUE;
    }
}
/*-------------------------------------------------------------------------
 * Function : VIDH264DecodeTime_RegisterCmd
 *            Register video command (1 for each API function)
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VIDH264DecodeTime_RegisterCmd (void)
{
    BOOL RetErr;
    U32  i;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand("VVID_DUMP_DEC_TIME",    VVID_DEC_TimeDump_Command, "Dumps Preproc time over pictures");

    if (RetErr)
    {
        STTST_Print(("VIDH264DecodeTime_RegisterCmd()     : failed !\n" ));
    }
    else
    {
        STTST_Print(("VIDH264DecodeTime_RegisterCmd()     : ok\n" ));
    }
    return(RetErr ? FALSE : TRUE);

} /* end VIDH264DecodeTime_RegisterCmd */
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

/* End of dump_mme.c */
