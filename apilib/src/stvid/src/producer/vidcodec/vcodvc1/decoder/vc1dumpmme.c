/*******************************************************************************

File name   : vc1dumpmme.c

Description : ????

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
02/09/2005         Creation                                         Laurent Delaporte

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>  /* va_start */
#include <stdarg.h> /* va_start */
#include <string.h> /* memset */
#endif

#include "vc1dumpmme.h"
#include "vc1decode.h"
#include "testtool.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Public variables --------------------------------------------------------- */

/* Private Defines ---------------------------------------------------------- */

#define DUMP_FILE_NAME  "../../vc1dump.txt"
#define CONFIG_PATTERN      "dump_mme"
#define CONFIG_PATTERN_ALL  "dump_mme_all"

#define DEC_TIME_DUMP_FILE_NAME  "../../dump_dec_time.txt"
/*#define MAX_DEC_TIME 1000 */
#ifdef ST_OS21
    #define FORMAT_SPEC_OSCLOCK "ll"
    #define OSCLOCK_T_MILLE     1000LL
    #define OSCLOCK_T_MILLION   1000000LL
#else
    #define FORMAT_SPEC_OSCLOCK ""
    #define OSCLOCK_T_MILLE     1000
    #define OSCLOCK_T_MILLION   1000000
#endif

/* Private Defines ---------------------------------------------------------- */

#define CONFIG_FILE_NAME            "config_dump.txt"
#define NO_DUMP_VALUE               0xffffffff

/* Private Types ------------------------------------------------------------ */

/* Private Variables (static) ----------------------------------------------- */
#if !defined ST_OSLINUX
static DumpFile_s DumpFile;
static DumpFile_s* DumpFile_p = &DumpFile;

static unsigned int Counter = 0;
#endif

/* Indicate if all the VC9_TransformParam_t fields shall*/
/* be dumped, even those who are implementation dependant.*/

#ifdef STVID_VALID_DEC_TIME
#define MAX_DEC_TIME 15000
static FILE* DEC_Time_DumpFile_p = NULL;
static int DEC_Time_Counter = 0;
typedef struct DEC_Time_Table_s
{
  osclock_t                        DEC_Time;
  U32                              BitBufferSize;
} DEC_Time_Table_t;
static DEC_Time_Table_t DEC_Time_Table[MAX_DEC_TIME];

extern U32 ValidDisplayLockedCounter[MAX_DEC_TIME];

#endif


/* Private Function Prototypes ---------------------------------------------- */
#if !defined ST_OSLINUX
static int myfprintf(FILE *stream, const char *format, ...);
#endif

static void VC9_DumpEntryPoint(VC9_EntryPointParam_t* EntryPoint_p);
static void ReadDumpParams(
    char* Pattern_p,
    unsigned int* CounterBegin_p,
    unsigned int* CounterEnd_p,
    unsigned int* FlushFlag_p);

static ST_ErrorCode_t VVID_DEC_TimeDump_Term(void);
static ST_ErrorCode_t VVID_DEC_TimeDump_Init(char * filename);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
void DumpFile_fopen(DumpFile_s* DumpFile_p, char* FileName_p, char* Mode_p, char* Pattern_p, unsigned int* Counter_p)
{
    /*ReadDumpParams(Pattern_p, &DumpFile_p->CounterBegin, &DumpFile_p->CounterEnd, &DumpFile_p->FlushFlag);*/
#ifdef ST_OSLINUX
    return ;
#else

    DumpFile_p->CounterBegin = 0;
    DumpFile_p->CounterEnd = 0xFFFF;
    if ((DumpFile_p->CounterBegin != NO_DUMP_VALUE) && (DumpFile_p->CounterEnd != NO_DUMP_VALUE))
    {
        DumpFile_p->File_p = fopen(FileName_p, Mode_p);
        if (DumpFile_p->File_p == NULL)
        {
            printf("*** fopen error on file '%s' ***\n", FileName_p);
            exit( -1 );
        }
    }
    else
    {
        DumpFile_p->File_p = NULL;
    }

    DumpFile_p->Counter_p = Counter_p;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
void DumpFile_fclose(DumpFile_s* DumpFile_p)
{
#ifdef ST_OSLINUX
    return ;
#else
    if (DumpFile_p->File_p)
    {
        fclose(DumpFile_p->File_p);
        DumpFile_p->File_p = NULL;
    }
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
int DumpFile_fprintf(DumpFile_s* DumpFile_p, const char *format, ...)
{
#ifdef ST_OSLINUX
    return -1;
#else

    int ret = 0;

    if (DumpFile_check(DumpFile_p))
    {
        va_list argptr;

        va_start(argptr, format);     /* Initialize variable arguments. */

        ret = vfprintf(DumpFile_p->File_p, format, argptr);

        va_end(argptr);

        if (DumpFile_p->FlushFlag)
        {
            fflush(DumpFile_p->File_p);
        }
    }

    return ret;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
#if !defined (ST_OSLINUX)

int DumpFile_vfprintf(DumpFile_s* DumpFile_p, const char *format, va_list argptr)
{

    int ret = 0;

    if (DumpFile_check(DumpFile_p))
    {
        ret = vfprintf(DumpFile_p->File_p, format, argptr);

        if (DumpFile_p->FlushFlag)
        {
            fflush(DumpFile_p->File_p);
        }
    }

    return ret;
}
#endif

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
extern long DumpFile_fwrite(const void *buffer, long size, long count, DumpFile_s *DumpFile_p)
{
#ifdef ST_OSLINUX
    return -1;
#else
    long ret = 0;

    if (DumpFile_check(DumpFile_p))
    {
        ret = fwrite(buffer, size, count, DumpFile_p->File_p);

        if (DumpFile_p->FlushFlag)
        {
            fflush(DumpFile_p->File_p);
        }
    }

    return ret;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : 0 if dump isn't done
 * *******************************************************************************/
int DumpFile_check(DumpFile_s* DumpFile_p)
{
#ifdef ST_OSLINUX
    return -1;
#else
    if (DumpFile_p->File_p != NULL)
    {
        unsigned int CounterValue = *(DumpFile_p->Counter_p);

        if ((CounterValue >= DumpFile_p->CounterBegin) && (CounterValue <= DumpFile_p->CounterEnd))
        {
            return 1;
        }
    }

    return 0;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void ReadDumpParams(
    char* Pattern_p,
    unsigned int* CounterBegin_p,
    unsigned int* CounterEnd_p,
    unsigned int* FlushFlag_p)
{
#ifdef ST_OSLINUX
    return ;
#else

#define LINE_LENGTH 256

    FILE* ConfigFile_p;
    char Line[LINE_LENGTH];
    char FullPattern[LINE_LENGTH];

    /* Default value: no dump */
    *CounterBegin_p = *CounterEnd_p = NO_DUMP_VALUE;

    ConfigFile_p = fopen(CONFIG_FILE_NAME, "r");
    if (ConfigFile_p != NULL)
    {
        strcpy(FullPattern, Pattern_p);
        strcat(FullPattern, ": from %d to %d ; flush=%d");

        /* Read the config file, line by line */
        while (fgets(Line, LINE_LENGTH, ConfigFile_p) != NULL)
        {
            /* Search for a line related to the current file */
            if (sscanf(Line, FullPattern, CounterBegin_p, CounterEnd_p, FlushFlag_p) == 3)
            {
                break;
            }
        }

        fclose(ConfigFile_p);
    }
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
#if !defined ST_OSLINUX
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
#endif
/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void DumpBuffersAddress(VC9_DecodedBufferAddress_t BufferAddresses)
{
/*
    myfprintf(DumpFile_p, "   Frame buffer Adresses\n");
    myfprintf(DumpFile_p, "     Luma             = 0x%08x\n", BufferAddresses.Luma_p);
    myfprintf(DumpFile_p, "     Chroma           = 0x%08x\n", BufferAddresses.Chroma_p);
    myfprintf(DumpFile_p, "     Luma Decimated   = 0x%08x\n", BufferAddresses.LumaDecimated_p);
    myfprintf(DumpFile_p, "     Chroma Decimated = 0x%08x\n", BufferAddresses.ChromaDecimated_p);
    myfprintf(DumpFile_p, "     MB Struct        = 0x%08x\n", BufferAddresses.MBStruct_p);
*/
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
MME_ERROR VC1_HDM_Init(void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    MME_ERROR Error;

    Error = MME_SUCCESS;
    Counter = 0;

    DumpFile_fopen(DumpFile_p, DUMP_FILE_NAME, "w", CONFIG_PATTERN, &Counter);
    if (DumpFile_p->File_p == NULL)
    {
        DumpFile_fopen(DumpFile_p, DUMP_FILE_NAME, "w", CONFIG_PATTERN_ALL, &Counter);
    }

    DumpFile_fprintf(DumpFile_p, "VC9_MME_API_VERSION = %s\n", VC9_MME_API_VERSION);

    return Error;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
MME_ERROR VC1_HDM_Term(void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    DumpFile_fclose(DumpFile_p);

    return MME_SUCCESS;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
MME_ERROR VC1_HDM_DumpCommand(MME_Command_t* CmdInfo_p)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    VC9_TransformParam_t* TransformParam_p;
    MME_ERROR Error;
    U32 i;

    Error = MME_SUCCESS;
    Counter += 1;

    switch (CmdInfo_p->CmdCode)
    {
        case MME_TRANSFORM:

            TransformParam_p= (VC9_TransformParam_t *)(CmdInfo_p->Param_p);

            if (!DumpFile_check(DumpFile_p))
            {
                break;;
            }

            DumpFile_fprintf(DumpFile_p, "\nMME_TRANSFORM #%d\n", Counter);
            DumpFile_fprintf(DumpFile_p, "   StructSize = %d\n", CmdInfo_p->StructSize);
            DumpFile_fprintf(DumpFile_p, "   ParamSize = %d\n", CmdInfo_p->ParamSize);

            /* Relevant in all profiles */
        #ifdef DUMP_ALL_FIELDS
            DumpFile_fprintf(DumpFile_p, "   CircularBufferBeginAddr_p = %d\n", TransformParam_p->CircularBufferBeginAddr_p);
            DumpFile_fprintf(DumpFile_p, "   CircularBufferEndAddr_p = %d\n", TransformParam_p->CircularBufferEndAddr_p);

            DumpFile_fprintf(DumpFile_p, "   PictureStartAddrCompressedBuffer_p = %d\n", TransformParam_p->PictureStartAddrCompressedBuffer_p);
            DumpFile_fprintf(DumpFile_p, "   PictureStopAddrCompressedBuffer_p = %d\n", TransformParam_p->PictureStopAddrCompressedBuffer_p);

            DumpFile_fprintf(DumpFile_p, "   BackwardReferenceLuma_p = %d\n", TransformParam_p->RefPicListAddress.BackwardReferenceLuma_p);
            DumpFile_fprintf(DumpFile_p, "   BackwardReferenceChroma_p = %d\n", TransformParam_p->RefPicListAddress.BackwardReferenceChroma_p);
            DumpFile_fprintf(DumpFile_p, "   BackwardReferenceMBStruct_p = %d\n", TransformParam_p->RefPicListAddress.BackwardReferenceMBStruct_p);
            DumpFile_fprintf(DumpFile_p, "   ForwardReferenceLuma_p = %d\n", TransformParam_p->RefPicListAddress.ForwardReferenceLuma_p);
            DumpFile_fprintf(DumpFile_p, "   ForwardReferenceChroma_p = %d\n", TransformParam_p->RefPicListAddress.ForwardReferenceChroma_p);
        #else
            DumpFile_fprintf(DumpFile_p, "   CompressedBufferSize = %d\n", TransformParam_p->PictureStopAddrCompressedBuffer_p - TransformParam_p->PictureStartAddrCompressedBuffer_p);
        #endif /* DUMP_ALL_FIELDS */
            DumpFile_fprintf(DumpFile_p, "   BackwardReferencePictureSyntax = %d\n", TransformParam_p->RefPicListAddress.BackwardReferencePictureSyntax);
            DumpFile_fprintf(DumpFile_p, "   ForwardReferencePictureSyntax = %d\n", TransformParam_p->RefPicListAddress.ForwardReferencePictureSyntax);
            DumpFile_fprintf(DumpFile_p, "   MainAuxEnable = %d\n", TransformParam_p->MainAuxEnable);
            DumpFile_fprintf(DumpFile_p, "   HorizontalDecimationFactor = %d\n", TransformParam_p->HorizontalDecimationFactor);
            DumpFile_fprintf(DumpFile_p, "   VerticalDecimationFactor = %d\n", TransformParam_p->VerticalDecimationFactor);
            DumpFile_fprintf(DumpFile_p, "   DecodingMode = %d\n", TransformParam_p->DecodingMode);
            DumpFile_fprintf(DumpFile_p, "   AebrFlag = %d\n", TransformParam_p->AebrFlag);
            DumpFile_fprintf(DumpFile_p, "   Profile = %d\n", TransformParam_p->Profile);
            DumpFile_fprintf(DumpFile_p, "   PictureSyntax = %d\n", TransformParam_p->PictureSyntax);
            DumpFile_fprintf(DumpFile_p, "   PictureType = %d\n", TransformParam_p->PictureType);
            DumpFile_fprintf(DumpFile_p, "   FinterpFlag = %d\n", TransformParam_p->FinterpFlag);
            DumpFile_fprintf(DumpFile_p, "   FrameHorizSize = %d\n", TransformParam_p->FrameHorizSize);
            DumpFile_fprintf(DumpFile_p, "   FrameVertSize = %d\n", TransformParam_p->FrameVertSize);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_ForwTop_1 = %d\n", TransformParam_p->IntensityComp.ForwTop_1);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_ForwTop_2 = %d\n", TransformParam_p->IntensityComp.ForwTop_2);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_ForwBot_1 = %d\n", TransformParam_p->IntensityComp.ForwBot_1);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_ForwBot_2 = %d\n", TransformParam_p->IntensityComp.ForwBot_2);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_BackTop_1 = %d\n", TransformParam_p->IntensityComp.BackTop_1);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_BackTop_2 = %d\n", TransformParam_p->IntensityComp.BackTop_2);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_BackBot_1 = %d\n", TransformParam_p->IntensityComp.BackBot_1);
			DumpFile_fprintf(DumpFile_p, "   IntensityComp_BackBot_2 = %d\n", TransformParam_p->IntensityComp.BackBot_2);
            DumpFile_fprintf(DumpFile_p, "   RndCtrl = %d\n", TransformParam_p->RndCtrl);
            DumpFile_fprintf(DumpFile_p, "   NumeratorBFraction  = %d\n", TransformParam_p->NumeratorBFraction );
            DumpFile_fprintf(DumpFile_p, "   DenominatorBFraction   = %d\n", TransformParam_p->DenominatorBFraction  );

            /* Relevant only in simple/main profile */
            DumpFile_fprintf(DumpFile_p, "   MultiresFlag = %d\n", TransformParam_p->MultiresFlag);
            DumpFile_fprintf(DumpFile_p, "   HalfWidthFlag = %d\n", TransformParam_p->HalfWidthFlag);
            DumpFile_fprintf(DumpFile_p, "   HalfHeightFlag = %d\n", TransformParam_p->HalfHeightFlag);
            DumpFile_fprintf(DumpFile_p, "   SyncmarkerFlag = %d\n", TransformParam_p->SyncmarkerFlag);
            DumpFile_fprintf(DumpFile_p, "   RangeredFlag = %d\n", TransformParam_p->RangeredFlag);
            DumpFile_fprintf(DumpFile_p, "   Maxbframes = %d\n", TransformParam_p->Maxbframes);

            /* Relevant only in advance profile */
            DumpFile_fprintf(DumpFile_p, "   PostprocFlag = %d\n", TransformParam_p->PostprocFlag);
            DumpFile_fprintf(DumpFile_p, "   PulldownFlag = %d\n", TransformParam_p->PulldownFlag);
            DumpFile_fprintf(DumpFile_p, "   InterlaceFlag = %d\n", TransformParam_p->InterlaceFlag);
            DumpFile_fprintf(DumpFile_p, "   TfcntrFlag = %d\n", TransformParam_p->TfcntrFlag);
            DumpFile_fprintf(DumpFile_p, "   SliceCount = %d\n", TransformParam_p->StartCodes.SliceCount);
            for ( i = 0 ; i < TransformParam_p->StartCodes.SliceCount ; i++)
            {
                DumpFile_fprintf(DumpFile_p, "   SLICE %d\n", i+1);
        #ifdef DUMP_ALL_FIELDS
                DumpFile_fprintf(DumpFile_p, "   SliceStartAddrCompressedBuffer_p = %d\n", TransformParam_p->StartCodes.SliceArray[i].SliceStartAddrCompressedBuffer_p);
        #else
                DumpFile_fprintf(DumpFile_p, "   SliceStartAddrOffset = %d\n", TransformParam_p->StartCodes.SliceArray[i].SliceStartAddrCompressedBuffer_p - TransformParam_p->PictureStartAddrCompressedBuffer_p);
        #endif
                DumpFile_fprintf(DumpFile_p, "   SliceAddress = %d\n", TransformParam_p->StartCodes.SliceArray[i].SliceAddress);
            }
			DumpFile_fprintf(DumpFile_p, "   BackwardRefdist = %d\n", TransformParam_p->BackwardRefdist);
            DumpFile_fprintf(DumpFile_p, "   FramePictureLayerSize  = %d\n", TransformParam_p->FramePictureLayerSize );
            DumpFile_fprintf(DumpFile_p, "   TffFlag   = %d\n", TransformParam_p->TffFlag  );
            DumpFile_fprintf(DumpFile_p, "   SecondFieldFlag   = %d\n", TransformParam_p->SecondFieldFlag  );
            DumpFile_fprintf(DumpFile_p, "   RefDist   = %d\n", TransformParam_p->RefDist  );

            DumpFile_fprintf(DumpFile_p, "   ENTRY_POINT\n");
            VC9_DumpEntryPoint(&(TransformParam_p->EntryPoint));
            break;

        default :
            DumpFile_fprintf(DumpFile_p, "*** HDM_DumpCommand: unknown CmdCode '%d' ***\n", CmdInfo_p->CmdCode);
            Error = MME_INVALID_ARGUMENT;
            break;
    }

    return Error;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
static void VC9_DumpEntryPoint(
    VC9_EntryPointParam_t* EntryPoint_p)
{
#ifdef ST_OSLINUX
    return ;
#else
    /* Relevant in all profiles */
    DumpFile_fprintf(DumpFile_p, "   LoopfilterFlag = %d\n", EntryPoint_p->LoopfilterFlag);
    DumpFile_fprintf(DumpFile_p, "   FastuvmcFlag = %d\n", EntryPoint_p->FastuvmcFlag);
    DumpFile_fprintf(DumpFile_p, "   ExtendedmvFlag = %d\n", EntryPoint_p->ExtendedmvFlag);
    DumpFile_fprintf(DumpFile_p, "   Dquant = %d\n", EntryPoint_p->Dquant);
    DumpFile_fprintf(DumpFile_p, "   VstransformFlag = %d\n", EntryPoint_p->VstransformFlag);
    DumpFile_fprintf(DumpFile_p, "   OverlapFlag = %d\n", EntryPoint_p->OverlapFlag);
    DumpFile_fprintf(DumpFile_p, "   Quantizer = %d\n", EntryPoint_p->Quantizer);

    /* Relevant only in advance profile */
    DumpFile_fprintf(DumpFile_p, "   ExtendedDmvFlag = %d\n", EntryPoint_p->ExtendedDmvFlag);
    DumpFile_fprintf(DumpFile_p, "   PanScanFlag = %d\n", EntryPoint_p->PanScanFlag);
    DumpFile_fprintf(DumpFile_p, "   RefdistFlag = %d\n", EntryPoint_p->RefdistFlag);
#endif
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
static ST_ErrorCode_t VVID_DEC_TimeDump_Init(char * filename)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    ST_ErrorCode_t Error;

    Error = ST_NO_ERROR;

    DEC_Time_DumpFile_p = fopen(filename, "w");
    if (DEC_Time_DumpFile_p == NULL)
    {
        printf("*** DEC_Time_Dump: fopen error on file '%s' ***\n", DEC_TIME_DUMP_FILE_NAME);
        Error = ST_ERROR_OPEN_HANDLE;
    }

    return Error;
#endif
}

/*******************************************************************************
 * Name        : VVID_DEC_TimeDump_Term
 * Description : terminates the preprocessor time dump process
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
static ST_ErrorCode_t VVID_DEC_TimeDump_Term(void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    if (DEC_Time_DumpFile_p != NULL)
    {
        (void)fclose(DEC_Time_DumpFile_p);
        DEC_Time_DumpFile_p = NULL;
    }

    return ST_NO_ERROR;
#endif
}


/*******************************************************************************
 * Name        : VVID_VC1DEC_TimeStore_Command
 * Description : Stores the Decode timing information
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
ST_ErrorCode_t VVID_VC1DEC_TimeStore_Command(VC1DecoderPrivateData_t * PrivateData_p)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    ST_ErrorCode_t Error;
    U32 BitBufferSize;

    Error = ST_NO_ERROR;

    if (DEC_Time_Counter < MAX_DEC_TIME - 1)
    {
      DEC_Time_Counter += 1;

#if 0
	  if ((U32)PrivateData_p->DecoderContext.H264PreprocCommand.TransformerPreprocParam.PictureStopAddrBitBuffer_p < (U32)PrivateData_p->DecoderContext.H264PreprocCommand.TransformerPreprocParam.PictureStartAddrBitBuffer_p)
      {
         /* Write pointer has done a loop, not the Read pointer */
         BitBufferSize = (U32)PrivateData_p->BitBufferEndAddress_p
                         - (U32)PrivateData_p->DecoderContext.H264PreprocCommand.TransformerPreprocParam.PictureStartAddrBitBuffer_p
                         + (U32)PrivateData_p->DecoderContext.H264PreprocCommand.TransformerPreprocParam.PictureStopAddrBitBuffer_p
                         - (U32)PrivateData_p->DecoderContext.H264PreprocCommand.TransformerPreprocParam.BitBufferBeginAddress_p
                         + 1;
      }
      else
      {
        /* Normal: start <= read <= write <= top */
        BitBufferSize = (U32)PrivateData_p->DecoderContext.H264PreprocCommand.TransformerPreprocParam.PictureStopAddrBitBuffer_p
                        - (U32)PrivateData_p->DecoderContext.H264PreprocCommand.TransformerPreprocParam.PictureStartAddrBitBuffer_p;
      }

	  DEC_Time_Table[DEC_Time_Counter].BitBufferSize = BitBufferSize;

#endif /* 0 */

      /* Decoder */
      DEC_Time_Table[DEC_Time_Counter].DEC_Time = time_minus(PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobCompletionTime,
                                                    PrivateData_p->DecoderContext.DECODER_DecodingJobResults.DecodingJobStatus.JobSubmissionTime);

	  /* This is defined in the H.264 decoder and set in vid_disp.c*/
      ValidDisplayLockedCounter[DEC_Time_Counter] = 0;
    }
    return Error;
#endif
}/* end VVID_VC1DEC_TimeStore_Command */


/*******************************************************************************
 * Name        : VVID_VC1DEC_TimeDump_Command
 * Description : terminates the decoder time dump process
 * Parameters  : void
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ST_ErrorCode_t
 * *******************************************************************************/
static BOOL VVID_VC1DEC_TimeDump_Command(STTST_Parse_t *pars_p, char *result_sym_p)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    BOOL    RetErr;
    U32 NbPicToDump;

    RetErr = STTST_GetInteger(pars_p, 10 , (S32*)&NbPicToDump );

    if (DEC_Time_Counter < NbPicToDump)
    {
        NbPicToDump = DEC_Time_Counter;
    }
    if (VVID_VC1DEC_TimeDump_Command_function(DEC_TIME_DUMP_FILE_NAME, DEC_Time_Counter))
    {
        return TRUE;
    }
    return FALSE;
#endif
}

BOOL VVID_VC1DEC_TimeDump_Command_function(char * filename, U32 DEC_Time_Counter)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

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

#if 0
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

#endif /* 0*/
      myfprintf(DEC_Time_DumpFile_p, "DEC_TRANSFORM  ST40-DECTime DispLockCount\n");

      for(i=1; i <= DEC_Time_Counter; i++)
      {

#if 0
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

#endif /* 0*/

        myfprintf(DEC_Time_DumpFile_p, "%d %10"FORMAT_SPEC_OSCLOCK"d %d\n",
										i /* DEC_TRANSFORM */,
                                        DEC_Time_Table[i].DEC_Time*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                        ValidDisplayLockedCounter[i]
                                        );

      }
      VVID_DEC_TimeDump_Term();
      return FALSE;
    }
    else
    {
      return TRUE;
    }
#endif
}

/* TODO (LD) : add this testtool command */
/*-------------------------------------------------------------------------
 * Function : VIDVC1DecodeTime_RegisterCmd
 *            Register video command (1 for each API function)
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VIDVC1DecodeTime_RegisterCmd (void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    BOOL RetErr;
    U32  i;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand("VVID_VC1_DUMP_DEC_TIME",    VVID_VC1DEC_TimeDump_Command, "Dumps Preproc time over pictures");

    if (RetErr)
    {
        STTST_Print(("VIDVC1DecodeTime_RegisterCmd()     : failed !\n" ));
    }
    else
    {
        STTST_Print(("VIDVC1DecodeTime_RegisterCmd()     : ok\n" ));
    }
    return(RetErr ? FALSE : TRUE);
#endif
} /* end VIDVC1DecodeTime_RegisterCmd */
#endif/* STVID_VALID_DEC_TIME */

/* C++ support */
#ifdef __cplusplus
}
#endif

/* End of vc1dumpmme.c */
