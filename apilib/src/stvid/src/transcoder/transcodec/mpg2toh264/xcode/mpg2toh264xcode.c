/*!
 *******************************************************************************
 * \file mpg2toh264xcode.c
 *
 * \brief Transcodec MPEG2 to H264 xcode decoder functions
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "mpg2toh264xcode.h"
#include "testtool.h"

#if !defined ST_OSLINUX
#include "sttbx.h"
#endif

/* Static functions prototypes ---------------------------------------------- */
static BOOL GlobalSent = FALSE;
/* Constants ---------------------------------------------------------------- */
/* Xcode decoder */
const XCODE_FunctionsTable_t XCODE_MPG2TOH264Functions =
{
    mpg2toh264Xcode_Init,
    mpg2toh264Xcode_TranscodePicture,
    mpg2toh264Xcode_Term
};

/* Functions ---------------------------------------------------------------- */
static void mpg2toh264_TransformerCallback (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData);
static XCodeDecimationFactor_t mpg2toh264_BuildDecimationFactor(STVID_DecimationFactor_t const TargetDecimationFactor);
static Xcode_RateControl_t mpg2toh264_BuildRateControlMode(STVID_RateControlMode_t const TargetRateControlMode);
static BOOL mpg2toh264_BuildHalfPictureFlag(U32 DecimationFactors, XCODER_SpeedMode_t SpeedMode);
static Xcode_DisplayAspectRatio_t mpg2toh264_BuildAspectRatio(STVID_DisplayAspectRatio_t Aspect);
static Xcode_PictureCodingType_t mpg2toh264_BuildPictureCodingType(STVID_MPEGFrame_t MPEGFrame);
static Xcode_PictureStructure_t mpg2toh264_BuildPictureStructure(STVID_PictureStructure_t TargetPictureStructure);
static Xcode_ScanType_t mpg2toh264_BuildPictureScanType(STGXOBJ_ScanType_t TargetScanType);
static U32 mpg2toh264_BuildPictureFlags(STVID_PictureStructure_t TargetPictureStructure, VIDCOM_PictureGenericData_t PictureGenericData);
static Xcode_EncodingMode_t mpg2toh264_BuildEncodingMode(STVID_PictureStructure_t PictureStructure);
static Xcode_H264_EntropyEncodingMode_t mpg2toh264_BuildEntropyCodingMode(XCODER_SpeedMode_t SpeedMode);

/* Xcode decoder */
/*******************************************************************************
Name        : mpg2toh264_TransformerCallback
Description :
Parameters  : Event
 *            CallbackData
 *            UserData
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void mpg2toh264_TransformerCallback (MME_Event_t Event, MME_Command_t * CallbackData, void *UserData)
{ 
    MME_Command_t                         * Command_p;
    XCODE_Properties_t *                    XCODE_Properties_p;
    mpg2toh264xcode_PrivateData_t         * XCODE_Data_p;
    XCode_TransformStatusAdditionalInfo_t * XCode_TransformStatusAdditionalInfo_p;
    
    UNUSED_PARAMETER(Event);
    
    XCODE_Properties_p  = (XCODE_Properties_t *) UserData;
    XCODE_Data_p   = (mpg2toh264xcode_PrivateData_t *) XCODE_Properties_p->PrivateData_p;
    Command_p       = (MME_Command_t *) CallbackData;
    XCode_TransformStatusAdditionalInfo_p = (XCode_TransformStatusAdditionalInfo_t *) Command_p->CmdStatus.AdditionalInfo_p;

    if (Command_p->CmdCode == MME_SET_GLOBAL_TRANSFORM_PARAMS)
    {
        /* We return from a set global transform params */
        semaphore_signal(XCODE_Data_p->GlobalTransformParamsCompletedSemaphore_p);
    }
    else
    {
        /* inform the transcode controller of the terminated command */
        XCODE_Data_p->mpg2toh264XcoderContext.XCODER_TranscodingJobResults.ErrorCode = XCODER_NO_ERROR;
    
        XCODE_Data_p->mpg2toh264XcoderContext.XCODER_TranscodingJobResults.TranscodingJobStatus.JobState = XCODER_JOB_COMPLETED;
        XCODE_Data_p->mpg2toh264XcoderContext.XCODER_TranscodingJobResults.TranscodingJobStatus.JobCompletionTime = time_now();

        XCODE_Data_p->mpg2toh264XcoderContext.XCODER_TranscodingJobResults.TranscodingJobStatus.UsedOutputBufferSizeInByte = (XCode_TransformStatusAdditionalInfo_p->UsedOutputBufferSizeInBits + 7) / 8;   
        /* We return from a transform */
        semaphore_signal(XCODE_Data_p->TransformCompletedSemaphore_p);
    }
}

/*******************************************************************************
Name        : mpg2toh264Xcode_GetPictureMaxSizeInByte
Description : 
Parameters  : 
Assumptions :
Limitations :
Returns     : returns the maximum number of bytes for a picture
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Xcode_GetPictureMaxSizeInByte(const XCODE_Handle_t XcodeHandle)
{
#if 0/* for Warnings */
    mpg2toh264xcode_PrivateData_t * XCODE_Data_p;
    XCODE_Properties_t *            XCODE_Properties_p;
#endif
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(XcodeHandle);

    return (ErrorCode);
}

#ifdef XCODE_DEMO
/*******************************************************************************
Name        : mp2toh264_AllocateH264FrameBuffersForDemo
Description : Allocate target frame buffer for Demo only
Parameters  : XcodeHandle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t mp2toh264_AllocateH264FrameBuffersForDemo(const XCODE_Handle_t XcodeHandle)
{
    mpg2toh264xcode_PrivateData_t *         XCODE_Data_p;
    XCODE_Properties_t *                    XCODE_Properties_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    STAVMEM_FreeBlockParams_t               FreeParams;
    STAVMEM_BlockHandle_t                   Hdl;
    void *                                  VirtualAddress;
    BOOL RetErr;
    U32 IHSize;
    U32 IVSize;
    U32 LoopIndex;

    UNUSED_PARAMETER(FreeParams);

    XCODE_Properties_p = (XCODE_Properties_t *) XcodeHandle;
    XCODE_Data_p = (mpg2toh264xcode_PrivateData_t *) XCODE_Properties_p->PrivateData_p;
        
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

    RetErr = STTST_EvaluateInteger("IHSIZE",(S32*)&IHSize,10);
    RetErr = STTST_EvaluateInteger("IVSIZE",(S32*)&IVSize,10);

    AllocBlockParams.PartitionHandle          = XCODE_Properties_p->AvmemPartitionHandle;
    AllocBlockParams.Alignment                = 2048;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    for(LoopIndex = 0; LoopIndex < MAX_RECONBUFFER_IDX; LoopIndex++)
    {
        /* Luma buffer */          
        AllocBlockParams.Size = IHSize * IVSize;
  
        if (STAVMEM_AllocBlock(&AllocBlockParams, &Hdl) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        if (STAVMEM_GetBlockAddress(Hdl, (void **)&VirtualAddress) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        XCODE_Data_p->ReconBuffer[LoopIndex].Luma_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
        XCODE_Data_p->ReconBuffer[LoopIndex].LumaHdl = Hdl;
        
        /* Chroma buffer */          
        AllocBlockParams.Size = IHSize * IVSize / 2;
  
        if (STAVMEM_AllocBlock(&AllocBlockParams, &Hdl) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        if (STAVMEM_GetBlockAddress(Hdl, (void **)&VirtualAddress) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        XCODE_Data_p->ReconBuffer[LoopIndex].Chroma_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
        XCODE_Data_p->ReconBuffer[LoopIndex].ChromaHdl = Hdl;

        /* MBDescr buffer */          
        AllocBlockParams.Size = IHSize * IVSize / 256 * 160;
  
        if (STAVMEM_AllocBlock(&AllocBlockParams, &Hdl) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        if (STAVMEM_GetBlockAddress(Hdl, (void **)&VirtualAddress) != ST_NO_ERROR)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        XCODE_Data_p->ReconBuffer[LoopIndex].MBDescr_p = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
        XCODE_Data_p->ReconBuffer[LoopIndex].MBDescrHdl = Hdl;

    }

#if 1
    XCODE_Data_p->ReconBufferIdx[0] = OLDEST_REF;
    XCODE_Data_p->ReconBufferIdx[1] = NEWEST_REF;
    XCODE_Data_p->ReconBufferIdx[2] = CURRENT_PIC;
#endif   
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : mp2toh264_DeAllocateH264FrameBuffersForDemo
Description : DeAllocate target frame buffer for Demo only
Parameters  : XcodeHandle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t mp2toh264_DeAllocateH264FrameBuffersForDemo(const XCODE_Handle_t XcodeHandle)
{
    mpg2toh264xcode_PrivateData_t *         XCODE_Data_p;
    XCODE_Properties_t *                    XCODE_Properties_p;
    STAVMEM_FreeBlockParams_t               FreeParams;
    U32 LoopIndex;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    
    XCODE_Properties_p = (XCODE_Properties_t *) XcodeHandle;
    XCODE_Data_p = (mpg2toh264xcode_PrivateData_t *) XCODE_Properties_p->PrivateData_p;
        
    FreeParams.PartitionHandle = XCODE_Properties_p->AvmemPartitionHandle;

    for(LoopIndex = 0; LoopIndex < MAX_RECONBUFFER_IDX; LoopIndex++)
    {
        if (XCODE_Data_p->ReconBuffer[LoopIndex].LumaHdl != (STAVMEM_BlockHandle_t)NULL)
        {
            ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(XCODE_Data_p->ReconBuffer[LoopIndex].LumaHdl));
        }
        if (XCODE_Data_p->ReconBuffer[LoopIndex].ChromaHdl != (STAVMEM_BlockHandle_t)NULL)
        {
            ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(XCODE_Data_p->ReconBuffer[LoopIndex].ChromaHdl));
        }
        if (XCODE_Data_p->ReconBuffer[LoopIndex].MBDescrHdl != (STAVMEM_BlockHandle_t)NULL)
        {
            ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(XCODE_Data_p->ReconBuffer[LoopIndex].MBDescrHdl));
        }
    }
    
    return(ErrorCode);
}

#endif /* XCODE_DEMO */

/*******************************************************************************
Name        : mpg2toh264Xcode_Init
Description : mpg2toh264 transcodec initialization
Parameters  : XcodeHandle
 *            InitParams_p
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Xcode_Init(const XCODE_Handle_t XcodeHandle, const XCODE_InitParams_t * const InitParams_p)
{
    mpg2toh264xcode_PrivateData_t *         XCODE_Data_p;
    XCODE_Properties_t *                    XCODE_Properties_p;
    ST_ErrorCode_t                          ErrorCode;
    EMBX_TRANSPORT                          tp;
    EMBX_ERROR                              EMBXError;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32                                     DeviceRegistersBaseAddress;
    void *                                  RegistersBaseAddress_p;
    char                                    TransformerName[MME_MAX_TRANSFORMER_NAME];
    MME_TransformerInitParams_t             MMETransformerInitParams;
    U8                                      i;
    
    ErrorCode = ST_NO_ERROR;
    XCODE_Properties_p = (XCODE_Properties_t *) XcodeHandle;

    if ((XcodeHandle == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate XCODE PrivateData in EMBX shared memory to allow Multicom to perform Address translation */
    EMBXError = EMBX_OpenTransport(MME_VIDEO_TRANSPORT_NAME, &tp);
    if(EMBXError != EMBX_SUCCESS)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    EMBXError = EMBX_Alloc(tp, sizeof(mpg2toh264xcode_PrivateData_t), (EMBX_VOID **)(&(XCODE_Properties_p->PrivateData_p)));
    if(EMBXError != EMBX_SUCCESS)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    XCODE_Data_p = (mpg2toh264xcode_PrivateData_t *) XCODE_Properties_p->PrivateData_p;
    memset(XCODE_Data_p, 0xA5, sizeof(mpg2toh264xcode_PrivateData_t)); /* fill in data with 0xA5 by default */

    XCODE_Data_p->GlobalTransformParamsCompletedSemaphore_p = semaphore_create_fifo(0);
    XCODE_Data_p->TransformCompletedSemaphore_p = semaphore_create_fifo(0);
    
    /* Save init params */
    XCODE_Data_p->TranscoderHandle = InitParams_p->TranscoderHandle;
    
    /* Allocate DataBuffer for Global Param */
    EMBXError = EMBX_Alloc(tp, NUM_MME_MP2TOH264_PARAM_BUFFERS * sizeof(MME_DataBuffer_t *),
                           (EMBX_VOID **)(&(XCODE_Data_p->CmdInfoParam.DataBuffers_p)));
    if(EMBXError != EMBX_SUCCESS)
    {
        return(ST_ERROR_NO_MEMORY);
    }

  	for(i=0; i< NUM_MME_MP2TOH264_PARAM_BUFFERS; i++)
  	{
        EMBXError = EMBX_Alloc(tp, sizeof(MME_DataBuffer_t),
                               (EMBX_VOID **)(&(XCODE_Data_p->CmdInfoParam.DataBuffers_p[i])));
        if(EMBXError != EMBX_SUCCESS)
        {
            return(ST_ERROR_NO_MEMORY);
        }
    }       

    /* ScatterPages allocation */
  	for(i=0; i< NUM_MME_MP2TOH264_PARAM_BUFFERS; i++)
	  {
        EMBXError = EMBX_Alloc(tp, sizeof(MME_ScatterPage_t),
                               (EMBX_VOID **)(&(XCODE_Data_p->CmdInfoParam.DataBuffers_p[i]->ScatterPages_p)));
        if(EMBXError != EMBX_SUCCESS)
        {
            return(ST_ERROR_NO_MEMORY);
        }
    }       

    /* Allocate DataBuffer for Transform Param */
    EMBXError = EMBX_Alloc(tp, NUM_MME_MP2TOH264_TRANSFORM_BUFFERS * sizeof(MME_DataBuffer_t *),
                           (EMBX_VOID **)(&(XCODE_Data_p->CmdInfoFmw.DataBuffers_p)));
    if(EMBXError != EMBX_SUCCESS)
    {
        return(ST_ERROR_NO_MEMORY);
    }

  	for(i=0; i< NUM_MME_MP2TOH264_TRANSFORM_BUFFERS; i++)
  	{
        EMBXError = EMBX_Alloc(tp, sizeof(MME_DataBuffer_t),
                               (EMBX_VOID **)(&(XCODE_Data_p->CmdInfoFmw.DataBuffers_p[i])));
        if(EMBXError != EMBX_SUCCESS)
        {
            return(ST_ERROR_NO_MEMORY);
        }
    }       

    /* ScatterPages allocation */
  	for(i=0; i< NUM_MME_MP2TOH264_TRANSFORM_BUFFERS; i++)
	  {
        EMBXError = EMBX_Alloc(tp, sizeof(MME_ScatterPage_t),
                               (EMBX_VOID **)(&(XCODE_Data_p->CmdInfoFmw.DataBuffers_p[i]->ScatterPages_p)));
        if(EMBXError != EMBX_SUCCESS)
        {
            return(ST_ERROR_NO_MEMORY);
        }
    }       

    EMBXError = EMBX_CloseTransport(tp);
    if(EMBXError != EMBX_SUCCESS)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    
    /* Initialize the Xcode transformer */

    /* Keep track of job completed call back function */
    XCODE_Data_p->XcoderJobCompletedCB = InitParams_p->XcoderJobCompletedCB;
    
    /* Get Transformer name */    
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    RegistersBaseAddress_p = (void *) (((U8 *) InitParams_p->STVID_XcodeInitParams.DeviceBaseAddress_p) + ((U32) InitParams_p->STVID_XcodeInitParams.BaseAddress_p));
    DeviceRegistersBaseAddress = (U32)STAVMEM_CPUToDevice(RegistersBaseAddress_p,&VirtualMapping);
    #if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    #if defined(ST_DELTAMU_GREEN_HE)
    DeviceRegistersBaseAddress = 0xAC220000;
    #else
    DeviceRegistersBaseAddress = 0xAC211000;
    #endif /* ST_DELTAMU_GREEN_HE */ 
    #endif /* (ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */ 
    sprintf(TransformerName, "%s_%08X", MP2H264_XCODE_MME_TRANSFORMER_NAME, DeviceRegistersBaseAddress);

    /* Initialisation parameters set up */
    MMETransformerInitParams.Priority = MME_PRIORITY_NORMAL;
    MMETransformerInitParams.StructSize = sizeof (MME_TransformerInitParams_t);
    MMETransformerInitParams.Callback = &mpg2toh264_TransformerCallback;
    MMETransformerInitParams.CallbackUserData = (void *) XcodeHandle; /* UserData */
    MMETransformerInitParams.TransformerInitParamsSize = sizeof(Xcode_MPEG2H264_InitTransformerParam_t);
    MMETransformerInitParams.TransformerInitParams_p = (MME_GenericParams_t *) &(XCODE_Data_p->Xcode_MPEG2H264_InitTransformerParam);

    do
    {
        ErrorCode = MME_InitTransformer(TransformerName, &MMETransformerInitParams, &XCODE_Data_p->MME_TransformerHandle);
    }
    while (ErrorCode != MME_SUCCESS);

  	if (ErrorCode != MME_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME_InitTransformer(Xcoder) failed, result=$%lx", ErrorCode));
        return (ErrorCode);
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Transformer(Xcoder) initialized, handle %d", XCODE_Data_p->MME_TransformerHandle));

#ifdef XCODE_DEMO
    /* In demo mode, we do not use the H264 decoder, but we use the transcoder in "auto reconstruction" mode */
    /* So, we must allocate frame buffers instead of the ones typically allocated by H264 decoder */
    {
        U32 LoopIndex;
        for(LoopIndex = 0; LoopIndex < MAX_RECONBUFFER_IDX ; LoopIndex++)
        {
            XCODE_Data_p->ReconBuffer[LoopIndex].LumaHdl = (STAVMEM_BlockHandle_t)NULL;
            XCODE_Data_p->ReconBuffer[LoopIndex].ChromaHdl = (STAVMEM_BlockHandle_t)NULL;
            XCODE_Data_p->ReconBuffer[LoopIndex].MBDescrHdl = (STAVMEM_BlockHandle_t)NULL;
        }
#if 0
        XCODE_Data_p->OldestReconBufferIdx = NOT_USED;
        XCODE_Data_p->NewestReconBufferIdx = NOT_USED;
        XCODE_Data_p->CurPicReconBufferIdx = NOT_USED;
#endif
    }
    ErrorCode = mp2toh264_AllocateH264FrameBuffersForDemo(XcodeHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = mp2toh264_DeAllocateH264FrameBuffersForDemo(XcodeHandle);
        return(ST_ERROR_NO_MEMORY);
    }
#endif

#if defined(STVID_VALID) && defined(XCODE_DUMP)
    XCODE_DumpInit();
#endif

    return(ErrorCode);

}

/*******************************************************************************
Name        : mpg2toh264_BuildDecimationFactor
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : XCodeDecimationFactor_t
*******************************************************************************/
static XCodeDecimationFactor_t mpg2toh264_BuildDecimationFactor(STVID_DecimationFactor_t const TargetDecimationFactor)
{
    XCodeDecimationFactor_t DecimationFactor = XCODE_DECIMATION_FACTOR_NONE;    
    
    if (TargetDecimationFactor & STVID_DECIMATION_FACTOR_H2)
    {
        DecimationFactor |= XCODE_DECIMATION_FACTOR_HDEC_2;
    }
    if (TargetDecimationFactor & STVID_DECIMATION_FACTOR_H4)
    {
        DecimationFactor |= XCODE_DECIMATION_FACTOR_HDEC_4;
    }
    if (TargetDecimationFactor & STVID_DECIMATION_FACTOR_V2)
    {
        DecimationFactor |= XCODE_DECIMATION_FACTOR_VDEC_2;
    }

    return(DecimationFactor);
}

/*******************************************************************************
Name        : mpg2toh264_BuildRateControlMode
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : Xcode_RateControl_t
*******************************************************************************/
static Xcode_RateControl_t mpg2toh264_BuildRateControlMode(STVID_RateControlMode_t const TargetRateControlMode)
{
    Xcode_RateControl_t RateControl = XCODE_RATE_CONTROL_NONE;
    
    if (TargetRateControlMode == STVID_RATE_CONTROL_MODE_VBR)
    {
        RateControl = XCODE_RATE_CONTROL_VBR;
    }
    else if (TargetRateControlMode == STVID_RATE_CONTROL_MODE_CBR)
    {
        RateControl = XCODE_RATE_CONTROL_CBR;
    }

    return(RateControl);
}

/*******************************************************************************
Name        : mpg2toh264_BuildAspectRatio
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : Xcode_DisplayAspectRatio_t
*******************************************************************************/
static Xcode_DisplayAspectRatio_t mpg2toh264_BuildAspectRatio(STVID_DisplayAspectRatio_t Aspect)
{
    Xcode_DisplayAspectRatio_t DisplayAspectRatio;
    
    switch(Aspect)
    {
    case STVID_DISPLAY_ASPECT_RATIO_16TO9: 
         DisplayAspectRatio = XCODE_DISPLAY_ASPECT_RATIO_16TO9; 
         break;
    case STVID_DISPLAY_ASPECT_RATIO_4TO3:
         DisplayAspectRatio = XCODE_DISPLAY_ASPECT_RATIO_4TO3; 
         break;
    case STVID_DISPLAY_ASPECT_RATIO_221TO1:
         DisplayAspectRatio = XCODE_DISPLAY_ASPECT_RATIO_221TO1; 
         break;    
    case STVID_DISPLAY_ASPECT_RATIO_SQUARE:
         DisplayAspectRatio = XCODE_DISPLAY_ASPECT_RATIO_SQUARE; 
         break;    
    default: /* STVID_DISPLAY_ASPECT_RATIO_EXTENDED_PAR */
         /* TODO: default value for DisplayAspectRatio */
         DisplayAspectRatio = XCODE_DISPLAY_ASPECT_RATIO_SQUARE; 
    }
    
    return (DisplayAspectRatio);
}

/*******************************************************************************
Name        : mpg2toh264_BuildEntropyCodingMode
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : Xcode_H264_EntropyEncodingMode_t
*******************************************************************************/
static Xcode_H264_EntropyEncodingMode_t mpg2toh264_BuildEntropyCodingMode(XCODER_SpeedMode_t SpeedMode)
{
    /* See table 6 (Transcoder Mode) in Transcoding Specifications */
    if (SpeedMode == XCODER_MODE_SPEED)
    {
        return (XCODE_H264_ENTROPY_ENCODING_MODE_CAVLC);
    }
    
    /* default */
    return (XCODE_H264_ENTROPY_ENCODING_MODE_CABAC);
}

/*******************************************************************************
Name        : mpg2toh264_BuildHalfPictureFlag
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : BOOL
*******************************************************************************/
static BOOL mpg2toh264_BuildHalfPictureFlag(U32 DecimationFactors, XCODER_SpeedMode_t SpeedMode)
{
    /* See MME API constraint on decimation factor */
    if (DecimationFactors & XCODE_DECIMATION_FACTOR_VDEC_2)
    {
        /* See table 6 (Transcoder Mode) in Transcoding Specifications */
        if (SpeedMode != XCODER_MODE_STANDARD)
        {
            return (TRUE);
        }
    }

    /* default */
    return (FALSE);
}

/*******************************************************************************
Name        : mpg2toh264_BuildPictureCodingType
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : Xcode_PictureCodingType_t
*******************************************************************************/
static Xcode_PictureCodingType_t mpg2toh264_BuildPictureCodingType(STVID_MPEGFrame_t MPEGFrame)
{
    Xcode_PictureCodingType_t PictureCodingType;
    switch(MPEGFrame)
    {
        case STVID_MPEG_FRAME_I:
            PictureCodingType = XCODE_I_PICTURE;
            break;
        case STVID_MPEG_FRAME_P:
            PictureCodingType = XCODE_P_PICTURE;
            break;
        default:
            PictureCodingType = XCODE_B_PICTURE;        
    }
    return(PictureCodingType);
}

/*******************************************************************************
Name        : mpg2toh264_BuildPictureStructure
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : Xcode_PictureStructure_t
*******************************************************************************/
static Xcode_PictureStructure_t mpg2toh264_BuildPictureStructure(STVID_PictureStructure_t TargetPictureStructure)
{
    Xcode_PictureStructure_t PictureStructure;
    switch(TargetPictureStructure)
    {
        case STVID_PICTURE_STRUCTURE_TOP_FIELD:
            PictureStructure = XCODE_PICTURE_STRUCTURE_TOP_FIELD;
            break;
        case STVID_PICTURE_STRUCTURE_BOTTOM_FIELD:
            PictureStructure = XCODE_PICTURE_STRUCTURE_BOTTOM_FIELD;
            break;
        default:
            PictureStructure = XCODE_PICTURE_STRUCTURE_FRAME;            
    }
    return(PictureStructure);
}

/*******************************************************************************
Name        : mpg2toh264_BuildPictureScanType
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : Xcode_ScanType_t
*******************************************************************************/
static Xcode_ScanType_t mpg2toh264_BuildPictureScanType(STGXOBJ_ScanType_t TargetScanType)
{
    Xcode_ScanType_t ScanType;
    if (TargetScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        ScanType = XCODE_SCAN_TYPE_PROGRESSIVE;
    }
    else
    {
        ScanType = XCODE_SCAN_TYPE_INTERLACED;
    }
    return(ScanType);
}

/*******************************************************************************
Name        : mpg2toh264_BuildPictureFlags
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : U32
*******************************************************************************/
static U32 mpg2toh264_BuildPictureFlags(STVID_PictureStructure_t TargetPictureStructure, VIDCOM_PictureGenericData_t PictureGenericData)
{
    U32 PictureFlags = 0;
    if (PictureGenericData.RepeatFirstField)
    {
        PictureFlags |= XCODE_PICTURE_FLAGS_REPEAT_FIRST_FIELD;
    }
    if (PictureGenericData.IsFirstOfTwoFields && (TargetPictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD))
    {
        PictureFlags |= XCODE_PICTURE_FLAGS_TOP_FIELD_FIRST;    
    }
    return(PictureFlags);   
}

/*******************************************************************************
Name        : mpg2toh264_BuildEncodingMode
Description : parameter translation from STVID to Xcoder specific
Parameters  : 
Assumptions :
Limitations :
Returns     : Xcode_EncodingMode_t
*******************************************************************************/
static Xcode_EncodingMode_t mpg2toh264_BuildEncodingMode(STVID_PictureStructure_t PictureStructure)
{
    Xcode_EncodingMode_t EncodingMode;
    
    if (PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
    {
        /* TODO: implement algorithm to select Field/Frame encoding for frame source picture */
        EncodingMode = XCODE_ENCODING_MODE_FRAME;  
    }
    else
    {
        EncodingMode = XCODE_ENCODING_MODE_FIELD;
    }
    return(EncodingMode);
}

/*******************************************************************************
Name        : mpg2toh264Xcode_TranscodePicture
Description : transcode picture
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Xcode_TranscodePicture(const XCODE_Handle_t XcodeHandle, const XCODE_TranscodePictureParams_t * const TranscodePictureParams_p)
{
    mpg2toh264xcode_PrivateData_t * XCODE_Data_p;
    XCODE_Properties_t *            XCODE_Properties_p;
    ST_ErrorCode_t                  ErrorCode;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* Variables for global params setup */
    Xcode_GlobalParams_t            Xcode_GlobalParams;
    Xcode_MPEG2H264_GlobalParams_t  MPEG2H264_GlobalParams;
    /* Variables for transform setup */
    Xcode_TransformParams_t                Xcode_TransformParams;
    Xcode_MPEG2H264_TransformParams_t      MPEG2H264_TransformParams;
    Xcode_MPEG2H264_Debug_t                MPEG2H264_Debug;
    /* Aliases to help reading the code */
    STVID_PictureInfos_t *                      MainPictureInfos_p;
    VIDCOM_GlobalDecodingContextGenericData_t * MainGDCGD_p;
#ifdef XCODE_DEMO
    H264ReconBufferIdx_t Swap;
#endif
           
    ErrorCode = ST_NO_ERROR;
    XCODE_Properties_p = (XCODE_Properties_t *) XcodeHandle;
    XCODE_Data_p = (mpg2toh264xcode_PrivateData_t *) XCODE_Properties_p->PrivateData_p;


    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

    XCODE_Data_p->mpg2toh264XcoderContext.XCODER_TranscodingJobResults.TranscodingJobStatus.JobSubmissionTime = time_now();

    /* Global Parameters */
    /* Setup aliases first */
    MainPictureInfos_p = (STVID_PictureInfos_t *)&(TranscodePictureParams_p->MainPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos);
    MainGDCGD_p = (VIDCOM_GlobalDecodingContextGenericData_t *)&(TranscodePictureParams_p->MainPictureBuffer.ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData);

    /* TODO: global generic parameters shall reside in generic transcoder, 
       but this implies sharing the generic interface header file. 
       And today, there is one single file that contains both generic and 
       specific MME API parameters
    */
      
    /* FrameRate in Xcoder is the sequence frame rate multiplied by 100: thus the stvid frameRate/10! */   
    Xcode_GlobalParams.FrameRate = MainPictureInfos_p->VideoParams.FrameRate/10;
    Xcode_GlobalParams.PictureWidth = MainPictureInfos_p->BitmapParams.Width;
    Xcode_GlobalParams.PictureHeight = MainPictureInfos_p->BitmapParams.Height;

    Xcode_GlobalParams.DecimationFactors    = mpg2toh264_BuildDecimationFactor(TranscodePictureParams_p->TargetDecimationFactor);
    Xcode_GlobalParams.ChromaFormat         = XCODE_CHROMA_FORMAT_420; /* No other choice! */
    Xcode_GlobalParams.RCOutputBitrateValue = TranscodePictureParams_p->TargetBitRate;
    Xcode_GlobalParams.RateControlMode      = mpg2toh264_BuildRateControlMode(TranscodePictureParams_p->RateControlMode);

    Xcode_GlobalParams.HalfPictureFlag = mpg2toh264_BuildHalfPictureFlag(Xcode_GlobalParams.DecimationFactors, TranscodePictureParams_p->SpeedMode);

    Xcode_GlobalParams.AspectRatio         = mpg2toh264_BuildAspectRatio(MainGDCGD_p->SequenceInfo.Aspect);
    Xcode_GlobalParams.ProgressiveSequence = (MainGDCGD_p->SequenceInfo.ScanType == STVID_SCAN_TYPE_PROGRESSIVE)?TRUE:FALSE;
    Xcode_GlobalParams.VideoFormat         = MainGDCGD_p->SequenceInfo.VideoFormat;
    Xcode_GlobalParams.ColourPrimaries     = MainGDCGD_p->ColourPrimaries;
    Xcode_GlobalParams.TransferCharacteristics = MainGDCGD_p->TransferCharacteristics;
    Xcode_GlobalParams.MatrixCoefficients = MainGDCGD_p->MatrixCoefficients;

    /* Specific global parameters */
    MPEG2H264_GlobalParams.profile_idc = XCODE_H264_BASELINE_PROFILE; /* TODO */
    MPEG2H264_GlobalParams.EntropyCodingMode = mpg2toh264_BuildEntropyCodingMode(TranscodePictureParams_p->SpeedMode);

    /* Build MME command */
    XCODE_Data_p->CmdInfoParam.StructSize  = sizeof(MME_Command_t);
    XCODE_Data_p->CmdInfoParam.CmdCode     = MME_SET_GLOBAL_TRANSFORM_PARAMS;
    XCODE_Data_p->CmdInfoParam.CmdEnd      = MME_COMMAND_END_RETURN_NOTIFY;
    XCODE_Data_p->CmdInfoParam.DueTime     = 0;
    XCODE_Data_p->CmdInfoParam.NumberInputBuffers  = 1;
    XCODE_Data_p->CmdInfoParam.NumberOutputBuffers = 0;
    /* Specific parameters are passed through DataBuffer[0] */
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->StructSize = sizeof (MME_DataBuffer_t);
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->UserData_p = NULL;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->Flags = 0;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->StreamNumber = 0;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->NumberOfScatterPages = 1;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->StartOffset = 0;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->ScatterPages_p->Page_p = &MPEG2H264_GlobalParams;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->ScatterPages_p->Size   = sizeof(MPEG2H264_GlobalParams);
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->ScatterPages_p->BytesUsed  = 0;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->ScatterPages_p->FlagsIn    = 0;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->ScatterPages_p->FlagsOut   = 0;
    XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->TotalSize = XCODE_Data_p->CmdInfoParam.DataBuffers_p[0]->ScatterPages_p->Size;
    
    XCODE_Data_p->CmdInfoParam.CmdStatus.AdditionalInfoSize = 0;
    XCODE_Data_p->CmdInfoParam.CmdStatus.AdditionalInfo_p = NULL;
    XCODE_Data_p->CmdInfoParam.ParamSize   = sizeof(Xcode_GlobalParams);
    XCODE_Data_p->CmdInfoParam.Param_p = (MME_GenericParams_t)&(Xcode_GlobalParams);

    /* TODO: Quick trick to send one single Global Param command */
    if (!GlobalSent)
    {
#if defined(STVID_VALID) && defined(XCODE_DUMP)
    XCODE_DumpGlobalParam(&XCODE_Data_p->CmdInfoParam);
#endif

    ErrorCode =  MME_SendCommand(XCODE_Data_p->MME_TransformerHandle, &XCODE_Data_p->CmdInfoParam);
    /* Wait call back */
    semaphore_wait(XCODE_Data_p->GlobalTransformParamsCompletedSemaphore_p);
    GlobalSent = TRUE;
    }

    /* Transform parameters */
  	Xcode_TransformParams.EndOfBitstreamFlag = 0; /* TODO */
  	Xcode_TransformParams.TemporalReference = TranscodePictureParams_p->MainPictureBuffer.ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
  	Xcode_TransformParams.PictureCodingType = mpg2toh264_BuildPictureCodingType(MainPictureInfos_p->VideoParams.MPEGFrame);
  	Xcode_TransformParams.PictureStructure  = mpg2toh264_BuildPictureStructure(MainPictureInfos_p->VideoParams.PictureStructure);
  	Xcode_TransformParams.PictureScanType   = mpg2toh264_BuildPictureScanType(MainPictureInfos_p->VideoParams.ScanType);
  	Xcode_TransformParams.PictureFlags      = mpg2toh264_BuildPictureFlags(MainPictureInfos_p->VideoParams.PictureStructure, TranscodePictureParams_p->MainPictureBuffer.ParsedPictureInformation.PictureGenericData);
  	Xcode_TransformParams.EncodingMode      = mpg2toh264_BuildEncodingMode(MainPictureInfos_p->VideoParams.PictureStructure);

    if (Xcode_GlobalParams.DecimationFactors != XCODE_DECIMATION_FACTOR_NONE)
    {
       /* Deal with decimated buffer */
       if (!TranscodePictureParams_p->DecimatedPictureBufferIsValid)
       {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"mpg2toh264xcode.c: Decimated picture buffer is NULL! \n" ));
       }
       else
       {
  	       Xcode_TransformParams.InputBuffer.Luma_p = (Xcode_LumaAddress_t) TranscodePictureParams_p->DecimatedPictureBuffer.FrameBuffer_p->Allocation.Address_p;
  	       Xcode_TransformParams.InputBuffer.Chroma_p = (Xcode_ChromaAddress_t) TranscodePictureParams_p->DecimatedPictureBuffer.FrameBuffer_p->Allocation.Address2_p;
  	   }
  	}
  	else
  	{
  	   /* Deal with main buffer */
  	   Xcode_TransformParams.InputBuffer.Luma_p = (Xcode_LumaAddress_t) TranscodePictureParams_p->MainPictureBuffer.FrameBuffer_p->Allocation.Address_p;
  	   Xcode_TransformParams.InputBuffer.Chroma_p = (Xcode_ChromaAddress_t) TranscodePictureParams_p->MainPictureBuffer.FrameBuffer_p->Allocation.Address2_p;
    }
    /* MBDescr buffer is defined with Main Picture Buffer only! */
    /* MBDescr buffer for transcoder is shifted by 1 word (First word is for Driver usage only) */
/*  	Xcode_TransformParams.InputBuffer.Param_p = (Xcode_GenericParam_t) ((U32)TranscodePictureParams_p->MainPictureBuffer.PPB.Address_p);*/
    Xcode_TransformParams.InputBuffer.Param_p = (Xcode_GenericParam_t) ((U32)TranscodePictureParams_p->MainPictureBuffer.PPB.Address_p + 4);

  	Xcode_TransformParams.OutputBuffer.CompressedData_p = (U32 *)TranscodePictureParams_p->Write_p;
  	Xcode_TransformParams.OutputBuffer.CompressedDataBufferSize = TranscodePictureParams_p->XcoderOutputBufferAvailableSize;

  	Xcode_TransformParams.AdditionalFlags = XCODE_ADDITIONAL_FLAG_NONE;

    /* Specific parameters for transform */
#ifdef XCODE_DEMO
#if 1
    MPEG2H264_TransformParams.RefPictureBuffer[0].Luma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[0]].Luma_p;
    MPEG2H264_TransformParams.RefPictureBuffer[0].Chroma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[0]].Chroma_p;
    MPEG2H264_TransformParams.RefPictureBuffer[0].Param_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[0]].MBDescr_p;
    
    MPEG2H264_TransformParams.RefPictureBuffer[1].Luma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[1]].Luma_p;
    MPEG2H264_TransformParams.RefPictureBuffer[1].Chroma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[1]].Chroma_p;
    MPEG2H264_TransformParams.RefPictureBuffer[1].Param_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[1]].MBDescr_p;

    MPEG2H264_TransformParams.ReconBuffer.Luma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[2]].Luma_p; /* TODO: manage reference list */
    MPEG2H264_TransformParams.ReconBuffer.Chroma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[2]].Chroma_p; /* TODO: manage reference list */
    MPEG2H264_TransformParams.ReconBuffer.Param_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->ReconBufferIdx[2]].MBDescr_p; /* TODO: manage reference list */
#endif
#if 0
    if (XCODE_Data_p->OldestReconBufferIdx != NOT_USED)
    {
        MPEG2H264_TransformParams.RefPictureBuffer[0].Luma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->OldestReconBufferIdx].Luma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[0].Chroma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->OldestReconBufferIdx].Chroma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[0].Param_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->OldestReconBufferIdx].MBDescr_p;
    }
    else
    {
        /* When not used, the buffer shall point to a valid address */
        /* Here, we choose index 0, but it is "dont care" */
        MPEG2H264_TransformParams.RefPictureBuffer[0].Luma_p = XCODE_Data_p->ReconBuffer[0].Luma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[0].Chroma_p = XCODE_Data_p->ReconBuffer[0].Chroma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[0].Param_p = XCODE_Data_p->ReconBuffer[0].MBDescr_p;

        XCODE_Data_p->CurPicReconBufferIdx = OLDEST_REF;        
    }
    if (XCODE_Data_p->NewestReconBufferIdx != NOT_USED)
    {
        MPEG2H264_TransformParams.RefPictureBuffer[1].Luma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->NewestReconBufferIdx].Luma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[1].Chroma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->NewestReconBufferIdx].Chroma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[1].Param_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->NewestReconBufferIdx].MBDescr_p;
    }
    else
    {
        /* When not used, the buffer shall point to a valid address */
        /* Here, we choose index 0, but it is "dont care" */
        MPEG2H264_TransformParams.RefPictureBuffer[1].Luma_p = XCODE_Data_p->ReconBuffer[0].Luma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[1].Chroma_p = XCODE_Data_p->ReconBuffer[0].Chroma_p;
        MPEG2H264_TransformParams.RefPictureBuffer[1].Param_p = XCODE_Data_p->ReconBuffer[0].MBDescr_p;

        XCODE_Data_p->CurPicReconBufferIdx = NEWEST_REF;        

    }

    if ((XCODE_Data_p->CurPicReconBufferIdx != OLDEST_REF) && (XCODE_Data_p->CurPicReconBufferIdx != NEWEST_REF))
    {
        XCODE_Data_p->CurPicReconBufferIdx = CURRENT_PIC;
    }
    MPEG2H264_TransformParams.ReconBuffer.Luma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->CurPicReconBufferIdx].Luma_p; /* TODO: manage reference list */
    MPEG2H264_TransformParams.ReconBuffer.Chroma_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->CurPicReconBufferIdx].Chroma_p; /* TODO: manage reference list */
    MPEG2H264_TransformParams.ReconBuffer.Param_p = XCODE_Data_p->ReconBuffer[XCODE_Data_p->CurPicReconBufferIdx].MBDescr_p; /* TODO: manage reference list */
#endif /* #if 0 */
#endif

    MPEG2H264_Debug.AllowedMbTypes = ALL_MB_TYPES_ALLOWED;

    /* Build MME command */
    XCODE_Data_p->CmdInfoFmw.StructSize  = sizeof(MME_Command_t);
    XCODE_Data_p->CmdInfoFmw.CmdCode     = MME_TRANSFORM;
    XCODE_Data_p->CmdInfoFmw.CmdEnd      = MME_COMMAND_END_RETURN_NOTIFY;
    XCODE_Data_p->CmdInfoFmw.DueTime     = 0;
    XCODE_Data_p->CmdInfoFmw.NumberInputBuffers  = 2;
    XCODE_Data_p->CmdInfoFmw.NumberOutputBuffers = 0;
    /* Specific parameters are passed through DataBuffer[0] */
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->StructSize = sizeof (MME_DataBuffer_t);
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->UserData_p = NULL;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->Flags = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->StreamNumber = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->NumberOfScatterPages = 1;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->StartOffset = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->ScatterPages_p->Page_p = &MPEG2H264_TransformParams;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->ScatterPages_p->Size   = sizeof(MPEG2H264_TransformParams);
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->ScatterPages_p->BytesUsed  = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->ScatterPages_p->FlagsIn    = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->ScatterPages_p->FlagsOut   = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->TotalSize = XCODE_Data_p->CmdInfoFmw.DataBuffers_p[0]->ScatterPages_p->Size;

    /* Debug information are conveyed through DataBuffer[1] */
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->StructSize = sizeof (MME_DataBuffer_t);
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->UserData_p = NULL;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->Flags = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->StreamNumber = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->NumberOfScatterPages = 1;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->StartOffset = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->ScatterPages_p->Page_p = &MPEG2H264_Debug;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->ScatterPages_p->Size   = sizeof(MPEG2H264_Debug);
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->ScatterPages_p->BytesUsed  = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->ScatterPages_p->FlagsIn    = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->ScatterPages_p->FlagsOut   = 0;
    XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->TotalSize = XCODE_Data_p->CmdInfoFmw.DataBuffers_p[1]->ScatterPages_p->Size;
    
    XCODE_Data_p->CmdInfoFmw.CmdStatus.AdditionalInfoSize = sizeof(XCode_TransformStatusAdditionalInfo_t);
    XCODE_Data_p->CmdInfoFmw.CmdStatus.AdditionalInfo_p = &(XCODE_Data_p->TransformStatusAdditionalInfo);
    XCODE_Data_p->CmdInfoFmw.ParamSize   = sizeof(Xcode_TransformParams);
    XCODE_Data_p->CmdInfoFmw.Param_p = (MME_GenericParams_t)&(Xcode_TransformParams);

#if defined(STVID_VALID) && defined(XCODE_DUMP)
    XCODE_DumpTransformParam(&Xcode_GlobalParams, &XCODE_Data_p->CmdInfoFmw);
#endif

    ErrorCode =  MME_SendCommand(XCODE_Data_p->MME_TransformerHandle, &XCODE_Data_p->CmdInfoFmw);

    /* Wait call back */
    semaphore_wait(XCODE_Data_p->TransformCompletedSemaphore_p);
    
#ifdef XCODE_DEMO
    /* Simplified sliding window on reference pictures */
#if 1
    if (Xcode_TransformParams.PictureCodingType != XCODE_B_PICTURE)
    {
        Swap = XCODE_Data_p->ReconBufferIdx[0];
        XCODE_Data_p->ReconBufferIdx[0] = XCODE_Data_p->ReconBufferIdx[1];
        XCODE_Data_p->ReconBufferIdx[1] = XCODE_Data_p->ReconBufferIdx[2];
        XCODE_Data_p->ReconBufferIdx[2] = Swap;
    }
#endif
#if 0
    if (Xcode_TransformParams.PictureCodingType != XCODE_B_PICTURE)
    {
        XCODE_Data_p->OldestReconBufferIdx = XCODE_Data_p->NewestReconBufferIdx;
        XCODE_Data_p->NewestReconBufferIdx = XCODE_Data_p->CurPicReconBufferIdx;
    }
#endif /* #if O */
#endif

    /* Inform generic transcoder of job completion: Updating write pointer */
     XCODE_Data_p->XcoderJobCompletedCB(XCODE_Data_p->TranscoderHandle, XCODE_Data_p->mpg2toh264XcoderContext.XCODER_TranscodingJobResults);
    
    return(ErrorCode);
}

/*******************************************************************************
Name        : mpg2toh264Xcode_Term
Description : terminate transcodec
Parameters  : 
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Xcode_Term(const XCODE_Handle_t XcodeHandle)
{
    mpg2toh264xcode_PrivateData_t * XCODE_Data_p;
    XCODE_Properties_t * XCODE_Properties_p;
    ST_ErrorCode_t ErrorCode;
    U8             i;
    
    ErrorCode = ST_NO_ERROR;
    XCODE_Properties_p = (XCODE_Properties_t *) XcodeHandle;
    XCODE_Data_p = (mpg2toh264xcode_PrivateData_t *) XCODE_Properties_p->PrivateData_p;

    /* delete semaphores */
    semaphore_delete(XCODE_Data_p->TransformCompletedSemaphore_p);
    semaphore_delete(XCODE_Data_p->GlobalTransformParamsCompletedSemaphore_p);

	  /* Terminate MME API */
    /* TODO: check returned value */
    MME_TermTransformer(XCODE_Data_p->MME_TransformerHandle);
    
    /* Free allocated buffers */
    for(i=0; i< NUM_MME_MP2TOH264_PARAM_BUFFERS; i++)
    {
        /* TODO: check returned value */
        EMBX_Free((EMBX_VOID **)(&(XCODE_Data_p->CmdInfoParam.DataBuffers_p[i]->ScatterPages_p)));
    }
        
    for(i=0; i< NUM_MME_MP2TOH264_PARAM_BUFFERS; i++)
    {
        /* TODO: check returned value */
        EMBX_Free((EMBX_VOID **)(&(XCODE_Data_p->CmdInfoParam.DataBuffers_p[i])));
    }
    /* TODO: check returned value */
    EMBX_Free((EMBX_VOID **)(&(XCODE_Data_p->CmdInfoParam.DataBuffers_p)));
        
    for(i=0; i< NUM_MME_MP2TOH264_TRANSFORM_BUFFERS; i++)
    {
        /* TODO: check returned value */
        EMBX_Free((EMBX_VOID **)(&(XCODE_Data_p->CmdInfoFmw.DataBuffers_p[i]->ScatterPages_p)));
    }    
    for(i=0; i< NUM_MME_MP2TOH264_TRANSFORM_BUFFERS; i++)
    {
        /* TODO: check returned value */
        EMBX_Free((EMBX_VOID **)(&(XCODE_Data_p->CmdInfoFmw.DataBuffers_p[i])));
    }
    /* TODO: check returned value */
    EMBX_Free((EMBX_VOID **)(&(XCODE_Data_p->CmdInfoFmw.DataBuffers_p)));

    /* TODO: check returned value */
	  EMBX_Free((EMBX_VOID **)(&(XCODE_Data_p)));

#if defined(STVID_VALID) && defined(XCODE_DUMP)
    XCODE_DumpTerm();
#endif

    return(ErrorCode);
}


/* End of mpg2toh264xcode.c */
