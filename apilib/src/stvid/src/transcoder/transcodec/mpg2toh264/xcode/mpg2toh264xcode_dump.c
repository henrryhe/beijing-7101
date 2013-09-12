/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif
#include <stdarg.h> /* va_start */
#include "stavmem.h" /* Virtual memory mapping */

#include "mpg2toh264xcode.h"

#if !defined ST_OSLINUX
#include "sttbx.h"
#endif

#define XCODE_DUMP_DIR "../../xcode_dump"
#define FILENAME_MAX_LENGTH 256
#define OMEGA2_420 0x94

static FILE* DumpFile_p = NULL;
static Counter = 0;

static int myfprintf(FILE *stream, const char *format, ...);

static BOOL SaveAddData(U32 width, U32 height, void * AddDataAddress_p, char * Filename_p)

{
    FILE    *Fp;
    U32     rows=0;
    U32     cols=0;
    U32     DataBytes=0;
    char    Filename[FILENAME_MAX_LENGTH];

    strcpy(Filename, Filename_p);
    strcat(Filename, ".bin");

    DataBytes = (((width+15)/16)*((height+15)/16)) * sizeof(XCode_MPEG2H264_MBDescr_t);

    /* Open files */
    if ((Fp = fopen(Filename, "wb")) == NULL)
    {
        STTBX_Print(("Error opening file %s\n", Filename));
        return FALSE;
    }

    /* Write data to files */
    fwrite(AddDataAddress_p, sizeof(U8), DataBytes, Fp);
    fclose(Fp);
    return FALSE;
}

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
ST_ErrorCode_t XCODE_DumpInit(void)
{
    ST_ErrorCode_t Error;

    Error = ST_NO_ERROR;

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
ST_ErrorCode_t XCODE_DumpTerm(void)
{

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
void XCODE_DumpGlobalParam(MME_Command_t * CmdInfoParam_p)
{
    /* Open txt files */
    char GlobalFileName[256];
    ST_ErrorCode_t Error;
    
    Xcode_MPEG2H264_GlobalParams_t * MPEG2H264_GlobalParams_p;
    Xcode_GlobalParams_t * Xcode_GlobalParams_p;
    sprintf(GlobalFileName, "%s/pic%d_global.txt", XCODE_DUMP_DIR, Counter);
    DumpFile_p = fopen(GlobalFileName, "w");
    if (DumpFile_p == NULL)
    {
        printf("*** XCODE_DumpGlobalParam: fopen error on file '%s' ***\n", GlobalFileName);
        Error = ST_ERROR_OPEN_HANDLE;
    }

    MPEG2H264_GlobalParams_p = (Xcode_MPEG2H264_GlobalParams_t *)CmdInfoParam_p->DataBuffers_p[0]->ScatterPages_p->Page_p;
    Xcode_GlobalParams_p = (Xcode_GlobalParams_t *)CmdInfoParam_p->Param_p;

    myfprintf(DumpFile_p, "MME Command (Global) for picture %d:\n", Counter);
    myfprintf(DumpFile_p, "------------------------------------\n");
    myfprintf(DumpFile_p, "StructSize : %d\n", CmdInfoParam_p->StructSize);
    myfprintf(DumpFile_p, "CmdCode : %d\n", CmdInfoParam_p->CmdCode);
    myfprintf(DumpFile_p, "CmdEnd  : %d\n", CmdInfoParam_p->CmdEnd);
    myfprintf(DumpFile_p, "DueTime : %d\n", CmdInfoParam_p->DueTime);
    myfprintf(DumpFile_p, "NumberInputBuffers : %d\n", CmdInfoParam_p->NumberInputBuffers);
    myfprintf(DumpFile_p, "NumberOutputBuffers : %d\n", CmdInfoParam_p->NumberOutputBuffers);
    /* Specific parameters are passed through DataBuffer[0] */
    myfprintf(DumpFile_p, "DataBuffers_p[0]->StructSize :%d\n", CmdInfoParam_p->DataBuffers_p[0]->StructSize);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->UserData_p : %d\n", (U32)CmdInfoParam_p->DataBuffers_p[0]->UserData_p);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->Flags : %d\n", CmdInfoParam_p->DataBuffers_p[0]->Flags);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->StreamNumber : %d\n", CmdInfoParam_p->DataBuffers_p[0]->StreamNumber);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->NumberOfScatterPages : %d\n", CmdInfoParam_p->DataBuffers_p[0]->NumberOfScatterPages);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->StartOffset : %d\n", CmdInfoParam_p->DataBuffers_p[0]->StartOffset);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->ScatterPages_p->BytesUsed : %d\n", CmdInfoParam_p->DataBuffers_p[0]->ScatterPages_p->BytesUsed);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->ScatterPages_p->FlagsIn : %d\n", CmdInfoParam_p->DataBuffers_p[0]->ScatterPages_p->FlagsIn);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->ScatterPages_p->FlagsOut : %d\n", CmdInfoParam_p->DataBuffers_p[0]->ScatterPages_p->FlagsOut);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->TotalSize : %d\n", CmdInfoParam_p->DataBuffers_p[0]->TotalSize);
    myfprintf(DumpFile_p, "CmdStatus.AdditionalInfoSize : %d\n", CmdInfoParam_p->CmdStatus.AdditionalInfoSize);
    myfprintf(DumpFile_p, "CmdStatus.AdditionalInfo_p : %d\n", (U32)CmdInfoParam_p->CmdStatus.AdditionalInfo_p);
    myfprintf(DumpFile_p, "\n");
    myfprintf(DumpFile_p, "MPEG2H264_GlobalParams:\n");
    myfprintf(DumpFile_p, "-----------------------\n");
    myfprintf(DumpFile_p, "profile_idc : %d\n", MPEG2H264_GlobalParams_p->profile_idc);
    myfprintf(DumpFile_p, "EntropyCodingMode : %d\n", MPEG2H264_GlobalParams_p->EntropyCodingMode);
    myfprintf(DumpFile_p, "\n");
    myfprintf(DumpFile_p, "MPEG2H264_GlobalParams:\n");
    myfprintf(DumpFile_p, "-----------------------\n");
    myfprintf(DumpFile_p, "Size: %d\n", CmdInfoParam_p->ParamSize);
    myfprintf(DumpFile_p, "FrameRate : %d\n", Xcode_GlobalParams_p->FrameRate);
    myfprintf(DumpFile_p, "PictureWidth : %d\n", Xcode_GlobalParams_p->PictureWidth);
    myfprintf(DumpFile_p, "PictureHeight : %d\n", Xcode_GlobalParams_p->PictureHeight);
    myfprintf(DumpFile_p, "DecimationFactors : %d\n", Xcode_GlobalParams_p->DecimationFactors);
    myfprintf(DumpFile_p, "ChromaFormat : %d\n", Xcode_GlobalParams_p->ChromaFormat); /* No other choice! */
    myfprintf(DumpFile_p, "RCOutputBitrateValue : %d\n", Xcode_GlobalParams_p->RCOutputBitrateValue);
    myfprintf(DumpFile_p, "RateControlMode : %d\n", Xcode_GlobalParams_p->RateControlMode);
    myfprintf(DumpFile_p, "HalfPictureFlag : %d\n", Xcode_GlobalParams_p->HalfPictureFlag);
    myfprintf(DumpFile_p, "AspectRatio : %d\n", Xcode_GlobalParams_p->AspectRatio);
    myfprintf(DumpFile_p, "ProgressiveSequence : %d\n", Xcode_GlobalParams_p->ProgressiveSequence);
    myfprintf(DumpFile_p, "VideoFormat : %d\n", Xcode_GlobalParams_p->VideoFormat);
    myfprintf(DumpFile_p, "ColourPrimaries : %d\n", Xcode_GlobalParams_p->ColourPrimaries);
    myfprintf(DumpFile_p, "TransferCharacteristics : %d\n", Xcode_GlobalParams_p->TransferCharacteristics);
    myfprintf(DumpFile_p, "MatrixCoefficients : %d\n", Xcode_GlobalParams_p->MatrixCoefficients);

    (void)fclose(DumpFile_p);
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
void XCODE_DumpTransformParam(Xcode_GlobalParams_t * Xcode_GlobalParams_p, MME_Command_t * CmdInfoFmw_p)
{
    /* Open txt files */
    char TransformFileName[256];
    char InputBufferGAMFileName[256];
    char InputBufferMBDescrFileName[256];
    ST_ErrorCode_t Error;
    Xcode_MPEG2H264_TransformParams_t * MPEG2H264_TransformParams_p;
    Xcode_TransformParams_t * Xcode_TransformParams_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32 Width;
    U32 Height;
    
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

    sprintf(TransformFileName, "%s/pic%03d_transform.txt", XCODE_DUMP_DIR, Counter);
    DumpFile_p = fopen(TransformFileName, "w");
    if (DumpFile_p == NULL)
    {
        printf("*** XCODE_DumpTransformParam: fopen error on file '%s' ***\n", TransformFileName);
        Error = ST_ERROR_OPEN_HANDLE;
    }

    sprintf(InputBufferGAMFileName, "%s/pic%03d_input", XCODE_DUMP_DIR, Counter);
    sprintf(InputBufferMBDescrFileName, "%s/pic%03d_mbdecr", XCODE_DUMP_DIR, Counter);

    MPEG2H264_TransformParams_p = (Xcode_MPEG2H264_TransformParams_t *)CmdInfoFmw_p->DataBuffers_p[0]->ScatterPages_p->Page_p;
    Xcode_TransformParams_p = (Xcode_TransformParams_t *)CmdInfoFmw_p->Param_p;
    
    myfprintf(DumpFile_p, "MME Command (Transform) for picture %d:\n", Counter);
    myfprintf(DumpFile_p, "---------------------------------------\n");
    myfprintf(DumpFile_p, "StructSize : %d\n", CmdInfoFmw_p->StructSize);
    myfprintf(DumpFile_p, "CmdCode : %d\n", CmdInfoFmw_p->CmdCode);
    myfprintf(DumpFile_p, "CmdEnd  : %d\n", CmdInfoFmw_p->CmdEnd);
    myfprintf(DumpFile_p, "DueTime : %d\n", CmdInfoFmw_p->DueTime);
    myfprintf(DumpFile_p, "NumberInputBuffers : %d\n", CmdInfoFmw_p->NumberInputBuffers);
    myfprintf(DumpFile_p, "NumberOutputBuffers : %d\n", CmdInfoFmw_p->NumberOutputBuffers);
    /* Specific parameters are passed through DataBuffer[0] */
    myfprintf(DumpFile_p, "DataBuffers_p[0]->StructSize :%d\n", CmdInfoFmw_p->DataBuffers_p[0]->StructSize);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->UserData_p : %d\n", (U32)CmdInfoFmw_p->DataBuffers_p[0]->UserData_p);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->Flags : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->Flags);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->StreamNumber : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->StreamNumber);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->NumberOfScatterPages : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->NumberOfScatterPages);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->StartOffset : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->StartOffset);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->ScatterPages_p->BytesUsed : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->ScatterPages_p->BytesUsed);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->ScatterPages_p->FlagsIn : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->ScatterPages_p->FlagsIn);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->ScatterPages_p->FlagsOut : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->ScatterPages_p->FlagsOut);
    myfprintf(DumpFile_p, "DataBuffers_p[0]->TotalSize : %d\n", CmdInfoFmw_p->DataBuffers_p[0]->TotalSize);
    myfprintf(DumpFile_p, "CmdStatus.AdditionalInfoSize : %d\n", CmdInfoFmw_p->CmdStatus.AdditionalInfoSize);
    myfprintf(DumpFile_p, "CmdStatus.AdditionalInfo_p : %d\n", (U32)CmdInfoFmw_p->CmdStatus.AdditionalInfo_p);
    myfprintf(DumpFile_p, "\n");
    myfprintf(DumpFile_p, "MPEG2H264_TransformParams:\n");
    myfprintf(DumpFile_p, "--------------------------\n");
    /* Transform parameters */
  	myfprintf(DumpFile_p, "EndOfBitstreamFlag : %d\n", Xcode_TransformParams_p->EndOfBitstreamFlag);
  	myfprintf(DumpFile_p, "TemporalReference : %d\n", Xcode_TransformParams_p->TemporalReference);
  	myfprintf(DumpFile_p, "PictureCodingType : %d\n", Xcode_TransformParams_p->PictureCodingType);
  	myfprintf(DumpFile_p, "PictureStructure  : %d\n", Xcode_TransformParams_p->PictureStructure);
  	myfprintf(DumpFile_p, "PictureScanType   : %d\n", Xcode_TransformParams_p->PictureScanType);
  	myfprintf(DumpFile_p, "PictureFlags      : %d\n", Xcode_TransformParams_p->PictureFlags);
  	myfprintf(DumpFile_p, "EncodingMode      : %d\n", Xcode_TransformParams_p->EncodingMode);

    /* InputBuffer.Luma and Chroma */ 
    /* Compute Width, Height */
    if (Xcode_GlobalParams_p->DecimationFactors & STVID_DECIMATION_FACTOR_H2)
    {
        Width = Xcode_GlobalParams_p->PictureWidth/2;
    }
    else if (Xcode_GlobalParams_p->DecimationFactors & STVID_DECIMATION_FACTOR_H4)
    {
        Width = Xcode_GlobalParams_p->PictureWidth/4;        
    }
    else
    {
        Width = Xcode_GlobalParams_p->PictureWidth;
    }
    if (Xcode_GlobalParams_p->DecimationFactors & STVID_DECIMATION_FACTOR_V2)
    {
        Height = Xcode_GlobalParams_p->PictureHeight/2;
    }
    else
    {
        Height = Xcode_GlobalParams_p->PictureHeight; 
    }

    Width = 16 * ((Width+15)/16);
    Height = 16 * ((Height+15)/16);
    vidprod_SavePicture(OMEGA2_420, Width, Height,
                                    STAVMEM_DeviceToCPU(Xcode_TransformParams_p->InputBuffer.Luma_p, &VirtualMapping) /* LumaAddress */,
                                    STAVMEM_DeviceToCPU(Xcode_TransformParams_p->InputBuffer.Chroma_p, &VirtualMapping) /* ChromaAddress */,
                                    InputBufferGAMFileName);

    SaveAddData(Xcode_GlobalParams_p->PictureWidth,
                Xcode_GlobalParams_p->PictureHeight,
                STAVMEM_DeviceToCPU(Xcode_TransformParams_p->InputBuffer.Param_p, &VirtualMapping) ,
                InputBufferMBDescrFileName);

    (void)fclose(DumpFile_p);

    Counter++;
}
