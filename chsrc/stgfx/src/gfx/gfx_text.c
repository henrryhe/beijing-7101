/********************************************************

        STGFX fonts related functions

********************************************************/

/*#define STTBX_PRINT / temporary debug */
/*#include "sttbx.h"*/
/*#define STTBX_Print(x) yxl 2004-05-16 cancel this line*/

#include <string.h>
#include "gfx_tools.h" /* includes stgfx_init.h, ... */
#include "stolf.h"
#include "stbdf.h"
#include "sttxt.h"

/******************************************************************************
Function Name : stgfx_CharCodeToIdx
  Description : Apply a CharMapping to convert a character code to a glyph
                array index
   Parameters : mapping struct, input code, location for output index
      Returns : ST_NO_ERROR / STGFX_ERROR_NOT_CHARACTER (when character falls
                within an unmapped gap)
******************************************************************************/
ST_ErrorCode_t stgfx_CharCodeToIdx(const STGFX_CharMapping_t* CharMapping_p,
                                   U16 Code, U16* Idx_p)
{
    U16 i, idx;

    idx = Code; /* provides default 0-0 range at start */
    for(i = 0; i < CharMapping_p->NumEntries; i++)
    {
        if (Code < CharMapping_p->Entries_p[i].Code) 
        {
            /* before start of this block */
            if (idx < CharMapping_p->Entries_p[i].Idx)
            {
                break; /* match within previous block */
            }
            return STGFX_ERROR_NOT_CHARACTER;
        }
        
        /* otherwise calculate index of tentative match within current block */
        idx = CharMapping_p->Entries_p[i].Idx +
            (Code - CharMapping_p->Entries_p[i].Code);
    }

    *Idx_p = idx;
    return ST_NO_ERROR;
}

ST_ErrorCode_t stgfx_DrawText(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  const ST_String_t         String,
  const U16*                WString,
  S32                       XPos,
  S32                       YPos,
  U32                       NumOfChars)
{
  ST_ErrorCode_t        Err = ST_NO_ERROR;
  STBLIT_BlitContext_t  BlitContext;
    
  /* check handle validity */
  if(NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }

  /* copy blit params to blitting context (and check GC_p, Bitmap_p) */
  Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
  if (Err != ST_NO_ERROR)
  {
    return Err;
  }
  BlitContext.NotifyBlitCompletion      = FALSE;

  if (GC_p->Font_p == NULL)
  {
    return(STGFX_ERROR_INVALID_GC);
  }
  
  if((String == NULL) && (WString == NULL))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  
  if (XPos>0) 
  {
    XPos >>= stgfx_GetUserAccuracy(Handle);
  } 
  else 
  {
    XPos = -((-XPos)>>stgfx_GetUserAccuracy(Handle));
  }

  if (YPos>0) 
  {
    YPos >>= stgfx_GetUserAccuracy(Handle);
  } 
  else 
  {
    YPos = -((-YPos)>>stgfx_GetUserAccuracy(Handle));
  }

  switch (GC_p->Font_p->FontType & STGFX_FONT_TYPE_MASK) 
  {
  case STGFX_BITMAPPED_FONT:
      if (GC_p->Font_p->FontData.BitmappedFontData_p != NULL)
      {
        Err = DrawBitmappedText(Handle, &BlitContext, Bitmap_p, GC_p,
                                String, WString, XPos, YPos, NumOfChars);
      } 
      else 
      {
        Err = STGFX_ERROR_INVALID_FONT;
      }
      break;
  case STGFX_OUTLINE_FONT:
      if((GC_p->Font_p->FontData.OutlineFontData_p != NULL) && GC_p->FontSize)
      {
        Err = DrawOutlineText(&BlitContext,String,WString,XPos,YPos,
                              Handle,NumOfChars,Bitmap_p,GC_p);
      }
      else
      {
        Err = STGFX_ERROR_INVALID_FONT;
      }
      break;
  case STGFX_TXT_FONT:
      if ((GC_p->Font_p->FontData.TxtFontData_p != NULL) &&
          (GC_p->TxtFontAttributes_p != NULL))
      {
        Err = DrawTxtText(&BlitContext,Bitmap_p,GC_p,
                          Handle,String,WString,XPos,YPos,NumOfChars);
      } 
      else 
      {
        Err = STGFX_ERROR_INVALID_FONT;
      }
      break;
  default:
      Err = STGFX_ERROR_INVALID_FONT;
      break;
  }

  return(Err);
}

ST_ErrorCode_t STGFX_GetWCharAscent(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  U16                       Character,
  S16*                      Ascent_p)
{
  ST_ErrorCode_t Err=ST_NO_ERROR;

  if (Font_p && Ascent_p) 
  {
    if(Font_p->FontType & STGFX_USE_CHAR_MAPPING)
    {
        Err = stgfx_CharCodeToIdx(&Font_p->CharMapping, Character, &Character);
        if(Err != ST_NO_ERROR) return Err; /* STGFX_ERROR_NOT_CHARACTER */
    }
  
    switch (Font_p->FontType & STGFX_FONT_TYPE_MASK) 
    {
    case STGFX_BITMAPPED_FONT:
        if (!Font_p->FontData.BitmappedFontData_p)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
        Err=GetBitmappedCharAscent(Font_p->FontData.BitmappedFontData_p,
                                   Character,Ascent_p);
        break;
    case STGFX_OUTLINE_FONT:
        if (!(Font_p->FontData.OutlineFontData_p && FontSize))
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
        Err=GetOutlineCharAscent(Character, FontSize,
                                 Font_p->FontData.OutlineFontData_p,Ascent_p);
        break;
    case STGFX_TXT_FONT:
        if (!Font_p->FontData.TxtFontData_p)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
        Err=GetTxtCharAscent(Font_p->FontData.TxtFontData_p,
                             FontSize, Character, Ascent_p);
        break;
    default:
        Err=STGFX_ERROR_INVALID_FONT;
        break;
    }
  } 
  else 
  {
    Err=ST_ERROR_BAD_PARAMETER;
  }

  return(Err);
}

ST_ErrorCode_t STGFX_GetWCharDescent(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  U16                       Character,
  S16*                      Descent_p)
{

  ST_ErrorCode_t Err=ST_NO_ERROR;

  if (Font_p && Descent_p)
  {
    if(Font_p->FontType & STGFX_USE_CHAR_MAPPING)
    {
        Err = stgfx_CharCodeToIdx(&Font_p->CharMapping, Character, &Character);
        if(Err != ST_NO_ERROR) return Err; /* STGFX_ERROR_NOT_CHARACTER */
    }
  
    switch (Font_p->FontType & STGFX_FONT_TYPE_MASK)
    {
    case STGFX_BITMAPPED_FONT:
      if (Font_p->FontData.BitmappedFontData_p){
        Err=GetBitmappedCharDescent(Font_p->FontData.BitmappedFontData_p,
                                    Character,Descent_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
        case STGFX_OUTLINE_FONT:
          if (Font_p->FontData.OutlineFontData_p && FontSize){
            Err=GetOutlineCharDescent(Character, FontSize,
                                      Font_p->FontData.OutlineFontData_p,Descent_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_TXT_FONT:
      if (Font_p->FontData.TxtFontData_p){
        Err=GetTxtCharDescent(Font_p->FontData.TxtFontData_p,
                              FontSize, Character, Descent_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
      
    default:
      Err=STGFX_ERROR_INVALID_FONT;
      break;
    }
  }
  else
  {
    Err=ST_ERROR_BAD_PARAMETER;
  }

  return(Err);
}


ST_ErrorCode_t STGFX_GetWCharWidth(
   const STGFX_Font_t*       Font_p,
   U16                       FontSize,
   U16                       Character,
   U16*                      Width_p)
{
  ST_ErrorCode_t Err=ST_NO_ERROR;

  if (Font_p && Width_p)
  {
    if(Font_p->FontType & STGFX_USE_CHAR_MAPPING)
    {
        Err = stgfx_CharCodeToIdx(&Font_p->CharMapping, Character, &Character);
        if(Err != ST_NO_ERROR) return Err; /* STGFX_ERROR_NOT_CHARACTER */
    }
  
    switch (Font_p->FontType & STGFX_FONT_TYPE_MASK)
    {
    case STGFX_BITMAPPED_FONT:
      if (Font_p->FontData.BitmappedFontData_p)
      {
        Err=GetBitmappedCharWidth(Font_p->FontData.BitmappedFontData_p,
                                  Character, Width_p);
      }
      else
      {
        Err=STGFX_ERROR_INVALID_FONT;
      }
      break;
    case STGFX_OUTLINE_FONT:
      if (Font_p->FontData.OutlineFontData_p && FontSize)
      {
        Err=GetOutlineCharWidth(Character, FontSize,
                                Font_p->FontData.OutlineFontData_p, Width_p);
      }
      else
      {
        Err=STGFX_ERROR_INVALID_FONT;
      }
      break;
    case STGFX_TXT_FONT:
      if (Font_p->FontData.TxtFontData_p)
      {
        Err=GetTxtCharWidth(Font_p->FontData.TxtFontData_p,
                            FontSize, Character, Width_p);
      }
      else
      {
        Err=STGFX_ERROR_INVALID_FONT;
      }
      break;
    default:
      Err=STGFX_ERROR_INVALID_FONT;
      break;
    }
  }
  else
  {
    Err=ST_ERROR_BAD_PARAMETER;
  }

  return(Err);
}

ST_ErrorCode_t STGFX_GetFontAscent(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  S16*                      Ascent_p)
{
  ST_ErrorCode_t Err=ST_NO_ERROR;

  if (Font_p && Ascent_p)
  {
    switch (Font_p->FontType & STGFX_FONT_TYPE_MASK)
    {
    case STGFX_BITMAPPED_FONT:
      if (Font_p->FontData.BitmappedFontData_p){
        Err=GetBitmappedFontAscent(Font_p->FontData.BitmappedFontData_p,
                                   Ascent_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_OUTLINE_FONT:
      if (Font_p->FontData.OutlineFontData_p && FontSize){
        Err=GetOutlineFontAscent(FontSize,Font_p->FontData.OutlineFontData_p,
                                 Ascent_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_TXT_FONT:
      if (Font_p->FontData.TxtFontData_p){
        Err=GetTxtFontAscent(Font_p->FontData.TxtFontData_p, FontSize, Ascent_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    default:
      Err=STGFX_ERROR_INVALID_FONT;
      break;
    }
  } else {
    Err=ST_ERROR_BAD_PARAMETER;
  }

  return(Err);
}

ST_ErrorCode_t STGFX_GetFontHeight(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  U16*                      Height_p)
{
  ST_ErrorCode_t Err=ST_NO_ERROR;

  if (Font_p && Height_p)
  {
    switch (Font_p->FontType & STGFX_FONT_TYPE_MASK)
    {
    case STGFX_BITMAPPED_FONT:
      if (Font_p->FontData.BitmappedFontData_p){
        Err=GetBitmappedFontHeight(Font_p->FontData.BitmappedFontData_p,
                                   Height_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_OUTLINE_FONT:
      if (Font_p->FontData.OutlineFontData_p && FontSize){
        Err=GetOutlineFontHeight(FontSize,Font_p->FontData.OutlineFontData_p,
                                 Height_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_TXT_FONT:
      if (Font_p->FontData.TxtFontData_p){
        Err=GetTxtFontHeight(Font_p->FontData.TxtFontData_p, FontSize, Height_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    default:
      Err=STGFX_ERROR_INVALID_FONT;
      break;
    }
  } else {
    Err=ST_ERROR_BAD_PARAMETER;
  }

  return(Err);
}

ST_ErrorCode_t stgfx_GetPrintableChars(
  const STGFX_Font_t*       Font_p,
  U32                       FontSize,
  const ST_String_t         TextString,
  const U16*                WTextString,
  U32                       AvailableLength,
  U32*                      NumOfChars_p)
{
  ST_ErrorCode_t Err = ST_NO_ERROR;
  const STGFX_CharMapping_t* CharMapping_p;

  if (Font_p && (TextString || WTextString) && NumOfChars_p && AvailableLength)
  {
    CharMapping_p = (Font_p->FontType & STGFX_USE_CHAR_MAPPING)
                        ? &Font_p->CharMapping : NULL;
                        
    switch (Font_p->FontType & STGFX_FONT_TYPE_MASK)
    {
    case STGFX_BITMAPPED_FONT:
      if (Font_p->FontData.BitmappedFontData_p){
        Err = GetBitmappedPrintableChars(Font_p->FontData.BitmappedFontData_p,
                                         CharMapping_p, TextString, WTextString,
                                         AvailableLength, NumOfChars_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_OUTLINE_FONT:
      if (Font_p->FontData.OutlineFontData_p && FontSize){
        Err = GetOutlinePrintableChars(Font_p->FontData.OutlineFontData_p,
                                       CharMapping_p, TextString, WTextString,
                                       FontSize, AvailableLength, NumOfChars_p);
      } else {
    Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_TXT_FONT:
      if (Font_p->FontData.TxtFontData_p){
        Err = GetTxtPrintableChars(Font_p->FontData.TxtFontData_p,
                                   CharMapping_p, TextString, WTextString,
                                   FontSize, AvailableLength, NumOfChars_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    default:
      Err=STGFX_ERROR_INVALID_FONT;
      break;
    }
  } else {
    Err=ST_ERROR_BAD_PARAMETER;
  }
  return(Err);
}


ST_ErrorCode_t stgfx_GetTextWidth(
  STGFX_Handle_t            Handle,
  const STGFX_Font_t*       Font_p,
  U32                       FontSize,
  const ST_String_t         TextString, 
  const U16*                WTextString,
  U32*                      Width_p)
{
    ST_ErrorCode_t Err=ST_NO_ERROR;
    const STGFX_CharMapping_t* CharMapping_p;

    if(NULL == stgfx_HandleToUnit(Handle))
    {
      return(ST_ERROR_INVALID_HANDLE);
    }

    if (Font_p && (TextString || WTextString) && Width_p)
    {
        CharMapping_p = (Font_p->FontType & STGFX_USE_CHAR_MAPPING)
                            ? &Font_p->CharMapping : NULL;

        switch (Font_p->FontType & STGFX_FONT_TYPE_MASK)
        {
        case STGFX_BITMAPPED_FONT:
            if (Font_p->FontData.BitmappedFontData_p){
                Err =
                    GetBitmappedTextWidth(Font_p->FontData.BitmappedFontData_p,
                                          CharMapping_p, TextString,
                                          WTextString, Width_p);
            } else {
                Err=ST_ERROR_BAD_PARAMETER;
            }
            break;
        case STGFX_OUTLINE_FONT:
            if (Font_p->FontData.OutlineFontData_p && FontSize){
                Err= GetOutlineTextWidth(Handle,
                                         Font_p->FontData.OutlineFontData_p,
                                         CharMapping_p, TextString, WTextString,
                                         FontSize, Width_p);
            } else {
                Err=ST_ERROR_BAD_PARAMETER;
            }
            break;
        case STGFX_TXT_FONT:
            if (Font_p->FontData.TxtFontData_p){
                Err = GetTxtTextWidth(Font_p->FontData.TxtFontData_p,
                                      CharMapping_p, TextString, WTextString,
                                      FontSize, Width_p);
            } else {
                Err=ST_ERROR_BAD_PARAMETER;
            }
            break;
        default:
            Err=STGFX_ERROR_INVALID_FONT;
            break;
        }
    } else {
        Err=ST_ERROR_BAD_PARAMETER;
    }
    return(Err);
}

ST_ErrorCode_t stgfx_GetTextHeight(
  const STGFX_Font_t*       Font_p,
  U32                       FontSize,
  const ST_String_t         TextString,
  const U16*                WTextString,
  U32*                      Height_p)
{
  ST_ErrorCode_t Err=ST_NO_ERROR;
  const STGFX_CharMapping_t* CharMapping_p;

  if (Font_p && (TextString || WTextString) && Height_p)
  {
    CharMapping_p = (Font_p->FontType & STGFX_USE_CHAR_MAPPING)
                        ? &Font_p->CharMapping : NULL;

    switch (Font_p->FontType & STGFX_FONT_TYPE_MASK)
    {
    case STGFX_BITMAPPED_FONT:
      if (Font_p->FontData.BitmappedFontData_p){
        Err = GetBitmappedTextHeight(Font_p->FontData.BitmappedFontData_p,
                                     CharMapping_p, TextString, WTextString,
                                     Height_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_OUTLINE_FONT:
      if (Font_p->FontData.OutlineFontData_p && FontSize){
        Err= GetOutlineTextHeight(Font_p->FontData.OutlineFontData_p,
                                  CharMapping_p, TextString, WTextString,
                                  FontSize, Height_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    case STGFX_TXT_FONT:
      if (Font_p->FontData.TxtFontData_p){
        Err = GetTxtTextHeight(Font_p->FontData.TxtFontData_p, CharMapping_p,
                               TextString, WTextString, FontSize, Height_p);
      } else {
        Err=ST_ERROR_BAD_PARAMETER;
      }
      break;
    default:
      Err=STGFX_ERROR_INVALID_FONT;
      break;
    }
  } else {
    Err=ST_ERROR_BAD_PARAMETER;
  }
  return(Err);
}

