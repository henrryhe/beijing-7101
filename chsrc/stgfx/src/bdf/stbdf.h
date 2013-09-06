/*******************************************************************************
File name   : stbdf.h

Description : stgfx bdf font functions header file

COPYRIGHT (C) STMicroelectronics 2001.
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef STBDF_H
#define STBDF_H


/* Includes ----------------------------------------------------------------- */
#include "stgfx.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
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
);
                                 
ST_ErrorCode_t GetBitmappedFontAscent(
  const STGFX_BitmappedFont_t* Font_p,
  S16*                         Ascent_p
);

ST_ErrorCode_t GetBitmappedFontHeight(
  const STGFX_BitmappedFont_t*  Font_p, 
  U16*                          Height_p
);

ST_ErrorCode_t GetBitmappedFontWidth(
  const STGFX_BitmappedFont_t* Font_p,
  U16*                         Width_p
);

ST_ErrorCode_t GetBitmappedCharWidth(
  const STGFX_BitmappedFont_t*  Font_p,
  U16                           Character, 
  U16*                          Width_p
);

ST_ErrorCode_t GetBitmappedCharAscent(
  const STGFX_BitmappedFont_t*  Font_p,
  U16                           Character, 
  S16*                          Ascent_p
);

ST_ErrorCode_t GetBitmappedCharDescent(
  const STGFX_BitmappedFont_t*  Font_p,
  U16                           Character, 
  S16*                          Descent_p
);

ST_ErrorCode_t GetBitmappedPrintableChars(
  const STGFX_BitmappedFont_t*  Font_p,
  const STGFX_CharMapping_t*    CharMapping_p,
  const ST_String_t             TextString,
  const U16*                    WTextString, 
  U32                           AvailableLength,
  U32*                          NumOfChars_p
);

ST_ErrorCode_t GetBitmappedTextWidth(
  const STGFX_BitmappedFont_t*  Font_p,
  const STGFX_CharMapping_t*    CharMapping_p,
  const ST_String_t             TextString,
  const U16*                    WTextString, 
  U32*                          Width_p
);

ST_ErrorCode_t GetBitmappedTextHeight(
  const STGFX_BitmappedFont_t*  Font_p,
  const STGFX_CharMapping_t*    CharMapping_p,
  const ST_String_t             TextString,
  const U16*                    WTextString, 
  U32*                          Height_p
);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef STBDF_H */
