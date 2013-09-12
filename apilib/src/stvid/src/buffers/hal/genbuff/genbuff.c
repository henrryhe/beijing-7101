/*******************************************************************************

File name   : genbuff.c

Description : generic HAL video buffer manager

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
18 Jun 2003        Created                                           PLe

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif


#include "stddefs.h"

#include "buffers.h"
#include "halv_buf.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Private Macros ----------------------------------------------------------- */
#define  Min(Val1, Val2) (((Val1) < (Val2))?(Val1):(Val2))
#define  Max(Val1, Val2) (((Val1) > (Val2))?(Val1):(Val2))


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

const HALBUFF_FunctionsTable_t HALBUFF_GenbuffFunctions = {
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
    UNUSED_PARAMETER(BufferHandle);
    return(0x100); /* 256 */
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
    UNUSED_PARAMETER(BufferHandle);
    UNUSED_PARAMETER(ForbiddenBorderArray_p);

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
    UNUSED_PARAMETER(BufferHandle);
#ifdef ST_OSLINUX
    return(LINUX_PAGE_ALIGNMENT > 0x800 ? LINUX_PAGE_ALIGNMENT : 0x800); /* at least 2048 */
#else
    return(0x800); /* 2048 */
#endif
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
    UNUSED_PARAMETER(BufferHandle);
    UNUSED_PARAMETER(CompressionLevel);
    UNUSED_PARAMETER(PictureWidth_p);
    UNUSED_PARAMETER(PictureHeight_p);
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
    U32     PictureHeight, PictureWidth;

    UNUSED_PARAMETER(BufferHandle);

    PictureWidth = *PictureWidth_p;
    PictureHeight = *PictureHeight_p;

    if (DecimationFactor == STVID_MPEG_DEBLOCK_RECONSTRUCTION)
    {
        /* no other decimation is applyed, so only STVID_MPEG_DEBLOCK_RECONSTRUCTION will be applyed for SD streams,
         * the size of decimated frame buffers should not exceed 720*576 */
        PictureWidth = Min(PictureWidth,720);
        PictureHeight = Min(PictureHeight,576);
    }
    else if (DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
    {
        if ((DecimationFactor & STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2) == STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2)
        {
            PictureWidth = 640; /* half of 1280 */
        }
        if (DecimationFactor & STVID_DECIMATION_FACTOR_H2)
        {
            PictureWidth /= 2;
        }
        if (DecimationFactor & STVID_DECIMATION_FACTOR_V2)
        {
            PictureHeight /= 2;
        }
        if (DecimationFactor & STVID_DECIMATION_FACTOR_H4)
        {
            PictureWidth /= 4;
        }
        if (DecimationFactor & STVID_DECIMATION_FACTOR_V4)
        {
            PictureHeight /= 4;
        }
        if (DecimationFactor & STVID_MPEG_DEBLOCK_RECONSTRUCTION)
        {
            /* The STVID_MPEG_DEBLOCK_RECONSTRUCTION is used in combination with an other decimation factor.
             * This mode is allowed only with SD streams. For HD ones the other selected decimation factor will be considered.
             * the size of the frame buffers should be big enought to contain either the decimated picture in the case of HD stream
             * or the deblocked picture for SD ones*/
            PictureWidth = Max(PictureWidth,720);
            PictureHeight = Max(PictureHeight,576);
        }
    }

    *PictureWidth_p = PictureWidth;
    *PictureHeight_p = PictureHeight;

} /* End of ApplyDecimationFactorToPictureSize() function */

/*******************************************************************************
Name        : ComputeBufferSizes
Description : Compute luma and chroma buffer sizes, according to the width,
              height and buffer type parameters.

              Macro bocks are stored as groups of MB one after each other,
              if the image is 7 MB x 5 MB and the MB group is 2*2 it will
              be stored as following:

                 ------------image------------
                 | 1   2   5   6   9  10  13 |
                 | 3   4   7   8  11  12  15 |
                 |14  17  18  21  22  25  26 |
                 |16  19  20  23  24  27  28 |
                 |29  30  33  34  37  38  41 |42 <-|
                 -----------------------------     |- dummy column
 Dummy slice ->   31  32  35  36  39  40  43  44 <-|

              To decode correctly, the chip needs to have complete groups of MB,
              the following dummy MB will be then allocated:
                  dummy slice:  31  32  35  36  39  40  43
                  dummy column: 42  44

              NB: for a MB group of w*h for Luma the associated Chroma MB group
              will be (2w)*h.

              cf Issue 334 for more information.

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
#define MACROBLOCK_GROUP_HEIGHT       	(2)
#define MACROBLOCK_GROUP_WIDTH        	(2)
#define MACROBLOCK_GROUP_Y_SURFACE	(MACROBLOCK_GROUP_HEIGHT * MACROBLOCK_GROUP_WIDTH)
#define MACROBLOCK_GROUP_C_SURFACE	(MACROBLOCK_GROUP_HEIGHT * MACROBLOCK_GROUP_WIDTH * 2)

    U32  WidthNbMB, HeightNbMB;

    UNUSED_PARAMETER(BufferHandle);

    /* Number of Macro Blocks for width */
    WidthNbMB= (AllocParams_p->PictureWidth + 15) / 16;

    /* Number of Macro Blocks for height */
    HeightNbMB= (AllocParams_p->PictureHeight + 15) / 16;
    /* Round it to upper number of needed slices (dummy slice) */
    HeightNbMB= ((HeightNbMB +  MACROBLOCK_GROUP_HEIGHT - 1) / MACROBLOCK_GROUP_HEIGHT) * MACROBLOCK_GROUP_HEIGHT;

    /* Set the luma buffer size.        */
    /* Round it to upper number of needed MB (dummy column) according to Luma MB group surface */
    *LumaBufferSize_p = ((HeightNbMB*WidthNbMB + MACROBLOCK_GROUP_Y_SURFACE - 1) / MACROBLOCK_GROUP_Y_SURFACE) * MACROBLOCK_GROUP_Y_SURFACE;
    *LumaBufferSize_p *= 256;	/* 256 bytes per Macro Block */

    /* Set the chroma buffer size.        */
    /* Round it to upper number of needed MB (dummy column) according to Chroma MB group surface */
    /* NB: For Chroma, SURFACE is twice the Luma surface */
    *ChromaBufferSize_p = ((HeightNbMB*WidthNbMB + MACROBLOCK_GROUP_C_SURFACE - 1) / MACROBLOCK_GROUP_C_SURFACE) * MACROBLOCK_GROUP_C_SURFACE;
    switch (AllocParams_p->BufferType)
    {
        case VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL :
            /* 1 byte luma + 1 byte chroma */
            *ChromaBufferSize_p *= 256;	 /* 256 bytes per Macro Block */
            break;

        default :
            /* 1 byte luma + 0.5 byte chroma */
            *ChromaBufferSize_p *= 128;	 /* 128 bytes per Macro Block */
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
    UNUSED_PARAMETER(BufferHandle);
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
    UNUSED_PARAMETER(BufferHandle);
} /* End of Term() function */

/* End of genbuff.c */
