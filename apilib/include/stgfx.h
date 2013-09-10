/*******************************************************************************

File name   : stgfx.h

Description : STGFX module header file

COPYRIGHT (C) STMicroelectronics 2000/2001.

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STGFX_H
#define __STGFX_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stlite.h"
#include "stblit.h"
#include "stgxobj.h"
#include "sttbx.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STGFX_DRIVER_ID    0x80
#define STGFX_DRIVER_BASE  (STGFX_DRIVER_ID << 16)

enum
{
  STGFX_ERROR_INVALID_GC = STGFX_DRIVER_BASE,
  STGFX_ERROR_XYL_POOL_EXCEEDED,
  STGFX_ERROR_INVALID_FONT,
  /*STGFX_FILE_NOT_FOUND,*/
  /*STGFX_ERROR_OPENING_FONT_FILE,*/
  STGFX_ERROR_NOT_CHARACTER,
  /*STGFX_ERROR_INVALID_RECNUM,*/
  STGFX_ERROR_STEVT_DRV,
  STGFX_ERROR_STBLIT_DRV,
  STGFX_ERROR_STBLIT_BLIT,
  STGFX_ERROR_STGXOBJ,
  STGFX_ERROR_STAVMEM
};


typedef enum STGFX_CapStyle_e
{
  STGFX_RECTANGLE_CAP,
  STGFX_PROJECTING_CAP,
  STGFX_ROUND_CAP
} STGFX_CapStyle_t;

typedef enum STGFX_ClipMode_e
{
  STGFX_NO_CLIPPING,
  STGFX_INTERNAL_CLIPPING,
  STGFX_EXTERNAL_CLIPPING
} STGFX_ClipMode_t;

typedef enum STGFX_FontType_e
{
  STGFX_BITMAPPED_FONT,
  STGFX_OUTLINE_FONT,
  STGFX_TXT_FONT,
  STGFX_USE_CHAR_MAPPING = 256
} STGFX_FontType_t;

typedef enum STGFX_JoinStyle_e
{
  STGFX_RECTANGLE_JOIN,
  STGFX_FLAT_JOIN,
  STGFX_ROUNDED_JOIN
} STGFX_JoinStyle_t;

typedef enum STGFX_PolygonFillMode_e
{
  STGFX_WINDING_FILL,
  STGFX_EVENODD_FILL
} STGFX_PolygonFillMode_t;

typedef enum STGFX_SectorMode_e
{
  STGFX_CHORD_MODE,
  STGFX_RADIUS_MODE
} STGFX_SectorMode_t;

typedef enum STGFX_ShadowDirection_e
{
  STGFX_NORTH,
  STGFX_SOUTH,
  STGFX_EAST,
  STGFX_WEST,
  STGFX_NORTH_EAST,
  STGFX_SOUTH_EAST,
  STGFX_SOUTH_WEST,
  STGFX_NORTH_WEST,
  STGFX_NORTH_SOUTH,
  STGFX_EAST_WEST,
  STGFX_NORTH_SOUTH_EAST_WEST
} STGFX_ShadowDirection_t;

typedef enum STGFX_TxtFontEffect_e
{
  STGFX_NO_EFFECT,
  STGFX_FRINGING,
  STGFX_ROUNDING,
  STGFX_ROUNDING_FRINGING,
  STGFX_SHADOW
} STGFX_TxtFontEffect_t;

/* Exported Types ----------------------------------------------------------- */

typedef struct STGFX_Capability_s
{
  U8                        MaxNumPriorities;
  BOOL                      Blocking;
  BOOL                      ColorModeConversion;
  U8                        MaxNumHandles;
  U32                       MaxLineWidth;
  U32                       MaxNumPolyPoints;
} STGFX_Capability_t;

typedef struct STGFX_TxtFont_s
{
  U8            NbColor;
  U8*           IsForeground_p;
  U16           FontWidth;
  U16           FontHeight;
  U16           NumOfChars;
  U8*           GlyphData_p;
} STGFX_TxtFont_t;

typedef struct STGFX_TxtFontAttributes_s
{
  U8*                       Color_p;
  STGFX_TxtFontEffect_t     Effect;
  U8                        ZoomWidth;
  U8                        ZoomHeight;
  U8                        UnderlineHeight;
  U8                        UnderlineColor;
  U8                        OverlineHeight;
  U8                        OverlineColor;
  U8                        FringeColor;
  U8                        RoundingColor;
  U8                        ShadowWidth;
  U8                        ShadowColor;
  STGFX_ShadowDirection_t   ShadowDirection;
} STGFX_TxtFontAttributes_t;

typedef struct STGFX_BitmappedGlyph_s
{
  U8   GlyphIndex;
  U16  BoundingBoxW;
  U16  BoundingBoxH;
  S16  BoundingBoxX;
  S16  BoundingBoxY;
  U16  Width;
  U16  Height;
  U8*  GlyphData_p;
} STGFX_BitmappedGlyph_t;

typedef struct STGFX_BitmappedFont_s
{
  S16                       FontAscent;
  S16                       FontDescent;
  U16                       DefaultChar;
  U16                       NumOfGlyphs;
  STGFX_BitmappedGlyph_t*   Glyph_p;
} STGFX_BitmappedFont_t;

typedef struct STGFX_OutlineGlyph_s
{
  U32                       GlyphOffset;
  S16                       Ascent;
  S16                       Descent;
  S16                       BoundingBoxX;
  U16                       Width;
} STGFX_OutlineGlyph_t;

typedef struct STGFX_OutlineFont_s
{
  U32                       GlyphSectionSize;
  U8*                       GlyphSection_p;
  U16                       OutlineResolution;
  S16                       Ascent;
  S16                       Descent;
  S16                       Baseline;
  U16                       NumCharacters;
  STGFX_OutlineGlyph_t*     Glyph_p;
} STGFX_OutlineFont_t;

typedef union STGFX_FontData_u
{
  STGFX_BitmappedFont_t*    BitmappedFontData_p;
  STGFX_OutlineFont_t*      OutlineFontData_p;
  STGFX_TxtFont_t*          TxtFontData_p;
} STGFX_FontData_t;

typedef struct STGFX_CharMapEntry_s
{
  U16                       Code;
  U16                       Idx;
} STGFX_CharMapEntry_t;

typedef struct STGFX_CharMapping_s
{
  STGFX_CharMapEntry_t*     Entries_p;
  U16                       NumEntries;
} STGFX_CharMapping_t;

typedef struct STGFX_Font_s
{
  STGFX_FontType_t          FontType;
  STGFX_FontData_t          FontData;
  STGFX_CharMapping_t       CharMapping;
} STGFX_Font_t;

typedef struct STGFX_GC_s
{
  STBLIT_AluMode_t           AluMode;
  STGFX_CapStyle_t           CapStyle;
  STGFX_ClipMode_t           ClipMode;
  STGXOBJ_Rectangle_t        ClipRectangle;
  STGXOBJ_ColorKey_t         ColorKey;
  STBLIT_ColorKeyCopyMode_t  ColorKeyCopyMode;
  STGXOBJ_Color_t            DrawColor;
  BOOL                       EnableFilling;
  STGXOBJ_Color_t            FillColor;
  STGXOBJ_Bitmap_t*          DrawTexture_p;
  STGXOBJ_Bitmap_t*          FillTexture_p;
  STGFX_Font_t*              Font_p;
  U16                        FontSize;
  U8                         GlobalAlpha;
  STGFX_JoinStyle_t          JoinStyle;
  U8*                        LineStyle_p;
  U16                        LineWidth;
  STGFX_PolygonFillMode_t    PolygonFillMode;
  U8                         Priority;
  STGXOBJ_Color_t            TextForegroundColor;
  STGXOBJ_Color_t            TextBackgroundColor;
  STGFX_TxtFontAttributes_t* TxtFontAttributes_p;
  BOOL                       UseTextBackground;
  U16                        XAspectRatio;
  U16                        YAspectRatio;
  STGXOBJ_Palette_t*         Palette_p;
} STGFX_GC_t;

typedef U32                 STGFX_Handle_t;

typedef struct STGFX_InitParams_s
{
  U32                       NumBitsAccuracy;
  ST_DeviceName_t           BlitName;
  ST_Partition_t*           CPUPartition_p;
  STAVMEM_PartitionHandle_t AVMemPartitionHandle;
  ST_DeviceName_t           EventHandlerName;
} STGFX_InitParams_t;

typedef struct STGFX_TermParams_s
{
  BOOL                      ForceTerminate;
} STGFX_TermParams_t;

typedef struct STGFX_Point_s
{
  S32                       X;
  S32                       Y;
} STGFX_Point_t;

/* Exported Variables ------------------------------------------------------- */
/* NONE */

/* Exported Macros ---------------------------------------------------------- */
/* NONE */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t STGFX_Close(STGFX_Handle_t Handle);

ST_ErrorCode_t STGFX_Open(
  const ST_DeviceName_t     DeviceName,
  STGFX_Handle_t*           Handle_p
);

ST_Revision_t STGFX_GetRevision(void);

ST_ErrorCode_t STGFX_GetCapability(
  const ST_DeviceName_t       DeviceName,
  STGFX_Capability_t * const  Capability_p
);

ST_ErrorCode_t STGFX_Init(
  const ST_DeviceName_t      DeviceName,
  const STGFX_InitParams_t*  Params_p
);


ST_ErrorCode_t STGFX_CopyArea(
  STGFX_Handle_t                    Handle,
  const STGXOBJ_Bitmap_t*           SrcBitmap_p,
  STGXOBJ_Bitmap_t*                 DstBitmap_p,
  const STGFX_GC_t*                 GC_p,
  const STGXOBJ_Rectangle_t*        SrcRectangle_p,
  S32                               DstPositionX,
  S32                               DstPositionY
);

#if 1
ST_ErrorCode_t STGFX_CopyAreaA(
  STGFX_Handle_t                    Handle,
  const STGXOBJ_Bitmap_t*           SrcBitmap_p,
  STGXOBJ_Bitmap_t*                 DstBitmap_p,
  const STGFX_GC_t*                 GC_p,
  const STGXOBJ_Rectangle_t*        SrcRectangle_p,
  const STGXOBJ_Rectangle_t*        DstRectangle_p
);
#endif


ST_ErrorCode_t STGFX_SetGCDefaultValues(
  const STGXOBJ_Color_t             DrawColor,
  const STGXOBJ_Color_t             FillColor,
  STGFX_GC_t*                       GC_p
);


ST_ErrorCode_t STGFX_DrawArc(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       XC,
  S32                       YC,
  U32                       HRadius,
  U32                       VRadius,
  S32                       StartAngle,
  U32                       ArcAngle,
  S32                       Orientation
);


ST_ErrorCode_t STGFX_DrawDot(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       X1,
  S32                       Y1
);


ST_ErrorCode_t STGFX_DrawEllipse(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       XC,
  S32                       YC,
  U32                       HRadius,
  U32                       VRadius,
  S32                       Orientation
);


ST_ErrorCode_t STGFX_DrawLine(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       X1,
  S32                       Y1,
  S32                       X2,
  S32                       Y2
);

ST_ErrorCode_t STGFX_DrawPolygon(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  U32                       NumPoints,
  STGFX_Point_t*            Points_p
);


ST_ErrorCode_t STGFX_DrawPolyline(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  U32                       NumPoints,
  STGFX_Point_t*            Points_p
);

ST_ErrorCode_t STGFX_DrawRectangle(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  STGXOBJ_Rectangle_t       Rectangle,
  S32                       Orientation
);

ST_ErrorCode_t STGFX_DrawRoundRectangle(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  STGXOBJ_Rectangle_t       Rectangle,
  U32                       HRadius,
  U32                       VRadius,
  S32                       Orientation
);

ST_ErrorCode_t STGFX_DrawSector(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       XC,
  S32                       YC,
  U32                       HRadius,
  U32                       VRadius,
  S32                       StartAngle,
  U32                       ArcAngle,
  S32                       Orientation,
  STGFX_SectorMode_t        SectorMode
);

#define STGFX_DrawText(Handle, Bitmap_p, GC_p, \
                       String, XPos, YPos, NumOfChars) \
  stgfx_DrawText(Handle, Bitmap_p, GC_p, String, NULL, XPos, YPos, NumOfChars)
  
#define STGFX_DrawWText(Handle, Bitmap_p, GC_p, \
                        String, XPos, YPos, NumOfChars) \
  stgfx_DrawText(Handle, Bitmap_p, GC_p, NULL, String, XPos, YPos, NumOfChars)

ST_ErrorCode_t STGFX_FloodFill(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       XSeed,
  S32                       YSeed,
  const STGXOBJ_Color_t*    ContourColor_p
);

ST_ErrorCode_t STGFX_GetCapability(
  const ST_DeviceName_t     DeviceName,
  STGFX_Capability_t*       Capability_p
);

#define STGFX_GetCharAscent(Font_p, FontSize, Character, Ascent_p) \
  STGFX_GetWCharAscent(Font_p, FontSize, (U16)(U8)Character, Ascent_p)
  /* U8 first to be sure we don't sign-extend if they pass in a char */
  
ST_ErrorCode_t STGFX_GetWCharAscent(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  U16                       Character,
  S16*                      Ascent_p
);

#define STGFX_GetCharDescent(Font_p, FontSize, Character, Descent_p) \
  STGFX_GetWCharDescent(Font_p, FontSize, (U16)(U8)Character, Descent_p)

ST_ErrorCode_t STGFX_GetWCharDescent(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  U16                       Character,
  S16*                      Descent_p
);

#define STGFX_GetCharWidth(Font_p, FontSize, Character, Width_p) \
  STGFX_GetWCharWidth(Font_p, FontSize, (U16)(U8)Character, Width_p)

ST_ErrorCode_t STGFX_GetWCharWidth(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  U16                       Character,
  U16*                      Width_p
);

ST_ErrorCode_t STGFX_GetFontAscent(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  S16*                      Ascent_p
);

ST_ErrorCode_t STGFX_GetFontHeight(
  const STGFX_Font_t*       Font_p,
  U16                       FontSize,
  U16*                      Height_p
);

#define STGFX_GetPrintableChars(Font_p, FontSize, TextString, \
                                    AvailableLength, NumOfChars_p) \
  stgfx_GetPrintableChars(Font_p, FontSize, TextString, NULL, \
                          AvailableLength, NumOfChars_p)

#define STGFX_GetPrintableWChars(Font_p, FontSize, TextString, \
                                    AvailableLength, NumOfChars_p) \
  stgfx_GetPrintableChars(Font_p, FontSize, NULL, TextString, \
                          AvailableLength, NumOfChars_p)

#define STGFX_GetTextWidth(Handle, Font_p, FontSize, TextString, Width_p) \
  stgfx_GetTextWidth(Handle, Font_p, FontSize, TextString, NULL, Width_p)

#define STGFX_GetWTextWidth(Handle, Font_p, FontSize, TextString, Width_p) \
  stgfx_GetTextWidth(Handle, Font_p, FontSize, NULL, TextString, Width_p)

#define STGFX_GetTextHeight(Font_p, FontSize, TextString, Height_p) \
  stgfx_GetTextHeight(Font_p, FontSize, TextString, NULL, Height_p)

#define STGFX_GetWTextHeight(Font_p, FontSize, TextString, Height_p) \
  stgfx_GetTextHeight(Font_p, FontSize, NULL, TextString, Height_p)

ST_ErrorCode_t STGFX_ReadPixel(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  S32                       X,
  S32                       Y,
  STGXOBJ_Color_t*          PixelValue_p
);

ST_ErrorCode_t STGFX_Sync(STGFX_Handle_t Handle);

ST_ErrorCode_t STGFX_Term(
  const ST_DeviceName_t           DeviceName,
  const STGFX_TermParams_t *const TermParams_p
);

ST_ErrorCode_t STGFX_Clear(
    STGFX_Handle_t    Handle,
    STGXOBJ_Bitmap_t* Bitmap_p,
    STGXOBJ_Color_t*  Color
);

/**** internal functions - do not use directly ****/

/* WString argument applies iff String == NULL */

ST_ErrorCode_t stgfx_DrawText(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  const ST_String_t         String,
  const U16*                WString,
  S32                       XPos,
  S32                       YPos,
  U32                       NumOfChars
);

ST_ErrorCode_t stgfx_GetPrintableChars(
  const STGFX_Font_t*       Font_p,
  U32                       FontSize,
  const ST_String_t         TextString,
  const U16*                WTextString,
  U32                       AvailableLength,
  U32*                      NumOfChars_p
);

ST_ErrorCode_t stgfx_GetTextWidth(
  STGFX_Handle_t            Handle,
  const STGFX_Font_t*       Font_p,
  U32                       FontSize,
  const ST_String_t         TextString,
  const U16*                WTextString,
  U32*                      Width_p
);

ST_ErrorCode_t stgfx_GetTextHeight(
  const STGFX_Font_t*       Font_p,
  U32                       FontSize,
  const ST_String_t         TextString,
  const U16*                WTextString,
  U32*                      Height_p
);


#ifdef STGFX_TIME_OPS
/* begin recording at internal time points */
void STGFX_StartTiming(void);

/* dump currently stored timings, and either stop recording or start again */
void STGFX_DumpTimings(BOOL Restart);
#else
#define STGFX_StartTiming()
#define STGFX_DumpTimings(Restart)
#endif

#ifdef STGFX_COUNT_AVMEM
/* counts provided by STGFX internals */
extern U32 STGFX_AvmemAllocCount;
extern U32 STGFX_AvmemFreeCount;
extern U32 STGFX_AvmemFreeErrorCount;
 /* errors occur when we try to free the same block freed twice, for example */
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STGFX_H */

/* End of stgfx.h */

