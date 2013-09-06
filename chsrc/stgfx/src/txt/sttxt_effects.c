/*
    File:           sttxt_effects.c
    Version:        1.09.45
    Author:         Gianluca Carcassi
    Date:           20.03.2001
    Description:    Teletext fonts effects routines

    Date            Modification            Name
    ----            ------------            ----
    17.01.2001      Creation                Gianluca Carcassi
    30.01.2001      First working version   Gianluca Carcassi
    27.02.2001      Eff. Fringing &Rounding Gianluca Carcassi
    01.03.2001      Fix effects :-)         Gianluca Carcassi
    20.03.2001      Minor fixes             Gianluca Carcassi
    26.04.2001      Comments added          Gianluca Carcassi
*/

#include "sttxt.h"

/*
    General informations:

    in the bitmap passed  to the effect functions  the first pointer (data1)  is
    the destination  glyph  zoomed  2x2  from  the  original  size so any dot is
    a matrix of 4 pixels.  The  second pointer (data2) is the mask of foreground
    pixels in original  size.

    All algoritms  finds edges of   characters scanning two sequent rows  in the
    mask.

    Main variables used in any function:
        x, y                indexes for the current pixel
        GlyphPix_p          pointer to the start of destination glyph
        UpperPix_p          pointer to the upper line in the glyph mask, is used
                            in the scan of edges
        LowerPix_p          pointer to the lower line in the glyph mask, is used
                            in the scan of edges
        Color               is the effect color
        Width               original width of the glyph
        Height              original height of the glyph
        Width2X             width x 2 used in destination bitmap for pixel positioning
        Width4X             width x 4 used in destination bitmap for pixel positioning
*/


/*
    Effect: Fringing

    Local variables:
        IsUpperLineFlagL    Flag from Left of the Upper Line
        IsUpperLineFlagD    Flag from Down of the Upper Line
        IsBottomLineFlagL   Flag from Left of the Bottom Line
        IsBottomLineFlagU   Flag from Up of the Bottom Line

    Method: look for any edge of glyph scanning two sequent rows
*/
void sttxt_EffectFringing(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32 x, y;
    BOOL IsUpperLineFlagL, IsUpperLineFlagD, IsBottomLineFlagL, IsBottomLineFlagU;
    U8  Color   = FontAttr_p->FringeColor;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8* GlyphPix_p  = SrcDblBitmap_p->Data1_p;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    register U8* UpperPix_p = SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<Height; ++y)
    {
        IsUpperLineFlagL  = IsUpperLineFlagD = IsBottomLineFlagL =
                                               IsBottomLineFlagU = FALSE;
        for(x=0; x<Width; ++x)
        {
            /* Upper Line checks start here */
            if (*UpperPix_p)
            {
                /* if entering in the char apply effect */
                if (!IsUpperLineFlagL)
                {
                    IsUpperLineFlagL = TRUE;
                    if (x)
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)-1;
                        *Pixel_p = *(Pixel_p+Width2X) = Color;
                    }
                }
                /* if bottom edge apply effect */
                if (!*LowerPix_p)
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X;
                    *Pixel_p = *(Pixel_p+1) = Color;
                    /* check the SE corner */
                    if (!IsUpperLineFlagD)
                    {
                        if (x && !*(LowerPix_p-1)) *(Pixel_p-1) = Color;
                        IsUpperLineFlagD = TRUE;
                    }
                }
                else    IsUpperLineFlagD = TRUE;
            }
            else
            {
                /* check the SW corner */
                if (IsUpperLineFlagD)
                {
                    IsUpperLineFlagD = FALSE;
                    if (!*(LowerPix_p-1) && !*LowerPix_p)
                        *(GlyphPix_p+(x<<1)+Width4X) = Color;
                }
                /* if exiting from the char apply effect */
                if (IsUpperLineFlagL)
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1);
                    *Pixel_p = *(Pixel_p+Width2X) = Color;
                    IsUpperLineFlagL = FALSE;
                }
            }
            /* Lower Line checks start here */
            if (*LowerPix_p && (y<(Height-1)))
            {
                /* if upper edge apply effect */
                if (!*UpperPix_p)
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X;
                    *Pixel_p = *(Pixel_p+1) = Color;
                    /* check the NE corner */
                    if (!IsBottomLineFlagU)
                    {
                        if (x && !*(UpperPix_p-1)) *(Pixel_p-1) = Color;
                        IsBottomLineFlagU = TRUE;
                    }
                }
                else    IsBottomLineFlagU = TRUE;
            }
            /* check the NW corner */
            else if (IsBottomLineFlagU)
            {
                IsBottomLineFlagU = FALSE;
                if (!*UpperPix_p && !*(UpperPix_p-1))
                    *(GlyphPix_p+(x<<1)+Width2X) = Color;
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function EffectFringing */










/*
    Effect: Rounding

    Method: look for any couple of pixels in a cross position
            checking (p(x,y) & p(x+1,y+1)) and (p(x,y+1) & p(x+1,y))
*/
void sttxt_EffectRounding(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                          STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32 x, y;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->RoundingColor;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p + Width2X;
    register U8* UpperPix_p = SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    --Height; --Width;
    for(y=0; y<Height; ++y)
    {
        for(x=0; x<Width; ++x)
        {
            /* if pixels crossed (way 1) apply effect */
            if ( *UpperPix_p && *(LowerPix_p+1) && !*LowerPix_p && !*(UpperPix_p+1) )
            {
                U8* Pixel_p = GlyphPix_p+(x<<1)+2;
                *Pixel_p = *(Pixel_p+Width2X-1) = Color;
            }
            /* if pixels crossed (way 2) apply effect */
            if ( *LowerPix_p && *(UpperPix_p+1) && !*UpperPix_p && !*(LowerPix_p+1) )
            {
                U8* Pixel_p = GlyphPix_p+(x<<1)+1;
                *Pixel_p = *(Pixel_p+Width2X+1) = Color;
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        ++UpperPix_p; ++LowerPix_p;
        /* next line in the destination bitmap */
        GlyphPix_p += Width4X;
    }
}   /* End of function EffectRounding */






/*
    Effect: Shadow North

    Local variables:
        SWidthYrow          is the maximum Y shadow for the current row
        SWidthYpix          is the maximum Y shadow for the current pixel ( <= SWidthYrow )

    Method: find where orizontal edges enters in the character, then apply effect
*/
void sttxt_EffectShadowN(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32 x, y;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    --Height;
    for(y=0; y<Height; ++y)
    {
        /* calculate the maximum Y shadow for the current row */
        S32 SWidthYrow;
        {
            S32 Delta = ((y+1)<<1)-SWidth;
            SWidthYrow = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        for(x=0; x<Width; ++x)
        {
            /* North */
            if (*LowerPix_p && !*UpperPix_p)
            {
                /* calculate the maximum Y shadow for the current pixel */
                /* shadow must not overwrite others foreground pixels */
                S32 SWidthYpix = SWidthYrow;
                {
                    U8* Next_p = UpperPix_p;
                    S32 i;
                    for(i=2; i<SWidthYpix; i += 2)
                        if (*(Next_p -= Width)) break;
                    if ( i < SWidthYpix )    SWidthYpix = i;
                }
                /* draw shadow */
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X;
                    S32 i;
                    for(i=SWidthYpix; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+1) = Color;
                        Pixel_p -= Width2X;
                    }
                }
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        /* next line in the destination bitmap */
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowN */



/*
    Effect: Shadow South

    Local variables:
        SWidthYrow          is the maximum Y shadow for the current row
        SWidthYpix          is the maximum Y shadow for the current pixel ( <= SWidthYrow )

    Method: find where orizontal edges exits from the character, then apply effect
*/
void sttxt_EffectShadowS(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32 x, y;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<(Height-1); ++y)
    {
        /* calculate the maximum Y shadow for the current row */
        S32 SWidthYrow;
        {
            S32 Delta = ((Height-y)<<1)-SWidth;
            SWidthYrow = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        for(x=0; x<Width; ++x)
        {
            /* South */
            if (*UpperPix_p && !*LowerPix_p)
            {
                /* calculate the maximum Y shadow for the current pixel */
                /* shadow must not overwrite others foreground pixels */
                S32 SWidthYpix = SWidthYrow;
                {
                    U8* Next_p = LowerPix_p;
                    S32 i;
                    for(i=2; i<SWidthYpix; i += 2)
                        if (*(Next_p += Width)) break;
                    if ( i < SWidthYpix )    SWidthYpix = i;
                }
                /* draw shadow */
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X;
                    S32 i;
                    for(i=SWidthYpix; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+1) = Color;
                        Pixel_p += Width2X;
                    }
                }
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        /* next line in the destination bitmap */
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowS */



/*
    Effect: Shadow East

    Local variables:
        SWidthYrow          is the maximum Y shadow for the current row
        SWidthYpix          is the maximum Y shadow for the current pixel ( <= SWidthYrow )
        IsInside            boolean flag, check if current pixel is inside the character

    Method: find where vertical edges exits from the character, then apply effect
*/
void sttxt_EffectShadowE(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32  x, y;
    BOOL IsInside;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;

    for(y=0; y<Height; ++y)
    {
        IsInside = FALSE;
        for(x=0; x<Width; ++x)
        {
            /* East */
            if (*UpperPix_p)    IsInside = TRUE;
            else if (IsInside)
            {
                /* calculate the maximum X shadow for the current column */
                S32 SWidthX;
                IsInside =  FALSE;
                {
                    S32 Delta = ((Width-x)<<1)-SWidth;
                    SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                }
                /* calculate the maximum X shadow for the current pixel */
                /* shadow must not overwrite others foreground pixels */
                {
                    U8* Next_p = UpperPix_p;
                    S32 i;
                    for(i=2; i<SWidthX; i += 2)
                        if (*(++Next_p)) break;
                    if ( i < SWidthX )    SWidthX = i;
                }
                /* draw shadow */
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1);
                    S32 i;
                    for(i=SWidthX; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+Width2X) = Color;
                        ++Pixel_p;
                    }
                }
            }
            ++UpperPix_p;
        }
        /* next line in the destination bitmap */
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowE */



/*
    Effect: Shadow West

    Local variables:
        SWidthYrow          is the maximum Y shadow for the current row
        SWidthYpix          is the maximum Y shadow for the current pixel ( <= SWidthYrow )
        IsInside            boolean flag, check if current pixel is inside the character

    Method: find where vertical edges enters in the character, then apply effect
*/
void sttxt_EffectShadowW(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32  x, y;
    BOOL IsInside;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;

    for(y=0; y<Height; ++y)
    {
        IsInside = FALSE;
        for(x=0; x<Width; ++x)
        {
            /* West */
            if (*UpperPix_p)
            {
                if (!IsInside && x)
                {
                    /* calculate the maximum X shadow for the current column */
                    S32 SWidthX;
                    {
                         S32 Delta = (x<<1)-SWidth;
                         SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                    }
                    /* calculate the maximum X shadow for the current pixel */
                    /* shadow must not overwrite others foreground pixels */
                    {
                        U8* Next_p = UpperPix_p-1;
                        S32 i;
                        for(i=2; i<SWidthX; i += 2)
                            if (*(--Next_p)) break;
                        if ( i < SWidthX )    SWidthX = i;
                    }
                    /* draw shadow */
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)-1;
                        S32 i;
                        for(i=SWidthX; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+Width2X) = Color;
                            --Pixel_p;
                        }
                    }
                }
                IsInside = TRUE;
            }
            else    IsInside = FALSE;
            ++UpperPix_p;
        }
        /* next line in the destination bitmap */
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowW */





/*

    All  the others  shadow effects  are a  simple combinations  of the  already
    commented functions. So please refer to the previous functions to understand
    the code

*/

void sttxt_EffectShadowNE(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32  x, y;
    BOOL IsInsideU, IsInsideD;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<Height; ++y)
    {
        S32 SWidthYrow;
        {
            S32 Delta = ((y+1)<<1)-SWidth;
            SWidthYrow = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        IsInsideU = IsInsideD = FALSE;
        for(x=0; x<Width; ++x)
        {
            if (*UpperPix_p)    IsInsideU = TRUE;
            else if (IsInsideU)
            {
                S32 SWidthX;
                IsInsideU =  FALSE;
                /* East */
                {
                    S32 Delta = ((Width-x)<<1)-SWidth;
                    SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                }
                {
                    U8* Next_p = UpperPix_p;
                    S32 i;
                    for(i=2; i<SWidthX; i += 2)
                        if (*(++Next_p)) break;
                    if ( i < SWidthX )    SWidthX = i;
                }
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1);
                    S32 i;
                    for(i=0; i<SWidthX; ++i)
                    {
                        *Pixel_p = *(Pixel_p+Width2X) = Color;
                        ++Pixel_p;
                    }
                }
            }
            if (*LowerPix_p && (y<(Height-1)))
            {
                IsInsideD = TRUE;
                /* North */
                if (!*UpperPix_p)
                {
                    S32 SWidthYpix = SWidthYrow;
                    {
                        U8* Next_p = UpperPix_p;
                        S32 i;
                        for(i=2; i<SWidthYpix; i += 2)
                            if (*(Next_p -= Width)) break;
                        if ( i < SWidthYpix )    SWidthYpix = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X;
                        S32 i;
                        for(i=SWidthYpix; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+1) = Color;
                            Pixel_p -= Width2X;
                        }
                    }
                }
            }
            else  if (IsInsideD)
            {
                IsInsideD =  FALSE;
                /* North+East */
                if (!*(UpperPix_p-1) && !*UpperPix_p)
                {
                    S32 SWidthX;
                    {
                         S32 Delta = ((Width-x)<<1)-SWidth;
                         SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X;
                        U8* NextH_p = UpperPix_p;
                        S32 i;
                        for(i=0; i<SWidthYrow; ++i)
                        {
                            S32 j;
                            U8* PixelTmp_p = Pixel_p;
                            U8* NextW_p = NextH_p;
                            if (!*(NextW_p-1))
                                for(j=0; j<SWidthX; ++j)
                                {
                                    if (*NextW_p) break;
                                    *PixelTmp_p++ = Color;
                                    if (j & 1) ++NextW_p;
                                }
                            Pixel_p -= Width2X;
                            if (i & 1) NextH_p -= Width;
                        }
                    }
                }
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowNE */



void sttxt_EffectShadowNW(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32  x, y;
    BOOL IsInsideU, IsInsideD;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<Height; ++y)
    {
        S32 SWidthYrow;
        {
            S32 Delta = ((y+1)<<1)-SWidth;
            SWidthYrow = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        IsInsideU = IsInsideD = FALSE;
        for(x=0; x<Width; ++x)
        {
            if (*UpperPix_p)
            {
                /* West */
                if (!IsInsideU && x)
                {
                    S32 SWidthX;
                    {
                         S32 Delta = (x<<1)-SWidth;
                         SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                    }
                    {
                        U8* Next_p = UpperPix_p-1;
                        S32 i;
                        for(i=2; i<SWidthX; i += 2)
                            if (*(--Next_p)) break;
                        if ( i < SWidthX )    SWidthX = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)-1;
                        S32 i;
                        for(i=SWidthX; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+Width2X) = Color;
                            --Pixel_p;
                        }
                    }
                }
                IsInsideU = TRUE;
            }
            else    IsInsideU = FALSE;
            if (*LowerPix_p && (y<(Height-1)))
            {
                if (!*UpperPix_p)
                {
                    /* North */
                    S32 SWidthYpix = SWidthYrow;
                    {
                        U8* Next_p = UpperPix_p;
                        S32 i;
                        for(i=2; i<SWidthYpix; i += 2)
                                if (*(Next_p -= Width)) break;
                        if ( i < SWidthYpix )    SWidthYpix = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X;
                        S32 i;
                        for(i=SWidthYpix; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+1) = Color;
                            Pixel_p -= Width2X;
                        }
                    }
                    /* North+West */
                    if (!IsInsideD && x)
                    {
                        S32 SWidthX;
                        {
                             S32 Delta = (x<<1)-SWidth;
                             SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                        }
                        {
                            U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X-1;
                            U8* NextH_p = UpperPix_p-1;
                            S32 i;
                            for(i=0; i<SWidthYrow; ++i)
                            {
                                S32 j;
                                U8* PixelTmp_p = Pixel_p;
                                U8* NextW_p = NextH_p;
                                if (!*(NextW_p+1))
                                    for(j=0; j<SWidthX; ++j)
                                    {
                                        if (*NextW_p) break;
                                        *PixelTmp_p-- = Color;
                                        if (j & 1) --NextW_p;
                                    }
                                Pixel_p -= Width2X;
                                if (i & 1) NextH_p -= Width;
                            }
                        }
                    }
                }
                IsInsideD = TRUE;
            }
            else    IsInsideD = FALSE;
            ++UpperPix_p; ++LowerPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowNW */




void sttxt_EffectShadowSE(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32  x, y;
    BOOL IsInside;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<Height; ++y)
    {
        S32 SWidthYrow;
        {
            S32 Delta = ((Height-y)<<1)-SWidth;
            SWidthYrow = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        IsInside = FALSE;
        for(x=0; x<Width; ++x)
        {
            if (*UpperPix_p)
            {
                IsInside = TRUE;
                /* South */
                if (!*LowerPix_p)
                {
                    S32 SWidthYpix = SWidthYrow;
                    {
                        U8* Next_p = LowerPix_p;
                        S32 i;
                        for(i=2; i<SWidthYpix; i += 2)
                            if (*(Next_p += Width)) break;
                        if ( i < SWidthYpix )    SWidthYpix = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X;
                        S32 i;
                        for(i=SWidthYpix; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+1) = Color;
                            Pixel_p += Width2X;
                        }
                    }
                }
            }
            else if (IsInside)
            {
                S32 SWidthX;
                IsInside =  FALSE;
                {
                    S32 Delta = ((Width-x)<<1)-SWidth;
                    SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                }
                /* East */
                {
                    {
                        U8* Next_p = UpperPix_p;
                        S32 i;
                        for(i=2; i<SWidthX; i += 2)
                            if (*(++Next_p)) break;
                        if ( i < SWidthX )    SWidthX = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1);
                        S32 i;
                        for(i=SWidthX; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+Width2X) = Color;
                            ++Pixel_p;
                        }
                    }
                }
                /* South+East */
                if (/*!*(LowerPix_p-1) &&*/ !*LowerPix_p)
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X;
                    U8* NextH_p = LowerPix_p;
                    S32 i;
                    for(i=0; i<SWidthYrow; ++i)
                    {
                        S32 j;
                        U8* PixelTmp_p = Pixel_p;
                        U8* NextW_p = NextH_p;
                        if (!*(NextW_p-1))
                            for(j=0; j<SWidthX; ++j)
                            {
                                if (*NextW_p) break;
                                *PixelTmp_p++ = Color;
                                if (j & 1) ++NextW_p;
                            }
                        Pixel_p += Width2X;
                        if (i & 1) NextH_p += Width;
                    }
                }
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowSE */



void sttxt_EffectShadowSW(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32  x, y;
    BOOL IsInsideU, IsInsideD;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<Height; ++y)
    {
        S32 SWidthYrow;
        {
            S32 Delta = ((Height-y)<<1)-SWidth;
            SWidthYrow = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        IsInsideU = IsInsideD = FALSE;
        for(x=0; x<Width; ++x)
        {
            if (*UpperPix_p)
            {
                /* West */
                if (!IsInsideU && x)
                {
                    S32 SWidthX;
                    {
                         S32 Delta = (x<<1)-SWidth;
                         SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                    }
                    {
                        U8* Next_p = UpperPix_p-1;
                        S32 i;
                        for(i=2; i<SWidthX; i += 2)
                            if (*(--Next_p)) break;
                        if ( i < SWidthX )    SWidthX = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)-1;
                        S32 i;
                        for(i=SWidthX; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+Width2X) = Color;
                            --Pixel_p;
                        }
                    }
                    /* South-West */
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X-1;
                        U8* NextH_p = LowerPix_p-1;
                        S32 i;
                        for(i=0; i<SWidthYrow; ++i)
                        {
                            S32 j;
                            U8* PixelTmp_p = Pixel_p;
                            U8* NextW_p = NextH_p;
                            if (!*(NextW_p+1))
                                for(j=0; j<SWidthX; ++j)
                                {
                                    if (*NextW_p) break;
                                    *PixelTmp_p-- = Color;
                                    if (j & 1) --NextW_p;
                                }
                            Pixel_p += Width2X;
                            if (i & 1) NextH_p += Width;
                        }
                    }
                }
                /* South */
                if (!*LowerPix_p)
                {
                    S32 SWidthYpix = SWidthYrow;
                    {
                        U8* Next_p = LowerPix_p;
                        S32 i;
                        for(i=2; i<SWidthYpix; i += 2)
                            if (*(Next_p += Width)) break;
                        if ( i < SWidthYpix )    SWidthYpix = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X;
                        S32 i;
                        for(i=SWidthYpix; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+1) = Color;
                            Pixel_p += Width2X;
                        }
                    }
                }
                IsInsideU = TRUE;
            }
            else    IsInsideU = FALSE;
            ++UpperPix_p; ++LowerPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowSW */



void sttxt_EffectShadowNS(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32 x, y;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<Height; ++y)
    {
        S32 SWidthYnorth, SWidthYsouth;
        {
            S32 Delta = ((y+1)<<1)-SWidth;
            SWidthYnorth = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        {
            S32 Delta = ((Height-y)<<1)-SWidth;
            SWidthYsouth = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        for(x=0; x<Width; ++x)
        {
            /* North */
            if (*LowerPix_p && !*UpperPix_p  && (y<(Height-1)))
            {
                S32 SWidthYpix = SWidthYnorth;
                {
                    U8* Next_p = UpperPix_p;
                    S32 i;
                    for(i=2; i<SWidthYpix; i += 2)
                        if (*(Next_p -= Width)) break;
                    if ( i < SWidthYpix )    SWidthYpix = i;
                }
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X;
                    S32 i;
                    for(i=SWidthYpix; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+1) = Color;
                        Pixel_p -= Width2X;
                    }
                }
            }
            /* South */
            if (*UpperPix_p && !*LowerPix_p)
            {
                S32 SWidthYpix = SWidthYsouth;
                {
                    U8* Next_p = LowerPix_p;
                    S32 i;
                    for(i=2; i<SWidthYpix; i += 2)
                        if (*(Next_p += Width)) break;
                    if ( i < SWidthYpix )    SWidthYpix = i;
                }
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X;
                    S32 i;
                    for(i=SWidthYpix; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+1) = Color;
                        Pixel_p += Width2X;
                    }
                }
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowNS */



void sttxt_EffectShadowEW(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32  x, y;
    BOOL IsInside;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;

    for(y=0; y<Height; ++y)
    {
        IsInside = FALSE;
        for(x=0; x<Width; ++x)
        {
            if (*UpperPix_p)
            {
                /* West */
                if (!IsInside)
                {
                    S32 SWidthX;
                    {
                         S32 Delta = (x<<1)-SWidth;
                         SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                    }
                    {
                        U8* Next_p = UpperPix_p-1;
                        S32 i;
                        for(i=2; i<SWidthX; i += 2)
                            if (*(--Next_p)) break;
                        if ( i < SWidthX )    SWidthX = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)-1;
                        S32 i;
                        for(i=SWidthX; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+Width2X) = Color;
                            --Pixel_p;
                        }
                    }
                }
                IsInside = TRUE;
            }
            else if (IsInside)
            {
                /* East */
                S32 SWidthX;
                {
                    S32 Delta = ((Width-x)<<1)-SWidth;
                    SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                }
                {
                    U8* Next_p = UpperPix_p;
                    S32 i;
                    for(i=2; i<SWidthX; i += 2)
                        if (*(++Next_p)) break;
                    if ( i < SWidthX )    SWidthX = i;
                }
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1);
                    S32 i;
                    for(i=SWidthX; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+Width2X) = Color;
                        ++Pixel_p;
                    }
                }
                IsInside =  FALSE;
            }
            ++UpperPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowEW */



void sttxt_EffectShadowNSEW(STGXOBJ_Bitmap_t* SrcDblBitmap_p,
                    STGFX_TxtFontAttributes_t* FontAttr_p)
{
    S32 x, y;
    BOOL IsInside;
    S32 Width   = SrcDblBitmap_p->Width  >> 1;
    S32 Height  = SrcDblBitmap_p->Height >> 1;
    U8  Color   = FontAttr_p->ShadowColor;
    U8  SWidth  = FontAttr_p->ShadowWidth;
    U32 Width2X = (Width<<1);
    U32 Width4X   = (Width2X<<1);
    U8* GlyphPix_p  = (U8*)SrcDblBitmap_p->Data1_p;
    register U8* UpperPix_p = (U8*)SrcDblBitmap_p->Data2_p;
    register U8* LowerPix_p = UpperPix_p + Width;

    for(y=0; y<Height; ++y)
    {
        S32 SWidthYnorth, SWidthYsouth;
        {
            S32 Delta = ((y+1)<<1)-SWidth;
            SWidthYnorth = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        {
            S32 Delta = ((Height-y)<<1)-SWidth;
            SWidthYsouth = (Delta < 0) ? SWidth + Delta : SWidth;
        }
        IsInside = FALSE;
        for(x=0; x<Width; ++x)
        {
            if (*UpperPix_p)
            {
                /* West */
                if (!IsInside)
                {
                    S32 SWidthX;
                    {
                         S32 Delta = (x<<1)-SWidth;
                         SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                    }
                    {
                        U8* Next_p = UpperPix_p-1;
                        S32 i;
                        for(i=2; i<SWidthX; i += 2)
                            if (*(--Next_p)) break;
                        if ( i < SWidthX )    SWidthX = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)-1;
                        S32 i;
                        for(i=SWidthX; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+Width2X) = Color;
                            --Pixel_p;
                        }
                    }
                }
                /* South */
                if (!*LowerPix_p)
                {
                    S32 SWidthYpix = SWidthYsouth;
                    {
                        U8* Next_p = LowerPix_p;
                        S32 i;
                        for(i=2; i<SWidthYpix; i += 2)
                            if (*(Next_p += Width)) break;
                        if ( i < SWidthYpix )    SWidthYpix = i;
                    }
                    {
                        U8* Pixel_p = GlyphPix_p+(x<<1)+Width4X;
                        S32 i;
                        for(i=SWidthYpix; i>0; --i)
                        {
                            *Pixel_p = *(Pixel_p+1) = Color;
                            Pixel_p += Width2X;
                        }
                    }
                }
                IsInside = TRUE;
            }
            else if (IsInside)
            {
                /* East */
                S32 SWidthX;
                {
                    S32 Delta = ((Width-x)<<1)-SWidth;
                    SWidthX = (Delta < 0) ? SWidth + Delta : SWidth;
                }
                {
                    U8* Next_p = UpperPix_p;
                    S32 i;
                    for(i=2; i<SWidthX; i += 2)
                        if (*(++Next_p)) break;
                    if ( i < SWidthX )    SWidthX = i;
                }
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1);
                    S32 i;
                    for(i=SWidthX; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+Width2X) = Color;
                        ++Pixel_p;
                    }
                }
                IsInside =  FALSE;
            }
            /* North */
            if (*LowerPix_p && !*UpperPix_p && (y<(Height-1)))
            {
                S32 SWidthYpix = SWidthYnorth;
                {
                    U8* Next_p = UpperPix_p;
                    S32 i;
                    for(i=2; i<SWidthYpix; i += 2)
                        if (*(Next_p -= Width)) break;
                    if ( i < SWidthYpix )    SWidthYpix = i;
                }
                {
                    U8* Pixel_p = GlyphPix_p+(x<<1)+Width2X;
                    S32 i;
                    for(i=SWidthYpix; i>0; --i)
                    {
                        *Pixel_p = *(Pixel_p+1) = Color;
                        Pixel_p -= Width2X;
                    }
                }
            }
            ++UpperPix_p; ++LowerPix_p;
        }
        GlyphPix_p += Width4X;
    }
}  /* End of function sttxt_EffectShadowNSEW */

