#ifndef GLYPH_H
#define GLYPH_H

#include "stddefs.h"
#include "stgfx.h"

#define MAX_OLF_SIZE     128
#define MAX_OLF_SCANPTS   32

/* for stolf.c: */
ST_ErrorCode_t DrawGlyph(
  STBLIT_BlitContext_t* BlitContext_p,
  U16                   Idx,
  S32                   XScreen,
  S32                   YScreen, 
  STGFX_Handle_t        Handle,
  STGXOBJ_Bitmap_t*     Bmp_p, 
  const STGFX_GC_t*     GC_p,
  S32*                  XMax_p
);

/* for loadpfr.c (in test harness): */
ST_ErrorCode_t GetGlyphPoints(U8**,STGFX_Point_t*,S16*,S16*,U8*,
                              STGFX_Point_t*,BOOL*);  
ST_ErrorCode_t GetControlledCoordinates(U8** Src_p, U8* NumXOrus_p, 
                                        S16* OrusData_p);
void GetSimpleElementParams(U8** Src_p, S16* XScale_p,
                            S16* YScale_p, S16* XGlyph_p,
                            S16* YGlyph_p, S32* Offset_p);
                            
#endif
