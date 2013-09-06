/*
    File:           sttxt.c
    Description:    Teletext fonts drawtext routine
*/

/*#include <debug.h>*/

#if 0 /* debug */
#define STTBX_PRINT
#include <stdio.h>
#endif

#include <string.h>
#include "sttbx.h"
#include "gfx_tools.h" /*stgfx_init.h*/
#include "sttxt.h"

static void ConvertGlyph1to8Zoom1x1(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph2to8Zoom1x1(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph4to8Zoom1x1(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph8to8Zoom1x1(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph1to8Zoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph2to8Zoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph4to8Zoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph8to8Zoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph1to8ZoomXxY(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph2to8ZoomXxY(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph4to8ZoomXxY(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
static void ConvertGlyph8to8ZoomXxY(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);

static void ProcessGlyph1to8AndZoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                        const STGFX_GC_t*);
static void ProcessGlyph2to8AndZoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                        const STGFX_GC_t*);
static void ProcessGlyph4to8AndZoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                        const STGFX_GC_t*);
static void ProcessGlyph8to8AndZoom2x2(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                        const STGFX_GC_t*);

static void ZoomXxYGlyph8to8(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*);


/*static void DumpBMP(STGXOBJ_Bitmap_t* Bitmap_p);*/


/******************************************************************************
Function Name : DrawTxtText
  Description : Draws teletext
   Parameters :
  Assumptions : Parameter checking done at upper level (Bitmap_p, GC_p, Handle,
                String, GC_p->Font_p->FontData.TxtFontData_p,
                GC_p->TxtFontAttributes_p
******************************************************************************/
ST_ErrorCode_t DrawTxtText(STBLIT_BlitContext_t* BlitContext_p,
                           STGXOBJ_Bitmap_t* DstBitmap_p,
                           const STGFX_GC_t* GC_p,
                           STGFX_Handle_t Handle,
                           const ST_String_t String,
                           const U16* WString,
                           S32 DstPosX,
                           S32 DstPosY,
                           U32 NumOfChars)
{
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    STGFX_TxtFont_t             Font = *GC_p->Font_p->FontData.TxtFontData_p;
    STGFX_TxtFontAttributes_t   FontAttr = *GC_p->TxtFontAttributes_p; /* why copy? */
    U8*  EffectMatrix_p = NULL;
    U32  GlyphSize      = ((Font.FontWidth*Font.NbColor+7)>>3)*Font.FontHeight;
    U32  Increment      = Font.FontWidth*FontAttr.ZoomWidth;
    S32  PosX           = DstPosX;
    U16  Ch;
    U32  i;
    STBLIT_BlitContext_t BlitContext = *BlitContext_p;
    STGXOBJ_Rectangle_t  BCClipRect = BlitContext.ClipRectangle;
    STGXOBJ_Bitmap_t     SourceGlyph;
    STGXOBJ_Bitmap_t     DestGlyph = {STGXOBJ_COLOR_TYPE_CLUT8,
                                      STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE,
                                      FALSE,
                                      STGXOBJ_ITU_R_BT601,
                                      STGXOBJ_ASPECT_RATIO_SQUARE,
                                      0,
                                      0,
                                      0,
                                      0,
                                      NULL,
                                      0,
                                      NULL,
                                      0,
                                      STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB,
                                      FALSE};
    void*                   Memory_p;
    U8                      TPalette[256]; /* replacement Color_p for !UseTextBackground */
    STBLIT_Source_t         BlitSrc;
    STBLIT_Handle_t         BlitHandle;
    STBLIT_Destination_t    BlitDst;
    U32                     BlockSize, QuadGlyphSize;
    STAVMEM_BlockHandle_t   AVMEM_BlockHandle;
    
    void (*ConvertGlyphNto8AndZoom)(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                    STGFX_TxtFontAttributes_t*);
    void (*ProcessGlyphNto8AndZoom2x2)(STGXOBJ_Bitmap_t*, STGXOBJ_Bitmap_t*,
                                       const STGFX_GC_t*);


    BlitHandle = stgfx_GetBlitterHandle(Handle);
    BlitContext.NotifyBlitCompletion  = TRUE;
    BlitContext.EventSubscriberID = stgfx_GetEventSubscriberID(Handle);

    /* Check zoom values */
    switch (FontAttr.ZoomWidth)
    {
        case 1: case 2: case 4: break;
        default: return (STGFX_ERROR_INVALID_GC);
    }
    switch (FontAttr.ZoomHeight)
    {
        case 1: case 2: case 4: case 8: break;
        default: return (STGFX_ERROR_INVALID_GC);
    }
    
    switch (Font.NbColor)
    {
        case 1: SourceGlyph.ColorType = STGXOBJ_COLOR_TYPE_CLUT1; break;
        case 2: SourceGlyph.ColorType = STGXOBJ_COLOR_TYPE_CLUT2; break;
        case 4: SourceGlyph.ColorType = STGXOBJ_COLOR_TYPE_CLUT4; break;
        case 8: SourceGlyph.ColorType = STGXOBJ_COLOR_TYPE_CLUT8; break;
        default: return (STGFX_ERROR_INVALID_GC);
    }
    SourceGlyph.Width  = Font.FontWidth;
    SourceGlyph.Height = Font.FontHeight;
    SourceGlyph.Pitch  = ((Font.FontWidth*Font.NbColor+7)>>3);
    DestGlyph.Width    = DestGlyph.Pitch = Font.FontWidth * FontAttr.ZoomWidth;
    DestGlyph.Height   = Font.FontHeight * FontAttr.ZoomHeight;
    DestGlyph.Data1_p  = NULL;

    /* calculate bytes needed for final output to STBLIT (always CLUT8) */
    BlockSize = Font.FontWidth * Font.FontHeight
                 * FontAttr.ZoomWidth * FontAttr.ZoomHeight;
                 
    /* Adjust clipping parameters */
    if (BCClipRect.PositionY < 0)
    {
        BCClipRect.Height += BCClipRect.PositionY;
        BCClipRect.PositionY = 0;
    }
    {
        S32 Distance = (S32)DstBitmap_p->Height - BCClipRect.PositionY;
        if (Distance > 0)
        {
            if (BCClipRect.Height > Distance)    BCClipRect.Height  = Distance;
        }
        else return( ST_NO_ERROR );
    }
    if (( DstPosY < (S32)(BCClipRect.PositionY-DestGlyph.Height) )||
            ( DstPosY > (S32)(BCClipRect.PositionY+BCClipRect.Height) ))
        return( ST_NO_ERROR );

    if (BCClipRect.PositionX < 0)
    {
        BCClipRect.Width += BCClipRect.PositionX;
        BCClipRect.PositionX = 0;
    }
    {
        S32 Distance = (S32)DstBitmap_p->Width - BCClipRect.PositionX;
        if (Distance > 0)
        {
            if (BCClipRect.Width > Distance)    BCClipRect.Width  = Distance;
        }
        else return( ST_NO_ERROR );
    }
    BlitContext.ClipRectangle = BCClipRect;

    /* Prepare Blitter structures */
    BlitSrc.Type            = STBLIT_SOURCE_TYPE_BITMAP;
    BlitSrc.Data.Bitmap_p   = &DestGlyph;
    BlitDst.Bitmap_p        = DstBitmap_p;
    BlitSrc.Palette_p       = GC_p->Palette_p; /* let BLIT decide if NULL ok */
    BlitDst.Palette_p       = NULL;
    BlitSrc.Rectangle.PositionX = BlitSrc.Rectangle.PositionY = 0;
    BlitSrc.Rectangle.Width  = BlitDst.Rectangle.Width  = DestGlyph.Width;
    BlitSrc.Rectangle.Height = BlitDst.Rectangle.Height = DestGlyph.Height;
    BlitDst.Rectangle.PositionY = DstPosY;

    /* Prepare for Background transparence */
    if ( !GC_p->UseTextBackground )
    {
        U32 j;
        static const U16 Power[] = {1,2,4,8,16,32,64,128,256};
        U32 NumOfColors = Power[Font.NbColor];
        U8  ColorKey;

        STTBX_Print(("DrawTxtText: setting up background transparence colour key\n"));
        /* find the first background colour */
        for (j=0; j<NumOfColors; ++j)
            if (!Font.IsForeground_p[j])
                break;
        
        if (j < NumOfColors)
        {
            /* at least one background colour */
            
            BlitContext.ColorKey.Type = STGXOBJ_COLOR_KEY_TYPE_CLUT8;
            BlitContext.ColorKeyCopyMode = STBLIT_COLOR_KEY_MODE_SRC;

            ColorKey = FontAttr.Color_p[j];
            BlitContext.ColorKey.Value.CLUT8.PaletteEntryMin =
            BlitContext.ColorKey.Value.CLUT8.PaletteEntryMax = ColorKey;
            BlitContext.ColorKey.Value.CLUT8.PaletteEntryOut = FALSE;
            BlitContext.ColorKey.Value.CLUT8.PaletteEntryEnable = TRUE;

            /* set the remaining background colours to the same value */
            for (j=0; j<NumOfColors; ++j)
            {
                TPalette[j] = Font.IsForeground_p[j] ?
                                FontAttr.Color_p[j] : ColorKey;
            }
            FontAttr.Color_p = (U8*)&TPalette;
        }
    }

    /* No Zoom & Zoom / No effect */
    if (!FontAttr.OverlineHeight && !FontAttr.UnderlineHeight
        && FontAttr.Effect == STGFX_NO_EFFECT
        || FontAttr.ZoomWidth==1 && FontAttr.ZoomHeight==1)
    {
        S32 LeftArea  = BCClipRect.PositionX - DestGlyph.Width;
        S32 RightArea = BCClipRect.PositionX + BCClipRect.Width;
        
        /* Set the optimized N bit to 8 bit conversion function */
        if (FontAttr.ZoomWidth==1 && FontAttr.ZoomHeight==1)
        {
            switch (Font.NbColor)
            {
                case 1:
                    ConvertGlyphNto8AndZoom = ConvertGlyph1to8Zoom1x1;
                    break;
                case 2:
                    ConvertGlyphNto8AndZoom = ConvertGlyph2to8Zoom1x1;
                    break;
                case 4:
                    ConvertGlyphNto8AndZoom = ConvertGlyph4to8Zoom1x1;
                    break;
                case 8:
                    ConvertGlyphNto8AndZoom = ConvertGlyph8to8Zoom1x1;
                    break;
                default:
                    return (STGFX_ERROR_INVALID_GC);
            }
        }
        else if (FontAttr.ZoomWidth==2 && FontAttr.ZoomHeight==2)
        {
            switch (Font.NbColor)
            {
                case 1:
                    ConvertGlyphNto8AndZoom = ConvertGlyph1to8Zoom2x2;
                    break;
                case 2:
                    ConvertGlyphNto8AndZoom = ConvertGlyph2to8Zoom2x2;
                    break;
                case 4:
                    ConvertGlyphNto8AndZoom = ConvertGlyph4to8Zoom2x2;
                    break;
                case 8:
                    ConvertGlyphNto8AndZoom = ConvertGlyph8to8Zoom2x2;
                    break;
                default:
                    return (STGFX_ERROR_INVALID_GC);
            }
        }
        else
        {
            switch (Font.NbColor)
            {
                case 1:
                    ConvertGlyphNto8AndZoom = ConvertGlyph1to8ZoomXxY;
                    break;
                case 2:
                    ConvertGlyphNto8AndZoom = ConvertGlyph2to8ZoomXxY;
                    break;
                case 4:
                    ConvertGlyphNto8AndZoom = ConvertGlyph4to8ZoomXxY;
                    break;
                case 8:
                    ConvertGlyphNto8AndZoom = ConvertGlyph8to8ZoomXxY;
                    break;
                default:
                    return (STGFX_ERROR_INVALID_GC);
            }
        }

#ifdef STGFX_WRITE_WORDS_TO_AVMEM
        /* We must do our bytewise operations within local memory */
        Memory_p = stgfx_CPUMEM_malloc(Handle, BlockSize);
        if (Memory_p == NULL)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        
        if (BlockSize % 4)
        {
          /* pad size for AVMEM allocations to U32 word
            as that's how we do the copying */
          BlockSize += (4 - BlockSize % 4);
        }
#endif

        /* Loop for characters */
        for(i=0; (i<NumOfChars); i++, PosX += Increment)
        {
            if(String != NULL)
            {
                /* narrow-character string */
                if(!String[i]) break;
                Ch = (U16)(U8) String[i];
            }
            else
            {
                /* wide-character string */
                if(!WString[i]) break;
                Ch = WString[i];
            }

            if (GC_p->Font_p->FontType & STGFX_USE_CHAR_MAPPING)
            {
                if(ST_NO_ERROR !=
                      stgfx_CharCodeToIdx(&GC_p->Font_p->CharMapping, Ch, &Ch))
                {
                    break;
                }
            }

            if(Ch >= Font.NumOfChars) break;

            if ( PosX < LeftArea )
            {
                /* char completely left of clipping rect - go to next */
                continue;
            }
            if ( PosX > RightArea )
            {
                /* all remaining chars right of clipping rect */
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
                stgfx_CPUMEM_free(Handle, Memory_p);
#endif
                return( ST_NO_ERROR );
            }
            
#ifndef STGFX_WRITE_WORDS_TO_AVMEM
            /* we go direct to AVMEM */
            Error = stgfx_AVMEM_malloc(Handle, BlockSize,
                                       &AVMEM_BlockHandle, &Memory_p);
            if(Error != ST_NO_ERROR)
            {
                return(Error);
            }
#endif
            
            DestGlyph.Data1_p  = Memory_p;
            
            SourceGlyph.Data1_p = Font.GlyphData_p + Ch*GlyphSize;
            ConvertGlyphNto8AndZoom(&SourceGlyph, &DestGlyph, &FontAttr);
            
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
            /* Now copy wordwise to SDRAM, where the blitter can read it
              (ASSUMES both allocators give word aligned memory) */
              
            Error = stgfx_AVMEM_malloc(Handle, BlockSize,
                                       &AVMEM_BlockHandle, &DestGlyph.Data1_p);
            if(Error != ST_NO_ERROR)
            {
                stgfx_CPUMEM_free(Handle, Memory_p);
                return(Error);
            }

            {
                U32* Src_p = (U32*)Memory_p;
                U32* Dst_p = (U32*)DestGlyph.Data1_p;
                U32* SrcLimit_p = (U32*)((U32)Src_p + BlockSize);
                
                while (Src_p < SrcLimit_p)
                {
                    *Dst_p++ = *Src_p++;
                }
            }
#endif

            BlitDst.Rectangle.PositionX = PosX;
            BlitContext.UserTag_p = (void*)AVMEM_BlockHandle;
            Error = STBLIT_Blit(BlitHandle, &BlitSrc, NULL,
                                &BlitDst, &BlitContext);
            if (Error != ST_NO_ERROR)
            {
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
                stgfx_CPUMEM_free(Handle, Memory_p);
#endif
                stgfx_AVMEM_free(Handle, &AVMEM_BlockHandle);
                return(STGFX_ERROR_STBLIT_BLIT);
            }
        }
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
        stgfx_CPUMEM_free(Handle, Memory_p);
#endif
        return( Error );
    }
    /* Zoom + Effect */
    else
    {
        S32 LeftArea  = BCClipRect.PositionX - DestGlyph.Width;
        S32 RightArea = BCClipRect.PositionX + BCClipRect.Width;
        STGXOBJ_Bitmap_t   Glyph2x2 = {STGXOBJ_COLOR_TYPE_CLUT8,
                                       STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE,
                                       FALSE,
                                       STGXOBJ_ITU_R_BT601,
                                       STGXOBJ_ASPECT_RATIO_SQUARE,
                                       0,
                                       0,
                                       0,
                                       0,
                                       NULL,
                                       0,
                                       NULL,
                                       0,
                                       STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB,
                                       FALSE};
        Glyph2x2.Width   = Glyph2x2.Pitch = Font.FontWidth << 1;
        Glyph2x2.Height  = Font.FontHeight << 1;

        /* Zoom to 2x2, apply effects, then zoom more if required */
        
        if (FontAttr.Effect != STGFX_NO_EFFECT)
        {
            switch (Font.NbColor)
            {
                case 1:
                    ProcessGlyphNto8AndZoom2x2 = ProcessGlyph1to8AndZoom2x2;
                    break;
                case 2:
                    ProcessGlyphNto8AndZoom2x2 = ProcessGlyph2to8AndZoom2x2;
                    break;
                case 4:
                    ProcessGlyphNto8AndZoom2x2 = ProcessGlyph4to8AndZoom2x2;
                    break;
                case 8:
                    ProcessGlyphNto8AndZoom2x2 = ProcessGlyph8to8AndZoom2x2;
                    break;
                default:
                    return (STGFX_ERROR_INVALID_GC);
            }
        }
        else
        {
            switch (Font.NbColor)
            {
                case 1: ConvertGlyphNto8AndZoom = ConvertGlyph1to8Zoom2x2; break;
                case 2: ConvertGlyphNto8AndZoom = ConvertGlyph2to8Zoom2x2; break;
                case 4: ConvertGlyphNto8AndZoom = ConvertGlyph4to8Zoom2x2; break;
                case 8: ConvertGlyphNto8AndZoom = ConvertGlyph8to8Zoom2x2; break;
                default: return (STGFX_ERROR_INVALID_GC);
            }
        }

        QuadGlyphSize = 4 * BlockSize;

#ifdef STGFX_WRITE_WORDS_TO_AVMEM
        /* We must do our bytewise operations within local memory */
        Memory_p = stgfx_CPUMEM_malloc(Handle, 2*QuadGlyphSize);
        if (Memory_p == NULL)
        {
            return(ST_ERROR_NO_MEMORY);
        }
        
        if (BlockSize % 4)
        {
          /* pad size for AVMEM allocations to U32 word
            as that's how we do the copying */
          BlockSize += (4 - BlockSize % 4);
        }
#endif

        /* Loop for characters */
        for(i=0; (i<NumOfChars); i++, PosX += Increment)
        {
            if(String != NULL)
            {
                /* narrow-character string */
                if(!String[i]) break;
                Ch = (U16)(U8) String[i];
            }
            else
            {
                /* wide-character string */
                if(!WString[i]) break;
                Ch = WString[i];
            }

            if (GC_p->Font_p->FontType & STGFX_USE_CHAR_MAPPING)
            {
                if(ST_NO_ERROR !=
                      stgfx_CharCodeToIdx(&GC_p->Font_p->CharMapping, Ch, &Ch))
                {
                    break;
                }
            }

            if(Ch >= Font.NumOfChars) break;

            if ( PosX < LeftArea )
            {
                /* char completely left of clipping rect - go to next */
                continue;
            }
            if ( PosX > RightArea )
            {
                /* all remaining chars right of clipping rect */
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
                stgfx_CPUMEM_free(Handle, Memory_p);
#endif
                return( ST_NO_ERROR );
            }
        
#ifndef STGFX_WRITE_WORDS_TO_AVMEM
            /* we go direct to AVMEM */
            Error = stgfx_AVMEM_malloc(Handle, 2*QuadGlyphSize,
                                       &AVMEM_BlockHandle, &Memory_p);
            if(Error != ST_NO_ERROR)
            {
                return(Error);
            }
#endif

            EffectMatrix_p = (U8*)Memory_p + QuadGlyphSize;
            DestGlyph.Data1_p  = Memory_p;
            Glyph2x2.Data1_p = Memory_p;
            Glyph2x2.Data2_p = EffectMatrix_p;
    
            SourceGlyph.Data1_p = Font.GlyphData_p + Ch*GlyphSize;

            /* Some effect require a support matrix */
            if (FontAttr.Effect != STGFX_NO_EFFECT)
            {
                /* Zoom 2x2 */
                ProcessGlyphNto8AndZoom2x2(&SourceGlyph, &Glyph2x2, GC_p);

                /* Effects */
                switch ( FontAttr.Effect )
                {
                    case STGFX_FRINGING:
                    {
                        sttxt_EffectFringing(&Glyph2x2, &FontAttr);
                        break;
                    }
                    case STGFX_ROUNDING:
                    {
                        sttxt_EffectRounding(&Glyph2x2, &FontAttr);
                        break;
                    }
                    case STGFX_ROUNDING_FRINGING:
                    {
                        sttxt_EffectFringing(&Glyph2x2, &FontAttr);
                        sttxt_EffectRounding(&Glyph2x2, &FontAttr);
                        break;
                    }
                    case STGFX_SHADOW:
                    {
                        switch(FontAttr.ShadowDirection)
                        {
                            case STGFX_NORTH:
                                sttxt_EffectShadowN(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_SOUTH:
                                sttxt_EffectShadowS(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_EAST:
                                sttxt_EffectShadowE(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_WEST:
                                sttxt_EffectShadowW(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_NORTH_EAST:
                                sttxt_EffectShadowNE(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_SOUTH_EAST:
                                sttxt_EffectShadowSE(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_SOUTH_WEST:
                                sttxt_EffectShadowSW(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_NORTH_WEST:
                                sttxt_EffectShadowNW(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_NORTH_SOUTH:
                                sttxt_EffectShadowNS(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_EAST_WEST:
                                sttxt_EffectShadowEW(&Glyph2x2, &FontAttr);
                                break;
                            case STGFX_NORTH_SOUTH_EAST_WEST:
                                sttxt_EffectShadowNSEW(&Glyph2x2, &FontAttr);
                                break;
                            default:
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
                                stgfx_CPUMEM_free(Handle, Memory_p);
#endif
                                return( STGFX_ERROR_INVALID_GC );
                        } /* switch(FontAttr.ShadowDirection) */
                        break;
                    }
                    default:
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
                        stgfx_CPUMEM_free(Handle, Memory_p);
#endif
                        return( STGFX_ERROR_INVALID_GC );
                } /* switch(FontAttr.Effect) */
            }
            else
            {
                /* Zoom 2x2 */
                ConvertGlyphNto8AndZoom(&SourceGlyph, &Glyph2x2, &FontAttr);
                /*STTBX_Print(("SourceGlyph:\n"));*/
                /*DumpBMP(&SourceGlyph);*/
                /*STTBX_Print(("Glyph2x2:\n"));*/
                /*DumpBMP(&Glyph2x2);*/
            }

            /* Overline Effect */
            if (FontAttr.OverlineHeight)
            {
                U32 Size = FontAttr.OverlineHeight * Glyph2x2.Width;
                memset(Memory_p, FontAttr.OverlineColor, Size);
            }
            /* Underline Effect */
            if (FontAttr.UnderlineHeight)
            {
                U32 Size = FontAttr.UnderlineHeight * Glyph2x2.Width;
                U8* Limit_p = (U8*)Memory_p + Glyph2x2.Width * Glyph2x2.Height;
                U8* PixelD_p = Limit_p - Size;
                memset(PixelD_p, FontAttr.UnderlineColor, Size);
            }
            if (FontAttr.ZoomWidth!=2 || FontAttr.ZoomHeight!=2)
            {
                STTBX_Print(("DrawTxtText: Apply second zoom\n"));
                ZoomXxYGlyph8to8(&Glyph2x2, &DestGlyph);
            }
            
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
            /* Now copy wordwise to SDRAM, where the blitter can read it
              (ASSUMES both allocators give word aligned memory) */
              
            Error = stgfx_AVMEM_malloc(Handle, BlockSize,
                                       &AVMEM_BlockHandle, &DestGlyph.Data1_p);
            if(Error != ST_NO_ERROR)
            {
                stgfx_CPUMEM_free(Handle, Memory_p);
                return(Error);
            }

            {
                U32* Src_p = (U32*)Memory_p;
                U32* Dst_p = (U32*)DestGlyph.Data1_p;
                U32* SrcLimit_p = (U32*)((U32)Src_p + BlockSize);
                
                while (Src_p < SrcLimit_p)
                {
                    *Dst_p++ = *Src_p++;
                }
            }
#endif

            BlitDst.Rectangle.PositionX = PosX;
            BlitContext.UserTag_p = (void*)AVMEM_BlockHandle;
            Error = STBLIT_Blit(BlitHandle, &BlitSrc, NULL,
                                &BlitDst, &BlitContext);
            /*STTBX_Print (("STBLIT_Blit %c done. Press a key...\n"));
            getchar();*/
            /* gamma dependent calls - end - */
            if (Error != ST_NO_ERROR)
            {
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
                stgfx_CPUMEM_free(Handle, Memory_p);
#endif
                stgfx_AVMEM_free(Handle, &AVMEM_BlockHandle);
                return(STGFX_ERROR_STBLIT_BLIT);
            }
        }
#ifdef STGFX_WRITE_WORDS_TO_AVMEM
        stgfx_CPUMEM_free(Handle, Memory_p);
#endif
        return Error;
    }
}   /* End of function DrawTxtText */

void ConvertGlyph1to8Zoom1x1(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8  LastCode    = 0;
    U8  BgColor     = (U8)FontAttr_p->Color_p[0];
    U8  FgColor     = (U8)FontAttr_p->Color_p[1];
    U8  Color       = BgColor;

    STTBX_Print(("ConvertGlyph1to8Zoom1x1\n"));
    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 7;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x01);
            if (Code != LastCode)
                Color = (LastCode = Code) ? FgColor : BgColor;
            *Pixel_p++ = Color;
            if (!Shift) Shift = 7, ++GlyphData_p;
            else        --Shift;
        }
        ++GlyphData_p;
    }
}   /* End of function ConvertGlyph1to8Zoom1x1 */


void ConvertGlyph2to8Zoom1x1(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 6;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x03);
            if (Code != LastCode)
                Color = (U8)Colors_p[LastCode = Code];
            *Pixel_p++ = Color;
            if (!Shift) Shift = 6, ++GlyphData_p;
            else        Shift -= 2;
        }
        ++GlyphData_p;
    }
}   /* End of function ConvertGlyph2to8Zoom1x1 */


void ConvertGlyph4to8Zoom1x1(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 4;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x0F);
            if (Code != LastCode)
                Color = (U8)Colors_p[LastCode = Code];
            *Pixel_p++ = Color;
            if (!Shift) Shift = 4, ++GlyphData_p;
            else        Shift -= 4;
        }
        ++GlyphData_p;
    }
}   /* End of function ConvertGlyph4to8Zoom1x1 */


void ConvertGlyph8to8Zoom1x1(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 i;
    U8* Colors_p    = FontAttr_p->Color_p;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for (i = SrcBitmap_p->Width*SrcBitmap_p->Height; i!=0; --i)
    {
            U8 Code = *GlyphData_p++;
            if (Code != LastCode)
                Color = (U8)Colors_p[LastCode = Code];
            *Pixel_p++ = Color;
    }
}   /* End of function ConvertGlyph8to8Zoom1x1 */


void ConvertGlyph1to8Zoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  BgColor     = (U8)FontAttr_p->Color_p[0];
    U8  FgColor     = (U8)FontAttr_p->Color_p[1];
    U8  Color       = BgColor;

    STTBX_Print(("ConvertGlyph1to8Zoom2x2\n"));
    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 7;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x01);
            if (Code != LastCode)
                Color = (LastCode = Code) ? FgColor : BgColor;
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
            if (!Shift) Shift = 7, ++GlyphData_p;
            else        --Shift;
        }
        ++GlyphData_p; /* JF: even if we ended on a glyph byte boundary? */
        Pixel_p   += WidthZoomed; /* skip over other row */
        NextRow_p += WidthZoomed; /* skip over other row */
    }
}   /* End of function ConvertGlyph1to8Zoom2x2 */


void ConvertGlyph2to8Zoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 6;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x03);
            if (Code != LastCode)
                Color = (U8)Colors_p[LastCode = Code];
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
            if (!Shift) Shift = 6, ++GlyphData_p;
            else        Shift -= 2;
        }
        ++GlyphData_p;
        Pixel_p   += WidthZoomed;
        NextRow_p += WidthZoomed;
    }
}   /* End of function ConvertGlyph2to8Zoom2x2 */


void ConvertGlyph4to8Zoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 4;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x0F);
            if (Code != LastCode)
                Color = (U8)Colors_p[LastCode = Code];
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
            if (!Shift) Shift = 4, ++GlyphData_p;
            else        Shift -= 4;
        }
        ++GlyphData_p;
        Pixel_p   += WidthZoomed;
        NextRow_p += WidthZoomed;
    }
}   /* End of function ConvertGlyph4to8Zoom2x2 */


void ConvertGlyph8to8Zoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x;

        for(x=W; x!=0; --x)
        {
            U8 Code = *GlyphData_p++;
            if (Code != LastCode)
                Color = (U8)Colors_p[LastCode = Code];
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
        }
        Pixel_p   += WidthZoomed;
        NextRow_p += WidthZoomed;
    }
}   /* End of function ConvertGlyph8to8Zoom2x2 */


void ConvertGlyph1to8ZoomXxY(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    S32 WidthZoomed = DstBitmap_p->Width;
    S32 ZoomW       = (WidthZoomed / W );
    S32 ZoomH       = (DstBitmap_p->Height / H ) - 1;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8  LastCode    = 0;
    U8  BgColor     = (U8)FontAttr_p->Color_p[0];
    U8  FgColor     = (U8)FontAttr_p->Color_p[1];
    U8  Color       = BgColor;

    for(y=H; y!=0; --y)
    {
        U32 x, i;
        U8* LastRow_p = Pixel_p;
        U32 Shift = 7;

        for(x=W; x!=0; --x)
        {
       		U32 i;
            U8 Code = (*GlyphData_p>>Shift)&(0x01);
            if (Code != LastCode)
                Color = (LastCode = Code) ? FgColor : BgColor;
            for(i=ZoomW; i!=0; --i) *Pixel_p++ = Color;
            if (!Shift) Shift = 7, ++GlyphData_p;
            else        --Shift;
        }
        ++GlyphData_p;
        for(i=ZoomH; i!=0; --i)
        {
            /*
            U32 x;
            for(x=Pixel_p-LastRow_p; x!=0; --x) *Pixel_p++ = *LastRow_p++;
            */
            memcpy(Pixel_p, LastRow_p, WidthZoomed);
            Pixel_p += WidthZoomed;
        }
    }
}   /* End of function ConvertGlyph1to8ZoomXxY */


void ConvertGlyph2to8ZoomXxY(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    S32 WidthZoomed = DstBitmap_p->Width;
    S32 ZoomW       = (WidthZoomed / W );
    S32 ZoomH       = (DstBitmap_p->Height / H ) - 1;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x, i;
        U8* LastRow_p = Pixel_p;
        U32 Shift = 6;

        for(x=W; x!=0; --x)
        {
       		U32 i;
            U8 Code = (*GlyphData_p>>Shift)&(0x03);
            if (Code != LastCode)
                Color = (U8)Colors_p[ LastCode = Code ];
            for(i=ZoomW; i!=0; --i) *Pixel_p++ = Color;
            if (!Shift) Shift = 6, ++GlyphData_p;
            else        Shift -= 2;
        }
        ++GlyphData_p;
        for(i=ZoomH; i!=0; --i)
        {
            /*
            U32 x;
            for(x=Pixel_p-LastRow_p; x!=0; --x) *Pixel_p++ = *LastRow_p++;
            */
            memcpy(Pixel_p, LastRow_p, WidthZoomed);
            Pixel_p += WidthZoomed;
        }
    }
}   /* End of function ConvertGlyph2to8ZoomXxY */


void ConvertGlyph4to8ZoomXxY(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    S32 WidthZoomed = DstBitmap_p->Width;
    S32 ZoomW       = (WidthZoomed / W );
    S32 ZoomH       = (DstBitmap_p->Height / H ) - 1;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x, i;
        U8* LastRow_p = Pixel_p;
        U32 Shift = 4;

        for(x=W; x!=0; --x)
        {
       		U32 i;
            U8 Code = (*GlyphData_p>>Shift)&(0x0f);
            if (Code != LastCode)
                Color = (U8)Colors_p[ LastCode = Code ];
            for(i=ZoomW; i!=0; --i) *Pixel_p++ = Color;
            if (!Shift) Shift = 4, ++GlyphData_p;
            else        Shift -= 4;
        }
        ++GlyphData_p;
        for(i=ZoomH; i!=0; --i)
        {
            /*
            U32 x;
            for(x=Pixel_p-LastRow_p; x!=0; --x) *Pixel_p++ = *LastRow_p++;
            */
            memcpy(Pixel_p, LastRow_p, WidthZoomed);
            Pixel_p += WidthZoomed;
        }
    }
}   /* End of function ConvertGlyph4to8ZoomXxY */


void ConvertGlyph8to8ZoomXxY(STGXOBJ_Bitmap_t* SrcBitmap_p,
                             STGXOBJ_Bitmap_t* DstBitmap_p,
                             STGFX_TxtFontAttributes_t* FontAttr_p)
{
    U32 y;
    U8* Colors_p    = FontAttr_p->Color_p;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    S32 WidthZoomed = DstBitmap_p->Width;
    S32 ZoomW       = (WidthZoomed / W );
    S32 ZoomH       = (DstBitmap_p->Height / H ) - 1;
    U8* LastPixelS_p= (U8*)SrcBitmap_p->Data1_p + W * H;
    U8* LastPixelD_p= (U8*)DstBitmap_p->Data1_p +
                        WidthZoomed * DstBitmap_p->Height;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];

    for(y=H; y!=0; --y)
    {
        U32 x, i;
        U8* LastRow_p = LastPixelD_p-WidthZoomed;

        for(x=W; x>0; --x)
        {
            U32 i;
            U8 Code = *(--LastPixelS_p);
            if (Code != LastCode)
                Color = (U8)Colors_p[LastCode = Code];
            for(i = ZoomW; i>0; --i) *(--LastPixelD_p) = Color;
        }
        for(i=ZoomH; i>0; --i)
        {
            /*
            U32 x;
            for(x=Increment; x!=0; --x) *(--LastPixelD_p) = *(--LastRow_p);
            */
            LastPixelD_p -= WidthZoomed;
            memcpy(LastPixelD_p, LastRow_p, WidthZoomed);
        }
    }
}   /* End of function ConvertGlyph8to8ZoomXxY */



void ZoomXxYGlyph8to8(STGXOBJ_Bitmap_t* SrcBitmap_p,
                      STGXOBJ_Bitmap_t* DstBitmap_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    S32 WidthZoomed = DstBitmap_p->Width;
    S32 ZoomW       = (WidthZoomed / W );
    S32 ZoomH       = (DstBitmap_p->Height / H ) - 1;
    U8* LastPixelS_p = (U8*)SrcBitmap_p->Data1_p + W * H;
    U8* LastPixelD_p = (U8*)DstBitmap_p->Data1_p +
                        WidthZoomed * DstBitmap_p->Height;

    for(y=H; y!=0; --y)
    {
        U32 x, i;
        U8* LastRow_p = LastPixelD_p-WidthZoomed;

        for(x=W; x>0; --x)
        {
            U32 i;
            U8 Code = *(--LastPixelS_p);
            for(i = ZoomW; i>0; --i) *(--LastPixelD_p) = Code;
        }
        for(i=ZoomH; i>0; --i)
        {
            /*
            U32 x;
            for(x=Increment; x!=0; --x) *(--LastPixelD_p) = *(--LastRow_p);
            */
            LastPixelD_p -= WidthZoomed;
            memcpy(LastPixelD_p, LastRow_p, WidthZoomed);
        }
    }
}   /* End of function ZoomXxYGlyph8to8 */



void ProcessGlyph1to8AndZoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                                STGXOBJ_Bitmap_t* DstBitmap_p,
                                const STGFX_GC_t* GC_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  BgColor     = (U8)GC_p->TxtFontAttributes_p->Color_p[0];
    U8  FgColor     = (U8)GC_p->TxtFontAttributes_p->Color_p[1];
    U8  Color       = BgColor;
    U8* EffectMatrix_p = DstBitmap_p->Data2_p;
    U8* Mask = GC_p->Font_p->FontData.TxtFontData_p->IsForeground_p;
    U8  Flag        = Mask[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 7;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x01);
            if (Code != LastCode)
            {
                Color = (LastCode = Code) ? FgColor : BgColor;
                Flag  = Mask[ Code ];
            }
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
            *EffectMatrix_p++ = Flag;
            if (!Shift) Shift = 7, ++GlyphData_p;
            else        --Shift;
        }
        ++GlyphData_p;
        Pixel_p   += WidthZoomed;
        NextRow_p += WidthZoomed;
    }
}   /* End of function ProcessGlyph1to8AndZoom2x2 */



void ProcessGlyph2to8AndZoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                                STGXOBJ_Bitmap_t* DstBitmap_p,
                                const STGFX_GC_t* GC_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8* Colors_p    = GC_p->TxtFontAttributes_p->Color_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];
    U8* EffectMatrix_p = DstBitmap_p->Data2_p;
    U8* Mask = GC_p->Font_p->FontData.TxtFontData_p->IsForeground_p;
    U8  Flag        = Mask[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 6;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x03);
            if (Code != LastCode)
            {
                Color = (U8)Colors_p[LastCode = Code];
                Flag  = Mask[ Code ];
            }
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
            *EffectMatrix_p++ = Flag;
            if (!Shift) Shift = 6, ++GlyphData_p;
            else        Shift -= 2;
        }
        ++GlyphData_p;
        Pixel_p   += WidthZoomed;
        NextRow_p += WidthZoomed;
    }
}   /* End of function ProcessGlyph2to8AndZoom2x2 */



void ProcessGlyph4to8AndZoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                                STGXOBJ_Bitmap_t* DstBitmap_p,
                                const STGFX_GC_t* GC_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8* Colors_p    = GC_p->TxtFontAttributes_p->Color_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];
    U8* EffectMatrix_p = DstBitmap_p->Data2_p;
    U8* Mask = GC_p->Font_p->FontData.TxtFontData_p->IsForeground_p;
    U8  Flag        = Mask[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        U32 Shift = 4;

        for(x=W; x!=0; --x)
        {
            U8 Code = (*GlyphData_p>>Shift)&(0x03);
            if (Code != LastCode)
            {
                Color = (U8)Colors_p[LastCode = Code];
                Flag  = Mask[ Code ];
            }
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
            *EffectMatrix_p++ = Flag;
            if (!Shift) Shift = 4, ++GlyphData_p;
            else        Shift -= 4;
        }
        ++GlyphData_p;
        Pixel_p   += WidthZoomed;
        NextRow_p += WidthZoomed;
    }
}   /* End of function ProcessGlyph4to8AndZoom2x2 */



void ProcessGlyph8to8AndZoom2x2(STGXOBJ_Bitmap_t* SrcBitmap_p,
                                STGXOBJ_Bitmap_t* DstBitmap_p,
                                const STGFX_GC_t* GC_p)
{
    U32 y;
    S32 W           = SrcBitmap_p->Width;
    S32 H           = SrcBitmap_p->Height;
    U8* GlyphData_p = SrcBitmap_p->Data1_p;
    U8* Pixel_p     = DstBitmap_p->Data1_p;
    U8* Colors_p    = GC_p->TxtFontAttributes_p->Color_p;
    S32 WidthZoomed = DstBitmap_p->Width;
    U8* NextRow_p   = Pixel_p + WidthZoomed;
    U8  LastCode    = 0;
    U8  Color       = (U8)Colors_p[0];
    U8* EffectMatrix_p = DstBitmap_p->Data2_p;
    U8* Mask = GC_p->Font_p->FontData.TxtFontData_p->IsForeground_p;
    U8  Flag        = Mask[0];

    for(y=H; y!=0; --y)
    {
        U32 x;
        for(x=W; x!=0; --x)
        {
            U8 Code = *GlyphData_p++;
            if (Code != LastCode)
            {
                Color = (U8)Colors_p[LastCode = Code];
                Flag  = Mask[ Code ];
            }
            *Pixel_p++   = Color; *Pixel_p++   = Color;
            *NextRow_p++ = Color; *NextRow_p++ = Color;
            *EffectMatrix_p++ = Flag;
        }
        Pixel_p   += WidthZoomed;
        NextRow_p += WidthZoomed;
    }
}   /* End of function ProcessGlyph8to8AndZoom2x2 */

#if 0
static void DumpBMP(STGXOBJ_Bitmap_t* Bitmap_p)
{
    /* debug function to display a glyph bitmap, eg before/after zooming */
    int x, y;
    U8 * Data_p = Bitmap_p->Data1_p;
    U32 BufSize = Bitmap_p->Pitch*3 + 1;
    char * LineBuf_p;
    char * Out_p;
    
    LineBuf_p = malloc(BufSize);
    if(LineBuf_p == NULL)
    {
        STTBX_Print(("DumpBMP: failed allocating %u byte buffer\n", BufSize));
        return;
    }
    
    for(y = 0; y < Bitmap_p->Height; ++y)
    {
        Out_p = LineBuf_p;
        for(x = 0; x < Bitmap_p->Pitch; ++x)
        {
            Out_p += sprintf(Out_p, "%02X ", *Data_p++);
        }
        STTBX_Print(("%s\n", LineBuf_p));
    }
    
    free(LineBuf_p);
}
#endif

/* end of sttxt.c */
