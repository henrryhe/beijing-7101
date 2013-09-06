/*******************************************************************************
File name   : stbdf.c

Description : STGFX functions for rendering of BDF fonts

COPYRIGHT (C) STMicroelectronics 2000/2001.
*******************************************************************************/

/*#define STTBX_PRINT / temporary debug */

/* Includes ----------------------------------------------------------------- */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sttbx.h"
#include "stavmem.h"
#include "stbdf.h"
#include "gfx_tools.h" /* includes stgfx_init.h, ... */

/* Private Constants -------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

ST_ErrorCode_t DrawBitmappedText(
  STGFX_Handle_t        Handle,
  STBLIT_BlitContext_t* BlitContext_p,
  STGXOBJ_Bitmap_t*     Bmp_p,
  const STGFX_GC_t*     GC_p,
  const ST_String_t     String, 
  const U16*            WString, 
  S32                   XPos, 
  S32                   YPos,
  U32                   NumOfChars
)
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  STGFX_BitmappedFont_t* Font_p = GC_p->Font_p->FontData.BitmappedFontData_p;
  STGXOBJ_Bitmap_t       GlyphBmap;
  STAVMEM_BlockHandle_t  GlyphBH;
  STBLIT_Handle_t        BlitHandle;
  STBLIT_BlitContext_t   BC;
  STGXOBJ_Rectangle_t    Rectangle; /* target rectangle for current glyph */
  STGXOBJ_Color_t*       Color_p; /* colour for current glyph */
  STAVMEM_AllocBlockParams_t  AVMEMAllocBlockParams;
  STAVMEM_BlockHandle_t  WorkBufferBH;
  /*const U32              MaskPaletteSize = 2*4; - CLUT2 */
  const U32              MaskPaletteSize = 129*4; /* partial workaround GNBvd16542 */
  U16                    CurrentChar;
  U32                    i;
  S32                    NewX, NewY; /* next blit position (top-left of cell) */
  S32                    NewOriginX; /* cell origin position */
  U16                    j;
  U32*                   Data_p;
  U32                    IsBackground; /* whether we're drawing fg or bg */
 
  BlitHandle = stgfx_GetBlitterHandle(Handle);
  
  /* prepare constant bits of GlyphBmap */
  GlyphBmap.ColorType = STGXOBJ_COLOR_TYPE_ALPHA1;
  GlyphBmap.BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
  GlyphBmap.PreMultipliedColor = FALSE;
  GlyphBmap.ColorSpaceConversion = STGXOBJ_CONVERSION_MODE_UNKNOWN;
  GlyphBmap.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
  GlyphBmap.Offset = 0;
  GlyphBmap.SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
  GlyphBmap.BigNotLittle = FALSE;
  
  /* prepare BlitContext (why do it again?) */
  Err = stgfx_InitBlitContext(Handle, &BC, GC_p, Bmp_p);
  if (Err != ST_NO_ERROR)
  {
    return Err;
  }
  BC.EnableMaskBitmap           = TRUE;
  BC.MaskBitmap_p               = &GlyphBmap;
  BC.MaskRectangle.PositionX    = 0;
  BC.MaskRectangle.PositionY    = 0;
  /* GlyphBmap data, Width and Height supplied for each glyph below */
  
  /* BC.WorkBuffer is actually used for a mask palette, which means we need one
    for the foreground and one for the background. We might do better writing the
    colour expansion explicitly - we could avoid any copying then if the font's
    glyph images were guaranteed to be 4-byte aligned (which even malloc does) */

  /* palettes need 16-byte alignment instead of the usual 4, and STBLIT performs
    a dangerous round-down of the address if we provide a non-aligned one */
  AVMEMAllocBlockParams.PartitionHandle =
                      (((stgfx_Unit_t*)Handle)->Device_p)->AVMemPartitionHandle;
  AVMEMAllocBlockParams.Size            = 2*MaskPaletteSize;
  AVMEMAllocBlockParams.Alignment       = 16; /* needed for palettes */
  AVMEMAllocBlockParams.AllocMode       = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
  AVMEMAllocBlockParams.NumberOfForbiddenRanges  = 0;
  AVMEMAllocBlockParams.ForbiddenRangeArray_p    = NULL;
  AVMEMAllocBlockParams.NumberOfForbiddenBorders = 0;
  AVMEMAllocBlockParams.ForbiddenBorderArray_p   = NULL;

  Err = STAVMEM_AllocBlock(&AVMEMAllocBlockParams, &WorkBufferBH);
  if ( Err != ST_NO_ERROR )
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error allocating AVMEM memory : 0x%08X",Err));
      return (STGFX_ERROR_STAVMEM);
  }

  Err = stgfx_AVMEM_GetPhysAddr(Handle, WorkBufferBH, (void**) &BC.WorkBuffer_p);
  if ( Err != ST_NO_ERROR )
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error allocating memory : 0x%08X",Err));
      stgfx_AVMEM_free(Handle, &WorkBufferBH);
      return (STGFX_ERROR_STAVMEM);
  }
  
  /* rendering of the string */
  for (IsBackground = 0;
       (IsBackground < (GC_p->UseTextBackground ? 2 : 1));
       ++IsBackground)
  {
    NewOriginX = XPos;
    
    if(IsBackground)
    {
      Color_p = (STGXOBJ_Color_t*) &(GC_p->TextBackgroundColor);
    }
    else
    {
      Color_p = (STGXOBJ_Color_t*) &(GC_p->TextForegroundColor);
    }
    
    for (i=0; (i<NumOfChars); i++)
    {
      if(String != NULL)
      {
        /* narrow-character string */
        if(!String[i]) break;
        CurrentChar = (U16)(U8) String[i];
      }
      else
      {
        /* wide-character string */
        if(!WString[i]) break;
        CurrentChar = WString[i];
      }

      if (GC_p->Font_p->FontType & STGFX_USE_CHAR_MAPPING)
      {
        if(ST_NO_ERROR != stgfx_CharCodeToIdx(&GC_p->Font_p->CharMapping,
                                              CurrentChar, &CurrentChar))
        {
          CurrentChar = Font_p->DefaultChar; /* don't draw */
        }
      }

      if(CurrentChar >= Font_p->NumOfGlyphs)
      {
        CurrentChar = Font_p->DefaultChar;
      }

      /* calculate x, y to place glyph bitmap */
      NewX = NewOriginX + Font_p->Glyph_p[CurrentChar].BoundingBoxX;
      NewY = YPos - (Font_p->Glyph_p[CurrentChar].BoundingBoxH +
                     Font_p->Glyph_p[CurrentChar].BoundingBoxY);

      GlyphBmap.Width  = Font_p->Glyph_p[CurrentChar].BoundingBoxW;
      GlyphBmap.Height = Font_p->Glyph_p[CurrentChar].BoundingBoxH;
      GlyphBmap.Pitch  = ((7+Font_p->Glyph_p[CurrentChar].BoundingBoxW)>>3);
      GlyphBmap.Size1 = GlyphBmap.Height*GlyphBmap.Pitch;
      
      if (GlyphBmap.Size1 % 4)
      {
        /* pad size to U32 word as that's how we do the copying */
        GlyphBmap.Size1 += (4 - GlyphBmap.Size1 % 4);
      }

      /* Why copy the data? Alignment? Even malloc should be 4-byte aligned */
      /* also location - needs to be accessible to hardware blitter */

      Err = stgfx_AVMEM_malloc(Handle, GlyphBmap.Size1, &GlyphBH, (void**) &Data_p);
      if ( Err != ST_NO_ERROR )
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error allocating memory : 0x%08X",Err));
          stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
          /* previously drawn glyph bitmaps will have their blocks freed as usual */
          return (Err);
      }

      /* Copy wordwise to improve performance and avoid errors going across
        fixed-width data bus to devices like 7020 */
        
      if(IsBackground)
      {
        for(j=0; j < GlyphBmap.Size1/4; j++)
        {
          /* copy inverted */
          Data_p[j] = ~((U32*)Font_p->Glyph_p[CurrentChar].GlyphData_p)[j];
        }
      }
      else
      {
        for(j=0; j < GlyphBmap.Size1/4; j++)
        {
          /* copy verbatim */
          Data_p[j] = ((U32*)Font_p->Glyph_p[CurrentChar].GlyphData_p)[j];
        }
      }
      
      GlyphBmap.Data1_p = Data_p;

      BC.MaskRectangle.Width        = GlyphBmap.Width;
      BC.MaskRectangle.Height       = GlyphBmap.Height;

      Rectangle.PositionX = NewX;
      Rectangle.PositionY = NewY;
      Rectangle.Width     = GlyphBmap.Width;
      Rectangle.Height    = GlyphBmap.Height;

      BC.UserTag_p     = (void*)GlyphBH;

      /* old code used STBLIT_Blit or masked colour fill where
        (GC_p->TextForegroundColor.Type != Bmp_p->ColorType) */

      Err = STBLIT_FillRectangle(BlitHandle, Bmp_p, &Rectangle, Color_p, &BC);
      if(Err != ST_NO_ERROR)
      {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error during BLIT operation: 0x%08X", Err));
        stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
        /* previously drawn glyph bitmaps will have their blocks freed as usual */
        return(STGFX_ERROR_STBLIT_BLIT);
      }
      NewOriginX += Font_p->Glyph_p[CurrentChar].Width;
    }
    
    /* new work buffer for the new colours */
    BC.WorkBuffer_p = (void*)((U32)BC.WorkBuffer_p + MaskPaletteSize);
  }
  
  Err = stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
  
  return Err;
}


/****************************************************************************
 Metric functions: all assume pointers have been previously validated
  by the top-level api functions
*****************************************************************************/
 
ST_ErrorCode_t GetBitmappedCharWidth(const STGFX_BitmappedFont_t* Font_p,
                                     U16   Character, 
                                     U16*  Width_p)
{
  if (Character < Font_p->NumOfGlyphs)
    *Width_p = Font_p->Glyph_p[Character].Width;
  else 
    return STGFX_ERROR_NOT_CHARACTER;

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetBitmappedCharAscent(const STGFX_BitmappedFont_t* Font_p,
                                      U16   Character, 
                                      S16*  Ascent_p)
{
  if (Character < Font_p->NumOfGlyphs)
    *Ascent_p = Font_p->Glyph_p[Character].BoundingBoxH + 
              Font_p->Glyph_p[Character].BoundingBoxY;
  else 
    return STGFX_ERROR_NOT_CHARACTER;

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetBitmappedCharDescent(const STGFX_BitmappedFont_t* Font_p,
                                       U16   Character, 
                                       S16*  Descent_p)
{
  if (Character < Font_p->NumOfGlyphs)
    *Descent_p = - Font_p->Glyph_p[Character].BoundingBoxY; 
  else 
    return STGFX_ERROR_NOT_CHARACTER;

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetBitmappedFontAscent(const STGFX_BitmappedFont_t* Font_p, 
                                      S16* Ascent_p)
{
  *Ascent_p = Font_p->FontAscent;

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetBitmappedFontHeight(const STGFX_BitmappedFont_t* Font_p, 
                                      U16* Height_p)
{
  *Height_p = Font_p->FontAscent + Font_p->FontDescent;

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetBitmappedPrintableChars(
                      const STGFX_BitmappedFont_t* Font_p,
                      const STGFX_CharMapping_t* CharMapping_p,
                      const ST_String_t TextString,
                      const U16* WTextString,
                      U32  AvailableLength,
                      U32* NumOfChars_p)
{
  S32 i;
  U16 Character;
  S32 Width = 0;

  *NumOfChars_p = 0;

  for(i=0; 1 /* breaks below */; i++)
  {
    if(TextString != NULL)
    {
      /* narrow-character string */
      if(!TextString[i]) break;
      Character = (U16)(U8) TextString[i];
    }
    else
    {
      /* wide-character string */
      if(!WTextString[i]) break;
      Character = WTextString[i];
    }
  
    if (CharMapping_p != NULL)
    {
      if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,Character,&Character))
      {
        return STGFX_ERROR_NOT_CHARACTER;
      }
    }

    if (Character < Font_p->NumOfGlyphs)
      Width += Font_p->Glyph_p[Character].Width;
    else 
      return STGFX_ERROR_NOT_CHARACTER;

    if (Width < AvailableLength)
      (*NumOfChars_p) ++;
    else
      break;
  }

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetBitmappedTextWidth(const STGFX_BitmappedFont_t* Font_p,
                                     const STGFX_CharMapping_t* CharMapping_p,
                                     const ST_String_t TextString,
                                     const U16* WTextString,
                                     U32* Width_p)
{
  S32 i;
  U16 Character;

  *Width_p = 0;

  for(i = 0; 1 /* breaks below */; i++)
  {
    if(TextString != NULL)
    {
      /* narrow-character string */
      if(!TextString[i]) break;
      Character = (U16)(U8) TextString[i];
    }
    else
    {
      /* wide-character string */
      if(!WTextString[i]) break;
      Character = WTextString[i];
    }
  
    if (CharMapping_p != NULL)
    {
      if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,Character,&Character))
      {
        return STGFX_ERROR_NOT_CHARACTER;
      }
    }

    if (Character < Font_p->NumOfGlyphs)
      *Width_p += Font_p->Glyph_p[Character].Width;
    else 
      return STGFX_ERROR_NOT_CHARACTER;
  }

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetBitmappedTextHeight(const STGFX_BitmappedFont_t* Font_p,
                                      const STGFX_CharMapping_t* CharMapping_p,
                                      const ST_String_t TextString,
                                      const U16* WTextString,
                                      U32* Height_p)
{
  S32 i;
  U16 Character;

  *Height_p = 0;

  for(i = 0; 1 /* breaks below */; i++)
  {
    if(TextString != NULL)
    {
      /* narrow-character string */
      if(!TextString[i]) break;
      Character = (U16)(U8) TextString[i];
    }
    else
    {
      /* wide-character string */
      if(!WTextString[i]) break;
      Character = WTextString[i];
    }
  
    if (CharMapping_p != NULL)
    {
      if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,Character,&Character))
      {
        return STGFX_ERROR_NOT_CHARACTER;
      }
    }

    if (Character < Font_p->NumOfGlyphs)
    {
      if((*Height_p) < Font_p->Glyph_p[Character].BoundingBoxH)
        *Height_p = Font_p->Glyph_p[Character].BoundingBoxH;
    }
    else 
      return STGFX_ERROR_NOT_CHARACTER;
  }

  return ST_NO_ERROR;
}
