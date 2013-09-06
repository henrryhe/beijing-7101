/******************************************************************************

    File Name   : stolf.c

    Description : Outline font functions

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
 #include <debug.h> 
#endif  /*end yxl 2006-11-30 add this line*/
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <string.h> 
#include "pfrgen.h"
/*#include "pfrhead.h"*/
/*#include "logrec.h"*/
/*#include "physrec.h"*/
#include "stolf.h"
#include "glyph.h"
#include "gfx_tools.h" /* stgfx_init.h */

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/*#define NCHARS      256*/

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */


ST_ErrorCode_t DrawOutlineText(STBLIT_BlitContext_t* BlitContext_p,
                               const ST_String_t Text, const U16* WText,
                               S32 XScreen, S32 YScreen,
                               STGFX_Handle_t Handle, U32 NumChars,
                               STGXOBJ_Bitmap_t* Bmp_p, const STGFX_GC_t* GC_p)
{
  S32 XMax;
  U16 Character;
  U32 Pos;
  STGFX_OutlineFont_t* Font_p = GC_p->Font_p->FontData.OutlineFontData_p;
  ST_ErrorCode_t DrawErr = ST_NO_ERROR;
  U8 Size = MIN(GC_p->FontSize,MAX_OLF_SIZE);
  BOOL First = TRUE;

  YScreen -= (Size*Font_p->Baseline)/Font_p->OutlineResolution;
  /* initial x offset needs to be with first char actually drawn - see below */
  
  /* DrawGlyph now allocates XYL arrays dynamically */
  BlitContext_p->NotifyBlitCompletion = TRUE;
  
  for(Pos = 0; Pos < NumChars; ++Pos)
  {
    if(Text != NULL)
    {
      /* narrow-character string */
      if(!Text[Pos]) break;
      Character = (U16)(U8) Text[Pos];
    }
    else
    {
      /* wide-character string */
      if(!WText[Pos]) break;
      Character = WText[Pos];
    }
  
    if (GC_p->Font_p->FontType & STGFX_USE_CHAR_MAPPING)
    {
      if(ST_NO_ERROR != stgfx_CharCodeToIdx(&GC_p->Font_p->CharMapping,
                                            Character, &Character))
      {
        Character = Font_p->NumCharacters; /* don't draw */
      }
    }
    
    if (Character < Font_p->NumCharacters) /* NCHARS */
    {
      if(First)
      {
        /* do start offset with first character actually drawn */
        XScreen -= (Size*Font_p->Glyph_p[Character].BoundingBoxX)/
                        Font_p->OutlineResolution;
        First = FALSE;
      }

      DrawErr=DrawGlyph(BlitContext_p, Character, XScreen, YScreen,
                        Handle, Bmp_p, GC_p, &XMax);
      if (DrawErr != ST_NO_ERROR) break;
      
      XScreen += (Font_p->Glyph_p[Character].Width*Size)/
                     (Font_p->OutlineResolution);
    }
  }
  
  return(DrawErr);
}

ST_ErrorCode_t GetOutlineCharAscent(U16 Idx, U16 Size, 
                                    STGFX_OutlineFont_t* Font_p,
                                    S16* Ascent_p)
{
  Size = MIN(Size,MAX_OLF_SIZE); 
  if (Idx < Font_p->NumCharacters)
  {
    S32 Ascent;
    Ascent = (Font_p->Glyph_p[Idx].Ascent*Size)/
            (Font_p->OutlineResolution);
    *Ascent_p = Ascent;
    return ST_NO_ERROR;
  } else {
    return STGFX_ERROR_NOT_CHARACTER;
  }
}

ST_ErrorCode_t GetOutlineCharDescent(U16 Idx, U16 Size, 
                                     STGFX_OutlineFont_t* Font_p,
                                     S16* Descent_p)
{
  Size = MIN(Size,MAX_OLF_SIZE); 
  if (Idx < Font_p->NumCharacters)
  {
    S32 Descent;
    Descent = (Font_p->Glyph_p[Idx].Descent*Size)/
            (Font_p->OutlineResolution);
    *Descent_p = Descent;
    return ST_NO_ERROR;
  } else {
    return STGFX_ERROR_NOT_CHARACTER;
  }
}



ST_ErrorCode_t GetOutlineCharWidth(U16 Idx, U16 Size,
                                   STGFX_OutlineFont_t* Font_p, U16* Width_p)
{
  Size = MIN(Size,MAX_OLF_SIZE);
  if (Idx < Font_p->NumCharacters)
  {
    *Width_p = (Font_p->Glyph_p[Idx].Width * Size)/Font_p->OutlineResolution;
    return ST_NO_ERROR;
  }
  else
  {
    return STGFX_ERROR_NOT_CHARACTER;
  }
}

ST_ErrorCode_t GetOutlineFontHeight(U16 Size, 
                                    STGFX_OutlineFont_t* Font_p,
                                    U16* Height_p)
{
  Size = MIN(Size,MAX_OLF_SIZE); 
  *Height_p = ((Font_p->Ascent+Font_p->Descent)*Size) /
                  (Font_p->OutlineResolution);
  return ST_NO_ERROR;
}


ST_ErrorCode_t GetOutlineFontAscent(U16 Size, 
                                    STGFX_OutlineFont_t* Font_p,
                                    S16* Ascent_p)
{
  Size = MIN(Size,MAX_OLF_SIZE); 
  *Ascent_p = (Font_p->Ascent*Size) / (Font_p->OutlineResolution);
  return ST_NO_ERROR;
}

ST_ErrorCode_t GetOutlinePrintableChars(
                      const STGFX_OutlineFont_t* Font_p,
                      const STGFX_CharMapping_t* CharMapping_p,
                      const ST_String_t TextString,
                      const U16* WTextString,
                      U16  FontSize,
                      U32  AvailableLength,
                      U32* NumOfChars_p)
{
  U16 Character;
  int i;
  int Width = 0;

  *NumOfChars_p = 0;
  FontSize = MIN(FontSize,MAX_OLF_SIZE); 

  /* nothing matching DrawOutlineText's removal of first BoundingBoxX ? */
  
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
        Character = Font_p->NumCharacters; /* don't draw */
      }
    }
    
    if (Character < Font_p->NumCharacters) /* NCHARS */
      Width += ((Font_p->Glyph_p[Character].Width*FontSize)/
                (Font_p->OutlineResolution));
    else 
      return STGFX_ERROR_NOT_CHARACTER;

    if (Width < AvailableLength)
      (*NumOfChars_p) ++;
    else
      break;
  }

  return ST_NO_ERROR;
}

ST_ErrorCode_t GetOutlineTextWidth(STGFX_Handle_t Handle,
                                   const STGFX_OutlineFont_t* Font_p,
                                   const STGFX_CharMapping_t* CharMapping_p,
                                   const ST_String_t TextString,
                                   const U16* WTextString,
                                   U16  FontSize,
                                   U32* Width_p)
{
  S32             i;
  U16             Character;
  BOOL            Last = FALSE;
  STGFX_GC_t      GC;
  S32             XMax = 0;
  STGFX_Font_t    Font;
  ST_ErrorCode_t  Err;

  /**Width_p = 0;*/

  FontSize = MIN(FontSize,MAX_OLF_SIZE);

  Character = (TextString != NULL) ? (U16)(U8) TextString[0] : WTextString[0];
  if(!Character)
  {
    /* zero-length string */
    *Width_p = 0;
    return ST_NO_ERROR;
  }
  
  if (CharMapping_p != NULL)
  {
    if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,Character,&Character))
    {
      Character = Font_p->NumCharacters; /* don't draw */
    }
  }

  /* char range checked below */
  XMax = -(FontSize*Font_p->Glyph_p[Character].BoundingBoxX) /
              Font_p->OutlineResolution; /* like DrawOutlineText */

  for(i = 0; 1 /* breaks below */; i++)
  {
    if(TextString != NULL)
    {
      /* narrow-character string */
      if(!TextString[i+1]) Last = TRUE;
      Character = (U16)(U8) TextString[i];
    }
    else
    {
      /* wide-character string */
      if(!WTextString[i+1]) Last = TRUE;
      Character = WTextString[i];
    }
  
    if (CharMapping_p != NULL)
    {
      if(ST_NO_ERROR != stgfx_CharCodeToIdx(CharMapping_p,Character,&Character))
      {
        Character = Font_p->NumCharacters; /* don't draw */
      }
    }
    
    if (Character < Font_p->NumCharacters) /* NCHARS */
    {
      if(Last)
      {
        Font.FontType = STGFX_OUTLINE_FONT; /* no char mapping */
        Font.FontData.OutlineFontData_p = (STGFX_OutlineFont_t*)Font_p;

        GC.Font_p = &Font;
        GC.FontSize = FontSize;
        
        /* obtain actual XMax of final character, not spacing parameter */
        Err = DrawGlyph(NULL, Character, XMax, 0, Handle, NULL, &GC, &XMax);
        break;
      }
      else
      {
        XMax += (Font_p->Glyph_p[Character].Width * FontSize) /
                    Font_p->OutlineResolution;
      }
    }
    else
    {
      return STGFX_ERROR_NOT_CHARACTER;
    }
  }
  
  *Width_p = (U32)XMax;

  return Err;
}

ST_ErrorCode_t GetOutlineTextHeight(const STGFX_OutlineFont_t* Font_p,
                                    const STGFX_CharMapping_t* CharMapping_p,
                                    const ST_String_t TextString,
                                    const U16* WTextString,
                                    U16  FontSize,
                                    U32* Height_p)
{
  U16 Character;
  U32 Height;
  int i;

  *Height_p = 0;
  FontSize = MIN(FontSize,MAX_OLF_SIZE); 

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
        Character = Font_p->NumCharacters; /* don't draw */
      }
    }
    
    if (Character < Font_p->NumCharacters) /* NCHARS */
    {
      Height = (U32)(Font_p->Glyph_p[Character].Ascent + 
                        Font_p->Glyph_p[Character].Descent);

      if((*Height_p) < Height)
        *Height_p = Height;
    }
    else 
      return STGFX_ERROR_NOT_CHARACTER;
  }

  *Height_p *= FontSize;
  *Height_p /= Font_p->OutlineResolution;

  return ST_NO_ERROR;
}
