/*******************************************************************************

File name   : mpg2dumpmme.c

Description : ????

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
16/12/2003         Creation                                         Didier SIRON

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#endif
#include "mpg2dumpmme.h"
#include "mpg2dec.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Public variables --------------------------------------------------------- */

/* Private Defines ---------------------------------------------------------- */
#ifdef STVID_VALID_DEC_TIME
#define MAX_DEC_TIME 15000
#ifdef ST_OS21
    #define FORMAT_SPEC_OSCLOCK "ll"
    #define OSCLOCK_T_MILLE     1000LL
    #define OSCLOCK_T_MILLION   1000000LL
#else
    #define FORMAT_SPEC_OSCLOCK ""
    #define OSCLOCK_T_MILLE     1000
    #define OSCLOCK_T_MILLION   1000000
#endif
#endif

/* Set this define to 1 if you want to have all the debug printf
   Set this define to 0 if you want to have only the error printf */
#define VERBOSE_     1

#define DUMP_FILE_NAME  "mpg2_dumpmme.txt"

/* Private Types ------------------------------------------------------------ */

/* Private Variables (static) ----------------------------------------------- */
#ifdef STVID_VALID_DEC_TIME
static FILE* DEC_Time_DumpFile_p = NULL;
static int DEC_Time_Counter = 0;
typedef struct DEC_Time_Table_s
{
  osclock_t                        DEC_Time;
  U32                              BitBufferSize;
#ifdef STVID_VALID_MEASURE_TIMING
  U32                              LX_DEC_Time;
#endif /* STVID_VALID_MEASURE_TIMING */
} DEC_Time_Table_t;
static DEC_Time_Table_t DEC_Time_Table[MAX_DEC_TIME];
extern U32 ValidDisplayLockedCounter[MAX_DEC_TIME];
#endif

static FILE* DumpFile_p = NULL;
static int GlobalParamCounter = 0;
static int TransformParamCounter = 0;

static MPEG2_SetGlobalParamSequence_t SPS_s;
static MPEG2_TransformParam_t PPS_s;

/* Private Macros ----------------------------------------------------------- */

/* Private Function Prototypes ---------------------------------------------- */

static int myfprintf(FILE *stream, const char *format, ...);
static void MPEG2_DumpSPS( MPEG2_SetGlobalParamSequence_t* SPS_p);
static void MPEG2_DumpPPS( MPEG2_TransformParam_t* PPS_p);
MME_ERROR MPEG2_HDM_Init(void);
MME_ERROR MPEG2_HDM_Term(void);
MME_ERROR MPEG2_HDM_DumpCommand(MME_Command_t* CmdInfo_p);

/* Functions ---------------------------------------------------------------- */


#ifdef STVID_VALID_DEC_TIME

/*******************************************************************************
 * Name        : VVID_DEC_TimeDump_Init
 * Description : initialize the decoder time dump process
 * Parameters  : filename
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
static ST_ErrorCode_t VVID_MPEG2DEC_TimeDump_Init(char * filename)
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
 * Name        : VVID_MPEG2DEC_TimeDump_Term
 * Description : terminates the preprocessor time dump process
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
static ST_ErrorCode_t VVID_MPEG2DEC_TimeDump_Term(void)
{

    if (DEC_Time_DumpFile_p != NULL)
    {
        (void)fclose(DEC_Time_DumpFile_p);
        DEC_Time_DumpFile_p = NULL;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
 * Name        : VVID_MPEG2DEC_TimeStore_Command
 * Description : Stores the Decode timing information
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
ST_ErrorCode_t VVID_MPEG2DEC_TimeStore_Command(MPEG2DecoderPrivateData_t * PrivateData_p)
{
    ST_ErrorCode_t Error;
    U32 BitBufferSize;

    Error = ST_NO_ERROR;

    if (DEC_Time_Counter < MAX_DEC_TIME - 1)
    {
      DEC_Time_Counter += 1;

      /* Decoder */
      DEC_Time_Table[DEC_Time_Counter].DEC_Time = time_minus(PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobCompletionTime,
                                                    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime);

#ifdef STVID_VALID_MEASURE_TIMING
      DEC_Time_Table[DEC_Time_Counter].LX_DEC_Time = PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.LXDecodeTimeInMicros;
#endif /* STVID_VALID_MEASURE_TIMING */
	  /* This is defined in the H.264 decoder and set in vid_disp.c*/
      ValidDisplayLockedCounter[DEC_Time_Counter] = 0;
    }
    return Error;
}/* end VVID_MPEG2DEC_TimeStore_Command */


/*******************************************************************************
 * Name        : VVID_MPEG2DEC_TimeDump_Command
 * Description : terminates the decoder time dump process
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
static BOOL VVID_MPEG2DEC_TimeDump_Command(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;
    U32 NbPicToDump;

    RetErr = STTST_GetInteger(pars_p, 10 , (S32*)&NbPicToDump );

    if (DEC_Time_Counter < NbPicToDump)
    {
        NbPicToDump = DEC_Time_Counter;
    }
    if (VVID_MPEG2DEC_TimeDump_Command_function(DEC_TIME_DUMP_FILE_NAME, DEC_Time_Counter))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL VVID_MPEG2DEC_TimeDump_Command_function(char * filename, U32 DEC_Time_Counter)
{
    osclock_t TOut_ClockTicks;
    U32 i;
    char Interlace;
    U32 NbDisplayQueue = 0;
    U32 MaxDecodingTime = 0 ;
    U32 NbPicturesOverBudget = 0;

    TOut_ClockTicks = (osclock_t) ST_GetClocksPerSecond();

    VVID_MPEG2DEC_TimeDump_Init(filename);

    if(DEC_Time_DumpFile_p != NULL)
    {
#ifdef STVID_VALID_MEASURE_TIMING
      myfprintf(DEC_Time_DumpFile_p, "DEC_TRANSFORM  ST40-DECTime LX-DECTime DispLockCount\n");
#else /* STVID_VALID_MEASURE_TIMING */
      myfprintf(DEC_Time_DumpFile_p, "DEC_TRANSFORM  ST40-DECTime DispLockCount\n");
#endif /* not STVID_VALID_MEASURE_TIMING */

      for(i=1; i <= DEC_Time_Counter; i++)
      {
#ifdef STVID_VALID_MEASURE_TIMING
        myfprintf(DEC_Time_DumpFile_p, "%d %10"FORMAT_SPEC_OSCLOCK"d %10d %d\n",
										i /* DEC_TRANSFORM */,
                                        DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        DEC_Time_Table[i].LX_DEC_Time,
                                        ValidDisplayLockedCounter[i]
                                        );
#else /* STVID_VALID_MEASURE_TIMING */
        myfprintf(DEC_Time_DumpFile_p, "%d %10"FORMAT_SPEC_OSCLOCK"d %d\n",
										i /* DEC_TRANSFORM */,
                                        DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        ValidDisplayLockedCounter[i]
                                        );
#endif /* not STVID_VALID_MEASURE_TIMING */
      }
      VVID_MPEG2DEC_TimeDump_Term();
      return FALSE;
    }
    else
    {
      return TRUE;
    }
}

/*-------------------------------------------------------------------------
 * Function : VIDMPEG2DecodeTime_RegisterCmd
 *            Register video command (1 for each API function)
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VIDMPEG2DecodeTime_RegisterCmd (void)
{
    BOOL RetErr;
    U32  i;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand("VVID_MPEG2_DUMP_DEC_TIME",    VVID_MPEG2DEC_TimeDump_Command, "Dumps Preproc time over pictures");

    if (RetErr)
    {
        STTST_Print(("VIDMPEG2DecodeTime_RegisterCmd()     : failed !\n" ));
    }
    else
    {
        STTST_Print(("VIDMPEG2DecodeTime_RegisterCmd()     : ok\n" ));
    }
    return(RetErr ? FALSE : TRUE);

} /* end VIDMPEG2DecodeTime_RegisterCmd */
#endif/* STVID_VALID_DEC_TIME */

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
MME_ERROR MPEG2_HDM_Init(void)
{
    MME_ERROR Error;

    Error = MME_SUCCESS;

    GlobalParamCounter = 0;
    TransformParamCounter = 0;

    if (VERBOSE_)
    {
        DumpFile_p = fopen(DUMP_FILE_NAME, "w");
        if (DumpFile_p == NULL)
        {
            printf("*** MPEG2_HDM_Init: fopen error on file '%s' ***\n", DUMP_FILE_NAME);
            Error = MME_HANDLES_STILL_OPEN;
        }
    }
    else
    {
        DumpFile_p = NULL;
    }

    myfprintf(DumpFile_p, "------------------------------\n");
    myfprintf(DumpFile_p, "Transformer Initial Parameters\n");
    myfprintf(DumpFile_p, "------------------------------\n");
    myfprintf(DumpFile_p, "MPEG_Decoding_Mode = 0\n"); /* To be aligned with reference decoder */

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
MME_ERROR MPEG2_HDM_Term(void)
{
    if (DumpFile_p != NULL)
    {
        (void)fclose(DumpFile_p);
        DumpFile_p = NULL;
    }
    return MME_SUCCESS;
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void MPEG2_DumpSPS(
    MPEG2_SetGlobalParamSequence_t* SPS_p)
{
	int i;

  myfprintf(DumpFile_p, "------------------------\n");
  myfprintf(DumpFile_p, "Sequence Parameters #%d\n", GlobalParamCounter);
  myfprintf(DumpFile_p, "------------------------\n");
  myfprintf(DumpFile_p, "MPEG_Decoding_Mode = 0\n"); /* to be aligned with reference decoder */
#ifdef MME11
	myfprintf(DumpFile_p, "   MPEGStreamTypeFlag = %d\n", SPS_p->MPEGStreamTypeFlag);
#endif
    myfprintf(DumpFile_p, "horizontal_size = %d\n", SPS_p->horizontal_size);
    myfprintf(DumpFile_p, "vertical_size = %d\n", SPS_p->vertical_size);
    myfprintf(DumpFile_p, "progressive_sequence = %d\n", SPS_p->progressive_sequence);
    myfprintf(DumpFile_p, "chroma_format = %d\n", SPS_p->chroma_format);
	for (i=0; i<64; i++)
	{
		myfprintf(DumpFile_p, "intra_quantizer_matrix[%d] = %d\n", i, SPS_p->intra_quantiser_matrix[i]);
	}
	for (i=0; i<64; i++)
	{
		myfprintf(DumpFile_p, "non_intra_quantizer_matrix[%d] = %d\n", i, SPS_p->non_intra_quantiser_matrix[i]);
	}
	for (i=0; i<64; i++)
	{
		myfprintf(DumpFile_p, "chroma_intra_quantizer_matrix[%d] = %d\n", i, SPS_p->chroma_intra_quantiser_matrix[i]);
	}
	for (i=0; i<64; i++)
	{
		myfprintf(DumpFile_p, "chroma_non_intra_quantizer_matrix[%d] = %d\n", i, SPS_p->chroma_non_intra_quantiser_matrix[i]);
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
static void MPEG2_DumpPPS(
    MPEG2_TransformParam_t* PPS_p)
{
	myfprintf(DumpFile_p, "PictureSize = 0x%x\n", (U32)PPS_p->PictureStopAddrCompressedBuffer_p - (U32)PPS_p->PictureStartAddrCompressedBuffer_p);
	myfprintf(DumpFile_p, "DecodedTemporalReference = %d\n", PPS_p->DecodedBufferAddress.DecodedTemporalReferenceValue);
	myfprintf(DumpFile_p, "ForwardTemporalReference = %d\n", PPS_p->RefPicListAddress.ForwardTemporalReferenceValue);
	myfprintf(DumpFile_p, "BackwardTemporalReference = %d\n", PPS_p->RefPicListAddress.BackwardTemporalReferenceValue);
  myfprintf(DumpFile_p, "MainAuxEnable = %d\n", PPS_p->MainAuxEnable);
  myfprintf(DumpFile_p, "HorizontalDecimationFactor = %d\n", PPS_p->HorizontalDecimationFactor);
  myfprintf(DumpFile_p, "VerticalDecimationFactor = %d\n", PPS_p->VerticalDecimationFactor);
  myfprintf(DumpFile_p, "DecodingMode = %d\n", PPS_p->DecodingMode);
  myfprintf(DumpFile_p, "AdditionalFlags = %d\n", PPS_p->AdditionalFlags);
	myfprintf(DumpFile_p, "picture_coding_type = %d\n", PPS_p->PictureParameters.picture_coding_type);
	myfprintf(DumpFile_p, "forward_horizontal_f_code = %d\n", PPS_p->PictureParameters.forward_horizontal_f_code);
	myfprintf(DumpFile_p, "forward_vertical_f_code = %d\n", PPS_p->PictureParameters.forward_vertical_f_code);
	myfprintf(DumpFile_p, "backward_horizontal_f_code = %d\n", PPS_p->PictureParameters.backward_horizontal_f_code);
	myfprintf(DumpFile_p, "backward_vertical_f_code = %d\n", PPS_p->PictureParameters.backward_vertical_f_code);
	myfprintf(DumpFile_p, "intra_dc_precision = %d\n", PPS_p->PictureParameters.intra_dc_precision);
	myfprintf(DumpFile_p, "picture_structure = %d\n", PPS_p->PictureParameters.picture_structure);
  myfprintf(DumpFile_p, "top_field_first = %d\n", ((PPS_p->PictureParameters.mpeg_decoding_flags) & MPEG_DECODING_FLAGS_TOP_FIELD_FIRST));
	myfprintf(DumpFile_p, "frame_pred_frame_dct = %d\n", ((PPS_p->PictureParameters.mpeg_decoding_flags) & MPEG_DECODING_FLAGS_FRAME_PRED_FRAME_DCT) >> 1);
	myfprintf(DumpFile_p, "concealment_motion_vectors = %d\n", ((PPS_p->PictureParameters.mpeg_decoding_flags) & MPEG_DECODING_FLAGS_CONCEALMENT_MOTION_VECTORS) >> 2);
  myfprintf(DumpFile_p, "q_scale_type = %d\n", ((PPS_p->PictureParameters.mpeg_decoding_flags) & MPEG_DECODING_FLAGS_Q_SCALE_TYPE) >> 3);
	myfprintf(DumpFile_p, "intra_vlc_format = %d\n", ((PPS_p->PictureParameters.mpeg_decoding_flags) & MPEG_DECODING_FLAGS_INTRA_VLC_FORMAT) >> 4);
	myfprintf(DumpFile_p, "alternate_scan = %d\n", ((PPS_p->PictureParameters.mpeg_decoding_flags)& MPEG_DECODING_FLAGS_ALTERNATE_SCAN) >> 5);
  myfprintf(DumpFile_p, "deblocking_filter_disable = %d\n", ((PPS_p->PictureParameters.mpeg_decoding_flags)& MPEG_DECODING_FLAGS_DEBLOCKING_FILTER_ENABLE) >> 6);
}
/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
MME_ERROR MPEG2_HDM_DumpCommand(MME_Command_t* CmdInfo_p)
{
    MME_ERROR Error;
    MPEG2_TransformParam_t *picture_decode_struct;
    MPEG2_SetGlobalParamSequence_t *global_struct;

    Error = MME_SUCCESS;

    switch (CmdInfo_p->CmdCode)
    {
        case MME_SET_GLOBAL_TRANSFORM_PARAMS:
            global_struct = (MPEG2_SetGlobalParamSequence_t *)(CmdInfo_p->Param_p);
			      GlobalParamCounter++;
			      MPEG2_DumpSPS(global_struct);
            break;

        case MME_TRANSFORM:

   			picture_decode_struct= (MPEG2_TransformParam_t *)(CmdInfo_p->Param_p);

            TransformParamCounter += 1;
            myfprintf(DumpFile_p, "-----------------------\n");
            myfprintf(DumpFile_p, "Picture Parameters #%d\n", TransformParamCounter);
            myfprintf(DumpFile_p, "-----------------------\n");

            MPEG2_DumpPPS(picture_decode_struct);

            break;

        default :
            myfprintf(DumpFile_p, "*** MPEG2_HDM_DumpCommand: unknown CmdCode '%d' ***\n", CmdInfo_p->CmdCode);
			Error = MME_INVALID_ARGUMENT;
            break;
    }

    return Error;
}

/* C++ support */
#ifdef __cplusplus
}
#endif

