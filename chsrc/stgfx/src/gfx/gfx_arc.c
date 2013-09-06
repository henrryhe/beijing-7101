/*******************************************************************************
File name   : gfx_ellipse1.c

Description : gfx ellipse and arc source file

COPYRIGHT (C) STMicroelectronics 2000.
*******************************************************************************/

/*#define STTBX_PRINT*/

/*#define DONEPAUSE*/
#ifdef DONEPAUSE
#define STTBX_PRINT
#define DonePause(DescrStr) \
  if(GC_p->LineWidth > 1 && GC_p->LineStyle_p != NULL) \
  { \
    STTBX_Print((DescrStr " done. Press a key...\n")); \
    getchar(); \
  }
/*#define DonePause(DescrStr) \
  STTBX_Print((DescrStr " done. Press a key...\n")); getchar()*/
#else
#define DonePause(DescrStr)
#endif
     
/* Includes ----------------------------------------------------------------- */
#include "sttbx.h"
#include "gfx_tools.h" /* includes stgfx_init.h, ... */
#include <stdio.h>
#include <string.h>

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/*#define PARAM 4*/
/*#define BIG_NUMBER PARAM*MAX_SCREEN_HEIGTH*/
#define PI         11520 /* 180 degrees * 64: PI in GFX units */
#define PI_FLOAT   (3.14159265359)
/*#define INT_ACCURACY 64*/

/* Private Variables -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/*#define FRACT_TO_UINT1(a, shift)  (((a) +(1<<((shift)-1))) >> (shift))*/
/*#define FLOAT_TO_INT1(a) (((a) >= 0 ) ? (S32)((a) + (float)0.5) : (S32)((a)-(float)0.5))*/
/*#define INCREMENT(a) (((a) >= 0 ) ? ((a)+1) : ((a)-1))*/
/*#define DECREMENT(a) (((a) < 0 ) ? ((a)+1) : ((a)-1))*/

/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t stgfx_2dRasterThickEllipse(  
   U32 BHeight, S32 XC, S32 YC, U32 HRadius, U32 VRadius,
   S32 Orientation, BOOL Fill, U16 LineWidth,
   U32* SizeInner, STBLIT_XYL_t*  InnerArray_p,
   U32* SizeOuter, STBLIT_XYL_t*  OuterArray_p,
   U32* SizeThicknessArray, STBLIT_XYL_t* ThicknessArray_p,
   U32* SizeFillArray, STBLIT_XYL_t* FillArray_p );

/*ST_ErrorCode_t stgfx_2dRasterDashedEllipse( 
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
  STBLIT_XYL_t*  EELineArray_p ); */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : STGFX_DrawEllipse()
Description : Draws an ellipse 
Parameters  : Handle, 
              Bitmap_p,
              GC_p,
              XC,             Center X coordinate
              YC,             Center Y coordinate
              HRadius,        horizontal semi axis length
              VRadius,        vertical semi axis length 
              Orientation     angle between the 'horizontal' axis and the
                              horizontal x axis 
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_INVALID_HANDLE
              ST_ERROR_BAD_PARAMETER
              STGFX_ERROR_INVALID_GC
              STGFX_ERROR_XYL_POOL_EXCEEDED
*******************************************************************************/
ST_ErrorCode_t STGFX_DrawEllipse(
  STGFX_Handle_t          Handle,
  STGXOBJ_Bitmap_t*       TargetBitmap_p,
  const STGFX_GC_t*       GC_p,
  S32                     XC,
  S32                     YC,
  U32                     HRadius,
  U32                     VRadius,
  S32                     Orientation
)
{
  /* LastErr stores the most recent non-fatal error */
  ST_ErrorCode_t         Err, LastErr = ST_NO_ERROR;
  STBLIT_BlitContext_t   BlitContext;
  STBLIT_Handle_t        BlitHandle;
  STAVMEM_BlockHandle_t  InnerEllipseBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  STAVMEM_BlockHandle_t  OuterEllipseBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  STAVMEM_BlockHandle_t  ThicknessBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  STAVMEM_BlockHandle_t  FillingBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  STAVMEM_BlockHandle_t  StartLineBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  STAVMEM_BlockHandle_t  EndLineBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  U32                    LRadius, SRadius;
  S8                     Accuracy;
  U8                     Acc;
  STBLIT_XYL_t*          InnerXYL_p;
  U32                    SizeInner; /*SizeInner = PARAM*Bitmap_p->Height;*/
  STBLIT_XYL_t*          OuterXYL_p;
  U32                    SizeOuter;
  STBLIT_XYL_t*          FillArray;
  U32                    SizeFillArray;
  STBLIT_XYL_t*          ThicknessArray;
  U32                    SizeThicknessArray;
  STBLIT_XYL_t*          SELineArray;
  STBLIT_XYL_t*          EELineArray;
  U32                    SizeSELineArray;
  U32                    SizeEELineArray;
  S32                    Points[8];
  S32                    GeometricPoints[8]; /*geometric arc endpoints*/
  U32                    i;

  /* tiling items */
  STGXOBJ_Color_t        Color;
  STGXOBJ_Bitmap_t       MaskBitmap;
  STGXOBJ_Bitmap_t*      Bitmap_p = TargetBitmap_p;
  STAVMEM_BlockHandle_t  MaskDataBH = STAVMEM_INVALID_BLOCK_HANDLE;
  STAVMEM_BlockHandle_t  WorkBufferBH = STAVMEM_INVALID_BLOCK_HANDLE;


  /* New memory allocation policy:
   *  initialise all block handles to STAVMEM_INVALID_BLOCK_HANDLE
   *  if an error occurs after the first potential allocation,
   *    goto quit, which frees all block handles
   *  when a handle has been passed for asynchronous free, return the
   *    local copy to STAVMEM_INVALID_BLOCK_HANDLE - we mustn't free
   *    or read it from that point on
   *  the exceptions are the mask bitmap data allocations, which are
   *    used more than once, and always schedule-freed at the end
   */

  DonePause("STGFX_DrawEllipse entry");
  TIME_HERE("STGFX_DrawEllipse entry");
  
  /* input parameters checking */
  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  BlitHandle = stgfx_GetBlitterHandle(Handle);
  
  /* identify Large and Small radii (before aspect ratio?) */
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

  /* check parameters and, in case, draw degenerate ellipses  */
  if(LRadius==0 && SRadius==0)
  {
    return(STGFX_DrawDot(Handle, Bitmap_p, GC_p, XC, YC));
  }
  /* too thick arcs not allowed */
  if( (GC_p->LineWidth-1)/2 >= SRadius )
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  if(SRadius==0)
  {
    return(STGFX_DrawLine( Handle, Bitmap_p, GC_p, 
           (S32) (XC-(S32)((double)LRadius*cos(-TO_RAD(Orientation))) ), 
           (S32) (YC-(S32)((double)LRadius*sin(-TO_RAD(Orientation))) ), 
           (S32) (XC+(S32)((double)LRadius*cos(-TO_RAD(Orientation))) ), 
           (S32) (YC+(S32)((double)LRadius*sin(-TO_RAD(Orientation))) ) ));
  }
  if(GC_p->LineWidth == 0 || GC_p->LineWidth > STGFX_MAX_LINE_WIDTH)
  {
    return(STGFX_ERROR_INVALID_GC);
  }

  /* copy blit params to blitting context */
  Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
  if (Err != ST_NO_ERROR)
  {
    return(Err);
  }
  
  /* optimised solid ellipse (DrawColor == FillColor). LineWidth > 1
    would just need radii adjustments, but isn't required for now.
    Texturing needs separate outline and fill */
  if((GC_p->LineWidth == 1) && (GC_p->LineStyle_p == NULL) &&
     (GC_p->EnableFilling) && (Orientation % (90*64) == 0) &&
     (GC_p->DrawTexture_p == NULL) && (GC_p->FillTexture_p == NULL) &&
     (GC_p->FillColor.Type == GC_p->DrawColor.Type) &&
     stgfx_AreColorsEqual(&(GC_p->FillColor), &(GC_p->DrawColor)))
  {
    STBLIT_XYL_t* XYL_p;
    S32 asq, bsq, c;
    S32 x, y;
    S32 d;

    /* dispose of Orientation */
    if(Orientation % (180*64) == 0)
    {
      HRadius = LRadius;
      VRadius = SRadius;
    }
    else
    {
      HRadius = SRadius;
      VRadius = LRadius;
    }
    
    if(GC_p->XAspectRatio != GC_p->YAspectRatio)
    {
      /* I haven't worked out in detail what goes on below, but for polygons
        we apply all the change to the x component and divide before multiply */
        
      XC = (XC/GC_p->XAspectRatio)*GC_p->YAspectRatio;
      HRadius = (HRadius/GC_p->XAspectRatio)*GC_p->YAspectRatio;
    }
    
    /* discard fractional parts now; x and y always move by integer steps */
    Acc = stgfx_GetUserAccuracy(Handle);
    if(Acc > 0)
    {
      XC >>= Acc;
      YC >>= Acc;
      HRadius >>= Acc;
      VRadius >>= Acc;
    }

    asq = HRadius*HRadius;
    bsq = VRadius*VRadius;

    x = 0;
    y = VRadius;

    /* d = (bsq - (VR*asq) + (asq/4)), but round up on the division because all we
      care about are zero-crossings, and all other contributions are integer */
    d = bsq - (VRadius*asq) + (asq+3)/4;

    SizeFillArray = 2*VRadius + 2;
    Err = stgfx_AVMEM_malloc(Handle, SizeFillArray*sizeof(STBLIT_XYL_t),
                             &FillingBlockHandle, (void**) &FillArray);
    if(Err != ST_NO_ERROR)
    {
      return Err;
    }
    
    TIME_HERE("STGFX_DrawEllipse solid case setup done");

    XYL_p = FillArray;
    
    /* first loop chooses between E and NE */
    /* loop condition is asq*(y - 0.5) > bsq*(x+1), which we transform to: */
    c = (asq+1)/2 + bsq;
    while((asq*y - bsq*x > c) && (x < HRadius))
    {
      if(d<0) /* midpoint is internal, new point is E */
      {
        d += bsq*(2*x + 3);
      }
      else /* midpoint is external, new point is NE */
      {
        /* write segments for previous y now we know their extent */
        XYL_p->PositionX = XC - x;
        XYL_p->PositionY = YC + y;
        XYL_p->Length = 2*x + 1;
        ++XYL_p;

        XYL_p->PositionX = XC - x;
        XYL_p->PositionY = YC - y;
        XYL_p->Length = 2*x + 1;
        ++XYL_p;
      
        /* update to new position */
        d += bsq*(2*x + 3) + 2*asq*(1-y);
        y -= 1;
      }

      x += 1;
    }

    /* (bsq*(x+0.5)^2) + (asq*(y-1)^2) - asq*bsq;
      Original code had 0.25 rather than 0.5 as in Feller */
    d = (bsq*(x*x + x) - asq*bsq + (bsq+3)/4) + (asq*(y-1)*(y-1));

    /* second loop chooses between N and NE */
    while(y > 0)
    {
      /* write previous segment */
      XYL_p->PositionX = XC - x;
      XYL_p->PositionY = YC + y;
      XYL_p->Length = 2*x + 1;
      ++XYL_p;

      XYL_p->PositionX = XC - x;
      XYL_p->PositionY = YC - y;
      XYL_p->Length = 2*x + 1;
      ++XYL_p;
    
      if(d<0) /* midpoint is internal, new point is NE */
      {
        d += (2*bsq*(x+1) + asq*(3-2*y));
        x += 1;
      }
      else /* midpoint is external, new point is N */
      {
        d += (asq*(3 - 2*y));
      }
      
      y -= 1;
    }
    
    /* write final segment at y = 0 */
    XYL_p->PositionX = XC - x;
    XYL_p->PositionY = YC; /* - y=0 */
    XYL_p->Length = 2*x + 1;
    ++XYL_p;

    if(XYL_p - FillArray > SizeFillArray)
    {
      return(STGFX_ERROR_XYL_POOL_EXCEEDED);
    }
    SizeFillArray = XYL_p - FillArray;

    TIME_HERE("STGFX_DrawEllipse solid case rastering done");
    
    BlitContext.UserTag_p = (void*)FillingBlockHandle;
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                              FillArray, SizeFillArray,
                              (STGXOBJ_Color_t*) &(GC_p->FillColor),
                              &BlitContext);
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &FillingBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    
    TIME_SYNC(Handle);
    TIME_HERE("STGFX_DrawEllipse solid case drawing done");
    return LastErr;
  }

  /* convert argument accuracy to internal accuracy */
  Accuracy = STGFX_INTERNAL_ACCURACY - stgfx_GetUserAccuracy(Handle);
  if(Accuracy>0)
  {
    Acc = Accuracy;
    XC <<= Accuracy;
    YC <<= Accuracy;
    LRadius <<= Accuracy;
    SRadius <<= Accuracy;
  }
  else if(Accuracy < 0)
  {
    Acc = -Accuracy;
    XC = XC / (1 << Acc);
    YC = YC / (1 << Acc);
    LRadius >>= Acc;
    SRadius >>= Acc;
  }
  
  /* here must be checked the Aspect Ratio */
  if(GC_p->XAspectRatio != GC_p->YAspectRatio)
  {
    double K; /* aspect ratio correction factor */
    double K2; /* aspect ratio correction factor, squared */ 
    double FloatOrientation; /* orientation converted to double prec. */

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
  }

  /* calculate XYL arrays size */
  SizeInner = 8*FRACT_TO_UINT(LRadius)+2;
  SizeOuter = SizeInner + 2 * GC_p->LineWidth;
  SizeFillArray = SizeInner;
  SizeThicknessArray = 4*SizeOuter;
  if(GC_p->LineStyle_p == NULL)
  {
    SizeSELineArray = GC_p->LineWidth * 2 + 2;
    SizeEELineArray = GC_p->LineWidth * 2 + 2;
  }
  else
  {
    SizeSELineArray = 4*SizeOuter;
    SizeEELineArray = 4*SizeOuter;
  }
  
  DonePause("STGFX_DrawEllipse startup");
  
  /* first tiling analysis */
  if((GC_p->DrawTexture_p != NULL) || (GC_p->FillTexture_p != NULL))
  {
    Err = stgfx_InitTexture(Handle, GC_p, TargetBitmap_p, &MaskBitmap,
                            &BlitContext.WorkBuffer_p, &WorkBufferBH);
    DonePause("STGFX_DrawEllipse: InitTexture()");
    if (Err != ST_NO_ERROR)
    {
      return(Err); /* first allocation */
    }
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
    DonePause("STGFX_DrawEllipse: PrepareTexture(Draw)");
    if (Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &WorkBufferBH);
          /* safe to free directly as not yet passed to STBLIT */
      return(Err);
    }

    Bitmap_p = &MaskBitmap;
  }

  /* allocates 2 xyl arrays */
  Err = stgfx_AVMEM_malloc(Handle, SizeInner*sizeof(STBLIT_XYL_t),
                           &InnerEllipseBlockHandle,
                           (void**) &InnerXYL_p);
  if(Err != ST_NO_ERROR) goto quit;
  
  if( (GC_p->EnableFilling) && (GC_p->LineStyle_p == NULL) )
  {
    Err = stgfx_AVMEM_malloc(Handle, SizeFillArray*sizeof(STBLIT_XYL_t),
                             &FillingBlockHandle,
                             (void**) &FillArray);
    if(Err != ST_NO_ERROR) goto quit;
  }
  
  
  /* There are 4 cases:
   * 1) continuous, thick=1
   * 2) dashed, thick=1
   * 3) continuous thick>1
   * 4) dashed thick>1
   * Filling is performed afterwards, only for continuous ellipses */
   
  if(GC_p->LineWidth == 1)
  {
    TIME_HERE("STGFX_DrawEllipse allocations done");
  
    if(GC_p->LineStyle_p == NULL)
    {
      /* Case 1): continuous line with thickness = 1 */
      Err = stgfx_2dRasterSimpleEllipse(Bitmap_p->Height, XC, YC, 
                                        LRadius, SRadius, 
                                        Orientation, GC_p->EnableFilling, 
                                        &SizeInner, InnerXYL_p,
                                        &SizeFillArray, FillArray);
      if(Err != ST_NO_ERROR) goto quit;
    }
    else 
    { 
      /* Case 2): dashed ellipse with thickness = 1 */
      Err = stgfx_2dRasterDashedArc(Bitmap_p->Height, XC, YC, 
                                    LRadius, SRadius, 
                                    0, 64*360, 
                                    Orientation,
                                    GC_p->LineStyle_p,
                                    GC_p->LineWidth,
                                    &SizeInner, InnerXYL_p,
                                    NULL, NULL,
                                    NULL, NULL,
                                    NULL, NULL,
                                    NULL, NULL,
                                    NULL, NULL);
      if(Err != ST_NO_ERROR) goto quit;
    }
    
    TIME_HERE("STGFX_DrawEllipse rastering done");
    
#if 0 /* temporary debug of extra seg & halt on GX1 */
    for(i=0; i < SizeInner; ++i)
    {
        if(InnerXYL_p[i].Length == 0)
            printf("InnerXYL_p[%i] = 0\n", i);
        if(InnerXYL_p[i].Length > 50)
            printf("InnerXYL_p[%i] = %u\n", i, InnerXYL_p[i].Length);
    }
#endif

    /* draw XYL array in cases 1) and 2) */
    BlitContext.UserTag_p                 = (void*)InnerEllipseBlockHandle;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                 InnerEllipseBlockHandle));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                              InnerXYL_p, SizeInner,
                              &Color, &BlitContext);
    DonePause("STGFX_DrawEllipse: DrawXYLArray(InnerXYL_p)");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &InnerEllipseBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    
    /* either way we've now lost ownership of InnerEllipseBlockHandle: */
    InnerEllipseBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  }
  else  /* line width > 1 */
  {
    /* Cases 3) and 4) require extra XYL arrays */
    Err = stgfx_AVMEM_malloc(Handle, SizeOuter*sizeof(STBLIT_XYL_t),
                             &OuterEllipseBlockHandle,
                             (void**) &OuterXYL_p);
    if(Err != ST_NO_ERROR) goto quit;
    
    Err = stgfx_AVMEM_malloc(Handle, 
                             SizeThicknessArray*sizeof(STBLIT_XYL_t),
                             &ThicknessBlockHandle,
                             (void**) &ThicknessArray);
    if(Err != ST_NO_ERROR) goto quit;

    if(GC_p->LineStyle_p == NULL)
    {
      TIME_HERE("STGFX_DrawEllipse allocations done");

      /* Case 3): continuous with thickness > 1  */
      Err = stgfx_2dRasterThickEllipse(Bitmap_p->Height, XC, YC, 
                                       LRadius, SRadius, Orientation,  
                                       GC_p->EnableFilling, GC_p->LineWidth,
                                       &SizeInner,          InnerXYL_p,
                                       &SizeOuter,          OuterXYL_p,
                                       &SizeThicknessArray, ThicknessArray,
                                       &SizeFillArray,      FillArray);
      if(Err != ST_NO_ERROR) goto quit;
      
      TIME_HERE("STGFX_DrawEllipse rastering done");
    }
    else
    {
      /* Case 4): dashed with thickness>1  */
      i = 0;
      for(i = 0; (GC_p->LineStyle_p[i] != 0); ++i)
      {
        if(GC_p->LineStyle_p[i] == 1)
        {
          Err = STGFX_ERROR_INVALID_GC;
          goto quit;
        }
      }
      
      Err = stgfx_AVMEM_malloc(Handle, 
                               SizeSELineArray*sizeof(STBLIT_XYL_t),
                               &StartLineBlockHandle,
                               (void**) &SELineArray);
      if(Err != ST_NO_ERROR) goto quit;
      
      Err = stgfx_AVMEM_malloc(Handle, 
                               SizeEELineArray*sizeof(STBLIT_XYL_t), 
                               &EndLineBlockHandle,
                               (void**) &EELineArray);
      if(Err != ST_NO_ERROR) goto quit;

      TIME_HERE("STGFX_DrawEllipse allocations done");

      Err = stgfx_2dRasterDashedArc(Bitmap_p->Height, XC, YC, LRadius, SRadius,
                                    0, 64*360, Orientation,
                                    GC_p->LineStyle_p, GC_p->LineWidth,
                                    &SizeInner,          InnerXYL_p,
                                    &SizeOuter,          OuterXYL_p,
                                    &SizeThicknessArray, ThicknessArray,
                                    &SizeSELineArray,    SELineArray,
                                    &SizeEELineArray,    EELineArray,
                                    Points,              GeometricPoints);
      if(Err != ST_NO_ERROR) goto quit;

      TIME_HERE("STGFX_DrawEllipse rastering done");
      
      /* Draw and free starting segments array */
      BlitContext.UserTag_p                 = (void*)StartLineBlockHandle;
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                  StartLineBlockHandle));
      Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                                SELineArray, SizeSELineArray,
                                &Color, &BlitContext);
      DonePause("STGFX_DrawEllipse: DrawXYLArray(SELineArray)");
      if(Err != ST_NO_ERROR)
      {
        stgfx_AVMEM_free(Handle, &StartLineBlockHandle);
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }
      StartLineBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;

      /* Draw and free ending segments array */
      BlitContext.UserTag_p                 = (void*)EndLineBlockHandle;
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                  EndLineBlockHandle));
      Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                                EELineArray, SizeEELineArray,
                                &Color, &BlitContext);
      DonePause("STGFX_DrawEllipse: DrawXYLArray(EELineArray)");
      if(Err != ST_NO_ERROR)
      {
        stgfx_AVMEM_free(Handle, &EndLineBlockHandle);
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }
      EndLineBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
      
    } /* end of case 4) */

#if 0 /* temporary debug of halt on GX1 */
    for(i=0; i < SizeInner; ++i)
    {
        if(InnerXYL_p[i].Length == 0)
            printf("InnerXYL_p[%i] = 0\n", i);
        if(InnerXYL_p[i].Length > 50)
            printf("InnerXYL_p[%i] = %u\n", i, InnerXYL_p[i].Length);
    }

    for(i=0; i < SizeOuter; ++i)
    {
        if(OuterXYL_p[i].Length == 0)
            printf("OuterXYL_p[%i] = 0\n", i);
        if(OuterXYL_p[i].Length > 50)
            printf("OuterXYL_p[%i] = %u\n", i, OuterXYL_p[i].Length);
    }
#endif

    /* Draw the inner ellipse in both cases 3) and 4) */
    BlitContext.UserTag_p                 = (void*)InnerEllipseBlockHandle;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                InnerEllipseBlockHandle));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                              InnerXYL_p, SizeInner,
                              &Color, &BlitContext);
    DonePause("STGFX_DrawEllipse: DrawXYLArray(InnerXYL_p)");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &InnerEllipseBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    InnerEllipseBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;

    /* Draw the outer ellipse in both cases 3) and 4) */
    BlitContext.UserTag_p                 = (void*)OuterEllipseBlockHandle;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                OuterEllipseBlockHandle));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                              OuterXYL_p, SizeOuter,
                              &Color, &BlitContext);
    DonePause("STGFX_DrawEllipse: DrawXYLArray(OuterXYL_p)");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &OuterEllipseBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    OuterEllipseBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;

    /* Draw the thickness filling in both cases 3) and 4) */
    BlitContext.UserTag_p                 = (void*)ThicknessBlockHandle;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                 ThicknessBlockHandle));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                              ThicknessArray, SizeThicknessArray,
                              &Color, &BlitContext);
    DonePause("STGFX_DrawEllipse: DrawXYLArray(ThicknessArray)");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &ThicknessBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    ThicknessBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
  }  /* end of all cases  */
    
  TIME_SYNC(Handle);
  TIME_HERE("STGFX_DrawEllipse outline drawing done");

  /* tiling for the contour */
  if(GC_p->DrawTexture_p != NULL)
  {
#if 0 /* old code used restricted span */
    S32 MaxSpan;
    S32 xMin, xMax, yMin, yMax;

    /* geometry */
    MaxSpan = FRACT_TO_UINT(LRadius);
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
    DonePause("STGFX_DrawEllipse: ApplyTexture(Draw)");
    MaskDataBH = STAVMEM_INVALID_BLOCK_HANDLE;
    if(Err != ST_NO_ERROR)
    {
      LastErr = Err;
    }

    TIME_SYNC(Handle);
    TIME_HERE("STGFX_DrawEllipse outline texturing done");
  }

  /* filling */
  if(GC_p->EnableFilling && (GC_p->LineStyle_p == NULL))
  {
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
      DonePause("STGFX_DrawEllipse: PrepareTexture(Fill)");
      if (Err != ST_NO_ERROR)
      {
        stgfx_AVMEM_free(Handle, &FillingBlockHandle); /* just this one left */
        stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
            /* need to schedule as already in use by STBLIT */
        return(Err);
      }

      Bitmap_p = &MaskBitmap;
    }

    BlitContext.UserTag_p                 = (void*)FillingBlockHandle;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                 FillingBlockHandle));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                              FillArray, SizeFillArray,
                              &Color, &BlitContext);
    DonePause("STGFX_DrawEllipse: DrawXYLArray(FillArray)");
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &FillingBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    FillingBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    
    TIME_SYNC(Handle);
    TIME_HERE("STGFX_DrawEllipse fill drawing done");
    
    /* if fill tiling is on */
    if(GC_p->FillTexture_p != NULL)
    {
#if 0 /* old code used restricted span */
      S32 MaxSpan;
      S32 xMin, xMax, yMin, yMax;

      /* geometry */
      MaxSpan = FRACT_TO_UINT(LRadius);
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
      DonePause("STGFX_DrawEllipse: ApplyTexture(Fill)");
      MaskDataBH = STAVMEM_INVALID_BLOCK_HANDLE;
      if(Err != ST_NO_ERROR)
      {
        LastErr = Err;
      }
      
      TIME_SYNC(Handle);
      TIME_HERE("STGFX_DrawEllipse fill texturing done");
    }
  }

quit:
  if(Err != ST_NO_ERROR)
  {
    LastErr = Err; /* return it below */
  
    /* Free any avmem blocks we still own */
    stgfx_AVMEM_free(Handle, &InnerEllipseBlockHandle);
    stgfx_AVMEM_free(Handle, &OuterEllipseBlockHandle);
    stgfx_AVMEM_free(Handle, &ThicknessBlockHandle);
    stgfx_AVMEM_free(Handle, &FillingBlockHandle);
    stgfx_AVMEM_free(Handle, &StartLineBlockHandle);
    stgfx_AVMEM_free(Handle, &EndLineBlockHandle);
  }
  else
  {
    /* whatever was allocated should have been scheduled for freeing
      and marked clean */
      
    if( (InnerEllipseBlockHandle != STAVMEM_INVALID_BLOCK_HANDLE) ||
        (OuterEllipseBlockHandle != STAVMEM_INVALID_BLOCK_HANDLE) ||
        (ThicknessBlockHandle != STAVMEM_INVALID_BLOCK_HANDLE) ||
        (FillingBlockHandle != STAVMEM_INVALID_BLOCK_HANDLE) ||
        (StartLineBlockHandle != STAVMEM_INVALID_BLOCK_HANDLE) ||
        (EndLineBlockHandle != STAVMEM_INVALID_BLOCK_HANDLE) )
    {
      STTBX_Print (("STGFX_DrawEllipse: Non-error case didn't clear up all memory\n"));
    }
  }

  /* must schedule mask bitmap free after all uses of it */
  if(MaskDataBH != STAVMEM_INVALID_BLOCK_HANDLE)
  {
    stgfx_ScheduleAVMEM_free(Handle, &MaskDataBH);
  }

  if(WorkBufferBH != STAVMEM_INVALID_BLOCK_HANDLE)
  {
    stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
  }

  return(LastErr);
} /* end of STGFX_DrawEllipse() */


/* stgfx_2dRasterSimpleEllipse also used by STGFX_DrawDot, so in gfx_conic.c */


/****************************************************************************\
Name        : stgfx_2dRasterThickEllipse()
Description : stgfx_2dRasterThickEllipse fill in 4 array xyl:
              the first one is the outer boundary, the second one is the inner
              boundary, the third one is the thickness, and the last one is
              the filling ( if required otherwise this is empty ) 
Parameters  :     U32 BHeight, 
                  S32 XC, 
                  S32 YC, 
                  U32 HRadius, 
                  U32 VRadius,
                  S32 Orientation,
                  BOOL Fill,
                  U16 LineWidth,
                  U32* SizeInner, 
                  STBLIT_XYL_t*  InnerArray_p,
                  U32* SizeOuter, 
                  STBLIT_XYL_t*  OuterArray_p,
                  U32* SizeThicknessArray,
                  STBLIT_XYL_t* ThicknessArray_p,
                  U32* SizeFillArray,
                  STBLIT_XYL_t* FillArray_p
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_XYL_POOL_EXCEEDED
\****************************************************************************/
static ST_ErrorCode_t stgfx_2dRasterThickEllipse(  
   U32 BHeight, S32 XC, S32 YC, U32 HRadius, U32 VRadius,
   S32 Orientation, BOOL Fill, U16 LineWidth,
   U32* SizeInner, STBLIT_XYL_t*  InnerArray_p,
   U32* SizeOuter, STBLIT_XYL_t*  OuterArray_p,
   U32* SizeThicknessArray, STBLIT_XYL_t* ThicknessArray_p,
   U32* SizeFillArray, STBLIT_XYL_t* FillArray_p )
{
  ST_ErrorCode_t         Err = ST_NO_ERROR;
  U32                    LRadius;
  U32                    SRadius;
  U32                    PosMinIn;
  U32                    PosMaxIn;
  U32                    PosMinOut;
  U32                    PosMaxOut;
  U32                    TotSidesIn;
  U32                    TotSidesOut;
  U32                    OuterLRadius;
  U32                    OuterSRadius;
  U32                    j = 0;
  stgfx_Side_t           Sides[6];
  stgfx_Side_t*          Sides_p;

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

  /* calculates XYL array of outer ellipse */
  OuterLRadius = LRadius + ((LineWidth<<STGFX_INTERNAL_ACCURACY)/2);
  OuterSRadius = SRadius + ((LineWidth<<STGFX_INTERNAL_ACCURACY)/2);
  /*OuterLRadius = LRadius + ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);*/
  /*OuterSRadius = SRadius + ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);*/

  Err = stgfx_2dRasterSimpleEllipse(BHeight, XC, YC, OuterLRadius,
                                    OuterSRadius, Orientation,
                                    FALSE, SizeOuter, OuterArray_p,
                                    SizeFillArray, FillArray_p);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  
#if 0
  /* TEMPORARY DEBUG:
    look for gaps in y that could cause overlong Thickness segments */
  for(j = 0; j < *SizeOuter - 1; ++j)
  {
    if(GFXABS(OuterArray_p[j+1].PositionY - OuterArray_p[j].PositionY) != 1)
    {
      printf("Suspicious dy from stgfx_2dRasterSimpleEllipse:\n"
             "OuterArray_p[%i].PositionY = %i; OuterArray_p[%i].PositionY = %i\n",
             j, OuterArray_p[j].PositionY, j+1, OuterArray_p[j+1].PositionY);
    }
  }
#endif

  /* line width is an even or an odd number? */
  if(LineWidth & 0x1) /* line width is odd */
  {
    LRadius = LRadius - ((LineWidth<<STGFX_INTERNAL_ACCURACY)/2);
    SRadius = SRadius - ((LineWidth<<STGFX_INTERNAL_ACCURACY)/2);
    /*LRadius = LRadius - ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);*/
    /*SRadius = SRadius - ((LineWidth/2)<<STGFX_INTERNAL_ACCURACY);*/
  }
  else /* LineWidth is even */
  {
    LRadius = LRadius - ((LineWidth/2 - 1)<<STGFX_INTERNAL_ACCURACY);
    SRadius = SRadius - ((LineWidth/2 - 1)<<STGFX_INTERNAL_ACCURACY);
  }


  /* fills in the inner ellipse array xyl */
  Err = stgfx_2dRasterSimpleEllipse(BHeight, XC, YC, LRadius,
                                    SRadius, Orientation,
                                    FALSE, SizeInner, InnerArray_p,
                                    SizeFillArray, FillArray_p);
  if(Err != ST_NO_ERROR) 
  {
    return(Err);
  }

#if 0
  /* TEMPORARY DEBUG:
    look for gaps in y that could cause overlong Thickness segments */
  for(j = 0; j < *SizeInner - 1; ++j)
  {
    if(GFXABS(InnerArray_p[j+1].PositionY - InnerArray_p[j].PositionY) != 1)
    {
      printf("Suspicious dy from stgfx_2dRasterSimpleEllipse:\n"
             "InnerArray_p[%i].PositionY = %i; InnerArray_p[%i].PositionY = %i\n",
             j, InnerArray_p[j].PositionY, j+1, InnerArray_p[j+1].PositionY);
    }
  }
#endif

  /* it is the turn of the thickness array */
  if(LineWidth > 1) /* always true and Fill generation would crash if not */
  {
    SplitArrayXYL( OuterArray_p, *SizeOuter, &PosMinOut, &PosMaxOut, TRUE,
                   Sides_p, &TotSidesOut );
    if(TotSidesOut == 3)
    {
      Sides_p[2].Spin = Sides_p[0].Spin;
    }
    SplitArrayXYL( InnerArray_p, *SizeInner, &PosMinIn, &PosMaxIn, TRUE,
                   &(Sides_p[TotSidesOut]), &TotSidesIn );

    for(j = TotSidesOut; j < (TotSidesOut+TotSidesIn); j++)
    {
      Sides_p[j].Spin += 3; 
      Sides_p[j].Up = -(Sides_p[j].Up);
    }

    if(TotSidesIn == 3)
    {
      Sides_p[TotSidesOut+2].Spin = Sides_p[TotSidesOut].Spin;
    }
    
    Err = stgfx_FillGenericPolygon( BHeight, 
                                    OuterArray_p[PosMinOut].PositionY, 
                                    OuterArray_p[PosMaxOut].PositionY, 
                                    Sides_p, 
                                    (TotSidesIn + TotSidesOut),
                                    ThicknessArray_p, SizeThicknessArray,
                                    STGFX_WINDING_FILL );
    if(Err != ST_NO_ERROR) 
    {
      return(Err);
    }

#if 0
    /* TEMPORARY DEBUG: look for overlong Thickness elements */
    for(j = 0; j < *SizeThicknessArray; ++j)
    {
      if(ThicknessArray_p[j].Length > 100)
      {
        printf("stgfx_2dRasterThickEllipse: length %u element"
               " found at ThicknessArray_p[%u] for Orientation %i\n",
               ThicknessArray_p[j].Length, j, Orientation);
      }
    }
#endif
  }
  
  if(Fill)  /* if the inner ellipse must be filled */
  {
    Err = stgfx_FillGenericPolygon( BHeight, 
                                    InnerArray_p[PosMinIn].PositionY, 
                                    InnerArray_p[PosMaxIn].PositionY, 
                                    &(Sides_p[TotSidesOut]), 
                                    TotSidesIn,
                                    FillArray_p, SizeFillArray,
                                    STGFX_WINDING_FILL );
    if(Err != ST_NO_ERROR) 
    {
      return(Err);
    }
  }
  
  /* In order to call the FillGenericPolygon function
     (for the thickness and for the filling) it is needed
     to repeat the first element of the array xyl; but now we
     can ignore it, both in the inner and in the outer ellipses */
  if(OuterArray_p[*SizeOuter-1].PositionY == OuterArray_p[0].PositionY)
  {
    (*SizeOuter)--;
  }
 
  if(InnerArray_p[*SizeInner-1].PositionY == InnerArray_p[0].PositionY)
  {
    (*SizeInner)--;
  }

  return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : STGFX_DrawArc()
Description : Draws an arc
Parameters  : Bitmap_p,
              GC,
              XC,
              YC,
              HRadius,
              VRadius,
              StartAngle,
              ArcAngle,
              Orientation
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
              STGFX_ERROR_INVALID_HANDLE
              ST_ERROR_BAD_PARAMETER
              STGFX_ERROR_INVALID_GC
              STGFX_ERROR_XYL_POOL_EXCEEDED
*******************************************************************************/
ST_ErrorCode_t  STGFX_DrawArc(
  STGFX_Handle_t          Handle,
  STGXOBJ_Bitmap_t*       TargetBitmap_p,
  const STGFX_GC_t*       GC_p,
  S32                     XC,
  S32                     YC,
  U32                     HRadius,
  U32                     VRadius,
  S32                     StartAngle,
  U32                     ArcAngle,
  S32                     Orientation
)
{
  /* LastErr stores the most recent non-fatal error */
  ST_ErrorCode_t         Err, LastErr = ST_NO_ERROR;
  STBLIT_BlitContext_t   BlitContext;
  STBLIT_Handle_t        BlitHandle;
  STAVMEM_BlockHandle_t  XYLBlockHandle;
  U32                    LRadius;
  U32                    SRadius;
  U32                    Size, SizeOuter = 0, SizeThicknessArray = 0;
  U32                    SizeSELineArray = 0, SizeEELineArray = 0;
  U32                    SizeStartCap = 0, SizeEndCap = 0;
  STBLIT_XYL_t           *XYL_p, *OuterArray, *ThicknessArray, *SELineArray;
  STBLIT_XYL_t           *EELineArray, *StartCapArray, *EndCapArray;
  S32                    EndAngle;
  S8                     Accuracy;
  U8                     Acc;
  STGFX_GC_t             LocalGC;
  U32                    Radius;
  S32                    XCoord;
  S32                    YCoord;
  S32                    Tmp;
  S32                    Points[8]; /* pixels of arc endpoints */
  S32                    GeometricPoints[8]; /*geometric arc endpoints*/
  S32                    LineWidth;
  U32                    i;
  S32                    DeltaX;
  S32                    DeltaY;
  
  /* tiling items */
  STGXOBJ_Bitmap_t       MaskBitmap;
  STGXOBJ_Bitmap_t*      Bitmap_p = TargetBitmap_p;
  STAVMEM_BlockHandle_t  MaskDataBH;
  STAVMEM_BlockHandle_t  WorkBufferBH = STAVMEM_INVALID_BLOCK_HANDLE;
    /* allows us to call stgfx_AVMEM_free even if not allocated */
  
  
  TIME_HERE("STGFX_DrawArc");
  
  /* validate handle */  
  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }

  BlitHandle = stgfx_GetBlitterHandle(Handle);

  /* copy blit params to blitting context & validate GC_p, Bitmap_p */
  Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
  if (Err != ST_NO_ERROR)
  {
    return(Err);
  }
  BlitContext.NotifyBlitCompletion = FALSE;
  /* we allocate one block freed at the end */

  LocalGC = *GC_p;
  
  /* input parameters checking */  
  if( GC_p->LineWidth == 0 || GC_p->LineWidth > STGFX_MAX_LINE_WIDTH )
  {
    return(STGFX_ERROR_INVALID_GC);
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

  /* check parameters and draw degenerate arcs when required */
  if(LRadius==0 && SRadius==0) 
  {
    return(STGFX_DrawDot(Handle, Bitmap_p, GC_p, XC, YC));
  }
  if( (GC_p->LineWidth-1)/2 >= SRadius )
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  if( ArcAngle == 0 )
  {
    Accuracy = STGFX_INTERNAL_ACCURACY - stgfx_GetUserAccuracy(Handle);
   
    /* sets arguments accuracy to internal accuracy */
    if(Accuracy > 0)
    {
      Acc = Accuracy;
      LRadius <<= Accuracy;
      SRadius <<= Accuracy;
    }
    else if(Accuracy < 0)
    {
      Acc = -Accuracy;
      LRadius >>= Acc;
      SRadius >>= Acc;
    }
  
    CheckAngle(&StartAngle);
    LocalGC.CapStyle = STGFX_ROUND_CAP;
    Radius = (U32)
        ((double)LRadius*(double)LRadius*(double)SRadius*(double)SRadius /
         (((double)SRadius*(double)SRadius*
              cos(TO_RAD(StartAngle))*cos(TO_RAD(StartAngle))) +
         ((double)LRadius*(double)LRadius*
              sin(TO_RAD(StartAngle))*sin(TO_RAD(StartAngle)))));
    XCoord = (S32)(sqrt(Radius)*cos(TO_RAD(StartAngle)));
    YCoord = (S32)(sqrt(Radius)*sin(TO_RAD(StartAngle)));
    Tmp = XCoord;
    XCoord = (S32)((double)Tmp*cos(-TO_RAD(Orientation)) + 
                   (double)YCoord*sin(-TO_RAD(Orientation)));
    YCoord = (S32)((double)YCoord*cos(-TO_RAD(Orientation)) - 
                   (double)Tmp*sin(-TO_RAD(Orientation)));  
    if(Accuracy > 0)
    {
      XCoord = XCoord >> Acc;
      YCoord = YCoord >> Acc;
    }
    else if(Accuracy < 0)
    {
      XCoord = XCoord << Acc;
      YCoord = YCoord << Acc;
    }
    return(STGFX_DrawDot(Handle, Bitmap_p, &LocalGC, XC+XCoord, YC-YCoord));
  }
  if(ArcAngle >= (360*64))
  {
    LocalGC.EnableFilling = FALSE;
    return(STGFX_DrawEllipse(Handle, Bitmap_p, &LocalGC, XC, YC,
                             LRadius, SRadius, Orientation));
  }
  if(SRadius==0) /* draw a line */
  { 
    /* this line should be long as the projection of the arc ! */
    return(STGFX_DrawLine( Handle, Bitmap_p, GC_p, 
            (S32) (XC - (S32)(LRadius * (float)cos(-TO_RAD(Orientation)))), 
            (S32) (YC - (S32)(LRadius * (float)sin(-TO_RAD(Orientation)))), 
            (S32) (XC + (S32)(LRadius * (float)cos(-TO_RAD(Orientation)))), 
            (S32) (YC + (S32)(LRadius * (float)sin(-TO_RAD(Orientation)))) ));
  }
  
  Accuracy = STGFX_INTERNAL_ACCURACY - stgfx_GetUserAccuracy(Handle);
  /* sets arguments accuracy to internal accuracy */
  if(Accuracy > 0)
  {
    Acc = Accuracy;
    XC <<= Accuracy;
    YC <<= Accuracy;
    LRadius <<= Accuracy;
    SRadius  <<= Accuracy;
  }
  else if(Accuracy < 0)
  {
    Acc = -Accuracy;
    XC = XC / (1 << Acc);
    YC = YC / (1 << Acc);
    LRadius >>= Acc;
    SRadius >>= Acc;
  }
  
  /* calculate XYL array sizes (ignoring aspect ratio?) */
  Size = 4*FRACT_TO_UINT(LRadius)+2; /* four quadrants plus 2 sides @ YC */
  if(GC_p->LineWidth > 1)
  {
    SizeOuter = Size + GC_p->LineWidth; /* LineWidth/2 added to top & bottom */
    SizeThicknessArray = 4*SizeOuter; /* not rigorous */
    if(GC_p->LineStyle_p == NULL)
    {
      SizeSELineArray = SizeEELineArray = GC_p->LineWidth * 2 + 2;
    }
    else
    {
      SizeSELineArray = SizeEELineArray = 2*SizeOuter;
      /* not really needed if capped */
    }
    
    if((GC_p->CapStyle == STGFX_PROJECTING_CAP) && (GC_p->LineWidth > 1) ||
       (GC_p->CapStyle == STGFX_ROUND_CAP) && (GC_p->LineWidth > 2))
    {
      /*SizeStartCap = SizeEndCap = GC_p->LineWidth * 4;*/
      SizeStartCap = SizeEndCap = GC_p->LineWidth * INCREASE_XYL_SIZE_CAP;
    }
  }
  
  Err = stgfx_AVMEM_malloc(Handle, sizeof(STBLIT_XYL_t) *
                           (Size+SizeOuter+SizeThicknessArray+SizeSELineArray
                                +SizeEELineArray+SizeStartCap+SizeEndCap),
                           &XYLBlockHandle, (void**) &XYL_p);
  if(Err != ST_NO_ERROR)
  {
    return(Err);
  }
  
  if(GC_p->LineWidth > 1)
  {
    /* parcel-out other xyl arrays from within the single block */
    OuterArray      = XYL_p + Size;
    ThicknessArray  = OuterArray + SizeOuter;
    SELineArray     = ThicknessArray + SizeThicknessArray;
    EELineArray     = SELineArray + SizeSELineArray;
    
    /* these last two ignored for STGFX_RECTANGLE_CAP */
    StartCapArray   = EELineArray + SizeEELineArray;
    EndCapArray     = StartCapArray + SizeStartCap;
  }

  CheckAngle(&StartAngle);
  if(ArcAngle > (64*360))
  {
    ArcAngle = 64*360; /* ellipse */
  }
  if(StartAngle >= PI)
  {
     Orientation += PI;
     StartAngle -= PI;
  } 
  EndAngle = StartAngle + ArcAngle;
  
  /* here must be checked the Aspect Ratio */
  if(GC_p->XAspectRatio != GC_p->YAspectRatio)
  {
    double K; /* aspect ratio correction factor */
    double K2; /* aspect ratio correction factor, squared */ 
    double FloatOrientation; /* orientation converted to double prec. */

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
  }

  /* tiling analysis */
  if(GC_p->DrawTexture_p != NULL)
  {
      /* draw to mask and then use that to apply texture */
      Err = stgfx_InitTexture(Handle, GC_p, TargetBitmap_p, &MaskBitmap,
                              &BlitContext.WorkBuffer_p, &WorkBufferBH);

      if (Err != ST_NO_ERROR)
      {
        stgfx_AVMEM_free(Handle, &XYLBlockHandle);
        return Err;
      }

      Err = stgfx_PrepareTexture(Handle, &MaskBitmap, &MaskDataBH,
                                 &BlitContext, &(LocalGC.DrawColor));
      if (Err != ST_NO_ERROR)
      {
        /* safe to free things directly as not yet passed to STBLIT */
        stgfx_AVMEM_free(Handle, &XYLBlockHandle);
        stgfx_AVMEM_free(Handle, &WorkBufferBH);
        return Err;
      }

      Bitmap_p = &MaskBitmap;
  }

  TIME_HERE("STGFX_DrawArc setup and allocations done");
  
  /* There are 4 cases:                         */
  /* 1) continuous, thick=1                     */
  /* 2) dashed, thick=1                         */
  /* 3) continuous thick>1                      */
  /* 4) dashed thick>1                          */
  if(GC_p->LineWidth ==1)
  {
    /* cases 1) and 2) */
    if(GC_p->LineStyle_p == NULL)
    {
#ifndef ST_GX1 /* apparent compiler bug prevents CalcAlignedSector loop running on GX1 */
      if (Orientation % (90*64) == 0)
      {
        /* new optimised code: for axis-aligned cases, raster in a similar way
          to filled sectors */

        /* Turn LRadius, SRadius back to HRadius, VRadius */
        if(Orientation == (64*90)|| (Orientation == (64*270)))
        {
           GFXSWAP(LRadius, SRadius); 
        }
        
        StartAngle += Orientation;
        if(StartAngle > 360*64) StartAngle -= 360*64;
        
        /* discard fractional parts - the algorithm is integer from here on */
        XC = FRACT_TO_INT(XC);
        YC = FRACT_TO_INT(YC);
        LRadius = FRACT_TO_UINT(LRadius);
        SRadius = FRACT_TO_UINT(SRadius);
  
        Err = stgfx_CalcAlignedSector(Handle, FALSE, XC, YC,
                                      (U16)LRadius, (U16)SRadius,
                                      StartAngle, ArcAngle, XYL_p, &Size);
        if(Err != ST_NO_ERROR)
        {
          stgfx_AVMEM_free(Handle, &XYLBlockHandle);
          stgfx_AVMEM_free(Handle, &WorkBufferBH);
          return(Err);
        }
        
        TIME_HERE("STGFX_DrawArc aligned case rastering done");
        /*printf("STGFX_DrawArc aligned case will draw %u segments\n", Size);*/
      }
      else
#endif /* ndef ST_GX1 */
      {
        /* case 1): continuous arc with thickness = 1 */ 
        Err = stgfx_2dRasterSimpleArc( XC, YC, LRadius, SRadius, 
                                       StartAngle, 
                                       ArcAngle, Orientation, &Size, XYL_p,
                                       Points);
      }
    }
    else
    {
      /* case 2): dashed arc with thickness = 1 */
      Err = stgfx_2dRasterDashedArc( Bitmap_p->Height, XC, YC, 
                                     LRadius, SRadius, 
                                     StartAngle, ArcAngle, 
                                     Orientation,
                                     GC_p->LineStyle_p,
                                     GC_p->LineWidth,
                                     &Size, XYL_p,
                                     NULL, NULL,
                                     NULL, NULL, 
                                     NULL, NULL,
                                     NULL, NULL, 
                                     NULL, NULL );
    }
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &XYLBlockHandle);
      stgfx_AVMEM_free(Handle, &WorkBufferBH);
      return(Err);
    }
    
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, XYL_p, Size,
                              &(LocalGC.DrawColor), &BlitContext);
    DonePause("STGFX_DrawArc: DrawXYLArray(XYL_p)");
    if(Err != ST_NO_ERROR)
    {
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
  } 
  else
  {
    /* cases 3) and 4) : thickness > 1 */
    if( GC_p->LineStyle_p == NULL )
    {
      /* case 3): continuous line with thickness > 1 */
      Err = stgfx_2dRasterThickArc( Bitmap_p->Height, 
                                    XC, YC, LRadius, SRadius, 
                                    StartAngle, ArcAngle, Orientation, 
                                    GC_p->LineWidth,
                                    &Size,               XYL_p,
                                    &SizeOuter,          OuterArray,
                                    &SizeThicknessArray, ThicknessArray, 
                                    &SizeSELineArray,    SELineArray,
                                    &SizeEELineArray,    EELineArray,
                                    Points,              GeometricPoints );
    } 
    else /* Line Style != NULL */
    {
      /* case 4): dashed line with thickness > 1 */
      for(i=0; (GC_p->LineStyle_p[i] != 0); ++i)
      {
        if(GC_p->LineStyle_p[i] == 1)
        {
          stgfx_AVMEM_free(Handle, &XYLBlockHandle);
          stgfx_AVMEM_free(Handle, &WorkBufferBH);
          return(STGFX_ERROR_INVALID_GC);
        }
      }
      Err = stgfx_2dRasterDashedArc( Bitmap_p->Height, XC, YC, 
                                     LRadius, SRadius, 
                                     StartAngle, ArcAngle, Orientation,
                                     GC_p->LineStyle_p,    GC_p->LineWidth,
                                     &Size,                XYL_p,
                                     &SizeOuter,           OuterArray,
                                     &SizeThicknessArray,  ThicknessArray,
                                     &SizeSELineArray,     SELineArray,
                                     &SizeEELineArray,     EELineArray,
                                     Points,               GeometricPoints );
    }
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &XYLBlockHandle);
      stgfx_AVMEM_free(Handle, &WorkBufferBH);
      return(Err);
    }

    if((GC_p->CapStyle == STGFX_PROJECTING_CAP) && (GC_p->LineWidth > 1) ||
       (GC_p->CapStyle == STGFX_ROUND_CAP) && (GC_p->LineWidth > 2))
    {
      /*Distance = 4*LRadius;*/
      DeltaX = (Points[0] - Points[2]);
      DeltaY = (Points[1] - Points[3]);
      if(DeltaX >= 0)
      {
        DeltaX = DeltaX + (1<<STGFX_INTERNAL_ACCURACY);
      }
      else
      {
        DeltaX = DeltaX - (1<<STGFX_INTERNAL_ACCURACY);
      }
      if(DeltaY >= 0)
      {
        DeltaY = DeltaY + (1<<STGFX_INTERNAL_ACCURACY);
      }
      else
      {
        DeltaY = DeltaY - (1<<STGFX_INTERNAL_ACCURACY);
      }
      LineWidth = (U32)sqrt(((double)(DeltaX) * (double)(DeltaX)) +
                            ((double)(DeltaY) * (double)(DeltaY)));
      LineWidth = LineWidth>>STGFX_INTERNAL_ACCURACY;
      Err = stgfx_2dArcRasterCap( Handle, Points[0], Points[1],
                                  Points[2], Points[3], GC_p->CapStyle,
                                  LineWidth, 
                                  &SizeStartCap, StartCapArray, TRUE );
      if(Err != ST_NO_ERROR)
      {
        stgfx_AVMEM_free(Handle, &XYLBlockHandle);
        stgfx_AVMEM_free(Handle, &WorkBufferBH);
        return(Err);
      }

      if(GC_p->CapStyle == STGFX_ROUND_CAP)
      {
        GFXSWAP(Points[4], Points[6]);
        GFXSWAP(Points[5], Points[7]);
      }
      DeltaX = (Points[4] - Points[6]);
      DeltaY = (Points[5] - Points[7]);
      if(DeltaX >= 0)
      {
        DeltaX = DeltaX + (1<<STGFX_INTERNAL_ACCURACY);
      }
      else
      {
        DeltaX = DeltaX - (1<<STGFX_INTERNAL_ACCURACY);
      }
      if(DeltaY >= 0)
      {
        DeltaY = DeltaY + (1<<STGFX_INTERNAL_ACCURACY);
      }
      else
      {
        DeltaY = DeltaY - (1<<STGFX_INTERNAL_ACCURACY);
      }
      LineWidth = (U32)sqrt(((double)(DeltaX) * (double)(DeltaX)) +
                            ((double)(DeltaY) * (double)(DeltaY)));
      LineWidth = LineWidth>>STGFX_INTERNAL_ACCURACY;
      Err = stgfx_2dArcRasterCap( Handle, Points[4], Points[5],
                               Points[6], Points[7], GC_p->CapStyle,
                               LineWidth,
                               &SizeEndCap, EndCapArray, FALSE);
      if(Err != ST_NO_ERROR)
      {
        stgfx_AVMEM_free(Handle, &XYLBlockHandle);
        stgfx_AVMEM_free(Handle, &WorkBufferBH);
        return(Err);
      }

#if 0      
      DumpXYL(StartCapArray, SizeStartCap, "StartCapArray before overlap removal:");
      DumpXYL(SELineArray, SizeSELineArray, "SELineArray before overlap removal:");
      DumpXYL(EndCapArray, SizeEndCap, "EndCapArray before overlap removal:");
      DumpXYL(EELineArray, SizeEELineArray, "EELineArray before overlap removal:");
#endif

      /* removing of every overlapping (many can't arise) */
      if( GC_p->AluMode != STBLIT_ALU_COPY )
      {
        stgfx_RemoveXYLOverlaps(XYL_p, &Size, StartCapArray, &SizeStartCap);
        stgfx_RemoveXYLOverlaps(XYL_p, &Size, EndCapArray, &SizeEndCap);
        stgfx_RemoveXYLOverlaps(OuterArray, &SizeOuter, 
                                StartCapArray, &SizeStartCap);
        stgfx_RemoveXYLOverlaps(OuterArray, &SizeOuter,
                                EndCapArray, &SizeEndCap);
        stgfx_RemoveXYLOverlaps(ThicknessArray, &SizeThicknessArray,
                                StartCapArray, &SizeStartCap);
        stgfx_RemoveXYLOverlaps(ThicknessArray, &SizeThicknessArray,
                                EndCapArray, &SizeEndCap);
        stgfx_RemoveXYLOverlaps(SELineArray, &SizeSELineArray,
                                StartCapArray, &SizeStartCap);
        stgfx_RemoveXYLOverlaps(EELineArray, &SizeEELineArray,
                                EndCapArray, &SizeEndCap);
        if(ArcAngle > (64*270))
        {
          stgfx_RemoveXYLOverlaps(StartCapArray, &SizeStartCap,
                                  EndCapArray, &SizeEndCap);
        }
      }
      
      Err = STBLIT_DrawXYLArray( BlitHandle, Bitmap_p, 
                                 StartCapArray, SizeStartCap, 
                                 &(LocalGC.DrawColor), &BlitContext );
#if 0
      DumpXYL(StartCapArray, SizeStartCap, "StartCapArray drawn:");
      DumpXYL(SELineArray, SizeSELineArray, "SELineArray drawn:");
#endif
      DonePause("STGFX_DrawArc: DrawXYLArray(StartCapArray)");
      if(Err != ST_NO_ERROR)
      {
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }

      Err = STBLIT_DrawXYLArray( BlitHandle, Bitmap_p, 
                                 EndCapArray, SizeEndCap,
                                 &(LocalGC.DrawColor), &BlitContext );
#if 0
      DumpXYL(EndCapArray, SizeEndCap, "EndCapArray drawn:");
      DumpXYL(EELineArray, SizeEELineArray, "EELineArray drawn:");
#endif
      DonePause("STGFX_DrawArc: DrawXYLArray(EndCapArray)");
      if(Err != ST_NO_ERROR)
      {
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }
    } /* end of Raster Cap */
    
    /* draw and free arrays for cases 3) and 4) */
    Err = STBLIT_DrawXYLArray( BlitHandle, Bitmap_p, XYL_p, Size,
                               &(LocalGC.DrawColor), &BlitContext );
    DonePause("STGFX_DrawArc: DrawXYLArray(XYL_p)");
    if(Err != ST_NO_ERROR)
    {
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    Err = STBLIT_DrawXYLArray( BlitHandle, Bitmap_p,
                               OuterArray, SizeOuter,
                               &(LocalGC.DrawColor), &BlitContext );
    DonePause("STGFX_DrawArc: DrawXYLArray(OuterArray)");
    if(Err != ST_NO_ERROR)
    {
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    Err = STBLIT_DrawXYLArray( BlitHandle, Bitmap_p, 
                               SELineArray, SizeSELineArray,
                               &(LocalGC.DrawColor), &BlitContext );
    DonePause("STGFX_DrawArc: DrawXYLArray(SELineArray)");
    if(Err != ST_NO_ERROR)
    {
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    Err = STBLIT_DrawXYLArray( BlitHandle, Bitmap_p, 
                               EELineArray, SizeEELineArray,
                               &(LocalGC.DrawColor), &BlitContext );
    DonePause("STGFX_DrawArc: DrawXYLArray(EELineArray)");
    if(Err != ST_NO_ERROR)
    {
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    Err = STBLIT_DrawXYLArray( BlitHandle, Bitmap_p, 
                               ThicknessArray, SizeThicknessArray,
                               &(LocalGC.DrawColor), &BlitContext );
    DonePause("STGFX_DrawArc: DrawXYLArray(ThicknessArray)");
    if(Err != ST_NO_ERROR)
    {
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
  } /* end of all the 4 arc cases */
  
  TIME_SYNC(Handle);
  TIME_HERE("STGFX_DrawArc drawing done");
  
  /* tiling for the contour */
  if(GC_p->DrawTexture_p != NULL)
  {
#if 0 /* old code used restricted span */
    S32 MaxSpan;
    S32 xMin, xMax, yMin, yMax;

    MaxSpan = FRACT_TO_UINT(LRadius);
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
#endif /* extract from old draw tiling */
            
    Bitmap_p = TargetBitmap_p; /* go back to writing to real target */

    /* ApplyTexture takes care of freeing MaskDataBH */
    Err = stgfx_ApplyTexture(Handle, 0, 0, GC_p, GC_p->DrawTexture_p,
                             &MaskBitmap, &MaskDataBH, Bitmap_p,
                             &BlitContext);
    if(Err != ST_NO_ERROR)
    {
      LastErr = Err;
    }

    stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
  }

  stgfx_ScheduleAVMEM_free(Handle, &XYLBlockHandle);
  
  TIME_SYNC(Handle);
  TIME_HERE("STGFX_DrawArc scheduled memory free");

  return(LastErr);
} /* end of STGFX_DrawArc() */


/* Arc raster functions are used in many places (round cap/join, sectors),
  so are in gfx_conic.c */
  

/****************************************************************************\
Name        : InitArrayXYL()
Description : initializes an xyl array 
Parameters  : 
Assumptions : 
Limitations : 
Returns     :
void InitArrayXYL(
   U32 Size, 
   STBLIT_XYL_t* ArrayXYL
)
{
  U32 i;
  for(i = 0; i < Size; i++)
  {
     ArrayXYL[i].PositionX = 0;
     ArrayXYL[i].PositionY = 0;
     ArrayXYL[i].Length = 0;
  }
}
\****************************************************************************/

