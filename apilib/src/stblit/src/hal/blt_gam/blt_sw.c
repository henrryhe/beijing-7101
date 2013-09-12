/*******************************************************************************

File name   : blt_sw.c

Description : Software optimisation routines for STBLIT

COPYRIGHT (C) STMicroelectronics 2002

Date               Modification                                     Name
----               ------------                                     ----
Apr 2002           Created                                           GS
May 2002           Modified                                          TM
******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "blt_init.h"
#include "blt_sw.h"
#include "stblit.h"
#include "stgxobj.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* For now, use STAVMEM for copyrects. There are some cases where the new code is
   faster. There are some where STAVMEM is faster. Its not clear exactly what causes
   this. Until it is better understood, then stick with STAVMEM, as 8bpp hasnt been
   optimised fully yet. */
#define USE_STAVMEM_FOR_COPYRECT 1

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */



/*******************************************************************************
Name        : GetBppForColorType
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetBppForColorType(STGXOBJ_ColorType_t TypeIn, U32 *Bpp_p)
{
    switch (TypeIn)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *Bpp_p = 32;
            break;

        case STGXOBJ_COLOR_TYPE_RGB565:
        case STGXOBJ_COLOR_TYPE_ARGB1555:
        case STGXOBJ_COLOR_TYPE_ARGB4444:
        case STGXOBJ_COLOR_TYPE_ACLUT88:
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            *Bpp_p = 16;
            break;

        case STGXOBJ_COLOR_TYPE_ACLUT44:
        case STGXOBJ_COLOR_TYPE_CLUT8:
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
        case STGXOBJ_COLOR_TYPE_BYTE:
            *Bpp_p = 8;
            break;

        case STGXOBJ_COLOR_TYPE_CLUT2:
            *Bpp_p = 2;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *Bpp_p = 4;
            break;

#if 0
/* We dont support these modes in the SW implementations yet! */
        case STGXOBJ_COLOR_TYPE_RGB888:
            *Bpp_p = 24;
            break;
       case STGXOBJ_COLOR_TYPE_ARGB8565:
       case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *Bpp_p = 24;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *Bpp_p = 1;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *Bpp_p = 2;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *Bpp_p = 4;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *Bpp_p = 1;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            *Bpp_p = 4;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
            *Bpp_p = 24;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *Bpp_p = 24;
            break;
#endif
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : FormatColorForMemory
Description : Generate a raw 32bit color value for writing to the frame buffer.
              For formats whose pixel boundaries align to 32 bits, the value
              will be replicated to 32bpp
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t FormatColorForMemory(STGXOBJ_Color_t* ColorIn_p, U32* ValueOut_p, U32 *Bpp_p)
{
    STGXOBJ_ColorType_t     TypeIn  = ColorIn_p->Type ;
    STGXOBJ_ColorValue_t    ValueIn = ColorIn_p->Value;

    switch (TypeIn)
    {
        case STGXOBJ_COLOR_TYPE_ARGB8888:
        case STGXOBJ_COLOR_TYPE_ARGB8888_255:
            *ValueOut_p = (U32)(((ValueIn.ARGB8888.Alpha & 0xFF) << 24) |
                                ((ValueIn.ARGB8888.R & 0xFF) << 16)     |
                                ((ValueIn.ARGB8888.G & 0xFF) << 8) |
                                 (ValueIn.ARGB8888.B & 0xFF));
            *Bpp_p = 32;
            break;
        case STGXOBJ_COLOR_TYPE_RGB565:
            *ValueOut_p = (U32)(((ValueIn.RGB565.R & 0x1F) << 11) |
                                ((ValueIn.RGB565.G & 0x3F) << 5) |
                                 (ValueIn.RGB565.B & 0x1F));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB1555:
            *ValueOut_p = (U32)(((ValueIn.ARGB1555.Alpha & 0x1) << 15) |
                                ((ValueIn.ARGB1555.R & 0x1F) << 10)     |
                                ((ValueIn.ARGB1555.G & 0x1F) << 5) |
                                 (ValueIn.ARGB1555.B & 0x1F));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ARGB4444:
            *ValueOut_p = (U32)(((ValueIn.ARGB4444.Alpha & 0xF) << 12) |
                                ((ValueIn.ARGB4444.R & 0xF) << 8) |
                                ((ValueIn.ARGB4444.G & 0xF) << 4) |
                                 (ValueIn.ARGB4444.B & 0xF));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT44:
            *ValueOut_p = (U32)(((ValueIn.ACLUT44.Alpha & 0xF) << 4) |
                                 (ValueIn.ACLUT44.PaletteEntry & 0xF));
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 8;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
        case STGXOBJ_COLOR_TYPE_BYTE:
            *ValueOut_p =  (U32)(ValueIn.CLUT8 & 0xFF);
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 8;
            break;
        case STGXOBJ_COLOR_TYPE_ACLUT88:
        case STGXOBJ_COLOR_TYPE_ACLUT88_255:
            *ValueOut_p = (U32)(((ValueIn.ACLUT88.Alpha & 0xFF) << 8) |
                                 (ValueIn.ACLUT88.PaletteEntry & 0xFF));
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA8:
        case STGXOBJ_COLOR_TYPE_ALPHA8_255:
            *ValueOut_p =  (U32)(ValueIn.ALPHA8 & 0xFF);
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 8;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 8) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 24) |
                                  (ValueIn.UnsignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 16;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422:
            *ValueOut_p =  (U32)(((ValueIn.SignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.SignedYCbCr888_444.Y  & 0xFF) << 8) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 24) |
                                  (ValueIn.SignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 16;
            break;
#if 0
/* We dont support these modes in the SW implementations yet! */
        case STGXOBJ_COLOR_TYPE_RGB888:
            *ValueOut_p = (U32)(((ValueIn.RGB888.R & 0xFF) << 16) |
                                ((ValueIn.RGB888.G & 0xFF) << 8) |
                                 (ValueIn.RGB888.B & 0xFF));
            *Bpp_p = 24;
            break;
       case STGXOBJ_COLOR_TYPE_ARGB8565:
       case STGXOBJ_COLOR_TYPE_ARGB8565_255:
            *ValueOut_p = (U32)(((ValueIn.ARGB8565.Alpha & 0xFF) << 16) |
                                ((ValueIn.ARGB8565.R & 0x1F) << 11) |
                                ((ValueIn.ARGB8565.G & 0x3F) << 5) |
                                 (ValueIn.ARGB8565.B & 0x1F));
            *Bpp_p = 24;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT1:
            *ValueOut_p =  (U32)(ValueIn.CLUT1 & 0x1);
            *ValueOut_p |= *ValueOut_p << 1;
            *ValueOut_p |= *ValueOut_p << 2;
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 1;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT2:
            *ValueOut_p =  (U32)(ValueIn.CLUT2 & 0x3);
            *ValueOut_p |= *ValueOut_p << 2;
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 2;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            *ValueOut_p =  (U32)(ValueIn.CLUT4 & 0xF);
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 4;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA1:
            *ValueOut_p =  (U32)(ValueIn.ALPHA1 & 0x1);
            *ValueOut_p |= *ValueOut_p << 1;
            *ValueOut_p |= *ValueOut_p << 2;
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 1;
            break;
        case STGXOBJ_COLOR_TYPE_ALPHA4:
            *ValueOut_p =  (U32)(ValueIn.ALPHA4 & 0xF);
            *ValueOut_p |= *ValueOut_p << 4;
            *ValueOut_p |= *ValueOut_p << 8;
            *ValueOut_p |= *ValueOut_p << 16;
            *Bpp_p = 4;
            break;
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)(((ValueIn.UnsignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.UnsignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.UnsignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 24;
            break;
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444:
            *ValueOut_p =  (U32)(((ValueIn.SignedYCbCr888_444.Cr & 0xFF) << 16) |
                                 ((ValueIn.SignedYCbCr888_444.Y  & 0xFF) << 8) |
                                  (ValueIn.SignedYCbCr888_444.Cb & 0xFF));
            *Bpp_p = 24;
            break;
#endif
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stblit_sw_fillrect
Description :
Parameters  :
Assumptions : Auto Clipping to bitmap boundaries done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_sw_fillrect(  stblit_Device_t*      Device_p,
                                    STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_Rectangle_t*  Rectangle_p,
                                    STGXOBJ_Rectangle_t*  MaskRectangle_p,
                                    STGXOBJ_Color_t*      Color_p,
                                    STBLIT_BlitContext_t* BlitContext_p)
{

    /* Assumption : Left, Top are inside the rectangle, Right, Bottom are ouside the rectangle */
    S32 Left, Right, Top, Bottom;
    U32 Bpp, RawColor;
    U8  *Data_p, *Line_p;
    U32 ByteCount, Line, Pixel;
    U32 Tmp;
    S32 LeftMask, TopMask;
	U32 Address;

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* We dont support colour conversion of any kind */
    if (Bitmap_p->ColorType != Color_p->Type)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (FormatColorForMemory(Color_p, &RawColor, &Bpp) != ST_NO_ERROR)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    Left    = Rectangle_p->PositionX;
    Right   = Left + Rectangle_p->Width;
    Top     = Rectangle_p->PositionY;
    Bottom  = Top + Rectangle_p->Height;

    LeftMask    =  MaskRectangle_p->PositionX ;
    TopMask     =  MaskRectangle_p->PositionY;

    /* Clip to Cliprect */
    if (BlitContext_p->EnableClipRectangle)
    {
        if (!BlitContext_p->WriteInsideClipRectangle)
        {
            /* This needs to generate up to 4 sub rectangles. Can be implemented
               quite easily if needed. Recommend toggling EnableClipRectangle
               and calling this routine for each of the 4 possible rectangles. */
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
        }

        if (Left < BlitContext_p->ClipRectangle.PositionX)
        {
            LeftMask += BlitContext_p->ClipRectangle.PositionX - Left;
            Left      = BlitContext_p->ClipRectangle.PositionX;

        }

        if (Top < BlitContext_p->ClipRectangle.PositionY)
        {
            TopMask += BlitContext_p->ClipRectangle.PositionY - Top;
            Top      = BlitContext_p->ClipRectangle.PositionY;
        }

        if (Right > (S32)(BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width))
        {
            Right      = BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width;
        }

        if (Bottom > (S32)(BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height))
        {
            Bottom      = BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height;
        }


    }

    /* Check to see if we are completely clipped out */
    if (Right <= Left || Bottom <= Top) {
        if (BlitContext_p->NotifyBlitCompletion)
        {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Notify completion as required. */
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
        }
        return ST_NO_ERROR;
    }

    /* At this point, we know the exact rectangle to fill. */

    /*  Convert Bitmap data virtual address to Cpu one */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp =  (U32)stblit_DeviceToCpu((void*)(Bitmap_p->Data1_p));
#else
    Tmp =  (U32)STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping));
#endif

    /* Work out the pointer to the first byte and the byte width to fill */
    Data_p = ((U8 *)Tmp) + Bitmap_p->Offset + Top * Bitmap_p->Pitch + (Left << (Bpp >> 4));
    ByteCount = (Right - Left) << (Bpp >> 4);

    if (Color_p->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
        Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
    {
        /* Compressed YUV modes have 3 bytes for even pixels, one byte for odd! */
        Data_p += (Left & 1);
        ByteCount += (Right & 1) - (Left & 1);
    }

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        S32 NbPixel;
        U32 NbLine;
        U8 *LineMask_p, *Mask_p, *PixelMask_p, *Pixel_p;
        U32 InitialMaskBitOffset;
        U32 BytePerPixel, i;

        /*  Convert Mask data virtual address to Cpu one */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32)stblit_DeviceToCpu((void*)(BlitContext_p->MaskBitmap_p->Data1_p));
#else
        Tmp =  (U32)STAVMEM_VirtualToCPU((void*)BlitContext_p->MaskBitmap_p->Data1_p, &(Device_p->VirtualMapping));
#endif

        /* Assumptions:
         *     + Mask is Alpha1
         *     + ALU Copy
         *     + Bpp >= 8 bit
         */
        Mask_p =  ((U8 *)Tmp) + BlitContext_p->MaskBitmap_p->Offset + TopMask * BlitContext_p->MaskBitmap_p->Pitch + (LeftMask >> 3);

        NbLine               = Bottom - Top;
        InitialMaskBitOffset = LeftMask & 0x7;
        BytePerPixel         = Bpp >> 3;


        switch (Bpp)
        {
            case 8 :
                if (BlitContext_p->MaskBitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB)
                {
                    /* left most pixel is the MSB! */
                    for (Line = 0, Line_p = Data_p, LineMask_p = Mask_p; Line < NbLine; Line++,Line_p += Bitmap_p->Pitch, LineMask_p += BlitContext_p->MaskBitmap_p->Pitch)
                    {
                        static U32 GiveWordMaskRightPixLSB[16] = {
                        0x00000000, 0xFF000000, 0x00FF0000, 0xFFFF0000,
                        0x0000FF00, 0xFF00FF00, 0x00FFFF00, 0xFFFFFF00,
                        0x000000FF, 0xFF0000FF, 0x00FF00FF, 0xFFFF00FF,
                        0x0000FFFF, 0xFF00FFFF, 0x00FFFFFF, 0xFFFFFFFF};


                        static U32 EndMask[] = {
                        0x00000000, 0x80000000, 0xC0000000, 0xE0000000,
                        0xF0000000, 0xF8000000, 0xFC000000, 0xFE000000,
                        0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
                        0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000,
                        0xFFFF0000, 0xFFFF8000, 0xFFFFC000, 0xFFFFE000,
                        0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFF00,
                        0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0,
                        0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE};

                        U32 Mask = 0, CarryMask = 0, TempMask;
                        U32 *Word_p, *WordTemp_p;
                        U8  *WordMask_p;
                        U32 WordMask;
                        S32 NbAlignedPixel;

                        WordMask_p  = LineMask_p;
                        Word_p      = (U32 *)(((U32)Line_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                        NbPixel     = Right - Left;

                        /* Work out number of pixels to fill */
                        NbAlignedPixel = NbPixel + (((U32)Line_p) & 3);

                        while (NbAlignedPixel > 0)
                        {
                            /* Mask = 0 carried into loop */

                            /* generate a 32 bit mask with the topmost bit representing the first pixel.
                            Even if the primitive is less than 32 bits, all 32bits need to be
                            initialised correctly. Any bit set in this mask will be set on the screen. */
                            if (NbPixel + LeftMask > 0)
                            {
                                Mask = WordMask_p[0] << 24;
                                if (NbPixel + LeftMask > 8)
                                {
                                    Mask |= WordMask_p[1] << 16;
                                    if (NbPixel + LeftMask > 16)
                                    {
                                        Mask |= WordMask_p[2] << 8;
                                        if (NbPixel + LeftMask > 24)
                                        {
                                            Mask |= WordMask_p[3];
                                        }
                                    }
                                }

                                if (LeftMask)
                                {
                                    if (NbPixel + LeftMask > 32)
                                    {
                                        Mask = (Mask << LeftMask) | (WordMask_p[4] >> (8 - LeftMask));
                                    }
                                    else
                                    {
                                        Mask <<= LeftMask;
                                    }
                                }

                                /* Remove any bits beyond the end of the region to draw */
                                if (NbPixel < 32)
                                    Mask &= EndMask[NbPixel];

                                WordMask_p += 4;    /* Advance to next 32 bits of mask */
                            }


                            /* Shift the mask to allow for 32 bit aligned writes. */
                            switch (((U32)Line_p) & 3)
                            {
                            case 1:
                                TempMask = Mask << 31;
                                Mask = (Mask >> 1) | CarryMask;
                                CarryMask = TempMask;
                                break;
                            case 2:
                                TempMask = Mask << 30;
                                Mask = (Mask >> 2) | CarryMask;
                                CarryMask = TempMask;
                                break;
                            case 3:
                                TempMask = Mask << 29;
                                Mask = (Mask >> 3) | CarryMask;
                                CarryMask = TempMask;
                                break;
                            }

                            WordTemp_p = Word_p;
                            while (Mask)
                            {
                                if (Mask >> 28)
                                {
                                    WordMask = GiveWordMaskRightPixLSB[Mask >> 28];
                                    WordTemp_p[0] = (WordTemp_p[0] & ~WordMask) | (WordMask & RawColor);
                                }
                                WordTemp_p ++;
                                Mask <<= 4;
                            }
                            Word_p += 8;

                            NbPixel -= 32;
                            NbAlignedPixel -= 32;
                        }
                    }
                }
                else
                {
                    /* left most pixel is the LSB! */
                    for (Line = 0, Line_p = Data_p, LineMask_p = Mask_p; Line < NbLine; Line++,Line_p += Bitmap_p->Pitch, LineMask_p += BlitContext_p->MaskBitmap_p->Pitch)
                    {
                        /* Only works for 8bpp */
                        static U32 GiveWordMaskRightPixMSB[16] = {
                            0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF,
                            0x00FF0000, 0x00FF00FF, 0x00FFFF00, 0x00FFFFFF,
                            0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
                            0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF};


                        static U32 EndMask[] = {
                            0x00000000, 0x00000001, 0x00000003, 0x00000007,
                            0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
                            0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
                            0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
                            0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
                            0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
                            0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
                            0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF};

                        U32 Mask = 0, CarryMask = 0, TempMask;
                        U32 *Word_p, *WordTemp_p;
                        U8  *WordMask_p;
                        U32 WordMask;
                        S32 NbAlignedPixel;

                        WordMask_p  = LineMask_p;
                        Word_p      = (U32 *)(((U32)Line_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                        NbPixel     = Right - Left;

                        /* Work out number of pixels to fill */
                        NbAlignedPixel = NbPixel + (((U32)Line_p) & 3);

                        while (NbAlignedPixel > 0)
                        {
                            /* Mask = 0 carried into loop */

                            /* generate a 32 bit mask with the topmost bit representing the first pixel.
                            Even if the primitive is less than 32 bits, all 32bits need to be
                            initialised correctly. Any bit set in this mask will be set on the screen. */
                            if (NbPixel + LeftMask > 0)
                            {
                                Mask = WordMask_p[0];
                                if (NbPixel + LeftMask > 8)
                                {
                                    Mask |= WordMask_p[1] << 8;
                                    if (NbPixel + LeftMask > 16)
                                    {
                                        Mask |= WordMask_p[2] << 16;
                                        if (NbPixel + LeftMask > 24)
                                        {
                                            Mask |= WordMask_p[3] << 24;
                                        }
                                    }
                                }

                                if (LeftMask)
                                {
                                    if (NbPixel + LeftMask > 32)
                                    {
                                        Mask = (Mask >> LeftMask) | (WordMask_p[4] << (32 - LeftMask));
                                    }
                                    else
                                    {
                                        Mask >>= LeftMask;
                                    }
                                }

                                /* Remove any bits beyond the end of the region to draw */
                                if (NbPixel < 32)
                                    Mask &= EndMask[NbPixel];

                                WordMask_p += 4;    /* Advance to next 32 bits of mask */
                            }


                            /* Shift the mask to allow for 32 bit aligned writes. */
                            switch (((U32)Line_p) & 3)
                            {
                            case 1:
                                TempMask = Mask >> 31;
                                Mask = (Mask << 1) | CarryMask;
                                CarryMask = TempMask;
                                break;
                            case 2:
                                TempMask = Mask >> 30;
                                Mask = (Mask << 2) | CarryMask;
                                CarryMask = TempMask;
                                break;
                            case 3:
                                TempMask = Mask >> 29;
                                Mask = (Mask << 3) | CarryMask;
                                CarryMask = TempMask;
                                break;
                            }

                            WordTemp_p = Word_p;
                            while (Mask)
                            {
                                if (Mask & 0xf)
                                {
                                    WordMask = GiveWordMaskRightPixMSB[Mask & 0xf];
                                    WordTemp_p[0] = (WordTemp_p[0] & ~WordMask) | (WordMask & RawColor);
                                }
                                WordTemp_p ++;
                                Mask >>= 4;
                            }
                            Word_p += 8;

                            NbPixel -= 32;
                            NbAlignedPixel -= 32;
                        }
                    }
                }
                break;
            case 32 :
                NbPixel = Right - Left;
                if (BlitContext_p->MaskBitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB)
                {
                    for (Line = 0, Line_p = Data_p, LineMask_p = Mask_p; Line < NbLine; Line++,Line_p += Bitmap_p->Pitch, LineMask_p += BlitContext_p->MaskBitmap_p->Pitch)
                    {
                        for (Pixel = 0, Pixel_p = Line_p, PixelMask_p = LineMask_p; Pixel < (U32)NbPixel; Pixel++, Pixel_p += 4, PixelMask_p = LineMask_p + ((Pixel + InitialMaskBitOffset) >> 3))
                        {
                            if ((((*PixelMask_p) >> ((~(Pixel + InitialMaskBitOffset)) & 0x7)) & 0x1) != 0 )
                            {
								/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
								Address = (U32)Pixel_p;

								*((U32*)Address) = RawColor;
                            }
                        }
                    }
                }
                else
                {
                    for (Line = 0, Line_p = Data_p, LineMask_p = Mask_p; Line < NbLine; Line++,Line_p += Bitmap_p->Pitch, LineMask_p += BlitContext_p->MaskBitmap_p->Pitch)
                    {
                        for (Pixel = 0, Pixel_p = Line_p, PixelMask_p = LineMask_p; Pixel < (U32)NbPixel; Pixel++, Pixel_p += 4, PixelMask_p = LineMask_p + ((Pixel + InitialMaskBitOffset) >> 3))
                        {
                            if ((((*PixelMask_p) >> ((Pixel + InitialMaskBitOffset) & 0x7)) & 0x1) != 0 )
                            {
								/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
								Address = (U32)Pixel_p;

                                *((U32*)Address) = RawColor;
                            }
                        }
                    }
                }
                break;
            default :
                NbPixel = Right - Left;
                if (BlitContext_p->MaskBitmap_p->SubByteFormat == STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB)
                {
                    for (Line = 0, Line_p = Data_p, LineMask_p = Mask_p; Line < NbLine; Line++,Line_p += Bitmap_p->Pitch, LineMask_p += BlitContext_p->MaskBitmap_p->Pitch)
                    {
                        for (Pixel = 0, Pixel_p = Line_p, PixelMask_p = LineMask_p; Pixel < (U32)NbPixel; Pixel++, Pixel_p += BytePerPixel, PixelMask_p = LineMask_p + ((Pixel + InitialMaskBitOffset) >> 3))
                        {
                            if ((((*PixelMask_p) >> ((~(Pixel + InitialMaskBitOffset)) & 0x7)) & 0x1) != 0 )
                            {
                                for (i=0;i < BytePerPixel;i++ )
                                {
                                    *((U8*)(Pixel_p + i)) = RawColor >> (i * 8);
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (Line = 0, Line_p = Data_p, LineMask_p = Mask_p; Line < NbLine; Line++,Line_p += Bitmap_p->Pitch, LineMask_p += BlitContext_p->MaskBitmap_p->Pitch)
                    {
                        for (Pixel = 0, Pixel_p = Line_p, PixelMask_p = LineMask_p; Pixel < (U32)NbPixel; Pixel++, Pixel_p += BytePerPixel, PixelMask_p = LineMask_p + ((Pixel + InitialMaskBitOffset) >> 3))
                        {
                            if ((((*PixelMask_p) >> ((Pixel + InitialMaskBitOffset) & 0x7)) & 0x1) != 0 )
                            {
                                for (i=0;i < BytePerPixel;i++ )
                                {
                                    *((U8*)(Pixel_p + i)) = RawColor >> (i * 8);
                                }
                            }
                        }
                    }
                }
                break;
        }
    }
    else /* No Mask */
    {
        /* Fill initial unaligned left strip. We have to worry about alignment
        and width in this case. */
        if (((U32)Data_p) & 3)
        {
            U32 nBytes = 4 - (((U32)Data_p) & 3);
            U32 Color = RawColor;

            /* Work out number of bytes to fill */
            if (ByteCount < nBytes)
                nBytes = ByteCount;

            if (nBytes == 2)
            {
                U8 C1 = (Color >> 16) & 0xFF;
                U8 C2 = Color >> 24;
                for (Line = Top, Line_p = Data_p; (S32)Line < Bottom; Line++, Line_p += Bitmap_p->Pitch)
                {
                    Line_p[0] = C1;
                    Line_p[1] = C2;
                }
                ByteCount -= 2;
                Data_p += 2;
            }
            else if (nBytes == 3)
            {
                U8 C1 = (Color >> 8) & 0xFF;
                U8 C2 = (Color >> 16) & 0xFF;
                U8 C3 = Color >> 24;
                for (Line = Top, Line_p = Data_p; (S32)Line < Bottom; Line++, Line_p += Bitmap_p->Pitch)
                {
                    Line_p[0] = C1;
                    Line_p[1] = C2;
                    Line_p[2] = C3;
                }
                ByteCount -= 3;
                Data_p += 3;
            }
            else
            {
                U8 C1 = Color >> 24;
                for (Line = Top, Line_p = Data_p; (S32)Line < Bottom; Line++, Line_p += Bitmap_p->Pitch)
                {
                    Line_p[0] = C1;
                }
                ByteCount -= 1;
                Data_p += 1;
            }
        }

        /* At this point Data_p is the first 32 bit aligned address of the first line */

        if (ByteCount >= 4) {
            U32 Count;
            for (Line = Top, Line_p = Data_p; (S32)Line < Bottom; Line++, Line_p += Bitmap_p->Pitch)
            {
                /*We have to use Word_p as U8* otherwise (used as U32*)  we have a wrong address when casting Line_p
                 * from U8* to U32* (in the case where Line_p address is not 32 bits aligned) */
                U8 *Word_p = Line_p;
                Count = ByteCount >> 2 ; /* /4 */

                /* Fill in 4 bytes with each loop. */
                while (Count)
                {
                    Word_p[0] = (RawColor) & 0xFF;
                    Word_p[1] = (RawColor >> 8) & 0xFF;
                    Word_p[2] = (RawColor >> 16) & 0xFF;
                    Word_p[3] = (RawColor >> 24) & 0xFF;
                    Word_p += 4;
                    Count --;
                }
            }

            /* Setup Bytecount and Data_p for final strip.*/
            Data_p += ByteCount & 0xFFFFFFFC;
            ByteCount &= 3;
        }

        /* Fill final unaligned right strip. We have to worry about alignment
        and width in this case. */
        if (ByteCount)
        {
            if (ByteCount == 2)
            {
                U16 C1 = RawColor & 0xFFFF;
                for (Line = Top, Line_p = Data_p; (S32)Line < Bottom; Line++, Line_p += Bitmap_p->Pitch)
                {
					/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
					Address = (U32)Line_p;

					*((U16 *)Address) = C1;
                }
            }
            else if (ByteCount == 3)
            {
                U16 C1 = RawColor & 0xFFFF;
                U8 C2 = (RawColor >> 16) & 0xFF;
                for (Line = Top, Line_p = Data_p; (S32)Line < Bottom; Line++, Line_p += Bitmap_p->Pitch)
                {
					/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
					Address = (U32)Line_p;

					*(U16 *)Address = C1;
                    Line_p[2] = C2;
                }
            }
            else
            {
                U8 C1 = RawColor & 0xFF;
                for (Line = Top, Line_p = Data_p; (S32)Line < Bottom; Line++, Line_p += Bitmap_p->Pitch)
                {
                    Line_p[0] = C1;
                }
            }
        }
    }


    if (BlitContext_p->NotifyBlitCompletion)
    {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
        /* Notify completion as required. */
        STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                               BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
    }


    return ST_NO_ERROR;
}
/*******************************************************************************
Name        : stblit_sw_xyl
Description :
Parameters  :
Assumptions :  Auto Clipping to bitmap boundaries done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_sw_xyl( stblit_Device_t*      Device_p,
                              STGXOBJ_Bitmap_t*     Bitmap_p,
                              STBLIT_XYL_t*         XYL_p,
                              U32                   Entries,
                              STGXOBJ_Color_t*      Color_p,
                              STBLIT_BlitContext_t* BlitContext_p )
{

    /* Assumption : ClipLeft, ClipTop are inside the rectangle, ClipRight, ClipBottom are ouside the rectangle */
    S32 ClipLeft, ClipRight, ClipTop, ClipBottom;
    S32 X1, X2;
    U32 Bpp, RawColor, i;
    U8  *Line_p;
    U32 ByteCount;
    U32 Tmp;
	U32 Address;

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);

    /* We dont support colour conversion of any kind */
    if (Bitmap_p->ColorType != Color_p->Type)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if (FormatColorForMemory(Color_p, &RawColor, &Bpp) != ST_NO_ERROR)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    /* Set default Clip to Bitmap size */
    ClipLeft = 0;
    ClipTop = 0;
    ClipRight = Bitmap_p->Width;
    ClipBottom = Bitmap_p->Height;

    /* Clip to Cliprect */
    if (BlitContext_p->EnableClipRectangle)
    {
        if (!BlitContext_p->WriteInsideClipRectangle)
        {
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
        }

        if (ClipLeft < BlitContext_p->ClipRectangle.PositionX)
            ClipLeft = BlitContext_p->ClipRectangle.PositionX;

        if (ClipTop < BlitContext_p->ClipRectangle.PositionY)
            ClipTop = BlitContext_p->ClipRectangle.PositionY;

        if (ClipRight > (S32)(BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width))
            ClipRight = BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width;

        if (ClipBottom > (S32)(BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height))
            ClipBottom = BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height;
    }

    /* At this point, we have a valid cliprect */
    for (i = 0; i < Entries; i++, XYL_p++)
    {
        /* Check span against clip rect */
        if (XYL_p->PositionY < ClipTop || XYL_p->PositionY >= ClipBottom)
            continue;

        X1 = XYL_p->PositionX;
        X2 = XYL_p->PositionX + XYL_p->Length;

        if (X1 < ClipLeft)
            X1 = ClipLeft;

        if (X2 > ClipRight)
            X2 = ClipRight;

        if (X2 <= X1)
            continue;

        /* Now need to fill between X1 and X2 */

        /*  Convert Bitmap data virtual address to Cpu one */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        Tmp =  (U32)stblit_DeviceToCpu((void*)(Bitmap_p->Data1_p));
#else
        Tmp =  (U32)STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p, &(Device_p->VirtualMapping));
#endif

        /* Work out the pointer to the first byte and the byte width to fill */
        Line_p = ((U8 *)Tmp) + Bitmap_p->Offset + XYL_p->PositionY * Bitmap_p->Pitch + (X1 << (Bpp >> 4));
        ByteCount = (X2 - X1) << (Bpp >> 4);

        if (Color_p->Type == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
            Color_p->Type == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
        {
            /* Compressed YUV modes have 3 bytes for even pixels, one byte for odd! */
            Line_p += (X1 & 1);
            ByteCount += (X2 & 1) - (X1 & 1);
        }

        /* Fill initial unaligned left strip. We have to worry about alignment
           and width in this case. */
        if (((U32)Line_p) & 3)
        {
            U32 nBytes = (((U32)Line_p) & 3);
            U32 Color = RawColor;

            /* Work out number of bytes to fill */
            nBytes = 4 - nBytes;
            if (ByteCount < nBytes)
                nBytes = ByteCount;

            if (nBytes == 2)
            {
                Line_p[0] = (Color >> 16) & 0xFF;
                Line_p[1] = Color >> 24;
                ByteCount -= 2;
                Line_p += 2;
            }
            else if (nBytes == 3)
            {
                Line_p[0] = (Color >> 8) & 0xFF;
                Line_p[1] = (Color >> 16) & 0xFF;
                Line_p[2] = Color >> 24;
                ByteCount -= 3;
                Line_p += 3;
            }
            else
            {
                Line_p[0] = Color >> 24;
                ByteCount -= 1;
                Line_p += 1;
            }
        }

        if (ByteCount >= 4) {
			U32 Count;
            U32 *Word_p = 0;

			/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
			Address = (U32)Line_p;

			Count = ByteCount >> 5;
            *Word_p = Address;


            /* Fill in 32 bytes with each loop. */
            while (Count)
            {
                Word_p[0] = RawColor;
                Word_p[1] = RawColor;
                Word_p[2] = RawColor;
                Word_p[3] = RawColor;
                Word_p[4] = RawColor;
                Word_p[5] = RawColor;
                Word_p[6] = RawColor;
                Word_p[7] = RawColor;
                Word_p += 8;
                Count--;
            }

            Count = (ByteCount & 0x1c) >> 2;
            while (Count)
            {
                Word_p[0] = RawColor;
                Word_p++;
                Count--;
            }

            /* Setup Bytecount and Line_p for final strip.*/
            Line_p += ByteCount & 0xFFFFFFFC;
            ByteCount &= 3;
        }

        if (ByteCount)
        {
            if (ByteCount == 2)
            {
				/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
				Address = (U32)Line_p;

				*(U16 *)Address = RawColor & 0xFFFF;
            }
            else if (ByteCount == 3)
            {
				/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
				Address = (U32)Line_p;

                *(U16 *)Address = RawColor & 0xFFFF;
                Line_p[2] = (RawColor >> 16) & 0xFF;
            }
            else
            {
                Line_p[0] = RawColor & 0xFF;
            }
        }



    }


    if (BlitContext_p->NotifyBlitCompletion)
    {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
        /* Notify completion as required. */
        STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                               BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name        : stblit_sw_copyrect
Description :
Parameters  :
Assumptions :  Auto Clipping to bitmap boundaries done at upper level
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_sw_copyrect(  stblit_Device_t*      Device_p,
                                    STGXOBJ_Bitmap_t*     SrcBitmap_p,
                                    STGXOBJ_Rectangle_t*  SrcRectangle_p,
                                    STGXOBJ_Bitmap_t*     DstBitmap_p,
                                    S32                   DstPositionX,
                                    S32                   DstPositionY,
                                    STBLIT_BlitContext_t* BlitContext_p )
{

    /* Assumption : SLeft, DLeft, DTop are inside the rectangle, SRight, DRight, DBottom are ouside the rectangle */
    S32 DLeft, DRight, DTop, DBottom, SLeft, STop;
    U32 Bpp;
    U8  *SData_p, *DData_p, *SrcAdress, *DstAdress, SrcByte, DstByte, SrcMask, DstMask;
    U32 BytesPerLine;
    U32 Tmp1, Tmp2;
    U32 Line, NbLine;
    int i;
	U32 Address;

#if !USE_STAVMEM_FOR_COPYRECT
    U32 ByteCount, Count;

    /* Data caches used during copy */
    U32 D0, D1, D2, D3, D4;
#endif

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Device_p);


    /* We dont support colour conversion of any kind */
    if (SrcBitmap_p->ColorType != DstBitmap_p->ColorType)
    {
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    if (GetBppForColorType(SrcBitmap_p->ColorType, &Bpp) != ST_NO_ERROR)
    {
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    if ( (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_NONE) &&
         ( (SrcBitmap_p->ColorType==STGXOBJ_COLOR_TYPE_CLUT2) || (SrcBitmap_p->ColorType==STGXOBJ_COLOR_TYPE_CLUT4) ) )
    {
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    SLeft    = SrcRectangle_p->PositionX;
    DLeft    = DstPositionX;
    DRight   = DLeft + SrcRectangle_p->Width;
    STop     = SrcRectangle_p->PositionY;
    DTop     = DstPositionY;
    DBottom  = DTop + SrcRectangle_p->Height;


    /* Clip to Cliprect */
    if (BlitContext_p->EnableClipRectangle)
    {
        if (!BlitContext_p->WriteInsideClipRectangle)
        {
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
        }

        if (DLeft < BlitContext_p->ClipRectangle.PositionX)
        {
            SLeft += BlitContext_p->ClipRectangle.PositionX - DLeft;
            DLeft = BlitContext_p->ClipRectangle.PositionX;
        }

        if (DTop < BlitContext_p->ClipRectangle.PositionY)
        {
            STop += BlitContext_p->ClipRectangle.PositionY - DTop;
            DTop = BlitContext_p->ClipRectangle.PositionY;
        }

        if (DRight > (S32)(BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width))
            DRight = BlitContext_p->ClipRectangle.PositionX + BlitContext_p->ClipRectangle.Width;

        if (DBottom > (S32)(BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height))
            DBottom = BlitContext_p->ClipRectangle.PositionY + BlitContext_p->ClipRectangle.Height;
    }

    /* Check to see if we are completely clipped out */
    if (DRight <= DLeft || DBottom <= DTop) {
        if (BlitContext_p->NotifyBlitCompletion)
        {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Notify completion as required. */
            STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
        }
        return ST_NO_ERROR;
    }

    /* At this point, we know the exact rectangle to copy */

    /*  Convert Bitmap data virtual address to Cpu one */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Tmp1 =  (U32)stblit_DeviceToCpu((void*)(SrcBitmap_p->Data1_p));
    Tmp2 =  (U32)stblit_DeviceToCpu((void*)(DstBitmap_p->Data1_p));
#else
    Tmp1 =  (U32)STAVMEM_VirtualToCPU((void*)SrcBitmap_p->Data1_p, &(Device_p->VirtualMapping));
    Tmp2 =  (U32)STAVMEM_VirtualToCPU((void*)DstBitmap_p->Data1_p, &(Device_p->VirtualMapping));
#endif

    /* Work out the pointer to the first byte and the byte width to copy */
    NbLine  = DBottom - DTop;

    switch (SrcBitmap_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_CLUT2:
            if ( (SLeft%4)==(DLeft%4) )  /* Source and Destination are similarly aligned in left side*/
            {
                if ((SLeft%4)!=0) /* Source and Destination are not byte aligned (in the left)*/
                {
                    if (SrcBitmap_p->SubByteFormat==DstBitmap_p->SubByteFormat)
                    {
                        /* Calculating Source and Destination Mask */
                        SrcMask = DstMask = 255;
                        if (SrcBitmap_p->SubByteFormat==STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB)
                        {
                            SrcMask = SrcMask >> ((SLeft%4)*2);
                            DstMask = ~SrcMask;
                        }
                        else
                        {
                            SrcMask = SrcMask << ((SLeft%4)*2);
                            DstMask = ~SrcMask;
                        }

                        /* Calculating Source and Destination adress of non aligned left-colon  */
                        SrcAdress = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + ((SLeft-(SLeft%4)) >> 2);
                        DstAdress = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + ((DLeft-(SLeft%4)) >> 2);

                        /* Starting filling the non aligned left-colon*/
                        for (i=1;i<=(S32)NbLine;i++)
                        {
                            SrcByte = STSYS_ReadRegMem8(SrcAdress);
                            DstByte = STSYS_ReadRegMem8(DstAdress);

                            SrcByte = SrcByte & SrcMask;
                            DstByte = DstByte & DstMask;

                            DstByte = DstByte | SrcByte;
                            STSYS_WriteRegMem8(DstAdress,DstByte);

                            SrcAdress+=SrcBitmap_p->Pitch;
                            DstAdress+=DstBitmap_p->Pitch;
                        }

                        /* Calculating Source and Destination adress of the first left aligned pixel (Used By DMA-Copy)*/
                        SLeft=SLeft+4-(SLeft%4);
                        DLeft=DLeft+4-(DLeft%4);
                    }
                    else
                    {
                        return ST_ERROR_FEATURE_NOT_SUPPORTED;  /* Not Yet done*/
                    }
                }
            }
            else
            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;  /* Not Yet done*/
            }

            if ((DRight%4)!=0) /* Source and Destination are not byte aligned (in the right)*/
            {
                if (SrcBitmap_p->SubByteFormat==DstBitmap_p->SubByteFormat)
                {
                    /* Calculating Source and Destination Mask */
                    SrcMask = DstMask = 255;
                    if (SrcBitmap_p->SubByteFormat==STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB)
                    {
                        SrcMask = SrcMask << ((4-DRight%4)*2);
                        DstMask = ~SrcMask;
                    }
                    else
                    {
                        SrcMask = SrcMask >> ((4-DRight%4)*2);
                        DstMask = ~SrcMask;
                    }

                    /* Calculating Source and Destination adress of the last right aligned pixel */
                    DRight=DRight-(DRight%4);

                    /* Calculating Source and Destination adress of non aligned right-colon  */
                    SrcAdress = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + (DRight >> 2);
                    DstAdress = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + (DRight >> 2);

                    /* Starting filling the non aligned right-colon*/
                    for (i=1;i<=(S32)NbLine;i++)
                    {
                        SrcByte = STSYS_ReadRegMem8(SrcAdress);
                        DstByte = STSYS_ReadRegMem8(DstAdress);

                        SrcByte = SrcByte & SrcMask;
                        DstByte = DstByte & DstMask;

                        DstByte = DstByte | SrcByte;
                        STSYS_WriteRegMem8(DstAdress,DstByte);

                        SrcAdress+=SrcBitmap_p->Pitch;
                        DstAdress+=DstBitmap_p->Pitch;
                    }
                }
            }

            /* Now calculating Source and Destination adress of the first aligned pixel*/
            SData_p = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + (SLeft >> 2);
            DData_p = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + (DLeft >> 2);
            BytesPerLine = ((DRight - DLeft) >> 2);
            break;

        case STGXOBJ_COLOR_TYPE_CLUT4:
            if ( (SLeft%2)==(DLeft%2) )  /* Source and Destination are similarly aligned in left side*/
            {
                if ((SLeft%2)==1) /* Source and Destination are not byte aligned (in the left)*/
                {
                    if (SrcBitmap_p->SubByteFormat==DstBitmap_p->SubByteFormat)
                    {
                        /* Calculating Source and Destination Mask */
                        SrcMask = DstMask = 255;
                        if (SrcBitmap_p->SubByteFormat==STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB)
                        {
                            SrcMask = SrcMask >> 4;
                            DstMask = ~SrcMask;
                        }
                        else
                        {
                            SrcMask = SrcMask << 4;
                            DstMask = ~SrcMask;
                        }

                        /* Calculating Source and Destination adress of non aligned left-colon  */
                        SrcAdress = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + ((SLeft-1) >> 1);
                        DstAdress = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + ((DLeft-1) >> 1);

                        /* Starting filling the non aligned left-colon*/
                        for (i=1;i<=(S32)NbLine;i++)
                        {
                            SrcByte = STSYS_ReadRegMem8(SrcAdress);
                            DstByte = STSYS_ReadRegMem8(DstAdress);

                            SrcByte = SrcByte & SrcMask;
                            DstByte = DstByte & DstMask;

                            DstByte = DstByte | SrcByte;
                            STSYS_WriteRegMem8(DstAdress,DstByte);

                            SrcAdress+=SrcBitmap_p->Pitch;
                            DstAdress+=DstBitmap_p->Pitch;
                        }

                        /* Calculating Source and Destination adress of the first left aligned pixel (Used By DMA-Copy)*/
                        SLeft=SLeft+1;
                        DLeft=DLeft+1;
                    }
                    else
                    {
                        return ST_ERROR_FEATURE_NOT_SUPPORTED;  /* Not Yet done*/
                    }
                }
            }
            else
            {
                return ST_ERROR_FEATURE_NOT_SUPPORTED;  /* Not Yet done*/
            }

            if ((DRight%2)!=0) /* Source and Destination are not byte aligned (in the right)*/
            {
                if (SrcBitmap_p->SubByteFormat==DstBitmap_p->SubByteFormat)
                {
                    /* Calculating Source and Destination Mask */
                    SrcMask = DstMask = 255;
                    if (SrcBitmap_p->SubByteFormat==STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB)
                    {
                        SrcMask = SrcMask << 4;
                        DstMask = ~SrcMask;
                    }
                    else
                    {
                        SrcMask = SrcMask >> 4;
                        DstMask = ~SrcMask;
                    }

                    /* Calculating Source and Destination adress of the last right aligned pixel */
                    DRight=DRight-1;

                    /* Calculating Source and Destination adress of non aligned right-colon  */
                    SrcAdress = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + (DRight >> 1);
                    DstAdress = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + (DRight >> 1);

                    /* Starting filling the non aligned right-colon*/
                    for (i=1;i<=(S32)NbLine;i++)
                    {
                        SrcByte = STSYS_ReadRegMem8(SrcAdress);
                        DstByte = STSYS_ReadRegMem8(DstAdress);

                        SrcByte = SrcByte & SrcMask;
                        DstByte = DstByte & DstMask;

                        DstByte = DstByte | SrcByte;
                        STSYS_WriteRegMem8(DstAdress,DstByte);

                        SrcAdress+=SrcBitmap_p->Pitch;
                        DstAdress+=DstBitmap_p->Pitch;
                    }
                }
            }

            /* Now calculating Source and Destination adress of the first aligned pixel*/
            SData_p = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + (SLeft >> 1);
            DData_p = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + (DLeft >> 1);
            BytesPerLine = ((DRight - DLeft) >> 1);
            break;

        default:
            SData_p = ((U8 *)Tmp1) + SrcBitmap_p->Offset + STop * SrcBitmap_p->Pitch + (SLeft << (Bpp >> 4));
            DData_p = ((U8 *)Tmp2) + DstBitmap_p->Offset + DTop * DstBitmap_p->Pitch + (DLeft << (Bpp >> 4));
            BytesPerLine = (DRight - DLeft) << (Bpp >> 4);
    }

    if (DstBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 ||
        DstBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
    {
        /* Compressed YUV modes have 3 bytes for even pixels, one byte for odd! */
        SData_p += (SLeft & 1);
        DData_p += (DLeft & 1);
        BytesPerLine += (DRight & 1) - (DLeft & 1);
    }

    if (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_NONE)
    {
#if USE_STAVMEM_FOR_COPYRECT
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        printf("COPY BLOCK 2D\n");
#else
        STAVMEM_CopyBlock2D((void*)SData_p,
                        BytesPerLine,
                        DBottom - DTop,
                        (U32)SrcBitmap_p->Pitch,
                        (void*)DData_p,(U32)DstBitmap_p->Pitch);

#endif
#else

        /* STAVMEM is exceptionally fast for 8 byte aligned copys. */
        if (((SData_p - DData_p) & 7) == 0 && BytesPerLine > 16)
        {
            STAVMEM_CopyBlock2D((void*)SData_p,
                            BytesPerLine,
                            DBottom - DTop,
                            (U32)SrcBitmap_p->Pitch,
                            (void*)DData_p,(U32)DstBitmap_p->Pitch);

            if (BlitContext_p->NotifyBlitCompletion)
            {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
                /* Notify completion as required. */
                STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                                   BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
            }

            return ST_NO_ERROR;
        }

        if (((SData_p - DData_p) & 3) == 0 && BytesPerLine > 3)
        {
            /* 32 bit alignment on copies */
            if (SData_p > DData_p)
            {
                /* left to right, top to bottom */
                for (Line = DTop; Line < DBottom; Line++)
                {
                    U32 *SLine_p = (U32 *) SData_p;
                    U32 *DLine_p = (U32 *) DData_p;
                    ByteCount = BytesPerLine;

                    /* Align to 32 bits on destination */
                    if ((U32)SLine_p & 1)
                    {
                        *(U8 *)DLine_p = *(U8 *)SLine_p;
                        SLine_p = (U32 *) (((U32)SLine_p)+1);
                        DLine_p = (U32 *) (((U32)DLine_p)+1);
                        ByteCount--;
                    }
                    if ((U32)SLine_p & 2)
                    {
                        *(U16 *)DLine_p = *(U16 *)SLine_p;
                        SLine_p = (U32 *) (((U32)SLine_p)+2);
                        DLine_p = (U32 *) (((U32)DLine_p)+2);
                        ByteCount -= 2;
                    }

                    Count = ByteCount >> 4;
                    while (Count)
                    {
                        D0 = SLine_p[0];
                        D1 = SLine_p[1];
                        D2 = SLine_p[2];
                        D3 = SLine_p[3];

                        DLine_p[0] = D0;
                        DLine_p[1] = D1;
                        DLine_p[2] = D2;
                        DLine_p[3] = D3;

                        SLine_p += 4;
                        DLine_p += 4;
                        Count--;
                    }

                    Count = (ByteCount >> 2) & 3;
                    while (Count)
                    {
                        DLine_p[0] = SLine_p[0];
                        SLine_p++;
                        DLine_p++;
                        Count--;
                    }

                    /* Fill final bits */
                    if (ByteCount & 2)
                    {
                        *(U16 *)DLine_p = *(U16 *)SLine_p;
                        SLine_p = (U32 *) (((U32)SLine_p)+2);
                        DLine_p = (U32 *) (((U32)DLine_p)+2);
                    }

                    if (ByteCount & 1)
                    {
                        *(U8 *)DLine_p = *(U8 *)SLine_p;
                    }

                    SData_p += SrcBitmap_p->Pitch;
                    DData_p += DstBitmap_p->Pitch;
                }
            }
            else
            {
                /* right to left, bottom to top */
                SData_p += (DBottom - DTop - 1) * SrcBitmap_p->Pitch;
                DData_p += (DBottom - DTop - 1) * DstBitmap_p->Pitch;

                for (Line = DBottom; Line > DTop; Line--)
                {
                    U32 *SLine_p = (U32 *) (SData_p + BytesPerLine);
                    U32 *DLine_p = (U32 *) (DData_p + BytesPerLine);
                    ByteCount = BytesPerLine;

                    /* Align to 32 bits on destination */
                    if ((U32)SLine_p & 1)
                    {
                        SLine_p = (U32 *) (((U32)SLine_p)-1);
                        DLine_p = (U32 *) (((U32)DLine_p)-1);
                        *(U8 *)DLine_p = *(U8 *)SLine_p;
                        ByteCount--;
                    }
                    if ((U32)SLine_p & 2)
                    {
                        SLine_p = (U32 *) (((U32)SLine_p)-2);
                        DLine_p = (U32 *) (((U32)DLine_p)-2);
                        *(U16 *)DLine_p = *(U16 *)SLine_p;
                        ByteCount -= 2;
                    }

                    Count = ByteCount >> 4;
                    while (Count)
                    {
                        SLine_p -= 4;
                        DLine_p -= 4;

                        D3 = SLine_p[3];
                        D2 = SLine_p[2];
                        D1 = SLine_p[1];
                        D0 = SLine_p[0];

                        DLine_p[3] = D3;
                        DLine_p[2] = D2;
                        DLine_p[1] = D1;
                        DLine_p[0] = D0;
                        Count--;
                    }

                    Count = (ByteCount >> 2) & 3;
                    while (Count)
                    {
                        SLine_p--;
                        DLine_p--;
                        DLine_p[0] = SLine_p[0];
                        Count--;
                    }

                    /* Fill final bits */
                    if (ByteCount & 2)
                    {
                        SLine_p = (U32 *) (((U32)SLine_p)-2);
                        DLine_p = (U32 *) (((U32)DLine_p)-2);
                        *(U16 *)DLine_p = *(U16 *)SLine_p;
                    }

                    if (ByteCount & 1)
                    {
                        SLine_p = (U32 *) (((U32)SLine_p)-1);
                        DLine_p = (U32 *) (((U32)DLine_p)-1);
                        *(U8 *)DLine_p = *(U8 *)SLine_p;
                    }

                    SData_p -= SrcBitmap_p->Pitch;
                    DData_p -= DstBitmap_p->Pitch;
                }
            }
        }
        else if (((SData_p - DData_p) & 1) == 0 && BytesPerLine > 1)
        {
            /* 16 bit alignment throughout */
            if (SData_p > DData_p)
            {
                /* left to right, top to bottom */
                for (Line = DTop; Line < DBottom; Line++)
                {
                    U32 *SLine_p = (U32 *) SData_p;
                    U32 *DLine_p = (U32 *) DData_p;
                    ByteCount = BytesPerLine;

                    /* Align to 32 bits on destination */
                    if ((U32)DLine_p & 1)
                    {
                        *(U8 *)DLine_p = *(U8 *)SLine_p;
                        SLine_p = (U32 *) (((U32)SLine_p)+1);
                        DLine_p = (U32 *) (((U32)DLine_p)+1);
                        ByteCount--;
                    }

                    /* Align destination to point to 32 bit aligned pixel */
                    if ((U32)DLine_p & 2)
                    {
                        *(U16 *)DLine_p = *(U16 *)SLine_p;
                        SLine_p = (U32 *) (((U32)SLine_p)+2);
                        DLine_p = (U32 *) (((U32)DLine_p)+2);
                        ByteCount -= 2;
                    }

                    /* We are always shifting by 16 bits. Therefore, we carry a valid
                       16 bits into our inner loop in D0. Our source points to the next
                       word of data. */

                    /* A ce niveau Dst est 32bit aligned et src aligned sur 16bit*/
                    /* On revient ensuite a un multiple de 32 sur la source */
                    SLine_p = (U32 *) ((U32)SLine_p & ~3);
                    D0 = *SLine_p++;

                    Count = ByteCount >> 4;
                    while (Count)
                    {
                        D1 = SLine_p[0];
                        D2 = SLine_p[1];
                        D3 = SLine_p[2];
                        D4 = SLine_p[3];

                        DLine_p[0] = (D0 >> 16) + (D1 << 16);
                        DLine_p[1] = (D1 >> 16) + (D2 << 16);
                        DLine_p[2] = (D2 >> 16) + (D3 << 16);
                        DLine_p[3] = (D3 >> 16) + (D4 << 16);
                        D0 = D4;

                        SLine_p += 4;
                        DLine_p += 4;
                        Count--;
                    }

                    Count = (ByteCount >> 2) & 3;
                    while (Count)
                    {
                        D1 = *SLine_p++;
                        *DLine_p++ = (D0 >> 16) + (D1 << 16);
                        D0 = D1;
                        Count--;
                    }

                    if (ByteCount & 2)
                    {
                        *(U16 *)DLine_p = (D0 >> 16);
                        SLine_p = (U32 *)(((U32)SLine_p) + 2);
                        DLine_p = (U32 *)(((U32)DLine_p) + 2);
                    }

                    if (ByteCount & 1)
                    {
                        *(U8 *)DLine_p = *(((U8 *)SLine_p)-2);
                    }

                    SData_p += SrcBitmap_p->Pitch;
                    DData_p += DstBitmap_p->Pitch;
                }
            }
            else
            {
                /* right to left, bottom to top */
                SData_p += (DBottom - DTop - 1) * SrcBitmap_p->Pitch;
                DData_p += (DBottom - DTop - 1) * DstBitmap_p->Pitch;

                for (Line = DBottom; Line > DTop; Line--)
                {
                    U32 *SLine_p = (U32 *) (SData_p + BytesPerLine);
                    U32 *DLine_p = (U32 *) (DData_p + BytesPerLine);
                    ByteCount = BytesPerLine;

                    if ((U32)SLine_p & 1)
                    {
                        SLine_p = (U32 *) (((U32)SLine_p)-1);
                        DLine_p = (U32 *) (((U32)DLine_p)-1);
                        *(U8 *)DLine_p = *(U8 *)SLine_p;
                        ByteCount--;
                    }

                    /* Align destination to point to 32 bit aligned pixel */
                    if ((U32)DLine_p & 2 || ByteCount == 2)
                    {
                        /* Also want to hit this case if just copying a single pixel */
                        SLine_p = (U32 *) (((U32)SLine_p)-2);
                        DLine_p = (U32 *) (((U32)DLine_p)-2);
                        *(U16 *)DLine_p = *(U16 *)SLine_p;
                        ByteCount -= 2;
                    }

                    /* We are always shifting by 16 bits. Therefore, we carry a valid
                       16 bits into our inner loop in D0. Our source points to the next
                       word of data. */
                    SLine_p = (U32 *) ((U32)SLine_p & ~3);
                    D0 = *SLine_p;

                    Count = ByteCount >> 4;
                    while (Count)
                    {
                        SLine_p -= 4;
                        DLine_p -= 4;

                        D1 = SLine_p[3];
                        D2 = SLine_p[2];
                        D3 = SLine_p[1];
                        D4 = SLine_p[0];

                        DLine_p[3] = (D1 >> 16) + (D0 << 16);
                        DLine_p[2] = (D2 >> 16) + (D1 << 16);
                        DLine_p[1] = (D3 >> 16) + (D2 << 16);
                        DLine_p[0] = (D4 >> 16) + (D3 << 16);
                        D0 = D4;

                        Count--;
                    }

                    Count = (ByteCount >> 2) & 3;
                    while (Count)
                    {
                        SLine_p--;
                        DLine_p--;
                        D1 = SLine_p[0];
                        DLine_p[0] = (D1 >> 16) + (D0 << 16);
                        D0 = D1;
                        Count--;
                    }

                    if (ByteCount & 2)
                    {
                        SLine_p = (U32 *)(((U32)SLine_p) - 2);
                        DLine_p = (U32 *)(((U32)DLine_p) - 2);
                        *(U16 *)DLine_p = (D0 & 0xFFFF);
                    }


                    if (ByteCount & 1)
                    {
                        SLine_p = (U32 *) (((U32)SLine_p)-1);
                        DLine_p = (U32 *) (((U32)DLine_p)-1);
                        *(U8 *)DLine_p = *(U8 *)SLine_p;
                    }

                    SData_p -= SrcBitmap_p->Pitch;
                    DData_p -= DstBitmap_p->Pitch;
                }
            }
        }
        else
        {
            /* 8 bit alignment */
            if (SData_p > DData_p)
            {
                /* left to right, top to bottom */
                for (Line = DTop; Line < DBottom; Line++)
                {
                    U8 *SLine_p = (U8 *) SData_p;
                    U8 *DLine_p = (U8 *) DData_p;
                    ByteCount = BytesPerLine;

                    Count = ByteCount >> 2;
                    while (Count)
                    {
                        DLine_p[0] = SLine_p[0];
                        DLine_p[1] = SLine_p[1];
                        DLine_p[2] = SLine_p[2];
                        DLine_p[3] = SLine_p[3];
                        SLine_p += 4;
                        DLine_p += 4;
                        Count--;
                    }

                    Count = ByteCount & 3;
                    while (Count)
                    {
                        DLine_p[0] = SLine_p[0];
                        SLine_p++;
                        DLine_p++;
                        Count--;
                    }

                    SData_p += SrcBitmap_p->Pitch;
                    DData_p += DstBitmap_p->Pitch;
                }
            }
            else
            {
                /* right to left, bottom to top */
                SData_p += (DBottom - DTop - 1) * SrcBitmap_p->Pitch;
                DData_p += (DBottom - DTop - 1) * DstBitmap_p->Pitch;

                for (Line = DBottom; Line > DTop; Line--)
                {
                    U8 *SLine_p = (U8 *) (SData_p + BytesPerLine);
                    U8 *DLine_p = (U8 *) (DData_p + BytesPerLine);
                    ByteCount = BytesPerLine;

                    Count = ByteCount >> 2;
                    while (Count)
                    {
                        SLine_p -= 4;
                        DLine_p -= 4;
                        DLine_p[3] = SLine_p[3];
                        DLine_p[2] = SLine_p[2];
                        DLine_p[1] = SLine_p[1];
                        DLine_p[0] = SLine_p[0];
                        Count--;
                    }

                    Count = ByteCount & 3;
                    while (Count)
                    {
                        SLine_p--;
                        DLine_p--;
                        DLine_p[0] = SLine_p[0];
                        Count--;
                    }

                    SData_p -= SrcBitmap_p->Pitch;
                    DData_p -= DstBitmap_p->Pitch;
                }
            }
        }
    #endif
    }
    else if (BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_SRC)
    {
        /* Only CLUT8 is supported */
        if ((BlitContext_p->ColorKey.Type == STGXOBJ_COLOR_KEY_TYPE_CLUT8) && (SrcBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8))
        {
            if (BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryEnable == TRUE)
            {
                if (BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryOut == FALSE)
                {
                    U8  *SLine_p, *DLine_p;

                    if (SData_p > DData_p)
                     {

                        static U32 EndMask[4]   = {0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF};

                        /* left to right, top to bottom */
                        for (Line = 0, SLine_p = SData_p, DLine_p = DData_p; Line < NbLine; Line++,SLine_p += SrcBitmap_p->Pitch,
                             DLine_p +=DstBitmap_p->Pitch)

                        {
                            U8*  SWord_p        = (U8 *)(((U32)SLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32* DWord_p        = (U32*)(((U32)DLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32 CarryMask       = 0;
                            U32 CarrySrc        = 0;
                            U32 DAlignment      = ((U32)DLine_p) & 3;
                            U32 SAlignment      = ((U32)SLine_p) & 3;
                            U32 SLeftBit        = SAlignment << 3;
                            S32 NbPixel         = DRight - DLeft;
                            S32 NbAlignedSPixel = NbPixel + SAlignment;

                            while (NbAlignedSPixel > 0)
                            {
                                U32 Mask;
                                U32 SWord;
                                U8* Word_p;

								/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
								Address = (U32)SWord_p;

								Mask     = 0;
                                SWord    = *((U32*)Address); /* SWord_p is 32 bits word aligned */
                                SWord_p += 4;                /* Advance to next 32 bits of Src */

                                if (SAlignment)
                                {
									/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
									Address = (U32)SWord_p;

									if (NbAlignedSPixel > 4)
                                    {
                                        SWord = (SWord >> SLeftBit) | (((U32)(*((U32*)(Address)))) << (32 - SLeftBit));
                                    }
                                    else
                                    {
                                        SWord >>= SLeftBit;
                                    }
                                }

                                Word_p = (U8*)&SWord;

                                if ((Word_p[0] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[0] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask = 0xFF;
                                }
                                if ((Word_p[1] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[1] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF00;
                                }
                                if ((Word_p[2] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[2] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF0000;
                                }
                                if ((Word_p[3] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[3] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF000000;
                                 }

                                 /* Remove any byte beyond the end of the region to draw */
                                 if (NbPixel < 4)
                                 {
                                     Mask &= EndMask[NbPixel];
                                 }

                                 /* Shift the SWord & Mask to allow for 32 bit aligned writes in the dst. */
                                 switch (DAlignment)
                                 {
                                    U32 TempMask, TempSrc;

                                 case 1:
                                     TempSrc   = SWord >> 24;
                                     TempMask  = Mask >> 24;
                                     SWord     = (SWord << 8) | CarrySrc;
                                     Mask      = ( Mask << 8) | CarryMask;
                                     CarrySrc  = TempSrc;
                                     CarryMask = TempMask;

                                     break;
                                case 2:
                                    TempSrc   = SWord >> 16;
                                    TempMask  = Mask >> 16;
                                    SWord     = (SWord << 16) | CarrySrc;
                                    Mask      = ( Mask << 16) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                case 3:
                                    TempSrc   = SWord >> 8;
                                    TempMask  = Mask >> 8;
                                    SWord     = (SWord << 24) | CarrySrc;
                                    Mask      = ( Mask << 24) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                }

                                if (Mask != 0)
                                {
                                    /* Write Dst */
                                    DWord_p[0] = (DWord_p[0] & ~Mask) | (Mask & SWord);
                                }

                                DWord_p += 1;
                                NbPixel -= 4;
                                NbAlignedSPixel -= 4;

                                if (NbAlignedSPixel <= 0)
                                {
                                    if (((S32)(NbPixel + DAlignment)) > 0)
                                    {
                                        /* Write remaining bytes in Dst */
                                        DWord_p[0] = (DWord_p[0] & ~CarryMask) | (CarryMask & CarrySrc);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        static U32 EndMask[4]   = {0x00000000, 0xFF000000, 0xFFFF0000, 0x0FFFFFF00};


                        /* Go to last pixel from last line */
                        DData_p += (DBottom - DTop - 1) * DstBitmap_p->Pitch + DRight - DLeft - 1;
                        SData_p += (DBottom - DTop - 1) * SrcBitmap_p->Pitch + DRight - DLeft - 1;

                        /* right, bottom to top */
                        for (Line = 0, SLine_p = SData_p, DLine_p = DData_p; Line < NbLine; Line++,SLine_p -= SrcBitmap_p->Pitch,
                             DLine_p -=DstBitmap_p->Pitch)

                        {
                            U8*  SWord_p        = (U8 *)(((U32)SLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32* DWord_p        = (U32*)(((U32)DLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32 CarryMask       = 0;
                            U32 CarrySrc        = 0;
                            U32 DAlignment      = ((U32)~((U32)DLine_p)) & 3;
                            U32 SAlignment      = ((U32)~((U32)SLine_p)) & 3;
                            U32 SLeftBit        = SAlignment << 3;
                            S32 NbPixel         = DRight - DLeft;
                            S32 NbAlignedSPixel = NbPixel + SAlignment;


                            while (NbAlignedSPixel > 0)
                            {
                                U32 Mask;
                                U32 SWord;
                                U8* Word_p;

								/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
								Address = (U32)SWord_p;

                                Mask     = 0;
                                SWord    = *((U32*)Address); /* SWord_p is 32 bits word aligned */
                                SWord_p -= 4;                /* Advance to previous 32 bits of Src */

                                if (SAlignment)
                                {
									/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
									Address = (U32)SWord_p;

									if (NbAlignedSPixel > 4)
                                    {
                                        SWord = (SWord << SLeftBit) | (((U32)(*((U32*)(Address)))) >> (32 - SLeftBit));
                                    }
                                    else
                                    {
                                        SWord <<= SLeftBit;
                                    }
                                }

                                Word_p = (U8*)&SWord;

                                if ((Word_p[0] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[0] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask = 0xFF;
                                }
                                if ((Word_p[1] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[1] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF00;
                                }
                                if ((Word_p[2] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[2] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF0000;
                                }
                                if ((Word_p[3] < BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) ||
                                    (Word_p[3] > BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF000000;
                                 }

                                 /* Remove any byte beyond the end of the region to draw */
                                 if (NbPixel < 4)
                                 {
                                     Mask &= EndMask[NbPixel];
                                 }

                                 /* Shift the SWord & Mask to allow for 32 bit aligned writes in the dst. */
                                 switch (DAlignment)
                                 {
                                    U32 TempMask, TempSrc;

                                 case 1:
                                     TempSrc   = SWord << 24;
                                     TempMask  = Mask << 24;
                                     SWord     = (SWord >> 8) | CarrySrc;
                                     Mask      = ( Mask >> 8) | CarryMask;
                                     CarrySrc  = TempSrc;
                                     CarryMask = TempMask;

                                     break;
                                case 2:
                                    TempSrc   = SWord << 16;
                                    TempMask  = Mask << 16;
                                    SWord     = (SWord >> 16) | CarrySrc;
                                    Mask      = ( Mask >> 16) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                case 3:
                                    TempSrc   = SWord << 8;
                                    TempMask  = Mask << 8;
                                    SWord     = (SWord >> 24) | CarrySrc;
                                    Mask      = ( Mask >> 24) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                }

                                if (Mask != 0)
                                {
                                    /* Write Dst */
                                    DWord_p[0] = (DWord_p[0] & ~Mask) | (Mask & SWord);
                                }

                                DWord_p -= 1;
                                NbPixel -= 4;
                                NbAlignedSPixel -= 4;

                                if (NbAlignedSPixel <= 0)
                                {
                                    if (((S32)(NbPixel + DAlignment)) > 0)
                                    {
                                        /* Write remaining bytes in Dst */
                                        DWord_p[0] = (DWord_p[0] & ~CarryMask) | (CarryMask & CarrySrc);
                                    }
                                }
                            }
                        }
                    }
                }
                else /*  BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryOut == TRUE */
                {
                    U8  *SLine_p, *DLine_p;

                    if (SData_p > DData_p)
                     {
                         static U32 EndMask[4]   = {0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF};

                        /* left to right, top to bottom */
                        for (Line = 0, SLine_p = SData_p, DLine_p = DData_p; Line < NbLine; Line++,SLine_p += SrcBitmap_p->Pitch,
                             DLine_p +=DstBitmap_p->Pitch)

                        {
                            U8*  SWord_p        = (U8 *)(((U32)SLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32* DWord_p        = (U32*)(((U32)DLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32 CarryMask       = 0;
                            U32 CarrySrc        = 0;
                            U32 DAlignment      = ((U32)DLine_p) & 3;
                            U32 SAlignment      = ((U32)SLine_p) & 3;
                            U32 SLeftBit        = SAlignment << 3;
                            S32 NbPixel         = DRight - DLeft;
                            S32 NbAlignedSPixel = NbPixel + SAlignment;



                            while (NbAlignedSPixel > 0)
                            {
                                U32 Mask;
                                U32 SWord;
                                U8* Word_p;

								/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
								Address = (U32)SWord_p;

								Mask     = 0;
                                SWord    = *((U32*)Address); /* SWord_p is 32 bits word aligned */
                                SWord_p += 4;                /* Advance to next 32 bits of Src */

                                if (SAlignment)
                                {
									/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
									Address = (U32)SWord_p;

									if (NbAlignedSPixel > 4)
                                    {
                                        SWord = (SWord >> SLeftBit) | (((U32)(*((U32*)(Address)))) << (32 - SLeftBit));
                                    }
                                    else
                                    {
                                        SWord >>= SLeftBit;
                                    }
                                }

                                Word_p = (U8*)&SWord;

                                if ((Word_p[0] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[0] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask = 0xFF;
                                }
                                if ((Word_p[1] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[1] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF00;
                                }
                                if ((Word_p[2] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[2] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF0000;
                                }
                                if ((Word_p[3] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[3] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF000000;
                                 }

                                 /* Remove any byte beyond the end of the region to draw */
                                 if (NbPixel < 4)
                                 {
                                     Mask &= EndMask[NbPixel];
                                 }

                                 /* Shift the SWord & Mask to allow for 32 bit aligned writes in the dst. */
                                 switch (DAlignment)
                                 {
                                    U32 TempMask, TempSrc;

                                 case 1:
                                     TempSrc   = SWord >> 24;
                                     TempMask  = Mask >> 24;
                                     SWord     = (SWord << 8) | CarrySrc;
                                     Mask      = ( Mask << 8) | CarryMask;
                                     CarrySrc  = TempSrc;
                                     CarryMask = TempMask;

                                     break;
                                case 2:
                                    TempSrc   = SWord >> 16;
                                    TempMask  = Mask >> 16;
                                    SWord     = (SWord << 16) | CarrySrc;
                                    Mask      = ( Mask << 16) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                case 3:
                                    TempSrc   = SWord >> 8;
                                    TempMask  = Mask >> 8;
                                    SWord     = (SWord << 24) | CarrySrc;
                                    Mask      = ( Mask << 24) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                }

                                if (Mask != 0)
                                {
                                    /* Write Dst */
                                    DWord_p[0] = (DWord_p[0] & ~Mask) | (Mask & SWord);
                                }

                                DWord_p += 1;
                                NbPixel -= 4;
                                NbAlignedSPixel -= 4;

                                if (NbAlignedSPixel <= 0)
                                {
                                    if (((S32)(NbPixel + DAlignment)) > 0)
                                    {
                                        /* Write remaining bytes in Dst */
                                        DWord_p[0] = (DWord_p[0] & ~CarryMask) | (CarryMask & CarrySrc);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        static U32 EndMask[4]   = {0x00000000, 0xFF000000, 0xFFFF0000, 0x0FFFFFF00};


                        /* Go to last pixel from last line */
                        DData_p += (DBottom - DTop - 1) * DstBitmap_p->Pitch + DRight - DLeft - 1;
                        SData_p += (DBottom - DTop - 1) * SrcBitmap_p->Pitch + DRight - DLeft - 1;

                        /* right, bottom to top */
                        for (Line = 0, SLine_p = SData_p, DLine_p = DData_p; Line < NbLine; Line++,SLine_p -= SrcBitmap_p->Pitch,
                             DLine_p -=DstBitmap_p->Pitch)

                        {
                            U8*  SWord_p        = (U8 *)(((U32)SLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32* DWord_p        = (U32*)(((U32)DLine_p) & 0xFFFFFFFC); /* Pointer to aligned DWORD */
                            U32 CarryMask       = 0;
                            U32 CarrySrc        = 0;
                            U32 DAlignment      = ((U32)~((U32)DLine_p)) & 3;
                            U32 SAlignment      = ((U32)~((U32)SLine_p)) & 3;
                            U32 SLeftBit        = SAlignment << 3;
                            S32 NbPixel         = DRight - DLeft;
                            S32 NbAlignedSPixel = NbPixel + SAlignment;

                            while (NbAlignedSPixel > 0)
                            {
                                U32 Mask;
                                U32 SWord;
                                U8* Word_p;

								/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
								Address = (U32)SWord_p;

                                Mask     = 0;
                                SWord    = *((U32*)Address); /* SWord_p is 32 bits word aligned */
                                SWord_p -= 4;                /* Advance to previous 32 bits of Src */

                                if (SAlignment)
                                {
                                    if (NbAlignedSPixel > 4)
                                    {
										/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
										Address = (U32)SWord_p;

										SWord = (SWord << SLeftBit) | (((U32)(*((U32*)(Address)))) >> (32 - SLeftBit));
                                    }
                                    else
                                    {
                                        SWord <<= SLeftBit;
                                    }
                                }

                                Word_p = (U8*)&SWord;

                                if ((Word_p[0] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[0] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask = 0xFF;
                                }
                                if ((Word_p[1] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[1] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF00;
                                }
                                if ((Word_p[2] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[2] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF0000;
                                }
                                if ((Word_p[3] >= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMin) &&
                                    (Word_p[3] <= BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryMax))
                                {
                                     Mask |= 0xFF000000;
                                 }

                                 /* Remove any byte beyond the end of the region to draw */
                                 if (NbPixel < 4)
                                 {
                                     Mask &= EndMask[NbPixel];
                                 }

                                 /* Shift the SWord & Mask to allow for 32 bit aligned writes in the dst. */
                                 switch (DAlignment)
                                 {
                                    U32 TempMask, TempSrc;

                                 case 1:
                                     TempSrc   = SWord << 24;
                                     TempMask  = Mask << 24;
                                     SWord     = (SWord >> 8) | CarrySrc;
                                     Mask      = ( Mask >> 8) | CarryMask;
                                     CarrySrc  = TempSrc;
                                     CarryMask = TempMask;

                                     break;
                                case 2:
                                    TempSrc   = SWord << 16;
                                    TempMask  = Mask << 16;
                                    SWord     = (SWord >> 16) | CarrySrc;
                                    Mask      = ( Mask >> 16) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                case 3:
                                    TempSrc   = SWord << 8;
                                    TempMask  = Mask << 8;
                                    SWord     = (SWord >> 24) | CarrySrc;
                                    Mask      = ( Mask >> 24) | CarryMask;
                                    CarrySrc  = TempSrc;
                                    CarryMask = TempMask;
                                    break;
                                }

                                if (Mask != 0)
                                {
                                    /* Write Dst */
                                    DWord_p[0] = (DWord_p[0] & ~Mask) | (Mask & SWord);
                                }

                                DWord_p -= 1;
                                NbPixel -= 4;
                                NbAlignedSPixel -= 4;

                                if (NbAlignedSPixel <= 0)
                                {
                                    if (((S32)(NbPixel + DAlignment)) > 0)
                                    {
                                        /* Write remaining bytes in Dst */
                                        DWord_p[0] = (DWord_p[0] & ~CarryMask) | (CarryMask & CarrySrc);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            /* else  (BlitContext_p->ColorKey.Value.CLUT8.PaletteEntryEnable == FALSE)
                => Always match => No src pixel are copied into dst */
        }
        else
        {
            return ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
    }
    else /* BlitContext_p->ColorKeyCopyMode == STBLIT_COLOR_KEY_MODE_DST */
    {
      return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    if (BlitContext_p->NotifyBlitCompletion)
    {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
        /* Notify completion as required. */
        STEVT_NotifySubscriber(Device_p->EvtHandle,Device_p->EventID[STBLIT_BLIT_COMPLETED_ID],
                               BlitContext_p->UserTag_p, BlitContext_p->EventSubscriberID);
#endif
    }

    return ST_NO_ERROR;

}

/* End of blt_sw.c */




