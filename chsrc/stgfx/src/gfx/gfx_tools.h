/*******************************************************************************

File name   : gfx_tools.h

Description : stgfx tools header file, covering gfx_common.c, gfx_tools.c,
              gfx_conic.c and gfx_texture.c. For simplicity does not test
              STGFX_INCLUDE macros, unbuilt functions will simply be unfound
              at link time

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                  Name
----               ------------                                  ----
2000-10-10         Created                                       Adriano Melis
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __GFX_TOOLS_H
#define __GFX_TOOLS_H


/* Includes ----------------------------------------------------------------- */
#include <math.h>
#include "stgfx_init.h" /* includes stgfx.h, ... */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* reserve least significant byte of STGFX_FontType_t for basic font type
  (excluding flags like STGFX_USE_CHAR_MAPPING) */
#define STGFX_FONT_TYPE_MASK 255

#define TILE_COLOR_IDX 255
#define MAX_SCREEN_HEIGTH 600
#define INCREASE_XYL_SIZE_CAP 3
#define YRES 570
#define PI         11520 /* 180 degrees * 64: PI in GFX units */
#define PI_FLOAT   (3.14159265359)
#define FRACT_TO_UINT(a) (((a)+(1<<(STGFX_INTERNAL_ACCURACY-1))) >>  \
                         STGFX_INTERNAL_ACCURACY)
#define FRACT_TO_INT(a) ((a>=0)? FRACT_TO_UINT(a) : (-FRACT_TO_UINT(-a)))

#define GFXSWAP(a,b)     {a^=b; b^=a; a^=b;}
#define GFXSGN(a)        ((a < 0) ? -1 : 1)
#define GFXABS(a)        (((a) < 0) ? (-(a)) : (a))
#define GFXMIN(a,b)      ((a)<(b) ? (a) : (b))
#define GFXMAX(a,b)      ((a)<(b) ? (b) : (a))
#define GFXCROSS(a,b,x)  ( (x>=a && x<=b) ? TRUE : FALSE)
#define IS_ODD(a) (a & 0x1) ? TRUE : FALSE /*  if(LineWidth & 0x1)  
                                               line width is odd */
#define TO_RAD(alpha) ((PI_FLOAT * (double)alpha)/ (double)PI)


/* Exported Types ----------------------------------------------------------- */
typedef struct
{
  STBLIT_XYL_t*   xylData;   /* xyl array of the segment */
  U32             length;    /* length of the segment */
  S8              Up;        /* to understand winding:
                                 +1 y increases, -1 y decreases */
  U8              Spin;      /* to catch vertices */
} stgfx_Side_t;

typedef enum
{
  Close,
  Open
} stgfx_PolyMode;

typedef enum
{
  ArcLine,
  LineLine,
  Lines
} stgfx_JoinSector;

/* ConicStatus is used to maintain the status
  of Conic() between segments of a dashed arc */
typedef struct 
{
  float x;
  float y;
  float d;
  float u;
  float v;
  float k1;
  float k2;
  float k3;
  S32   octant;         /* octant in which next Conic() call will start (1-8) */
  S32   dxsquare;
  S32   dysquare;
  S32   dxdiag;
  S32   dydiag;
  S32   i;              /* total number of xyl written (not used) */
  S32   Counter;        /* Total dash & gap segments emitted so far */
  S32   Length;         /* Number of pixels for this dash segment (from LineStyle) */
  S32   LastOctant;
  float XS;             /* start point of next segment */
  float YS;
  float XSLastOctant;   /* S on final entry to last octant (not used) */
  float YSLastOctant;
  float XE;             /* initially the final endpoint of the overall arc; */
  float YE;             /*   updated to the endpoint of each segment */
  float X0;             /* final endpoint of inner arc; used to terminate */
  float Y0;             /*   final dash segment */
  float EdSdx;          /* normal direction at final inner arc endpoint; used */
  float EdSdy;          /*   to end outer arc (why different from standard case?) */
  U32   AvailableSize;  /* Total NumXYL available */
  BOOL  IsInnerArc;
  BOOL  IsEllipse;
  BOOL  IsFirstOctant;  /* TRUE the first time we enter the first octant
                            (we enter it twice for an ellipse) */
  BOOL  IsCompleted;    /* TRUE terminates dash drawing loop,
                            set when we have finished in the final octant */
  BOOL  IsTheVeryLast;  /* TRUE when we enter LastOctant for the last time
                            (we enter it twice for an ellipse) */
  U32   TotalOctantCount; /* First Conic() call stores overall number of times
                            we will need to enter a new octant here */
} ConicStatus_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void CheckAngle(S32* Angle_p);

ST_ErrorCode_t stgfx_2dRasterSimpleLine(
   S32           X1,
   S32           Y1,
   S32           X2,
   S32           Y2,
   U32*          NumXYL,
   STBLIT_XYL_t* XYLArray
);

ST_ErrorCode_t stgfx_2dRasterDashedLine(
   STGFX_Handle_t Handle,
   S32            X1,
   S32            Y1,
   S32            X2,
   S32            Y2,
   U8*            LineStyle,
   U16            LineWidth,
   U32*           NumXYL,
   STBLIT_XYL_t*  XYLArray
);

ST_ErrorCode_t stgfx_2dRasterThickLine(
   STGFX_Handle_t Handle,
   S32            X1,
   S32            Y1,
   S32            X2,
   S32            Y2,
   U16            LineWidth,
   U32*           NumXYL,
   STBLIT_XYL_t*  XYLArray
);

ST_ErrorCode_t stgfx_2dLineRasterCap(
  STGFX_Handle_t   Handle,
  S32              Xcap,
  S32              Ycap,
  S32              X,
  S32              Y,
  STGFX_CapStyle_t CapStyle,
  U16              LineWidth,  
  U32*             NumXYL,
  STBLIT_XYL_t*    XYLArray
);

ST_ErrorCode_t stgfx_2dArcRasterCap(
  STGFX_Handle_t   Handle,
  S32              Xcap,
  S32              Ycap,
  S32              X,
  S32              Y,
  STGFX_CapStyle_t CapStyle,
  U16              LineWidth,  
  U32*             NumXYL,
  STBLIT_XYL_t*    XYLArray,
  BOOL             IsStart
);

ST_ErrorCode_t stgfx_2dRasterJoin(
    STGFX_Handle_t         Handle,
    S32                    X1,
    S32                    Y1,
    S32                    Xjoin,
    S32                    Yjoin,
    S32                    X3,
    S32                    Y3,
    S32                    XintArc,
    S32                    YintArc,
    S32                    XextArc,
    S32                    YextArc,
    stgfx_JoinSector       JoinSector,
    U16                    LineWidth,
    STGFX_JoinStyle_t      JoinStyle,
    U8*                    LineStyle,
    STAVMEM_BlockHandle_t* JoinBlockHandleH_p,
    STBLIT_XYL_t**         XYL_p_p,
    U32*                   NumXYL
);

ST_ErrorCode_t stgfx_FillGenericPolygon(
  U32                     BmHeight,
  S32                     YTop,
  S32                     YBot,
  stgfx_Side_t*           Sides_p,         /* sides data */
  U32                     SidesNum,        /* number of sides */
  STBLIT_XYL_t*           XYLFillData,     /* filling XYL data */
  U32*                    XYLLength,
  STGFX_PolygonFillMode_t PolygonFillMode  /* filling mode */
);

ST_ErrorCode_t stgfx_DrawPoly(
  STGFX_Handle_t    Handle,
  STGXOBJ_Bitmap_t* Bitmap_p,
  const STGFX_GC_t* GC_p,
  U32               NumPoints,
  STGFX_Point_t*    Points_p,
  stgfx_PolyMode    PolyMode
);

ST_ErrorCode_t stgfx_FillSimplePolygon(
  STGFX_Handle_t Handle,
  U32            NumPoints,
  STGFX_Point_t* Points_p,
  U32*           NumXYL,
  STBLIT_XYL_t*  XYLArray
);

void stgfx_RemoveXYLOverlaps(
   STBLIT_XYL_t* XYLArray1,
   U32*          NumXYL1,
   STBLIT_XYL_t* XYLArray2,
   U32*          NumXYL2
);

void stgfx_RemoveZeroXYL(
   STBLIT_XYL_t* XYLArray,
   U32*          NumXYL
);

ST_ErrorCode_t stgfx_2dRasterSimpleEllipse( 
   U32 BHeight, S32 XC, S32 YC, 
   U32 HRadius, U32 VRadius,
   S32 Orientation,
   BOOL Fill,
   U32* NumXYL, 
   STBLIT_XYL_t*  XYLArray_p,
   U32* SizeFillArray,
   STBLIT_XYL_t* FillArrayXYL_p
);

ST_ErrorCode_t stgfx_2dRasterSimpleArc( 
  S32 XC, S32 YC, U32 HRadius, 
  U32 VRadius,
  S32 StartAngle, U32 ArcAngle, 
  S32 Orientation,
  U32* NumXYL, STBLIT_XYL_t*  XYLArray_p,
  S32* Points_p
);

ST_ErrorCode_t stgfx_2dRasterThickArc(  
  U32 BHeight, S32 XC, S32 YC, 
  U32 HRadius, 
  U32 VRadius,
  S32 StartAngle, U32 ArcAngle, 
  S32 Orientation,
  U16 LineWidth,
  U32* SizeInner, 
  STBLIT_XYL_t*  InnerArray_p,
  U32* SizeOuter, 
  STBLIT_XYL_t*  OuterArray_p,
  U32* SizeThicknessArray,
  STBLIT_XYL_t*  ThicknessArray_p,
  U32* SizeSELineArray,
  STBLIT_XYL_t*  SELineArray_p,
  U32* SizeEELineArray,
  STBLIT_XYL_t*  EELineArray_p,
  S32* Points_p,
  S32* GeometricPoints_p
);

ST_ErrorCode_t stgfx_2dRasterDashedArc( 
  U32 BHeight, S32 XC, S32 YC, 
  U32 HRadius, U32 VRadius, 
  S32 StartAngle, U32 ArcAngle, 
  S32 Orientation,
  U8* LineStyle,
  U16 LineWidth,
  U32* SizeInner, 
  STBLIT_XYL_t*  InnerArray_p,
  U32* SizeOuter, 
  STBLIT_XYL_t*  OuterArray_p,
  U32* SizeThicknessArray,
  STBLIT_XYL_t*  ThicknessArray_p,
  U32* SizeSELineArray,
  STBLIT_XYL_t*  SELineArray_p,
  U32* SizeEELineArray,
  STBLIT_XYL_t*  EELineArray_p,
  S32* Points_p,
  S32* GeometricPoints_p
);

void stgfx_PrepareMaskBitmap(
    STGXOBJ_Bitmap_t* TargetBitmap_p,
    STGXOBJ_Bitmap_t* MaskBitmap_p
);

ST_ErrorCode_t stgfx_AVMEM_FreeAll(
    STGFX_Handle_t         Handle,
    U32                    NumAVMEMAllocation,
    STAVMEM_BlockHandle_t* AVMXYLAllocated
);

ST_ErrorCode_t stgfx_CalcLimitDashedLine(
    S32            X1,
    S32            Y1,
    S32            X2,
    S32            Y2,
    U8*            LineStyle,
    U16            LineWidth,
    U32*           NumXYL
);

void FindPointOnNormal(
    S32* InP, 
    S32* OutP, 
    U32  R
);

void CalcyTopyBot(
    U32  NumPoints, 
    S32* yTop, 
    S32* yBot, 
    S32* YY
);

void CalcXYLArray(
    S32           yTop, 
    S32           yBot, 
    U32           Size, 
    STBLIT_XYL_t* tmpXYLArray, 
    STBLIT_XYL_t* XYLArray
);

ST_ErrorCode_t Conic( float XP, float YP, float XS, float YS, 
                      float XE, float YE, float A, float B, 
                      float C,float  D, float E, float F,
                      float XC, float YC, float XD, float YD,
                      U32* NumXYL, STBLIT_XYL_t*  XYLArray_p,
                      BOOL RecStatus, ConicStatus_t* Status,
                      BOOL IsJoin );
                      
void CalcConicCoefficients( float LRadius, float SRadius, 
                            S32 Orientation,
                            float*  A, float*  B, float*  C, float*  D,
                            float*  E, float*  F, float*  XP, float*  YP,
                            float*  XD, float*  YD );
                            
void SplitArrayXYL( STBLIT_XYL_t* XYLArray, U32 Size, 
                    U32* PosMin, U32* PosMax,
                    BOOL IsEllipse,
                    stgfx_Side_t* Sides_p,
                    U32* TotSides );
                    
void ChooseThePoint( S32* Points, S32* PointsOnNormal,
                     S32* XJ, S32* YJ, BOOL IsStart );

ST_ErrorCode_t stgfx_InitBlitContext(STGFX_Handle_t Handle,
                                     STBLIT_BlitContext_t * BlitContext_p,
                                     const STGFX_GC_t * GC_p,
                                     const STGXOBJ_Bitmap_t * Bitmap_p);
                                     
void DumpXYL(STBLIT_XYL_t* XYL_p, U32 NumXYL, char* Prefix_p);


ST_ErrorCode_t stgfx_PrepareWorkBuffer(STGFX_Handle_t Handle,
    U32 MaxWidth, U32 MaxHeight, STGXOBJ_ColorType_t ColorType,
    void** WorkBuffer_p_p, STAVMEM_BlockHandle_t* BlockHandle_p);

ST_ErrorCode_t stgfx_InitTexture(STGFX_Handle_t Handle, const STGFX_GC_t* GC_p,
    STGXOBJ_Bitmap_t* TargetBitmap_p, STGXOBJ_Bitmap_t* MaskBitmap_p,
    void** WorkBuffer_p_p, STAVMEM_BlockHandle_t* WorkBlockHandle_p);

ST_ErrorCode_t stgfx_PrepareTexture(STGFX_Handle_t Handle,
    STGXOBJ_Bitmap_t* MaskBitmap_p, STAVMEM_BlockHandle_t* BlockHandle_p,
    STBLIT_BlitContext_t* BlitContext_p, STGXOBJ_Color_t* Color_p);

ST_ErrorCode_t stgfx_ApplyTexture(STGFX_Handle_t Handle, U32 OriginX,
    U32 OriginY, const STGFX_GC_t* GC_p, STGXOBJ_Bitmap_t* TileBitmap_p,
    STGXOBJ_Bitmap_t* MaskBitmap_p, STAVMEM_BlockHandle_t* MaskBlockHandle_p,
    STGXOBJ_Bitmap_t* TargetBitmap_p, STBLIT_BlitContext_t* BlitContext_p);

ST_ErrorCode_t stgfx_AlphaExtendColor(const STGXOBJ_Color_t* Src_p,
                                      STGXOBJ_Color_t* Dst_p);

BOOL stgfx_AreColorsEqual(const STGXOBJ_Color_t* ColA_p,
                          const STGXOBJ_Color_t* ColB_p);

S32 QuickSin(S32 x);
S32 QuickCos(S32 x);
S32 QuickTan(S32 x);
S32 QuickCot(S32 x);

void stgfx_CalcAlignedCurve(U16* Array_p, U16 HRadius, U16 VRadius);
ST_ErrorCode_t stgfx_CalcAlignedSector(STGFX_Handle_t Handle, BOOL Filled,
                                       S32 XC, S32 YC,
                                       U16 HRadius, U16 VRadius,
                                       S32 StartAngle, U32 ArcAngle,
                                       STBLIT_XYL_t* XYLArray, U32* NumXYL);

ST_ErrorCode_t stgfx_2dRasterFastSimpleEllipse( 
    U32 BHeight, 
    S32 XC, 
    S32 YC, 
    U32 XRadius, 
    U32 YRadius,
    BOOL Fill,
    U32* LengthXYL, 
    STBLIT_XYL_t* XYLArray_p,
    U32* SizeFillArray, 
    STBLIT_XYL_t* FillArray_p 
);

ST_ErrorCode_t stgfx_CharCodeToIdx(const STGFX_CharMapping_t* CharMapping_p,
                                   U16 Code, U16* Idx_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __TOOLS_H */

/* End of tools.h */


