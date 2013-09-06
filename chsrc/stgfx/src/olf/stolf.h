#ifndef STOLF_H
#define STOLF_H

#include "stddefs.h"
#include "stgfx.h"
#include "stgxobj.h"

ST_ErrorCode_t GetOutlinePrintableChars(const STGFX_OutlineFont_t*,
                                        const STGFX_CharMapping_t*,
                                        const ST_String_t,const U16*,
                                        U16,U32,U32*);
ST_ErrorCode_t GetOutlineTextWidth(STGFX_Handle_t Handle,
                                   const STGFX_OutlineFont_t* Font_p,
                                   const STGFX_CharMapping_t* CharMapping_p,
                                   const ST_String_t TextString,
                                   const U16* WTextString,
                                   U16  FontSize,
                                   U32* Width_p);
  
ST_ErrorCode_t GetOutlineTextHeight(const STGFX_OutlineFont_t*,
                                    const STGFX_CharMapping_t*,
                                    const ST_String_t,const U16*,U16,U32*);

ST_ErrorCode_t DrawOutlineText(STBLIT_BlitContext_t*,const ST_String_t,
                               const U16*,S32,S32,STGFX_Handle_t,U32,
                               STGXOBJ_Bitmap_t*,const STGFX_GC_t*);

ST_ErrorCode_t GetOutlineCharAscent(U16,U16,STGFX_OutlineFont_t*,S16*);

ST_ErrorCode_t GetOutlineCharDescent(U16,U16,STGFX_OutlineFont_t*,S16*);

ST_ErrorCode_t GetOutlineCharWidth(U16,U16,STGFX_OutlineFont_t*,U16*); 

ST_ErrorCode_t GetOutlineFontAscent(U16,STGFX_OutlineFont_t*,S16*);

ST_ErrorCode_t GetOutlineFontHeight(U16,STGFX_OutlineFont_t*,U16*);

#endif

