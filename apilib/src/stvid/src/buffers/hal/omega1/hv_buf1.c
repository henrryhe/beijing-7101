/*******************************************************************************

File name   : hv_buf1.c

Description : HAL video buffer manager omega 1 family source file

COPYRIGHT (C) STMicroelectronics 2004

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
static U32 GetBitBufferAlignment(const HALBUFF_Handle_t BufferHandle);
static void GetForbiddenBorders(const HALBUFF_Handle_t BufferHandle, U32 * const NumberOfForbiddenBorders_p, void ** const ForbiddenBorderArray_p);
static U32 GetFrameBufferAlignment(const HALBUFF_Handle_t BufferHandle);
static ST_ErrorCode_t Init(const HALBUFF_Handle_t BufferHandle);
static void Term(const HALBUFF_Handle_t BufferHandle);


/* Global Variables --------------------------------------------------------- */

const HALBUFF_FunctionsTable_t HALBUFF_Omega1Functions = {
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
    return(0x400); /* 1024 */
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
    /* There is a need for a forbidden border between each memory chip on the board.
       Set to 3 for a set of 4 chips, could be less depending on the customer board. */
    *NumberOfForbiddenBorders_p = 3;

    /* Warning : values below are multiples of 0x200000, i.e. they apply constraints according to 16 MBits memory chips.
                 Would the chips be bigger on the board, values could be multiplied accordingly :
                 32 MBits <-> 0x400000 multiples, 64 MBits <-> 0x800000 multiples, etc. */
    ForbiddenBorderArray_p[0] = (void *) 0x200000;
    ForbiddenBorderArray_p[1] = (void *) 0x400000;
    ForbiddenBorderArray_p[2] = (void *) 0x600000;
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
    switch (((HALBUFF_Data_t *) BufferHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            /* On 5514 and 5516/17, we can have 128MBits on board and then the alignment constraint is 2048. */
            /* If the board has only 64MBits, the alignment constraint is only 1024. But here we can't       */
            /* detect the Shared Memory size. So we always return 2048. This introduces the a small lost     */
            /* of memory in this case. An optimization could be done later.                                  */
            return(0x800); /* 2048 */
            break;

        default : /* 5512 5518 */
            return(0x400); /* 1024 */
            break;
    }
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
        const STVID_CompressionLevel_t CompressionLevel, U32 * const PictureWidth_p, U32 * const PictureHeight_p )
{
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



/* End of hv_buf1.c */
