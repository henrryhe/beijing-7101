/*******************************************************************************

File name   : halv_dec.h

Description : HAL video decode header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
06 Jul 2000        Created                                           HG
13 Mar 2001        EnableSecondaryDecode prototype changes.          GGn
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_DECODE_H
#define __HAL_VIDEO_DECODE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "vid_mpeg.h"

#ifdef ST_inject
#include "inject.h"
#endif /* ST_inject */

#include "multidec.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Decode mask IT : They are the same for all the HALs */
#define HALDEC_MISALIGNMENT_START_CODE_DETECTOR_AHEAD_MASK  0x00020000
#define HALDEC_MISALIGNMENT_PIPELINE_AHEAD_MASK             0x00010000
#define HALDEC_VLD_READY_MASK                               0x00008000
#define HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK    0x00004000
#define HALDEC_BITSTREAM_FIFO_FULL_MASK                     0x00002000
#define HALDEC_INCONSISTENCY_ERROR_MASK                     0x00001000
#define HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK               0x00000800
#define HALDEC_BIT_BUFFER_NEARLY_FULL_MASK                  0x00000400
#define HALDEC_DECODING_OVERFLOW_MASK                       0x00000200
#define HALDEC_DECODING_SYNTAX_ERROR_MASK                   0x00000100
#define HALDEC_OVERTIME_ERROR_MASK                          0x00000080
#define HALDEC_DECODING_UNDERFLOW_MASK                      0x00000040
#define HALDEC_DECODER_IDLE_MASK                            0x00000020
#define HALDEC_PIPELINE_IDLE_MASK                           0x00000010
#define HALDEC_PIPELINE_START_DECODING_MASK                 0x00000008
#define HALDEC_HEADER_FIFO_EMPTY_MASK                       0x00000004
#define HALDEC_HEADER_FIFO_NEARLY_FULL_MASK                 0x00000002
#define HALDEC_START_CODE_HIT_MASK                          0x00000001

#ifdef STVID_STVAPI_ARCHITECTURE
#define HALDEC_MACROBLOCK_TO_RASTER_IS_IDLE                 0x00000004
#define HALDEC_END_OF_SECONDARY_RASTER_RECONSTRUCTION       0x00000002
#define HALDEC_END_OF_MAIN_RASTER_RECONSTRUCTION            0x00000001
#endif

typedef enum
{
    HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR,
    HALDEC_BIT_BUFFER_HW_MANAGED_LINEAR,
    HALDEC_BIT_BUFFER_SW_MANAGED_CIRCULAR,
    HALDEC_BIT_BUFFER_SW_MANAGED_LINEAR
} HALDEC_BitBufferType_t;

typedef enum
{
    HALDEC_DECODE_PICTURE_MODE_NORMAL,
    HALDEC_DECODE_PICTURE_MODE_FROM_ADDRESS
} HALDEC_DecodePictureMode_t;

typedef enum
{
    HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE,
    HALDEC_DECODE_SKIP_CONSTRAINT_MUST_SKIP,
    HALDEC_DECODE_SKIP_CONSTRAINT_NONE
} HALDEC_DecodeSkipConstraint_t;

typedef enum
{
    HALDEC_SKIP_PICTURE_1_THEN_DECODE,
    HALDEC_SKIP_PICTURE_2_THEN_DECODE,
    HALDEC_SKIP_PICTURE_1_THEN_STOP
} HALDEC_SkipPictureMode_t;

typedef enum
{
    HALDEC_START_CODE_SEARCH_MODE_NORMAL,
    HALDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS
} HALDEC_StartCodeSearchMode_t;

/* Exported Types ----------------------------------------------------------- */

/* Decode Handle type */
typedef void * HALDEC_Handle_t;

typedef struct
{
#ifdef STVID_STVAPI_ARCHITECTURE
    void (*EnablePrimaryRasterReconstruction)(const HALDEC_Handle_t DecodeHandle);
    void (*DisablePrimaryRasterReconstruction)(const HALDEC_Handle_t DecodeHandle);
    void (*EnableSecondaryRasterReconstruction)(const HALDEC_Handle_t DecodeHandle, const STGVOBJ_DecimationFactor_t HSecondaryRasterReconstruction, const STGVOBJ_DecimationFactor_t VSecondaryRasterReconstruction, const STVID_ScanType_t ScanType);
    void (*DisableSecondaryRasterReconstruction)(const HALDEC_Handle_t DecodeHandle);
    U32 (*GetMacroBlockToRasterStatus)(const HALDEC_Handle_t DecodeHandle);
    void (*SetPrimaryRasterReconstructionLumaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p,
                    const U32             Pitch);
    void (*SetPrimaryRasterReconstructionChromaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    void (*SetSecondaryRasterReconstructionLumaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p,
                    const U32             Pitch);
    void (*SetSecondaryRasterReconstructionChromaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
    void (*DecodingSoftReset)(const HALDEC_Handle_t DecodeHandle, const BOOL IsRealTime);
    void (*DisableSecondaryDecode)(const HALDEC_Handle_t DecodeHandle);
    void (*EnableSecondaryDecode)(
                    const HALDEC_Handle_t           DecodeHandle,
                    const STVID_DecimationFactor_t  H_Decimation,
                    const STVID_DecimationFactor_t  V_Decimation,
                    const STVID_ScanType_t          ScanType);
    void (*DisablePrimaryDecode)(const HALDEC_Handle_t DecodeHandle);
    void (*EnablePrimaryDecode)(const HALDEC_Handle_t DecodeHandle);
    void (*DisableHDPIPDecode)(const HALDEC_Handle_t DecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
    void (*EnableHDPIPDecode)(const HALDEC_Handle_t DecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
    void (*FlushDecoder)(const HALDEC_Handle_t DecodeHandle, BOOL * const DiscontinuityStartCodeInserted_p);
    U16  (*Get16BitsHeaderData)(const HALDEC_Handle_t DecodeHandle);
    U16  (*Get16BitsStartCode)(
                    const HALDEC_Handle_t DecodeHandle,
                    BOOL * const          StartCodeOnMSB_p);
    U32 (*Get32BitsHeaderData)(const HALDEC_Handle_t DecodeHandle);
    U32  (*GetAbnormallyMissingInterruptStatus)(const HALDEC_Handle_t DecodeHandle);
    ST_ErrorCode_t (*GetDataInputBufferParams)(const HALDEC_Handle_t DecodeHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p);
    void (*GetDecodeSkipConstraint)(const HALDEC_Handle_t HALDecodeHandle, HALDEC_DecodeSkipConstraint_t * const DecodeSkipConstraint_p);
    U32 (*GetBitBufferOutputCounter)(const HALDEC_Handle_t DecodeHandle);
    ST_ErrorCode_t (*GetCDFifoAlignmentInBytes)(const HALDEC_Handle_t HALDecodeHandle, U32 * const CDFifoAlignment_p);
    U32  (*GetDecodeBitBufferLevel)(const HALDEC_Handle_t DecodeHandle);
#ifdef STVID_HARDWARE_ERROR_EVENT
    STVID_HardwareError_t  (*GetHardwareErrorStatus)(const HALDEC_Handle_t DecodeHandle);
#endif
    U32  (*GetInterruptStatus)(const HALDEC_Handle_t DecodeHandle);
    ST_ErrorCode_t (*GetLinearBitBufferTransferedDataSize)(const HALDEC_Handle_t DecodeHandle, U32 * const TransferedDataSize_p);
    BOOL (*GetPTS)(
                    const HALDEC_Handle_t DecodeHandle,
                    U32 *  const          PTS_p,
                    BOOL * const          PTS33_p);

    ST_ErrorCode_t  (*GetSCDAlignmentInBytes)(const HALDEC_Handle_t HALDecodeHandle, U32 * const SCDAlignment_p);
    ST_ErrorCode_t (*GetStartCodeAddress)(
                    const HALDEC_Handle_t HALDecodeHandle,
                    void ** const BufferAddress_p);
    U32 (*GetStatus)(const HALDEC_Handle_t DecodeHandle);
    ST_ErrorCode_t (*Init)(const HALDEC_Handle_t DecodeHandle);
    BOOL (*IsHeaderDataFIFOEmpty)(const HALDEC_Handle_t DecodeHandle);
    void (*LoadIntraQuantMatrix)(
                    const HALDEC_Handle_t     DecodeHandle,
                    const QuantiserMatrix_t * Matrix_p,
                    const QuantiserMatrix_t * ChromaMatrix_p);
    void (*LoadNonIntraQuantMatrix)(
                    const HALDEC_Handle_t  DecodeHandle,
                    const QuantiserMatrix_t * Matrix_p,
                    const QuantiserMatrix_t * ChromaMatrix_p);
    void (*PipelineReset)(const HALDEC_Handle_t  DecodeHandle);
    void (*PrepareDecode)(
                    const HALDEC_Handle_t   DecodeHandle,
                    void * const            DecodeAddressFrom_p,
                    const U16               pTemporalReference,
                    BOOL * const            WaitForVLD_p);
    void (*ResetPESParser)(const HALDEC_Handle_t DecodeHandle);
    void (*SearchNextStartCode)(
                    const HALDEC_Handle_t              DecodeHandle,
                    const HALDEC_StartCodeSearchMode_t SearchMode,
                    const BOOL                         FirstSliceDetect,
                    void * const                       SearchAddressFrom_p);
    void (*SetBackwardChromaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    void (*SetBackwardLumaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    ST_ErrorCode_t (* SetDataInputInterface)(
           const HALDEC_Handle_t           DecodeHandle,
           ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                                void ** const Address_p),
           void (*InformReadAddress)(void * const Handle, void * const Address),
            void * const Handle );
    void (*SetDecodeBitBuffer)(
                    const HALDEC_Handle_t           DecodeHandle,
                    void * const                    BufferAddressStart_p,
                    const U32                       BufferSize,
                    const HALDEC_BitBufferType_t    BitBufferType);
    void (*SetDecodeBitBufferThreshold)(
                    const HALDEC_Handle_t DecodeHandle,
                    const U32             BitBufferThreshold);
    void (*SetDecodeChromaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    void (*SetDecodeConfiguration)(
                    const HALDEC_Handle_t               DecodeHandle,
                    const StreamInfoForDecode_t * const StreamInfo_p);
    void (*SetDecodeLumaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    void (*SetForwardChromaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    void (*SetForwardLumaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
#ifdef ST_deblock
    void (*SetDeblockingMode)(
                    const HALDEC_Handle_t DecodeHandle,
                    const BOOL DeblockingEnabled         );
#ifdef VIDEO_DEBLOCK_DEBUG
    void (*SetDeblockingStrength)(
                    const HALDEC_Handle_t DecodeHandle,
                    const int DeblockingStrength);
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */
    ST_ErrorCode_t (*GetPictureDescriptors)(
            const HALDEC_Handle_t HALDecodeHandle,
            STVID_PictureDescriptors_t * PictureDescriptors_p);
    void (*SetFoundStartCode)(
                    const HALDEC_Handle_t DecodeHandle,
                    const U8              StartCode,
                    BOOL * const FirstPictureStartCodeFound_p);
    void (*SetInterruptMask)(
                    const HALDEC_Handle_t DecodeHandle,
                    const U32             Mask);
    void (*SetMainDecodeCompression)(
                    const HALDEC_Handle_t           DecodeHandle,
                    const STVID_CompressionLevel_t  Compression);
    void (*SetSecondaryDecodeChromaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    void (*SetSecondaryDecodeLumaFrameBuffer)(
                    const HALDEC_Handle_t DecodeHandle,
                    void * const          BufferAddress_p);
    void (*SetSkipConfiguration)(
                    const HALDEC_Handle_t               DecodeHandle,
                    const STVID_PictureStructure_t *    const PictureStructure_p);
    ST_ErrorCode_t (*SetSmoothBackwardConfiguration)(
                    const HALDEC_Handle_t DecodeHandle,
                    const BOOL            SetNotReset);
    void (*SetStreamID)(
                    const HALDEC_Handle_t DecodeHandle,
                    const U8 StreamID);
    void (*SetStreamType)(
                    const HALDEC_Handle_t    DecodeHandle,
                    const STVID_StreamType_t StreamType);
    ST_ErrorCode_t (*Setup)(
                    const HALDEC_Handle_t     DecodeHandle,
                    const STVID_SetupParams_t * const SetupParams_p);
    void (*SkipPicture)(
                    const HALDEC_Handle_t          DecodeHandle,
                    const BOOL                     ExecuteOnlyOnNextVsync,
                    const HALDEC_SkipPictureMode_t SkipMode);
    void (*StartDecodePicture)(
                    const HALDEC_Handle_t           DecodeHandle,
                    const BOOL                      ExecuteOnlyOnNextVsync,
                    const BOOL                      MainDecodeOverWrittingDisplay,
                    const BOOL                      SecondaryDecodeOverWrittingDisplay);
#ifdef ST_SecondInstanceSharingTheSameDecoder
    void (*StartDecodeStillPicture)(
                    const HALDEC_Handle_t           DecodeHandle,
                    const void *                    BitBufferAddress_p,
                    const VIDBUFF_PictureBuffer_t * DecodedPicture_p,
                    const StreamInfoForDecode_t * StreamInfo_p);
#endif /* ST_SecondInstanceSharingTheSameDecoder */
    void (*Term)(const HALDEC_Handle_t DecodeHandle);
#ifdef STVID_DEBUG_GET_STATUS
	void (*GetDebugStatus)(
	                const HALDEC_Handle_t           DecodeHandle,
                    STVID_Status_t * const          Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
	void (*WriteStartCode)(
                    const HALDEC_Handle_t  HALDecodeHandle,
                    const U32             SCVal,
                    const void * const    SCAdd_p);
} HALDEC_FunctionsTable_t;

/* Parameters required to initialise HALDEC */
typedef struct
{
    ST_Partition_t *    CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STVID_DeviceType_t  DeviceType;
    void *              RegistersBaseAddress_p;  /* Base address of the HALDEC registers */
    void *              CompressedDataInputBaseAddress_p; /* Base address of video compressed input registers (sdmpgo2 only) */
    void *              SharedMemoryBaseAddress_p;
    ST_DeviceName_t     EventsHandlerName;
    ST_DeviceName_t     VideoName;              /* Name of the video instance */
    U8                  DecoderNumber;  /* As defined in vid_com.c. See definition there. */
#ifdef ST_inject
    VIDINJ_Handle_t     InjecterHandle;
#endif /* ST_inject */
#ifdef ST_multidecodeswcontroller
    VIDDEC_MultiDecodeHandle_t MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
    STAVMEM_PartitionHandle_t   AvmemPartition;
    struct
    {
        U32                     Event;
        U32                     Level;
        U32                     Number;
    }SharedItParams;
    U32                     BitBufferSize;
} HALDEC_InitParams_t;

typedef struct {
    const HALDEC_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *        CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t          EventsHandle;
    ST_DeviceName_t         VideoName;              /* Name of the video instance */
    STVID_DeviceType_t      DeviceType;
    void *                  RegistersBaseAddress_p;  /* Base address of the HALDEC registers */
    void *                  CompressedDataInputBaseAddress_p; /* Base address of Compressed data input registers */
    U32                     ValidityCheck;
    void *                  SDRAMMemoryBaseAddress_p;
    U8                      DecoderNumber;  /* As defined in vid_com.c. See definition there. */
#ifdef ST_inject
    VIDINJ_Handle_t         InjecterHandle;
#endif /* ST_inject */
#ifdef ST_multidecodeswcontroller
    VIDDEC_MultiDecodeHandle_t MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
    STAVMEM_PartitionHandle_t   AvmemPartition;
    /* If It is handled by the hal : */
    U32                     InterruptEvt;
    U32                     InterruptLevel;
    U32                     InterruptNumber;
    /* Bitbuffer allocation */
    U32                     BitBufferSize;
    void *                  PrivateData_p;
} HALDEC_Properties_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */
#ifdef STVID_STVAPI_ARCHITECTURE
#define HALDEC_EnablePrimaryRasterReconstruction(DecodeHandle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->EnablePrimaryRasterReconstruction(DecodeHandle)
#define HALDEC_DisablePrimaryRasterReconstruction(DecodeHandle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->DisablePrimaryRasterReconstruction(DecodeHandle)
#define HALDEC_EnableSecondaryRasterReconstruction(DecodeHandle, HSecondaryRasterReconstruction, VSecondaryRasterReconstruction, ScanType)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->EnableSecondaryRasterReconstruction(DecodeHandle, HSecondaryRasterReconstruction, VSecondaryRasterReconstruction, ScanType)
#define HALDEC_DisableSecondaryRasterReconstruction(DecodeHandle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->DisableSecondaryRasterReconstruction(DecodeHandle)
#define HALDEC_GetMacroBlockToRasterStatus(DecodeHandle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetMacroBlockToRasterStatus(DecodeHandle)
#define HALDEC_SetPrimaryRasterReconstructionLumaFrameBuffer(DecodeHandle, BufferAddress_p, Pitch) \
     ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetPrimaryRasterReconstructionLumaFrameBuffer(DecodeHandle, BufferAddress_p, Pitch)
#define HALDEC_SetPrimaryRasterReconstructionChromaFrameBuffer(DecodeHandle, BufferAddress_p) \
     ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetPrimaryRasterReconstructionChromaFrameBuffer(DecodeHandle, BufferAddress_p)
#define HALDEC_SetSecondaryRasterReconstructionLumaFrameBuffer(DecodeHandle, BufferAddress_p, Pitch) \
     ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetSecondaryRasterReconstructionLumaFrameBuffer(DecodeHandle, BufferAddress_p, Pitch)
#define HALDEC_SetSecondaryRasterReconstructionChromaFrameBuffer(DecodeHandle, BufferAddress_p) \
     ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetSecondaryRasterReconstructionChromaFrameBuffer(DecodeHandle, BufferAddress_p)
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

#define HALDEC_DecodingSoftReset(DecodeHandle, IsRealTime)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->DecodingSoftReset(DecodeHandle, IsRealTime)

#define HALDEC_DisableSecondaryDecode(DecodeHandle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->DisableSecondaryDecode(DecodeHandle)

#define HALDEC_EnableSecondaryDecode(DecodeHandle,H_Decimation,V_Decimation,ScanType)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->EnableSecondaryDecode(DecodeHandle,H_Decimation,V_Decimation,ScanType)

#define HALDEC_DisablePrimaryDecode(DecodeHandle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->DisablePrimaryDecode(DecodeHandle)

#define HALDEC_EnablePrimaryDecode(DecodeHandle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->EnablePrimaryDecode(DecodeHandle)
#define HALDEC_EnableHDPIPDecode(DecodeHandle,HDPIPParams_p)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->EnableHDPIPDecode(DecodeHandle,HDPIPParams_p)
#define HALDEC_DisableHDPIPDecode(DecodeHandle,HDPIPParams_p)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->DisableHDPIPDecode(DecodeHandle,HDPIPParams_p)

#define HALDEC_FlushDecoder(DecodeHandle, DiscontinuityStartCodeInserted_p)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->FlushDecoder(DecodeHandle,DiscontinuityStartCodeInserted_p)
#define HALDEC_Get16BitsStartCode(DecodeHandle,StartCodeOnMSB_p)                                                ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->Get16BitsStartCode(DecodeHandle,StartCodeOnMSB_p)
#define HALDEC_Get16BitsHeaderData(DecodeHandle)                                                                ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->Get16BitsHeaderData(DecodeHandle)
#define HALDEC_Get32BitsHeaderData(DecodeHandle)                                                                ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->Get32BitsHeaderData(DecodeHandle)
#define HALDEC_GetAbnormallyMissingInterruptStatus(DecodeHandle)                                                ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetAbnormallyMissingInterruptStatus(DecodeHandle)
#define HALDEC_GetDecodeSkipConstraint(DecodeHandle,DecodeSkipConstraint)                                       ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetDecodeSkipConstraint(DecodeHandle,DecodeSkipConstraint)
#define HALDEC_GetBitBufferOutputCounter(DecodeHandle)                                                          ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetBitBufferOutputCounter(DecodeHandle)
#define HALDEC_GetCDFifoAlignmentInBytes(DecodeHandle, CDFifoAlignment_p)                                       ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetCDFifoAlignmentInBytes(DecodeHandle, CDFifoAlignment_p)
#define HALDEC_GetDecodeBitBufferLevel(DecodeHandle)                                                            ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetDecodeBitBufferLevel(DecodeHandle)
#define HALDEC_GetDataInputBufferParams(DecodeHandle,BaseAddress_p,Size_p)                                      ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetDataInputBufferParams(DecodeHandle,BaseAddress_p,Size_p)
#ifdef STVID_HARDWARE_ERROR_EVENT
#define HALDEC_GetHardwareErrorStatus(DecodeHandle)                                                             ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetHardwareErrorStatus(DecodeHandle)
#endif
#define HALDEC_GetInterruptStatus(DecodeHandle)                                                                 ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetInterruptStatus(DecodeHandle)
#define HALDEC_GetLinearBitBufferTransferedDataSize(DecodeHandle,TransferedDataSize_p)                          ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetLinearBitBufferTransferedDataSize(DecodeHandle,TransferedDataSize_p)
#define HALDEC_GetPTS(DecodeHandle,PTS_p,PTS33_p)                                                               ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetPTS(DecodeHandle,PTS_p,PTS33_p)
#define HALDEC_GetSCDAlignmentInBytes(DecodeHandle,SCDAlignment_p)                                              ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetSCDAlignmentInBytes(DecodeHandle, SCDAlignment_p)
#define HALDEC_GetStatus(DecodeHandle)                                                                          ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetStatus(DecodeHandle)
#define HALDEC_GetStartCodeAddress(DecodeHandle, BufferAddress_p)                                               ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetStartCodeAddress(DecodeHandle, BufferAddress_p)
#define HALDEC_IsHeaderDataFIFOEmpty(DecodeHandle)                                                              ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->IsHeaderDataFIFOEmpty(DecodeHandle)
#define HALDEC_LoadIntraQuantMatrix(DecodeHandle,Matrix_p,ChromaMatrix_p)                                       ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->LoadIntraQuantMatrix(DecodeHandle,Matrix_p,ChromaMatrix_p)
#define HALDEC_LoadNonIntraQuantMatrix(DecodeHandle,Matrix_p,ChromaMatrix_p)                                    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->LoadNonIntraQuantMatrix(DecodeHandle,Matrix_p,ChromaMatrix_p)
#define HALDEC_PipelineReset(DecodeHandle)                                                                      ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->PipelineReset(DecodeHandle)
#define HALDEC_PrepareDecode(DecodeHandle,DecodeAddressFrom_p,pTemporalReference,WaitForVLD_p)                  ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->PrepareDecode(DecodeHandle,DecodeAddressFrom_p,pTemporalReference, WaitForVLD_p)
#define HALDEC_ResetPESParser(DecodeHandle)                                                                     ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->ResetPESParser(DecodeHandle)
#define HALDEC_SearchNextStartCode(DecodeHandle,SearchMode,FirstSliceDetect,SearchAddressFrom_p)                ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SearchNextStartCode(DecodeHandle,SearchMode,FirstSliceDetect,SearchAddressFrom_p)
#define HALDEC_SetBackwardChromaFrameBuffer(DecodeHandle,BufferAddress_p)                                       ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetBackwardChromaFrameBuffer(DecodeHandle,BufferAddress_p)
#define HALDEC_SetBackwardLumaFrameBuffer(DecodeHandle,BufferAddress_p)                                         ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetBackwardLumaFrameBuffer(DecodeHandle,BufferAddress_p)
#define HALDEC_SetDecodeBitBuffer(DecodeHandle,BufferAddressStart_p,BufferSize,BufferType)                      ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDecodeBitBuffer(DecodeHandle,BufferAddressStart_p,BufferSize,BufferType)
#define HALDEC_SetDecodeBitBufferThreshold(DecodeHandle,BitBufferThresholdAddress_p)                            ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDecodeBitBufferThreshold(DecodeHandle,BitBufferThresholdAddress_p)
#define HALDEC_SetDecodeChromaFrameBuffer(DecodeHandle,BufferAddress_p)                                         ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDecodeChromaFrameBuffer(DecodeHandle,BufferAddress_p)
#define HALDEC_SetDecodeConfiguration(DecodeHandle,StreamInfo_p)                                                ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDecodeConfiguration(DecodeHandle,StreamInfo_p)
#define HALDEC_SetDecodeLumaFrameBuffer(DecodeHandle,BufferAddress_p)                                           ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDecodeLumaFrameBuffer(DecodeHandle,BufferAddress_p)
#define HALDEC_SetForwardChromaFrameBuffer(DecodeHandle,BufferAddress_p)                                        ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetForwardChromaFrameBuffer(DecodeHandle,BufferAddress_p)
#define HALDEC_SetForwardLumaFrameBuffer(DecodeHandle,BufferAddress_p)                                          ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetForwardLumaFrameBuffer(DecodeHandle,BufferAddress_p)
#ifdef ST_deblock
#define HALDEC_SetDeblockingMode(DecodeHandle, DeblockingEnabled)                                               ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDeblockingMode(DecodeHandle, DeblockingEnabled)
#ifdef VIDEO_DEBLOCK_DEBUG
#define HALDEC_SetDeblockingStrength(DecodeHandle, DeblockingStrength)                                          ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDeblockingStrength(DecodeHandle, DeblockingStrength)
#endif /* VIDEO_DEBLOCK_DEBUG */
#endif /* ST_deblock */
#define HALDEC_GetPictureDescriptors(DecodeHandle, PictureDescriptors_p)                                        ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetPictureDescriptors(DecodeHandle, PictureDescriptors_p)
#define HALDEC_SetFoundStartCode(DecodeHandle,StartCode,FirstPictureStartCodeFound_p)                           ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetFoundStartCode(DecodeHandle,StartCode,FirstPictureStartCodeFound_p)
#define HALDEC_SetInterruptMask(DecodeHandle,Mask)                                                              ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetInterruptMask(DecodeHandle,Mask)
#define HALDEC_SetMainDecodeCompression(DecodeHandle,Compression)                                               ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetMainDecodeCompression(DecodeHandle,Compression)
#define HALDEC_SetSecondaryDecodeChromaFrameBuffer(DecodeHandle,BufferAddress_p)                                ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetSecondaryDecodeChromaFrameBuffer(DecodeHandle,BufferAddress_p)
#define HALDEC_SetSecondaryDecodeLumaFrameBuffer(DecodeHandle,BufferAddress_p)                                  ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetSecondaryDecodeLumaFrameBuffer(DecodeHandle,BufferAddress_p)
#define HALDEC_SetSkipConfiguration(DecodeHandle,PictureStructure_p)                                            ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetSkipConfiguration(DecodeHandle,PictureStructure_p)
#define HALDEC_SetSmoothBackwardConfiguration(DecodeHandle,SetNotReset)                                         ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetSmoothBackwardConfiguration(DecodeHandle,SetNotReset)
#define HALDEC_SetStreamID(DecodeHandle,StreamID)                                                               ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetStreamID(DecodeHandle,StreamID)
#define HALDEC_SetStreamType(DecodeHandle,StreamType)                                                           ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetStreamType(DecodeHandle,StreamType)
#define HALDEC_Setup(DecodeHandle, SetupParams_p)                                                               ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->Setup(DecodeHandle, SetupParams_p)
#define HALDEC_SkipPicture(DecodeHandle,OnlyOnNextVsync,Skip)                                                   ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SkipPicture(DecodeHandle,OnlyOnNextVsync,Skip)
#define HALDEC_StartDecodePicture(DecodeHandle,OnlyOnNextVsync,MainDecodeOverWrittingDisplay,SecondaryDecodeOverWrittingDisplay)         ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->StartDecodePicture(DecodeHandle,OnlyOnNextVsync,MainDecodeOverWrittingDisplay,SecondaryDecodeOverWrittingDisplay)
#ifdef ST_SecondInstanceSharingTheSameDecoder
#define HALDEC_StartDecodeStillPicture(DecodeHandle,BitBufferAddress_p, DecodedPicture_p, StreamInfo_p)         ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->StartDecodeStillPicture(DecodeHandle,BitBufferAddress_p, DecodedPicture_p, StreamInfo_p)
#endif /* ST_SecondInstanceSharingTheSameDecoder */
#define HALDEC_SetDataInputInterface(DecodeHandle,GetWriteAddress,InformReadAddress,Handle)\
    ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDataInputInterface(DecodeHandle,GetWriteAddress,InformReadAddress,Handle)
#define HALDEC_WriteStartCode(DecodeHandle,SCVal,SCAdd_p)                                       ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->WriteStartCode(DecodeHandle,SCVal,SCAdd_p)
#ifdef STVID_DEBUG_GET_STATUS
#define HALDEC_GetDebugStatus(DecodeHandle,Status_p)                                                            ((HALDEC_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetDebugStatus(DecodeHandle,Status_p)
#endif /* STVID_DEBUG_GET_STATUS */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t HALDEC_Init(const HALDEC_InitParams_t * const InitParams_p, HALDEC_Handle_t * const DecodeHandle_p);
ST_ErrorCode_t HALDEC_Term(const HALDEC_Handle_t DecodeHandle);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_DECODE_H */

/* End of halv_dec.h */
