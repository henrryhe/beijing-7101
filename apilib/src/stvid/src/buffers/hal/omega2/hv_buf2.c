/*******************************************************************************

File name   : hv_buf2.c

Description : HAL video buffer manager omega 2 family source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
18 Jul 2000        Created                                           HG
25 Jan 2001        Changed CPU memory access to device access        HG
03 Mar 2001        ApplyDecimationFactorToBufferSize and
                   ApplyCompressionLevelToBufferSize coding.         GG
25 Jul 2001        ApplyDecimationFactorToBufferSize becommes
                   ApplyDecimationFactorToPictureSize and
                   ApplyCompressionLevelToBufferSize becommes
                   ApplyCompressionLevelToPictureSize               GG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "buffers.h"
#include "halv_buf.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Private Macros ----------------------------------------------------------- */

#define HALBUFF_Read8(Address)     STSYS_ReadRegDev8((void *) (Address))
#define HALBUFF_Read16(Address)    STSYS_ReadRegDev16BE((void *) (Address))
#define HALBUFF_Read32(Address)    STSYS_ReadRegDev32BE((void *) (Address))

#define HALBUFF_Write8(Address, Value)  STSYS_WriteRegDev8((void *) (Address), (Value))
#define HALBUFF_Write16(Address, Value) STSYS_WriteRegDev16BE((void *) (Address), (Value))
#define HALBUFF_Write32(Address, Value) STSYS_WriteRegDev32BE((void *) (Address), (Value))


/* Private Function prototypes ---------------------------------------------- */

static void ApplyCompressionLevelToPictureSize (const HALBUFF_Handle_t BufferHandle, const STVID_CompressionLevel_t CompressionLevel,
        U32 * const PictureWidth_p, U32 * const PictureHeight_p);
static void ApplyDecimationFactorToPictureSize (const HALBUFF_Handle_t BufferHandle, const STVID_DecimationFactor_t DecimationFactor,
        U32 * const PictureWidth_p, U32 * const PictureHeight_p);
static void ComputeBufferSizes (const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p,
        U32 * const LumaBufferSize_p, U32 * const ChromaBufferSize_p);
static void GetForbiddenBorders(const HALBUFF_Handle_t BufferHandle, U32 * const NumberOfForbiddenBorders_p, void ** const ForbiddenBorderArray_p);
static U32 GetBitBufferAlignment(const HALBUFF_Handle_t BufferHandle);
static U32 GetFrameBufferAlignment(const HALBUFF_Handle_t BufferHandle);
static ST_ErrorCode_t Init(const HALBUFF_Handle_t BufferHandle);
static void Term(const HALBUFF_Handle_t BufferHandle);


/* Global Variables --------------------------------------------------------- */

const HALBUFF_FunctionsTable_t HALBUFF_Omega2Functions = {
    Init,
    Term,
    GetBitBufferAlignment,
    GetFrameBufferAlignment,
    ApplyCompressionLevelToPictureSize,
    ApplyDecimationFactorToPictureSize,
    ComputeBufferSizes,
    GetForbiddenBorders
};


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : GetBitBufferAlignment
Description : Get alignment constraint for bit buffer
Parameters  : HAL buffer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 GetBitBufferAlignment(const HALBUFF_Handle_t BufferHandle)
{
    return(2048);
} /* End of GetBitBufferAlignment() function */


/*******************************************************************************
Name        : GetForbiddenBorders
Description : Get forbidden borders according to the chip/SDRAM access constraints
Parameters  : HAL buffer manager handle
Assumptions : SDRAM at address 0. Then values must be shifted of SDRAMAddress by upper level.
Limitations :
Returns     : Nothing
*******************************************************************************/
static void GetForbiddenBorders(const HALBUFF_Handle_t BufferHandle, U32 * const NumberOfForbiddenBorders_p, void ** const ForbiddenBorderArray_p)
{
    *NumberOfForbiddenBorders_p = 0;

} /* End of GetForbiddenBorders() function */

/*******************************************************************************
Name        : GetFrameBufferAlignment
Description : Get alignment constraint for frame buffer
Parameters  : HAL buffer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 GetFrameBufferAlignment(const HALBUFF_Handle_t BufferHandle)
{
    return(2048);
} /* End of GetFrameBufferAlignment() function */

/*******************************************************************************
Name        : ApplyCompressionLevelToPictureSize
Description : Apply a factor to the buffer's size according to the wanted
              compression level.
Parameters  : HAL buffer manager handle.
              CompressionLevel of the device.
              PictureWidth_p, adress of the picture's width to convert.
              PictureHeight_p, adress of the picture's height to convert.
Assumptions : The CompressionLevel is valid, otherwise nothing is done.
              *PictureWidth_p and PictureHeight_p are not null.
Limitations :
Returns     : None.
*******************************************************************************/
static void ApplyCompressionLevelToPictureSize (const HALBUFF_Handle_t BufferHandle,
        const STVID_CompressionLevel_t CompressionLevel, U32 * const PictureWidth_p, U32 * const PictureHeight_p)
{
    switch (CompressionLevel)
    {
        case STVID_COMPRESSION_LEVEL_1:
            *PictureWidth_p  = (*PictureWidth_p  * 2) / 3;
            *PictureHeight_p = (*PictureHeight_p * 2) / 3;
            break;
        case STVID_COMPRESSION_LEVEL_2:
            *PictureWidth_p  /= 4;
            *PictureHeight_p /= 4;
            break;
        default :
            /* In that case, nothing is done. */
            break;
    }
} /* End of ApplyCompressionLevelToPictureSize() function */

/*******************************************************************************
Name        : ApplyDecimationFactorToPictureSize
Description : Apply a factor to the picture's size according to the decimation
              factor.
Parameters  : HAL buffer manager handle.
              DecimationFactor of the device.
              PictureWidth_p, adress of the picture's width to convert.
              PictureHeight_p, adress of the picture's height to convert.
Assumptions : The Decimation is valid, otherwise nothing is done.
              *PictureWidth_p and PictureHeight_p are not null.
Limitations :
Returns     : None.
*******************************************************************************/
static void ApplyDecimationFactorToPictureSize (const HALBUFF_Handle_t BufferHandle,
        const STVID_DecimationFactor_t DecimationFactor, U32 * const PictureWidth_p, U32 * const PictureHeight_p)
{
    if (DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
    {
        if (DecimationFactor & STVID_DECIMATION_FACTOR_H2)
        {
            *PictureWidth_p /= 2;
        }
        if (DecimationFactor & STVID_DECIMATION_FACTOR_V2)
        {
            *PictureHeight_p /= 2;
        }
        if (DecimationFactor & STVID_DECIMATION_FACTOR_H4)
        {
            *PictureWidth_p /= 4;
        }
        if (DecimationFactor & STVID_DECIMATION_FACTOR_V4)
        {
            *PictureHeight_p /= 4;
        }
    }
} /* End of ApplyDecimationFactorToPictureSize() function */

/*******************************************************************************
Name        : ComputeBufferSizes
Description : Compute luma and chroma buffer sizes, according to the width,
              height and buffer type parameters.
Parameters  : HAL buffer manager handle.
              Allocation parameters.
              LumaBufferSize_p, adress of the lume buffer to compute.
              ChromaBufferSize_p, adress of the chroma buffer to compute.
Assumptions :
Limitations :
Returns     : None.
*******************************************************************************/
static void ComputeBufferSizes (const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p,
        U32 * const LumaBufferSize_p, U32 * const ChromaBufferSize_p)
{
    U32 UpRoundedWidth, UpRoundedHeight;

    /* CD: this algorithm could be optimized (see .../hal/genbuff/genbuff.c) */

    /* Compute luma size needed                         */
    /* Due to 7015/7020 Memory organization             */
    /* Width must be an integer number of macroblocs    */
    UpRoundedWidth  = ((AllocParams_p->PictureWidth + 15) / 16) * 16;
    /* Height must be an even number of macrobloc       */
    UpRoundedHeight = ((AllocParams_p->PictureHeight + 31) / 32) * 32;

    *LumaBufferSize_p = UpRoundedWidth * UpRoundedHeight;

    /* Compute chroma size needed                           */
    /* Due to 7015/7020 Memory organization                  */
    /* Height must be even number of macroblocs (as Luma)   */
    switch (AllocParams_p->BufferType)
    {
    case VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL :
        /* 1 byte luma + 1 byte chroma */
        *ChromaBufferSize_p = *LumaBufferSize_p;
        break;
    default :
        /* 1 byte luma + 0.5 byte chroma */

        /* in 4:2:0 chroma size is half of the luma size */
        /* and if width is odd number of macroblocks and */
        /* height is not %4 of macroblocks, width must be increase by 1 macroblock */
        if ((UpRoundedHeight % (4*16) != 0) && (UpRoundedWidth % (2*16) != 0))
        {
            UpRoundedWidth += 16;
        }

        *ChromaBufferSize_p = (UpRoundedWidth * UpRoundedHeight) / 2;
        break;
    }

} /* End of ComputeBufferSizes()function */

/*******************************************************************************
Name        : Init
Description : Initialise chip
Parameters  : HAL buffer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALBUFF_Handle_t BufferHandle)
{
    return(ST_NO_ERROR);
} /* End of Init() function */


/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  : HAL buffer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void Term(const HALBUFF_Handle_t BufferHandle)
{
} /* End of Term() function */



/* End of hv_buf2.c */
