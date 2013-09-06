/*******************************************************************************

File Name   : gfx_sector.c

Description : STGFX sector functions, exported and private

COPYRIGHT (C) STMicroelectronics 2001.

*******************************************************************************/

/*#define STTBX_PRINT*/

/*#define DONEPAUSE*/
#ifdef DONEPAUSE
#define STTBX_PRINT
#define DonePause(DescrStr) \
  STTBX_Print((DescrStr " done. Press a key...\n")); getchar()
#else
#define DonePause(DescrStr)
#endif

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "sttbx.h"

#include "gfx_tools.h" /* includes stgfx_init.h, ... */

/* Private Constants -------------------------------------------------------- */

/* Private Macros    -------------------------------------------------------- */

/* Private Types */

/* Private Variables -------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
static void ComputeEllipsePoint(
  U32 HR,     /* horizontal (X) semi axis */
  U32 VR,     /* vertical (Y) semi axis */
  S32 Angle,  /* Angle of the point, in 64ths of degrees */
  S32 Orientation, /* Orientation (rotation of the ellipse, in 64ths of deg) */
  S32* X,          /* pointer to the X to be computed */
  S32* Y           /* pointer to the Y to be computed */
);

/* Static Functions' bodies ------------------------------------------------- */
static void ComputeEllipsePoint(
  U32  HR,     /* horizontal (X) semi axis */
  U32  VR,     /* vertical (Y) semi axis */
  S32  Angle,  /* Angle of the point, in 64ths of degrees */
  S32  Orientation, /* Orientation (rotation of the ellipse, in 64ths of deg) */
  S32* X,          /* pointer to the X to be computed */
  S32* Y           /* pointer to the Y to be computed */
)
{
  double SqRadius; /* square of the radius */
  double dbHR = (double)HR; /* double version of HR */
  double dbVR = (double)VR; /* double version of VR */
  double radAngle; /* double version of Angle in radians */
  double radOrientation; /* double version of Orientation in radians */
 
  radAngle = (PI_FLOAT*(double)Angle)/PI;  
  radOrientation = (PI_FLOAT*(double)Orientation)/PI;
  SqRadius = (dbHR*dbHR*dbVR*dbVR) / ((dbVR*dbVR*cos(radAngle)*cos(radAngle)) +
                                      (dbHR*dbHR*sin(radAngle)*sin(radAngle)));
  SqRadius = sqrt(SqRadius);
  dbHR =  SqRadius*cos(radAngle+radOrientation); /* X in double prec */
  dbVR = -SqRadius*sin(radAngle+radOrientation); /* Y in double prec */

  *X = (S32)dbHR;
  *Y = (S32)dbVR;
}

static int stgfx_CompXYL(const void* a, const void* b);

#if 0 /* Temporary debug */
static void CheckXYL(const STBLIT_XYL_t* XYL_p, U32 XYLNum, char* Descr_p)
{
    U32 i;
    
    /* simple check for suspicious XYL before drawing */
    for(i=0; i < XYLNum; ++i)
    {
        if(XYL_p[i].Length == 0)
            printf("%s[%i] = 0\n", Descr_p, i);
        if(XYL_p[i].Length > 250)
            printf("%s[%i] = %u\n", Descr_p, i, XYL_p[i].Length);
            /* The longest segments in the Sector1 test are just over 200 */
    }
}
#else
#define CheckXYL(XYL_p, XYLNum, Descr_p)
#endif

/* Functions (API) ----------------------------------------------------- */
/*******************************************************************************
Name        : STGFX_DrawSector()
Description : draws a sector according to a specified graphics context
Parameters  :  
Assumptions : The target bitmap, the GC and the parameters are correct
Limitations : 
Returns     : error if a parameter is not correct
*******************************************************************************/
ST_ErrorCode_t STGFX_DrawSector(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         TargetBitmap_p,
  const STGFX_GC_t*         GC_p,
  S32                       XC,
  S32                       YC,
  U32                       HRadius,
  U32                       VRadius,
  S32                       StartAngle,
  U32                       ArcAngle,
  S32                       Orientation,
  STGFX_SectorMode_t        SectorMode
)
{
  /* LastErr stores the most recent non-fatal error */
  ST_ErrorCode_t       Err, LastErr = ST_NO_ERROR;
  STBLIT_BlitContext_t BlitContext; /* blitting context */
  STBLIT_Handle_t      BlitHandle; /* blit handle */
  S32                  Shift; /* shift value */
  S32                  X[2],  Y[2];   /* straight sides' extrema */
  S32                  XJ[2], YJ[2]; /* extrema of the segments used
                                        for joints computation */
  /* straight: if sectormode is STGFX_CHORD_MODE only 0 is used;
     if radius mode is STGFX_RADIUS_MODE both are used */
  STBLIT_XYL_t*          StraightXYL[2];
  U32                    StraightXYLNum[2];
  STAVMEM_BlockHandle_t  StraightXYLBlockHandle[2];
  
  /* arc: if line width is == 1 only 0 is used;
     otherwise all five are used */
  STBLIT_XYL_t*          ArcXYL[5];
  U32                    ArcXYLNum[5];
  STAVMEM_BlockHandle_t  ArcXYLBlockHandle[5];
  /* joints */
  STBLIT_XYL_t*          JointsXYL[3];
  U32                    JointsXYLNum[3];
  STAVMEM_BlockHandle_t  JointsXYLBlockHandle[3];
  
  /* fill: used only if enable filling is TRUE and line style is continuous */
  STBLIT_XYL_t*          FillXYL;
  U32                    FillXYLNum;
  STAVMEM_BlockHandle_t  FillXYLBlockHandle;
  
  stgfx_Side_t         Sides[5]; /* used when EnableFilling is true */
  U32                  i, j; /* counters */
  STGFX_GC_t           LocalGC; /* graphics context */ 
  U32                  iMax;

  double K; /* aspect ratio correction factor */
  double K2; /* aspect ratio correction factor, squared */ 
  double FloatOrientation; /* orientation converted to double prec. */
  S32 Points[8]; /* points which describe the extrema of the arc */
  S32 GeometricPoints[8];
  S32 PointsOnNormal[4]; /* these points which describe the axis on
                            which the joints are computed */
  S32 EndAngle;
  stgfx_JoinSector  JoinSector;

  /* tiling items */
  STGXOBJ_Color_t        Color;
  STGXOBJ_Bitmap_t       MaskBitmap;
  STGXOBJ_Bitmap_t*      Bitmap_p = TargetBitmap_p;
  STAVMEM_BlockHandle_t  MaskDataBH;
  STAVMEM_BlockHandle_t  WorkBufferBH = STAVMEM_INVALID_BLOCK_HANDLE;
  
  
  TIME_HERE("STGFX_DrawSector");
  
  /* with ArcAngle >= 2*PI the sector is an ellipse */
  if(ArcAngle>=2*PI) /* with ArcAngle >= 2*PI the sector is an ellipse */
  {
    return(STGFX_DrawEllipse(Handle, Bitmap_p, GC_p,
                             XC, YC, HRadius, VRadius, Orientation));
  }
  
  /* check handle validity */
  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  
  /* could let InitBlitContext do these checks if we move things about */
  if((GC_p == NULL) || (TargetBitmap_p == NULL))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  LocalGC   = *GC_p;
  
  /* angles are moved to the first circle */
  StartAngle = StartAngle%(2*PI);
  if(StartAngle<0)
  {
    StartAngle += (2*PI);
  }
  Orientation = Orientation%(2*PI);
  if(Orientation<0)
  {
    Orientation += (2*PI);
  }
  EndAngle = StartAngle + (S32)ArcAngle + Orientation;
  
  BlitHandle = stgfx_GetBlitterHandle(Handle);

  /* parameters checking */
  if(SectorMode!=STGFX_CHORD_MODE && SectorMode!=STGFX_RADIUS_MODE)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  if(GC_p->LineWidth == 0)
  {
    return(STGFX_ERROR_INVALID_GC);
  }
  if(GC_p->CapStyle != STGFX_RECTANGLE_CAP &&
     GC_p->CapStyle != STGFX_PROJECTING_CAP &&
     GC_p->CapStyle != STGFX_ROUND_CAP)
  {
    return(STGFX_ERROR_INVALID_GC);
  }
  if (GC_p->JoinStyle != STGFX_RECTANGLE_JOIN &&
      GC_p->JoinStyle != STGFX_FLAT_JOIN      &&
      GC_p->JoinStyle != STGFX_ROUNDED_JOIN)
  {
    return(STGFX_ERROR_INVALID_GC);
  }

  /* degenerate cases */
  if(ArcAngle == 0 && SectorMode==STGFX_CHORD_MODE)
  {
    /* one arc point; invent a cap style */
    ComputeEllipsePoint(HRadius, VRadius, StartAngle, Orientation,
                        &X[0], &Y[0]);
    LocalGC.CapStyle = STGFX_ROUND_CAP;
    return(STGFX_DrawDot(Handle, Bitmap_p, &LocalGC, X[0]+XC, Y[0]+YC));
  }
  else if(ArcAngle == 0 && SectorMode==STGFX_RADIUS_MODE)
  {
    /* one radius line; invent a cap style */
    ComputeEllipsePoint(HRadius, VRadius, StartAngle, Orientation,
                        &X[0], &Y[0]);
    LocalGC.CapStyle = STGFX_ROUND_CAP;
    return(STGFX_DrawLine(Handle, Bitmap_p, &LocalGC, X[0]+XC, Y[0]+YC, XC, YC));
  }


  /* copy pertinent Graphic context params to the blitting context */
  Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
  if (Err != ST_NO_ERROR)
  {
    return(Err);
  }
  
  /* adjust fractional precision */
  Shift = STGFX_INTERNAL_ACCURACY - stgfx_GetUserAccuracy(Handle);
  if(Shift > 0 )
  {
    XC = XC << Shift;
    YC = YC << Shift;
    HRadius = HRadius << Shift; 
    VRadius = VRadius << Shift;
  }
  else if (Shift < 0)
  {
    U8 Sh = -Shift;
    XC = (XC + (1<<(Sh-1))*(XC>=0 ? 1 : -1)) / (1<<Sh);
    YC = (YC + (1<<(Sh-1))*(YC>=0 ? 1 : -1)) / (1<<Sh);
    HRadius = (HRadius + (1<<(Sh-1))) >> Sh; 
    VRadius = (VRadius + (1<<(Sh-1))) >> Sh; 
  }
  /* else, if Shift == 0 do nothing */
  
  
  /* aspect ratio corrections, only if AR is != 1 */
  if(GC_p->XAspectRatio != GC_p->YAspectRatio)
  {
    K = (double)GC_p->YAspectRatio/(double)GC_p->YAspectRatio;
    K2 = K*K;
    FloatOrientation = (PI_FLOAT*(double)Orientation)/PI;  
    
    /* center */
    XC = (XC/GC_p->XAspectRatio)*GC_p->YAspectRatio;
    
    /* semi-axes (HRadius and VRadius */
    HRadius = (U32)(HRadius*sqrt(K2*cos(FloatOrientation)*
                    cos(FloatOrientation) +
                    sin(FloatOrientation)*sin(FloatOrientation)));
    VRadius = (U32)(VRadius*sqrt(K2*sin(FloatOrientation)*
                              sin(FloatOrientation) +
              cos(FloatOrientation)*cos(FloatOrientation)));
  
    FloatOrientation = atan(tan(FloatOrientation)/K); /* corrected */
    Orientation = (S32)((FloatOrientation/PI_FLOAT)*PI);
    /* Arc angle is not modified */
  }

#ifndef ST_GX1 /* apparent compiler bug prevents CalcAlignedSector loop running on GX1 */
  if((GC_p->LineWidth == 1) && (GC_p->LineStyle_p == NULL) &&
     (Orientation % (90*64) == 0) && (GC_p->DrawTexture_p == NULL) &&
     (GC_p->FillTexture_p == NULL) && (GC_p->EnableFilling) &&
     (GC_p->FillColor.Type == GC_p->DrawColor.Type) &&
      stgfx_AreColorsEqual(&(GC_p->FillColor), &(GC_p->DrawColor)) &&
     (SectorMode == STGFX_RADIUS_MODE))
  {
    /* discard fractional parts - the algorithm is integer from here on */
    XC = FRACT_TO_INT(XC);
    YC = FRACT_TO_INT(YC);
    HRadius = FRACT_TO_UINT(HRadius);
    VRadius = FRACT_TO_UINT(VRadius);
  
    FillXYLNum = VRadius*3 + 1;
    Err = stgfx_AVMEM_malloc(Handle, FillXYLNum*sizeof(STBLIT_XYL_t),
                             &FillXYLBlockHandle, (void**) &FillXYL);
    if(Err != ST_NO_ERROR)
    {
      return(Err);
    }
    
    TIME_HERE("STGFX_DrawSector setup and avmem allocation done");
    
    Err = stgfx_CalcAlignedSector(Handle, TRUE, XC, YC,
                                  (U16)HRadius, (U16)VRadius,
                                  StartAngle, ArcAngle, FillXYL, &FillXYLNum);
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
      return(Err);
    }
    
    TIME_HERE("STGFX_DrawSector xyl calculation done");
    /*printf("STGFX_DrawSector aligned case will draw %u segments\n",
             FillXYLNum);*/
    
    CheckXYL(FillXYL, FillXYLNum, "STGFX_DrawSector Optimised FilXYL");
    BlitContext.UserTag_p = (void*)FillXYLBlockHandle;
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, FillXYL,
                              FillXYLNum, (STGXOBJ_Color_t*) &(GC_p->DrawColor),
                              &BlitContext);
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }

    TIME_SYNC(Handle);
    TIME_HERE("STGFX_DrawSector drawing done");
    return(LastErr);
  }
#endif /* ndef ST_GX1 */

  /* arc allocation(s) */
  /* the internal arc is used in EVERY case */
  ArcXYLNum[0] = ArcXYLNum[1] = ArcXYLNum[2] = 2*(GC_p->LineWidth + 2 +
        4*GFXMAX(FRACT_TO_UINT(HRadius),FRACT_TO_UINT(VRadius)));
  ArcXYLNum[3] = ArcXYLNum[4] = 2*(GC_p->LineWidth + 2);
  
  iMax = (GC_p->LineWidth==1) ? 1 : 5;
  if(GC_p->LineStyle_p != NULL) /* dashed */
  {
    for(i=0; i<iMax; i++)
    {
      ArcXYLNum[i] *= GFXMAX(FRACT_TO_UINT(HRadius),FRACT_TO_UINT(VRadius))/8;
    }
  }
  for(i=0; i<iMax; i++)
  {    
    Err = stgfx_AVMEM_malloc(Handle, ArcXYLNum[i]*sizeof(STBLIT_XYL_t),
                             &ArcXYLBlockHandle[i], (void**) &ArcXYL[i]);
    if(Err != ST_NO_ERROR)
    {
      /* free blocks and return */
      for(j=0; j<i; j++)
      {
        stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
      }
      return(Err);
    }
  }
  
  /* arc rendering */
  if(GC_p->LineWidth == 1)
  {
    if(GC_p->LineStyle_p == NULL) /* undashed simple arc */
    {
      Err = stgfx_2dRasterSimpleArc(XC, YC, HRadius, VRadius,
                                    StartAngle, ArcAngle, Orientation,
                                    &ArcXYLNum[0], ArcXYL[0], Points);
    }
    else /* dashed thin arc (linewidth==1) */
    {
      Err = stgfx_2dRasterDashedArc(Bitmap_p->Height, XC, YC, 
                                    HRadius, VRadius,
                                    StartAngle, ArcAngle, Orientation,
                                    GC_p->LineStyle_p, 1,
                                    &ArcXYLNum[0], ArcXYL[0],
                                    NULL, NULL,
                                    NULL, NULL,
                                    NULL, NULL,
                                    NULL, NULL,
                                    Points, NULL);
    }

    if(Err != ST_NO_ERROR)
    {
      /* free blocks and return */
      stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[0]);
      return(Err);      
    }

    X[0] = Points[0];
    Y[0] = Points[1];
    X[1] = Points[2];
    Y[1] = Points[3];
  }
  else
  {
    if(GC_p->LineStyle_p == NULL) /* undashed thick arcs */
    {
      Err = stgfx_2dRasterThickArc(Bitmap_p->Height, XC, YC, 
                                   HRadius, VRadius,
                                   StartAngle, ArcAngle, Orientation,
                                   GC_p->LineWidth,
                                   &ArcXYLNum[0], ArcXYL[0],
                                   &ArcXYLNum[1], ArcXYL[1],
                                   &ArcXYLNum[2], ArcXYL[2],
                                   &ArcXYLNum[3], ArcXYL[3],
                                   &ArcXYLNum[4], ArcXYL[4],
                                   Points, GeometricPoints);
    }
    else /* dashed thick */
    {
      Err = stgfx_2dRasterDashedArc(Bitmap_p->Height, XC, YC, 
                                    HRadius, VRadius,
                                    StartAngle, ArcAngle, Orientation,
                                    GC_p->LineStyle_p, GC_p->LineWidth,
                                    &ArcXYLNum[0], ArcXYL[0],
                                    &ArcXYLNum[1], ArcXYL[1],
                                    &ArcXYLNum[2], ArcXYL[2],
                                    &ArcXYLNum[3], ArcXYL[3],
                                    &ArcXYLNum[4], ArcXYL[4],
                                    Points, GeometricPoints);
    }

    if(Err != ST_NO_ERROR)
    {
      /* free blocks and return */
      for(i=0; i<5; i++)
      {
        stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
      }
      return(Err);      
    }
    
    /* computation of the geometry */
    X[0] = (Points[0]+Points[2])/2;
    Y[0] = (Points[1]+Points[3])/2;
    X[1] = (Points[4]+Points[6])/2;
    Y[1] = (Points[5]+Points[7])/2;

    /* find the 2 candidate points for the definition of the start joint */
    FindPointOnNormal(GeometricPoints, PointsOnNormal,
                      2*(GC_p->LineWidth<<STGFX_INTERNAL_ACCURACY));
                      
    /* Choice of the point */
    if(Points[1]>Points[3]) /* internal start point is below external point */
    {
      if(PointsOnNormal[0]<PointsOnNormal[2])
      {
        XJ[0] = PointsOnNormal[0];
        YJ[0] = PointsOnNormal[1];
      }
      else
      {
        XJ[0] = PointsOnNormal[2];
        YJ[0] = PointsOnNormal[3];
      }
    }
    else if(Points[1]<Points[3]) /* internal start point is above ext point */
    {
      if(PointsOnNormal[0]>PointsOnNormal[2])
      {
        XJ[0] = PointsOnNormal[0];
        YJ[0] = PointsOnNormal[1];
      }
      else
      {
        XJ[0] = PointsOnNormal[2];
        YJ[0] = PointsOnNormal[3];
      }
    }
    else /* int and ext start points have same y */
    {
      if(Points[0]<Points[2]) /* jpoint is the higher */
      {
        if(PointsOnNormal[1]<PointsOnNormal[3])
        {
          XJ[0] = PointsOnNormal[0];
          YJ[0] = PointsOnNormal[1];
        }
        else
        {
          XJ[0] = PointsOnNormal[2];
          YJ[0] = PointsOnNormal[3];
        }
      }
      else /* the lower */
      {
        if(PointsOnNormal[1]>PointsOnNormal[3])
        {
          XJ[0] = PointsOnNormal[0];
          YJ[0] = PointsOnNormal[1];
        }
        else
        {
          XJ[0] = PointsOnNormal[2];
          YJ[0] = PointsOnNormal[3];
        }
      }
    }
   
    /* find the 2 candidate points for the definition of the end joint */
    FindPointOnNormal(&GeometricPoints[4], PointsOnNormal,
                      2*(GC_p->LineWidth<<STGFX_INTERNAL_ACCURACY));
    /* Choice of the point */
    if(Points[5]>Points[7]) /* internal end  point is below external point */
    {
      if(PointsOnNormal[0]>PointsOnNormal[2])
      {
        XJ[1] = PointsOnNormal[0];
        YJ[1] = PointsOnNormal[1];
      }
      else
      {
        XJ[1] = PointsOnNormal[2];
        YJ[1] = PointsOnNormal[3];
      }
    }
    else if(Points[5]<Points[7]) /* internal end point is above ext point */
    {
      if(PointsOnNormal[0]<PointsOnNormal[2]) /* left! */
      {
        XJ[1] = PointsOnNormal[0];
        YJ[1] = PointsOnNormal[1];
      }
      else
      {
        XJ[1] = PointsOnNormal[2];
        YJ[1] = PointsOnNormal[3];
      }
    }
    else /* int and ext start points have same y */
    {
      if(Points[4]<Points[6]) /* the lower */
      {
        if(PointsOnNormal[1]>PointsOnNormal[3])
        {
          XJ[1] = PointsOnNormal[0];
          YJ[1] = PointsOnNormal[1];
        }
        else
        {
          XJ[1] = PointsOnNormal[2];
          YJ[1] = PointsOnNormal[3];
        }
      }
      else /* the higher */
      {
        if(PointsOnNormal[1]<PointsOnNormal[3])
        {
          XJ[1] = PointsOnNormal[0];
          YJ[1] = PointsOnNormal[1];
        }
        else
        {
          XJ[1] = PointsOnNormal[2];
          YJ[1] = PointsOnNormal[3];
        }
      }
    }
  }

  /* straight sides allocations */
  /* depending on the sector mode we need 1 or two XYL arrays */
  if(SectorMode == STGFX_CHORD_MODE) /* 1 array is needed */
  {
    StraightXYLNum[0] = GFXABS(FRACT_TO_INT(Y[0])-FRACT_TO_INT(Y[1])) +
                          GC_p->LineWidth + 2;
                          
    if(GC_p->LineStyle_p != NULL)
    {
      StraightXYLNum[0] = (GFXABS(FRACT_TO_INT(Y[0])-FRACT_TO_INT(Y[1])) +
                           GFXABS(FRACT_TO_INT(X[0])-FRACT_TO_INT(X[1])))*
                             GC_p->LineWidth;
    }
    
    Err = stgfx_AVMEM_malloc(Handle, StraightXYLNum[0]*sizeof(STBLIT_XYL_t),
                             &StraightXYLBlockHandle[0],
                             (void**) &StraightXYL[0]);
                             
    /* ArcBlock free and return below for (Err != ST_NO_ERROR) */
  }
  else /* RADIUS mode: 2 arrays are needed */
  {
    for(i=0; i<2; i++)
    {
      StraightXYLNum[i] = GFXABS(FRACT_TO_INT(YC)-FRACT_TO_INT(Y[i])) +
                          GC_p->LineWidth + 2;
                          
      /* for dashed lines we need extra space */
      if(GC_p->LineStyle_p != NULL)
      {
        StraightXYLNum[i] = (GFXABS(FRACT_TO_INT(YC)-FRACT_TO_INT(Y[i])) +
                             GFXABS(FRACT_TO_INT(XC)-FRACT_TO_INT(X[i])))*
                             GC_p->LineWidth;
      }
      
      Err = stgfx_AVMEM_malloc(Handle, StraightXYLNum[i]*sizeof(STBLIT_XYL_t),
                               &StraightXYLBlockHandle[i],
                               (void**) &StraightXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<i; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
        }
        break; /* ArcBlock free and return below */
      }
    }
  } /* straight SIDE(S) allocation completed */
  
  if(Err != ST_NO_ERROR)
  {
    /* free arc blocks and return */
    iMax = (GC_p->LineWidth==1) ? 1 : 5;
    for(i=0; i<iMax; i++)
    {
      stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
    }
    return(Err);      
  }
  
  
  /* straight sides rendering */
  if(SectorMode == STGFX_RADIUS_MODE)
  {
    for(i=0; i<2; i++)
    {
      /* choice of the rasterizing function */
      if(GC_p->LineWidth==1 && GC_p->LineStyle_p==NULL) /* thin undashed */
        Err = stgfx_2dRasterSimpleLine(XC, YC, X[i], Y[i],
                               &StraightXYLNum[i], StraightXYL[i]);
      else if(GC_p->LineWidth>1 && GC_p->LineStyle_p==NULL) /* thick undashed */
        Err = stgfx_2dRasterThickLine(Handle, 
                                      XC, YC, X[i], Y[i],
                                      GC_p->LineWidth,
                                      &StraightXYLNum[i], StraightXYL[i]);
      else/* dashed */
        Err = stgfx_2dRasterDashedLine(Handle, 
                                      XC, YC, X[i], Y[i],
                                      GC_p->LineStyle_p, GC_p->LineWidth,
                                      &StraightXYLNum[i], StraightXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<2; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
        }
        /* ArcBlock free and return below */
      }
    }
  }
  else /* SectorMode == STGFX_CHORD_MODE */
  {
    /* choice of the rasterizing function */
    if(GC_p->LineWidth==1 && GC_p->LineStyle_p==NULL) /* thin undashed */
    {
      Err = stgfx_2dRasterSimpleLine(X[0], Y[0], X[1], Y[1],
                             &StraightXYLNum[0], StraightXYL[0]);
    }
    else if(GC_p->LineWidth>1 && GC_p->LineStyle_p==NULL) /* thick undashed */
    {
      Err = stgfx_2dRasterThickLine(Handle, X[0], Y[0], X[1], Y[1],
                                    GC_p->LineWidth,
                                    &StraightXYLNum[0], StraightXYL[0]);
    }
    else /* dashed */
    {
      Err = stgfx_2dRasterDashedLine(Handle, X[0], Y[0], X[1], Y[1],
                                     GC_p->LineStyle_p, GC_p->LineWidth,
                                     &StraightXYLNum[0], StraightXYL[0]);
    }
    
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
      /* ArcBlock free and return below */
    }
  }

  if(Err != ST_NO_ERROR)
  {
    /* free blocks and return */
    iMax = (GC_p->LineWidth==1) ? 1 : 5;
    for(i=0; i<iMax; i++)
    {
      stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
    }
    return(Err);      
  }
  
  /* straight side(s) completed */
  
  /*****************************************************************/
  /* joints: rendering (allocation is done by stgfx_2dRasterJoin() */
  /*****************************************************************/
  if(GC_p->LineWidth>1) /* if LineWidth==1 there are no joints */
  {
    if(SectorMode == STGFX_CHORD_MODE) /* chord implies 2 joints */
    {
      JoinSector = ArcLine;
      
      /* Joint 0 */
      Err = stgfx_2dRasterJoin(Handle, X[1], Y[1], X[0], Y[0], XJ[0], YJ[0],
                               Points[0], Points[1], Points[2], Points[3], 
                               JoinSector, GC_p->LineWidth, GC_p->JoinStyle,
                               GC_p->LineStyle_p, &JointsXYLBlockHandle[0],
                               &JointsXYL[0], &JointsXYLNum[0]);
      if(Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        for(j=0; j<5; j++) /* must be 5 by LineWidth > 1 */
        {
          stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]); /* 1 by CHORD */
        return(Err);      
      }

      /* Joint 1 */
      Err = stgfx_2dRasterJoin(Handle, X[0], Y[0], X[1], Y[1], XJ[1], YJ[1],
                               Points[4], Points[5], Points[6], Points[7], 
                               JoinSector, GC_p->LineWidth, GC_p->JoinStyle,
                               GC_p->LineStyle_p, &JointsXYLBlockHandle[1],
                               &JointsXYL[1], &JointsXYLNum[1]);
      if(Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        for(j=0; j<5; j++)
        {
          stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[0]);
        return(Err);      
      }
    }
    else /* radius implies 3 joints  */
    { 
      /* Joint 0 */
      JoinSector = ArcLine;
      Err = stgfx_2dRasterJoin(Handle, XC, YC, X[0], Y[0], XJ[0], YJ[0],
                               Points[0], Points[1], Points[2], Points[3],
                               JoinSector, GC_p->LineWidth, GC_p->JoinStyle,
                               GC_p->LineStyle_p, &JointsXYLBlockHandle[0],
                               &JointsXYL[0], &JointsXYLNum[0]);
      if(Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        for(j=0; j<5; j++)
        {
          stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
        return(Err);      
      }

      /* Joint 1 */
      JoinSector = LineLine;
      Err = stgfx_2dRasterJoin(Handle, X[0], Y[0], XC, YC, X[1], Y[1],
                               0, 0, 0, 0,
                               JoinSector, GC_p->LineWidth, GC_p->JoinStyle,
                               GC_p->LineStyle_p, &JointsXYLBlockHandle[1],
                               &JointsXYL[1], &JointsXYLNum[1]);
      if(Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        for(j=0; j<5; j++)
        {
           stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[0]);
        return(Err);      
      }

      /* Joint 2 */
      JoinSector = ArcLine;
      Err = stgfx_2dRasterJoin(Handle, XC, YC, X[1], Y[1], XJ[1], YJ[1],
                               Points[4], Points[5], Points[6], Points[7],
                               JoinSector, GC_p->LineWidth, GC_p->JoinStyle,
                               GC_p->LineStyle_p, &JointsXYLBlockHandle[2],
                               &JointsXYL[2], &JointsXYLNum[2]);
      if(Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        for(j=0; j<5; j++)
        {
          stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[1]);
        return(Err);      
      }
    }
    
    /*  the central joint is merged with the 2 rays */
    if (GC_p->LineWidth > 1 && SectorMode==STGFX_RADIUS_MODE &&
        GC_p->LineStyle_p == NULL)
    {
      STBLIT_XYL_t*          NewStraightXYL[2];
      U32                    NewStraightXYLNum[2];
      STAVMEM_BlockHandle_t  NewStraightXYLBlockHandle[2];
      S32 TopStraight, BotStraight, TopJoint, BotJoint;
      S32 k, n;

      /* allocation */
      NewStraightXYLNum[0] = StraightXYLNum[0] + JointsXYLNum[1];
      NewStraightXYLNum[1] = StraightXYLNum[1] + JointsXYLNum[1];
    
      Err = stgfx_AVMEM_malloc(Handle, 
                               NewStraightXYLNum[0]*sizeof(STBLIT_XYL_t),
                               &NewStraightXYLBlockHandle[0],
                               (void**) &NewStraightXYL[0]);
      if(Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        for(j=0; j<5; j++)
        {
          stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[1]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[2]);
        return(Err);      
      }
      
      Err = stgfx_AVMEM_malloc(Handle, 
                               NewStraightXYLNum[1]*sizeof(STBLIT_XYL_t),
                               &NewStraightXYLBlockHandle[1],
                               (void**) &NewStraightXYL[1]);
      if(Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        for(j=0; j<5; j++)
        {
          stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[j]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &NewStraightXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[0]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[1]);
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[2]);
        return(Err);      
      }

      for(j=0; j<2; j++)
      {
        /* search of the indexes of XYL elements at the same Y */
        if(JointsXYL[1][0].PositionY<=StraightXYL[j][0].PositionY)
        {
          TopStraight = 0;
          TopJoint = StraightXYL[j][0].PositionY - JointsXYL[1][0].PositionY;
        }
        else
        {
          TopJoint = 0;
          TopStraight = JointsXYL[1][0].PositionY -
                           StraightXYL[j][0].PositionY;
        }
        
        if(JointsXYL[1][JointsXYLNum[1]-1].PositionY <= 
           StraightXYL[j][StraightXYLNum[j]-1].PositionY)
        {
          BotStraight = JointsXYL[1][JointsXYLNum[1]-1].PositionY -
                           StraightXYL[j][0].PositionY;
          BotJoint = JointsXYLNum[1]-1;
        }
        else
        {
          BotStraight = StraightXYLNum[j]-1;
          BotJoint = StraightXYL[j][StraightXYLNum[j]-1].PositionY - 
                     JointsXYL[1][0].PositionY;
        }
        
        /* copy unchanged fields straight */
        for(i=0, k=0; i<TopStraight; i++, k++)
        {
          NewStraightXYL[j][k] = StraightXYL[j][i];
        }
        /* copy changed fields */
        for(i=TopStraight, n=0; i<=BotStraight; i++, k++, n++)
        {
          NewStraightXYL[j][k].PositionY = JointsXYL[1][TopJoint+n].PositionY;
          NewStraightXYL[j][k].PositionX =
                      GFXMIN(StraightXYL[j][i].PositionX,
                             JointsXYL[1][TopJoint+n].PositionX);
          NewStraightXYL[j][k].Length =
                   GFXMAX(StraightXYL[j][i].PositionX+
                          (S32)StraightXYL[j][i].Length,
                          JointsXYL[1][TopJoint+n].PositionX +
                          (S32)JointsXYL[1][TopJoint+n].Length) -
                          NewStraightXYL[j][k].PositionX;
        }
        for(i=BotStraight+1; i<StraightXYLNum[j]; i++, k++)
        {
          NewStraightXYL[j][k] = StraightXYL[j][i];
        }
       
        /* copy unchanged fields joint */
        if(TopJoint == 0)
        {
          for(i=BotJoint+1; i<JointsXYLNum[1]; i++, k++)
          {
            NewStraightXYL[j][k] = JointsXYL[1][i];
          }
        }
        else
        {
          for(i=0; i<TopJoint; i++, k++)
          {
            NewStraightXYL[j][k] = JointsXYL[1][i];
          }
        }
       
        /* free original data now we've processed it */
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
        
        /* put new data in old slots */
        StraightXYL[j] = NewStraightXYL[j];
        StraightXYLBlockHandle[j] = NewStraightXYLBlockHandle[j];
        StraightXYLNum[j] = k;
      }
    
      /* the new straight XYLs must be re-sorted */
      qsort(StraightXYL[0], StraightXYLNum[0], sizeof(STBLIT_XYL_t),
            stgfx_CompXYL);
      qsort(StraightXYL[1], StraightXYLNum[1], sizeof(STBLIT_XYL_t),
            stgfx_CompXYL);
    } /* end of central joint reprocessing */
  } /* end of joints management / rendering */
  
  
  /*******************************/
  /* rendering of the contour is now  completed */
  /*******************************/
  
  /* first tiling analysis */
  if((GC_p->DrawTexture_p != NULL) || (GC_p->FillTexture_p != NULL))
  {
    Err = stgfx_InitTexture(Handle, GC_p, TargetBitmap_p, &MaskBitmap,
                            &BlitContext.WorkBuffer_p, &WorkBufferBH);
    if (Err != ST_NO_ERROR)
    {
      /* free blocks and return */
      iMax = (GC_p->LineWidth==1) ? 1 : 5;
      for(i=0; i<iMax; i++)
      {
        stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
      if(SectorMode == STGFX_RADIUS_MODE)
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
      if(GC_p->LineWidth>1)
      {
        iMax = (SectorMode == STGFX_RADIUS_MODE) ? 3 : 2;
        for(i=0; i<iMax;  i++)
          stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[i]);
      }
      return(Err);      
    }
  }

  /*******************************/
  /* FILLING                     */
  /*******************************/
  
  /* filling */
  if(GC_p->EnableFilling == TRUE && GC_p->LineStyle_p == NULL)
  {
    U32           Straight1stIdx;
    U32           First, Last;
    STBLIT_XYL_t* IntArcXYL = ArcXYL[0];
    U32           IntArcXYLNum = ArcXYLNum[0];
  
    /* the arc must be scanned in order to find the sides
    in whom it can be split */
    /* sides preparation */
    Sides[1].xylData = NULL; /* used as a flag: if this field is not
                                modified during the following for cycle,
                                the arc has 1 only polygon side */
    Sides[0].xylData = &IntArcXYL[0];
    Sides[0].Spin = 0;
    Sides[0].Up = (IntArcXYL[0].PositionY<IntArcXYL[1].PositionY) ? -1 : 1;
    for(i=1; i<=(IntArcXYLNum-2); i++)
    {
      if(IntArcXYL[i-1].PositionY == IntArcXYL[i+1].PositionY)
      {
        Sides[0].length = i+1;
        Sides[1].xylData = &IntArcXYL[i];
        First = i+1;
        break;
      }
    }
    if(Sides[1].xylData == NULL) /* arc gives 1 side to polygon */
    {
      Sides[0].length = IntArcXYLNum;
      Straight1stIdx = 1;
    }
    else
    {
      Sides[2].xylData = NULL; /* used as a flag: if this field is not
                                modified during the following for cycle,
                                the arc has 2 polygon sides */
      Sides[1].Spin = 1;
      Sides[1].Up = -Sides[0].Up;
      for(i=First; i<=(IntArcXYLNum-2); i++)
      {
        if(IntArcXYL[i-1].PositionY == IntArcXYL[i+1].PositionY)
        {
          Sides[1].length = (i+1) - First;
          Sides[2].xylData = &IntArcXYL[i];
          Sides[2].length =  IntArcXYLNum - i;
          Sides[2].Up = -Sides[1].Up;
          Sides[2].Spin = 2;
          Sides[2].xylData = &IntArcXYL[i];
          Straight1stIdx = 3;
          break;
        }
      }
      if(Sides[2].xylData == NULL) /* arc gives 2 sides to polygon */
      {
        Sides[1].length = IntArcXYLNum - First + 1;
        Straight1stIdx = 2;
      }
    } /* preparation of the polygon sides belonging to
         the internal arc completed */ 
    
    /* preparation of the straight side(s) : radii or chord */
    if(SectorMode == STGFX_CHORD_MODE)
    {
      Sides[Straight1stIdx].xylData = StraightXYL[0];
      Sides[Straight1stIdx].length = StraightXYLNum[0];
      if(Y[1]>Y[0])
        Sides[Straight1stIdx].Up = 1;
      else if(Y[1]<Y[0])
        Sides[Straight1stIdx].Up = -1;
      else
        Sides[Straight1stIdx].Up = 0;
      if(Sides[Straight1stIdx].Up == Sides[Straight1stIdx-1].Up)
        Sides[Straight1stIdx].Spin = Sides[Straight1stIdx-1].Spin;
      else
        Sides[Straight1stIdx].Spin = Sides[Straight1stIdx-1].Spin+1;
          
      /* find start element...........*/
      if(GC_p->LineWidth>1)
      {
        for(i=0; i<StraightXYLNum[0]; i++)
        {
          if(StraightXYL[0][i].PositionY == IntArcXYL[0].PositionY)
          {
            First = i;
            break;
          }
        }
        /* find last */
        for(i=0; i<StraightXYLNum[0]; i++)
        {
          if(StraightXYL[0][i].PositionY == IntArcXYL[IntArcXYLNum-1].PositionY)
          {
            Last = i;
            break;
          }
        }
        if(First<Last)
        {
          Sides[Straight1stIdx].xylData = &Sides[Straight1stIdx].xylData[First];
          Sides[Straight1stIdx].length = Last - First + 1;
        }
        else if(Last<First)
        {
          GFXSWAP(First , Last);
          Sides[Straight1stIdx].xylData = &Sides[Straight1stIdx].xylData[First];
          Sides[Straight1stIdx].length = Last - First + 1;        
        }
        else
        {
          Sides[Straight1stIdx].xylData = &Sides[Straight1stIdx].xylData[First];
          Sides[Straight1stIdx].length = 1;
        }      
      }
    }
    else /* RADIUS sector mode */
    {
      Sides[Straight1stIdx].xylData = StraightXYL[1];
      Sides[Straight1stIdx].length = StraightXYLNum[1];
      Sides[Straight1stIdx+1].xylData = StraightXYL[0];
      Sides[Straight1stIdx+1].length = StraightXYLNum[0];
      
      if(Y[1]>YC)
        Sides[Straight1stIdx].Up = 1;
      else if(Y[1]<YC)
        Sides[Straight1stIdx].Up = -1;
      else
        Sides[Straight1stIdx].Up = 0;
      if(Sides[Straight1stIdx].Up == Sides[Straight1stIdx-1].Up)
        Sides[Straight1stIdx].Spin = Sides[Straight1stIdx-1].Spin;
      else
        Sides[Straight1stIdx].Spin = Sides[Straight1stIdx-1].Spin+1;
      
      if(YC>Y[0])
        Sides[Straight1stIdx+1].Up = 1;
      else if(YC<Y[0])
        Sides[Straight1stIdx+1].Up = -1;
      else
        Sides[Straight1stIdx+1].Up = 0;
      if(Sides[Straight1stIdx+1].Up == Sides[Straight1stIdx].Up)
        Sides[Straight1stIdx+1].Spin = Sides[Straight1stIdx].Spin;
      else
        Sides[Straight1stIdx+1].Spin = Sides[Straight1stIdx].Spin+1;

      
      /* find start element  of the (Straight1stIdx)th side (straight 1) */
      if(GC_p->LineWidth>1)
      {
        j = 0; /* j used as a flag */
        for(i=0; i<StraightXYLNum[1]; i++)
        {
          if(StraightXYL[1][i].PositionY == ArcXYL[0][ArcXYLNum[0]-1].PositionY)
          {
            First = i;
            j = 1;
            break;
          }
        }      
      
        if(j)
        {
          if(Y[1]<YC)
          {
            Sides[Straight1stIdx].length = StraightXYLNum[1] - First;
            Sides[Straight1stIdx].xylData =
                                          &Sides[Straight1stIdx].xylData[First];
          }
          else if(Y[1]>YC)
          {
            Sides[Straight1stIdx].length = First + 1;
          }
        }
      
        /* find start element  of (Straight1stIdx+1)th side (straight 0) */
        j = 0;
        for(i=0; i<StraightXYLNum[0]; i++)
        {
          if(StraightXYL[0][i].PositionY==ArcXYL[0][0].PositionY)
          {
            First = i;
            j = 1;
            break;
          }
        }
        if(j)
        {
          if(Y[0]<YC)
          {
            Sides[Straight1stIdx+1].length = StraightXYLNum[0] - First;
            Sides[Straight1stIdx+1].xylData =
                                        &Sides[Straight1stIdx+1].xylData[First];
          }
          else if(Y[0]>YC)
          {
            Sides[Straight1stIdx+1].length = First + 1;
          }
        }
      }
    }    
    
    if(SectorMode == STGFX_CHORD_MODE)
      iMax = Straight1stIdx + 1;
    else
      iMax = Straight1stIdx + 2;
    
    for(i=iMax-1; i>0; i--)
    {
      if(Sides[i].Up == Sides[0].Up)
        Sides[i].Spin = 0;
      else
        break;
    }

    
    /* filling allocation */
    FillXYLNum = 4*(GFXMAX(FRACT_TO_UINT(VRadius), FRACT_TO_UINT(HRadius)));
    Err = stgfx_AVMEM_malloc(Handle, FillXYLNum*sizeof(STBLIT_XYL_t),
                             &FillXYLBlockHandle, (void**) &FillXYL);
    if(Err != ST_NO_ERROR)
    {
      /* free blocks and return */
      iMax = (GC_p->LineWidth==1) ? 1 : 5;
      for(i=0; i<iMax; i++)
      {
        stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
      if(SectorMode == STGFX_RADIUS_MODE)
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
      if(GC_p->LineWidth>1)
      {
        iMax = (SectorMode == STGFX_RADIUS_MODE) ? 3 : 2;
        for(i=0; i<iMax;  i++)
          stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &WorkBufferBH); /* if used */
      return(Err);      
    }
    
    /* compute filling */
    if(SectorMode == STGFX_CHORD_MODE)
      i = Straight1stIdx + 1;
    else
      i = Straight1stIdx + 2;
    Err = stgfx_FillGenericPolygon(Bitmap_p->Height,
                                FRACT_TO_INT(YC)-GFXMAX(FRACT_TO_UINT(HRadius),
                                FRACT_TO_UINT(VRadius))-1,
                                FRACT_TO_INT(YC)+GFXMAX(FRACT_TO_UINT(HRadius),
                                FRACT_TO_UINT(VRadius))+1,
                                Sides,
                                i,
                                FillXYL,
                                &FillXYLNum,
                                STGFX_WINDING_FILL);
    if(Err != ST_NO_ERROR)
    {
      /* free blocks and return */
      iMax = (GC_p->LineWidth==1) ? 1 : 5;
      for(i=0; i<iMax; i++)
      {
        stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
      if(SectorMode == STGFX_RADIUS_MODE)
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
      if(GC_p->LineWidth>1)
      {
        iMax = (SectorMode == STGFX_RADIUS_MODE) ? 3 : 2;
        for(i=0; i<iMax;  i++)
          stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
      stgfx_AVMEM_free(Handle, &WorkBufferBH);
      return(Err);      
    }    
    
    /* reprocess filling for non COPY alu modes */
    if(GC_p->AluMode != STBLIT_ALU_COPY)
    {
      /* side 0 */
      for(i=0; i<StraightXYLNum[0]; i++)
      {
        for(j=0; j<FillXYLNum; j++)
        {
          if(StraightXYL[0][i].PositionY != FillXYL[j].PositionY)
            continue;
          if(((FillXYL[j].PositionX+FillXYL[j].Length-1) >=
              StraightXYL[0][i].PositionX) &&
             (FillXYL[j].PositionX <= StraightXYL[0][i].PositionX))
          {
            FillXYL[j].Length = StraightXYL[0][i].PositionX - 
                                FillXYL[j].PositionX;
          }
          else if(((StraightXYL[0][i].PositionX+StraightXYL[0][i].Length-1) >=
                   FillXYL[j].PositionX) &&
                   (StraightXYL[0][i].PositionX <= FillXYL[j].PositionX))
          {
            FillXYL[j].Length = FillXYL[j].PositionX+FillXYL[j].Length -
                                (StraightXYL[0][i].PositionX +
                                StraightXYL[0][i].Length);
            FillXYL[j].PositionX = StraightXYL[0][i].PositionX +
                                   StraightXYL[0][i].Length;
          }
        }
      }
      if(SectorMode == STGFX_RADIUS_MODE)
      {
        for(i=0; i<StraightXYLNum[1]; i++)
        {
          for(j=0; j<FillXYLNum; j++)
          {
            if(StraightXYL[1][i].PositionY != FillXYL[j].PositionY)
              continue;
            if(((FillXYL[j].PositionX+FillXYL[j].Length-1) >=
                StraightXYL[1][i].PositionX) &&
              (FillXYL[j].PositionX <= StraightXYL[1][i].PositionX))
            {
              FillXYL[j].Length = StraightXYL[1][i].PositionX - 
                                  FillXYL[j].PositionX;
            }
            else if(((StraightXYL[1][i].PositionX+StraightXYL[1][i].Length-1) >=
                     FillXYL[j].PositionX) &&
                     (StraightXYL[1][i].PositionX <= FillXYL[j].PositionX))
            {
              FillXYL[j].Length = FillXYL[j].PositionX+FillXYL[j].Length -
                                  (StraightXYL[1][i].PositionX +
                                  StraightXYL[1][i].Length);
              FillXYL[j].PositionX = StraightXYL[1][i].PositionX +
                                     StraightXYL[1][i].Length;
            }
          }
        }
      }
    }

    /* preparation for fill drawing */
    if(GC_p->FillTexture_p == NULL)
    {
      /* direct draw to target bitmap */
      Color = GC_p->FillColor;
    }
    else
    {
      /* draw to mask and then use that to apply texture */
      Err = stgfx_PrepareTexture(Handle, &MaskBitmap, &MaskDataBH,
                                 &BlitContext, &Color);
      if (Err != ST_NO_ERROR)
      {
        /* free blocks and return */
        iMax = (GC_p->LineWidth==1) ? 1 : 5;
        for(i=0; i<iMax; i++)
        {
          stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
        }
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
        if(SectorMode == STGFX_RADIUS_MODE)
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
        if(GC_p->LineWidth>1)
        {
          iMax = (SectorMode == STGFX_RADIUS_MODE) ? 3 : 2;
          for(i=0; i<iMax;  i++)
            stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[i]);
        }
        stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
        stgfx_AVMEM_free(Handle, &WorkBufferBH);
            /* safe to free directly as not yet passed to STBLIT */
        return(Err);
      }

      Bitmap_p = &MaskBitmap;
    }
  
    CheckXYL(FillXYL, FillXYLNum, "STGFX_DrawSector FillXYL");
    BlitContext.UserTag_p                 = (void*)FillXYLBlockHandle;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                FillXYLBlockHandle));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, FillXYL,
                              FillXYLNum, &Color, &BlitContext);
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
  
    /* if fill tiling is on */
    if(GC_p->FillTexture_p != NULL)
    {
#if 0 /* old code used restricted span */
      S32 MaxSpan;
      S32 xMin, xMax, yMin, yMax;

      MaxSpan = HRadius>VRadius ? HRadius : VRadius;
      MaxSpan = FRACT_TO_UINT(MaxSpan);
      xMin = FRACT_TO_INT(XC) - MaxSpan - GC_p->LineWidth;
      if(xMin<=0)
          xMin = 0;
      yMin = FRACT_TO_INT(YC) - MaxSpan - GC_p->LineWidth;
      if(yMin<=0)
          yMin = 0;
      xMax = FRACT_TO_INT(XC) + MaxSpan + GC_p->LineWidth;
      if(xMax>=TargetBitmap_p->Width)
          xMax = TargetBitmap_p->Width - 1;
      yMax = FRACT_TO_INT(YC) + MaxSpan + GC_p->LineWidth;
      if(yMax>=TargetBitmap_p->Height)
          yMax = TargetBitmap_p->Height - 1;
#endif /* extract from old fill tiling code */

      Bitmap_p = TargetBitmap_p; /* go back to writing to real target */

      /* ApplyTexture takes care of freeing MaskDataBH */
      Err = stgfx_ApplyTexture(Handle, 0, 0, GC_p, GC_p->FillTexture_p,
                               &MaskBitmap, &MaskDataBH, Bitmap_p,
                               &BlitContext);
      MaskDataBH = STAVMEM_INVALID_BLOCK_HANDLE;
      if(Err != ST_NO_ERROR)
      {
        LastErr = Err;
      }
    }
  } /* end filling */
  
  /* remove overlaps if needed */
  if(GC_p->AluMode != STBLIT_ALU_COPY)
  {
    if(GC_p->LineWidth==1) /* just one arc array, no joins */
    {
      if(SectorMode == STGFX_RADIUS_MODE) /* two straight arrays */
      {
        stgfx_RemoveXYLOverlaps(ArcXYL[0], &ArcXYLNum[0],
                                StraightXYL[1], &StraightXYLNum[1]);
        stgfx_RemoveXYLOverlaps(StraightXYL[0], &StraightXYLNum[0],
                                StraightXYL[1], &StraightXYLNum[1]);
      }
      stgfx_RemoveXYLOverlaps(ArcXYL[0], &ArcXYLNum[0],
                              StraightXYL[0], &StraightXYLNum[0]);
    }
    else /* LineWidth > 1, so 5 arc arrays, plus joins */
    {
      if(SectorMode == STGFX_RADIUS_MODE) /* joins[0] and [2] will be drawn */
      {
        for(i=0; i<5; i++)
        {
          stgfx_RemoveXYLOverlaps(JointsXYL[0], &JointsXYLNum[0],
                                  ArcXYL[i], &ArcXYLNum[i]);
        }
        stgfx_RemoveXYLOverlaps(JointsXYL[0], &JointsXYLNum[0],
                                StraightXYL[0], &StraightXYLNum[0]);
        stgfx_RemoveXYLOverlaps(JointsXYL[0], &JointsXYLNum[0],
                                StraightXYL[1], &StraightXYLNum[1]);
        stgfx_RemoveXYLOverlaps(StraightXYL[0], &StraightXYLNum[0],
                                StraightXYL[1], &StraightXYLNum[1]);
        
        
        if(GC_p->LineStyle_p)
        {
          for(i=0; i<5; i++)
          {
            stgfx_RemoveXYLOverlaps(JointsXYL[1], &JointsXYLNum[1],
                                    ArcXYL[i], &ArcXYLNum[i]);
          }
          stgfx_RemoveXYLOverlaps(JointsXYL[1], &JointsXYLNum[1],
                                  StraightXYL[0], &StraightXYLNum[0]);
          stgfx_RemoveXYLOverlaps(JointsXYL[1], &JointsXYLNum[1],
                                  StraightXYL[1], &StraightXYLNum[1]);
        }
        
        for(i=0; i<5; i++)
        {
          stgfx_RemoveXYLOverlaps(JointsXYL[2], &JointsXYLNum[2],
                                  ArcXYL[i], &ArcXYLNum[i]);
        }
        stgfx_RemoveXYLOverlaps(JointsXYL[2], &JointsXYLNum[2],
                                StraightXYL[0], &StraightXYLNum[0]);
        stgfx_RemoveXYLOverlaps(JointsXYL[2], &JointsXYLNum[2],
                                StraightXYL[1], &StraightXYLNum[1]);
        for(i=0; i<5; i++)
        {
          stgfx_RemoveXYLOverlaps(StraightXYL[0], &StraightXYLNum[0],
                                  ArcXYL[i], &ArcXYLNum[i]);
          stgfx_RemoveXYLOverlaps(StraightXYL[1], &StraightXYLNum[1],
                                  ArcXYL[i], &ArcXYLNum[i]);
        }
        for(i=0; i<5; i++)
        {
          for(j=i+1; j<5; j++)
          {
            stgfx_RemoveXYLOverlaps(ArcXYL[i], &ArcXYLNum[i],
                                    ArcXYL[j], &ArcXYLNum[j]);
          }
        }
      }
      else /* CHORD mode - joins [0] and [1] will be drawn */
      {
        for(i=0; i<5; i++)
        {
          stgfx_RemoveXYLOverlaps(JointsXYL[0], &JointsXYLNum[0],
                                  ArcXYL[i], &ArcXYLNum[i]);
        }
        stgfx_RemoveXYLOverlaps(JointsXYL[0], &JointsXYLNum[0],
                                StraightXYL[0], &StraightXYLNum[0]);
        
        stgfx_RemoveXYLOverlaps(JointsXYL[1], &JointsXYLNum[1],
                                StraightXYL[0], &StraightXYLNum[0]);
        for(i=0; i<5; i++)
        {
          stgfx_RemoveXYLOverlaps(JointsXYL[1], &JointsXYLNum[1],
                                  ArcXYL[i], &ArcXYLNum[i]);

          stgfx_RemoveXYLOverlaps(StraightXYL[0], &StraightXYLNum[0],
                                  ArcXYL[i], &ArcXYLNum[i]);

          for(j=i+1; j<5; j++)
          {
            stgfx_RemoveXYLOverlaps(ArcXYL[i], &ArcXYLNum[i],
                                    ArcXYL[j], &ArcXYLNum[j]);
          }
        }
      } /* end of CHORD mode */
    } /* end of LineWidth > 1 */
    
    DonePause("RemoveXYLOverlaps calls");
  }

  /* preparation for contour drawing */
  if(GC_p->DrawTexture_p == NULL)
  {
    /* direct draw to target bitmap */
    Color = GC_p->DrawColor;
  }
  else
  {
    /* draw to mask and then use that to apply texture */
    Err = stgfx_PrepareTexture(Handle, &MaskBitmap, &MaskDataBH,
                               &BlitContext, &Color);
    if (Err != ST_NO_ERROR)
    {
      /* free blocks and return */
      iMax = (GC_p->LineWidth==1) ? 1 : 5;
      for(i=0; i<iMax; i++)
      {
        stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
      if(SectorMode == STGFX_RADIUS_MODE)
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
      if(GC_p->LineWidth>1)
      {
        iMax = (SectorMode == STGFX_RADIUS_MODE) ? 3 : 2;
        for(i=0; i<iMax;  i++)
          stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[i]);
      }
      /* FillXYLBlockHandle - already gone to STBLIT */
      stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
          /* need to schedule as already in use by STBLIT */
      return(Err);
    }

    Bitmap_p = &MaskBitmap;
  }

  /* straight(s) drawing */
  if(SectorMode == STGFX_RADIUS_MODE) /* draw straight 1 if present */
  {
    CheckXYL(StraightXYL[1], StraightXYLNum[1], "STGFX_DrawSector StraightXYL[1]");
    BlitContext.UserTag_p                 = (void*)StraightXYLBlockHandle[1];
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                StraightXYLBlockHandle[1]));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, StraightXYL[1],
                              StraightXYLNum[1], &Color, &BlitContext);
    DonePause("DrawXYLArray(StraightXYLNum[1])");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[1]);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }  
  }
  
  /* draw straight 0 */
  CheckXYL(StraightXYL[0], StraightXYLNum[0], "STGFX_DrawSector StraightXYL[0]");
  BlitContext.UserTag_p                 = (void*)StraightXYLBlockHandle[0];
  STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                StraightXYLBlockHandle[0]));
  Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, StraightXYL[0],
                            StraightXYLNum[0], &Color, &BlitContext);
  DonePause("DrawXYLArray(StraightXYLNum[0])");
  if(Err != ST_NO_ERROR)
  {
    stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[0]);
    LastErr = STGFX_ERROR_STBLIT_BLIT;
  }
  
  /* arc(s) drawing */
  iMax = (GC_p->LineWidth==1) ? 1 : 5;
  for(i=0; i<iMax; i++)
  {
    CheckXYL(ArcXYL[i], ArcXYLNum[i], "STGFX_DrawSector ArcXYL[i]");
    BlitContext.UserTag_p                 = (void*)ArcXYLBlockHandle[i];
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                  ArcXYLBlockHandle[i]));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, ArcXYL[i],
                              ArcXYLNum[i], &Color, &BlitContext);
    DonePause("DrawXYLArray(ArcXYLNum[i])");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &ArcXYLBlockHandle[i]);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }    
  }
  
  /* joints drawing */
  if(GC_p->LineWidth>1)
  {
    CheckXYL(JointsXYL[0], JointsXYLNum[0], "STGFX_DrawSector JointsXYL[0]");
    BlitContext.UserTag_p                 = (void*)JointsXYLBlockHandle[0];
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                  JointsXYLBlockHandle[0]));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, JointsXYL[0],
                              JointsXYLNum[0], &Color, &BlitContext);
    DonePause("DrawXYLArray(JointsXYLNum[0])");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[0]);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    
    i = (SectorMode==STGFX_RADIUS_MODE) ? 2 : 1;
    CheckXYL(JointsXYL[i], JointsXYLNum[i], "STGFX_DrawSector JointsXYL[i]");
    BlitContext.UserTag_p                 = (void*)JointsXYLBlockHandle[i];
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                  JointsXYLBlockHandle[i]));    
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, JointsXYL[i],
                              JointsXYLNum[i], &Color, &BlitContext);
    DonePause("DrawXYLArray(JointsXYLNum[1/2])");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[i]);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    
    if(GC_p->LineWidth>1 && SectorMode==STGFX_RADIUS_MODE)
    {
      if(GC_p->LineStyle_p!=NULL)
      {
        CheckXYL(JointsXYL[1], JointsXYLNum[1], "STGFX_DrawSector JointsXYL[1]");
        BlitContext.UserTag_p                 = (void*)JointsXYLBlockHandle[1];
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                      JointsXYLBlockHandle[1]));    
        Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, JointsXYL[1],
                                  JointsXYLNum[1], &Color, &BlitContext);
        DonePause("DrawXYLArray(JointsXYLNum[1])");
        if(Err != ST_NO_ERROR)
        {
          stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[1]);
          LastErr = STGFX_ERROR_STBLIT_BLIT;
        }
      }
      else
      {
        /* joints were merged with sides above */
        stgfx_AVMEM_free(Handle, &JointsXYLBlockHandle[1]);
      }
    }
  }

  /* draw tiling */
  if(GC_p->DrawTexture_p != NULL)
  {
#if 0 /* old code used restricted span */
    S32 MaxSpan;
    S32 xMin, xMax, yMin, yMax;

    MaxSpan = HRadius>VRadius ? HRadius : VRadius;
    MaxSpan = FRACT_TO_UINT(MaxSpan);
    xMin = FRACT_TO_INT(XC) - MaxSpan - GC_p->LineWidth;
    if(xMin<=0)
        xMin = 0;
    yMin = FRACT_TO_INT(YC) - MaxSpan - GC_p->LineWidth;
    if(yMin<=0)
        yMin = 0;
    xMax = FRACT_TO_INT(XC) + MaxSpan + GC_p->LineWidth;
    if(xMax>=TargetBitmap_p->Width)
        xMax = TargetBitmap_p->Width - 1;
    yMax = FRACT_TO_INT(YC) + MaxSpan + GC_p->LineWidth;
    if(yMax>=TargetBitmap_p->Height)
        yMax = TargetBitmap_p->Height - 1;
#endif /* extract from old draw tiling code */

    Bitmap_p = TargetBitmap_p; /* go back to writing to real target */

    /* ApplyTexture takes care of freeing MaskDataBH */
    Err = stgfx_ApplyTexture(Handle, 0, 0, GC_p, GC_p->DrawTexture_p,
                             &MaskBitmap, &MaskDataBH, Bitmap_p, &BlitContext);
    MaskDataBH = STAVMEM_INVALID_BLOCK_HANDLE;
    if(Err != ST_NO_ERROR)
    {
      LastErr = Err;
    }
  }
  
  if(WorkBufferBH != STAVMEM_INVALID_BLOCK_HANDLE)
  {
    stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
  }
  
  return (LastErr);
} /* end of STGFX_DrawSector() */


static int stgfx_CompXYL(const void* a, const void* b)
{
  if(((STBLIT_XYL_t*)a)->PositionY > ((STBLIT_XYL_t*)b)->PositionY)
    return(1);
  if(((STBLIT_XYL_t*)a)->PositionY < ((STBLIT_XYL_t*)b)->PositionY)
    return(-1);
  return(0);
}

/* End of gfx_sector.c */
