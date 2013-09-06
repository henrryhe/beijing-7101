/******************************************************************************

    File Name   : gfx_conic.c

    Description : Low-level conic functions, used for caps/dots/joins
                  in addition to arcs/ellipses

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include "sttbx.h"
#include "gfx_tools.h" /* includes stgfx_init.h, ... */
#include <stdio.h>
#include <string.h>
#include <limits.h> /* INT_MAX / INT_MIN */

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define PARAM 4 
#define BIG_NUMBER PARAM*MAX_SCREEN_HEIGTH
#define PI         11520 /* 180 degrees * 64: PI in GFX units */
#define PI_FLOAT   (3.14159265359)
/*#define INT_ACCURACY 64*/

#define TRIG_PREC 15 /* a safe shift for functions valued between 0 and 1 */
#define TRIG_UNITY (1<<TRIG_PREC)
#define TAN_PREC  10 /* tangent precision, a safe shift on real co-ords */

#define PI_TRIG 102944 /* PI * 2^15; 205887 = PI * 2^16 */

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/*#define FRACT_TO_UINT1(a, shift)  (((a) +(1<<((shift)-1))) >> (shift))*/
#define FLOAT_TO_INT1(a) (((a) >= 0 ) ? (S32)((a) + 0.5F) : (S32)((a)-0.5F))
/*#define INCREMENT(a) (((a) >= 0 ) ? ((a)+1) : ((a)-1)) */
#define DECREMENT(a) (((a) < 0 ) ? ((a)+1) : ((a)-1)) 

#define TRIG_TO_INT(a) (((a) >= 0) ? ((a) >> TRIG_PREC) : -(-(a) >> TRIG_PREC))

/* allow both to have hit the target, or one to have just missed provided the
  other has passed. Eventually, do the conversions to int and indeed absolute
  values earlier */
#define HIT_BOUNDARY(DX, DY, TargetDX, TargetDY) \
 ((GFXABS(DX) >= GFXABS(TargetDX) && \
   GFXABS(DY) >= GFXABS(TargetDY)) || \
  (GFXABS(DX) >  GFXABS(TargetDX) && \
   GFXABS(DY) == GFXABS(TargetDY)-1) || \
  (GFXABS(DX) == GFXABS(TargetDX)-1 && \
   GFXABS(DY) >  GFXABS(TargetDY))) \


/* Private Function Prototypes --------------------------------------------- */

static void InitConicParameters( S32 Octant, float A, float B, float C,
                                 float D, float E, float F, float* d,
                                 float* u, float* v, float* k1, float* k2, 
                                 float* k3, S32* dxsquare, S32* dysquare,
                                 S32* dxdiag, S32* dydiag );

static void GetOctant( float D, float E, S32* Octant);

static void CalcEllipsePoint( float LRadius, float SRadius, 
                              S32 Angle, float* XS, float* YS );  

/*static void CalcRadius( float LRadius, float SRadius, S32 Angle,
                        float* Radius );*/

/*static void EndPointAndDirection( S32 XC, S32 YC, U32 LRadius, 
                                  U32 SRadius, U16 LineWidth,
                                  S32 Angle,  
                                  S32 Orientation, S32 Distance,
                                  S32* SAngle,
                                  S32* EndPointX, S32* EndPointY,
                                  S32* DirectionX, S32* DirectionY,
                                  BOOL IsFirstPoint);*/

static void FindMaxMinArrayXYL( STBLIT_XYL_t* XYLArray, U32 Size,
                                U32* PosMax, U32* PosMin );

static void RotatePoint(S32 Orientation, float* XS, float* YS);

static void TranslatePoint( S32 Orientation, 
                            float LongRadius, 
                            float XCenter, float YCenter,
                            float* XTranslated, 
                            float* YTranslated );

static void CalcOutArcPoint(float XIn, float YIn, float LRadiusIn, 
                            float SRadiusIn, U16 LineWidth, double Angle,
                            float* XOut, float* YOut);

static ST_ErrorCode_t Thickness(
    U32 BHeight, float XC, float YC, BOOL IsEllipse,
    U32 SizeInner, STBLIT_XYL_t* InnerArray,
    U32 SizeOuter, STBLIT_XYL_t* OuterArray,
    U32* SizeThicknessArray, STBLIT_XYL_t*  ThicknessArray,
    U32* SizeSELineArray, STBLIT_XYL_t*  SELineArray,
    U32* SizeEELineArray, STBLIT_XYL_t*  EELineArray,
    S32* Points_p );

/*static void InnerAndOuterRadius(U16 LineWidth, U32 LRadius, U32 SRadius,
                                U32* InnerLRadius, U32* InnerSRadius,
                                U32* OuterLRadius, U32* OuterSRadius);*/

/* Functions --------------------------------------------------------------- */


/*******************************************************************************
Name        : stgfx_2dRasterSimpleEllipse()
Description : fills an array xyl of the boundary
              as well as the one of the filling; both of them must be allocated
              before calling this function. If the ellipse is empty, a null
              pointer shall be used instead
Parameters  : BHeight,        height of the bitmap 
              XC,             Center X coordinate
              YC,             Center Y coordinate
              HRadius,        horizontal semi axis length
              VRadius,        vertical semi axis length 
              Orientation     angle between the 'horizontal' axis and the
                              horizontal x axis 
              Fill            true if ellipse must be filled
              NumXYL          size of array xyl
              XYLArray_p      pointer to array xyl
              SizeFillArray   size of array xyl for the filling
              FillArrayXYL_p      pointer to array xyl for the filling
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
*******************************************************************************/
ST_ErrorCode_t stgfx_2dRasterSimpleEllipse( 
    U32 BHeight, 
    S32 XC, 
    S32 YC, 
    U32 HRadius, 
    U32 VRadius,
    S32 Orientation, 
    BOOL Fill,
    U32* NumXYL, 
    STBLIT_XYL_t*  XYLArray_p,
    U32* SizeFillArray, 
    STBLIT_XYL_t* FillArrayXYL_p 
)
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  U32                    LRadius;
  U32                    SRadius;
  float                  XP=0;
  float                  YP=0;
  float                  XD;
  float                  YD;
  float                  XCenter;
  float                  YCenter;
  float                  LongRadius;
  float                  ShortRadius;
  float                  A, B, C, D, E, F;
  U32                    PosMin;
  U32                    PosMax;
  stgfx_Side_t           Sides[3];
  U32                    TotSides;
  stgfx_Side_t*          Sides_p;
  U32                    AvailableEllipseSize = *NumXYL;
  U32                    AvailableFillSize = *SizeFillArray;
  Sides_p = &Sides[0];
  

  if(HRadius >= VRadius)
  {
    LRadius = HRadius;
    SRadius = VRadius;
  }
  else
  {
    LRadius = VRadius;
    SRadius = HRadius;
    Orientation += (90*64);
  }
   
  CheckAngle(&Orientation);
  
  if((HRadius == VRadius) || (Orientation == 0) ||
      (Orientation == (64*180)) ||
      (Orientation == (64*90)) ||
      (Orientation == (64*270)) ||
      (Orientation == (64*360)))
  {
    if(Orientation == (64*90)|| (Orientation == (64*270)))
    {
       GFXSWAP(LRadius, SRadius); 
    }
    Err = stgfx_2dRasterFastSimpleEllipse( BHeight, XC, YC, 
                                           LRadius, SRadius, Fill,
                                           NumXYL, XYLArray_p,
                                           SizeFillArray, FillArrayXYL_p );
  }
  else
  {
  
    XCenter = (float)XC / (float)(1<<STGFX_INTERNAL_ACCURACY);
    YCenter = (float)YC / (float)(1<<STGFX_INTERNAL_ACCURACY);
    LongRadius = (float)LRadius / (float)(1<<STGFX_INTERNAL_ACCURACY);
    ShortRadius = (float)SRadius / (float)(1<<STGFX_INTERNAL_ACCURACY);
    CalcConicCoefficients( LongRadius, ShortRadius, Orientation, 
                           &A, &B, &C, &D, &E, &F, &XP, &YP, &XD, &YD );
    Err = Conic( XP, YP, XP, YP, XP, YP, A, B, C, D, E, F, 
                 (XCenter-XD), (YCenter+YD), XD, YD, 
                 NumXYL, XYLArray_p, FALSE, NULL, FALSE );

    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }

    TIME_HERE("stgfx_2dRasterSimpleEllipse Conic done");
  
    if(Fill)  /* the ellipse has to be filled */
    {
      SplitArrayXYL( XYLArray_p, *NumXYL, &PosMin, &PosMax, TRUE,
                     Sides_p, &TotSides );

      TIME_HERE("stgfx_2dRasterSimpleEllipse SplitArrayXYL done");

      Err = stgfx_FillGenericPolygon( BHeight, 
                                      XYLArray_p[PosMin].PositionY, 
                                      XYLArray_p[PosMax].PositionY, 
                                      Sides_p, TotSides,
                                      FillArrayXYL_p, SizeFillArray,
                                      STGFX_WINDING_FILL );
    } /* end if Fill */
  }
  
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  if(*NumXYL > AvailableEllipseSize)
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }
  if(Fill && (*SizeFillArray > AvailableFillSize))
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }
  
  
  /* In order to call the FillGenericPolygon function it is needed
     to repeat the first element of the array xyl; but now we
     can ignore it (JF doesn't do anything in Fast case) */
  if((*NumXYL > 1)&&(XYLArray_p[*NumXYL-1].PositionY == XYLArray_p[0].PositionY))
  {
    (*NumXYL)--;
  }

  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stgfx_2dRasterFastSimpleEllipse()
Description : fills an array xyl of the ellipse boundary 
              in the following cases:
              1 - Orientation = 0
              2 - HRadius = VRadius
              This function is called only from stgfx_2dRasterSimpleEllipse
Parameters  : BHeight,        height of the bitmap 
              XC,             Center X coordinate
              YC,             Center Y coordinate
              HRadius,        horizontal semi axis length
              VRadius,        vertical semi axis length 
              Fill            whether to fill
              NumXYL          size of array xyl
              XYLArray_p      pointer to array xyl
              NumXYL          size of fill array
              XYLArray_p      pointer to fill array, if Fill = TRUE
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
*******************************************************************************/
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
)
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  STBLIT_XYL_t*          FillArrayStart_p = FillArray_p;
  STBLIT_XYL_t*          XYLArrayStart_p = XYLArray_p;
  U32                    AvailableSize = *LengthXYL;
  S32 d;
  S32 asq, bsq, c;
  S32 x, y;
  S32 idx, j;
  S32 Diff;
  U32 length;
  
  /* the maximum value of S32 is 2^31, so overflow for an R^4 variable will
    occur for R ~ 215. Note that only d in the second loop has this size;
    everything else is order R^3 or lower. Floating point costs far too much
    (a factor ~70) */
    
  /* discard fractional parts now; x and y always move by integer steps */
  XC = FRACT_TO_INT(XC);
  YC = FRACT_TO_INT(YC);
  XRadius = FRACT_TO_UINT(XRadius);
  YRadius = FRACT_TO_UINT(YRadius);

  length = YRadius; /* number of xyl per quadrant */
  asq = XRadius*XRadius;
  bsq = YRadius*YRadius;
  
  /* current algorithm initially generates [0] at [length*4], hence +1 */
  if( (length*4)+1 > AvailableSize)
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }
  
  /* TODO: Blend all element storage into the first generation loop
    check array addressing
    check order requirements of Thickness function
    test for very narrow ellipses */

  x = 0;
  y = YRadius;
  
  /* d = (bsq - (YR*asq) + (asq/4)), but round up on the division because all we
    care about are zero-crossings, and all other contributions are integer */
  d = bsq - (YRadius*asq) + (asq+3)/4;
  
  XYLArray_p = XYLArrayStart_p + 3*length;
  
  XYLArray_p->PositionX = x;
  XYLArray_p->PositionY = y;
  XYLArray_p->Length = 1;
  
  /* first loop chooses between E and NE */
  /* loop condition is asq*(y - 0.5) > bsq*(x+1), which we transform to: */
  c = (asq+1)/2 + bsq;
  while((asq*y - bsq*x > c) && (x < XRadius))
  {
    if(d<0) /* midpoint is internal, new point is E */
    {
      d += bsq*(2*x + 3);
      x += 1;
      XYLArray_p->Length++;
      /* XYLArrayStart_p[(3*length) + YRadius - y] */
    }
    else /* midpoint is external, new point is NE */
    {
      d += bsq*(2*x + 3) + 2*asq*(1-y);
      x += 1;
      y -= 1;

      ++XYLArray_p; /* to go to XYLArrayStart_p[(3*length)+YRadius-y] */
      XYLArray_p->PositionX = x;
      XYLArray_p->PositionY = y;
      XYLArray_p->Length = 1;
      
      if(Fill && (x > 0))
      {
        FillArray_p->PositionX = XC - x + 1;
        FillArray_p->PositionY = YC + y;
        FillArray_p->Length = 2*x - 1;
        ++FillArray_p;
        
        if(y > 0) /* very rarely needed in this loop */
        {
          FillArray_p->PositionX = XC - x + 1;
          FillArray_p->PositionY = YC - y;
          FillArray_p->Length = 2*x - 1;
          ++FillArray_p;
        }
      }
    }
  }
  
  if(y < 0)
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }
  
  /* (bsq*(x+0.5)^2) + (asq*(y-1)^2) - asq*bsq;
    Original code had 0.25 rather than 0.5 as in Feller */
  d = (bsq*(x*x + x) + (bsq+3)/4) + (asq*(y-1)*(y-1)) - asq*bsq;
    
  /* second loop chooses between N and NE */
  while(y > 0)
  {
    if(d<0) /* midpoint is internal, new point is NE */
    {
      d = d + (2*bsq*(x+1) + asq*(3-2*y));
      x++;
      y--;
    }
    else /* midpoint is external, new point is N */
    {
      d = d + (asq*(3 - 2*y));
      y--;
    }

    ++XYLArray_p; /* to go to XYLArrayStart_p[(3*length)+YRadius-y] */
    XYLArray_p->PositionX = x; 
    XYLArray_p->PositionY = y; 
    XYLArray_p->Length = 1; 

    if(Fill && (x > 0)) /* x > 0 rarely needed in this loop */
    {
      FillArray_p->PositionX = XC - x + 1;
      FillArray_p->PositionY = YC + y;
      FillArray_p->Length = 2*x - 1;
      ++FillArray_p;

      if(y > 0)
      {
        FillArray_p->PositionX = XC - x + 1;
        FillArray_p->PositionY = YC - y;
        FillArray_p->Length = 2*x - 1;
        ++FillArray_p;
      }
    }
  }
  
  XYLArrayStart_p[0].PositionX = XC + XYLArray_p->PositionX;
  XYLArrayStart_p[0].PositionY = YC + XYLArray_p->PositionY;
  XYLArrayStart_p[0].Length = XYLArray_p->Length;
  
  XYLArray_p = XYLArrayStart_p; /* restore for existing copy code below */
  
  TIME_HERE("stgfx_2dRasterFastSimpleEllipse fourth quadrant done");
  
  /* Up to now we were calculating the fourth quadrant only. 
     Now we can calculate all the others. Everything seems to be
     quite strange because every pixel must be put in anti-clockwise
     order in the array; this is required by the 'Thickness' function
     in case of thick ellipses. */
   
  XYLArray_p[3*length].PositionX = XYLArray_p[3*length].PositionX - 
                                   XYLArray_p[3*length].Length + 1;
  XYLArray_p[3*length].Length = (2 * XYLArray_p[3*length].Length) - 1;

  /* 3th quadrant  (until j < (4*length))*/
  for(idx = ((3*length)-1), j = (3*length)+1;
      idx > (2*length);
      idx--,j++)
  {
    XYLArray_p[idx].PositionX = -(XYLArray_p[j].PositionX + 
                                  XYLArray_p[j].Length) + XC + 1 ;
    XYLArray_p[idx].PositionY = XYLArray_p[j].PositionY + YC;
    XYLArray_p[idx].Length = XYLArray_p[j].Length;
  }

  /*XYLArray_p[2*length].PositionX = (XYLArray_p[0].PositionX) -
                                   (2*XRadius) ;*/
  Diff = XYLArray_p[0].PositionX - XC;                
  XYLArray_p[2*length].PositionX = XC - Diff; 
  XYLArray_p[2*length].PositionY = XYLArray_p[0].PositionY;
  XYLArray_p[2*length].Length = XYLArray_p[0].Length;

  /* 2nd quadrant ( until j < (3*length))*/
  for(idx = ((2*length)-1), j = ((2*length)+1);
      idx > (length); 
      idx--, j++)
  {
    XYLArray_p[idx].PositionX = XYLArray_p[j].PositionX;
    XYLArray_p[idx].PositionY = 2*YC - XYLArray_p[j].PositionY;
    XYLArray_p[idx].Length = XYLArray_p[j].Length;
  }

  /* 1st quadrant (until j < (4* length))*/
  for(idx= length, j = 3*length; idx >= 1; idx--, j++)
  {
    XYLArray_p[idx].PositionX = XC + XYLArray_p[j].PositionX;
    XYLArray_p[idx].PositionY = YC - XYLArray_p[j].PositionY;
    XYLArray_p[idx].Length = XYLArray_p[j].Length;
  }
  
  /* add Center coords to the reference quadrant (the 4th one) */
  for(idx = 3*length; idx < (4*length); idx++)
  {
    XYLArray_p[idx].PositionX += XC;
    XYLArray_p[idx].PositionY += YC;
  }
  
  *LengthXYL= 4*length;
  
  if(Fill)
  {
    *SizeFillArray = FillArray_p - FillArrayStart_p;
  }
  
  return(Err);
}

#ifndef ST_GX1 /* apparent compiler bug prevents CalcAlignedSector loop running on GX1 */
/******************************************************************************
Function Name : stgfx_CalcAlignedCurve
  Description : Produce a U16 array giving the x offsets for each step going
                up the curve of an axis-aligned ellipse. The segment length
                at each step should be this offset, or 1 if that is 0, so as
                to maintain a continuous chain of points. Array size is
                VRadius+1. U16's to reduce memory footprint. Based on
                stgfx_2dRasterFastSimpleEllipse.
   Parameters :
******************************************************************************/
void stgfx_CalcAlignedCurve(U16* Array_p, U16 HRadius, U16 VRadius)
{
  S32 asq, bsq, c;
  S32 x, y; /* still needed for the loop conditions */
  S32 d;

  asq = HRadius*HRadius;
  bsq = VRadius*VRadius;

  x = 0;
  y = VRadius;

  /* d = (bsq - (VR*asq) + (asq/4)), but round up on the division because all we
    care about are zero-crossings, and all other contributions are integer */
  d = bsq - (VRadius*asq) + (asq+3)/4;

  /* Write elements in reverse order, so [0] represents y = 0. This means that
    the increment for NE steps goes on the new, lower index element. */
  Array_p[y] = 0;

  /* first loop chooses between E and NE */
  /* loop condition is asq*(y - 0.5) > bsq*(x+1), which we transform to: */
  c = (asq+1)/2 + bsq;
  while((asq*y - bsq*x > c) && (x < HRadius))
  {
    if(d<0) /* midpoint is internal, new point is E */
    {
      d += bsq*(2*x + 3);
      x += 1;
      
      Array_p[y] += 1;
    }
    else /* midpoint is external, new point is NE */
    {
      d += bsq*(2*x + 3) + 2*asq*(1-y);
      x += 1;
      y -= 1;
      
      Array_p[y] = 1;
    }
  }

  /* (bsq*(x+0.5)^2) + (asq*(y-1)^2) - asq*bsq;
    Original code had 0.25 rather than 0.5 as in Feller */
  d = (bsq*(x*x + x) + (bsq+3)/4) + (asq*(y-1)*(y-1)) - asq*bsq;

  /* second loop chooses between N and NE */
  while(y > 0)
  {
    if(d<0) /* midpoint is internal, new point is NE */
    {
      d = d + (2*bsq*(x+1) + asq*(3-2*y));
      x += 1;
      y -= 1;
      
      Array_p[y] = 1;
    }
    else /* midpoint is external, new point is N */
    {
      d = d + (asq*(3 - 2*y));
      y -= 1;
      
      Array_p[y] = 0;
    }
  }
}

/******************************************************************************
Function Name : stgfx_CalcAlignedSector
  Description : Optimised calculation of xyl array for solid sectors of axis-
                aligned ellipses. Produces up to VRadius*3+1 elements
   Parameters : 0*64 <= StartAngle < 360*64, 0 <= ArcAngle <= 360*64
******************************************************************************/
ST_ErrorCode_t stgfx_CalcAlignedSector(STGFX_Handle_t Handle, BOOL Filled,
                                       S32 XC, S32 YC,
                                       U16 HRadius, U16 VRadius,
                                       S32 StartAngle, U32 ArcAngle,
                                       STBLIT_XYL_t* XYLArray, U32* NumXYL)
{
  BOOL UpArcOn, DownArcOn, DrawLeft, DrawRight;
  S32 EndAngle, EndAngleNorm;
  S32 xLeft, xRight, yUp, yDown;
  S32 StartLineFirstInc, StartLineInc, StartLineXFrac;
  S32 StartLineDir, StartLineXInt;
  S32 EndLineFirstInc, EndLineInc, EndLineXFrac;
  S32 EndLineDir, EndLineXInt;
  S32 LinesXMin, LinesXMax;
  S32 l; /* for unfilled case, S32 to reduce number of casts */
  U16 *SrcArray, *Src_p, *SrcLimit_p;
  STBLIT_XYL_t* XYL_p = XYLArray;

  /* calculate end angles */
  EndAngle = StartAngle + ArcAngle;
  EndAngleNorm = (EndAngle < 360*64) ? EndAngle : (EndAngle - 360*64);
  
  /* obtain core curve */
  SrcArray = (U16*) stgfx_CPUMEM_malloc(Handle, (VRadius+1)*sizeof(U16));
  if(SrcArray == NULL)
  {
    return ST_ERROR_NO_MEMORY;
  }
  stgfx_CalcAlignedCurve(SrcArray, HRadius, VRadius);
  
  Src_p = SrcArray;
  SrcLimit_p = Src_p + VRadius; /* We take VRadius upward steps */

  /*printf("&StartAngle %p, &StartLineXInt %p, &SrcLimit_p %p\n",
         (void*)&StartAngle, (void*)&StartLineXInt, (void*)&SrcLimit_p);*/
  /*printf("After CalcAlignedCurve Src_p %p, SrcLimit_p %p\n",
         (void*)Src_p, (void*)SrcLimit_p);*/
#if 0  
  /* TEMPORARY DEBUG */
  printf("stgfx_CalcAlignedSector SrcArray:\n   ");
  while(Src_p < SrcLimit_p)
  {
    printf(" %hu", *Src_p++);
  }
  printf("\n");
  Src_p = SrcArray;
#endif
  
  /* Get line directions */
  StartLineXFrac = XC << TRIG_PREC;
  /*printf("After StartLineXFrac Src_p %p, SrcLimit_p %p\n",
         (void*)Src_p, (void*)SrcLimit_p);*/
  StartLineXInt = XC;
  /*printf("After StartLineXInt Src_p %p, SrcLimit_p %p\n",
         (void*)Src_p, (void*)SrcLimit_p);*/
  if((StartAngle == 0*64) || (StartAngle == 180*64))
  {
    /* horizontal - only affects first segment */
    StartLineDir = 0;
    StartLineInc = StartLineFirstInc = 0; /* for neatness */
    /* could set StartLineX to outer position, ie +/- HRadius */
  }
  else
  {
    /*printf("After StartLine if(0 180) Src_p %p, SrcLimit_p %p\n",
           (void*)Src_p, (void*)SrcLimit_p);*/

    /* upward is -ve y */
    StartLineDir = (StartAngle < 180*64) ? -1 : +1;
    
    if((StartAngle == 90*64) || (StartAngle == 270*64))
    {
      StartLineInc = 0; /* vertical (QuickCot would also give this) */
      StartLineFirstInc = 0;
    }
    else
    {
      /* get x increment per y, in fixed point form */
      StartLineInc = QuickCot(StartAngle);
      if(StartAngle > 180*64)
        StartLineInc = -StartLineInc; /* compensate for y inversion */
      
      /* put half an increment at the start, half at the end, but pull back
        half a pixel as we always have a whole one at each end */
        
      /* The 'outer' edge of the line is the one with x further out from the
        centre for a given y */
        
      StartLineFirstInc = StartLineInc;
      if(StartLineInc > 0)
      {
        if(StartLineDir > 0)
        {
          /* down and right - want inner edge of line */
          StartLineFirstInc = (StartLineInc + TRIG_UNITY) / 2;
        }
        else
        {
          /* up and right - want outer edge of line */
          StartLineXFrac += GFXMAX(0, (StartLineInc - TRIG_UNITY) / 2);
        }
      }
      else
      {
        if(StartLineDir > 0)
        {
          /* down and left - want outer edge of line */
          StartLineXFrac += GFXMIN(0, (StartLineInc + TRIG_UNITY) / 2);
        }
        else
        {
          /* up and left - want inner edge of line */
          StartLineFirstInc = (StartLineInc - TRIG_UNITY) / 2;
        }
      }
      StartLineXInt = TRIG_TO_INT(StartLineXFrac);
    }
  }
  
  /*printf("After StartLine Src_p %p, SrcLimit_p %p\n",
         (void*)Src_p, (void*)SrcLimit_p);*/

  EndLineXFrac = XC << TRIG_PREC; /* replace Start/End, use EndAngleNorm */
  EndLineXInt = XC;
  if((EndAngleNorm == 0*64) || (EndAngleNorm == 180*64))
  {
    /* horizontal - only affects first segment */
    EndLineDir = 0;
    EndLineInc = EndLineFirstInc = 0; /* for neatness */
    /* could set EndLineX to outer position, ie +/- HRadius */
  }
  else
  {
    /* upward is -ve y */
    EndLineDir = (EndAngleNorm < 180*64) ? -1 : +1;
    
    if((EndAngleNorm == 90*64) || (EndAngleNorm == 270*64))
    {
      EndLineInc = 0; /* vertical (QuickCot would also give this) */
      EndLineFirstInc = 0;
    }
    else
    {
      /* get x increment per y, in fixed point form */
      EndLineInc = QuickCot(EndAngleNorm);
      if(EndAngleNorm > 180*64)
        EndLineInc = -EndLineInc; /* compensate for y inversion */
      
      EndLineFirstInc = EndLineInc;
      if(EndLineInc > 0)
      {
        if(EndLineDir > 0)
        {
          /* down and right - want outer edge of line */
          EndLineXFrac += GFXMAX(0, (EndLineInc - TRIG_UNITY) / 2);
        }
        else
        {
          /* up and right - want inner edge of line */
          EndLineFirstInc = (EndLineInc + TRIG_UNITY) / 2;
        }
      }
      else
      {
        if(EndLineDir > 0)
        {
          /* down and left - want inner edge of line */
          EndLineFirstInc = (EndLineInc - TRIG_UNITY) / 2;
        }
        else
        {
          /* up and left - want outer edge of line */
          EndLineXFrac += GFXMIN(0, (EndLineInc + TRIG_UNITY) / 2);
        }
      }
      EndLineXInt = TRIG_TO_INT(EndLineXFrac);
    }
  }
  
  /* identify if either half-plane will end in arc-arc mode
    (ie vertical radius lies strictly within arc) */
  UpArcOn = (StartAngle < 90*64) && (EndAngle > 90*64) ||
            (StartAngle > 90*64) && (EndAngle > (360+90)*64);
  DownArcOn = (StartAngle < 270*64) && (EndAngle > 270*64) ||
              (StartAngle > 270*64) && (EndAngle > (360+270)*64);
  
  /* start at y = 0 */
  xLeft = XC - HRadius; /* outermost position */
  xRight = XC + HRadius;
  yUp = YC;
  yDown = YC;

  /* three cases for first segment:
   *  i. full width segment (lines horizontal or in same half-plane)
   *  ii. half width segment (lines in different half planes)
   *  iii. 'point' in centre (two lines in one half-plane)
   */
  
  DrawLeft = (StartAngle <= 180*64) && (EndAngle >= 180*64) ||
             (StartAngle > 180*64) && (EndAngle >= (360+180)*64);
  DrawRight = (StartAngle == 0*64) || (EndAngle >= 360*64);
  
  if(Filled)
  {
    /* for first segment, identify min and max line x and clip to curve bounds */
    LinesXMin = XC;
    if(StartLineXInt < LinesXMin) LinesXMin = StartLineXInt;
    if(EndLineXInt < LinesXMin) LinesXMin = EndLineXInt;
    
    LinesXMax = XC;
    if(StartLineXInt > LinesXMax) LinesXMax = StartLineXInt;
    if(EndLineXInt > LinesXMax) LinesXMax = EndLineXInt;
    
    if(LinesXMin < xLeft) LinesXMin = xLeft;
    if(LinesXMin > xRight) LinesXMin = xRight;

    XYL_p->PositionY = yUp;
    if(DrawLeft && DrawRight)
    {
      /* full width segment */
      XYL_p->PositionX = xLeft;
      XYL_p->Length = 1 + xRight - xLeft;
    }
    else if(DrawLeft)
    {
      /* half width segment on left-hand side */
      XYL_p->PositionX = xLeft;
      XYL_p->Length = 1 + LinesXMax - xLeft;
    }
    else if(DrawRight)
    {
      /* half width segment on right-hand side */
      XYL_p->PositionX = LinesXMin;
      XYL_p->Length = 1 + xRight - LinesXMin;
    }
    else
    {
      /* just line ends */
      XYL_p->PositionX = LinesXMin;
      XYL_p->Length = 1 + LinesXMax - LinesXMin;
    }
    ++XYL_p;
  }
  else
  {
    /* unfilled arc, radii only to determine extent, not drawn */
    
    l = *Src_p;
    if(l == 0) l = 1;
    if(DrawLeft)
    {
      XYL_p->PositionX = xLeft;
      XYL_p->PositionY = yUp;
      XYL_p->Length = l;
      ++XYL_p;
    }
    
    if(DrawRight)
    {
      XYL_p->PositionX = xRight - (l-1);
      XYL_p->PositionY = yUp;
      XYL_p->Length = l;
      ++XYL_p;
    }
  }
  
  /* apply first increment to lines (no consequence if lines not turned on) */
  StartLineXFrac += StartLineFirstInc;
  EndLineXFrac += EndLineFirstInc;

  /*SrcLimit_p = Src_p + VRadius; - something scrubs this on GX1 */
    
  /*printf("Going into loop VRadius %hu, Src_p %p, SrcLimit_p %p\n",
         VRadius, (void*)Src_p, (void*)SrcLimit_p);*/
         
  while(Src_p < SrcLimit_p)
  {
    /* advance y */
    yUp -= 1;
    yDown += 1;
    
    /* advance curve */
    xLeft += *Src_p;
    xRight -= *Src_p;
    ++Src_p;

    /* check for line termination */
    if(StartLineDir != 0)
    {
      StartLineXInt = TRIG_TO_INT(StartLineXFrac);
      if((StartLineXInt > xRight) || (StartLineXInt < xLeft))
      {
        /*printf("Start line terminated at y = %i\n", yDown - YC);*/
        StartLineDir = 0;
      }
    }
    
    if(EndLineDir != 0) /* replace Start/End */
    {
      EndLineXInt = TRIG_TO_INT(EndLineXFrac);
      if((EndLineXInt > xRight) || (EndLineXInt < xLeft))
      {
        /*printf("End line terminated at y = %i\n", yDown - YC);*/
        EndLineDir = 0;
      }
    }
    
    if(Filled)
    {
      /* emit segments - top */
      if(StartLineDir == -1)
      {
        if(EndLineDir == -1)
        {
          if(EndAngle < 360*64)
          {
            /* segment between two lines */
            XYL_p->PositionX = EndLineXInt;
            XYL_p->PositionY = yUp;
            XYL_p->Length = 1 + StartLineXInt - EndLineXInt;
            ++XYL_p;
          }
          else
          {
            /* two segments: arc-line, line-arc */
            if(EndLineXInt > StartLineXInt)
            {
              /* two distinct segments */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yUp;
              XYL_p->Length = 1 + StartLineXInt - xLeft;
              ++XYL_p;

              XYL_p->PositionX = EndLineXInt;
              XYL_p->PositionY = yUp;
              XYL_p->Length = 1 + xRight - EndLineXInt;
              ++XYL_p;
            }
            else
            {
              /* segments would overlap; just need to write one */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yUp;
              XYL_p->Length = 1 + xRight - xLeft;
              ++XYL_p;
            }
          }
        }
        else
        {
          /* arc-line segment */
          XYL_p->PositionX = xLeft;
          XYL_p->PositionY = yUp;
          XYL_p->Length = 1 + StartLineXInt - xLeft;
          ++XYL_p;
        }
      }
      else if(EndLineDir == -1)
      {
        /* line-arc segment */
        XYL_p->PositionX = EndLineXInt;
        XYL_p->PositionY = yUp;
        XYL_p->Length = 1 + xRight - EndLineXInt;
        ++XYL_p;
      }
      else if(UpArcOn)
      {
        /* arc-arc segment */
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yUp;
        XYL_p->Length = 1 + xRight - xLeft;
        ++XYL_p;
      }

      /* emit segments - bottom
       (replace Dir -1/+1, PositionY yUp/yDown, arc-line/line-arc) */
      if(StartLineDir == +1)
      {
        if(EndLineDir == +1)
        {
          if(EndAngle < 360*64)
          {
            /* segment between two lines */
            XYL_p->PositionX = StartLineXInt;
            XYL_p->PositionY = yDown;
            XYL_p->Length = 1 + EndLineXInt - StartLineXInt;
            ++XYL_p;
          }
          else
          {
            /* two segments: arc-line, line-arc */
            if(StartLineXInt > EndLineXInt)
            {
              /* two distinct segments */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yDown;
              XYL_p->Length = 1 + EndLineXInt - xLeft;
              ++XYL_p;

              XYL_p->PositionX = StartLineXInt;
              XYL_p->PositionY = yDown;
              XYL_p->Length = 1 + xRight - StartLineXInt;
              ++XYL_p;
            }
            else
            {
              /* segments would overlap; just need to write one */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yDown;
              XYL_p->Length = 1 + xRight - xLeft;
              ++XYL_p;
            }
          }
        }
        else
        {
          /* line-arc segment */
          XYL_p->PositionX = StartLineXInt;
          XYL_p->PositionY = yDown;
          XYL_p->Length = 1 + xRight - StartLineXInt;
          ++XYL_p;
        }
      }
      else if(EndLineDir == +1)
      {
        /* arc-line segment */
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yDown;
        XYL_p->Length = 1 + EndLineXInt - xLeft;
        ++XYL_p;
      }
      else if(DownArcOn)
      {
        /* arc-arc segment */
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yDown;
        XYL_p->Length = 1 + xRight - xLeft;
        ++XYL_p;
      }
    }
    else /* unfilled arc, adapted from above */
    {
      l = *Src_p;
      if(l == 0) l = 1;
      
      /* inflate to full length if segments reach XC */
      if(xLeft + l == XC) l = 1 + xRight - xLeft;
      
      /* emit segments - top */
      if(StartLineDir == -1)
      {
        if(EndLineDir == -1)
        {
          if(EndAngle < 360*64)
          {
            /* segment between two lines */
          }
          else
          {
            /* two segments: arc-line, line-arc */
            if((EndLineXInt > StartLineXInt) || (xLeft + l < XC))
            {
              /* two distinct segments */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yUp;
              XYL_p->Length = GFXMIN(l, 1+StartLineXInt-xLeft);
              ++XYL_p;

              XYL_p->PositionX = GFXMAX(EndLineXInt, xRight-(l-1));
              XYL_p->PositionY = yUp;
              XYL_p->Length = 1 + xRight - XYL_p->PositionX;
              ++XYL_p;
            }
            else
            {
              /* segments would overlap; just need to write one */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yUp;
              XYL_p->Length = l;
              ++XYL_p;
            }
          }
        }
        else
        {
          /* arc-line segment */
          XYL_p->PositionX = xLeft;
          XYL_p->PositionY = yUp;
          XYL_p->Length = GFXMIN(l, 1+StartLineXInt-xLeft);
          ++XYL_p;
        }
      }
      else if(EndLineDir == -1)
      {
        /* line-arc segment */
        XYL_p->PositionX = GFXMAX(EndLineXInt, xRight-(l-1));
        XYL_p->PositionY = yUp;
        XYL_p->Length = 1 + xRight - XYL_p->PositionX;
        ++XYL_p;
      }
      else if(UpArcOn)
      {
        /* arc-arc segment */
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yUp;
        XYL_p->Length = l;
        ++XYL_p;
        
        if(xLeft + l <= XC)
        {
          /* there's a second distinct segment */
          XYL_p->PositionX = xRight - (l-1);
          XYL_p->PositionY = yUp;
          XYL_p->Length = l;
          ++XYL_p;
        }
      }

      /* emit segments - bottom
       (replace Dir -1/+1, PositionY yUp/yDown, arc-line/line-arc) */
      if(StartLineDir == +1)
      {
        if(EndLineDir == +1)
        {
          if(EndAngle < 360*64)
          {
            /* segment between two lines */
          }
          else
          {
            /* two segments: arc-line, line-arc */
            if((StartLineXInt > EndLineXInt) || (xLeft + l < XC))
            {
              /* two distinct segments */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yDown;
              XYL_p->Length = GFXMIN(l, 1+EndLineXInt-xLeft);
              ++XYL_p;

              XYL_p->PositionX = GFXMAX(StartLineXInt, xRight-(l-1));
              XYL_p->PositionY = yDown;
              XYL_p->Length = 1 + xRight - XYL_p->PositionX;
              ++XYL_p;
            }
            else
            {
              /* segments would overlap; just need to write one */
              XYL_p->PositionX = xLeft;
              XYL_p->PositionY = yDown;
              XYL_p->Length = l;
              ++XYL_p;
            }
          }
        }
        else
        {
          /* line-arc segment */
          XYL_p->PositionX = GFXMAX(StartLineXInt, xRight-(l-1));
          XYL_p->PositionY = yDown;
          XYL_p->Length = 1 + xRight - XYL_p->PositionX;
          ++XYL_p;
        }
      }
      else if(EndLineDir == +1)
      {
        /* arc-line segment */
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yDown;
        XYL_p->Length = GFXMIN(l, 1+EndLineXInt-xLeft);
        ++XYL_p;
      }
      else if(DownArcOn)
      {
        /* arc-arc segment */
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yDown;
        XYL_p->Length = l;
        ++XYL_p;
        
        if(xLeft + l <= XC)
        {
          /* there's a second distinct segment */
          XYL_p->PositionX = xRight - (l-1);
          XYL_p->PositionY = yDown;
          XYL_p->Length = l;
          ++XYL_p;
        }
      }
    }
    
    /* advance lines (no consequence if lines not on) */
    StartLineXFrac += StartLineInc;
    EndLineXFrac += EndLineInc;
  }
  
  /*printf("stgfx_CalcAlignedSector loop exit at y = %i\n", yDown - YC);*/
  
  stgfx_CPUMEM_free(Handle, SrcArray);
  if(XYL_p - XYLArray > *NumXYL)
    return STGFX_ERROR_XYL_POOL_EXCEEDED;
    
  *NumXYL = XYL_p - XYLArray;
  return ST_NO_ERROR;
}
#endif /* ndef ST_GX1 */

/****************************************************************************\
Name        : stgfx_2dRasterSimpleArc()
Description : fills in an XYL Array for an arc having thickness = 1 
Parameters  : S32 XC, 
              S32 YC, 
              U32 HRadius, 
              U32 VRadius,
              S32 StartAngle, 
              U32 ArcAngle, 
              S32 Orientation,
              U32* NumXYL, 
              STBLIT_XYL_t*  XYLArray_p
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
\****************************************************************************/
ST_ErrorCode_t stgfx_2dRasterSimpleArc( 
      S32 XC, 
      S32 YC, 
      U32 HRadius, 
      U32 VRadius,
      S32 StartAngle, 
      U32 AAngle, 
      S32 Orientation,
      U32* NumXYL, 
      STBLIT_XYL_t*  XYLArray_p,
      S32* PixelPoints
)
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  U32                    LRadius;
  U32                    SRadius;
  U32                    AvailableSize = *NumXYL;
  S32                    ArcAngle;
  float                  XP=0;
  float                  YP=0;
  float                  XD;
  float                  YD;
  float                  XCenter;
  float                  YCenter;
  float                  LongRadius;
  float                  ShortRadius;
  float                  XS, YS, XE, YE;
  float                  A, B, C, D, E, F;
  S32                    EndAngle;

  if(HRadius >= VRadius)
  {
    LRadius = HRadius;
    SRadius = VRadius;
  }
  else
  {
    LRadius = VRadius;
    SRadius = HRadius;
    Orientation += (90*64);
    StartAngle -= (90*64);
  }
  CheckAngle(&Orientation);
  CheckAngle(&StartAngle);
  ArcAngle = AAngle;
  if(ArcAngle >= 360*64)
  {
    ArcAngle = 360*64;
  }
  else if(ArcAngle < 0)
  {
    ArcAngle = -ArcAngle;
  }
  if(StartAngle >= PI)
  {
     /* optimisation: avoid going > 180 degrees from P to S */
     Orientation += PI;
     StartAngle -= PI;
  }
  EndAngle = StartAngle + ArcAngle;
  /*CheckAngle(&EndAngle);*/
  
  TIME_HERE("2dRasterSimpleArc");
  
  XCenter = (float)XC / (1<<STGFX_INTERNAL_ACCURACY);
  YCenter = (float)YC / (1<<STGFX_INTERNAL_ACCURACY);
  LongRadius = (float)LRadius / (1<<STGFX_INTERNAL_ACCURACY);
  ShortRadius = (float)SRadius / (1<<STGFX_INTERNAL_ACCURACY);

  TIME_HERE("2dRasterSimpleArc after float conversions");
  
  CalcConicCoefficients(LongRadius, ShortRadius, Orientation, 
                        &A, &B, &C, &D, &E, &F, &XP, &YP, &XD, &YD);

  TIME_HERE("2dRasterSimpleArc after CalcConicCoefficients");
  
  CalcEllipsePoint(LongRadius, ShortRadius, StartAngle, &XS, &YS);         
  RotatePoint(Orientation, &XS, &YS);
  CalcEllipsePoint(LongRadius, ShortRadius, EndAngle, &XE, &YE);         
  RotatePoint(Orientation, &XE, &YE);

  TIME_HERE("2dRasterSimpleArc after CalcEllipsePoint*2");
  
  Err = Conic(XP, YP, XS, YS, XE, YE, A, B, C, D, E, F, 
              (XCenter-XD), (YCenter+YD), XD, YD,
              NumXYL, XYLArray_p, FALSE, NULL, FALSE);
  if(Err != ST_NO_ERROR)
  {
    STTBX_Print(("stgfx_2dRasterSimpleArc->Conic error 0x%08X\n", Err));
    return(Err);
  }
  if(*NumXYL > AvailableSize)
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }

  TIME_HERE("2dRasterSimpleArc after Conic");
  
  if((XYLArray_p[0].PositionY- FRACT_TO_INT(YC)) < 0)
  {
    PixelPoints[0] = (XYLArray_p[0].PositionX +
                 XYLArray_p[0].Length-1);
  }
  else
  {
    PixelPoints[0] = (XYLArray_p[0].PositionX); 
  }
  PixelPoints[1] = (XYLArray_p[0].PositionY);

  if((XYLArray_p[*NumXYL-1].PositionY - FRACT_TO_INT(YC)) < 0)
  {
    PixelPoints[2] = (XYLArray_p[*NumXYL-1].PositionX);
  }
  else
  {
    PixelPoints[2] = (XYLArray_p[*NumXYL-1].PositionX +
              XYLArray_p[*NumXYL-1].Length-1);
  }
  PixelPoints[3] = (XYLArray_p[*NumXYL-1].PositionY);

  PixelPoints[0] = PixelPoints[0]<<STGFX_INTERNAL_ACCURACY;
  PixelPoints[1] = PixelPoints[1]<<STGFX_INTERNAL_ACCURACY;
  PixelPoints[2] = PixelPoints[2]<<STGFX_INTERNAL_ACCURACY;
  PixelPoints[3] = PixelPoints[3]<<STGFX_INTERNAL_ACCURACY;
  
  return(ST_NO_ERROR);
}


/****************************************************************************\
Name        : stgfx_2dRasterThickArc()
Description : fills in 5 XYL Arrays for an arc having thickness > 1
              (the first array is for the inner arc, 
               the second one is for the outer arc,
               the third one is for the thickness,
               the fourth one is for the starting segment
               the last one is for the ending segment
Parameters  :  U32 BHeight, 
               S32 XC, 
               S32 YC, 
               U32 HRadius, 
               U32 VRadius,
               S32 StartAngle, 
               U32 ArcAngle, 
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
               S32* Points_p
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
\****************************************************************************/
ST_ErrorCode_t stgfx_2dRasterThickArc(  
      U32 BHeight, 
      S32 XC, 
      S32 YC, 
      U32 HRadius, 
      U32 VRadius,
      S32 StartAngle, 
      U32 ArcAngle, 
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
)
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  U32                    LRadius;
  U32                    SRadius;
  BOOL                   IsWidthOdd;
  U32                    OuterLRadius;
  U32                    OuterSRadius;
  U32                    InnerLRadius;
  U32                    InnerSRadius;
  U32                    AvailableInnerSize = *SizeInner;
  U32                    AvailableOuterSize = *SizeOuter;
  U32                    AvailableThicknessSize = *SizeThicknessArray;
  U32                    AvailableStartLineSize = *SizeSELineArray;
  U32                    AvailableEndLineSize = *SizeEELineArray;
  float                  XSIn;
  float                  YSIn;
  float                  XEIn;
  float                  YEIn;
  float                  XSOut;
  float                  YSOut;
  float                  XEOut;
  float                  YEOut;
  float                  LRadiusIn;
  float                  LRadiusOut;
  float                  SRadiusIn;
  float                  SRadiusOut;
  float                  XCenter;
  float                  YCenter;
  double                 NormalAngleStart;
  double                 NormalAngleEnd;
  S32                    StartAngleOuter;
  S32                    EndAngleOuter;
  S32                    NormalArcAngle;
  stgfx_Side_t           Sides[8];
  stgfx_Side_t*          Sides_p;
  float                  PointsInFloating[8];
  S32                    PixelPoints1[4];
  S32                    PixelPoints2[4];
  S32 EndAngle;
  if(HRadius >= VRadius)
  {
    LRadius = HRadius;
    SRadius = VRadius;
  }
  else
  {
    LRadius = VRadius;
    SRadius = HRadius;
    Orientation += (90*64);
    StartAngle -= (90*64);
  }
  Sides_p = &Sides[0];
  CheckAngle(&Orientation);
  CheckAngle(&StartAngle);
  if(ArcAngle >= 23040)
  {
    ArcAngle = 23040;
  }
  EndAngle = StartAngle + ArcAngle;
  /*CheckAngle(&EndAngle);*/
  XCenter = (float)XC /(1<<STGFX_INTERNAL_ACCURACY);
  YCenter = (float)YC /(1<<STGFX_INTERNAL_ACCURACY);
  /* INTERNAL ELLIPSE */
  /* line width is an even or an odd number? */
  IsWidthOdd = ((LineWidth % 2)== 0) ? FALSE : TRUE;
  /* calculates the internal ellipse axis, depending on LineWidth */
  if(IsWidthOdd)
  {
    InnerLRadius = LRadius - ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
    InnerSRadius = SRadius - ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
  }
  else /* LineWidth is even */
  {
    InnerLRadius = LRadius - ((LineWidth/2 - 1)<<STGFX_INTERNAL_ACCURACY);
    InnerSRadius = SRadius - ((LineWidth/2 - 1)<<STGFX_INTERNAL_ACCURACY);
  }
  /* fills in the internal ellipse array xyl */
  Err = stgfx_2dRasterSimpleArc( XC, YC, InnerLRadius,
                                 InnerSRadius, StartAngle, ArcAngle,
                                 Orientation,
                                 SizeInner, InnerArray_p,
                                 PixelPoints1);
  if(Err != ST_NO_ERROR) 
  {
    return(Err);
  }
  if( *SizeInner > AvailableInnerSize)
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }
  /* calculates the internal arc ending points coordinates */
  LRadiusIn = (float)InnerLRadius /(1<<STGFX_INTERNAL_ACCURACY);
  SRadiusIn = (float)InnerSRadius /(1<<STGFX_INTERNAL_ACCURACY);
  CalcEllipsePoint( LRadiusIn, SRadiusIn, 
                    StartAngle, &XSIn, &YSIn);         
  CalcEllipsePoint( LRadiusIn, SRadiusIn, 
                    EndAngle, &XEIn, &YEIn);
  if(GeometricPoints_p != NULL)
  {
    PointsInFloating[0] = XSIn;
    PointsInFloating[1] = YSIn;
    RotatePoint(Orientation, &(PointsInFloating[0]), 
                             &(PointsInFloating[1]));
    TranslatePoint(Orientation, LRadiusIn, XCenter, YCenter,
                   &(PointsInFloating[0]), &(PointsInFloating[1]));
    GeometricPoints_p[0] = (S32)(PointsInFloating[0]*
                                    (1<<STGFX_INTERNAL_ACCURACY));
    GeometricPoints_p[1] = (S32)(PointsInFloating[1]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
    PointsInFloating[4] = XEIn;
    PointsInFloating[5] = YEIn;
    RotatePoint(Orientation, &(PointsInFloating[4]), 
                             &(PointsInFloating[5]));
    TranslatePoint(Orientation, LRadiusIn, XCenter, YCenter,
                   &(PointsInFloating[4]), &(PointsInFloating[5]));
    GeometricPoints_p[4] = (S32)(PointsInFloating[4]*
                                 (1<<STGFX_INTERNAL_ACCURACY));
    GeometricPoints_p[5] = (S32)(PointsInFloating[5]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
  }
  NormalAngleStart = (double)atan(((YSIn/
                     ((double)XSIn-(double)LRadiusIn))*
                     (((double)LRadiusIn*(double)LRadiusIn) / 
                     ((double)SRadiusIn*(double)SRadiusIn)))); 
  NormalAngleEnd = (double)atan(( ( YEIn/
                   ((double)XEIn-(double)LRadiusIn))*
                   (((double)LRadiusIn*(double)LRadiusIn) / 
                   ((double)SRadiusIn*(double)SRadiusIn)))); 
  /*NormalAngleStart = (double)atan((double)(((double)YSIn/
                     ((double)XSIn-(double)LRadiusIn))*
                     (((double)LRadiusIn*(double)LRadiusIn) / 
                     ((double)SRadiusIn*(double)SRadiusIn)))); 
  NormalAngleEnd = (double)atan((double)( ( (double)YEIn/
                   ((double)XEIn-(double)LRadiusIn))*
                   (((double)LRadiusIn*(double)LRadiusIn) / 
                   ((double)SRadiusIn*(double)SRadiusIn))));*/ 
  CalcOutArcPoint( XSIn, YSIn, LRadiusIn, SRadiusIn, LineWidth, 
                  NormalAngleStart, &XSOut, &YSOut);
  CalcOutArcPoint( XEIn, YEIn, LRadiusIn, SRadiusIn, LineWidth, 
                  NormalAngleEnd, &XEOut, &YEOut);
  if(GeometricPoints_p != NULL)
  {
    PointsInFloating[2] = XSOut;
    PointsInFloating[3] = YSOut;
    RotatePoint(Orientation, &(PointsInFloating[2]), 
                             &(PointsInFloating[3]));
    TranslatePoint(Orientation, LRadiusIn, XCenter, YCenter,
                   &(PointsInFloating[2]), &(PointsInFloating[3]));
    GeometricPoints_p[2] = (S32)(PointsInFloating[2]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
    GeometricPoints_p[3] = (S32)(PointsInFloating[3]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
    PointsInFloating[6] = XEOut;
    PointsInFloating[7] = YEOut;
    RotatePoint(Orientation, &(PointsInFloating[6]), 
                             &(PointsInFloating[7]));
    TranslatePoint(Orientation, LRadiusIn, XCenter, YCenter,
                   &(PointsInFloating[6]), &(PointsInFloating[7]));
    GeometricPoints_p[6] = (S32)(PointsInFloating[6]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
    GeometricPoints_p[7] = (S32)(PointsInFloating[7]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
  }
  /*StartAngleOuter = FLOAT_TO_INT1((atan((double)(YSOut/
                    ((double)XSOut-(double)LRadiusIn)))*PI/PI_FLOAT)); 
  EndAngleOuter = FLOAT_TO_INT1((atan((double)(YEOut/
                  ((double)XEOut-(double)LRadiusIn)))*PI/PI_FLOAT)); */
  StartAngleOuter = FLOAT_TO_INT1((atan(YSOut/
                    ((double)XSOut-(double)LRadiusIn)))*PI/PI_FLOAT); 
  EndAngleOuter = FLOAT_TO_INT1((atan(YEOut/
                  ((double)XEOut-(double)LRadiusIn)))*PI/PI_FLOAT); 

/* the important thing is the direction, so if the angles are
   < 0 here is useful to keep the direction but also to make angles > 0
   by adding 180 * 64 */  
  if((XSIn -LRadiusIn) < 0)
  {
    if(GFXABS(StartAngleOuter) < PI)
    {
    StartAngleOuter += (PI);
    }
    while(StartAngleOuter < 0)
    {
      StartAngleOuter += (2*PI);
    }
  }
  if((XEIn - LRadiusIn) < 0)
  {
    if(GFXABS(EndAngleOuter) < PI)
    {
    EndAngleOuter += (PI);
    }
    while(EndAngleOuter < 0)
    {
      EndAngleOuter += (2*PI);
    }
  }
  /* EXTERNAL ELLIPSE */
  OuterLRadius = LRadius + ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
  OuterSRadius = SRadius + ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
  NormalArcAngle = (EndAngleOuter - StartAngleOuter);
  if(NormalArcAngle < 0)
  {
    NormalArcAngle += (2*PI);
  }
  Err = stgfx_2dRasterSimpleArc( XC, YC, OuterLRadius,
                                 OuterSRadius, 
                                 StartAngleOuter, 
                                 NormalArcAngle,
                                 Orientation,
                                 SizeOuter, OuterArray_p,
                                 PixelPoints2);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  if( *SizeOuter > AvailableOuterSize)
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }
  /* calculates the external arc ending points coordinates */
  LRadiusOut = (float)OuterLRadius /(1<<STGFX_INTERNAL_ACCURACY);
  SRadiusOut = (float)OuterSRadius /(1<<STGFX_INTERNAL_ACCURACY);
  /* it is the turn of the thickness array */
  if(LineWidth > 1)
  { 
     Err = Thickness( BHeight, 
                      XCenter, 
                      YCenter, 
                      FALSE, 
                      *SizeInner, 
                      InnerArray_p,
                      *SizeOuter, OuterArray_p,
                      SizeThicknessArray,
                      ThicknessArray_p,
                      SizeSELineArray,
                      SELineArray_p,
                      SizeEELineArray,
                      EELineArray_p,
                      Points_p ); 

     if( Err != ST_NO_ERROR)
     {
       return(Err);
     }
     if( (*SizeThicknessArray > AvailableThicknessSize) ||
         (*SizeSELineArray > AvailableStartLineSize) ||
         (*SizeEELineArray > AvailableEndLineSize))
     {
        return(STGFX_ERROR_XYL_POOL_EXCEEDED);
     }
  }/* end if( LineWidth > 1 ) */
  return(ST_NO_ERROR);
}


/****************************************************************************\
Name        : stgfx_2dRasterDashedArc()
Description : fills in 5 XYL Arrays for a dashed arc having thickness > 1
              (the first array is for the inner arc, 
               the second one is for the outer arc,
               the third one is for the thickness,
               the fourth one is for the starting segment
               the last one is for the ending segment
Parameters  :  U32 BHeight, 
               S32 XC, 
               S32 YC, 
               U32 HRadius, 
               U32 VRadius,
               S32 StartAngle, 
               U32 ArcAngle, 
               S32 Orientation,
               U8* LineStyle, pointer to an array ended with a zero
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
               S32* Points_p
               S32* GeometricPoints_p
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
\****************************************************************************/
ST_ErrorCode_t stgfx_2dRasterDashedArc( 
    U32 BHeight, 
    S32 XC, 
    S32 YC, 
    U32 HRadius, 
    U32 VRadius, 
    S32 StartAngle, 
    U32 ArcAngle, 
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
)
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  U32                    LRadius;
  U32                    SRadius;
  S32                    EndAngle;
  U32                    Size = 0;
  U32                    Size1 = 0;
  U32                    SizeThickness = 0;
  U32                    SizeSE = 0;
  U32                    SizeEE = 0;
  ConicStatus_t          StatusIn;
  ConicStatus_t          StatusOut;
  BOOL                   Draw = TRUE;
  BOOL                   IsWidthOdd;
  float                  XCenter;
  float                  YCenter;
  float                  InnerLRadius;
  float                  InnerSRadius;
  float                  OuterLRadius;
  float                  OuterSRadius;
  /*float                  Radius;*/
  float                  A, B, C, D, E, F;
  float                  XD, YD;
  float                  XP, YP;
  float                  XS, YS, XE, YE;
  float                  XSIn, YSIn, XEIn, YEIn;
  float                  XInnerEnd, YInnerEnd;
  float                  AOut, BOut, COut, DOut, EOut, FOut;
  float                  XDOut, YDOut;
  float                  XPOut, YPOut;
  float                  XSOut, YSOut, XEOut, YEOut;
  double                 NormalAngleStart;
  double                 NormalAngleEnd;
  S32                    i = 0;
  U32                    AvailableSizeInner;
  U32                    AvailableSizeOuter;
  U32                    AvailableSizeThickness;
  U32                    AvailableSizeStartLine;
  U32                    AvailableSizeEndLine;
  U32                    CurrentSizeInner = 0;
  U32                    CurrentSizeOuter = 0;
  U32                    CurrentSizeThickness = 0;
  U32                    CurrentSizeStartLine = 0;
  U32                    CurrentSizeEndLine = 0;
  S32                    Points[8];
  float                  PointsInFloating[8];
  
  
  AvailableSizeInner = *SizeInner;
  *SizeInner = 0;
  if(LineWidth > 1)
  {
    AvailableSizeOuter = *SizeOuter;
    AvailableSizeThickness = *SizeThicknessArray;
    AvailableSizeStartLine = *SizeSELineArray;
    AvailableSizeEndLine = *SizeEELineArray;
    *SizeOuter = 0;
    *SizeThicknessArray = 0;
    *SizeSELineArray = 0;
    *SizeEELineArray = 0;
  }
  if(ArcAngle >= 360*64)
  {
    ArcAngle = 360*64;
    StatusIn.IsEllipse = TRUE;
  }
  else
  {
    StatusIn.IsEllipse = FALSE;
  }
  if(HRadius >= VRadius)
  {
    LRadius = HRadius;
    SRadius = VRadius;
  }
  else
  {
    LRadius = VRadius;
    SRadius = HRadius;
    Orientation += (90*64);
    StartAngle -= (90*64);
  }
  CheckAngle(&Orientation);
  CheckAngle(&StartAngle);
  if(StartAngle >= PI)
  {
     Orientation += PI;
     StartAngle -= PI;
  }
  EndAngle = StartAngle + ArcAngle;
  XCenter = (float)XC / (float)(1<<STGFX_INTERNAL_ACCURACY);
  YCenter = (float)YC / (float)(1<<STGFX_INTERNAL_ACCURACY);
  if(LineWidth == 1)
  {
    InnerLRadius = (float)LRadius / (float)(1<<STGFX_INTERNAL_ACCURACY);
    InnerSRadius = (float)SRadius / (float)(1<<STGFX_INTERNAL_ACCURACY);
      /*InnerLRadius = LRadius<<STGFX_INTERNAL_ACCURACY;
      InnerSRadius = SRadius<<STGFX_INTERNAL_ACCURACY;*/
  }
  else
  {
    /* INTERNAL ARC */

    /* line width is an even or an odd number? */
    IsWidthOdd = ((LineWidth % 2)== 0) ? FALSE : TRUE;
    /* calculates the internal ellipse axis, depending on LineWidth */
    if(IsWidthOdd)
    {
     
      InnerLRadius = (float)((LRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) -
                     (((float)LineWidth/2)-(float)0.5));
      InnerSRadius = (float)((SRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) -
                     (((float)LineWidth/2)-(float)0.5));
      OuterLRadius = (float)((LRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) +
                     (((float)LineWidth/2)-(float)0.5));
      OuterSRadius = (float)((SRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) +
                     (((float)LineWidth/2)-(float)0.5));
    }
    else /* LineWidth is even */
    {
      InnerLRadius = (float)((LRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) - ((float)LineWidth/2 - 1));
      InnerSRadius = (float)((SRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) - ((float)LineWidth/2 - 1));
      OuterLRadius = (float)((LRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) + ((float)LineWidth/2));
      OuterSRadius = (float)((SRadius /
                     (float)(1<<STGFX_INTERNAL_ACCURACY)) + ((float)LineWidth/2));
     }

     CalcConicCoefficients(OuterLRadius, OuterSRadius, Orientation, 
                           &AOut, &BOut, &COut, &DOut, &EOut, &FOut, 
                           &XPOut, &YPOut, &XDOut, &YDOut);
  }
  
  CalcConicCoefficients(InnerLRadius, InnerSRadius, Orientation, 
                        &A, &B, &C, &D, &E, &F, &XP, &YP, &XD, &YD);

  /* XInnerEnd and YInnerEnd are the coordinate of the very last point 
     in the arc */
  CalcEllipsePoint(InnerLRadius, InnerSRadius, StartAngle, &XS, &YS);
  CalcEllipsePoint(InnerLRadius, InnerSRadius, EndAngle,
                   &XInnerEnd, &YInnerEnd);

  /* DeltaXEnd and DeltaYEnd will be used to check if the current 
     point is over the line between the center and the last point */
  StatusIn.Counter = 0;
  StatusIn.IsFirstOctant = TRUE;
  StatusIn.IsCompleted = FALSE;
  RotatePoint(Orientation, &XInnerEnd, &YInnerEnd);
  StatusIn.XE = XInnerEnd;
  StatusIn.YE = YInnerEnd;
  StatusIn.AvailableSize = AvailableSizeInner;
  StatusIn.IsInnerArc = TRUE;
  StatusIn.IsTheVeryLast = FALSE;
  XSIn = XS;
  YSIn = YS;
  RotatePoint(Orientation, &XS, &YS);
  XE = XInnerEnd;
  YE = YInnerEnd;
  if(GeometricPoints_p != NULL)
  {
    PointsInFloating[0] = XS;
    PointsInFloating[1] = YS;
    TranslatePoint(Orientation, InnerLRadius, XCenter, YCenter,
                   &(PointsInFloating[0]), &(PointsInFloating[1]));
    GeometricPoints_p[0] = (S32)(PointsInFloating[0]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
    GeometricPoints_p[1] = (S32)(PointsInFloating[1]*
                                (1<<STGFX_INTERNAL_ACCURACY));
  }
  StatusOut.Counter = 0;
  StatusOut.AvailableSize = AvailableSizeOuter;
  /*CalcRadius(InnerLRadius, InnerSRadius, StartAngle, &Radius);*/
  
  /*while (StartAngle < EndAngle)*/
  while(!StatusIn.IsCompleted)
  {
    if(StatusIn.Counter > 0)
    {
      XS = StatusIn.XE;
      YS = StatusIn.YE;
    }

    Size = AvailableSizeInner - CurrentSizeInner;
    if(Draw)
    {
      StatusIn.Length = LineStyle[i] - 1;
    }
    else
    {
      StatusIn.Length = LineStyle[i] + 1;
    }
    Err = Conic( XP, YP, XS, YS, XE, YE, A, B, C, D, E, F, 
                 (XCenter-XD), (YCenter+YD), XD, YD, 
                 &Size, InnerArray_p, TRUE, &StatusIn, FALSE );
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
    if(Draw == TRUE)
    {
      CurrentSizeInner += Size;
      if (CurrentSizeInner > AvailableSizeInner)
      {
         return(STGFX_ERROR_XYL_POOL_EXCEEDED);
      }
    }
    
/******* OUTER ARC ******************************************/
    if(LineWidth>1)
    {
      if(Size == 0)
      {/* there is no inner arc to be thickened */
        return(ST_ERROR_BAD_PARAMETER);
      }
      if(StatusOut.Counter == 0) /* first time */
      {
        /* No RotatePoint() as XSIn, YSIn were taken before original rotation */
        NormalAngleStart = (double)atan(((double)YSIn /
                                ((double)XSIn-(double)InnerLRadius))*
                               (((double)InnerLRadius*(double)InnerLRadius) / 
                                ((double)InnerSRadius*(double)InnerSRadius)));
        CalcOutArcPoint( XSIn, YSIn, InnerLRadius, InnerSRadius, LineWidth, 
                         NormalAngleStart, &XSOut, &YSOut );

        /* before rotating, moves the point to the external 
           ellipse system coordinates */
        XSOut += (float)(U16)(LineWidth-1);

        RotatePoint(Orientation, &XSOut, &YSOut);
        if(GeometricPoints_p != NULL)
        {
          PointsInFloating[2] = XSOut;
          PointsInFloating[3] = YSOut;
          TranslatePoint(Orientation, OuterLRadius, XCenter, YCenter,
                         &(PointsInFloating[2]), &(PointsInFloating[3]));
          GeometricPoints_p[2] = (S32)(PointsInFloating[2]*
                                  (1<<STGFX_INTERNAL_ACCURACY));
          GeometricPoints_p[3] = (S32)(PointsInFloating[3]*
                                 (1<<STGFX_INTERNAL_ACCURACY));
        }
      }
      else
      {
        XSOut = StatusOut.XE;
        YSOut = StatusOut.YE;
      }
      XEIn = StatusIn.XE;
      YEIn = StatusIn.YE;

      RotatePoint(-Orientation, &XEIn, &YEIn);
      NormalAngleEnd = (double)atan( ((double)YEIn /
                           ((double)XEIn-(double)InnerLRadius))*
                          (((double)InnerLRadius*(double)InnerLRadius) / 
                           ((double)InnerSRadius*(double)InnerSRadius)));
      CalcOutArcPoint( XEIn, YEIn, InnerLRadius, InnerSRadius, LineWidth,
                       NormalAngleEnd, &XEOut, &YEOut );
      XEOut += (float)(U16)(LineWidth-1);
      RotatePoint(Orientation, &XEOut, &YEOut);
      Size1 = AvailableSizeOuter - CurrentSizeOuter;
      StatusOut.IsCompleted = StatusIn.IsCompleted;
      StatusOut.LastOctant = StatusIn.LastOctant;
      StatusOut.EdSdx = StatusIn.EdSdx;
      StatusOut.EdSdy = StatusIn.EdSdy;
      StatusOut.X0 = StatusIn.X0;
      StatusOut.Y0 = StatusIn.Y0;
      StatusOut.IsInnerArc = FALSE;
      Err = Conic( XPOut, YPOut, XSOut, YSOut, XEOut, YEOut, 
                   AOut, BOut, COut, DOut, EOut, FOut, 
                   (XCenter-XDOut), (YCenter+YDOut), XD, YD,
                   &Size1, OuterArray_p, TRUE, &StatusOut, FALSE );
      if(Err != ST_NO_ERROR)
      {
        return(Err);
      }
      if(Draw == TRUE)
      {
        CurrentSizeOuter += Size1;
        if(CurrentSizeOuter > AvailableSizeOuter)
        {
          return(STGFX_ERROR_XYL_POOL_EXCEEDED);
        }
      }
      else
      {
        if(!(StatusOut.IsCompleted))
          Size1 = 0;
      }
      
      /* used to have || StatusOut.IsCompleted, but Thickness can't work
        if there are no outer arc pixels */
      if(Size1 > 0)
      {
        SizeThickness = AvailableSizeThickness - CurrentSizeThickness;
        SizeSE = AvailableSizeStartLine - CurrentSizeStartLine;
        SizeEE = AvailableSizeEndLine - CurrentSizeEndLine;
        Err = Thickness( BHeight, XCenter, YCenter, FALSE, 
                         Size, InnerArray_p,
                         Size1, OuterArray_p,
                         &SizeThickness, ThicknessArray_p,
                         &SizeSE, SELineArray_p,
                         &SizeEE, EELineArray_p, 
                         Points );
        if(Err != ST_NO_ERROR)
        {
          return(Err);
        }
        if(StatusIn.Counter == 1)
        {
          Points_p[0] = Points[0];
          Points_p[1] = Points[1];
          Points_p[2] = Points[2];
          Points_p[3] = Points[3];
        }
        if(Draw == TRUE)
        {
          CurrentSizeThickness += SizeThickness;                   
          CurrentSizeStartLine += SizeSE;
          CurrentSizeEndLine += SizeEE;
          if(CurrentSizeThickness > AvailableSizeThickness ||
             CurrentSizeStartLine > AvailableSizeStartLine ||
             CurrentSizeEndLine > AvailableSizeEndLine )
          {
             return(STGFX_ERROR_XYL_POOL_EXCEEDED);
          }
        }
      }
      if(GeometricPoints_p != NULL)
      {
         PointsInFloating[6] = StatusOut.XE;
         PointsInFloating[7] = StatusOut.YE;
         TranslatePoint(Orientation, OuterLRadius, XCenter, YCenter,
                        &(PointsInFloating[6]), &(PointsInFloating[7]));
         GeometricPoints_p[6] = (S32)(PointsInFloating[6]*
                                 (1<<STGFX_INTERNAL_ACCURACY));
         GeometricPoints_p[7] = (S32)(PointsInFloating[7]*
                                (1<<STGFX_INTERNAL_ACCURACY));
        }
      }
      if(LineWidth == 1 && Points_p != NULL)
      {
        if(StatusIn.Counter == 1)
        {
          if((InnerArray_p[0].PositionY- FRACT_TO_INT(YC)) < 0)
          {
            Points_p[0] = (InnerArray_p[0].PositionX +
                           InnerArray_p[0].Length-1);
          } 
          else
          {
            Points_p[0] = (InnerArray_p[0].PositionX); 
          }
          Points_p[1] = (InnerArray_p[0].PositionY);
          Points_p[0] = Points_p[0]<<STGFX_INTERNAL_ACCURACY;
          Points_p[1] = Points_p[1]<<STGFX_INTERNAL_ACCURACY;
          
        }
        if((InnerArray_p[Size-1].PositionY - FRACT_TO_INT(YC)) < 0)
        {
          Points_p[2] = (InnerArray_p[Size-1].PositionX);
        }
        else
        {
          Points_p[2] = (InnerArray_p[Size-1].PositionX +
                            InnerArray_p[Size-1].Length-1);
        }
        Points_p[3] = (InnerArray_p[Size-1].PositionY);
        Points_p[2] = Points_p[2]<<STGFX_INTERNAL_ACCURACY;
        Points_p[3] = Points_p[3]<<STGFX_INTERNAL_ACCURACY;
      }
/******** end of OUTER ARC **********************************/
    if((LineWidth >1) && (Size1 > 0))
    {
      Points_p[4] = Points[4];
      Points_p[5] = Points[5];
      Points_p[6] = Points[6];
      Points_p[7] = Points[7];

      if(Draw == FALSE)
      {
        Size1 = 0;
      }
    }
    if(Draw == TRUE)
    {
      InnerArray_p += Size;
      if(LineWidth > 1)
      {
        OuterArray_p += Size1;
        ThicknessArray_p += SizeThickness;
        SELineArray_p += SizeSE;
        EELineArray_p += SizeEE;
      }
    }
    
    if( LineStyle[i+1] == 0 )
    {
      i = 0;
      Draw = TRUE;
    }
    else
    {
      i++;
      Draw = ( Draw == TRUE ) ? FALSE : TRUE;
    }
 /*   StartAngle += AAngle;*/
  } /* end while(SAngle <= EndAngle) */
  
  *SizeInner = CurrentSizeInner;
  if(LineWidth > 1)
  {
    *SizeOuter = CurrentSizeOuter;
    *SizeThicknessArray = CurrentSizeThickness;
    *SizeSELineArray = CurrentSizeStartLine;
    *SizeEELineArray = CurrentSizeEndLine;
  }
  if(GeometricPoints_p != NULL)
  {
    PointsInFloating[4] = StatusIn.XE;
    PointsInFloating[5] = StatusIn.YE;
    TranslatePoint(Orientation, InnerLRadius, XCenter, YCenter,
                   &(PointsInFloating[4]), &(PointsInFloating[5]));
    GeometricPoints_p[4] = (S32)(PointsInFloating[4]*
                            (1<<STGFX_INTERNAL_ACCURACY));
    GeometricPoints_p[5] = (S32)(PointsInFloating[5]*
                            (1<<STGFX_INTERNAL_ACCURACY));
  }
  
  return(Err);
}


/****************************************************************************\
Name        : Conic()
Description : fills in the xyl Array of continuous
              arcs/ellipses having line width = 1
              This functions uses floating point numbers.
              It is called by
                   stgfx_2dRasterSimpleEllipse,
                   stgfx_2dRasterThickEllipse,
                   stgfx_2dRasterSimpleArc,
                   stgfx_2dRasterThickArc,
                   stgfx_2dRasterDashedArc
Parameters  :  
             float XP, x coord of the starting point P of the calculation of
                       the points of every arc/ellipse
                       (P is always at the end of the maximum axis)
             float YP, y coord of P
             float XS, x coord of the starting point S to be drawn of any arc
             float YS, y coord of S
             float XE, x coord of the ending point E to be drawn of any arc
             float YE, y coord of E
             float A, coeff. of the x^2 term of the conic equation
             float B, coeff. of the xy term of the conic equation 
             float C, coeff. of the y^2 term of the conic equation
             float D, coeff. of the x term of the conic equation
             float E, coeff. of the y term of the conic equation
             float F, coeff. of the known term of the conic equation
             float XC, x coord of the Center, suitably translated in order
                       to be in the center of the ellipse 
                       (this term allows the translation of each point)
             float YC, y coord of the same point
             float XD,  
             float YD,
             U32* NumXYL, the size of the xyl array
             STBLIT_XYL_t*  XYLArray_p, the pointer to the xyl array 
             BOOL RecStatus, TRUE if we should read and write Status_p
             ConicStatus_t* Status_p, the pointer to the recorded status 
             BOOL IsJoin in case of the caller is 2dRasterJoin 
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
\****************************************************************************/
ST_ErrorCode_t Conic(
             float xp, float yp,
             float xs, float ys,
             float xe, float ye,
             float A, float B, float C, float D, float E, float F,
             float xc, float yc,
             float xd, float yd,
             U32* NumXYL, STBLIT_XYL_t*  XYLArray_p,
             BOOL RecStatus, ConicStatus_t* Status_p,
             BOOL IsJoin
)
{
  /* We track an implicit equation to decide which pixels to set. This equation
    is evaluated at xp, yp in our local co-ordinate system, and at integer
    offsets from it. We track the integer part of our position in the local
    co-ordinate system in X, Y */

  S32 X, Y; /* current point */
  S32 XP = FLOAT_TO_INT1(xp), YP = FLOAT_TO_INT1(yp);
  S32 XS = FLOAT_TO_INT1(xs), YS = FLOAT_TO_INT1(ys);
  S32 XE = FLOAT_TO_INT1(xe), YE = FLOAT_TO_INT1(ye);

  /* To generate XYL elements, we need to convert from our local co-ordinate
    syetem to screen co-ordinates. Before rouding, we should take account of the
    offset of our co-ordinate system from screen co-ordinates, and the constant
    fractional offset of the X, Y we will add from the actual points we evaluate
    the implicit equation at. This offset is ((xp-XP), (YP-yp)). Note the
    different sign for y arises from the fact it is positive going up in local
    co-ordinates, but positive going down in screen co-ordinates */

  S32 XC = FLOAT_TO_INT1(xc+xp)-XP, YC = FLOAT_TO_INT1(yc-yp)+YP;
  S32 XD = FLOAT_TO_INT1(xd), YD = FLOAT_TO_INT1(yd);
  S32 PreviousX, PreviousY;

  S32  Octant = 1; /* current octant */
  S32  dxsquare, dysquare;
  S32  dxdiag, dydiag;
  float  x, y, d, u, v, k1, k2, k3;
  float  dSdx, dSdy;
#if 0 /* JF: don't believe these are really needed */
  S32  XMin, YMin, XMax, YMax;
  S32  X_Min, Y_Min, X_Max, Y_Max;
#endif
  float  dSdxEnd, dSdyEnd;
  S32  X0, Y0;
  /*float  XCenter, YCenter; - unused */
  S32 X1Line, Y1Line, X2Line, Y2Line;
  /* DeltaSX, DeltaSY, DeltaEX, DeltaEY,
     StartX, StartY, EndX, EndY, are all
     related to the first point to be really
     drawn (S) and to the last one (E). In case of
     ellipse, the first point to be drawn
     is always the first point for the
     calculus, P (in this case P = S) */
  S32  DeltaSX = 0;
  S32  DeltaSY = 0;
  S32  DeltaEX = 0;
  S32  DeltaEY = 0;
  S32  StartX = 0;
  S32  StartY = 0;
  S32  EndX = BIG_NUMBER;
  S32  EndY = BIG_NUMBER;
  S32  OctantCount; /* number of octants remaining after current one */
  S32  LastOctant;
  S32  FirstOctant = -1;
  S32  i = 0;
  S32  tmp;
  S32  Length = 0;
  U32  AvailableSize;
  U32  Size;
  BOOL IsFirst = TRUE; /* set to FALSE as we enter first octant to draw in */
  BOOL IsLast = FALSE; /* set to TRUE as we enter last octant to draw in */
  BOOL IsArc = FALSE;
  BOOL EndImmediatly = FALSE;
  BOOL DrawCondition = FALSE;
  BOOL DrawInnerDashedArc = TRUE;
  BOOL Condition = TRUE;
  float  CoeffA = 0;
  float  CoeffB = 0;
  S32 Counter = 0;
  
  
  /* the first point is the first point P of any
     ellipse/arc which is the intersections between
     the major axis with the ellipse, on the right hand.
     The first point P is not the starting point of an
     arc, it is the starting point for calculating
     every ellipse/arc */
  
#if 0 /* JF: don't believe these are really needed */   
  if(XP > 0)
  {
    XMax = XP;
    XMin = 0;
  }
  else
  {
    XMax = 0;
    XMin = XP;
  }
  if(YP > 0)
  {
    YMax = YP;
    YMin = 0;
  }
  else
  {
    YMax = 0;
    YMin = YP;
  }
  
  /* JF: these are for dubious use in 'trimming' at the end */
  X_Min = GFXMIN(XMin + XC, XMax + XC);
  X_Max = GFXMAX(XMin + XC, XMax + XC);

  Y_Min = GFXMIN(-YMin + YC, -YMax + YC);
  Y_Max = GFXMAX(-YMin + YC, -YMax + YC);
#endif

  if(RecStatus == FALSE)
  {
    AvailableSize = *NumXYL;
  }
  else
  {
    AvailableSize = Status_p->AvailableSize;
    if((Status_p->IsInnerArc) && (Status_p->Counter == 0))
    {
      x = Status_p->XE;
      y = Status_p->YE;
      Status_p->X0 = (float)FLOAT_TO_INT1(x);
      Status_p->Y0 = (float)FLOAT_TO_INT1(y);
      dSdxEnd = 2*A*(x-xp) + B*(y-yp) + D;
      dSdyEnd = B*(x-xp) + 2*C*(y-yp) + E;
      GetOctant(dSdxEnd, dSdyEnd, &(Status_p->LastOctant));
      /*XCenter = XC + XD; - unused */
      /*YCenter = YC - YD; - unused */
    }
  }
  if((RecStatus == FALSE) || (Status_p->Counter == 0))
  {
#if 0 /* It is completely barmy to clear YP and not XP; Orientation adjustment
        if StartAngle > 180*64 probably does what was intended here */
    if(YS == 0)
    {
      YP = 0; /* JF: why? */
    }
    /*else if(((YP < (float)0.05) && (YP > 0)) ||
           ((YP > (float)(-0.05)) && (YP < 0)))
    {
      YP = 0;
    }*/
#endif
    
    /* the gradient components allow to discover in which octant is the first
      point P ( it depends on the ellipse orientation ). Note we must use the
      unrounded point to avoid errors on very narrow ellipses. */
    dSdx = 2*A*xp + B*yp + D;
    dSdy = B*xp + 2*C*yp + E;
    GetOctant(dSdx,dSdy, &Octant);

    if(((XS == XE)&&(YS == YE)) ||
       (RecStatus && Status_p->IsInnerArc && Status_p->IsEllipse))
    {
      /* it is an ellipse */
      DeltaSX = BIG_NUMBER;
      DeltaSY = BIG_NUMBER;
      StartX = 0;
      StartY = 0;
      FirstOctant = Octant;
    } else {                   /* it is an arc from S to E */
      x = (xp - xs);  /* JF: why P-S rather than S-D ?? */
      y = (yp - ys);
      if(!IsJoin && (x != 0 && y != 0)) /* why? */
      {
        x = DECREMENT(xp - xs);
        y = DECREMENT(yp - ys);
      }
      IsArc = TRUE;
      dSdx = 2*A*(-x) + B*(-y) + D;
      dSdy = B*(-x) + 2*C*(-y) + E;
      GetOctant(dSdx, dSdy, &FirstOctant);
      StartX = BIG_NUMBER;
      StartY = BIG_NUMBER;
      DeltaSX = FLOAT_TO_INT1(x);
      DeltaSY = FLOAT_TO_INT1(y);
    }

    /* After discovering the octant of the first point, P,
       it is possible to get the suitable initialization of each
       parameter. The InitConicParameters if useful just to the
       first initialization,
       after it, there is a fast way to pass from each octant to
       the following one (that is from each even octant to the
       following odd, and vice versa) */

    if(Octant <1 || Octant > 8)
    {
      if(RecStatus == TRUE)
      {
        Status_p->IsCompleted = TRUE;
      }
      return(ST_ERROR_BAD_PARAMETER);
    }
    else
    {
      InitConicParameters( Octant,  A,  B, C, D, E, F, &d,
                           &u, &v, &k1, &k2, &k3, &dxsquare, &dysquare,
                           &dxdiag, &dydiag );
    }
  }
  else if(RecStatus == TRUE) /* so there is a status to be retrieved */
  {
    /* retrieve the last conic status */
    Status_p->XS = (float)XS;
    Status_p->YS = (float)YS;
    d = Status_p->d;
    u = Status_p->u;
    v = Status_p->v;
    k1 = Status_p->k1;
    k2 = Status_p->k2;
    k3 = Status_p->k3;
    Octant = Status_p->octant;
    dxsquare = Status_p->dxsquare;
    dysquare = Status_p->dysquare;
    dxdiag = Status_p->dxdiag;
    dydiag = Status_p->dydiag;
    i = 0;
    DeltaSX = BIG_NUMBER;
    DeltaSY = BIG_NUMBER;
    StartX = 0;
    StartY = 0;
    IsArc = TRUE;
    /*dSdx = 2*A*(XS-XP) + B*(YS-YP) + D;
      dSdy = B*(XS-XP) + 2*C*(YS-YP) + E;
      GetOctant(dSdx, dSdy, &FirstOctant);*/
    FirstOctant = Octant;
  }

  /* now, we need to know which is the octant of the last point, E, of an
    ellipse/arc. octantCount is the total number of the involved octants.
    JF: why E-P rather than E-D ?? */
  x = xe - xp;
  y = ye - yp;
  dSdx = 2*A*x + B*y + D;
  dSdy = B*x + 2*C*y + E;
  GetOctant(dSdx, dSdy, &LastOctant);
  
  /* The total number of new octants we have to enter is the number of new
    octants moving from P to S plus the number of new octants from S to E */

  OctantCount = FirstOctant - Octant;
  if(OctantCount < 0) OctantCount += 8; /* handle wrap */
  /* if same octant, but P after S, add 8
    - can't happen since Orientation is adjusted to keep
          the resulting StartAngle < 180*64 */

  tmp = LastOctant - FirstOctant;
  if(tmp < 0) tmp += 8; /* handle wrap */
  /*if((tmp == 0) && (EndAngle <= StartAngle))*/
  if((tmp == 0) && ((XE-XD)*(YS-YD) - (XS-XD)*(YE-YD) >= 0))
  {
      /* handle end before start within the same octant */
      /* The cross product identifies if the angle (relative to D, the centre
        of the ellipse) from E to S is less than 180 degrees. Since tmp == 0
        means they are in the same quadrant, this is a check that E comes
        before S. We assume the caller wants a whole ellipse if E = S */
      tmp = 8;
  }

  OctantCount += tmp;

  if((RecStatus == TRUE) && Status_p->IsInnerArc && (Status_p->Counter == 0))
  {
    Status_p->TotalOctantCount = OctantCount;
  }
  
  /* now we actually calculate the curve */
  /* the starting point is always the first Point, P */
  X = XP;
  Y = YP;

  if((RecStatus == TRUE)&&(Status_p->Counter > 0))
  {
    /* if it is the inner arc, store the gradient's
      components in the EndPoint */

    /* retrieve the last conic status */
    X = FLOAT_TO_INT1(Status_p->x);
    Y = FLOAT_TO_INT1(Status_p->y);
    Status_p->XS = (float)X;
    Status_p->YS = (float)Y;
  }
  
  if(RecStatus && Status_p->IsInnerArc)
  {
    Status_p->X0 = (float)XE;
    Status_p->Y0 = (float)YE;
    X0 = FLOAT_TO_INT1(Status_p->X0);
    Y0 = FLOAT_TO_INT1(Status_p->Y0);
    if(!(Status_p->IsTheVeryLast) &&
      (Octant == Status_p->LastOctant)&&
        (Status_p->Counter > 0))
    {
      if(!(Status_p->IsEllipse) ||
        (Status_p->IsEllipse && !Status_p->IsFirstOctant))
        {
          Status_p->XSLastOctant = XS;
          Status_p->YSLastOctant = YS;
          Status_p->IsTheVeryLast = TRUE;
        }
    }
  }
  
  PreviousX = X;
  PreviousY = Y;

  /* in the while loop all the octants are covered passing
     from an even octant to the following odd one, and
     vice versa. This is possible just setting the
     parameters d, u, v, etc. */
  if(RecStatus)
  {
    X0 = FLOAT_TO_INT1(Status_p->X0);
    Y0 = FLOAT_TO_INT1(Status_p->Y0);
    CoeffA = Status_p->EdSdy / Status_p->EdSdx;
    CoeffB = (CoeffA * XE) - YE;
  }
  if(RecStatus && Status_p->IsInnerArc)
  {
    Condition = (Counter <= Status_p->Length);
  }
  else
  {
    Condition = (OctantCount >= 0 );
  }
  while ( Condition )
  {
    if(IS_ODD(Octant))
    {
      if((Octant == FirstOctant) && (IsFirst == TRUE))
      {
        StartX = X - XS;
        StartY = Y - YS;
        /*StartX += StartX < 0 ? 0.2F : StartX > 0 ? -0.2F : 0;*/
        /*StartY += StartY < 0 ? 0.2F : StartY > 0 ? -0.2F : 0;*/
        DeltaSX = 0;
        DeltaSY = 0;
        IsFirst = FALSE;
        if(OctantCount == 0)
        {
          EndX = (X - XE);
          EndY = (Y - YE);
          DeltaEX = 0;
          DeltaEY = 0;
          IsLast = TRUE;
        }
      }
      while(v <= k2/2)  /* when v > k2/2 the octant changes */
      {/*(S32)(GFXABS(DeltaSX)+(float)0.5)*/
        if(IsLast && RecStatus && !Status_p->IsInnerArc)
        {
          DrawCondition = ((((float)Y-( (float)X*CoeffA ) + CoeffB) >= 0) &&
                           (((float)YS-( (float)XS*CoeffA ) + CoeffB) >= 0))||
                          ((((float)Y-( (float)X*CoeffA ) + CoeffB) < 0) &&
                           (((float)YS-( (float)XS*CoeffA ) + CoeffB) < 0));
        }
        else if(IsLast && !RecStatus)
        {
          /* JF: I suspect this (rather than exact hitting of XE, YE below)
            is what most commonly causes us to stop at an endpoint */
          DrawCondition = ((GFXABS(DeltaEX) <= GFXABS(EndX)) &&
                           (GFXABS(DeltaEY) <= GFXABS(EndY)));
        }
        else
        {
          DrawCondition = TRUE;
        }
        /* JF changed to allow a co-ord to miss by one if the other has passed
          its boundary. This is important when we start/end near an extremum,
          because the integer values written don't always hit those that come
          from rounding thee calculated endpoints */
        if(HIT_BOUNDARY(DeltaSX, DeltaSY, StartX, StartY) && DrawCondition)
        {
          DeltaSX = BIG_NUMBER;
          DeltaSY = BIG_NUMBER;
          if(i==0)
          {
            /*x = XS; this can slightly shift the original ellipse y = YS;*/
            if( (i+1) > AvailableSize)
            {
              if(RecStatus == TRUE)
              {
                Status_p->IsCompleted = TRUE;
              }
              return(STGFX_ERROR_XYL_POOL_EXCEEDED);
            }
            else
            {
              XYLArray_p[i].PositionX = X+XC;
              XYLArray_p[i].PositionY = -Y+YC;
              Length = 1;
              i++;
            }
          }
          else if((Y == PreviousY) && (X == PreviousX + 1))
          {
            Length++;
          }
          else if((Y == PreviousY) && (X == (PreviousX - 1)))
          {
            XYLArray_p[i-1].PositionX -= 1;
            Length++;
          }
          else /* y !=  PreviousY */
          {
            XYLArray_p[i-1].Length = Length;
            if((i+1) > AvailableSize)
            {
              if(RecStatus == TRUE)
              {
                Status_p->IsCompleted = TRUE;
              }
              return(STGFX_ERROR_XYL_POOL_EXCEEDED);
            }
            else
            {
              XYLArray_p[i].PositionX = X+XC;
              XYLArray_p[i].PositionY = -Y+YC;
              Length = 1;
              i++;
            }
          }
          if(RecStatus == TRUE)
          {
            Status_p->x = (float)X;
            Status_p->y = (float)Y;
            Status_p->d = d;
            Status_p->u = u;
            Status_p->v = v;
            Status_p->k1 = k1;
            Status_p->k2 = k2;
            Status_p->k3 = k3;
            Status_p->octant = Octant;
            Status_p->dxsquare = dxsquare;
            Status_p->dysquare = dysquare;
            Status_p->dxdiag = dxdiag;
            Status_p->dydiag = dydiag;
          }

          if(RecStatus && (Status_p->IsInnerArc))
          {
            if((Octant == Status_p->LastOctant) &&
               (Status_p->IsTheVeryLast))
            {
            DrawInnerDashedArc =
             ((((float)Y - Status_p->Y0 - (((float)X - Status_p->X0)
              *((float)YD - Status_p->Y0)/((float)XD - Status_p->X0)) >= 0) &&
              ((float)YS - Status_p->Y0 - (((float)XS - Status_p->X0)
              *((float)YD - Status_p->Y0)/((float)XD - Status_p->X0)) >= 0)) ||
              (((float)Y - Status_p->Y0 - (((float)X - Status_p->X0)
              *((float)YD -Status_p->Y0)/((float)XD - Status_p->X0)) < 0) &&
              ((float)YS - Status_p->Y0 - (((float)XS - Status_p->X0)
              *((float)YD - Status_p->Y0)/((float)XD - Status_p->X0)) < 0)));

              if(!DrawInnerDashedArc)
              {
                Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
                Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
                Status_p->IsCompleted = TRUE;
                EndImmediatly = TRUE;
                break;
              }
            }
            Counter++;
            if(Counter > Status_p->Length)
            {
              Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
              Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
              EndImmediatly = TRUE;
              break;
            }
          }
        }
        else if(IsLast && !DrawCondition)
        {
          if(RecStatus && (Status_p->IsInnerArc))
          {
            Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
            Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
            /*if(Octant == Status_p->LastOctant)
            {
              Status_p->IsCompleted = TRUE;
            }*/
          }
          EndImmediatly = TRUE;
          break;
        }
        if(IsArc && IsLast && (X == XE) && (Y == YE))
        {
          if(RecStatus && (Status_p->IsInnerArc))
          {
            Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
            Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
            if( Octant == Status_p->LastOctant &&
                Status_p->IsTheVeryLast )
            {
              Status_p->IsCompleted = TRUE;
            }
          }
          EndImmediatly = TRUE;
          break;
        }
        
        /* d is the discriminant for each movement. The extra conditions ensure
          we don't step completely across a narrow ellipse - Foley fig 19.35 */
        if((d < 0) ||
           ((u - k2/2) > 0 && (u + k2/2) < 0) ||
           ((u - k2/2) < 0 && (u + k2/2)> 0))
        {
          PreviousX = X;
          PreviousY = Y;
          X += dxsquare;
          Y += dysquare;
          DeltaSX += dxsquare;
          DeltaSY += dysquare;
          DeltaEX += dxsquare;
          DeltaEY += dysquare;
          u += k1;
          v += k2;
          d += u;
        }
        else
        {
          PreviousX = X;
          PreviousY = Y;
          X += dxdiag;
          Y += dydiag;
          DeltaSX += dxdiag;
          DeltaSY += dydiag;
          DeltaEX += dxdiag;
          DeltaEY += dydiag;
          u += k2;
          v += k3;
          d += v;
        }
      }/* end while( v <= k2/2) */
      
      if(EndImmediatly)
      {
        break;
      }
      d = (d - u + v/2 - k2/2 + 3 * k3/8);
      u = (-u + v - k2/2 + k3/2);
      v = (v - k2 + k3/2);
      k1 = k1 -2*k2 + k3;
      k2 = k3 - k2;
      tmp = dxsquare;
      dxsquare = -dysquare;
      dysquare = tmp;
    } /* end if(IsOdd) */
    else /* octant is even */
    {
      if((Octant == FirstOctant)&& (IsFirst == TRUE))
      {
        DeltaSX = 0;
        DeltaSY = 0;
        StartX = X - XS; /* DeltaSX/Y at which we start drawing */
        StartY = Y - YS;
        /*StartX += StartX < 0 ? 0.2F : StartX > 0 ? -0.2F : 0;*/
        /*StartY += StartY < 0 ? 0.2F : StartY > 0 ? -0.2F : 0;*/
        IsFirst = FALSE;
        if(OctantCount == 0)
        {
          EndX = (X - XE); /* DeltaEX/Y at which we stop drawing */
          EndY = (Y - YE);
          DeltaEX = 0;
          DeltaEY = 0;
          IsLast = TRUE;
        }
      }
      while(u < k2/2) /* when u >= k2/2 the octant changes */
      {
        if(IsLast && RecStatus && !Status_p->IsInnerArc)
        {
          /* what does this represent? */
          DrawCondition = ((((float)Y-( (float)X*CoeffA ) + CoeffB) >= 0) &&
                           (((float)YS-( (float)XS*CoeffA ) + CoeffB) >= 0))||
                          ((((float)Y-( (float)X*CoeffA ) + CoeffB) < 0) &&
                           (((float)YS-( (float)XS*CoeffA ) + CoeffB) < 0));
        }
        else if(IsLast && !RecStatus)
        {
          DrawCondition = ((GFXABS(DeltaEX) <= GFXABS(EndX)) &&
                           (GFXABS(DeltaEY) <= GFXABS(EndY)));
        }
        else
        {
          DrawCondition = TRUE;
        }
        /* JF changed, as above */
        if(HIT_BOUNDARY(DeltaSX, DeltaSY, StartX, StartY) && DrawCondition)
        {
          DeltaSX = BIG_NUMBER;
          DeltaSY = BIG_NUMBER;
          if(i == 0)
          {
            if((i+1) > AvailableSize)
            {
              if(RecStatus == TRUE)
              {
                Status_p->IsCompleted = TRUE;
              }
              return(STGFX_ERROR_XYL_POOL_EXCEEDED);
            }
            else
            {
              XYLArray_p[i].PositionX = X+XC;
              XYLArray_p[i].PositionY = -Y+YC;
              Length = 1;
              i++;
            }
          }
          else if(( Y == PreviousY ) && ( X == (PreviousX + 1)))
          {
            Length++;
          }
          else if(( Y == PreviousY ) && ( X == (PreviousX - 1)))
          {
            XYLArray_p[i-1].PositionX -= 1;
            Length++;
          }
          else /* y != PreviousY */
          {
            XYLArray_p[i-1].Length = Length;
            if((i+1) > AvailableSize)
            {
              if(RecStatus == TRUE)
              {
                Status_p->IsCompleted = TRUE;
              }
              return(STGFX_ERROR_XYL_POOL_EXCEEDED);
            }
            else
            {
              XYLArray_p[i].PositionX = X+XC;
              XYLArray_p[i].PositionY = -Y+YC;
              Length = 1;
              i++;
            }
          }
          if(RecStatus == TRUE)
          {
            Status_p->x = FLOAT_TO_INT1(X);
            Status_p->y = FLOAT_TO_INT1(Y);
            Status_p->d = d;
            Status_p->u = u;
            Status_p->v = v;
            Status_p->k1 = k1;
            Status_p->k2 = k2;
            Status_p->k3 = k3;
            Status_p->octant = Octant;
            Status_p->dxsquare = dxsquare;
            Status_p->dysquare = dysquare;
            Status_p->dxdiag = dxdiag;
            Status_p->dydiag = dydiag;
          }

          if((RecStatus)&&(Status_p->IsInnerArc))
          {
            if((Octant == Status_p->LastOctant)&&
               (Status_p->IsTheVeryLast))
            {
            DrawInnerDashedArc =
             ((((float)Y - Status_p->Y0 - (((float)X - Status_p->X0)
              *((float)YD - Status_p->Y0)/((float)XD - Status_p->X0)) >= 0) &&
              ((float)YS - Status_p->Y0 - (((float)XS - Status_p->X0)
              *((float)YD - Status_p->Y0)/((float)XD - Status_p->X0)) >= 0)) ||
              (((float)Y - Status_p->Y0 - (((float)X - Status_p->X0)
              *((float)YD - Status_p->Y0)/((float)XD - Status_p->X0)) < 0) &&
              ((float)YS - Status_p->Y0 - (((float)XS - Status_p->X0)
              *((float)YD - Status_p->Y0)/((float)XD - Status_p->X0)) < 0)));

              if(!DrawInnerDashedArc)
              {

                Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
                Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
                Status_p->IsCompleted = TRUE;
                EndImmediatly = TRUE;
                break;
              }
            }
            Counter++;
            if(Counter > Status_p->Length)
            {
              Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
              Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
              EndImmediatly = TRUE;
              break;
            }
          }
        }
        else if(IsLast && !DrawCondition)
        {
          if(RecStatus && Status_p->IsInnerArc)
          {
            Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
            Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
          }
          EndImmediatly = TRUE;
          break;
        }
        if(IsArc && IsLast && (X == XE) && (Y == YE))
        {
          if(RecStatus && Status_p->IsInnerArc)
          {
            Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
            Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
            if(Octant == Status_p->LastOctant && Status_p->IsTheVeryLast)
            {
              Status_p->IsCompleted = TRUE;
            }
          }
          EndImmediatly = TRUE;
          break;
        }
        
        /* d is the discriminant for each movement. The extra conditions ensure
          we don't step completely across a narrow ellipse - Foley fig 19.35 */
        if((d < 0) ||
           ((v - k2/2) > 0 && ( v + k2/2) < 0) ||
           ((v - k2/2) < 0 && ( v + k2/2) > 0))
        {
          PreviousX = X;
          PreviousY = Y;
          X += dxdiag;
          Y += dydiag;
          DeltaSX += dxdiag;
          DeltaSY += dydiag;
          DeltaEX += dxdiag;
          DeltaEY += dydiag;
          u += k2;
          v += k3;
          d += v;
        }
        else
        {
          PreviousX = X;
          PreviousY = Y;
          X += dxsquare;
          Y += dysquare;
          DeltaSX += dxsquare;
          DeltaSY += dysquare;
          DeltaEX += dxsquare;
          DeltaEY += dysquare;
          u += k1;
          v += k2;
          d += u;
        }
      } /* end while( u < k2/2) */

      if(EndImmediatly)
      {
        break;
      }
      d = ( d + u -v + k1 - k2);
      v = (2*u - v + k1 - k2);
      u = u + k1 - k2;
      k3 = 4*(k1-k2) + k3;
      k2 = 2*k1 - k2;
      tmp = dxdiag;
      dxdiag = -dydiag;
      dydiag = tmp;
    } /* end if octant is even */
    
    /* now octant and octantCount must be updated */
    if(RecStatus && Status_p->IsTheVeryLast && Status_p->IsInnerArc &&
       (Octant == Status_p->LastOctant) && !(Status_p->IsFirstOctant &&
       Status_p->IsEllipse))
    {
      Status_p->IsCompleted = TRUE;
      EndImmediatly = TRUE;
      break;
    }
    Octant += 1;


    if(Octant > 8)
    {
      Octant -= 8;
    }

    if(RecStatus && Status_p->IsInnerArc)
    {
      if((!(Status_p->IsTheVeryLast) && (Octant == Status_p->LastOctant) &&
          !(Status_p->IsFirstOctant) && Status_p->IsEllipse) ||
          (!(Status_p->IsTheVeryLast) && (Octant == Status_p->LastOctant) &&
          (Status_p->TotalOctantCount == 1) && !Status_p->IsEllipse))
      {
        Status_p->XSLastOctant = (float)XS;
        Status_p->YSLastOctant = (float)YS;
        Status_p->IsTheVeryLast = TRUE;
      }

      if(Status_p->IsEllipse && Status_p->IsFirstOctant)
      {
        Status_p->IsFirstOctant = FALSE;
        Status_p->x = (float)X;
        Status_p->y = (float)Y;
        Status_p->d = d;
        Status_p->u = u;
        Status_p->v = v;
        Status_p->k1 = k1;
        Status_p->k2 = k2;
        Status_p->k3 = k3;
        Status_p->octant = Octant;
        Status_p->dxsquare = dxsquare;
        Status_p->dysquare = dysquare;
        Status_p->dxdiag = dxdiag;
        Status_p->dydiag = dydiag;
      }
    }
    OctantCount -= 1;
    if(RecStatus && Status_p->IsInnerArc && !Status_p->IsEllipse)
    {
       Status_p->TotalOctantCount -= 1;
    }
    if((OctantCount == 0) && (!IsLast) && (IsFirst == FALSE))
    {
      EndX = (X - XE);
      EndY = (Y - YE);
      DeltaEX = 0;
      DeltaEY = 0;
      DeltaSX = BIG_NUMBER;
      DeltaSY = BIG_NUMBER;
      StartX = 0;
      StartY = 0;
      IsLast = TRUE;
      
#if 0 /* JF: still need to draw the final pixel if we've just landed on it */
      if((EndX == 0) && (EndY == 0))
      {
        if(RecStatus && Status_p->IsInnerArc &&
          (Octant == Status_p->LastOctant) &&
          !(Status_p->IsFirstOctant && Status_p->IsEllipse))
        {
          Status_p->EdSdx = 2*A*(float)(S32)(X-XP) + B*(float)(S32)(Y-YP) + D;
          Status_p->EdSdy = B*(float)(S32)(X-XP) + 2*C*(float)(S32)(Y-YP) + E;
          Status_p->IsCompleted = TRUE;
        }
        break;
      }
#endif
    }
    if(RecStatus && Status_p->IsInnerArc)
    {
      Condition = (Counter < Status_p->Length);
    }
    else
    {
      Condition = (OctantCount >= 0 );
    }
  } /* end while(octantCount >= 0)*/

  if((i > 0) /*&&(i < BIG_NUMBER)*/ )
  {
    /* include theoretical endpoint if we stopped at an octant boundary without
      quite reaching it (DDTS 16718 issue #8) */
    if((OctantCount == -1) && (X == XE) && (Y == YE))
    {
      if(( Y == PreviousY ) && ( X == (PreviousX + 1)))
      {
        Length++;
      }
      else if(( Y == PreviousY ) && ( X == (PreviousX - 1)))
      {
        XYLArray_p[i-1].PositionX -= 1;
        Length++;
      }
      else /* y != PreviousY */
      {
        XYLArray_p[i-1].Length = Length;
        if((i+1) > AvailableSize)
        {
          if(RecStatus == TRUE)
          {
            Status_p->IsCompleted = TRUE;
          }
          return(STGFX_ERROR_XYL_POOL_EXCEEDED);
        }
        else
        {
          XYLArray_p[i].PositionX = X+XC;
          XYLArray_p[i].PositionY = -Y+YC;
          Length = 1;
          i++;
        }
      }
    }
  
    /* commit final segment and store its true Length */
    XYLArray_p[i-1].Length = Length;
    
#if 0 /* JF: This should never happen and I have never seen it do so */
    /* trim what is more than enough */
    if((!RecStatus && !IsArc) ||
      (RecStatus && Status_p->IsInnerArc && Status_p->IsEllipse))
    {
      S32 j; /* changed from U32 to help GX1 code */
      for(j = 0; j < i; j++)
      {
        if(
           ((XYLArray_p[j].PositionX > X_Max) &&
          ( (XYLArray_p[j].PositionY < Y_Min) ||
           (XYLArray_p[j].PositionY > Y_Max) )) ||
           ((XYLArray_p[j].PositionX < X_Min) &&
          ((XYLArray_p[j].PositionY < Y_Min) ||
           (XYLArray_p[j].PositionY > Y_Max) ))
           )
        {
          /* trim whole segment even if only a part of it is out of bounds? */
  #ifdef STGFX_AVOID_ZERO_LENGTH
          /* want item-by-item copy down for efficiency if this occurs often */
          memmove(XYLArray_p+j, XYLArray_p+j+1, (i-j)*sizeof(STBLIT_XYL_t));
          --i;
          --j; /* so next iteration considers element just copied down */
  #else
          XYLArray_p[j].Length = 0;
  #endif
        }
      }
    }
#endif

    /* ensure ellipse is closed if undashed (!RecStatus) */
    if(!RecStatus && !IsArc)
    {
      if(XYLArray_p[i-1].PositionY == XYLArray_p[0].PositionY)
      {
        /* combine first and last segments avoiding overlap or gap */
        X1Line = GFXMIN(XYLArray_p[0].PositionX, XYLArray_p[i-1].PositionX);
        X2Line = GFXMAX(XYLArray_p[0].PositionX + (S32)XYLArray_p[0].Length,
                        XYLArray_p[i-1].PositionX + (S32)XYLArray_p[i-1].Length);
                        /* (this is actually the pixel one past the end) */
        XYLArray_p[0].PositionX = X1Line;
        XYLArray_p[0].Length = X2Line - X1Line;
        --i; /* to lose last element */
      }
      else if
        ((GFXABS(XYLArray_p[i-1].PositionY - XYLArray_p[0].PositionY) > 1) ||
         (XYLArray_p[i-1].PositionX -
            (XYLArray_p[0].PositionX + (S32)XYLArray_p[0].Length) > 0) ||
         (XYLArray_p[0].PositionX -
            (XYLArray_p[i-1].PositionX + (S32)XYLArray_p[i-1].Length) > 0))
      {
        /* Complete ellipse with a line segment. The main case where this is
          needed is a narrow ellipse where we step into a later octant before
          quite getting back to the starting point, eg 101, 1, 45*64 */

        Size = AvailableSize - i;
        if(XYLArray_p[0].PositionX < XYLArray_p[i-1].PositionX)
          /* (no need to add Length to either since they won't ever cross) */
        {
          X1Line = XYLArray_p[i-1].PositionX<<STGFX_INTERNAL_ACCURACY;

          Y1Line = XYLArray_p[i-1].PositionY<<STGFX_INTERNAL_ACCURACY;
          
          if(XYLArray_p[i-1].Length > 1)
          {
            /* We have space to cut this segment
              so it won't overlap the end of the line */
              
            XYLArray_p[i-1].PositionX++;
            XYLArray_p[i-1].Length--;
          }
          else
          {
            /* Write line over the old segment, which ends at the same place */
            --i;
          }

          X2Line = (XYLArray_p[0].PositionX +
                    (S32)XYLArray_p[0].Length -1)<<STGFX_INTERNAL_ACCURACY;

          Y2Line = XYLArray_p[0].PositionY<<STGFX_INTERNAL_ACCURACY;
          
          stgfx_2dRasterSimpleLine(X1Line, Y1Line, X2Line,
                                   Y2Line, &Size, &XYLArray_p[i]);
          
          if(XYLArray_p[0].Length > 1)
          {
            /* We have space to cut this segment
              so it won't overlap the start of the line */
              
            XYLArray_p[0].Length--;
          }
          else if(XYLArray_p[0].PositionY == XYLArray_p[i].PositionY)
          {
            if(XYLArray_p[i].Length > 1)
            {
              /* We have space to cut this segment
                so it won't overlap the start of the main curve */
              
              XYLArray_p[i].PositionX++;
              XYLArray_p[i].Length--;
            }
            else
            {
              /* both have Length 1; leave element at [0] and delete first
                segment of line */
                
              --Size;
              memmove(&XYLArray_p[i], &XYLArray_p[i+1],
                      Size*sizeof(STBLIT_XYL_t));
            }
          }
          else /*XYLArray_p[0].PositionY == XYLArray_p[i+Size-1].PositionY*/
          {
            if(XYLArray_p[i+Size-1].Length > 1)
            {
              /* We have space to cut this segment
                so it won't overlap the start of the main curve */
              
              XYLArray_p[i+Size-1].PositionX++;
              XYLArray_p[i+Size-1].Length--;
            }
            else
            {
              /* both have Length 1; leave element at [0] and delete last
                segment of line */
                
              --Size;
            }
          }
        }
        else /*XYLArray_p[0].PositionX >= XYLArray_p[i-1].PositionX*/
        {
          X1Line = (XYLArray_p[i-1].PositionX +
                    (S32)XYLArray_p[i-1].Length -1)<<STGFX_INTERNAL_ACCURACY;
                    
          Y1Line = XYLArray_p[i-1].PositionY<<STGFX_INTERNAL_ACCURACY;

          if(XYLArray_p[i-1].Length > 1)
          {
            /* We have space to cut this segment
              so it won't overlap the end of the line */
              
            XYLArray_p[i-1].Length--;
          }
          else
          {
            /* Write line over the old segment, which ends at the same place */
            --i;
          }

          X2Line = XYLArray_p[0].PositionX<<STGFX_INTERNAL_ACCURACY;
          
          Y2Line = XYLArray_p[0].PositionY<<STGFX_INTERNAL_ACCURACY;
          
          stgfx_2dRasterSimpleLine(X1Line, Y1Line, X2Line,
                                   Y2Line, &Size, &XYLArray_p[i]);
                                   
          if(XYLArray_p[0].Length > 1)
          {
            /* We have space to cut this segment
              so it won't overlap the start of the line */
              
            XYLArray_p[0].PositionX++;
            XYLArray_p[0].Length--;
          }
          else if(XYLArray_p[0].PositionY == XYLArray_p[i].PositionY)
          {
            if(XYLArray_p[i].Length > 1)
            {
              /* We have space to cut this segment
                so it won't overlap the start of the main curve */
              
              XYLArray_p[i].Length--;
            }
            else
            {
              /* both have Length 1; leave element at [0] and delete first
                segment of line */
                
              --Size;
              memmove(&XYLArray_p[i], &XYLArray_p[i+1],
                      Size*sizeof(STBLIT_XYL_t));
            }
          }
          else /*XYLArray_p[0].PositionY == XYLArray_p[i+Size-1].PositionY*/
          {
            if(XYLArray_p[i+Size-1].Length > 1)
            {
              /* We have space to cut this segment
                so it won't overlap the start of the main curve */
              
              XYLArray_p[i+Size-1].Length--;
            }
            else
            {
              /* both have Length 1; leave element at [0] and delete last
                segment of line */
                
              --Size;
            }
          }
        }

        i += Size;
      }
    }
  }

  *NumXYL = i;

  if(RecStatus == TRUE)
  {
    /* store some other fields of the conic status structure */
    Status_p->XE = Status_p->x;
    Status_p->YE = Status_p->y;
    Status_p->i = i;
    Status_p->Counter++;
  }
  
  return(ST_NO_ERROR);
}

/****************************************************************************\
Name        :  InitConicParameters()
Description :  Initializes the conic parameters  
Parameters  :  
Assumptions : 
Limitations : 
Returns     :  
\****************************************************************************/ 
void InitConicParameters( 
     S32 Octant, 
     float A, 
     float B, 
     float C,
     float D, 
     float E, 
     float F, 
     float* DParam,
     float* UParam, 
     float* VParam, 
     float* K1, 
     float* K2, 
     float* K3, 
     S32* dxsquare, 
     S32* dysquare,
     S32* dxdiag, 
     S32* dydiag
)
{
switch(Octant)
  {
  case 1:
        *DParam = (A + B/2 + C/4 + D + E/2 + F);
        *UParam = (A + B/2 +D);
        *VParam = (A + B/2 +D +E);
        *K1 = 2 *A;
        *K2 = 2 *A +B;
        *K3 = *K2 + B + 2 *C;
        *dxsquare = 1; 
        *dysquare = 0;
        *dxdiag = 1;
        *dydiag = 1;
        break;
  case 2:
        *DParam = (A/4 + B/2 + C + D/2 + E + F);
        *UParam = (B/2 + C + E);
        *VParam = (B/2 +C + D +E);
        *K1 = 2 * C;
        *K2 = B + 2 * C;
        *K3 = 2 * A + 2 * B + 2 * C;
        *dxsquare = 0; 
        *dysquare = 1;
        *dxdiag = 1;
        *dydiag = 1;
        break;
  case 3:
        *DParam = (A/4 - B/2 + C - D/2 + E + F);
        *UParam = (-B/2 + C + E);
        *VParam = (C - (B/2) - D + E);
        *K1 = 2*C;
        *K2 = 2*C - B;
        *K3 = 2 * A - 2 * B + 2 * C;
        *dxsquare = 0; 
        *dysquare = 1;
        *dxdiag = -1;
        *dydiag = 1;
        break;
  case 4:
        *DParam = (A - B/2 + C/4 -D + E/2 + F);
        *UParam = (A - B/2 - D);
        *VParam = (A - B/2 - D + E);
        *K1 = 2 * A;
        *K2 = 2 * A - B;
        *K3 = 2 * A - 2 * B + 2 * C;
        *dxsquare = -1; 
        *dysquare = 0;
        *dxdiag = -1;
        *dydiag = 1;
        break;
  case 5:
        *DParam = (A + B/2 + C/4 - D - E/2 + F);
        *UParam = (A + B/2 - D);
        *VParam = (A + B/2 - D - E);
        *K1 = 2 * A;
        *K2 = 2*A + B;
        *K3 = 2 * A + 2 * B + 2 * C;
        *dxsquare = -1; 
        *dysquare = 0;
        *dxdiag = -1;
        *dydiag = -1;
        break;
  case 6:
        *DParam = (A/4 + B/2 + C - D/2 - E + F);
        *UParam = (B/2 + C - E);
        *VParam = (B/2 + C - D - E);
        *K1 = 2*C;
        *K2 = B + 2*C;
        *K3 = 2 * A + 2 * B + 2 * C;
        *dxsquare = 0; 
        *dysquare = -1;
        *dxdiag = -1;
        *dydiag = -1;
        break;
  case 7:
        *DParam = (A/4 - B/2 + C + D/2 - E + F);
        *UParam = (-B/2 + C - E);
        *VParam = (-B/2 + C + D - E);
        *K1 = 2*C;
        *K2 = 2*C - B;
        *K3 = 2 * A - 2 * B + 2 * C;
        *dxsquare = 0; 
        *dysquare = -1;
        *dxdiag = 1;
        *dydiag = -1;
        break;
  case 8:
        *DParam = (A - B/2 + C/4 + D - E/2 + F);
        *UParam = (A - B/2 + D);
        *VParam = (A - B/2 + D - E);
        *K1 = 2*A;
        *K2 = 2*A - B;
        *K3 = 2 * A - 2 * B + 2 * C;
        *dxsquare = 1; 
        *dysquare = 0;
        *dxdiag = 1;
        *dydiag = -1;
        break;
  }

}

/****************************************************************************\
Name        : GetOctant()
Description :  allows to know the current octant in the conic function 
               by means of the gradient components
Parameters  :  float D, component i of the gradient of the ellipse equation  
               float E, component j of the gradient of the ellipse equation 
               S32* Octant, the returned value is the octant of the point
                            in which the gradient has the component 
                            i = D and the component j = E
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
void GetOctant(
      float D, 
      float E, 
      S32* Octant
)
{
  /* round components to nearest millionth to systemise
    what happens with the error bounds of 0 */
    
  S32 dSdx = (S32)(D*1000000);
  S32 dSdy = (S32)(E*1000000);
  
  /* JF: consider octant boundaries to belong to the previous octant
    (seems to be better for getting Conic() to stop exactly on E) */
    
  if((dSdx > 0) && (dSdy <= 0))
  {
    if(dSdx > GFXABS(dSdy))
    { 
      *Octant = 2;
    }
    else
    {
      *Octant = 1;
    }      
  }
  else if((dSdx >= 0) && (dSdy > 0))
  {
    if (dSdx < dSdy)
    {
      *Octant = 4;
    }
    else
    {
      *Octant = 3;
    }
  }
  else if((dSdx < 0) && (dSdy >= 0))
  
  {
    if(dSdy >= GFXABS(dSdx))
    {
      *Octant = 5;
    }
    else
    {
      *Octant = 6;
    }
  }
  else /* ((dSdx <= 0) && ( dSdy < 0))*/
  {
    if(dSdx <= dSdy) /* GFXABS(dSdx) >= GFXABS(dSdy) */
    {
      *Octant = 7;
    }
    else
    {
      *Octant = 8;
    }
  }
}

/* FractToInt is a simple function which
   perform the rounding of a fixed point
   number (either positive or negative) 
S32 FractToInt(S32 a)
{
  if(a >= 0)
  {
    a = (a+(1<<(FRACT-1)))>>FRACT;
  } else {
    a = (a+(1<<(FRACT-1)));
    a >>= FRACT;
    a -=  (2<<(32 - FRACT - 1));
  }
  return(a);
}
*/
/****************************************************************************\
Name        : CalcConicCoefficients()
Description : CalcConicCoefficients calculate the coefficients of a generic 
              ellipse using the long axis, the short axis and the
              orientation with an horizontal line passing through the center
              The center is not the ellipse center, but is the leftmost
              point on the major axis
Parameters  : float LongRadius, major axis of the ellipse
              float ShortRadius, minor axis of the ellipse
              S32 Orientation, inclination angle between the major axis and 
                               the horizontal line
              float* A, 
              float* B,
              float* C,
              float* D,
              float* E,
              float* F,
              float* XP, x coord of the rightmost point P of the major axis
              float* YP, y coord of P
              float* XD, x coord of the rotation needed to rotate
                         the ellipse center in order to stay on the 
                         system origin
              float* YD
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
void CalcConicCoefficients(
       float LongRadius,
       float ShortRadius,
       S32 Orientation,
       float* A,
       float* B,
       float* C,
       float* D,
       float* E,
       float* F,
       float* XP,
       float* YP,
       float* XD,
       float* YD
       )
{
    double  Phi;  /* ATTENZIONE: verificare tipi ST */
    float  CosPhi;
    float  SinPhi;
    float  LRadius2;
    float  SRadius2;
    Phi = TO_RAD(Orientation);
    CosPhi = (float)(double)cos(Phi);
    SinPhi = (float)(double)sin(Phi);
    LRadius2 = LongRadius * LongRadius;
    SRadius2 = ShortRadius * ShortRadius;

    /* JF; reduced precision of local calculations because double
      didn't seem to be needed */
      
    *A = (CosPhi*CosPhi/LRadius2 + SinPhi*SinPhi/SRadius2);
    *B = (2.F * (SinPhi*CosPhi/LRadius2 - SinPhi*CosPhi/SRadius2));
    *C = (CosPhi*CosPhi/SRadius2 + SinPhi*SinPhi/LRadius2);
    *D = (2.F * CosPhi/LongRadius);
    *E = (2.F * SinPhi/LongRadius);
    *F = 0.F;
    *XP = (2.F * LongRadius*CosPhi);
    *YP = (2.F * LongRadius*SinPhi);
    *XD = (LongRadius*CosPhi);
    *YD = (LongRadius*SinPhi);
}

/****************************************************************************\
Name        : CalcEllipsePoint()
Description : gives the X and Y coordinates of a point S in the ellipse 
              under a given angle 
Parameters  : float LRadius, the Long emi axis 
              float SRadius, the Short emi axis
              S32 Angle, the angle between the diameter through the point S
                         and the horizontal line
              float* XS, the S x coord returned from the function
              float* YS, the S y coord
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
static void CalcEllipsePoint( float LRadius, float SRadius,
                              S32 Angle, float* XS, float* YS )
{
  /* co-ord range ~ +/- 10^4, so co-ord^4 range ~ +/- 10^16
    so we should be safe from overflow on float (+10^38 / -10^37) */
  
  float lr2 = LRadius*LRadius;
  float sr2 = SRadius*SRadius;
  float c = (float)cos(TO_RAD(Angle));
  float s = (float)sin(TO_RAD(Angle));
  float Radius;
  Radius = (float)sqrt((double)(float)(lr2*sr2/((sr2*c*c) + (lr2*s*s))));
  
  *XS = (LRadius + Radius*c);
  *YS = (Radius*s);
}


#if 0 /* these function are not actually used anywhere */
/****************************************************************************\
Name        : CalcRadius()
Description : gives the exact Radius of the ellipse under a given angle  
Parameters  : float LRadius, the Long emi axis 
              float SRadius, the Short emi axis
              S32 Angle, the angle between the diameter through the point S
                         and the horizontal line
              float* Radius, the resulted Radius value 
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
void CalcRadius(
      float LRadius, 
      float SRadius, 
      S32 Angle,
      float* Radius
)
{
  *Radius = (float)((double)LRadius*(double)LRadius*(double)SRadius*(double)SRadius/
           (((double)SRadius*(double)SRadius*
           cos(TO_RAD(Angle))*cos(TO_RAD(Angle)))+
           ((double)LRadius*(double)LRadius*
           sin(TO_RAD(Angle))*sin(TO_RAD(Angle)))));
  *Radius =  (float)sqrt(*Radius);
}

/****************************************************************************\
Name        : EndPointAndDirection()
Description : finds out the coordinates of two points,
              required by a CapStyle different from flat 
Parameters  : S32 XC, 
              S32 YC, 
              U32 LRadius, 
              U32 SRadius, 
              U16 LineWidth,
              S32 Angle,  
              S32 Orientation, 
              S32 Distance,
              S32* SAngle,
              S32* EndPointX, 
              S32* EndPointY,
              S32* DirectionX, 
              S32* DirectionY,
              BOOL IsFirstPoint,  
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
static void EndPointAndDirection( 
     S32 XC, 
     S32 YC, 
     U32 LRadius, 
     U32 SRadius, 
     U16 LineWidth,
     S32 Angle,  
     S32 Orientation,
     S32 Distance,
     S32* SAngle,
     S32* EndPointX, 
     S32* EndPointY,
     S32* DirectionX, 
     S32* DirectionY,
     BOOL IsFirstPoint 
)
{
  float XCenter;
  float YCenter;
  float LongRadius;
  float ShortRadius;
  double NormalAngle; /* no point converting it to float and back */
  double TangentAngle; /* "-------------------------------------" */
  float XD;
  float YD;
  float Dist;
  float EndX;
  float EndY;
  float DirX;
  float DirY;
  float XOut;
  float YOut;
  U16   Width;
  U32   InnerLRadius;
  U32   InnerSRadius;
  U32   OuterLRadius;
  U32   OuterSRadius;
  
  XCenter = (float)XC / (float)(1<<STGFX_INTERNAL_ACCURACY);
  YCenter = (float)YC / (float)(1<<STGFX_INTERNAL_ACCURACY);
  InnerAndOuterRadius( LineWidth, LRadius, SRadius,
                       &InnerLRadius, &InnerSRadius,
                       &OuterLRadius, &OuterSRadius );
  LongRadius = (float)InnerLRadius / (float)(1<<STGFX_INTERNAL_ACCURACY);
  ShortRadius = (float)InnerSRadius / (float)(1<<STGFX_INTERNAL_ACCURACY);
  XD = (float)((double)LongRadius*cos(TO_RAD(Orientation)));
  YD = (float)((double)LongRadius*sin(TO_RAD(Orientation)));
  Dist = (float)Distance / (float)(1<<STGFX_INTERNAL_ACCURACY);
  CalcEllipsePoint(LongRadius, ShortRadius, Angle, &EndX, &EndY);
                    
  NormalAngle = atan(((double)EndY/((double)EndX-(double)LongRadius))*
                     (((double)LongRadius*(double)LongRadius) / 
                      ((double)ShortRadius*(double)ShortRadius))); 
  if((LineWidth % 2)== 0)
  {
    Width = (LineWidth/2) -1;
  }
  else
  {
    Width = LineWidth/2;
  }
  
  CalcOutArcPoint( EndX, EndY, LongRadius, ShortRadius, Width+1, 
                   NormalAngle, &XOut, &YOut );
  
  TangentAngle = atan( -((double)(float)(XOut-LongRadius)/(double)YOut) *
                        (((double)ShortRadius*(double)ShortRadius) /
                            ((double)LongRadius*(double)LongRadius)) );

  if(IsFirstPoint) /* it is the starting point */
  {
    if((XOut-LongRadius) >= 0 && YOut >= 0)
    {  /* first quadrant */
      TangentAngle = -TangentAngle;
      DirX = XOut - (Dist * (float)cos(TangentAngle));
      DirY = YOut + (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) - 5760;
    }
    else if((XOut-LongRadius) < 0 && YOut >= 0 )
    {  /* second quadrant */
      DirX = XOut - (Dist * (float)cos(TangentAngle));
      DirY = YOut + (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) - 5760;
    }
    else if((XOut-LongRadius) < 0 && YOut < 0)     
    {  /* third quadrant */
      TangentAngle = -TangentAngle;
      DirX = XOut + (Dist * (float)cos(TangentAngle));
      DirY = YOut - (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) + 5760;
    }
    else /*((XOut-LRadiusIn) >= 0 && YOut < 0)) fourth quadrant */
    {
      DirX = XOut + (Dist * (float)cos(TangentAngle));
      DirY = YOut + (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) + 5760;
    }
  }
  else /* so it is the Ending point */
  {
    if((XOut-LongRadius) >= 0 && YOut >= 0)
    {  /* first quadrant */
      TangentAngle = -TangentAngle;
      DirX = XOut + (Dist * (float)cos(TangentAngle));
      DirY = YOut - (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) + 5760;
    }
    else if((XOut-LongRadius) < 0 && YOut >= 0 )
    {  /* second quadrant */
      TangentAngle = -TangentAngle;
      DirX = XOut + (Dist * (float)cos(TangentAngle));
      DirY = YOut - (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) + 5760;
    }
    else if((XOut-LongRadius) < 0 && YOut < 0)     
    {  /* third quadrant */
      TangentAngle = -TangentAngle;
      DirX = XOut - (Dist * (float)cos(TangentAngle));
      DirY = YOut + (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) - 5760;
    }
    else /*((XOut-LRadiusIn) >= 0 && YOut < 0)) fourth quadrant */
    {
      DirX = XOut - (Dist * (float)cos(TangentAngle));
      DirY = YOut - (Dist * (float)sin(TangentAngle));
      *SAngle = FLOAT_TO_INT1((float)TangentAngle*PI/(float)PI_FLOAT) - 5760;
    }
  }
  /* a rotation is needed */
  RotatePoint(Orientation, &XOut, &YOut);
  RotatePoint(Orientation, &DirX, &DirY);
  /* and now a translation */
  XOut += (XCenter - XD);
  YOut = -YOut + (YCenter + YD);
  DirX += (XCenter - XD);
  DirY = -DirY + (YCenter + YD);
  *EndPointX = (FLOAT_TO_INT1(XOut)<<STGFX_INTERNAL_ACCURACY);
  *EndPointY = (FLOAT_TO_INT1(YOut)<<STGFX_INTERNAL_ACCURACY);
  *DirectionX = (FLOAT_TO_INT1(DirX)<<STGFX_INTERNAL_ACCURACY);
  *DirectionY = (FLOAT_TO_INT1(DirY)<<STGFX_INTERNAL_ACCURACY);
}
#endif

/****************************************************************************\
Name        : FindMaxMinArrayXYL()
Description : FindMaxMinArrayXYL finds the position of the maximum and the
              minimum values of the y coordinate of any array xyl
Parameters  : STBLIT_XYL_t* XYLArray, 
              U32 Size,
              U32* PosMax, 
              U32* PosMin,  
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
static void FindMaxMinArrayXYL( STBLIT_XYL_t* XYLArray, U32 Size,
                                U32* PosMax, U32* PosMin )
{
   S32 i;
   S32 MaxElement = -1;
   S32 MinElement = BIG_NUMBER;
   *PosMax = 0;
   *PosMin = 0;
   for(i = 0; i < Size; i++)
   {
     if((XYLArray[i].PositionY < MinElement) &&
        (XYLArray[i].Length > 0))
     {
       *PosMin = i;
       MinElement = XYLArray[i].PositionY;
     }
     if((XYLArray[i].PositionY > MaxElement)&&
        (XYLArray[i].Length > 0))
     {
       *PosMax = i;
       MaxElement = XYLArray[i].PositionY;
     }
   }
}

/****************************************************************************\
Name        : SplitArrayXYL()
Description : SplitArrayXYL splits up an xyl array (of an arc or an ellipse)
              into 1, 2, or 3 'sides' within which y varies monatonically
              (as required by the stgfx_FillGenericPolygon function)
Parameters  : STBLIT_XYL_t* XYLArray, 
              U32 Size, 
              U32* PosMin, 
              U32* PosMax,
              BOOL IsEllipse,
              stgfx_Side_t* Sides_p,
              U32* TotSides 
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
void SplitArrayXYL( 
     STBLIT_XYL_t* XYLArray, 
     U32 Size, 
     U32* PosMin, 
     U32* PosMax,
     BOOL IsEllipse,
     stgfx_Side_t* Sides_p,
     U32* TotSides 
)
{
  U32 Min = 0;
  U32 Max = 0;
  Sides_p[0].xylData = NULL;
  Sides_p[1].xylData = NULL;
  Sides_p[2].xylData = NULL;
  FindMaxMinArrayXYL( XYLArray, Size, &Max, &Min );
  
  
  if(Max < Min) /* max y arises before min y */
  {
    if( Max > 0 )
    {
      *TotSides = 2;
      Sides_p[0].length = Max+1;/* maximum y record must be both in this
                                   side and in the next one */ 
      Sides_p[0].xylData = &(XYLArray[0]);
      Sides_p[0].Up = 1;
      Sides_p[0].Spin = 0;
      Sides_p[1].length = Min-Max+1;
      Sides_p[1].xylData = &(XYLArray[Max]);
      Sides_p[1].Up = -1;
      Sides_p[1].Spin = 1;

      if( Min < (Size-1) ) /* there are 3 sides */
      {
        /*if(IsEllipse)
        {
          Sides_p[2].length = Size - Min+1;
          Sides_p[2].Spin = Sides_p[0].Spin;
        }
        else
        {*/
          Sides_p[2].length = Size - Min;
          Sides_p[2].Spin = 2;
        /*}*/
        *TotSides += 1;
        Sides_p[2].xylData = &(XYLArray[Min]);
        Sides_p[2].Up = 1;
      } /* else there are just 2 sides */
    }
    else /* sides are less than 3 */
    {
      *TotSides = 1;
      Sides_p[0].length = Min - Max +1;
      Sides_p[0].xylData = &(XYLArray[Max]);
      Sides_p[0].Up = -1;
      Sides_p[0].Spin = 0;
      
      if((Size-1) - Min > 0) /* there are 2 sides */
      {
        if(IsEllipse)
        {
          Sides_p[1].length = Size - Min +1;
        }
        else
        {
          Sides_p[1].length = Size - Min; 
        }
        *TotSides += 1;
        Sides_p[1].xylData = &(XYLArray[Min]);
        Sides_p[1].Up = 1;
        Sides_p[1].Spin = 1;
      } /* else there is just 1 side */
    }
  }
  else /* Max > Min, ie max y arises after min y */
  {
    if( Min > 0 )
    {
      *TotSides = 2;
      Sides_p[0].length = Min+1;
      Sides_p[0].xylData = &(XYLArray[0]);
      Sides_p[0].Up = -1;
      Sides_p[0].Spin = 0;
      Sides_p[1].length = Max - Min+1;
      Sides_p[1].xylData = &(XYLArray[Min]);
      Sides_p[1].Up = 1;
      Sides_p[1].Spin = 1;

      if((Size-1) - Max > 0) /* there are 3 sides */
      {
         *TotSides += 1;
         /*
         if(IsEllipse)
         {
           Sides_p[2].length = Size - Max +1;
           Sides_p[2].Spin = Sides_p[0].Spin;
         }
         else
         {*/
           Sides_p[2].length = Size - Max; 
           Sides_p[2].Spin = 2;
         /*}*/
         Sides_p[2].xylData = &(XYLArray[Max]);
         Sides_p[2].Up = -1;
      } /* else there are just 2 sides */
    } 
    else /* there are less than 3 sides */
    {
      *TotSides = 1;
      Sides_p[0].length = Max - Min +1;
      Sides_p[0].xylData = &(XYLArray[Min]);
      Sides_p[0].Up = 1;
      Sides_p[0].Spin = 0;
      if( (Size-1) - Max > 0 ) /* there are 2 sides */
      {
        *TotSides += 1;
        if(IsEllipse)
        {
          Sides_p[1].length = Size - Max +1;
        }
        else
        {
          Sides_p[1].length = Size - Max;
        }
        Sides_p[1].xylData = &(XYLArray[Max]);
        Sides_p[1].Up = -1;
        Sides_p[1].Spin = 1;
      } /* else there is just 1 side */
    }
  } /* end if Max < Min */
  *PosMin = Min;
  *PosMax = Max;
} /* end of SplitArrayXYL function */

/****************************************************************************\
Name        : RotatePoint()
Description : Rotate the input point of the orientation angle 
              of the arc/ellipse
Parameters  : S32 Orientation, the ellipse rotation angle 
              float* XS, the x coord of the returned point S
              float* YS, the y coord of S
Assumptions : 
Limitations : 
Returns     : the rotated coordinates of the point
\****************************************************************************/
static void RotatePoint(S32 Orientation, float* XS, float* YS)
{
   float c = (float)cos(-TO_RAD(Orientation));
   float s = (float)sin(-TO_RAD(Orientation));
   
   float tmp = *XS;
   *XS = (tmp*c + (*YS)*s);
   *YS = ((*YS)*c - tmp*s);  
}

/****************************************************************************\
Name        : TranslatePoint()
Description : Translate the input point from a coordinates system
              having the origin on the ellipse 
              to the Bitmap origin (on the top left)
Parameters  : S32 Orientation, the ellipse rotation angle 
              float* XTranslated, the x coord of the translated point
              float* YTranslated, the y coord of the translated point 
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
static void TranslatePoint( S32 Orientation, 
                            float LongRadius, 
                            float XCenter, float YCenter,
                            float* XTranslated, 
                            float* YTranslated )
{
    double  Phi; 
    float  CosPhi;
    float  SinPhi;
    float   XD;
    float   YD;
    
    Phi = TO_RAD(Orientation);
    CosPhi = (float)cos(Phi);
    SinPhi = (float)sin(Phi);
    XD = LongRadius * CosPhi;
    YD = LongRadius * SinPhi;
    *XTranslated = *XTranslated + (XCenter - XD);
    *YTranslated = (YCenter + YD) - *YTranslated;
}

/****************************************************************************\
Name        : CalcOutArcPoint()
Description : takes as input an endpoint on the inner arc
              (in case of thick arc) and calculate the related point on the
              outer arc along the normal direction
Parameters  : 
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
static void CalcOutArcPoint(float XIn, float YIn, float LRadiusIn, 
                            float SRadiusIn, U16 LineWidth, double Angle,
                            float* XOut, float* YOut)
{
  /* the following is not necessary */
  if(Angle > (PI_FLOAT * 2.))
  {
    while(Angle > (PI_FLOAT*2.))
    {
      Angle -= (PI_FLOAT*2.);
    }
  }
  else if (Angle < 0)
  {
    while(Angle < (PI_FLOAT*2.))
    {
      Angle += (PI_FLOAT*2.);
    }
  }
  
  /* the following code handles Angle from 0 to PI */
  
  if((XIn-LRadiusIn) >= 0 && YIn >= 0)
  {  /* first quadrant */
     *XOut = (XIn + ((float)(U16)(LineWidth-1)*(float)cos(Angle)));
     *YOut = (YIn + ((float)(U16)(LineWidth-1)*(float)sin(Angle)));
  }
  else if((XIn-LRadiusIn) < 0 && YIn >= 0 )
  {  /* second quadrant */
     *XOut = (XIn - ((float)(U16)(LineWidth-1)*(float)cos(Angle)));
     *YOut = (YIn - ((float)(U16)(LineWidth-1)*(float)sin(Angle)));
  }
  else if((XIn-LRadiusIn) < 0 && YIn < 0)     
  {  /* third quadrant */
     *XOut = (XIn - ((float)(U16)(LineWidth-1)*(float)cos(Angle)));
     *YOut = (YIn - ((float)(U16)(LineWidth-1)*(float)sin(Angle)));
  }
  else /*((XIn-LRadiusIn) >= 0 && YIn < 0)) fourth quadrant */
  {
     *XOut = (XIn + ((float)(U16)(LineWidth-1)*(float)cos(Angle)));
     *YOut = (YIn + ((float)(U16)(LineWidth-1)*(float)sin(Angle)));
  }
}

/****************************************************************************\
Name        : Thickness()
Description : builds up a thick arc starting from its inner and outer arcs;
              the output is compound of 6 arrays: the first one is for 
              the internal arc, the second one is for the external arc,
              the third one is for the thickness, the fourth one is for
              the starting segments, and the last one is for the ending
              segments
Parameters  : U32 BHeight,  bitmap height is required by
                            stgfx_FillGenericPolygon 
              float XC,     
              float YC,
              BOOL IsEllipse,
              U32 SizeInner, 
              STBLIT_XYL_t* InnerArray,
              U32 SizeOuter, 
              STBLIT_XYL_t* OuterArray,
              U32* SizeThicknessArray,
              STBLIT_XYL_t*  ThicknessArray,
              U32* SizeSELineArray,
              STBLIT_XYL_t*  SELineArray,
              U32* SizeEELineArray,
              STBLIT_XYL_t*  EELineArray  
              S32* Points
Assumptions : SizeInner, SizeOuter > 0
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
\****************************************************************************/
static ST_ErrorCode_t Thickness( 
          U32 BHeight, float XC, float YC, BOOL IsEllipse,
          U32 SizeInner, STBLIT_XYL_t* InnerArray,
          U32 SizeOuter, STBLIT_XYL_t* OuterArray,
          U32* SizeThicknessArray, STBLIT_XYL_t*  ThicknessArray,
          U32* SizeSELineArray, STBLIT_XYL_t*  SELineArray,
          U32* SizeEELineArray, STBLIT_XYL_t*  EELineArray,
          S32* Points_p )
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  U32                    PosMinIn;
  U32                    PosMaxIn;
  U32                    PosMinOut;
  U32                    PosMaxOut;
  S32                    MaxElement = -1;
  S32                    MinElement = BIG_NUMBER;
  U32                    TotSidesIn;
  U32                    TotSidesOut;
  U32                    j = 0;
  S32                    XStartIn;
  S32                    YStartIn;
  S32                    XStartOut;
  S32                    YStartOut;
  S32                    XEndIn;
  S32                    YEndIn;
  S32                    XEndOut;
  S32                    YEndOut;
  stgfx_Side_t           Sides[8];
  stgfx_Side_t*          Sides_p;
  BOOL                   SecondLine = FALSE;
  
  
  Sides_p = &Sides[0];
  SplitArrayXYL( OuterArray, SizeOuter, &PosMinOut, &PosMaxOut, IsEllipse,
                 Sides_p, &TotSidesOut );
  MinElement = OuterArray[PosMinOut].PositionY;
  MaxElement = OuterArray[PosMaxOut].PositionY;
  
  /* invert sense for the Outer sides */
  for(j = 0; j < TotSidesOut; j++)
  {
     Sides_p[j].Up = -(Sides_p[j].Up);
  }
  
  /* Determine end segment endpoints - JF dubious about the alogrithm */
  if((InnerArray[SizeInner-1].PositionY -FLOAT_TO_INT1(YC)) < 0)
  {
    XEndIn = (InnerArray[SizeInner-1].PositionX);
  }
  else
  {
    XEndIn = (InnerArray[SizeInner-1].PositionX +
              InnerArray[SizeInner-1].Length-1);
  }
  YEndIn = (InnerArray[SizeInner-1].PositionY);
  
  if((OuterArray[SizeOuter-1].PositionY -FLOAT_TO_INT1(YC)) < 0)
  {
    XEndOut = (OuterArray[SizeOuter-1].PositionX);
  }
  else
  {
    XEndOut = (OuterArray[SizeOuter-1].PositionX +
               OuterArray[SizeOuter-1].Length-1);
  }
  YEndOut = OuterArray[SizeOuter-1].PositionY;
  
  /* due to the necessity of comparing also negative numbers, it is
    better to find out the up value or the end segment, before shifting */
  if(YEndOut<YEndIn)
  {
    Sides_p[TotSidesOut].Up = -1;
  } 
  else if(YEndOut>YEndIn)
  {
    Sides_p[TotSidesOut].Up = 1;
  } 
  else /* horizontal line */
  {
    Sides_p[TotSidesOut].Up = 0;
  }
  
  /* now, shifting is possible */
  XEndIn = XEndIn<<STGFX_INTERNAL_ACCURACY;
  YEndIn = YEndIn<<STGFX_INTERNAL_ACCURACY;
  XEndOut = XEndOut<<STGFX_INTERNAL_ACCURACY;
  YEndOut = YEndOut<<STGFX_INTERNAL_ACCURACY;
  Points_p[4] = XEndIn;
  Points_p[5] = YEndIn;
  Points_p[6] = XEndOut;
  Points_p[7] = YEndOut;
  
  Err = stgfx_2dRasterSimpleLine(XEndIn, YEndIn, 
                                 XEndOut, YEndOut,
                                 SizeEELineArray, EELineArray);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  Sides_p[TotSidesOut].xylData = EELineArray;
  Sides_p[TotSidesOut].length = *SizeEELineArray;

  /* As ever, we are adjusting Spin so that it increments with
    each change of Up value, but stays the same on consecutive sides
    which go in the same direction. Up = 0 is considered part of whichever
    thrust preceeds it */
    
  if((Sides_p[TotSidesOut-1].Up == Sides_p[TotSidesOut].Up)||
     (Sides_p[TotSidesOut].Up == 0))
  {
    Sides_p[TotSidesOut].Spin = Sides_p[TotSidesOut-1].Spin; 
  }
  else
  {
    Sides_p[TotSidesOut].Spin = Sides_p[TotSidesOut-1].Spin + 1; 
  }
  
  
  SplitArrayXYL( InnerArray, SizeInner, &PosMinIn, &PosMaxIn, IsEllipse,
                 &(Sides_p[TotSidesOut+1]), &TotSidesIn );
  if(InnerArray[PosMinIn].PositionY < MinElement)
  {
    MinElement = InnerArray[PosMinIn].PositionY;
  }
  if(InnerArray[PosMaxIn].PositionY > MaxElement)
  {
    MaxElement = InnerArray[PosMaxIn].PositionY;
  }
  
  /* case 1: the internal arc has just one side */
  if(TotSidesIn == 1)
  {
    if((Sides_p[TotSidesOut+1].Up == Sides_p[TotSidesOut].Up)||
       (Sides_p[TotSidesOut+1].Up == 0))
    {
      Sides_p[TotSidesOut+1].Spin = Sides_p[TotSidesOut].Spin;
    }
    else
    {
      Sides_p[TotSidesOut+1].Spin = Sides_p[TotSidesOut].Spin + 1;
    }
  } 
  else if(TotSidesIn == 2)
  {
    if((Sides_p[TotSidesOut+2].Up == Sides_p[TotSidesOut].Up)||
       (Sides_p[TotSidesOut+2].Up == 0))
    {
      Sides_p[TotSidesOut+2].Spin = Sides_p[TotSidesOut].Spin;
    }
    else
    {
      Sides_p[TotSidesOut+2].Spin = Sides_p[TotSidesOut].Spin + 1;
    }
    if((Sides_p[TotSidesOut+1].Up == Sides_p[TotSidesOut+2].Up)||
       (Sides_p[TotSidesOut+1].Up == 0))
    {
      Sides_p[TotSidesOut+1].Spin = Sides_p[TotSidesOut+2].Spin;
    }
    else
    {
      Sides_p[TotSidesOut+1].Spin = Sides_p[TotSidesOut+2].Spin + 1;
    }
  }
  else /* (TotSidesIn == 3) */
  {
    if((Sides_p[TotSidesOut+3].Up == Sides_p[TotSidesOut].Up)||
       (Sides_p[TotSidesOut+3].Up == 0))
    {
      Sides_p[TotSidesOut+3].Spin = Sides_p[TotSidesOut].Spin;
    }
    else
    {
      Sides_p[TotSidesOut+3].Spin = Sides_p[TotSidesOut].Spin + 1;
    }
    if((Sides_p[TotSidesOut+2].Up == Sides_p[TotSidesOut+3].Up)||
       (Sides_p[TotSidesOut+2].Up == 0))
    {
      Sides_p[TotSidesOut+2].Spin = Sides_p[TotSidesOut+3].Spin;
    }
    else
    {
      Sides_p[TotSidesOut+2].Spin = Sides_p[TotSidesOut+3].Spin + 1;
    } 
    if((Sides_p[TotSidesOut+1].Up == Sides_p[TotSidesOut+2].Up)||
       (Sides_p[TotSidesOut+1].Up == 0))
    {
      Sides_p[TotSidesOut+1].Spin = Sides_p[TotSidesOut+2].Spin;
    }
    else
    {
      Sides_p[TotSidesOut+1].Spin = Sides_p[TotSidesOut+2].Spin + 1;
    }
  }

  /* Calculate start segment endpoints */
  if((InnerArray[0].PositionY- FLOAT_TO_INT1(YC)) < 0)
  {
    XStartIn = (InnerArray[0].PositionX + InnerArray[0].Length-1);
  }
  else
  {
    XStartIn = (InnerArray[0].PositionX); 
  }
  YStartIn = (InnerArray[0].PositionY);
  
  if((OuterArray[0].PositionY- FLOAT_TO_INT1(YC)) < 0)
  {
    XStartOut = (OuterArray[0].PositionX + OuterArray[0].Length-1);
  }
  else
  {
     XStartOut = (OuterArray[0].PositionX);
  }
  YStartOut = OuterArray[0].PositionY;
  
  if(YStartOut<YStartIn)
  {
    Sides_p[TotSidesOut+TotSidesIn+1].Up = 1;
  } 
  else if (YStartOut>YStartIn)
  {
    Sides_p[TotSidesOut+TotSidesIn+1].Up = -1;
  } 
  else /* horizontal line */
  {
    Sides_p[TotSidesOut+TotSidesIn+1].Up = 0;
  }
  
  XStartIn = XStartIn<<STGFX_INTERNAL_ACCURACY;
  YStartIn = YStartIn<<STGFX_INTERNAL_ACCURACY;
  XStartOut = XStartOut<<STGFX_INTERNAL_ACCURACY;
  YStartOut = YStartOut<<STGFX_INTERNAL_ACCURACY;
  Points_p[0] = XStartIn;
  Points_p[1] = YStartIn;
  Points_p[2] = XStartOut;
  Points_p[3] = YStartOut;
  
  /* detect case where start and end segments are the same */
  if((FRACT_TO_INT(XStartIn) == FRACT_TO_INT(XEndIn)) &&
     (FRACT_TO_INT(YStartIn) == FRACT_TO_INT(YEndIn)) &&
     (FRACT_TO_INT(XStartOut) == FRACT_TO_INT(XEndOut)) &&
     (FRACT_TO_INT(YStartOut) == FRACT_TO_INT(YEndOut)))
  {
    SecondLine = FALSE;
    *SizeSELineArray = 0;
    *SizeThicknessArray = 0;
  }
  else
  {
    SecondLine = TRUE;
  }
  
  if(SecondLine)
  {
    /* raster start segment and thickness fill */
    Err = stgfx_2dRasterSimpleLine( XStartIn, 
                                    YStartIn, 
                                    XStartOut, 
                                    YStartOut,
                                    SizeSELineArray, 
                                    SELineArray );
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
    Sides_p[TotSidesOut+TotSidesIn+1].xylData = SELineArray;
    Sides_p[TotSidesOut+TotSidesIn+1].length = *SizeSELineArray;

    if((Sides_p[TotSidesOut+TotSidesIn+1].Up == Sides_p[TotSidesOut+1].Up) ||
       (Sides_p[TotSidesOut+TotSidesIn+1].Up == 0))
    {
      Sides_p[TotSidesOut+TotSidesIn+1].Spin = Sides_p[TotSidesOut+1].Spin;
    }
    if(Sides_p[0].Up == Sides_p[TotSidesOut+TotSidesIn+1].Up)
    {
      Sides_p[0].Spin = Sides_p[TotSidesOut+TotSidesIn+1].Spin;
    }
    
    Err = stgfx_FillGenericPolygon( BHeight, 
                                    MinElement, MaxElement,
                                    Sides_p, (TotSidesIn + TotSidesOut + 2),
                                    ThicknessArray, SizeThicknessArray,
                                    STGFX_WINDING_FILL );
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
  }
  
  /* Now there is a further thing to do:
     the filling function needs the edges overlapping,
     but the boundary which will be drawn can not have
     any kind of pixel overlapping. So the following 
     code erases the first and the last pixel of the
     start and end segments */
  
  /* On the GX1 we can't leave zero-length segments.
    Since our outputs are aggregated by 2dRasterDashedArc,
    we have little alternative but to memmove */
    
  if(SecondLine)
  {
    if(SELineArray[0].Length == 1)
    {
#ifdef STGFX_AVOID_ZERO_LENGTH
      (*SizeSELineArray)--;
      memmove(SELineArray, SELineArray+1,
              (*SizeSELineArray)*sizeof(STBLIT_XYL_t));
#else
      SELineArray[0].Length = 0;
#endif
    }
    else
    {
      SELineArray[0].PositionX += 1;
      SELineArray[0].Length -= 1;
    }
    if(SELineArray[*SizeSELineArray-1].Length == 1)
    {
      *SizeSELineArray -= 1;
    }
    else
    {
      SELineArray[*SizeSELineArray-1].Length -=1;
    }
  }
  
  /* and now the same for the other segment */
  if(EELineArray[0].Length == 1)
  {
#ifdef STGFX_AVOID_ZERO_LENGTH
    (*SizeEELineArray)--;
    memmove(EELineArray, EELineArray+1,
            (*SizeEELineArray)*sizeof(STBLIT_XYL_t));
#else
    EELineArray[0].Length = 0;
#endif
  }
  else
  {
    EELineArray[0].PositionX += 1;
    EELineArray[0].Length -= 1;
  }
  if(EELineArray[*SizeEELineArray-1].Length == 1)
  {
    *SizeEELineArray -= 1;
  }
  else
  {
    EELineArray[*SizeEELineArray-1].Length -=1;
  }
  return(ST_NO_ERROR);
}

   
#if 0 /* only use is by EndPointAndDirection */
/****************************************************************************\
Name        : InnerAndOuterRadius()
Description : when thickness is > 1, this function calculates the length
              of the radiuses (Short and Long) 
              both of the inner and the outer arcs/ellipses
Parameters  : U16 LineWidth, 
              U32 LRadius, 
              U32 SRadius,
              U32* InnerLRadius, 
              U32* InnerSRadius,
              U32* OuterLRadius, 
              U32* OuterSRadius
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
static void InnerAndOuterRadius( 
    U16 LineWidth, 
    U32 LRadius, 
    U32 SRadius,
    U32* InnerLRadius, 
    U32* InnerSRadius,
    U32* OuterLRadius, 
    U32* OuterSRadius
)
{
  BOOL IsWidthOdd;
  /* line width is an even or an odd number? */
  IsWidthOdd = ((LineWidth % 2)== 0) ? FALSE : TRUE;
  /* calculates the internal ellipse axis, depending on LineWidth */
  if(IsWidthOdd)
  {
    *InnerLRadius = LRadius - ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
    *InnerSRadius = SRadius - ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
  }
  else /* LineWidth is even */
  {
    *InnerLRadius = LRadius - ((LineWidth/2 - 1)<<STGFX_INTERNAL_ACCURACY);
    *InnerSRadius = SRadius - ((LineWidth/2 - 1)<<STGFX_INTERNAL_ACCURACY);
  }
  *OuterLRadius = LRadius + ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
  *OuterSRadius = SRadius + ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);
}
#endif

/* This function is used by stgfx_2dArcRasterCap */
void ChooseThePoint( S32* Points, S32* PointsOnNormal,
                     S32* XJ, S32* YJ, BOOL IsStart )
{
  if(IsStart)
  {
    if(Points[1]>Points[3]) /* internal start point is below external point */
    {
      if(PointsOnNormal[0]<PointsOnNormal[2])
      {
        *XJ = PointsOnNormal[0];
        *YJ = PointsOnNormal[1];
      }
      else
      {
        *XJ = PointsOnNormal[2];
        *YJ = PointsOnNormal[3];
      }
    }
    else if(Points[1]<Points[3]) /* internal start point is above ext point */
    {
      if(PointsOnNormal[0]>PointsOnNormal[2])
      {
        *XJ = PointsOnNormal[0];
        *YJ = PointsOnNormal[1];
      }
      else
      {
        *XJ = PointsOnNormal[2];
        *YJ = PointsOnNormal[3];
      }
    }
    else /* int and ext start points have same y */
    {
      if(Points[0]<Points[2]) /* jpoint is the higher */
      {
        if(PointsOnNormal[1]<PointsOnNormal[3])
        {
          *XJ = PointsOnNormal[0];
          *YJ = PointsOnNormal[1];
        }
        else
        {
          *XJ = PointsOnNormal[2];
          *YJ = PointsOnNormal[3];
        }
      }
      else /* the lower */
      {
        if(PointsOnNormal[1]>PointsOnNormal[3])
        {
          *XJ = PointsOnNormal[0];
          *YJ = PointsOnNormal[1];
        }
        else
        {
          *XJ = PointsOnNormal[2];
          *YJ = PointsOnNormal[3];
        }
      }
    }
  }
  else /* so we are studing around the ending point */
  {
    if(Points[5]>Points[7]) /* internal end  point is below external point */
    {
      if(PointsOnNormal[0]>PointsOnNormal[2])
      {
        *XJ = PointsOnNormal[0];
        *YJ = PointsOnNormal[1];
      }
      else
      {
        *XJ = PointsOnNormal[2];
        *YJ = PointsOnNormal[3];
      }
    }
    else if(Points[5]<Points[7]) /* internal end point is above ext point */
    {
      if(PointsOnNormal[0]<PointsOnNormal[2]) /* left! */
      {
        *XJ = PointsOnNormal[0];
        *YJ = PointsOnNormal[1];
      }
      else
      {
        *XJ = PointsOnNormal[2];
        *YJ = PointsOnNormal[3];
      }
    }
    else /* int and ext start points have same y */
    {
      if(Points[4]<Points[6]) /* the lower */
      {
        if(PointsOnNormal[1]>PointsOnNormal[3])
        {
          *XJ = PointsOnNormal[0];
          *YJ = PointsOnNormal[1];
        }
        else
        {
          *XJ = PointsOnNormal[2];
          *YJ = PointsOnNormal[3];
        }
      }
      else /* the higher */
      {
        if(PointsOnNormal[1]<PointsOnNormal[3])
        {
          *XJ = PointsOnNormal[0];
          *YJ = PointsOnNormal[1];
        }
        else
        {
          *XJ = PointsOnNormal[2];
          *YJ = PointsOnNormal[3];
        }
      }
    }
  }
}

/****************************************************************************\
Name        : CheckAngle()
Description : normalizes an angle, making it >=0 and < 2PI
Parameters  : Angle 
Assumptions : 
Limitations : 
Returns     : 
\****************************************************************************/
void CheckAngle(S32* Angle_p)
{
  *Angle_p = *Angle_p % (2*PI); 
  if(*Angle_p < 0)
    *Angle_p += (2*PI);
}

#ifndef ST_GX1 /* apparent compiler bug prevents CalcAlignedSector loop running on GX1 */
/******************************************************************************
Function Name : GetPowers
  Description : Produce powers for the given x < 1. The representation is 16-
                bit fixed point because that ensures x^2 fits within 32 bits,
                but the higher powers will be accurate to somewhat fewer bits,
                hopefully the 11 we actually need
   Parameters : array for results, with x in first element,
                highest power to produce (= # array elems)
  Assumptions : x < 1
******************************************************************************/
static __inline void GetPowers(U32 * Powers_p, U8 MaxPow)
{
    U32 x = *Powers_p;
    U32 u = x;
    
    while(--MaxPow)
    {
        u *= x;
        u >>= TRIG_PREC;
        *++Powers_p = u;
    }
}

/* low-level sin & cos evaluation, 0 <= x <= PI/4 */
#define SIN(Powers_p) \
  (Powers_p[1-1] - Powers_p[3-1]/(3*2) + Powers_p[5-1]/(5*4*3*2))
  /* lowest accuracy is 0.004% at PI/4 */

#define COS(Powers_p) \
  (TRIG_UNITY - Powers_p[2-1]/(2) + Powers_p[4-1]/(4*3*2))
  /* lowest accuracy is 0.05% at PI/4 */
  
/******************************************************************************
Function Name : QuickSin
  Description : Power series evaluation of sin(x), using integer arithmetic
   Parameters : x in 64'ths of a degree, 0 <= x < 360*64
******************************************************************************/
S32 QuickSin(S32 x)
{
    U32 Powers[5];
    S32 Sign = +1;
    
    /* x %= 360*64 */
    /* if(x < 0) x += 360*64; */
    
    if(x >= 180*64)
    {
      x -= 180*64;
      Sign = -1;
    }
    
    if(x > 90*64)
    {
      /* actually only needed above 135*64, but avoids negative
        argument to power series */
        
      x = 180*64 - x;
    }
    
    /* Angle conversion: PI < 4, so PI_TRIG needs 16+2=18 bits.
      The maximum angle, 45*64, needs 6+6 = 12 bits, total 30 */
      
    if(x <= 45*64) /* actual crossover in accuracy is higher */
    {
      Powers[0] = (x * PI_TRIG) / (180*64);
      GetPowers(Powers, 5);
      return Sign * (S32) SIN(Powers);
    }
    else
    {
      Powers[0] = ((90*64 - x) * PI_TRIG) / (180*64);
      GetPowers(Powers, 4);
      return Sign * (S32) COS(Powers);
    }
}

/******************************************************************************
Function Name : QuickCos
  Description : Power series evaluation of cos(x), using integer arithmetic
   Parameters : x in 64'ths of a degree, 0 <= x < 360*64
******************************************************************************/
S32 QuickCos(S32 x)
{
    U32 Powers[5];
    S32 Sign = +1;
    
    /* x %= 360*64 */
    /* if(x < 0) x += 360*64; */
    
    if(x >= 180*64)
    {
      x = 360*64 - x;
    }
    
    if(x > 90*64)
    {
      /* actually only needed above 135*64, but avoids negative
        argument to power series */
        
      x = 180*64 - x;
      Sign = -1;
    }
    
    /* Angle conversion: PI < 4, so PI_TRIG needs 16+2=18 bits.
      The maximum angle, 45*64, needs 6+6 = 12 bits, total 30 */
      
    if(x >= 45*64) /* actual crossover in accuracy is lower */
    {
      Powers[0] = ((90*64 - x) * PI_TRIG) / (180*64);
      GetPowers(Powers, 5);
      return Sign * (S32) SIN(Powers);
    }
    else
    {
      Powers[0] = (x * PI_TRIG) / (180*64);
      GetPowers(Powers, 4);
      return Sign * (S32) COS(Powers);
    }
}

#if 0 /* not actually used */
/******************************************************************************
Function Name : QuickTan
  Description : Power series evaluation of tan(x), using integer arithmetic
   Parameters : x in 64'ths of a degree, 0 <= x < 360*64
******************************************************************************/
S32 QuickTan(S32 x)
{
    S32 Sine = QuickSin(x), Cosine = QuickCos(x);
    
    /* construct as sin/cos, because direct tan series is horrid */

    /* slight inefficiency in evaluating the power series twice */
    
    if(Cosine == 0)
    {
      /* avoid divide by zero */
      return (Sine > 0) ? INT_MAX : INT_MIN;
    }
    
    /* can cater for sign in the normal way, as we already need to shift by only
      15 bits to ensure TRIG_UNITY << TRIG_PREC doesn't overflow */
      
    return (Sine << TRIG_PREC)/Cosine;
}
#endif

/******************************************************************************
Function Name : QuickCot
  Description : Power series evaluation of cot(x), using integer arithmetic
   Parameters : x in 64'ths of a degree, 0 <= x < 360*64
******************************************************************************/
S32 QuickCot(S32 x)
{
    S32 Sine = QuickSin(x), Cosine = QuickCos(x);
    
    /* construct as cos/sin, because direct cot series is horrid */

    /* slight inefficiency in evaluating the power series twice */
    
    if(Sine == 0)
    {
      /* avoid divide by zero */
      return (Cosine > 0) ? INT_MAX : INT_MIN;
    }
    
    /* can cater for sign in the normal way, as we already need to shift by only
      15 bits to ensure TRIG_UNITY << TRIG_PREC doesn't overflow */
      
    return (Cosine << TRIG_PREC)/Sine;
}
#endif /* ndef ST_GX1 */
