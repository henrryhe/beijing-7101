#include <stdlib.h> 
#include <string.h> 
#include <limits.h> 
#include "sttbx.h"
#include "stolf.h"
#include "pfrgen.h"
#include "glyph.h"
#include "gfx_tools.h" /* includes stgfx_init.h, ... */

typedef struct {
  /* fractional boundary x's for a given integer y */
  U16 Array[MAX_OLF_SIZE][MAX_OLF_SCANPTS];
  U16 XPos[MAX_OLF_SIZE]; /* number of boundary x's stored for a given y */
  U16 XPrev, YPrev; /* previous point (for LineTo, CurveTo, etc) */
} GlyphPointGrid_t;

#define LINE_TO        1
#define HLINE_TO       2
#define VLINE_TO       3
#define MOVE_TO_IN     4
#define MOVE_TO_OUT    5
#define HVCURVE_TO     6
#define VHCURVE_TO     7
#define CURVE_TO_D0    8
#define CURVE_TO_D1    9
#define CURVE_TO_D2   10
#define CURVE_TO_D3   11
#define CURVE_TO_D4   12
#define CURVE_TO_D5   13
#define CURVE_TO_D6   14
#define CURVE_TO_D7   15
#define DEPTH_MASK_1  0x70
#define DEPTH_MASK_2  0x7

#define G_COMP     0x80
#define SG_ZEROES  0x70
#define SG_XITEM   0x08
#define SG_1BYTE   0x04
#define SG_XCONT   0x02
#define SG_YCONT   0x01
#define SG_START   0x40

#define SG_STROKE     1
#define SG_EDGE       2

#define CG_XITEM   0x40
#define CG_NELEM   0x3f
#define CG_3BGPSO  0x80
#define CG_2BGPSS  0x40
#define CG_YSCALE  0x20
#define CG_XSCALE  0x10
#define CG_XPOS    0x03
#define CG_XPOS_1  0x01
#define CG_XPOS_2  0x02
#define CG_YPOS    0x0c
#define CG_YPOS_1  0x04
#define CG_YPOS_2  0x08

#define SC_SHIFT     12
#define SC_UNITS   (1<<SC_SHIFT)
#define N_CTL_PTS     4 /* start, 1st & 2nd control, end */
#define N_MAX_BEZIER  7
#define N_MIN_BEZIER  3

#define N_MAX_ORU    512

#define FRAC_SHIFT     4
#define FRAC_SCALE(x) ((x)<<FRAC_SHIFT)
#define FRAC_HALF     (1<<(FRAC_SHIFT-1))
#define FRAC_MASK      0x0f /* mask for those same 4 bits */

/*#define FRAC_RESCALE(x) (SGN(x)*(((ABS(x))+(1<<(FRAC_SHIFT-1)))>>FRAC_SHIFT))*/

#define FRAC_RESCALE(x) (((x)+(1<<(FRAC_SHIFT-1)))>>FRAC_SHIFT)

void GetSimpleElementParams(U8** Src_p, S16* XScale_p,
                            S16* YScale_p, S16* XGlyph_p,
                            S16* YGlyph_p, S32* Offset_p);
static ST_ErrorCode_t DecodeGlyph(S32,U16,S16,STGFX_OutlineFont_t*,
                                  GlyphPointGrid_t*);
static ST_ErrorCode_t DecodeSimpleGlyph(S32,S32,S32,S32,S32,S16,U16,U8*, 
                                        GlyphPointGrid_t*);
static void GetCoords(U8**,U8,STGFX_Point_t*,STGFX_Point_t*,S16*,S16*);
static void BezierCurve(STGFX_Point_t*,S32,STGFX_Point_t*,S32);
static void DigLine(S32,S32,GlyphPointGrid_t*);

static S32 intcompare(U16* I_p,U16* J_p) 
{
    return ( (S32)*I_p - (S32)*J_p );
}

static __inline void AddSinglePoint(GlyphPointGrid_t* Grid_p, S32 X, S32 Y) {
  if (Grid_p->XPos[Y]<(MAX_OLF_SCANPTS-1))
    Grid_p->Array[Y][Grid_p->XPos[Y]++] = X;
}

static __inline void AddStartPoint(GlyphPointGrid_t* Grid_p, S32 X, S32 Y) {
  Grid_p->XPrev = X; 
  Grid_p->YPrev = Y; 
}

static __inline void StartGrid(GlyphPointGrid_t* Grid_p){
  memset(Grid_p->XPos,0,sizeof(Grid_p->XPos[0])*MAX_OLF_SIZE);
}



/*
DrawGlyph()
If Bmp_p != NULL this function draws a glyph of a given font,
corresponding to a given character code, at a given position.

If a NULL destination bitmap is provided (Bmp_p==NULL), this
function returns the X coordinate of the right-most pixel of the
glyph on the destination bitmap. In such a case, the BlitContext_p
argument is ignored.

*/
ST_ErrorCode_t DrawGlyph(STBLIT_BlitContext_t* BlitContext_p,
                         U16 Idx, S32 XScreen, S32 YScreen, 
                         STGFX_Handle_t Handle,
                         STGXOBJ_Bitmap_t* Bmp_p, 
                         const STGFX_GC_t* GC_p,
                         S32*    XMax_p)
{
    U32 alloc, xylalloc, xyllen = 0;
    S32 i,j;
    S32 GridOffset;
    S32 StartX, EndX;
    GlyphPointGrid_t*     PointGrid_p;
    STBLIT_XYL_t*         XYL_p;
    STAVMEM_BlockHandle_t AVMEMBlockHandle;
    STGFX_OutlineFont_t*  Font_p = GC_p->Font_p->FontData.OutlineFontData_p;
    U8                    CharSize = MIN(GC_p->FontSize,MAX_OLF_SIZE);
    STBLIT_Handle_t       BlitHandle;
    STGXOBJ_Color_t       DrawColor;
    ST_ErrorCode_t        Error, LastErr = ST_NO_ERROR;
    
#ifdef STGFX_OLF_ANTIALIAS
    U32 xycalloc, xyclen = 0;
    U16 Cover;
    STBLIT_XYC_t*         XYC_p;
    STGXOBJ_Color_t*      ColorArray_p;
    
    /* can't sensibly scale a palette index for anti-aliasing, but assume it for
      GetTextWidth to give the largest potential width */
    BOOL DoAA = (Bmp_p == NULL) ||
                (Bmp_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8) &&
                (Bmp_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT4) &&
                (Bmp_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT2) &&
                (Bmp_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT1);
#endif
    
    BlitHandle = stgfx_GetBlitterHandle(Handle);
    
    if(XMax_p != NULL) *XMax_p = 0;
    
#ifdef STGFX_OLF_ANTIALIAS
    if(DoAA && (Bmp_p != NULL))
    {
        /* colour won't be valid coming from GetTextWidth;
          don't want to bail out with an error */
          
        Error = stgfx_AlphaExtendColor(&(GC_p->TextForegroundColor), &DrawColor);
        if(Error != ST_NO_ERROR) return Error;
    }
    else
#endif    
    {
        DrawColor = GC_p->TextForegroundColor;
    }

    /* ensure PointGrid is allocated, and fill it with info for this glyph */
    
    GridOffset = ((Font_p->Baseline-Font_p->Glyph_p[Idx].Ascent)*CharSize)/
                   Font_p->OutlineResolution;
    
    PointGrid_p = (GlyphPointGrid_t*) (((stgfx_Unit_t*)Handle)->PfrData_p);

    if (PointGrid_p == NULL)
    {
        PointGrid_p = stgfx_DEVMEM_malloc(Handle,sizeof(GlyphPointGrid_t));
        if(PointGrid_p == NULL) return ST_ERROR_NO_MEMORY;
        ((stgfx_Unit_t*)Handle)->PfrData_p = PointGrid_p;
    }

    StartGrid(PointGrid_p);
    Error = DecodeGlyph(CharSize, Idx, GridOffset, Font_p, PointGrid_p);
    if(Error != ST_NO_ERROR) return Error;

    
    /* scan PointGrid to establish top number of XYL elements needed. Each pair
      of x for a given y gives one element - any odd one at the end gives its
      own further one, and the total may be reduced by combining close ones */
      
    for (i=0, xylalloc=0; i<MAX_OLF_SIZE; i++)
    {
        xylalloc += (PointGrid_p->XPos[i]+1) / 2;
    }
    if (xylalloc == 0)
    {
        return LastErr; /* nothing to do */
        /* otherwise we crash when avmem tries to free a zero-length block */
    }
    
    alloc = xylalloc*sizeof(STBLIT_XYL_t);
    
#ifdef STGFX_OLF_ANTIALIAS
    if(DoAA)
    {
        /* we will want at most 1 xyc element at each end of each xyl */
        xycalloc = xylalloc*2;
        alloc += xycalloc * (sizeof(STBLIT_XYC_t) + sizeof(STGXOBJ_Color_t));
    }
#endif
    
    Error = stgfx_AVMEM_malloc(Handle, alloc, &AVMEMBlockHandle, (void**)(&XYL_p));
    if (Error != ST_NO_ERROR) return Error;
    
#ifdef STGFX_OLF_ANTIALIAS
    XYC_p = (STBLIT_XYC_t*)(XYL_p + xylalloc);
    ColorArray_p = (STGXOBJ_Color_t*)(XYC_p + xycalloc);
#endif
    
    /* I suspect ColorArray will only ever be used by STBLIT_DrawXYCArray, not
      passed to the hardware. Thus it has no special alignment requirements
      and doesn't really need to be in avmem - but one allocation is easier
      than two. 1KB is a bit big to go on the stack */
    
    /* transfer PointGrid information to XYL array */
    GridOffset += 1+YScreen;
    for (i=0; i<MAX_OLF_SIZE; i++) 
    {
        if (PointGrid_p->XPos[i]>1) 
        {
            qsort(PointGrid_p->Array[i], PointGrid_p->XPos[i], 
                  sizeof(PointGrid_p->Array[i][0]), 
                  (S32(*)(const void*,const void*))intcompare);
            if (PointGrid_p->XPos[i]%2) 
            {
                PointGrid_p->Array[i][PointGrid_p->XPos[i]]= 
                        PointGrid_p->Array[i][PointGrid_p->XPos[i-1]]; 
                PointGrid_p->XPos[i]++;
            }
            
            for (j=0; j<PointGrid_p->XPos[i]; ) /* j+=2 in loop below */
            {
                StartX = FRAC_RESCALE(PointGrid_p->Array[i][j]);
                
#ifdef STGFX_OLF_ANTIALIAS
                /* left boundary:
                    Abs-Rnd = -FRAC_HALF should give full coverage (16)
                    Abs-Rnd = FRAC_HALF-1 should give minimum coverage (1)
                    Zero can't happen with correct rounding */
                    
                Cover = FRAC_HALF + FRAC_SCALE(StartX)-PointGrid_p->Array[i][j];
                if((Cover < FRAC_SCALE(1)) && DoAA)
                {
                    XYC_p[xyclen].PositionX = XScreen + StartX++;
                    XYC_p[xyclen].PositionY = i+GridOffset;
                    ColorArray_p[xyclen] = DrawColor;
                    ColorArray_p[xyclen++].Value.ARGB8888.Alpha
                        = (DrawColor.Value.ARGB8888.Alpha*Cover)/FRAC_SCALE(1);
                }
#endif
                XYL_p[xyllen].PositionX = XScreen + StartX;
                XYL_p[xyllen].PositionY = i+GridOffset;
            
            
                /* find real segment end, skipping
                  boundaries that are close together */
                do j+=2;
                while ((j<PointGrid_p->XPos[i]) &&
                    (PointGrid_p->Array[i][j]-PointGrid_p->Array[i][j-1]
                                                    < FRAC_SCALE(2)));
                /* j is now one past the end */
             
             
                EndX = FRAC_RESCALE(PointGrid_p->Array[i][j-1]);
                
#ifdef STGFX_OLF_ANTIALIAS
                /* right boundary:
                    Abs-Rnd = -FRAC_HALF should give minimum coverage (1)
                    Abs-Rnd = FRAC_HALF-1 should give full coverage (16)
                    Zero can't happen with correct rounding */
                    
                Cover = 1+FRAC_HALF+PointGrid_p->Array[i][j-1]-FRAC_SCALE(EndX);
                if((Cover < FRAC_SCALE(1)) && DoAA)
                {
                    XYC_p[xyclen].PositionX = XScreen + EndX--;
                    XYC_p[xyclen].PositionY = i+GridOffset;
                    ColorArray_p[xyclen] = DrawColor;
                    ColorArray_p[xyclen++].Value.ARGB8888.Alpha
                        = (DrawColor.Value.ARGB8888.Alpha*Cover)/FRAC_SCALE(1);
                }
                
                if(EndX >= StartX) /* an xyl part remains */
#endif
                {
                    XYL_p[xyllen++].Length = 1 + EndX - StartX;
                }
            }
        }
    }
    
    if((xyllen > xylalloc)
#ifdef STGFX_OLF_ANTIALIAS
     || (xyclen > xycalloc)
#endif
     )
    {
        stgfx_AVMEM_free(Handle, &AVMEMBlockHandle);
        return STGFX_ERROR_XYL_POOL_EXCEEDED;
    }
    
    /* return the rightmost filled x if asked */
    if(XMax_p != NULL)
    {
        for (i=0, *XMax_p=0; i<xyllen; i++) 
        {
            if ((XYL_p[i].PositionX + XYL_p[i].Length) > *XMax_p) 
            {
                *XMax_p = (XYL_p[i].PositionX + XYL_p[i].Length);
                /* this is actually one past the last pixel written */
            }
        }
#ifdef STGFX_OLF_ANTIALIAS
        for (i=0, *XMax_p=0; i<xyclen; i++) 
        {
            if ((XYC_p[i].PositionX + 1) > *XMax_p) 
            {
                *XMax_p = (XYC_p[i].PositionX + 1);
                /* for consistency also one past the last pixel written */
            }
        }
#endif
    }
    
    /* draw the glyph if asked */
    if (Bmp_p != NULL) 
    {
        /* no text background supported? alternative: render to intermediate
          CLUT2 bitmap, which we can then expand producing foreground and
          background */
        
        if(xyllen != 0)
        {
            Error = STBLIT_DrawXYLArray(BlitHandle, Bmp_p, XYL_p, xyllen,
                                        &DrawColor, BlitContext_p);
                                        
            if (Error != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;
        }
        
#ifdef STGFX_OLF_ANTIALIAS
        if(xyclen != 0)
        {
            Error = STBLIT_DrawXYCArray(BlitHandle, Bmp_p, XYC_p, xyclen,
                                        ColorArray_p, BlitContext_p);
                                        
            if (Error != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;
        }
#endif
    }
    
    /* in some cases, we can free the memory directly,
      but it gets complex to keep track of */
    stgfx_ScheduleAVMEM_free(Handle, &AVMEMBlockHandle);
    
    return LastErr;
}


void GetSimpleElementParams(U8** Src_p, S16* XScale_p,
                            S16* YScale_p, S16* XGlyph_p,
                            S16* YGlyph_p, S32* Offset_p)
{
  U8 Flags;
  U8 Size;

  *XScale_p=SC_UNITS;
  *YScale_p=SC_UNITS;
  *XGlyph_p=0;
  *YGlyph_p=0;

  Flags = GetBytes(Src_p,1,0);

  if (Flags & CG_XSCALE) *XScale_p = GetBytes(Src_p,2,1);
  if (Flags & CG_YSCALE) *YScale_p = GetBytes(Src_p,2,1);

  switch (Flags & CG_XPOS) 
  {
    case CG_XPOS_1:
        Size = 2;
        break;
    case CG_XPOS_2:
        Size = 1;
        break;
    default:
        Size = 0;
        break;
  }
  *XGlyph_p = GetBytes(Src_p,Size,1);

  switch (Flags & CG_YPOS) 
  {
    case CG_YPOS_1:
        Size = 2;
        break;
    case CG_YPOS_2:
        Size = 1;
        break;
    default:
        Size = 0;
        break;
  }
  *YGlyph_p += GetBytes(Src_p,Size,1);

  Size = ((Flags & CG_2BGPSS)? 2:  1);
  
  SkipBytes(Src_p,Size);

  Size = ((Flags & CG_3BGPSO)? 3:  2);
  
  *Offset_p = GetBytes(Src_p,Size,0);

}

ST_ErrorCode_t GetControlledCoordinates(U8** Src_p, U8* NumXOrus_p, 
                                        S16* OrusData_p)
{
  S32 i;
  U8 Flags = 0;
  U8 TwoByteCoord = 0;
  U8 NumYOrus = 0;

  Flags = GetBytes(Src_p,1,0);
  if (Flags & SG_ZEROES) {
    return STGFX_ERROR_INVALID_FONT;
  } else {
    if (Flags & SG_1BYTE) {
      NumYOrus = GetBytes(Src_p,1,0);
      *NumXOrus_p = NumYOrus & 0xf;
      NumYOrus >>= 4;
    } else {
      if (Flags & SG_XCONT) *NumXOrus_p = GetBytes(Src_p,1,0);
      if (Flags & SG_YCONT) NumYOrus = GetBytes(Src_p,1,0);
    }
    for (i=0;i<(*NumXOrus_p+NumYOrus);i++) {
      if ((i & 0x7)==0) {
        TwoByteCoord=GetBytes(Src_p,1,0);
      }
      if (TwoByteCoord & (0x01 << (i & 0x7))) {
        OrusData_p[i] = GetBytes(Src_p,2,1);
      } else {
        OrusData_p[i] = GetBytes(Src_p,1,0);
        OrusData_p[i] += i ? OrusData_p[i-1] : 0;
      }
    }
  } 
  return ST_NO_ERROR;
} 

static ST_ErrorCode_t DecodeGlyph(S32 CharSize, U16 Idx, 
                                  S16 GridOffset, STGFX_OutlineFont_t* Font_p,
                                  GlyphPointGrid_t* Grid_p)
{
  S16 XCmp = 0;
  S16 YCmp = 0;
  S16 XScale = SC_UNITS;
  S16 YScale = SC_UNITS;
  U8* Src_p;
  BOOL IsCompound;
  ST_ErrorCode_t Err = ST_NO_ERROR;

  Src_p = Font_p->GlyphSection_p + Font_p->Glyph_p[Idx].GlyphOffset;
  IsCompound = *(Src_p) & G_COMP;

  if (IsCompound) {
    int i;
    U8 Flags = GetBytes(&Src_p,1,0);
    U8 NumElements = Flags & CG_NELEM;
    S32 GlyphOffset;
    if (Flags & CG_XITEM) 
    {
      LoadExtraItem(&Src_p,NULL,DUMMY);
    }
    for (i=0;i<NumElements;i++) 
    {
      GetSimpleElementParams(&Src_p, &XScale, &YScale, &XCmp, &YCmp, 
                             &GlyphOffset);
      Err = DecodeSimpleGlyph(XCmp, YCmp, XScale, YScale, CharSize, 
                              GridOffset, Font_p->OutlineResolution,
                              Font_p->GlyphSection_p+GlyphOffset,Grid_p);
      if(Err != ST_NO_ERROR) break;
    }
  } 
  else 
  {
    Err = DecodeSimpleGlyph(XCmp, YCmp, XScale, YScale, CharSize, GridOffset, 
                            Font_p->OutlineResolution, Src_p, Grid_p);
  }

  return Err;
} 

static ST_ErrorCode_t DecodeSimpleGlyph(S32 XCmp, S32 YCmp, S32 XScale, S32 YScale, 
                                        S32 CharSize, S16 GridOffset,
                                        U16 OutlineResolution, U8* Src_p,
                                        GlyphPointGrid_t* Grid_p)
{
  STGFX_Point_t In;
  STGFX_Point_t Start;
  STGFX_Point_t GridPoint;
  STGFX_Point_t Out[N_MAX_BEZIER];
  S16 OruVals[N_MAX_ORU];
  S16* XOruVal_p;
  S16* YOruVal_p;
  BOOL First;
  S32 i;
  U8 NumXOrus = 0;
  U8 Flags = 0;
  U8 NumOut;
  ST_ErrorCode_t Err = ST_NO_ERROR;

  Start.X = INT_MIN;
  In.X = 0;
  In.Y = 0;

  Flags = *Src_p;

  if (Flags & SG_ZEROES) {
    return STGFX_ERROR_INVALID_FONT;
  } else {
    Err = GetControlledCoordinates(&Src_p, &NumXOrus, OruVals);
    if(Err != ST_NO_ERROR) return Err;
    
    XOruVal_p = OruVals; 
    YOruVal_p = &OruVals[NumXOrus];

    if (Flags & SG_XITEM) {
      LoadExtraItem(&Src_p,NULL,DUMMY);
    }
    Flags = *Src_p;
    if (Flags & SG_START) /* detects initial MOVETO */
    {
      while (Flags) /* EndGlyph record is a single 0 byte */
      {
        Err = GetGlyphPoints(&Src_p,&In,XOruVal_p,YOruVal_p,&NumOut,Out,&First);
        if(Err != ST_NO_ERROR) return Err;
        
        for (i=0;i<NumOut;i++)
        {
          GridPoint.X = Out[i].X;
          GridPoint.X = (GridPoint.X*XScale + (XCmp << SC_SHIFT)) / SC_UNITS;
          GridPoint.X += OutlineResolution;
          GridPoint.X = (GridPoint.X * CharSize) /
                            (OutlineResolution >> FRAC_SHIFT);

          GridPoint.Y = Out[i].Y;
          GridPoint.Y = (GridPoint.Y*YScale + (YCmp << SC_SHIFT)) / SC_UNITS;
          GridPoint.Y = OutlineResolution - GridPoint.Y;
          GridPoint.Y = (GridPoint.Y * CharSize)/(OutlineResolution);

          GridPoint.Y -= GridOffset;

          if (!First) {
            DigLine(GridPoint.X, GridPoint.Y, Grid_p);
          } else { /* MOVETO */
            if (Start.X!=INT_MIN) { /* close previous contour */
              if ((Start.X!=GridPoint.X)||(Start.Y!=GridPoint.Y)) {
                DigLine(Start.X, Start.Y, Grid_p);
              }
            }
            Start.X = GridPoint.X;
            Start.Y = GridPoint.Y;
            AddStartPoint(Grid_p, Start.X, Start.Y);
          }
        }
        In.X = Out[NumOut-1].X;
        In.Y = Out[NumOut-1].Y;
        Flags = *Src_p;
      }
      if (Start.X!=INT_MIN) {
        if ((Start.X!=GridPoint.X)||(Start.Y!=GridPoint.Y)) {
          DigLine(Start.X, Start.Y, Grid_p); /* close the contour */
        }
      }
    /* } else {
      return STGFX_ERROR_INVALID_FONT;
      - occurs for space characters */
    }
  }
  return ST_NO_ERROR;
} 

ST_ErrorCode_t GetGlyphPoints(U8** Src_p, STGFX_Point_t* In_p,  
                              S16* XOru_p, S16* YOru_p, U8* Nout_p,
                              STGFX_Point_t* Out_p, BOOL* Start_p)
{  
  U8 Flags = 0;
  U8 Depth;
  ST_ErrorCode_t Err = ST_NO_ERROR;
  STGFX_Point_t  CtlPoints[N_CTL_PTS];

  *Nout_p = 0;
  *Start_p = FALSE;
  Flags = GetBytes(Src_p,1,0);
  switch (Flags >> 4) {
  case MOVE_TO_IN:
  case MOVE_TO_OUT:
    *Start_p = TRUE;
    GetCoords(Src_p, Flags, Out_p, In_p, XOru_p, YOru_p);
    *Nout_p = 1;
    break;
  case LINE_TO:
    GetCoords(Src_p, Flags, Out_p, In_p, XOru_p, YOru_p);
    *Nout_p = 1;
    break;
  case HLINE_TO:
    Out_p[0].X = XOru_p[Flags & 0xf];
    Out_p[0].Y = In_p->Y; 
    *Nout_p = 1;
    break;
  case VLINE_TO:
    Out_p[0].X = In_p->X; 
    Out_p[0].Y = YOru_p[Flags & 0xf];
    *Nout_p = 1;
    break;
  case HVCURVE_TO:
    Depth = MAX(N_MIN_BEZIER, Flags & DEPTH_MASK_2);
    CtlPoints[0].X = In_p->X;
    CtlPoints[0].Y = In_p->Y;
/*1st Point*/
    CtlPoints[1].X = CtlPoints[0].X + GetBytes(Src_p,1,1); 
    CtlPoints[1].Y = CtlPoints[0].Y;
/*2nd Point*/
    CtlPoints[2].X = XOru_p[GetBytes(Src_p,1,0)];
    CtlPoints[2].Y = CtlPoints[1].Y + GetBytes(Src_p,1,1); 
/*3rd Point*/
    CtlPoints[3].X = CtlPoints[2].X; 
    CtlPoints[3].Y = CtlPoints[2].Y + GetBytes(Src_p,1,1); 
    BezierCurve(CtlPoints,N_CTL_PTS,Out_p,Depth);
    *Nout_p = Depth;
    break;
  case VHCURVE_TO:
    Depth = MAX(N_MIN_BEZIER, Flags & DEPTH_MASK_2);
    CtlPoints[0].X = In_p->X;
    CtlPoints[0].Y = In_p->Y;
/*1st Point*/
    CtlPoints[1].X = CtlPoints[0].X;
    CtlPoints[1].Y = CtlPoints[0].Y + GetBytes(Src_p,1,1); 
/*2nd Point*/
    CtlPoints[2].X = CtlPoints[1].X + GetBytes(Src_p,1,1); 
    CtlPoints[2].Y = YOru_p[GetBytes(Src_p,1,0)];
/*3rd Point*/
    CtlPoints[3].X = CtlPoints[2].X + GetBytes(Src_p,1,1); 
    CtlPoints[3].Y = CtlPoints[2].Y; 
    BezierCurve(CtlPoints,N_CTL_PTS,Out_p,Depth);
    *Nout_p = Depth;
    break;
  case CURVE_TO_D0:
  case CURVE_TO_D1:
  case CURVE_TO_D2:
  case CURVE_TO_D3:
  case CURVE_TO_D4:
  case CURVE_TO_D5:
  case CURVE_TO_D6:
  case CURVE_TO_D7:
    Depth = MAX(N_MIN_BEZIER, (Flags & DEPTH_MASK_1) >> 4);
    CtlPoints[0].X = In_p->X;
    CtlPoints[0].Y = In_p->Y;
/*1st Point*/
    GetCoords(Src_p, Flags, &CtlPoints[1], &CtlPoints[0], XOru_p, YOru_p);
/*2nd Point*/
    Flags = GetBytes(Src_p,1,0);
    GetCoords(Src_p, Flags, &CtlPoints[2], &CtlPoints[1], XOru_p, YOru_p);
/*3rd Point*/
    Flags >>= 4;
    GetCoords(Src_p, Flags, &CtlPoints[3], &CtlPoints[2], XOru_p, YOru_p);
    BezierCurve(CtlPoints,N_CTL_PTS,Out_p,Depth);
    *Nout_p = Depth;
    break;
  default:
    Err = STGFX_ERROR_INVALID_FONT;
    break;
  }
  return Err;
} 

static void GetCoords(U8** Src_p, U8 Flags, STGFX_Point_t* New_p, 
                      STGFX_Point_t* Old_p, S16* XOru_p, S16* YOru_p)
{
  switch (Flags & 0x3) {
  case 0:
    New_p->X = XOru_p[GetBytes(Src_p,1,0)];
    break;
  case 1:
    New_p->X = GetBytes(Src_p,2,1);
    break;
  case 2:
    New_p->X = Old_p->X + GetBytes(Src_p,1,1); 
    break;
  default:
    New_p->X = Old_p->X;
    break;
  }
 
  Flags >>= 2; 
  switch (Flags & 0x3) {
  case 0:
    New_p->Y = YOru_p[GetBytes(Src_p,1,0)];
    break;
  case 1:
    New_p->Y = GetBytes(Src_p,2,1);
    break;
  case 2:
    New_p->Y = Old_p->Y + GetBytes(Src_p,1,1); 
    break;
  default:
    New_p->Y = Old_p->Y;
    break;
  }

} 

static void BezierCurve(STGFX_Point_t* In_p, S32 NumIn, 
                        STGFX_Point_t* Out_p, S32 NumOut)
{
  STGFX_Point_t BezierCoeffs[N_CTL_PTS];
  STGFX_Point_t BezierLoop[N_CTL_PTS];
  S32 ChooseNum, ChooseDen;
  S32 FirstNum,FirstDen,SecondNum,SecondDen;
  S32 i,j;

/*setup Bezier Coefficients*/
  for (i=0;i<NumIn;i++) {
    if (i==0) {
      ChooseNum=1;
      ChooseDen=1;
    } else if (i==1) {
      ChooseNum=NumIn-1;
      ChooseDen=1;
    } else {
      ChooseNum*=(NumIn-i);
      ChooseDen*=i;
    }
    BezierCoeffs[i].X = (In_p[i].X * ChooseNum)/ChooseDen; 
    BezierCoeffs[i].Y = (In_p[i].Y * ChooseNum)/ChooseDen; 
  }
  BezierLoop[0].X =  BezierCoeffs[0].X; 
  BezierLoop[0].Y =  BezierCoeffs[0].Y; 
  for (i=1;i<NumOut;i++) {
    FirstNum=i;
    FirstDen=NumOut;
    for (j=1;j<NumIn;j++) {
      BezierLoop[j].X =  (BezierCoeffs[j].X * FirstNum)/FirstDen; 
      BezierLoop[j].Y =  (BezierCoeffs[j].Y * FirstNum)/FirstDen; 
      FirstNum*=i;
      FirstDen*=NumOut;
    }
    Out_p[i-1].X = BezierLoop[NumIn-1].X;
    Out_p[i-1].Y = BezierLoop[NumIn-1].Y;
    SecondNum = FirstNum = NumOut - i;
    SecondDen = FirstDen = NumOut;
    for (j=NumIn-2;j>-1;j--) {
      Out_p[i-1].X += (BezierLoop[j].X*SecondNum)/SecondDen; 
      Out_p[i-1].Y += (BezierLoop[j].Y*SecondNum)/SecondDen; 
      SecondNum*=FirstNum;
      SecondDen*=FirstDen;
    }
  }
  
  /* write end point */
  Out_p[NumOut-1] = In_p[NumIn-1];
}

/* X2 should be in fractional representation, Y2 integer */
static void DigLine(S32 X2, S32 Y2, GlyphPointGrid_t* Grid_p)
{
  S32 B,DX,DY,YMin,YMax;
  S32 X0,Y0,XF,YF, X, Y;

  X0 = Grid_p->XPrev;
  XF = X2;
  
  /* y always steps by whole points, so no need for fractional form */
  Y0 = Grid_p->YPrev;
  YF = Y2;

  DX = XF - X0;
  DY = YF - Y0;

  YMin = MIN(YF,Y0);
  YMax = MAX(YF,Y0);

  if (DY==0)
  {
    /* don't draw - no new left/right boundaries */
  }
  else if (DX==0)
  {
    for (Y=YMin; Y<YMax; ++Y)
    {
      AddSinglePoint(Grid_p,XF,Y);
    }
  }
  else
  {
    B = XF*DY - YF*DX; /* FRAC_SCALE factor is present once in both terms */

    for (Y=YMin; Y<YMax; ++Y)
    {
      X = (Y*DX+B)/DY;
      AddSinglePoint(Grid_p,X,Y);
    }   
  }
  
  Grid_p->XPrev = X2;
  Grid_p->YPrev = Y2;
}
