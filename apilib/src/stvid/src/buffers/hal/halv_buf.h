/*******************************************************************************

File name   : halv_buf.h

Description : HAL video buffer manager header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
06 Jul 2000        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_BUFFER_MANAGER_H
#define __HAL_VIDEO_BUFFER_MANAGER_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "buffers.h"

#include "stavmem.h"

#ifdef ST_XVP_ENABLE_FGT
#include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */

typedef void * HALBUFF_Handle_t;


/* Parameters required to allocate a buffer */
/*typedef struct*/
/*{*/
/*    U32     BufferSize;*/
/*    VIDBUFF_AllocationMode_t AllocationMode;*/
/*} HALBUFF_AllocateBufferParams_t;*/
typedef VIDBUFF_AllocateBufferParams_t HALBUFF_AllocateBufferParams_t;

/* Parameters required to know everything about a buffer */
/*typedef struct*/
/*{*/
/*    void *  Address_p;*/
/*    U32     Size;*/
/*    VIDBUFF_AllocationMode_t AllocationMode;*/
/*    STAVMEM_BlockHandle_t AvmemBlockHandle;*/ /* Used if AllocationMode is SHARED_MEMORY */
/*    ST_Partition_t * CPUPartition_p;       */ /* Used if AllocationMode is CPU_PARTITION */
/*} HALBUFF_BufferParams_t;*/

typedef struct
{
/*    ST_ErrorCode_t (*AllocateBitBuffer)(*/
/*                    const HALBUFF_Handle_t BuffersHandle,*/
/*                    const HALBUFF_AllocateBufferParams_t * const AllocParams_p,*/
/*                    VIDBUFF_BufferAllocationParams_t * const BufferParams_p);*/
/*    ST_ErrorCode_t (*AllocateFrameBuffer)(*/
/*                    const HALBUFF_Handle_t BuffersHandle,*/
/*                    const HALBUFF_AllocateBufferParams_t * const AllocParams_p,*/
/*                    VIDBUFF_BufferAllocationParams_t * const BufferParams_p);*/
/*    ST_ErrorCode_t (*DeAllocateBitBuffer)(*/
/*                    const HALBUFF_Handle_t BuffersHandle,*/
/*                    VIDBUFF_BufferAllocationParams_t * const BufferParams_p);*/
/*    ST_ErrorCode_t (*DeAllocateFrameBuffer)(*/
/*                    const HALBUFF_Handle_t BuffersHandle,*/
/*                    VIDBUFF_BufferAllocationParams_t * const BufferParams_p);*/
    ST_ErrorCode_t (*Init)(const HALBUFF_Handle_t DecodeHandle);
    void (*Term)(const HALBUFF_Handle_t DecodeHandle);

    /* Functions for internal use */
    U32 (*GetBitBufferAlignment)(const HALBUFF_Handle_t BuffersHandle);
    U32 (*GetFrameBufferAlignment)(const HALBUFF_Handle_t BuffersHandle);
    void (*ApplyCompressionLevelToPictureSize)(const HALBUFF_Handle_t BufferHandle,const STVID_CompressionLevel_t CompressionLevel,
            U32 * const PictureWidth, U32 * const PictureHeight);
    void (*ApplyDecimationFactorToPictureSize)(const HALBUFF_Handle_t BufferHandle,const STVID_DecimationFactor_t DecimationFactor,
            U32 * const PictureWidth, U32 * const PictureHeight);
    void (*ComputeBufferSizes)(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p,
            U32 * const LumaBufferSize_p, U32 * const ChromaBufferSize_p);
    void (*GetForbiddenBorders)(const HALBUFF_Handle_t BufferHandle, U32 * const NumberOfForbiddenBorders_p, void ** const ForbiddenBorderArray_p);
} HALBUFF_FunctionsTable_t;


/* Parameters required to initialise HALBUFF */
typedef struct
{
    ST_Partition_t *    CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STVID_DeviceType_t  DeviceType;
    void *              SharedMemoryBaseAddress_p;
    VIDBUFF_BufferType_t FrameBuffersType;      /* Type required for all the frame buffers in the instance:
                        * VIDBUFF_BUFFER_FRAME_12BITS_PER_PIXEL for decode which fills frame buffers in 4:2:0
                        * VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL for digital input which fills frame buffers in 4:2:2
                                                 * or HD-PIP.   */

    STAVMEM_PartitionHandle_t AvmemPartitionHandle;
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t     FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
} HALBUFF_InitParams_t;

typedef struct HALBUFF_Infos_s
{
    U32     FrameBufferAdditionalDataBytesPerMB;  /* Size of additionnal data accociated to FB in bytes/MB (H264 DeltaPhy IP MB structure storage) */
} HALBUFF_Infos_t;

typedef struct {
    const HALBUFF_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *    CPUPartition_p; /* Where the module can allocate */
                                        /* memory for its internal usage */
    STVID_DeviceType_t  DeviceType;
    void *              SharedMemoryBaseAddress_p;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    U32                 ValidityCheck;
    STAVMEM_PartitionHandle_t AvmemPartitionHandle;
    STAVMEM_PartitionHandle_t AvmemPartitionHandleForFrameBuffers;
    STAVMEM_PartitionHandle_t AvmemPartitionHandleForDecimatedFrameBuffers;
    STAVMEM_PartitionHandle_t AvmemPartitionHandleForAdditionnalData;
    STAVMEM_PartitionHandle_t AvmemPartitionHandleForBitBuffer;
#if defined(DVD_SECURED_CHIP)
    STAVMEM_PartitionHandle_t AvmemPartitionHandleForESCopyBuffer;
#endif /* DVD_SECURED_CHIP */
    VIDBUFF_BufferType_t FrameBuffersType;      /* Type required for all the */
                                          /*frame buffers in the instance:   */
       /* VIDBUFF_BUFFER_FRAME_12BITS_PER_PIXEL for decode which fills frame buffers in 4:2:0*/
       /* VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL for digital input which fills frame buffers in 4:2:2
        * or HD-PIP.    */
    U32                 FrameBufferAdditionalDataBytesPerMB;
    void *              PrivateData_p;
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t     FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
} HALBUFF_Data_t;

typedef enum HALBUFF_FrameBufferType_e
{
    HALBUFF_FRAMEBUFFER_PRIMARY,
    HALBUFF_FRAMEBUFFER_DECIMATED
} HALBUFF_FrameBufferType_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t HALBUFF_SetInfos(const HALBUFF_Handle_t BufferHandle, HALBUFF_Infos_t * const Infos_p);
ST_ErrorCode_t HALBUFF_AllocateBitBuffer(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t HALBUFF_AllocateFrameBuffer(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p, const HALBUFF_FrameBufferType_t FrameBufferType);
ST_ErrorCode_t HALBUFF_ApplyCompressionLevelToPictureSize(const HALBUFF_Handle_t BufferHandle , const STVID_CompressionLevel_t CompressionLevel,
        U32 * const PictureWidth_p, U32 * const PictureHeight_p);
ST_ErrorCode_t HALBUFF_ApplyDecimationFactorToPictureSize(const HALBUFF_Handle_t BufferHandle , const STVID_DecimationFactor_t DecimationFactor,
        U32 * const PictureWidth_p, U32 * const PictureHeight_p);
ST_ErrorCode_t HALBUFF_DeAllocateBitBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t HALBUFF_DeAllocateFrameBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p, const HALBUFF_FrameBufferType_t FrameBufferType);
void HALBUFF_GetFrameBufferParams(const HALBUFF_Handle_t BufferHandle, const U32 PictureWidth, const U32 PictureHeight, U32 * const TotalSize_p, U32 * const Aligment_p);
ST_ErrorCode_t HALBUFF_Init(const HALBUFF_InitParams_t * const InitParams_p, HALBUFF_Handle_t * const BufferHandle_p);
ST_ErrorCode_t HALBUFF_Term(const HALBUFF_Handle_t BufferHandle);
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForFrameBuffers(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle, BOOL * AvmemPartitionHandleHasChanged_p);
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForDecimatedFrameBuffers(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle, BOOL * AvmemPartitionHandleHasChanged_p);
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForAdditionnalData(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle);
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForBitBuffer(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle);
ST_ErrorCode_t HALBUFF_ClearFrameBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p, const STVID_ClearParams_t * const ClearFrameBufferParams_p);
ST_ErrorCode_t HALBUFF_GetAdditionnalDataBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p);
ST_ErrorCode_t HALBUFF_FreeAdditionnalDataBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p);

#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t HALBUFF_AllocateESCopyBuffer(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t HALBUFF_DeAllocateESCopyBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForESCopyBuffer(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle);
#endif /* DVD_SECURED_CHIP */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_BUFFER_MANAGER_H */

/* End of halv_buf.h */
