/*
    File         :  sttxt.h
    Version      :  1.00.00
    Author       :  Gianluca Carcassi
    Date         :  17.01.2001
    Description  :  This file is part of the library STTXT

    Date        Modification        Name
    ----        ------------        ----
    17.01.2001  Creation            Gianluca Carcassi

*/

/* Define to prevent recursive inclusion */
#ifndef __STTXT_H
#define __STTXT_H

/* Includes -------------------------------------------------------- */
#include "stgfx.h"


/* Exported Constants ---------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SUPPORTED_CHAR  255

/* Exported Types -------------------------------------------------- */

/* Exported Variables ---------------------------------------------- */

/* Exported Macros ------------------------------------------------- */

/* Exported Functions ---------------------------------------------- */

ST_ErrorCode_t DrawTxtText  (STBLIT_BlitContext_t*, STGXOBJ_Bitmap_t*,
                             const STGFX_GC_t*, STGFX_Handle_t,
                             const ST_String_t, const U16*, S32, S32, U32);
                             
/* --- Metrics functions (file: sttxt_metrics.c) --- */
ST_ErrorCode_t GetTxtFontAscent (const STGFX_TxtFont_t*, U8, S16*);
ST_ErrorCode_t GetTxtFontHeight (const STGFX_TxtFont_t*, U8, U16*);
ST_ErrorCode_t GetTxtCharWidth  (const STGFX_TxtFont_t*, U8, U16, U16*);
ST_ErrorCode_t GetTxtCharAscent (const STGFX_TxtFont_t*, U8, U16, S16*);
ST_ErrorCode_t GetTxtCharDescent(const STGFX_TxtFont_t*, U8, U16, S16*);
ST_ErrorCode_t GetTxtPrintableChars(const STGFX_TxtFont_t*,
                                    const STGFX_CharMapping_t*,
                                    const ST_String_t, const U16*,
                                    U8, U32, U32*);
ST_ErrorCode_t GetTxtTextWidth  (const STGFX_TxtFont_t*,
                                 const STGFX_CharMapping_t*,
                                 const ST_String_t, const U16*, U8, U32*);
ST_ErrorCode_t GetTxtTextHeight (const STGFX_TxtFont_t*,
                                 const STGFX_CharMapping_t*,
                                 const ST_String_t, const U16*, U8, U32*);


/* Reserved Functions ---------------------------------------------- */

/* --- Effects functions (file: sttxt_effects.c) --- */
void sttxt_EffectFringing(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectRounding(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowN (STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowS (STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowE (STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowW (STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowNE(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowNW(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowSE(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowSW(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowNS(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowEW(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);
void sttxt_EffectShadowNSEW(STGXOBJ_Bitmap_t*, STGFX_TxtFontAttributes_t*);

/* --- Omega1 functions (file: sttxt_omega1.c) --- */
/*
ST_ErrorCode_t sttxt_PutBitmap(
    STGXOBJ_Bitmap_t*     SrcBitmap_p,
    STGXOBJ_Bitmap_t*     DstBitmap_p,
    S32                   DstPosX,
    S32                   DstPosY,
    STGXOBJ_Rectangle_t   BCClipRect
);
*/
#ifdef __cplusplus
}
#endif
#endif /* __STTXT_H */

/* end of sttxt.h */

