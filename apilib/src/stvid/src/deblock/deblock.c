/*******************************************************************************

File name   : deblock.c

Description : Mpeg2 deblocking feature  file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
24 Nov 2006        Created                                           IK
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#include <assert.h>
#endif

#ifdef STVID_DEBUG_DECODER
    #define STTBX_REPORT
#endif
#include "sttbx.h"

#ifndef STUB_FMW_ST40
#include "mme.h"
#include "embx.h"
#endif /* STUB_FMW_ST40 */

#define MULTICOM
#include "deblock.h"
#include "vid_mpeg.h"
/*!\brief Maximum width of the frame in macroblock unit.
The max horizontal_size is 1920 samples/line as given in standard Table 8-11.
mb_width is (horizontal_size + 15)/16. */
#define MB_WIDTH_MAX 45

/*!\brief Maximum height of the frame in macroblock unit.
The max vertical_size is 1088 lines/frame as given in standard Table 8-11.
mb_height is (vertical_size + 15)/16. */
#define MB_HEIGHT_MAX 36

/*!\brief Size in bytes of the picture parameters per MB. */
#define MB_PARAM_MAX 2

/* Types -------------------------------------------------------------------- */

typedef struct DEBLOCK_CompanionData_s
{
    MME_Command_t               CmdSeq;/* MME_SET_GLOBAL_TRANSFORM_PARAMS */
    MME_Command_t               CmdPic;/* MME_TRANSFORM */
    MPEG2Deblock_SequenceParam_t       SequenceParam;
    MPEG2Deblock_TransformParam_t      TransformParam;
    MPEG2Deblock_CommandStatus_t       CommandStatus;
} DEBLOCK_CompanionData_t;

typedef struct DEBLOCK_PrivateData_s
{
    ST_Partition_t *                        CPUPartition_p;
    STAVMEM_PartitionHandle_t               AvmemPartitionHandle;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STAVMEM_BlockHandle_t                   MBDescrBufferBlockHandle;
    void *                                  MBDescrBufferAddress_p;
    U32                                     MBDescrBufferSize;
    MME_TransformerHandle_t                 TranformerHandle;
    EMBX_TRANSPORT                          EmbxTransport;
    BOOL                                    IsEmbxTransportValid;
    DEBLOCK_CompanionData_t *               CompanionData_p;
   	U32                                     mb_width;
    U32                                     mb_height;
    void *                                  EndDeblockingCallbackFuction;
    BOOL                                    NewGlobalTransform;
    BOOL                                    TransformerInitDone;
    void *                                  DeltaBaseAddress_p;
} DEBLOCK_PrivateData_t;


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : FreePrivateData
Description : Free the private data structure
Parameters  : Pointer to the private data
Assumptions :
Limitations :
Returns     :  ST_ErrorCode_t
*******************************************************************************/

static void FreePrivateData(DEBLOCK_PrivateData_t * PrivateData_p)
{
    void *VirtualAddress = NULL;
    STAVMEM_FreeBlockParams_t FreeParams;
    EMBX_ERROR                      EMBXError;
    ST_Partition_t * CPUPartition_p = PrivateData_p->CPUPartition_p;

	if (PrivateData_p != NULL)
    {
        if (STAVMEM_GetBlockAddress(PrivateData_p->MBDescrBufferBlockHandle, (void **)&VirtualAddress)== ST_NO_ERROR)
        {
            FreeParams.PartitionHandle = PrivateData_p->AvmemPartitionHandle;
            STAVMEM_FreeBlock(&FreeParams, &(PrivateData_p->MBDescrBufferBlockHandle));
        }

        if (PrivateData_p->CompanionData_p != NULL)
        {
            EMBXError = EMBX_Free((EMBX_VOID *)(PrivateData_p->CompanionData_p));
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
        }
        SAFE_MEMORY_DEALLOCATE(PrivateData_p, CPUPartition_p, sizeof(PrivateData_p));
    }
}

/*******************************************************************************
Name        : VID_DeblockingInit
Description : Actions to do on chip when initialising
Parameters  : VID Deblocking manager handle, InitParams_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VID_DeblockingInit (VID_Deblocking_InitParams_t * const InitParams_p, VID_Deblocking_Handle_t * const DeblockHandle)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    DEBLOCK_CompanionData_t         *CompanionData_p;
    DEBLOCK_PrivateData_t           *PrivateData_p;
    STAVMEM_AllocBlockParams_t      AllocBlockParams;
    void                            *VirtualAddress = NULL;
    EMBX_TRANSPORT                  tp;

    if ((DeblockHandle == NULL)||(InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate private data structure */
    SAFE_MEMORY_ALLOCATE(PrivateData_p, DEBLOCK_PrivateData_t *, InitParams_p->CPUPartition_p, sizeof(DEBLOCK_PrivateData_t));
    if (PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    *DeblockHandle = (VID_Deblocking_Handle_t) PrivateData_p;

    /* Private data initialisation */
    PrivateData_p->IsEmbxTransportValid = FALSE;
    PrivateData_p->MBDescrBufferBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    PrivateData_p->MBDescrBufferAddress_p = NULL;

    PrivateData_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    PrivateData_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
    PrivateData_p->EndDeblockingCallbackFuction = InitParams_p->EndDeblockingCallbackFuction ;
    PrivateData_p->NewGlobalTransform = TRUE;
    PrivateData_p->TransformerInitDone= FALSE;
    PrivateData_p->DeltaBaseAddress_p =InitParams_p->DeltaBaseAddress_p;
    STAVMEM_GetSharedMemoryVirtualMapping(&(PrivateData_p->VirtualMapping));

    /* Set some generic parameters for AVMEM allocation */
    AllocBlockParams.PartitionHandle          = InitParams_p->AvmemPartitionHandle;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    /* Size the MB description buffer */
    AllocBlockParams.Alignment = 256;
    AllocBlockParams.Size = MB_WIDTH_MAX*MB_HEIGHT_MAX*MB_PARAM_MAX;
    AllocBlockParams.Size = (AllocBlockParams.Size + AllocBlockParams.Alignment - 1) & (~(AllocBlockParams.Alignment - 1));

    /* Allocate the MB description buffer */
    ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &PrivateData_p->MBDescrBufferBlockHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        goto cleanup;
    }

    /* Get the virtual address of the buffer */
    ErrorCode = STAVMEM_GetBlockAddress(PrivateData_p->MBDescrBufferBlockHandle,(void **)&VirtualAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        goto cleanup;
    }
    PrivateData_p->MBDescrBufferAddress_p = VirtualAddress;
    PrivateData_p->MBDescrBufferSize = AllocBlockParams.Size;

    /* Allocate Host/LX shared data in EMBX shared memory to allow Multicom
     * to perform address translation */
    ErrorCode = EMBX_OpenTransport(MME_VIDEO_TRANSPORT_NAME, &tp);
    if (ErrorCode != EMBX_SUCCESS)
    {
        goto cleanup;
    }

    PrivateData_p->EmbxTransport = tp;
    PrivateData_p->IsEmbxTransportValid = TRUE;

    /*  Allocate companion shared data structure allocation */
    ErrorCode = EMBX_Alloc(tp, sizeof(DEBLOCK_CompanionData_t),(EMBX_VOID **)(&(CompanionData_p)));
    if (ErrorCode != EMBX_SUCCESS)
    {
        goto cleanup;
    }

    /* Default deblocking user control */
    CompanionData_p->TransformParam.PictureParam.FilterOffsetQP = 15;
    /* Default transform parameters */
    CompanionData_p->TransformParam.MainAuxEnable = MPEG2DEBLOCK_MAINOUT_EN;
    CompanionData_p->TransformParam.HorizontalDecimationFactor = MPEG2DEBLOCK_HDEC_1;
    CompanionData_p->TransformParam.VerticalDecimationFactor = MPEG2DEBLOCK_VDEC_1;
    CompanionData_p->TransformParam.DeblockingEnable = MPEG2DEBLOCK_DEBLOCK_FULLSCREEN;

    CompanionData_p->CommandStatus.TransformTimeoutInMicros=60000;
    CompanionData_p->CommandStatus.DecodeTimeInMicros=0;
    CompanionData_p->CommandStatus.FilterTimeInMicros=0;
    CompanionData_p->CommandStatus.PictureMeanQP=0;
    CompanionData_p->CommandStatus.PictureVarianceQP=0;
    /* MME_SET_GLOBAL_TRANSFORM_PARAMS */
    CompanionData_p->CmdSeq.StructSize  = sizeof(MME_Command_t);
    CompanionData_p->CmdSeq.CmdEnd      = MME_COMMAND_END_RETURN_NO_INFO;
    CompanionData_p->CmdSeq.DueTime     = 0;
    CompanionData_p->CmdSeq.ParamSize   = sizeof(MPEG2Deblock_SequenceParam_t);
    CompanionData_p->CmdSeq.Param_p     = &CompanionData_p->SequenceParam;
    CompanionData_p->CmdSeq.CmdCode     = MME_SET_GLOBAL_TRANSFORM_PARAMS;
    CompanionData_p->CmdSeq.NumberInputBuffers  = 0;
    CompanionData_p->CmdSeq.NumberOutputBuffers = 0;
    CompanionData_p->CmdSeq.DataBuffers_p = NULL;
    CompanionData_p->CmdSeq.CmdStatus.AdditionalInfoSize = sizeof(MPEG2Deblock_CommandStatus_t) /* 0 */;
    CompanionData_p->CmdSeq.CmdStatus.AdditionalInfo_p =&(CompanionData_p->CommandStatus)/*NULL */  ;

    /* MME_TRANSFORM */
    CompanionData_p->CmdPic.StructSize  = sizeof(MME_Command_t);
    CompanionData_p->CmdPic.CmdEnd      = MME_COMMAND_END_RETURN_NOTIFY;
    CompanionData_p->CmdPic.DueTime     = 0;
    CompanionData_p->CmdPic.ParamSize   = sizeof(MPEG2Deblock_TransformParam_t);
    CompanionData_p->CmdPic.Param_p     = &CompanionData_p->TransformParam;
    CompanionData_p->CmdPic.CmdCode     = MME_TRANSFORM;
    CompanionData_p->CmdPic.NumberInputBuffers  = 0;
    CompanionData_p->CmdPic.NumberOutputBuffers = 0;
    CompanionData_p->CmdPic.DataBuffers_p = NULL;
    CompanionData_p->CmdPic.CmdStatus.AdditionalInfoSize = sizeof(MPEG2Deblock_CommandStatus_t) /* 0 */;
    CompanionData_p->CmdPic.CmdStatus.AdditionalInfo_p = &(CompanionData_p->CommandStatus)/*NULL */;

    PrivateData_p->CompanionData_p = CompanionData_p;

    return (ErrorCode);

cleanup:

    FreePrivateData(PrivateData_p);
    *DeblockHandle = (U32)NULL;

    return(ErrorCode);


}/* End of VID_DeblockingInit() function */

/*******************************************************************************
Name        : VID_DeblockingTerm
Description : Actions to do on chip when terminating
Parameters  : VID Deblocking manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VID_DeblockingTerm(VID_Deblocking_Handle_t * const DeblockHandle)
{
    DEBLOCK_PrivateData_t           *PrivateData_p;

  	if ((PrivateData_p=(DEBLOCK_PrivateData_t*)DeblockHandle)== NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\nERROR FILE %s LINE %d\n",__FILE__,__LINE__));
          return;
      }

	/* terminate MME API */
#ifdef STUB_FMW_ST40
    MMESTUB_TermTransformer(PrivateData_p->TranformerHandle);
#else
    MME_TermTransformer(PrivateData_p->TranformerHandle);
#endif

    FreePrivateData(PrivateData_p);
    *DeblockHandle = (U32)NULL;

} /* End of Term() function */

/*******************************************************************************
Name        : VID_DeblockingSetParams
Description : PSave params needed by firmware for deblocking
Parameters  : DeblockingHandle, StreamInfo_p
Assumptions :
Limitations :
Returns     :  ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VID_DeblockingSetParams ( VID_Deblocking_Handle_t * const DeblockHandle,VID_Deblocking_StreamInfo_t  StreamInfo,MPEG2Deblock_InitTransformerParam_t   TransformerInitParams)
{
  ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
  StreamInfoForDecode_t           *StreamInfo_p     = (StreamInfoForDecode_t *) StreamInfo;
  DEBLOCK_PrivateData_t           *PrivateData_p    = (DEBLOCK_PrivateData_t *) *DeblockHandle;
  DEBLOCK_CompanionData_t         *CompanionData_p  = (PrivateData_p->CompanionData_p);
  MPEG2Deblock_SequenceParam_t           *SequenceParam_p  = &(CompanionData_p->SequenceParam);
  MPEG2Deblock_TransformParam_t          *TransformParam_p = &(CompanionData_p->TransformParam);
  MPEG2Deblock_InitTransformerParam_t    MPEG2Deblock_TransformerInitParams;
  MME_TransformerInitParams_t     MME_TransformerInitParams;
  char                            TransformerName[MME_MAX_TRANSFORMER_NAME];
  U32                             DeviceDeltaBaseAddress;
  int                             i;

if (PrivateData_p->TransformerInitDone == FALSE)
  {
    /* Prefix transformer name with device registers base address */
    STAVMEM_GetSharedMemoryVirtualMapping(&(PrivateData_p->VirtualMapping));
    DeviceDeltaBaseAddress = (U32)STAVMEM_CPUToDevice(PrivateData_p->DeltaBaseAddress_p,&(PrivateData_p->VirtualMapping));

#ifdef NATIVE_CORE
   /* Current VSOC is using a model for deltamu with old MME_TRANSFORMER_NAME */
    sprintf(TransformerName, "%s", MPEG2DEBLOCK_MME_TRANSFORMER_NAME);
#else
    sprintf(TransformerName, "%s_%08X", MPEG2DEBLOCK_MME_TRANSFORMER_NAME,DeviceDeltaBaseAddress);
#endif

    /* MPEG2 transformer specific initialisation */
    MPEG2Deblock_TransformerInitParams.InputBufferBegin = TransformerInitParams.InputBufferBegin;
    MPEG2Deblock_TransformerInitParams.InputBufferEnd   = TransformerInitParams.InputBufferEnd;

    /* MME transformer initialisation */
    MME_TransformerInitParams.Priority = MME_PRIORITY_HIGHEST;
    MME_TransformerInitParams.StructSize = sizeof (MME_TransformerInitParams_t);
    MME_TransformerInitParams.Callback = PrivateData_p->EndDeblockingCallbackFuction ;/*&TransformerCallback*/
    MME_TransformerInitParams.CallbackUserData = &(CompanionData_p->CommandStatus);
    MME_TransformerInitParams.TransformerInitParamsSize = sizeof(MPEG2Deblock_TransformerInitParams);
    MME_TransformerInitParams.TransformerInitParams_p = &(MPEG2Deblock_TransformerInitParams);

#ifdef STUB_FMW_ST40
    ErrorCode = MMESTUB_InitTransformer(TransformerName, &MME_TransformerInitParams, &PrivateData_p->TranformerHandle);
#else
    ErrorCode = MME_InitTransformer(TransformerName, &MME_TransformerInitParams,&PrivateData_p->TranformerHandle);
#endif

    PrivateData_p->TransformerInitDone= TRUE;
  }


/* Set Global params for MME transformer  */
  PrivateData_p->NewGlobalTransform = StreamInfo_p->IntraQuantMatrixHasChanged || StreamInfo_p->NonIntraQuantMatrixHasChanged;
  if (PrivateData_p->NewGlobalTransform == TRUE)
  {
      SequenceParam_p->StructSize = sizeof(MPEG2Deblock_SequenceParam_t);
      SequenceParam_p->mpeg2 = !(StreamInfo_p->MPEG1BitStream);
      SequenceParam_p->horizontal_size = StreamInfo_p->HorizontalSize;
      SequenceParam_p->vertical_size = StreamInfo_p->VerticalSize;
      if (SequenceParam_p->mpeg2 == TRUE)
      {
        SequenceParam_p->progressive_sequence = StreamInfo_p->progressive_sequence;
        SequenceParam_p->chroma_format  = StreamInfo_p->chroma_format;
      }
      else
      {
        SequenceParam_p->progressive_sequence = 1;
        SequenceParam_p->chroma_format  = MPEG2DEBLOCK_CHROMA_4_2_0; /* for MPEG1, always 4:2:0 */
      }
      for (i = 0; i < MPEG2DEBLOCK_Q_MATRIX_SIZE; i++)
      {
      	SequenceParam_p->intra_quantiser_matrix[i] =   (*(StreamInfo_p->LumaIntraQuantMatrix_p))[i];
      	SequenceParam_p->non_intra_quantiser_matrix[i] = (*(StreamInfo_p->LumaNonIntraQuantMatrix_p))[i];
      	SequenceParam_p->chroma_intra_quantiser_matrix[i] = (*(StreamInfo_p->ChromaIntraQuantMatrix_p))[i];
      	SequenceParam_p->chroma_non_intra_quantiser_matrix[i] = (*(StreamInfo_p->ChromaNonIntraQuantMatrix_p))[i] ;
      }
  }

/* Set Command params for MME transformer: first part */

    TransformParam_p->PictureParam.StructSize                 =   sizeof ( MPEG2Deblock_PictureParam_t );
    TransformParam_p->PictureParam.picture_coding_type        =   StreamInfo_p->Picture.picture_coding_type;

    if (SequenceParam_p->mpeg2 == TRUE)
    {
    TransformParam_p->PictureParam.forward_horizontal_f_code  =   StreamInfo_p->PictureCodingExtension.f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL];
    TransformParam_p->PictureParam.forward_vertical_f_code    =   StreamInfo_p->PictureCodingExtension.f_code[F_CODE_FORWARD][F_CODE_VERTICAL];
    TransformParam_p->PictureParam.backward_horizontal_f_code =   StreamInfo_p->PictureCodingExtension.f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL];
    TransformParam_p->PictureParam.backward_vertical_f_code   =   StreamInfo_p->PictureCodingExtension.f_code[F_CODE_BACKWARD][F_CODE_VERTICAL];
    TransformParam_p->PictureParam.intra_dc_precision         =   StreamInfo_p->PictureCodingExtension.intra_dc_precision;
    TransformParam_p->PictureParam.picture_structure          =   StreamInfo_p->PictureCodingExtension.picture_structure;
    TransformParam_p->PictureParam.mpeg_decoding_flags        =   (U32)StreamInfo_p->PictureCodingExtension.top_field_first<<0 |
                                                                  (U32)StreamInfo_p->PictureCodingExtension.frame_pred_frame_dct<<1 |
                                                                  (U32)StreamInfo_p->PictureCodingExtension.concealment_motion_vectors<<2 |
                                                                  (U32)StreamInfo_p->PictureCodingExtension.q_scale_type<<3 |
                                                                  (U32)StreamInfo_p->PictureCodingExtension.intra_vlc_format<<4 |
                                                                  (U32)StreamInfo_p->PictureCodingExtension.alternate_scan<<5 |
                                                                  (U32)StreamInfo_p->PictureCodingExtension.progressive_frame<<7 ;
    }
    else
    {
    TransformParam_p->PictureParam.forward_horizontal_f_code  =   StreamInfo_p->Picture.full_pel_forward_vector;
    TransformParam_p->PictureParam.forward_vertical_f_code    =   StreamInfo_p->Picture.forward_f_code;
    TransformParam_p->PictureParam.backward_horizontal_f_code =   StreamInfo_p->Picture.full_pel_backward_vector;
    TransformParam_p->PictureParam.backward_vertical_f_code   =   StreamInfo_p->Picture.backward_f_code;
    TransformParam_p->PictureParam.intra_dc_precision         =   MPEG2DEBLOCK_INTRA_DC_PRECISION_8_BITS;
    TransformParam_p->PictureParam.picture_structure          =   MPEG2DEBLOCK_FRAME_TYPE;
    TransformParam_p->PictureParam.mpeg_decoding_flags        =   2; /* to match ref_output */
    }

    TransformParam_p->PictureParam.MBDescr_p = STAVMEM_VirtualToDevice(PrivateData_p->MBDescrBufferAddress_p,&PrivateData_p->VirtualMapping);

    return (ErrorCode);
}/* End of VID_DeblockingSetParams() function */

/*******************************************************************************
Name        : VID_DeblockingStart
Description : Prepare and send MME command for deblocking
Parameters  : HAL Decode manager handle
Assumptions :
Limitations :
Returns     :  None
*******************************************************************************/
void VID_DeblockingStart(VID_Deblocking_Handle_t * const DeblockHandle,MPEG2Deblock_TransformParam_t DeblockTransfromParams)
{
    ST_ErrorCode_t ErrorCode                  = ST_NO_ERROR;
    DEBLOCK_PrivateData_t   *PrivateData_p    = (DEBLOCK_PrivateData_t *) *DeblockHandle;
    DEBLOCK_CompanionData_t *CompanionData_p  = (PrivateData_p->CompanionData_p);
    MPEG2Deblock_TransformParam_t  *TransformParam_p = &(CompanionData_p->TransformParam);

    if (PrivateData_p->NewGlobalTransform == TRUE)
    {
#ifdef STUB_FMW_ST40
      ErrorCode =  MPEG2STUB_SendCommand(PrivateData_p->TranformerHandle, &(CompanionData_p->CmdSeq));
#else
      ErrorCode =  MME_SendCommand(PrivateData_p->TranformerHandle, &(CompanionData_p->CmdSeq));
#endif /* #ifdef STUB_FMW_ST40 */
    }

    /* Set Command params for MME transformer: second part */
    TransformParam_p->StructSize                              =   sizeof ( MPEG2Deblock_TransformParam_t );
    TransformParam_p->DeblockingEnable                        =   DeblockTransfromParams.DeblockingEnable ;
    TransformParam_p->PictureParam.FilterOffsetQP             =   DeblockTransfromParams.PictureParam.FilterOffsetQP ;
    TransformParam_p->PictureStartAddrCompressedBuffer_p      =   (U32*)((U32)DeblockTransfromParams.PictureStartAddrCompressedBuffer_p - 3);
    TransformParam_p->PictureStopAddrCompressedBuffer_p       =   (U32*)((U32)DeblockTransfromParams.PictureStopAddrCompressedBuffer_p - 3 );
    TransformParam_p->PictureBufferAddress.StructSize         =   sizeof ( MPEG2Deblock_PictureBufferAddress_t );
    TransformParam_p->PictureBufferAddress.DecodedLuma_p      =   DeblockTransfromParams.PictureBufferAddress.DecodedLuma_p;
    TransformParam_p->PictureBufferAddress.DecodedChroma_p    =   DeblockTransfromParams.PictureBufferAddress.DecodedChroma_p;
    TransformParam_p->PictureBufferAddress.MainLuma_p         =   DeblockTransfromParams.PictureBufferAddress.MainLuma_p;
    TransformParam_p->PictureBufferAddress.MainChroma_p       =   DeblockTransfromParams.PictureBufferAddress.MainChroma_p;

#ifdef STUB_FMW_ST40
      ErrorCode =  MPEG2STUB_SendCommand(PrivateData_p->TranformerHandle, &(CompanionData_p->CmdPic));
#else
      ErrorCode =  MME_SendCommand(PrivateData_p->TranformerHandle, &(CompanionData_p->CmdPic));
#endif /* #ifdef STUB_FMW_ST40 */


}/* End of VID_DeblockingStart() function */

