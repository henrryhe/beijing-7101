/*****************************************************************************

File name   : Sub_copy.c

Description : Sub picture Copy source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
29 Jan 2001        Taken from osd_copy.c                            TM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stlayer.h"
#include "sub_copy.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */

static void MaskedCopyLineUp(
        U32 FirstAddrSrc,
        U32 SrcFirstByteOffset,
        U32 SrcWidth,
        U32 FirstAddrDest,
        U32 DstFirstByteOffset,
        U8 BitsSLeft,
        U8 BitsSRight,
        U8 BitsDLeft,
        U8 BitsDRight,
        BOOL ChangeMasksSrc,
        BOOL ChangeMasksDst
        );

static void MaskedCopyLineDown(
        U32 FirstAddrSrc,
        U32 SrcFirstByteOffset,
        U32 SrcWidth,
        U32 FirstAddrDest,
        U32 DstFirstByteOffset,
        U8 BitsSLeft,
        U8 BitsSRight,
        U8 BitsDLeft,
        U8 BitsDRight,
        BOOL ChangeMasksSrc,
        BOOL ChangeMasksDst
        );

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : FirstByteTotalOffset
Description : Calculate offset of first byte concerned by the copy
Parameters  : Line number (0 to height-1), pitch, TRUE if half pitch,
              value of additional shift
Assumptions : AdditionalShift is 0 or 1
Limitations :
Returns     : offset
*******************************************************************************/
static  U32 FirstByteTotalOffset(
        U32 Line,
        U32 Pitch,
        BOOL PlusHalfPitch,
        U8 AdditionalShift
)
{
    U32 Offset;

    Offset = Line * Pitch;

    if (PlusHalfPitch)
    {
        /* Add length due to half pitches */
        Offset += Line / 2;
        /* Add 1 if odd line and additional shift not 0 */
        Offset += 1 & Line & AdditionalShift;
    }

    return(Offset);
}


/*******************************************************************************
Name        : MaskedCopyLineUp
Description : Copy a ligne up with masks (used in layersub_MaskedCopyBlock2D)
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void MaskedCopyLineUp(
        U32 FirstAddrSrc,
        U32 SrcFirstByteOffset,
        U32 SrcWidthOrg,
        U32 FirstAddrDest,
        U32 DstFirstByteOffset,
        U8 PassedBitsSLeft,
        U8 PassedBitsSRight,
        U8 PassedBitsDLeft,
        U8 PassedBitsDRight,
        BOOL ChangeMasksSrc,
        BOOL ChangeMasksDst
)
{
    U8 TempVal, LeftTempVal, RightTempVal, WriteVal;
    U32 x;
    U32 Width, DstWidth;
    U32 SrcWidth = SrcWidthOrg;
    U8 BitsSLeft = PassedBitsSLeft, BitsSRight = PassedBitsSRight, BitsDLeft = PassedBitsDLeft, BitsDRight = PassedBitsDRight;
    U8 MaskSLeft, MaskDLeft, MaskSRight, MaskDRight;
    BOOL ShiftLeft;

    /* Change masks if required (odd lines and half pitch) as follows:
        2 -> 6      6 -> 2      4 -> 8      8 -> 4  */
    if (ChangeMasksSrc)
    {
        BitsSLeft = (((BitsSLeft + 3) % 8) + 1);
        BitsSRight = (((BitsSRight + 3) % 8) + 1);
        /* Mask changed, so source width may need to be adjusted: one byte less/more may be concerned */
        if ((BitsSLeft + BitsSRight) > (PassedBitsSLeft + PassedBitsSRight))
        {
            /* Masks take 8 more bits, so width must be one byte less */
            SrcWidth--;
        }
        else if ((BitsSLeft + BitsSRight) < (PassedBitsSLeft + PassedBitsSRight))
        {
            /* Masks take 8 less bits, so width must be one byte more */
            SrcWidth++;
        }
    }
    if (ChangeMasksDst)
    {
        BitsDLeft = (((BitsDLeft + 3) % 8) + 1);
        BitsDRight = (((BitsDRight + 3) % 8) + 1);
    }

    /* Calculate default masks depending on bits to consider */
    MaskSLeft  =  ((1 << (    BitsSLeft )) - 1); /* 1 gives 00000001 */
    MaskDLeft  =  ((1 << (    BitsDLeft )) - 1); /* 3 gives 00000111 */
    MaskSRight = ~((1 << (8 - BitsSRight)) - 1); /* 3 gives 11100000 */
    MaskDRight = ~((1 << (8 - BitsDRight)) - 1); /* 1 gives 10000000 */

    /* Determine direction of shift: left or right depending on masks */
    if (BitsSLeft > BitsDLeft)
    {
        ShiftLeft = FALSE;
    }
    else
    {
        ShiftLeft = TRUE;
    }

    /* Calculate DstWidth: SrcWidth by default, then adjust +/- 1 depending on masks */
    DstWidth = SrcWidth;
    if ((BitsSLeft + BitsSRight) > (BitsDLeft + BitsDRight))
    {
        /* dest bits are 8 less than source bits: dest width must then be one byte more */
        DstWidth = SrcWidth + 1;
    }
    else if ((BitsSLeft + BitsSRight) < (BitsDLeft + BitsDRight))
    {
        /* dest bits are 8 more than source bits: dest width must then be one byte less */
        DstWidth = SrcWidth - 1;
    }
    /* Copy DstWidth bytes */
    Width = DstWidth;

    /* Particular case 1 byte to 1 byte */
    if ((DstWidth == 1) && (SrcWidth == 1))
    {
        /* Consider logical OR of the 2 masks, because they apply on the same single byte */
        MaskDLeft = (MaskDLeft & MaskDRight);
        MaskSLeft = (MaskSLeft & MaskSRight);
        /* Read source */
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset)) & MaskSLeft;
        /* Mask with destination */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;
        /* Added by TM 30 06 99 */
        if (ShiftLeft)
        {
            WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft))) & MaskDLeft;
        }
        else
        {
            WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft))) & MaskDLeft;
        }

        /* WriteVal |= TempVal & MaskDLeft;*/
        /* Write back destination */
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;

        /* Done: exit */
        return;
    }
    else if (DstWidth == 1)  /*SrcWidth == 2  and always shift left!*/
    {
        /* Mask for destination */
        MaskDLeft = (MaskDLeft & MaskDRight);
        /* Mask destination */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;

        /* Read source */
        RightTempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + 1)) & MaskSRight;
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset)) & MaskSLeft;

        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & MaskDLeft;
        /* Write back destination */
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;

        return;
    }
    else if (SrcWidth == 1)  /* DstWidth == 2  and always shift right , 2 bytes to write*/
    {
        /* Mask for source */
        MaskSLeft = (MaskSLeft & MaskSRight);
        /* Mask source */
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset)) & MaskSLeft;

        /* first  dst byte copy */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;
        WriteVal |= (TempVal >> (BitsSLeft - BitsDLeft)) & MaskDLeft;
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;

        /* last dst byte to copy */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset + 1 )) & ~MaskDRight;
        WriteVal |= (TempVal << (8 - BitsSLeft + BitsDLeft)) & MaskDRight;
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset + 1) = WriteVal;

        return;
    }


    /* Process first byte of line alone */
    /* Read source */
    RightTempVal = *(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + 1);
    TempVal = *(((U8 *) FirstAddrSrc) + SrcFirstByteOffset) & MaskSLeft;
    LeftTempVal = 0;
    /* Mask with destination */
    WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & MaskDLeft;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & MaskDLeft;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;

    /* Process middle bytes */
    for (x = 2; x <= Width - 1; x++)
    {
        /* Read source */
        LeftTempVal = TempVal;
        TempVal = RightTempVal;
        RightTempVal = *(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (x - 0));
        /* Reconstitute destination byte */
        if (ShiftLeft)
        {
            WriteVal = (TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft));
        }
        else
        {
            WriteVal = (TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft));
        }
        /* Write destination */
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset + (x - 1)) = WriteVal;
    }

    /* Process last byte of line alone */
    /* Read source */
    LeftTempVal = TempVal;
    /* Mask with destination */
    WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset + (Width - 1))) & ~MaskDRight;
    if (ShiftLeft)
    {
        TempVal = RightTempVal;
        RightTempVal = *(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (Width)) & MaskSRight;

        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & MaskDRight;
    }
    else
    {
        TempVal = RightTempVal & MaskSRight;
        RightTempVal = 0;

        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & MaskDRight;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + DstFirstByteOffset + (Width - 1)) = WriteVal;
}

/*******************************************************************************
Name        : MaskedCopyLineDown
Description : Copy a ligne down with masks (used in layersub_MaskedCopyBlock2D)
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static  void MaskedCopyLineDown(
        U32 FirstAddrSrc,
        U32 SrcFirstByteOffset,
        U32 SrcWidthOrg,
        U32 FirstAddrDest,
        U32 DstFirstByteOffset,
        U8 PassedBitsSLeft,
        U8 PassedBitsSRight,
        U8 PassedBitsDLeft,
        U8 PassedBitsDRight,
        BOOL ChangeMasksSrc,
        BOOL ChangeMasksDst
)
{
    U8 TempVal, LeftTempVal, RightTempVal, WriteVal;
    U32 x;
    U32 Width, DstWidth;
    U32 SrcWidth = SrcWidthOrg;
    U8 BitsSLeft = PassedBitsSLeft, BitsSRight = PassedBitsSRight, BitsDLeft = PassedBitsDLeft, BitsDRight = PassedBitsDRight;
    U8 MaskSLeft, MaskDLeft, MaskSRight, MaskDRight;
    BOOL ShiftLeft;

    /* Change masks if required (odd lines and half pitch) as follows:
        2 -> 6      6 -> 2      4 -> 8      8 -> 4  */
    if (ChangeMasksSrc)
    {
        BitsSLeft = (((BitsSLeft + 3) % 8) + 1);
        BitsSRight = (((BitsSRight + 3) % 8) + 1);
        /* Mask changed, so source width may need to be adjusted: one byte less/more may be concerned */
        if ((BitsSLeft + BitsSRight) > (PassedBitsSLeft + PassedBitsSRight))
        {
            /* Masks take 8 more bits, so width must be one byte less */
            SrcWidth--;
        }
        else if ((BitsSLeft + BitsSRight) < (PassedBitsSLeft + PassedBitsSRight))
        {
            /* Masks take 8 less bits, so width must be one byte more */
            SrcWidth++;
        }
    }
    if (ChangeMasksDst)
    {
        BitsDLeft = (((BitsDLeft + 3) % 8) + 1);
        BitsDRight = (((BitsDRight + 3) % 8) + 1);
    }

    /* Calculate default masks depending on bits to consider */
    MaskSLeft  =  ((1 << (    BitsSLeft )) - 1); /* 1 gives 00000001 */
    MaskDLeft  =  ((1 << (    BitsDLeft )) - 1); /* 3 gives 00000111 */
    MaskSRight = ~((1 << (8 - BitsSRight)) - 1); /* 3 gives 11100000 */
    MaskDRight = ~((1 << (8 - BitsDRight)) - 1); /* 1 gives 10000000 */

    /* Determine direction of shift: left or right depending on masks */
    if (BitsSLeft > BitsDLeft)
    {
        ShiftLeft = FALSE;
    }
    else
    {
        ShiftLeft = TRUE;
    }

    /* Calculate DstWidth: SrcWidth by default, then adjust +/- 1 depending on masks */
    DstWidth = SrcWidth;
    if ((BitsSLeft + BitsSRight) > (BitsDLeft + BitsDRight))
    {
        /* dest bits are 8 less than source bits: dest width must then be one byte more */
        DstWidth = SrcWidth + 1;
    }
    else if ((BitsSLeft + BitsSRight) < (BitsDLeft + BitsDRight))
    {
        /* dest bits are 8 more than source bits: dest width must then be one byte less */
        DstWidth = SrcWidth - 1;
    }
    /* Copy DstWidth bytes */
    Width = DstWidth;

    /* Particular case 1 byte to 1 byte */
    if ((DstWidth == 1) && (SrcWidth == 1))
    {
        /* Consider logical OR of the 2 masks, because they apply on the same single byte */
        MaskDLeft = (MaskDLeft & MaskDRight);
        MaskSLeft = (MaskSLeft & MaskSRight);
        /* Read source */
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset)) & MaskSLeft;
        /* Mask with destination */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;
        /* Added by TM 30 06 99 */
        if (ShiftLeft)
        {
            WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft))) & MaskDLeft;
        }
        else
        {
            WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft))) & MaskDLeft;
        }


        /* WriteVal |= TempVal & MaskDLeft;*/
        /* Write back destination */
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;

        /* Done: exit */
        return;
    }
    else if (DstWidth == 1)  /*SrcWidth == 2  and always shift left!*/
    {
        /* Mask for destination */
        MaskDLeft = (MaskDLeft & MaskDRight);
        /* Mask destination */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;

        /* Read source */
        RightTempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + 1)) & MaskSRight;
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset)) & MaskSLeft;

        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & MaskDLeft;
        /* Write back destination */
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;

        return;
    }
    else if (SrcWidth == 1)  /* DstWidth == 2  and always shift right , 2 bytes to write*/
    {
        /* Mask for source */
        MaskSLeft = (MaskSLeft & MaskSRight);
        /* Mask source */
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset)) & MaskSLeft;

        /* first  dst byte copy */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;
        WriteVal |= (TempVal >> (BitsSLeft - BitsDLeft)) & MaskDLeft;
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;

        /* last dst byte to copy */
        WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset + 1 )) & ~MaskDRight;
        WriteVal |= (TempVal << (8 - BitsSLeft + BitsDLeft)) & MaskDRight;
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset + 1) = WriteVal;

        return;
    }


    /* Process first byte of line alone */
    /* Read and mask source */
    LeftTempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (Width - 2)));
    /* Mask with destination */
    WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset + (Width - 1))) & ~MaskDRight;
    if (ShiftLeft)
    {
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (Width - 1)));
        RightTempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (Width))) & MaskSRight;

        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & MaskDRight;
    }
    else
    {
        TempVal = (*(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (Width - 1))) & MaskSRight;
        RightTempVal = 0;

        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & MaskDRight;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + DstFirstByteOffset + (Width - 1)) = WriteVal;

    /* Process middle bytes */
    for (x = Width - 1; x > 1; x--)
    {
        /* Read source */
        RightTempVal = TempVal;
        TempVal = LeftTempVal;
        LeftTempVal = *(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (x - 2));
/*        LeftTempVal = *(((U8 *) FirstAddrSrc) + SrcFirstByteOffset + (x + SrcWidth - Width - 2));*/
        /* Reconstitute destination byte */
        if (ShiftLeft)
        {
            WriteVal = (TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft));
        }
        else
        {
            WriteVal = (TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft));
        }
        /* Write destination */
        *(((U8 *) FirstAddrDest) + DstFirstByteOffset + (x - 1)) = WriteVal;
    }

    /* Process last byte of line alone */
    /* Read source */
    RightTempVal = TempVal;
    TempVal = LeftTempVal & MaskSLeft;
    LeftTempVal = 0;
    /* Mask with destination */
    WriteVal = (*(((U8 *) FirstAddrDest) + DstFirstByteOffset)) & ~MaskDLeft;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & MaskDLeft;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & MaskDLeft;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + DstFirstByteOffset) = WriteVal;
}


 /*******************************************************************************
Name        : layersub_MaskedCopyBlock2D
Description : Copy 2D with masks for first and last bytes and support of pitches multiple of 0.5 byte
Parameters  : id copy 2D + number of bits to consider in left and right bytes (1 to 8)
              + TRUE if pitch is equal to a value and a half, FALSE otherwise
Assumptions :
Limitations : Mirror not implemented as block handle for now !!!
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_MaskedCopyBlock2D(
        void *SrcAddr_p,
        U32 SrcWidth,
        U32 SrcHeight,
        U32 SrcPitch,
        void *DstAddr_p,
        U32 DstPitch,
        U8 BitsSLeft,
        U8 BitsSRight,
        U8 BitsDLeft,
        U8 BitsDRight,
        BOOL PlusHalfSrcPitch,
        BOOL PlusHalfDstPitch
)
{
    U32 y;
    U32 DstWidth = SrcWidth;
    U32 ChangeDirectionPoint;
    U32 FirstAddrSrc = (U32) SrcAddr_p, FirstAddrDest = (U32) DstAddr_p;
    U32 SrcOffset, DstOffset;
    U8 AdditionalSrc, AdditionalDst;
    BOOL ChangeMasksSrc = FALSE, ChangeMasksDst = FALSE;

    /* Calculate DstWidth */
    if ((BitsSLeft + BitsSRight) > (BitsDLeft + BitsDRight))
    {
        DstWidth = SrcWidth + 1;
    }
    else if ((BitsSLeft + BitsSRight) < (BitsDLeft + BitsDRight))
    {
        DstWidth = SrcWidth - 1;
    }

    /* Exit if pitches are invalid (smaller than width, or than (width-1) if PlusHalfPitch) */
    if ((SrcWidth > SrcPitch + 1) ||
        ((SrcWidth > SrcPitch) && (!PlusHalfSrcPitch)) ||
        ((DstWidth > DstPitch) && (!PlusHalfDstPitch)) ||
        (DstWidth > DstPitch + 1))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    /* Exit if number of bits are invalid */
    if ((BitsSLeft > 8) ||
        (BitsSRight > 8) ||
        (BitsDLeft > 8) ||
        (BitsDRight > 8) || /* At least one is greater than 8 */
        ((BitsSLeft * BitsSRight * BitsDLeft * BitsDRight) == 0))  /* At least one is 0 */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Overlap of regions: must care of directions of copy in order not to loose data */
    /* Depending on the location of the overlap, there will be 2 directions of copy with
       a particular point where the direction change. This is to support cases where there's
       overlap of the copy regions with a different pitch for source and destination.
       Example 1:
       Source:            |---|     |---|     |---|     |---|     |---|
       Destination:           |---|  |---|  |---|  |---|  |---|
       Direction of copy:     <<<<<<<<<<<<  >>>>>>>>>>>>>>>>>>>
       Example 2:
       Source:                |---|  |---|  |---|  |---|  |---|
       Destination:       |---|     |---|     |---|     |---|     |---|
       Direction of copy: >>>>>>>>>>>>>>>     <<<<<<<<<<<<<<<<<<<<<<<<<  */

    /* Calculate additional shift for the pitch when half pitch: 1 if mask <= 4, 0 otherwise */
    if (BitsSLeft <= 4)
    {
        AdditionalSrc = 1;
    }
    else
    {
        AdditionalSrc = 0;
    }
    if (BitsDLeft <= 4)
    {
        AdditionalDst = 1;
    }
    else
    {
        AdditionalDst = 0;
    }

    /* Start searching from first addresses */
    ChangeDirectionPoint = 1;
    if (FirstAddrDest < FirstAddrSrc)
    {
        /* Case where destination region comes before: start copy in forward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (SrcHeight + 1))
            {
                break;
            }
        } while ((FirstAddrDest + FirstByteTotalOffset(ChangeDirectionPoint - 1, DstPitch, PlusHalfDstPitch, AdditionalDst)) < (FirstAddrSrc + FirstByteTotalOffset(ChangeDirectionPoint - 1, SrcPitch, PlusHalfSrcPitch, AdditionalSrc)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from height down to ChangeDirectionPoint and from 1 up to ChangeDirectionPoint */
        /* From 1 to ChangeDirectionPoint-1, copy forward direction */
        for (y = 1; y < ChangeDirectionPoint; y++)
        {
            /* Check if the masks must be changed (case on odd lines when half pitch) */
            ChangeMasksDst = FALSE;
            ChangeMasksSrc = FALSE;
            if (((y - 1) & 1) == 1)
            {
                /* Odd lines */
                if (PlusHalfSrcPitch)
                {
                    ChangeMasksSrc = TRUE;
                }
                if (PlusHalfDstPitch)
                {
                    ChangeMasksDst = TRUE;
                }
            }
            /* Calculate first byte source and dest offsets */
            SrcOffset = FirstByteTotalOffset(y - 1, SrcPitch, PlusHalfSrcPitch, AdditionalSrc);
            DstOffset = FirstByteTotalOffset(y - 1, DstPitch, PlusHalfDstPitch, AdditionalDst);
            /* Copy one line */
            MaskedCopyLineUp(FirstAddrSrc, SrcOffset, SrcWidth, FirstAddrDest, DstOffset, BitsSLeft, BitsSRight, BitsDLeft, BitsDRight, ChangeMasksSrc, ChangeMasksDst);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy backward direction */
        for (y = SrcHeight; y >= ChangeDirectionPoint; y--)
        {
            /* Check if the masks must be changed (case on odd lines when half pitch) */
            ChangeMasksDst = FALSE;
            ChangeMasksSrc = FALSE;
            if (((y - 1) & 1) == 1)
            {
                /* Odd lines */
                if (PlusHalfSrcPitch)
                {
                    ChangeMasksSrc = TRUE;
                }
                if (PlusHalfDstPitch)
                {
                    ChangeMasksDst = TRUE;
                }
            }
            /* Calculate first byte source and dest offsets */
            SrcOffset = FirstByteTotalOffset(y - 1, SrcPitch, PlusHalfSrcPitch, AdditionalSrc);
            DstOffset = FirstByteTotalOffset(y - 1, DstPitch, PlusHalfDstPitch, AdditionalDst);
            /* Copy one line */
            MaskedCopyLineDown(FirstAddrSrc, SrcOffset, SrcWidth, FirstAddrDest, DstOffset, BitsSLeft, BitsSRight, BitsDLeft, BitsDRight, ChangeMasksSrc, ChangeMasksDst);
        }
    }
    else {
        /* Case where source region comes before: start copy in backward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (SrcHeight + 1))
            {
                break;
            }
        } while ((FirstAddrDest + FirstByteTotalOffset(ChangeDirectionPoint - 1, DstPitch, PlusHalfDstPitch, AdditionalDst)) > (FirstAddrSrc + FirstByteTotalOffset(ChangeDirectionPoint - 1, SrcPitch, PlusHalfSrcPitch, AdditionalSrc)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from ChangeDirectionPoint up to height and down to 1 */
        /* From 1 to ChangeDirectionPoint-1, copy backward direction */
        for (y = ChangeDirectionPoint - 1; y > 0; y--)
        {
            /* Check if the masks must be changed (case on odd lines when half pitch) */
            ChangeMasksDst = FALSE;
            ChangeMasksSrc = FALSE;
            if (((y - 1) & 1) == 1)
            {
                /* Odd lines */
                if (PlusHalfSrcPitch)
                {
                    ChangeMasksSrc = TRUE;
                }
                if (PlusHalfDstPitch)
                {
                    ChangeMasksDst = TRUE;
                }
            }
            /* Calculate first byte source and dest offsets */
            SrcOffset = FirstByteTotalOffset(y - 1, SrcPitch, PlusHalfSrcPitch, AdditionalSrc);
            DstOffset = FirstByteTotalOffset(y - 1, DstPitch, PlusHalfDstPitch, AdditionalDst);
            /* Copy one line */
            MaskedCopyLineDown(FirstAddrSrc, SrcOffset, SrcWidth, FirstAddrDest, DstOffset, BitsSLeft, BitsSRight, BitsDLeft, BitsDRight, ChangeMasksSrc, ChangeMasksDst);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy forward direction */
        for (y = ChangeDirectionPoint; y <= SrcHeight; y++)
        {
            /* Check if the masks must be changed (case on odd lines when half pitch) */
            ChangeMasksDst = FALSE;
            ChangeMasksSrc = FALSE;
            if (((y - 1) & 1) == 1)
            {
                /* Odd lines */
                if (PlusHalfSrcPitch)
                {
                    ChangeMasksSrc = TRUE;
                }
                if (PlusHalfDstPitch)
                {
                    ChangeMasksDst = TRUE;
                }
            }
            /* Calculate first byte source and dest offsets */
            SrcOffset = FirstByteTotalOffset(y - 1, SrcPitch, PlusHalfSrcPitch, AdditionalSrc);
            DstOffset = FirstByteTotalOffset(y - 1, DstPitch, PlusHalfDstPitch, AdditionalDst);
            /* Copy one line */
            MaskedCopyLineUp(FirstAddrSrc, SrcOffset, SrcWidth, FirstAddrDest, DstOffset, BitsSLeft, BitsSRight, BitsDLeft, BitsDRight, ChangeMasksSrc, ChangeMasksDst);
        }
    }

    return(ST_NO_ERROR);
}

/* End of Sub_copy.c */

