/*******************************************************************************

File name   : halv_buf.c

Description : HAL video buffer manager source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Jul 2000        Created                                           HG
03 Mar 2001        Compression and decimation management.            GG
25 Jul 2001        Frame buffer Alloc. with picture height and width GG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Temporary: work-arounds 7015 still pending on cut 1.2 */
/*#define STI7015_WA_BBL_SPURIOUS_OVERFLOW*/

/* Workarounds disappearing on cut 1.2 */
/* none */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "vid_ctcm.h"

#include "halv_buf.h"

#ifdef ST_omega1
#include "hv_buf1.h"
#endif
#if defined ST_omega2
#include "hv_buf2.h"
#endif
#ifdef ST_genbuff
#include "genbuff.h"
#endif
#if defined ST_genbuff
#include "genbuff.h"
#endif

#include "stavmem.h"
#include "sttbx.h"

#if defined TRACE_UART
#include "trace.h"
#endif

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
#define LUMA_WORD_FOR_BLACK_COLOR       0x10101010
#define CHROMA_WORD_FOR_BLACK_COLOR     0x80808080

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define HALBUFF_VALID_HANDLE 0x0145145f

/* Work-around for STi7015 bit buffer level overflow: add 500KB more bit buffer size ! */
/* Also applied to STi7020, until this bug is not resolved.                            */
#define STI7015_WA_BBL_SPURIOUS_OVERFLOW_ADDITIONAL_SIZE (512 * 1024)

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */
#ifdef ST_XVP_ENABLE_FGT
static void ComputeAccumulationBufferSizes (const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p,
        U32 * const LumaAccumBufferSize_p, U32 * const ChromaAccumBufferSize_p);
#endif /* ST_XVP_ENABLE_FGT */

/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : HALBUFF_SetInfos
Description : Set informations comming from appli or other modules
Parameters  : Video buffer hal handle

Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if NULL pointer
*******************************************************************************/
ST_ErrorCode_t HALBUFF_SetInfos(const HALBUFF_Handle_t BufferHandle, HALBUFF_Infos_t * const Infos_p)
{
    HALBUFF_Data_t * HALBUFF_Data_p = (HALBUFF_Data_t *) BufferHandle;

/* TODO (PC) : Check if change is allowed. Could be useful to avoid it if frame buffers are already allocated ? */
    HALBUFF_Data_p->FrameBufferAdditionalDataBytesPerMB = Infos_p->FrameBufferAdditionalDataBytesPerMB;

    return(ST_NO_ERROR);
} /* End of HALBUFF_SetInfos() function */

/*******************************************************************************
Name        : HALBUFF_AllocateBitBuffer
Description : Allocate memory for bit buffer
Parameters  : HAL buffer manager handle,
Assumptions : Pointers are not NULL
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
ST_ErrorCode_t HALBUFF_AllocateBitBuffer(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    STAVMEM_FreeBlockParams_t AvmemFreeParams;
    STAVMEM_BlockHandle_t BitBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    void * BufferAddr;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    void * VirtualAddress;

    switch (AllocParams_p->AllocationMode)
    {
        case VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY :
            /* Get alignment depending on the HAL */
            AvmemAllocParams.Alignment = ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->GetBitBufferAlignment(BufferHandle);

            /* Allocate bit buffer in specific partition */
            AvmemAllocParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForBitBuffer;

            /* Save the reference of the partition where this buffer is going to be allocated */
            BufferParams_p->PartitionHandle = AvmemAllocParams.PartitionHandle;

            AvmemAllocParams.Size = AllocParams_p->BufferSize;
#ifdef STI7015_WA_BBL_SPURIOUS_OVERFLOW
            if ( (((HALBUFF_Data_t *) BufferHandle)->DeviceType == STVID_DEVICE_TYPE_7015_MPEG) ||
                 (((HALBUFF_Data_t *) BufferHandle)->DeviceType == STVID_DEVICE_TYPE_7020_MPEG) )
            {
                /* Work-around on STi7015 because of spurious BBL overflows: sometimes
                the BBL can go up to 450KB higher than the bit buffer size, which should not be possible ! */
                AvmemAllocParams.Size += STI7015_WA_BBL_SPURIOUS_OVERFLOW_ADDITIONAL_SIZE;
            }
#endif /* STI7015_WA_BBL_SPURIOUS_OVERFLOW */
            AvmemAllocParams.Size = ((AvmemAllocParams.Size + AvmemAllocParams.Alignment - 1) & (~(AvmemAllocParams.Alignment - 1)));
            AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM; /* !!! ??? */

            AvmemAllocParams.NumberOfForbiddenRanges = 0;
            AvmemAllocParams.ForbiddenRangeArray_p = NULL;
            AvmemAllocParams.NumberOfForbiddenBorders = 0;
            AvmemAllocParams.ForbiddenBorderArray_p = NULL;
            Err = STAVMEM_AllocBlock(&AvmemAllocParams, &BitBufferHandle);
            if (Err != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate bit buffer !"));
                Err = ST_ERROR_NO_MEMORY;
            }
            else
            {
                Err = STAVMEM_GetBlockAddress(BitBufferHandle, &VirtualAddress);
                BufferAddr = STAVMEM_VirtualToDevice(VirtualAddress,
                      &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
                if (Err != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of bit buffer !"));
                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &BitBufferHandle);
                    Err = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    /* Return params of bit buffer */
                    BufferParams_p->TotalSize       = AvmemAllocParams.Size;
                    BufferParams_p->Address_p       = BufferAddr;
                    BufferParams_p->AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY;
                    BufferParams_p->AvmemBlockHandle = BitBufferHandle;
                    BufferParams_p->BufferType      = VIDBUFF_BUFFER_SIMPLE;
                }
            }
            break;

/*        case VIDBUFF_ALLOCATION_MODE_CPU_PARTITION :*/
                /* Problem with CPU partition alloc: handle alignment constraints ! */
        default :
            Err = ST_ERROR_NO_MEMORY;
    }

    if (Err == ST_ERROR_NO_MEMORY)
    {
        /* Set size to 0, to "help" user not using it */
        BufferParams_p->TotalSize = 0;
    }

    return(Err);
} /* End of HALBUFF_AllocateBitBuffer() function */


/*******************************************************************************
Name        : HALBUFF_AllocateFrameBuffer
Description : Allocate memory for frame buffer
Parameters  : HAL buffer manager handle,
Assumptions : Size is the size of the picture in pixels (width * height).
              1 byte per pixel for luma and 0.5 byte per pixel for chroma will be allocated.
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
ST_ErrorCode_t HALBUFF_AllocateFrameBuffer(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p, const HALBUFF_FrameBufferType_t FrameBufferType)
{
    STAVMEM_AllocBlockParams_t  AvmemAllocParams;
    STAVMEM_FreeBlockParams_t   AvmemFreeParams;
    void                        *ForbidBorder[3]; /* Size must be max of number of forbidden borders of all HAL's */
    STAVMEM_BlockHandle_t       FrameBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    void                        *BufferAddress_p;
    U32                         LumaBufferSize,
                                ChromaBufferSize;
    U32                         ChromaOffset;
    U32                         LumaAccumBufferSize,
                                ChromaAccumBufferSize;
    void                        *VirtualAddress;
    U32                         k;
    ST_ErrorCode_t              Err = ST_NO_ERROR;
#ifdef ST_XVP_ENABLE_FGT
    U32                         LumaAccumBufferOffset,
                                ChromaAccumBufferOffset;
#endif /* ST_XVP_ENABLE_FGT */

    switch (AllocParams_p->AllocationMode)
    {
        case VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY :

            /* Get alignment depending on the HAL */
            AvmemAllocParams.Alignment = ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->GetFrameBufferAlignment(BufferHandle);
            /* Compute chroma and luma buffer sizes, according to width, height and buffer type.    */
            ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->ComputeBufferSizes(BufferHandle,
                    AllocParams_p, &LumaBufferSize, &ChromaBufferSize);

            /* Calculate the chroma position.                   */
            ChromaOffset = ((LumaBufferSize + AvmemAllocParams.Alignment - 1) & (~(AvmemAllocParams.Alignment - 1)));
            /* and its size according to memory granularity.    */
            ChromaBufferSize = ((ChromaBufferSize + AvmemAllocParams.Alignment - 1) & (~(AvmemAllocParams.Alignment - 1)));

            /* Allocate frame buffer in shared memory.*/
            if (FrameBufferType == HALBUFF_FRAMEBUFFER_DECIMATED)
            {
                LumaAccumBufferSize = 0;
                ChromaAccumBufferSize = 0;

                AvmemAllocParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForDecimatedFrameBuffers;
            }
            else
            {
                LumaAccumBufferSize = 0;
                ChromaAccumBufferSize = 0;

#ifdef ST_XVP_ENABLE_FGT
                /* if film grain reconstruction enabled */
                if (VIDFGT_IsActivated(((HALBUFF_Data_t *)BufferHandle)->FGTHandle))
                {
                    ComputeAccumulationBufferSizes(BufferHandle, AllocParams_p, &LumaAccumBufferSize, &ChromaAccumBufferSize);
                    LumaAccumBufferOffset = ((LumaBufferSize + ChromaBufferSize + AvmemAllocParams.Alignment - 1) & (~(AvmemAllocParams.Alignment - 1)));
                    ChromaAccumBufferOffset = ((LumaBufferSize + ChromaBufferSize + LumaAccumBufferSize + AvmemAllocParams.Alignment - 1) & (~(AvmemAllocParams.Alignment - 1)));
                }
#endif
                AvmemAllocParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForFrameBuffers;
            }

            /* Save the reference of the partition where this buffer is going to be allocated */
            BufferParams_p->PartitionHandle = AvmemAllocParams.PartitionHandle;

            /* Calculate the total buffer size. */
            AvmemAllocParams.Size = ChromaOffset + ChromaBufferSize + LumaAccumBufferSize + ChromaAccumBufferSize;

            AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM; /* !!! ??? */
            AvmemAllocParams.NumberOfForbiddenRanges = 0;
            AvmemAllocParams.ForbiddenRangeArray_p = NULL;

            /* Get forbidden borders constraints. */
            ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->GetForbiddenBorders(BufferHandle,
                    &(AvmemAllocParams.NumberOfForbiddenBorders),
                    ForbidBorder);
            for (k = 0; k < AvmemAllocParams.NumberOfForbiddenBorders; k++)
            {
                /* Add SDRAM address offset to all borders constraints */
                ForbidBorder[k] = (void *) (((U32) (((HALBUFF_Data_t *)
                            BufferHandle)->VirtualMapping.VirtualBaseAddress_p))
                                     + ((U32) ForbidBorder[k]));
            }
            AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
            Err = STAVMEM_AllocBlock(&AvmemAllocParams, &FrameBufferHandle);
            if (Err != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate frame buffer !"));
                Err = ST_ERROR_NO_MEMORY;
            }
            else
            {
                Err = STAVMEM_GetBlockAddress(FrameBufferHandle, &VirtualAddress);
#if defined(ST_7020) && defined(mb376)
            if ( (((HALBUFF_Data_t *) BufferHandle)->DeviceType == STVID_DEVICE_TYPE_7020_MPEG) )
            {
                BufferAddress_p = STAVMEM_VirtualToDevice(VirtualAddress,
                      &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
            }
            else
            {
                BufferAddress_p = STAVMEM_VirtualToDevice2(VirtualAddress,
                      &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
            }
#else
                BufferAddress_p = STAVMEM_VirtualToDevice(VirtualAddress,
                      &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
#endif /* defined(ST_7020) && defined(mb376) */

                if (Err != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of frame buffer !"));
                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &FrameBufferHandle);
                    Err = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    /* Return params of frame buffer */
                    BufferParams_p->Address_p       = BufferAddress_p;
                    BufferParams_p->TotalSize       = AvmemAllocParams.Size;

                    BufferParams_p->Address2_p      = (void *) ((U32) BufferAddress_p + ChromaOffset);
                    BufferParams_p->Size2           = ChromaBufferSize;
#ifdef ST_XVP_ENABLE_FGT
                    BufferParams_p->FGTLumaAccumBuffer_p     = (void *) ((U32) BufferAddress_p + LumaAccumBufferOffset);
                    BufferParams_p->SizeFGTLumaAccumBuffer   = LumaAccumBufferSize;
                    BufferParams_p->FGTChromaAccumBuffer_p   = (void *) ((U32) BufferAddress_p + ChromaAccumBufferOffset);
                    BufferParams_p->SizeFGTChromaAccumBuffer = ChromaAccumBufferSize;
#endif /* ST_XVP_ENABLE_FGT */

                    BufferParams_p->AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY;
                    BufferParams_p->AvmemBlockHandle = FrameBufferHandle;
                    BufferParams_p->BufferType      = AllocParams_p->BufferType;
#ifdef ST_crc
                    {
                        void *CPUAddress_p;

                        /* Fill frame buffer with grey color in order to obtain reproducible decode in case
                           of first B picture with missing reference (open GOP) */
                        CPUAddress_p = STAVMEM_DeviceToCPU(BufferParams_p->Address_p,
                                    &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
                        memset(CPUAddress_p, 0x80, BufferParams_p->TotalSize-BufferParams_p->Size2);
                        CPUAddress_p = STAVMEM_DeviceToCPU(BufferParams_p->Address2_p,
                                    &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
                        memset(CPUAddress_p, 0x80, BufferParams_p->Size2);
                    }
#endif /* ST_CRC */
                }
            }
            break;

/*        case VIDBUFF_ALLOCATION_MODE_CPU_PARTITION :*/
                /* Problem with CPU partition alloc: handle alignment constraints ! */
        default :
            Err = ST_ERROR_NO_MEMORY;
    }

    if (Err == ST_ERROR_NO_MEMORY)
    {
        /* Set size to 0, to "help" user not using it */
        BufferParams_p->TotalSize = 0;
        BufferParams_p->Size2 = 0;
    }

    return(Err);
} /* End of HALBUFF_AllocateFrameBuffer() function */

/*******************************************************************************
Name        : HALBUFF_ApplyCompressionLevelToPictureSize
Description : Apply a compression to a buffer size.
Parameters  : HAL buffer manager handle,
              Compression level,
              adress of current Picture's width and height.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if action done correctly.
              ST_ERROR_BAD_PARAMETER, if at least one parameter is invalid.
*******************************************************************************/
ST_ErrorCode_t HALBUFF_ApplyCompressionLevelToPictureSize(const HALBUFF_Handle_t BufferHandle , const STVID_CompressionLevel_t CompressionLevel,
        U32 * const PictureWidth_p, U32 * const PictureHeight_p)
{
    /* Check the input parameters */
    if ( (*PictureWidth_p == 0) || (*PictureHeight_p == 0) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    switch (CompressionLevel)
    {
        case STVID_COMPRESSION_LEVEL_NONE:
            /* In this case, nothing to do. */
            return(ST_NO_ERROR);

        case STVID_COMPRESSION_LEVEL_1:
        case STVID_COMPRESSION_LEVEL_2:
            /* Just to check this parameter. */
            break;

        default :
            /* Bad parameter. */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->ApplyCompressionLevelToPictureSize(BufferHandle, CompressionLevel,
            PictureWidth_p, PictureHeight_p);

    return(ST_NO_ERROR);

} /* End of HALBUFF_ApplyCompressionLevelToPictureSize() function */

/*******************************************************************************
Name        : HALBUFF_ApplyDecimationFactorToBufferSize
Description : Apply a decimation factor to a buffer size.
Parameters  : HAL buffer manager handle,
              Decimation factor,
              adress of current Picture's width and height.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if action done correctly.
              ST_ERROR_BAD_PARAMETER, if at least one parameter is invalid.
*******************************************************************************/
ST_ErrorCode_t HALBUFF_ApplyDecimationFactorToPictureSize(const HALBUFF_Handle_t BufferHandle , const STVID_DecimationFactor_t DecimationFactor,
        U32 * const PictureWidth_p, U32 * const PictureHeight_p)
{
#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
    U32 FBarea;
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */

    /* Check the input parameters */
    if ( (*PictureWidth_p == 0) || (*PictureHeight_p == 0) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

	switch (DecimationFactor&~STVID_MPEG_DEBLOCK_RECONSTRUCTION)
    {
        case STVID_DECIMATION_FACTOR_H2:
        case STVID_DECIMATION_FACTOR_V2:
        case STVID_DECIMATION_FACTOR_H4:
        case STVID_DECIMATION_FACTOR_V4:
        case STVID_DECIMATION_FACTOR_H2 | STVID_DECIMATION_FACTOR_V4:
        case STVID_DECIMATION_FACTOR_H4 | STVID_DECIMATION_FACTOR_V2:
        case STVID_DECIMATION_FACTOR_2:
        case STVID_DECIMATION_FACTOR_4:
        case STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2 :
        case STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2 | STVID_DECIMATION_FACTOR_V2:
        case STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2 | STVID_DECIMATION_FACTOR_V4:
		case STVID_POSTPROCESS_RECONSTRUCTION:
            /* Just to check this parameter. */
            break;

        case STVID_DECIMATION_FACTOR_NONE:
            /* In this case, nothing to do. */
            return(ST_NO_ERROR);
            break;

        default :
            /* Bad parameter. */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->ApplyDecimationFactorToPictureSize(BufferHandle, DecimationFactor,
            PictureWidth_p, PictureHeight_p);

#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
        /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
         * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum) */
    do
    {
        FBarea = (*PictureWidth_p) * (*PictureHeight_p);
        if (FBarea < (720 * 576))
        {
            if (*PictureWidth_p < 720)
            {
                *PictureWidth_p = 720;
            }
            if (*PictureHeight_p < 576)
            {
                *PictureHeight_p = 576;
            }
        }
    } while (FBarea < (720 * 576));
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */

    return(ST_NO_ERROR);

} /* End of HALBUFF_ApplyDecimationFactorToBufferSize() function */

/*******************************************************************************
Name        : HALBUFF_DeAllocateBitBuffer
Description : De-allocate memory of a bit buffer
Parameters  : HAL buffer manager handle,
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HALBUFF_DeAllocateBitBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    STAVMEM_FreeBlockParams_t AvmemFreeParams;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    UNUSED_PARAMETER(BufferHandle);

    if (BufferParams_p->AllocationMode != VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY)
    {
        /* CPU partition alloc not supported yet */
        return(Err);
    }

    AvmemFreeParams.PartitionHandle = BufferParams_p->PartitionHandle;
    Err = STAVMEM_FreeBlock(&AvmemFreeParams, &(BufferParams_p->AvmemBlockHandle));

    return(ST_NO_ERROR);
} /* End of HALBUFF_DeAllocateBitBuffer() function */


/*******************************************************************************
Name        : HALBUFF_DeAllocateFrameBuffer
Description : De-allocate memory of a frame buffer
Parameters  : HAL buffer manager handle,
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HALBUFF_DeAllocateFrameBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p, const HALBUFF_FrameBufferType_t FrameBufferType)
{
    STAVMEM_FreeBlockParams_t AvmemFreeParams;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    UNUSED_PARAMETER(BufferHandle);
    UNUSED_PARAMETER(FrameBufferType);

    if (BufferParams_p->AllocationMode != VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY)
    {
        /* CPU partition alloc not supported yet */
        return(Err);
    }

    AvmemFreeParams.PartitionHandle = BufferParams_p->PartitionHandle;
    Err = STAVMEM_FreeBlock(&AvmemFreeParams, &(BufferParams_p->AvmemBlockHandle));

    return(ST_NO_ERROR);
} /* End of HALBUFF_DeAllocateFrameBuffer() function */


/*******************************************************************************
Name        : HALBUFF_GetFrameBufferParams
Description : Get total size and alignment required for the picture frame buffer
Parameters  : HAL buffer manager handle,
              PictureWidth is the width of the picture in pixels,
              PictureHeight is the height of the picture in pixels,
              pointers to return data
Assumptions : TotalSize_p and Aligment_p are not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
void HALBUFF_GetFrameBufferParams(const HALBUFF_Handle_t BufferHandle, const U32 PictureWidth, const U32 PictureHeight, U32 * const TotalSize_p, U32 * const Aligment_p)
{
    HALBUFF_AllocateBufferParams_t  AllocParams;
    U32 LumaBufferSize, ChromaBufferSize;

    /* Get alignment depending on the HAL */
    *Aligment_p = ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->GetFrameBufferAlignment(BufferHandle);

    /* Compute chroma and luma buffer sizes, according to width, height and buffer type.    */
    AllocParams.BufferType      = ((HALBUFF_Data_t *) BufferHandle)->FrameBuffersType;
    AllocParams.PictureWidth    = PictureWidth;
    AllocParams.PictureHeight   = PictureHeight;
    AllocParams.AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY; /* Useless here but in case a new mode appears ... */
    ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->ComputeBufferSizes(BufferHandle, &AllocParams,
                   &LumaBufferSize, &ChromaBufferSize);

    /* Determine Luma size including alignment constraints */
    *TotalSize_p =  (LumaBufferSize + *Aligment_p - 1) & (~(*Aligment_p - 1));       /* Luma + padding */
    /* Add Chroma size with alignment constraints */
    *TotalSize_p += ((ChromaBufferSize + *Aligment_p - 1) & (~(*Aligment_p - 1)));    /* Chroma + padding */

} /* End of HALBUFF_GetPictureAllocParams() function */


/*******************************************************************************
Name        : HALBUFF_Init
Description : Init buffer manager HAL
Parameters  : Init params, handle to return
Assumptions : InitParams_p and BufferHandle_p are not NULL
Limitations :
Returns     : in params: HAL decode handle
              ST_NO_ERROR if success
              ST_ERROR_NO_MEMORY if can't allocate
*******************************************************************************/
ST_ErrorCode_t HALBUFF_Init(const HALBUFF_InitParams_t * const InitParams_p, HALBUFF_Handle_t * const BufferHandle_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALBUFF_Data_t * HALBUFF_Data_p;

    /* Allocate a HALBUFF structure */
    SAFE_MEMORY_ALLOCATE(HALBUFF_Data_p, HALBUFF_Data_t *, InitParams_p->CPUPartition_p, sizeof(HALBUFF_Data_t));
    if (HALBUFF_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *BufferHandle_p = (HALBUFF_Handle_t *) HALBUFF_Data_p;
    HALBUFF_Data_p->ValidityCheck = HALBUFF_VALID_HANDLE;

    /* Store parameters */
    HALBUFF_Data_p->CPUPartition_p          = InitParams_p->CPUPartition_p;
    /* HALBUFF_Data_p->SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
     This values is form now on retrieved from AV mem */
    HALBUFF_Data_p->DeviceType              = InitParams_p->DeviceType;
    HALBUFF_Data_p->AvmemPartitionHandle    = InitParams_p->AvmemPartitionHandle;
    HALBUFF_Data_p->FrameBuffersType        = InitParams_p->FrameBuffersType;
    HALBUFF_Data_p->FrameBufferAdditionalDataBytesPerMB = 0;
    HALBUFF_Data_p->AvmemPartitionHandleForFrameBuffers             = InitParams_p->AvmemPartitionHandle;
    HALBUFF_Data_p->AvmemPartitionHandleForDecimatedFrameBuffers    = InitParams_p->AvmemPartitionHandle;
    HALBUFF_Data_p->AvmemPartitionHandleForAdditionnalData          = InitParams_p->AvmemPartitionHandle;
    HALBUFF_Data_p->AvmemPartitionHandleForBitBuffer                = InitParams_p->AvmemPartitionHandle;
#ifdef ST_XVP_ENABLE_FGT
    HALBUFF_Data_p->FGTHandle = InitParams_p->FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
    /* Get AV Mem mapping */
    STAVMEM_GetSharedMemoryVirtualMapping(&HALBUFF_Data_p->VirtualMapping );

    switch (HALBUFF_Data_p->DeviceType)
    {
#ifdef ST_omega1
        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            HALBUFF_Data_p->FunctionsTable_p = &HALBUFF_Omega1Functions;
            break;
#endif /* ST_omega1 */

#ifdef ST_omega2
        case STVID_DEVICE_TYPE_7015_MPEG :
        case STVID_DEVICE_TYPE_7020_MPEG :
#ifdef ST_diginput
        case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
#endif /* ST_diginput */
            HALBUFF_Data_p->FunctionsTable_p = &HALBUFF_Omega2Functions;
            break;
#endif /* ST_omega2 */

#ifdef ST_genbuff
        case STVID_DEVICE_TYPE_5528_MPEG :
        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5525_MPEG :
        case STVID_DEVICE_TYPE_5105_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_ZEUS_MPEG :
        case STVID_DEVICE_TYPE_ZEUS_H264 :
        case STVID_DEVICE_TYPE_ZEUS_VC1  :
        case STVID_DEVICE_TYPE_7109_MPEG :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_AVS :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
        case STVID_DEVICE_TYPE_7200_H264 :
        case STVID_DEVICE_TYPE_7200_VC1 :
        case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_7710_MPEG :
        case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
        case STVID_DEVICE_TYPE_STD2000_MPEG :
        case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT :
            HALBUFF_Data_p->FunctionsTable_p = &HALBUFF_GenbuffFunctions;
            break;
#endif

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* switch (HALBUFF_Data_p->DeviceType) */

    /* Initialise buffers manager */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Initialise buffer manager */
        ErrorCode = (HALBUFF_Data_p->FunctionsTable_p->Init)(*BufferHandle_p);
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: allocation failed, undo initialisations done */
        HALBUFF_Term(*BufferHandle_p);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : HALBUFF_Term
Description : Terminate buffer manager HAL
Parameters  : Buffer manager handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HALBUFF_Term(const HALBUFF_Handle_t BufferHandle)
{
    HALBUFF_Data_t * HALBUFF_Data_p;

    /* Find structure */
    HALBUFF_Data_p = (HALBUFF_Data_t *) BufferHandle;

    if (HALBUFF_Data_p->ValidityCheck != HALBUFF_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* De-validate structure */
    HALBUFF_Data_p->ValidityCheck = 0; /* not HALBUFF_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(HALBUFF_Data_p, HALBUFF_Data_p->CPUPartition_p, sizeof(HALBUFF_Data_t));

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : HALBUFF_SetAvmemPartitionForFrameBuffers
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForFrameBuffers(const HALBUFF_Handle_t BufferHandle,
                                                        const STAVMEM_PartitionHandle_t AvmemPartitionHandle,
                                                        BOOL *      AvmemPartitionHandleHasChanged_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALBUFF_Data_t * HALBUFF_Data_p = (HALBUFF_Data_t *) BufferHandle;

    if (BufferHandle == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (HALBUFF_Data_p->AvmemPartitionHandleForFrameBuffers == AvmemPartitionHandle)
    {
        *AvmemPartitionHandleHasChanged_p = FALSE;
    }
    else
    {
        *AvmemPartitionHandleHasChanged_p = TRUE;
    }

    HALBUFF_Data_p->AvmemPartitionHandleForFrameBuffers = AvmemPartitionHandle;

    return(ErrorCode);
} /* End of HALBUFF_SetAvmemPartitionForFrameBuffers() function */

/*******************************************************************************
Name        : HALBUFF_SetAvmemPartitionForDecimatedFrameBuffers
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForDecimatedFrameBuffers(const HALBUFF_Handle_t BufferHandle,
                                                                 const STAVMEM_PartitionHandle_t AvmemPartitionHandle,
                                                                 BOOL *      AvmemPartitionHandleHasChanged_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALBUFF_Data_t * HALBUFF_Data_p = (HALBUFF_Data_t *) BufferHandle;

    if (BufferHandle == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (HALBUFF_Data_p->AvmemPartitionHandleForDecimatedFrameBuffers == AvmemPartitionHandle)
    {
        *AvmemPartitionHandleHasChanged_p = FALSE;
    }
    else
    {
        *AvmemPartitionHandleHasChanged_p = TRUE;
    }

    HALBUFF_Data_p->AvmemPartitionHandleForDecimatedFrameBuffers = AvmemPartitionHandle;

    return(ErrorCode);
} /* End of HALBUFF_SetAvmemPartitionForDecimatedFrameBuffers() function */

/*******************************************************************************
Name        : HALBUFF_SetAvmemPartitionForBitBuffer
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForBitBuffer(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForBitBuffer = AvmemPartitionHandle;

    return(ErrorCode);
} /* End of HALBUFF_SetAvmemPartitionForBitBuffer() function */

/*******************************************************************************
Name        : HALBUFF_SetAvmemPartitionForAdditionnalData
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForAdditionnalData(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALBUFF_Data_t * HALBUFF_Data_p = (HALBUFF_Data_t *) BufferHandle;
    if (BufferHandle == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    HALBUFF_Data_p->AvmemPartitionHandleForAdditionnalData = AvmemPartitionHandle;

    return(ErrorCode);
} /* End of HALBUFF_SetAvmemPartitionForAdditionnalData() function */


/*******************************************************************************
Name        : HALBUFF_GetAdditionnalDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t HALBUFF_GetAdditionnalDataBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p)
{
    STAVMEM_AllocBlockParams_t  AvmemAllocParams;
    STAVMEM_FreeBlockParams_t   AvmemFreeParams;
    STAVMEM_BlockHandle_t       DataBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    void                        * VirtualAddress;
    U32                         AdditionalDataBufferSize;
    U32                         PictureWidth, PictureHeight;
    U32                         Alignment;
    ST_ErrorCode_t              ErrorCode;

    PictureBuffer_p->PPB.Address_p = NULL;
    PictureBuffer_p->PPB.AvmemPartitionHandleForAdditionnalData = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForAdditionnalData;
    PictureBuffer_p->PPB.AvmemBlockHandle = DataBufferHandle;
    PictureBuffer_p->PPB.Size = 0;
    PictureBuffer_p->PPB.FieldCounter = 0;

    PictureWidth = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
    PictureHeight = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Height;
    Alignment = 16;

    AdditionalDataBufferSize = ((PictureWidth+15)/16)*((PictureHeight+15)/16)*((HALBUFF_Data_t *) BufferHandle)->FrameBufferAdditionalDataBytesPerMB;
    AdditionalDataBufferSize = ((AdditionalDataBufferSize + Alignment - 1) & (~(Alignment - 1)));

    AvmemAllocParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForAdditionnalData;
    AvmemAllocParams.Size = AdditionalDataBufferSize;
    /*AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;*/
    /* Allocate BOTTOM TOP instead of TOP BOTTOM to avoid issue on 7100 with DMC when PPB is too close from the end of the physical memory (less than 120 KBytes) */
    AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AvmemAllocParams.Alignment = Alignment;

    AvmemAllocParams.NumberOfForbiddenRanges = 0;
    AvmemAllocParams.ForbiddenRangeArray_p = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders = 0;
    AvmemAllocParams.ForbiddenBorderArray_p = NULL;
    ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &DataBufferHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
#ifdef TRACE_UART
    TraceBuffer(("NoMem for %d %d-%d/%d\r\n",
            PictureBuffer_p->Decode.CommandId,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
        ));
#endif
        /* Error handling */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate AdditionnalDataBuffer !"));
        ErrorCode = ST_ERROR_NO_MEMORY;
    }
    else
    {
        ErrorCode = STAVMEM_GetBlockAddress(DataBufferHandle, &VirtualAddress);
        if (ErrorCode != ST_NO_ERROR)
        {
#ifdef TRACE_UART
            TraceBuffer(("ErrGetAddr for %d %d-%d/%d\r\n",
                PictureBuffer_p->Decode.CommandId,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
            ));
#endif
            /* Error handling */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of AdditionnalDataBuffer !"));
            /* Try to free block before leaving */
            AvmemFreeParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForAdditionnalData;
            STAVMEM_FreeBlock(&AvmemFreeParams, &DataBufferHandle);
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            PictureBuffer_p->PPB.Address_p = STAVMEM_VirtualToDevice(VirtualAddress, &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
            PictureBuffer_p->PPB.AvmemPartitionHandleForAdditionnalData = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForAdditionnalData;
            PictureBuffer_p->PPB.AvmemBlockHandle = DataBufferHandle;
            PictureBuffer_p->PPB.Size = AdditionalDataBufferSize;
#ifdef TRACE_UART
            TraceBuffer(("PPB allocated 0x%x size %d for %d %d-%d/%d\r\n",
                PictureBuffer_p->PPB.Address_p,
                PictureBuffer_p->PPB.Size,
                PictureBuffer_p->Decode.CommandId,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
        }
    }
    return(ErrorCode);
} /* End of HALBUFF_GetAdditionnalDataBuffer() function */

/*******************************************************************************
Name        : HALBUFF_FreeAdditionnalDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t HALBUFF_FreeAdditionnalDataBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t   AvmemFreeParams;
    STAVMEM_BlockHandle_t       DataBufferHandle;

    UNUSED_PARAMETER(BufferHandle);

    AvmemFreeParams.PartitionHandle = PictureBuffer_p->PPB.AvmemPartitionHandleForAdditionnalData;
    DataBufferHandle = PictureBuffer_p->PPB.AvmemBlockHandle;
    ErrorCode = STAVMEM_FreeBlock(&AvmemFreeParams, &DataBufferHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
#ifdef TRACE_UART
        TraceBuffer(("Can't free PPB of 0x%x (%d-%d/%d)\r\n",
            PictureBuffer_p->PPB.Address_p,
            PictureBuffer_p->Decode.CommandId,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
        ));
#endif
        /* Error handling */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error freeing AdditionnalDataBuffer !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
#ifdef TRACE_UART
            TraceBuffer(("PPB freed 0x%x (%d-%d/%d)\r\n",
                PictureBuffer_p->PPB.Address_p,
                PictureBuffer_p->Decode.CommandId,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
            ));
#endif
        PictureBuffer_p->PPB.Size = 0;
    }

    return(ErrorCode);
} /* End of HALBUFF_FreeAdditionnalDataBuffer() function */


/*******************************************************************************
Name        : HALBUFF_ClearFrameBuffer
Description : Fill a Frame Buffer with black
Parameters  : HAL buffer manager handle
Assumptions :
Limitations :
Returns     : STVID_ERROR_MEMORY_ACCESS, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HALBUFF_ClearFrameBuffer (const HALBUFF_Handle_t BufferHandle,
                                        VIDBUFF_BufferAllocationParams_t * const BufferParams_p,
                                        const STVID_ClearParams_t * const ClearFrameBufferParams_p)
{
    U32*            VirtualAddress_p;
    U32*            VirtualAddress2_p;
    void*           PatternAddress1_p;
    void*           PatternAddress2_p;
    U32             PatternSize1;
    U32             PatternSize2;
    ST_ErrorCode_t  ErrorCode;
    U32             LumaBlackPattern;
    U32             ChromaBlackPattern;
    HALBUFF_Data_t* HALBUFF_Data_p = (HALBUFF_Data_t*) BufferHandle;

    LumaBlackPattern   = LUMA_WORD_FOR_BLACK_COLOR;
    ChromaBlackPattern = CHROMA_WORD_FOR_BLACK_COLOR;

    /* Set patterns */
    switch(ClearFrameBufferParams_p->ClearMode)
    {
        case STVID_CLEAR_DISPLAY_PATTERN_FILL:
            PatternAddress1_p = ClearFrameBufferParams_p->PatternAddress1_p;
            PatternSize1      = ClearFrameBufferParams_p->PatternSize1;
            PatternAddress2_p = ClearFrameBufferParams_p->PatternAddress2_p;
            PatternSize2      = ClearFrameBufferParams_p->PatternSize2;
            break;
        case STVID_CLEAR_DISPLAY_BLACK_FILL:
        default:
            PatternAddress1_p = &LumaBlackPattern;
            PatternSize1      = 4;
            PatternAddress2_p = &ChromaBlackPattern;
            PatternSize2      = 4;
            break;
    }
    /* Convert luma and chroma device addresses to virtual addresses */
    VirtualAddress_p     = DeviceToVirtual((BufferParams_p->Address_p),  HALBUFF_Data_p->VirtualMapping);
    VirtualAddress2_p    = DeviceToVirtual((BufferParams_p->Address2_p), HALBUFF_Data_p->VirtualMapping);

    /* Fill the buffer with patterns */
    ErrorCode =  STAVMEM_FillBlock1D( (char*) PatternAddress1_p, PatternSize1, VirtualAddress_p, (BufferParams_p->TotalSize - BufferParams_p->Size2));
    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STAVMEM_FillBlock1D( (char*) PatternAddress2_p, PatternSize2, VirtualAddress2_p, BufferParams_p->Size2);
    }

    /* Translate error to an STVID private error before leaving */
    if(ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = STVID_ERROR_MEMORY_ACCESS;
    }

    return(ErrorCode);
} /* End of HALBUFF_ClearFrameBuffer() function */

#ifdef ST_XVP_ENABLE_FGT
/*******************************************************************************
Name        : ComputeAccumulationBufferSizes
Description : Compute luma and chroma Accum buffer sizes, according to the width,
              height and buffer type parameters.
Parameters  : HAL buffer manager handle.
              Allocation parameters.
              LumaAccumBufferSize_p, adress of the luma accumulation buffer to compute.
              ChromaAccumBufferSize_p, adress of the chroma accumulation buffer to compute.
Assumptions :
Limitations :
Returns     : None.
*******************************************************************************/
static void ComputeAccumulationBufferSizes (const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p,
        U32 * const LumaAccumBufferSize_p, U32 * const ChromaAccumBufferSize_p)
{
#define MACROBLOCK_GROUP_HEIGHT       	(2)
#define MACROBLOCK_GROUP_WIDTH        	(2)
#define MACROBLOCK_GROUP_Y_SURFACE	(MACROBLOCK_GROUP_HEIGHT * MACROBLOCK_GROUP_WIDTH)
#define MACROBLOCK_GROUP_C_SURFACE	(MACROBLOCK_GROUP_HEIGHT * MACROBLOCK_GROUP_WIDTH * 2)

    U32  WidthNbMB, HeightNbMB;

    UNUSED_PARAMETER(BufferHandle);

    /* Number of Macro Blocks for width */
    WidthNbMB= ((AllocParams_p->PictureWidth/4) + 15) / 16;

    /* Number of Macro Blocks for height */
    HeightNbMB= ((AllocParams_p->PictureHeight/2) + 15) / 16;
    /* Round it to upper number of needed slices (dummy slice) */
    HeightNbMB= ((HeightNbMB +  MACROBLOCK_GROUP_HEIGHT - 1) / MACROBLOCK_GROUP_HEIGHT) * MACROBLOCK_GROUP_HEIGHT;

    /* Set the luma buffer size.        */
    /* Round it to upper number of needed MB (dummy column) according to Luma MB group surface */
    *LumaAccumBufferSize_p = ((HeightNbMB*WidthNbMB + MACROBLOCK_GROUP_Y_SURFACE - 1) / MACROBLOCK_GROUP_Y_SURFACE) * MACROBLOCK_GROUP_Y_SURFACE;
    *LumaAccumBufferSize_p *= 256;	/* 256 bytes per Macro Block */

    /* Set the chroma buffer size.        */
    /* Round it to upper number of needed MB (dummy column) according to Chroma MB group surface */
    /* NB: For Chroma, SURFACE is twice the Luma surface */
    *ChromaAccumBufferSize_p = ((HeightNbMB*WidthNbMB + MACROBLOCK_GROUP_C_SURFACE - 1) / MACROBLOCK_GROUP_C_SURFACE) * MACROBLOCK_GROUP_C_SURFACE;
    switch (AllocParams_p->BufferType)
    {
        case VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL :
            /* 1 byte luma + 1 byte chroma */
            *ChromaAccumBufferSize_p *= 256;	 /* 256 bytes per Macro Block */
            break;

        default :
            /* 1 byte luma + 0.5 byte chroma */
            *ChromaAccumBufferSize_p *= 128;	 /* 128 bytes per Macro Block */
            break;
    }

} /* End of ComputeAccumulationBufferSizes()function */
#endif /* ST_XVP_ENABLE_FGT */


#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        : HALBUFF_AllocateESCopyBuffer
Description : Allocate memory for ES Copy buffer
Parameters  : HAL buffer manager handle,
Assumptions : Pointers are not NULL
              AVMEM_Partition is in non-secure memory area.
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
ST_ErrorCode_t HALBUFF_AllocateESCopyBuffer(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    STAVMEM_FreeBlockParams_t AvmemFreeParams;
    STAVMEM_BlockHandle_t ESCopyBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    void * BufferAddr;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    void * VirtualAddress;

    switch (AllocParams_p->AllocationMode)
    {
        case VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY :
            /* Get alignment depending on the HAL. Same alignment as the bit buffer. */
            AvmemAllocParams.Alignment = ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->GetBitBufferAlignment(BufferHandle);

            /* Allocate ES Copy buffer in shared memory */
            /* DG to do: Verify that partition is in non-secure memory with STMES function? */
            /* Or already done in VIDBUFF_AllocateESCopyBuffer()? */
            AvmemAllocParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandleForESCopyBuffer;

            /* Save the reference of the partition where this buffer is going to be allocated */
            BufferParams_p->PartitionHandle = AvmemAllocParams.PartitionHandle;

            AvmemAllocParams.Size = AllocParams_p->BufferSize;
            AvmemAllocParams.Size = ((AvmemAllocParams.Size + AvmemAllocParams.Alignment - 1) & (~(AvmemAllocParams.Alignment - 1)));
            AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AvmemAllocParams.NumberOfForbiddenRanges = 0;
            AvmemAllocParams.ForbiddenRangeArray_p = NULL;
            AvmemAllocParams.NumberOfForbiddenBorders = 0;
            AvmemAllocParams.ForbiddenBorderArray_p = NULL;

            Err = STAVMEM_AllocBlock(&AvmemAllocParams, &ESCopyBufferHandle);

            if (Err != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate ES Copy buffer !"));
                Err = ST_ERROR_NO_MEMORY;
            }
            else
            {
                Err = STAVMEM_GetBlockAddress(ESCopyBufferHandle, &VirtualAddress);
                BufferAddr = STAVMEM_VirtualToDevice(VirtualAddress,
                      &(((HALBUFF_Data_t *)(BufferHandle))->VirtualMapping));
                if (Err != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of ES Copy buffer !"));
                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = ((HALBUFF_Data_t *) BufferHandle)->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &ESCopyBufferHandle);
                    Err = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    /* Return params of ES Copy buffer */
                    BufferParams_p->TotalSize        = AvmemAllocParams.Size;
                    BufferParams_p->Address_p        = BufferAddr;
                    BufferParams_p->AllocationMode   = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY;
                    BufferParams_p->AvmemBlockHandle = ESCopyBufferHandle;
                    BufferParams_p->BufferType       = VIDBUFF_BUFFER_SIMPLE;
                }
            }
            break;

/*      case VIDBUFF_ALLOCATION_MODE_CPU_PARTITION :*/
                /* Problem with CPU partition alloc: handle alignment constraints ! */

        default :
            Err = ST_ERROR_NO_MEMORY;
    }

    if (Err == ST_ERROR_NO_MEMORY)
    {
        /* Set size to 0, to "help" user not using it */
        BufferParams_p->TotalSize = 0;
    }

    return(Err);
} /* End of HALBUFF_AllocateESCopyBuffer() function */


/*******************************************************************************
Name        : HALBUFF_DeAllocateESCopyBuffer
Description : De-allocate memory of an ES Copy buffer
Parameters  : HAL buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t HALBUFF_DeAllocateESCopyBuffer(const HALBUFF_Handle_t BufferHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    STAVMEM_FreeBlockParams_t AvmemFreeParams;
    UNUSED_PARAMETER(BufferHandle);

    if (BufferParams_p->AllocationMode != VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY)
    {
        /* CPU partition alloc not supported yet */
        return(ST_NO_ERROR);
    }

    /* De-allocate ES Copy buffer in memory */
    AvmemFreeParams.PartitionHandle = BufferParams_p->PartitionHandle;
    STAVMEM_FreeBlock(&AvmemFreeParams, &(BufferParams_p->AvmemBlockHandle));

    return(ST_NO_ERROR);
} /* End of HALBUFF_DeAllocateESCopyBuffer() function */

/*******************************************************************************
Name        : HALBUFF_SetAvmemPartitionForESCopyBuffer
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t HALBUFF_SetAvmemPartitionForESCopyBuffer(const HALBUFF_Handle_t BufferHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALBUFF_Data_t * HALBUFF_Data_p = (HALBUFF_Data_t *) BufferHandle;
    if (BufferHandle == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    HALBUFF_Data_p->AvmemPartitionHandleForESCopyBuffer = AvmemPartitionHandle;

    return(ErrorCode);
} /* End of HALBUFF_SetAvmemPartitionForESCopyBuffer() function */

#endif /* DVD_SECURED_CHIP */


/* End of halv_buf.c */
