/******************************************************************************

    File Name   : gfx_roundrect.c

    Description : RoundRectangle drawing

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#ifdef STGFX_TRACE_ALLOC
#define STTBX_PRINT
#define TRACE_ALLOC(x) STTBX_Print(x)
#else
#define TRACE_ALLOC(x)
#endif

#include <string.h>
#include "sttbx.h"

#include "gfx_tools.h" /* includes stgfx_init.h, ... */

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

void RotoTranslation(
    S32 Orientation, 
    S32 RectangleOriginX,
    S32 RectangleOriginY,
    S32* XRT, 
    S32* YRT
);

/* Functions --------------------------------------------------------------- */

/*******************************************************************************
Name        : STGFX_DrawRoundRectangle()
Description : draws a rounded rectangle according to a specified graphics
              context
Parameters  : Bitmap_p,
              GC_p,
              Rectangle,
              HRadius,
              VRadius,
              Orientation
Assumptions : The target bitmap, the GC and the parameters are correct
Limitations : 
Returns     : error if a parameter is not correct
*******************************************************************************/
/****************************************************************************** 
   this banner comment explains the geometry of the round rectangle and
   the indexes of the rectangle elements and the corrensponding data in
   this function. Orientation is assumed 0
           
          P[0]_________side 0____ ______P[1]
            /                           \
  corner 0 /                             \ corner 1
          /                               \
     P[7]/                                 \P[2] 
         |                                 |
         |                                 |
  side 3 |                                 | side 1
         |                                 |
         |                                 |
      P[6]\                                /P[3]
           \                              /
    corner 3\                            /corner 2
             \_________side 2___________/
           P[5]                         P[4]

IMPORTANT: ACCORDING TO THE Orientation PARAMETER, THE RECTANGLE AND ITS
ELEMENTS should ROTATE COUNTERCLOCKWISE AROUND THE POINT DEFINED BY
  (Rectangle.PositionX, Rectangle.PositionY)

  corners XYL arrays: taking corner 1:
     
    ____end___
    \---------\
     \---------\ext
   int\--fill---\
       \---------\
        \__start__\
   
   these are the XYL arrays needed by the corners rendering

*******************************************************************************/
ST_ErrorCode_t STGFX_DrawRoundRectangle(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         TargetBitmap_p,
  const STGFX_GC_t*         GC_p,
  STGXOBJ_Rectangle_t       Rectangle,
  U32                       HRadius,
  U32                       VRadius,
  S32                       Orientation
)
{
  /* LastErr stores the most recent non-fatal error */
  ST_ErrorCode_t         Err, LastErr = ST_NO_ERROR;
  STBLIT_BlitContext_t   BlitContext; /* blitting context */
  STBLIT_Handle_t        BlitHandle; /* blit handle */
  S32                    Shift; /* shift value */
  S32                    X[8], Y[8]; /* straight sides' extrema */
  
  /* the following are referred to the description of the round rec above */
  STBLIT_XYL_t*          StraightXYL[4] = {NULL, NULL, NULL, NULL};
  U32                    StraightXYLNum[4];
  STAVMEM_BlockHandle_t  StraightXYLBlockHandle[4];
  STBLIT_XYL_t*          CornerExtXYL[4] = {NULL, NULL, NULL, NULL};
  U32                    CornerExtXYLNum[4];
  STAVMEM_BlockHandle_t  CornerExtXYLBlockHandle[4];
  STBLIT_XYL_t*          CornerFillXYL[4] = {NULL, NULL, NULL, NULL};
  U32                    CornerFillXYLNum[4];
  STAVMEM_BlockHandle_t  CornerFillXYLBlockHandle[4];
  STBLIT_XYL_t*          CornerIntXYL[4] = {NULL, NULL, NULL, NULL};
  U32                    CornerIntXYLNum[4];
  STAVMEM_BlockHandle_t  CornerIntXYLBlockHandle[4];
  STBLIT_XYL_t*          CornerStartXYL[4] = {NULL, NULL, NULL, NULL};
  U32                    CornerStartXYLNum[4];
  STAVMEM_BlockHandle_t  CornerStartXYLBlockHandle[4];
  STBLIT_XYL_t*          CornerEndXYL[4] = {NULL, NULL, NULL, NULL};
  U32                    CornerEndXYLNum[4];
  STAVMEM_BlockHandle_t  CornerEndXYLBlockHandle[4];
  STBLIT_XYL_t*          FillXYL = NULL;
  U32                    FillXYLNum;
  STAVMEM_BlockHandle_t  FillXYLBlockHandle;
  S32                    PixelPoints[4][8];
  U32                    idx;
  STGFX_Point_t          PolygonPoints[4];
  S32                    ArcAngle = 90*64;
  stgfx_Side_t           Sides[10]; /* used when EnableFilling is true */
  S32                    XC[4], YC[4]; /* corners' centers */
  S32                    FractUnity = 1; /* this will be the fractional unity */
  U32                    i, j; /* counters */
  STGFX_GC_t             LocalGC; /* modifiable GC for degenerate cases */
  U32                    StartAngle[] = {5760, 0, 17280, 11520};
  U32                    MaxSide;
  S32                    XMin;
  S32                    YMin;
  S32                    XMax;
  S32                    YMax;
  U32                    TotOfSides;
  U32                    PosMin;
  U32                    PosMax;
  U32                    TotSides;
  S32                    XA[4];
  S32                    YA[4];
  S32                    XB[4];
  S32                    YB[4];
  U16                    Add1 = 0;
  U16                    Add2 = 0;
  
  /* tiling items */
  STGXOBJ_Color_t        Color;
  STGXOBJ_Bitmap_t       MaskBitmap;
  STGXOBJ_Bitmap_t*      Bitmap_p = TargetBitmap_p;
  STAVMEM_BlockHandle_t  MaskDataBH;
  STAVMEM_BlockHandle_t  WorkBufferBH = STAVMEM_INVALID_BLOCK_HANDLE;
  
  TIME_HERE("STGFX_DrawRoundRectangle");
  
  /* could let InitBlitContext do these checks if we move things about */
  if((TargetBitmap_p == NULL) || (GC_p == NULL))
  {
    return ST_ERROR_BAD_PARAMETER;
  }
  
  LocalGC = *GC_p;
  
  /* immediately check handles validity */
  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  BlitHandle = stgfx_GetBlitterHandle(Handle);

  /* bad dimensions */
  if((Rectangle.Width+1)<(2*HRadius))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  if((Rectangle.Height+1)<(2*VRadius))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }
  
  if(HRadius==0 || VRadius==0) /* in these cases collapses to a rectangle */
  {
    LocalGC.JoinStyle = STGFX_ROUNDED_JOIN;
    return(STGFX_DrawRectangle(Handle, Bitmap_p, &LocalGC,
                               Rectangle, Orientation));
  }
  else if((2*HRadius)==(Rectangle.Width+1) && (2*VRadius)>=(Rectangle.Height+1))
  { /* in these cases collapses to an ellipse */
    XC[0] = Rectangle.PositionX + Rectangle.Width/2;
    YC[0] = Rectangle.PositionY + Rectangle.Height/2;
    return(STGFX_DrawEllipse(Handle, Bitmap_p, GC_p, XC[0], YC[0],
                             HRadius, VRadius, Orientation));
  }
  
  
  /* parameters checking */
  if(GC_p->LineWidth == 0)
  {
    return(STGFX_ERROR_INVALID_GC);
  }
  if((GC_p->CapStyle != STGFX_RECTANGLE_CAP &&
      GC_p->CapStyle != STGFX_PROJECTING_CAP &&
      GC_p->CapStyle != STGFX_ROUND_CAP) ||
     (GC_p->JoinStyle != STGFX_RECTANGLE_JOIN &&
      GC_p->JoinStyle != STGFX_FLAT_JOIN      &&
      GC_p->JoinStyle != STGFX_ROUNDED_JOIN))
  {
    return(STGFX_ERROR_INVALID_GC);
  }
  
  /* copy pertinent Graphic context params to the blitting context */
  Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
  if (Err != ST_NO_ERROR)
  {
    return (Err);
  }
  
  if((GC_p->LineWidth == 1) && (GC_p->LineStyle_p == NULL) &&
     (Orientation % (90*64) == 0) && (GC_p->DrawTexture_p == NULL) &&
     (!(GC_p->EnableFilling) || (GC_p->FillTexture_p == NULL) &&
      (GC_p->FillColor.Type == GC_p->DrawColor.Type) &&
      stgfx_AreColorsEqual(&(GC_p->FillColor), &(GC_p->DrawColor))))
  {
    STBLIT_XYL_t* XYL_p;
    S32 xLeft, xRight, xDraw, yUp, yDown;
    U32 lDraw;
    S32 asq, bsq, c;
    S32 x, y;
    S32 d;

    /* dispose of Orientation */
    if(Orientation % (180*64)) /* will actually be 90*64 */
    {
      GFXSWAP(HRadius, VRadius);
    }
    
    if(GC_p->XAspectRatio != GC_p->YAspectRatio)
    {
      Rectangle.PositionX = (Rectangle.PositionX/GC_p->XAspectRatio)
                                * GC_p->YAspectRatio;
      Rectangle.Width = (Rectangle.Width/GC_p->XAspectRatio)
                                * GC_p->YAspectRatio;
      HRadius = (HRadius/GC_p->XAspectRatio)*GC_p->YAspectRatio;
    }
    
    /* discard fractional parts now; x and y always move by integer steps */
    Shift = stgfx_GetUserAccuracy(Handle);
    if(Shift > 0)
    {
      Rectangle.PositionX >>= Shift;
      Rectangle.PositionY >>= Shift;
      Rectangle.Width >>= Shift;
      Rectangle.Height >>= Shift;
      HRadius >>= Shift;
      VRadius >>= Shift;
    }

    FillXYLNum = 2*VRadius;
    if(!GC_p->EnableFilling) FillXYLNum += 2*(VRadius-1);
    
    Err = stgfx_AVMEM_malloc(Handle, FillXYLNum*sizeof(STBLIT_XYL_t),
                             &FillXYLBlockHandle, (void**) &FillXYL);
    if(Err != ST_NO_ERROR)
    {
      return Err;
    }
    
    TIME_HERE("STGFX_DrawRoundRectangle aligned case setup and allocations done");

    asq = HRadius*HRadius;
    bsq = VRadius*VRadius;

    x = 0;
    y = VRadius;

    /* d = (bsq - (VR*asq) + (asq/4)), but round up on the division because all we
      care about are zero-crossings, and all other contributions are integer */
    d = bsq - (VRadius*asq) + (asq+3)/4;

    XYL_p = FillXYL;
    
    xLeft = Rectangle.PositionX + HRadius; /* XC of top-left quarter ellipse */
    lDraw = Rectangle.Width - 2*HRadius;
    
    /* algorithm works from outside y inwards */
    yUp = Rectangle.PositionY;
    yDown = Rectangle.PositionY + Rectangle.Height - 1;
        
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
        if(!GC_p->EnableFilling)
        {
          if(y == VRadius)
          {
            /* first y is the same as filled case, two segments covering the
              curve start and the top and bottom sides */
              
            xRight = xLeft + lDraw - 1; /* offsets are relative to here */
          }
          else
          {
            lDraw /= 2; /* remove fake *2 in increment below */
          
            /* two more segments to draw for all y except the first */
            XYL_p->PositionX = xRight;
            XYL_p->PositionY = yUp;
            XYL_p->Length = lDraw;
            ++XYL_p;

            XYL_p->PositionX = xRight;
            XYL_p->PositionY = yDown;
            XYL_p->Length = lDraw;
            ++XYL_p;
            
            xRight += lDraw;
          }
        }
      
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yUp;
        XYL_p->Length = lDraw;
        ++XYL_p;

        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = yDown;
        XYL_p->Length = lDraw;
        ++XYL_p;
        
        if(!GC_p->EnableFilling) lDraw = 0; /* restart */
        
        /* update to new position */
        d += bsq*(2*x + 3) + 2*asq*(1-y);
        y -= 1;
        
        yUp += 1;
        yDown -= 1;
      }
      
      x += 1;

      xLeft -= 1;
      lDraw += 2;
    }

    /* (bsq*(x+0.5)^2) + (asq*(y-1)^2) - asq*bsq;
      Original code had 0.25 rather than 0.5 as in Feller */
    d = (bsq*(x*x + x) + (bsq+3)/4) + (asq*(y-1)*(y-1)) - asq*bsq;

    /* second loop chooses between N and NE */
    while(y > 0)
    {
      /* write previous segment */
      if(!GC_p->EnableFilling)
      {
        /* could remove this if guarantied at least one y step in first loop */
        if(y == VRadius)
        {
          /* first y is the same as filled case, two segments covering the
            curve start and the top and bottom sides */

          xRight = xLeft + lDraw - 1; /* offsets are relative to here */
        }
        else
        {
          lDraw /= 2; /* remove fake *2 in increment below */
          xDraw = xRight; /* where to draw this time */
          xRight += lDraw;
          
          if(lDraw == 0) lDraw = 1; /* N move without and E */

          /* two more segments to draw for all y except the first */
          XYL_p->PositionX = xDraw;
          XYL_p->PositionY = yUp;
          XYL_p->Length = lDraw;
          ++XYL_p;

          XYL_p->PositionX = xDraw;
          XYL_p->PositionY = yDown;
          XYL_p->Length = lDraw;
          ++XYL_p;
        }
      }

      XYL_p->PositionX = xLeft;
      XYL_p->PositionY = yUp;
      XYL_p->Length = lDraw;
      ++XYL_p;

      XYL_p->PositionX = xLeft;
      XYL_p->PositionY = yDown;
      XYL_p->Length = lDraw;
      ++XYL_p;
    
      if(!GC_p->EnableFilling) lDraw = 0; /* restart */

      if(d<0) /* midpoint is internal, new point is NE */
      {
        d += (2*bsq*(x+1) + asq*(3-2*y));
        x += 1;
        
        xLeft -= 1;
        lDraw += 2;
      }
      else /* midpoint is external, new point is N */
      {
        d += (asq*(3 - 2*y));
      }
      
      y -= 1;
      yUp += 1;
      yDown -= 1;
    }
    
    /* don't actually need y=0 segment because it is always full width
      and so can be done in the block fill */

    if(XYL_p - FillXYL > FillXYLNum)
    {
      return(STGFX_ERROR_XYL_POOL_EXCEEDED);
    }
    FillXYLNum = XYL_p - FillXYL;

    TIME_HERE("STGFX_DrawRoundRectangle aligned case xyl calculation done");
    
    BlitContext.UserTag_p = (void*)FillXYLBlockHandle;
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                              FillXYL, FillXYLNum,
                              (STGXOBJ_Color_t*) &(GC_p->DrawColor),
                              &BlitContext);
    if(Err != ST_NO_ERROR)
    {
      stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    
    TIME_SYNC(Handle);
    TIME_HERE("STGFX_DrawRoundRectangle aligned case xyl drawing done");
    
    /* fill central region */
    Rectangle.PositionY += VRadius;
    Rectangle.Height -= 2*VRadius;
    
    BlitContext.NotifyBlitCompletion = FALSE; /* no memory to free here */
    
    if(GC_p->EnableFilling)
    {
      Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                 (STGXOBJ_Color_t*) &(GC_p->DrawColor),
                                 &BlitContext);

      if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    else
    {
      xRight = Rectangle.PositionX + Rectangle.Width - 1; /* save for later */
      
      Rectangle.Width = 1;
      
      Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                 (STGXOBJ_Color_t*) &(GC_p->DrawColor),
                                 &BlitContext);

      if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;
      
      Rectangle.PositionX = xRight;
      
      Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                 (STGXOBJ_Color_t*) &(GC_p->DrawColor),
                                 &BlitContext);

      if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;
    }

    TIME_SYNC(Handle);
    TIME_HERE("STGFX_DrawRoundRectangle aligned case centre block fill done");
    
    return LastErr;
  }
  
  /* the "shifted unit" is shifted by the precision
     asked ny the STGFX user */
  FractUnity = FractUnity << stgfx_GetUserAccuracy(Handle);
  /* to arrange sides' positions (?) */
  Rectangle.Width -= FractUnity;
  Rectangle.Height -= FractUnity;
  
  /* adjust fractional precision */
  Shift = STGFX_INTERNAL_ACCURACY - stgfx_GetUserAccuracy(Handle);
  if(Shift > 0 )
  {
    Rectangle.PositionX = Rectangle.PositionX << Shift;
    Rectangle.PositionY = Rectangle.PositionY << Shift;
    Rectangle.Width = Rectangle.Width << Shift;
    Rectangle.Height = Rectangle.Height << Shift;
    HRadius = HRadius << Shift; 
    VRadius = VRadius << Shift;
  }
  else if (Shift < 0)
  {
    U8 Sh = -Shift;
    Rectangle.PositionX = (Rectangle.PositionX + (1<<(Sh-1))*
                          (Rectangle.PositionX>=0 ? 1 : -1)) / (1<<Sh);
    Rectangle.PositionY = (Rectangle.PositionY + (1<<(Sh-1))*
                          (Rectangle.PositionY>=0 ? 1 : -1)) / (1<<Sh);
    Rectangle.Width = (Rectangle.Width + (1<<(Sh-1)))>> Sh;
    Rectangle.Height = (Rectangle.Height + (1<<(Sh-1))) >> Sh;
    HRadius = (HRadius + (1<<(Sh-1))) >> Sh; 
    VRadius = (VRadius + (1<<(Sh-1))) >> Sh; 
  }
  /* if Shift == 0 do nothing */
  
  /* the actual fractional unity is used to adjust the position of
     the staright sides left and bottom in case
     of even line width */
  FractUnity = 1<<STGFX_INTERNAL_ACCURACY;


  MaxSide = GFXMAX(FRACT_TO_UINT(Rectangle.Height),
                   FRACT_TO_UINT(Rectangle.Width));
  /* straight sides */
  /* horizontals */
  /* extrema computation */
  X[0] = X[5] = Rectangle.PositionX + HRadius;
  Y[0] = Y[1] = Rectangle.PositionY;
  X[1] = X[4] = Rectangle.PositionX + ((Rectangle.Width - HRadius));
  Y[4] = Y[5] = Rectangle.PositionY + Rectangle.Height; 

  /* "vertical" sides */
  /* extrema computation */
  X[2] = X[3] = Rectangle.PositionX + Rectangle.Width;
  Y[2] = Y[7] = Rectangle.PositionY + VRadius;
  Y[3] = Y[6] = Rectangle.PositionY + (Rectangle.Height - VRadius);
  X[6] = X[7] = Rectangle.PositionX;

  /* aspect ratio correction */
  if(GC_p->XAspectRatio != GC_p->YAspectRatio)
  {
    for(i=0; i<8; i++)
      X[i] = (X[i]/GC_p->XAspectRatio)*GC_p->YAspectRatio;
  }

  /* straight sides allocations */
  for(i=0; i<4; i++)
  {
    if(HRadius >= VRadius)
    {
      StraightXYLNum[i] = MaxSide + GC_p->LineWidth + 2;
    }
    else
    {
      StraightXYLNum[i] = MaxSide + GC_p->LineWidth + 2;
    }

    /* for dashed lines we need extra space */
    if(GC_p->LineStyle_p != NULL)
    {
      StraightXYLNum[i] += GFXABS(FRACT_TO_INT(X[2*i])-FRACT_TO_INT(X[2*i+1]))*
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
      return(Err);
    }
    TRACE_ALLOC(("Allocated StraightXYL[%i] %u elements\n",
                 i, StraightXYLNum[i]));
  }
  

  /* Centers of the arcs */
  CheckAngle(&Orientation);
  XC[0] = XC[3] = X[0];
  YC[0] = YC[1] = Y[2];
  XC[1] = XC[2] = X[1];
  YC[2] = YC[3] = Y[3];
  for(i = 0; i < 4; i++)
  {
    RotoTranslation(Orientation, Rectangle.PositionX, Rectangle.PositionY, 
                    &(XC[i]),&(YC[i]));
  }
  
  /* Arcs allocation */
  /* the internal arcs are used in EVERY case */
  for(i=0; i<=3; i++)
  {
    CornerIntXYLNum[i] = 2*(GC_p->LineWidth + 2 +
            2*GFXMAX(FRACT_TO_UINT(HRadius),FRACT_TO_UINT(VRadius)));
    Err = stgfx_AVMEM_malloc(Handle, CornerIntXYLNum[i]*sizeof(STBLIT_XYL_t),
                             &CornerIntXYLBlockHandle[i],
                             (void**) &CornerIntXYL[i]);
    if(Err != ST_NO_ERROR)
    {
      for(j=0; j<i; j++)
      {
        stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
      }
      for(j=0; j<4; j++)
      {
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
      }
      return(Err);
    }
  }
  
  /* if the line is thick, dashed or undashed */
  if(GC_p->LineWidth>1)
  {
    /* external arcs*/
    for(i=0; i<=3; i++)
    {
      CornerExtXYLNum[i] = 2*(GC_p->LineWidth + 2 +
            2*GFXMAX(FRACT_TO_UINT(HRadius),FRACT_TO_UINT(VRadius)));
      Err = stgfx_AVMEM_malloc(Handle, CornerExtXYLNum[i]*sizeof(STBLIT_XYL_t),
                               &CornerExtXYLBlockHandle[i],
                               (void**) &CornerExtXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<i; j++)
        {
          stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
        }
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
        }
        return(Err);
      }
    }      
    /* arcs'  filling */
    for(i=0; i<=3; i++)
    {
      CornerFillXYLNum[i] = 2*(GC_p->LineWidth + 2 +
            2*GFXMAX(FRACT_TO_UINT(HRadius),FRACT_TO_UINT(VRadius)));
      Err = stgfx_AVMEM_malloc(Handle, CornerFillXYLNum[i]*sizeof(STBLIT_XYL_t),
                               &CornerFillXYLBlockHandle[i],
                               (void**) &CornerFillXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<i; j++)
        {
          stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[j]);
        }
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
        }
        return(Err);
      }
    }
    /* arcs' starts */
    for(i=0; i<=3; i++)
    {
      CornerStartXYLNum[i] = (GC_p->LineWidth + 2);
      if(GC_p->LineStyle_p != NULL)
      {
        CornerStartXYLNum[i] = CornerStartXYLNum[i]*
                          GFXMAX(FRACT_TO_UINT(HRadius),FRACT_TO_UINT(VRadius));
      }
      
      Err = stgfx_AVMEM_malloc(Handle,CornerStartXYLNum[i]*sizeof(STBLIT_XYL_t),
                               &CornerStartXYLBlockHandle[i],
                               (void**) &CornerStartXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<i; j++)
        {
          stgfx_AVMEM_free(Handle, &CornerStartXYLBlockHandle[j]);
        }
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[j]);
        }
        return(Err);
      }
    }
    /* arcs' ends */
    for(i=0; i<=3; i++)
    {
      CornerEndXYLNum[i] = GC_p->LineWidth + 2;
      if(GC_p->LineStyle_p != NULL)
      {
        CornerEndXYLNum[i] = CornerEndXYLNum[i]*
                          GFXMAX(FRACT_TO_UINT(HRadius),FRACT_TO_UINT(VRadius));
      }
      Err = stgfx_AVMEM_malloc(Handle, CornerEndXYLNum[i]*sizeof(STBLIT_XYL_t),
                               &CornerEndXYLBlockHandle[i],
                               (void**) &CornerEndXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<i; j++)
        {
          stgfx_AVMEM_free(Handle, &CornerEndXYLBlockHandle[j]);
        }
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerStartXYLBlockHandle[j]);
        }
        return(Err);
      }
    }
    
    /* rendering of thick arcs */
    if(GC_p->LineStyle_p == NULL) /* undashed thick arcs */
    {
      for(i = 0; i < 4; i++)
      {
        Err = stgfx_2dRasterThickArc(Bitmap_p->Height,
                                     XC[i], YC[i], 
                                     HRadius, VRadius,
                                     StartAngle[i], ArcAngle, Orientation,
                                     GC_p->LineWidth,
                                     &CornerIntXYLNum[i],
                                     CornerIntXYL[i],
                                     &CornerExtXYLNum[i],
                                     CornerExtXYL[i],
                                     &CornerFillXYLNum[i],
                                     CornerFillXYL[i],
                                     &CornerStartXYLNum[i],
                                     CornerStartXYL[i],
                                     &CornerEndXYLNum[i],
                                     CornerEndXYL[i],
                                     PixelPoints[i],
                                     NULL);
        if(Err != ST_NO_ERROR)
        {
          for(j=0; j<4; j++)
          {
            stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerStartXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerEndXYLBlockHandle[j]);
          }
          return(Err);
        }
      }
      for(i = 0; i < 4; i++)
      {
        PolygonPoints[0].X = PixelPoints[i][0];
        PolygonPoints[0].Y = PixelPoints[i][1];
        PolygonPoints[1].X = PixelPoints[i][2];
        PolygonPoints[1].Y = PixelPoints[i][3];
        if(i == 3)
        {
          idx = 0;
        }
        else
        {
          idx = i + 1;
        }
        PolygonPoints[2].X = PixelPoints[idx][6];
        PolygonPoints[2].Y = PixelPoints[idx][7];
        PolygonPoints[3].X = PixelPoints[idx][4];
        PolygonPoints[3].Y = PixelPoints[idx][5];

        Err = stgfx_FillSimplePolygon(Handle, 4, PolygonPoints, 
                                      &StraightXYLNum[i], 
                                      StraightXYL[i]);

        if(Err != ST_NO_ERROR)
        {
          for(j=0; j<4; j++)
          {
            stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerStartXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerEndXYLBlockHandle[j]);
          }
          return(Err);
        }
      }
    }
    else if(GC_p->LineStyle_p != NULL) /* dashed thick */
    {
      for(i = 0; i < 4; i++)
      {
        Err = stgfx_2dRasterDashedArc(Bitmap_p->Height,
                                XC[i], YC[i], 
                                HRadius, VRadius,
                                StartAngle[i], ArcAngle, Orientation,
                                GC_p->LineStyle_p,
                                GC_p->LineWidth,
                                &CornerIntXYLNum[i],
                                CornerIntXYL[i],
                                &CornerExtXYLNum[i],
                                CornerExtXYL[i],
                                &CornerFillXYLNum[i],
                                CornerFillXYL[i],
                                &CornerStartXYLNum[i],
                                CornerStartXYL[i],
                                &CornerEndXYLNum[i],
                                CornerEndXYL[i],
                                PixelPoints[i],
                                NULL);
        if(Err != ST_NO_ERROR)
        {
          for(j=0; j<4; j++)
          {
            stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerStartXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerEndXYLBlockHandle[j]);
          }
          return(Err);
        }
      }
      for(i = 0; i < 4; i++)
      {
        XA[i] = (PixelPoints[i][0] + PixelPoints[i][2])/2;
        YA[i] = (PixelPoints[i][1] + PixelPoints[i][3])/2;
        if(i == 3)
        {
          idx = 0;
        }
        else
        {
          idx = i + 1;
        }
        XB[i] = (PixelPoints[idx][6] + PixelPoints[idx][4])/2;
        YB[i] = (PixelPoints[idx][7] + PixelPoints[idx][5])/2;

        Err = stgfx_2dRasterDashedLine( Handle, XA[i], YA[i], XB[i], YB[i],
                                        GC_p->LineStyle_p,
                                        GC_p->LineWidth, 
                                        &StraightXYLNum[i], 
                                        StraightXYL[i] );
        if(Err != ST_NO_ERROR)
        {
          for(j=0; j<4; j++)
          {
            stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerStartXYLBlockHandle[j]);
            stgfx_AVMEM_free(Handle, &CornerEndXYLBlockHandle[j]);
          }
          return(Err);
        }
      }
    }
  }
  if(GC_p->LineWidth==1 && GC_p->LineStyle_p==NULL)/* simple arc */
  {
    for(i = 0; i < 4; i++)
    {
      Err = stgfx_2dRasterSimpleArc(XC[i], YC[i], HRadius, VRadius,
                                    StartAngle[i], ArcAngle, Orientation,
                                    &CornerIntXYLNum[i],
                                    CornerIntXYL[i],
                                    PixelPoints[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
        }
        return(Err);
      }
    }
    for(i = 0; i < 4; i++)
    {
      PolygonPoints[0].X = PixelPoints[i][0];
      PolygonPoints[0].Y = PixelPoints[i][1];
      if(i == 3)
      {
        idx = 0;
      }
      else
      {
        idx = i + 1;
      }
      PolygonPoints[1].X = PixelPoints[idx][2];
      PolygonPoints[1].Y = PixelPoints[idx][3];
      Err = stgfx_2dRasterSimpleLine(PolygonPoints[0].X, PolygonPoints[0].Y,
                                     PolygonPoints[1].X, PolygonPoints[1].Y,
                                     &StraightXYLNum[i], 
                                     StraightXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
        }
        return(Err);
      }
    }
  }
  else if(GC_p->LineWidth==1 &&
          GC_p->LineStyle_p!=NULL) /* dashed thin arc (linewidth==1) */
  {
    for(i = 0; i < 4; i++)
    {
      Err = stgfx_2dRasterDashedArc(Bitmap_p->Height,
                                  XC[i], YC[i], 
                                  HRadius, VRadius,
                                  StartAngle[i], ArcAngle, Orientation,
                                  GC_p->LineStyle_p,
                                  1,
                                  &CornerIntXYLNum[i],
                                  CornerIntXYL[i],
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  PixelPoints[i],
                                  NULL);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
        }
        return(Err);
      }
    }

    /* 4 dashed lines (thickness = 1)*/
    for(i = 0; i < 4; i++)
    {
      PolygonPoints[0].X = PixelPoints[i][0];
      PolygonPoints[0].Y = PixelPoints[i][1];
      if(i == 3)
      {
        idx = 0;
      }
      else
      {
        idx = i + 1;
      }
      PolygonPoints[1].X = PixelPoints[idx][2];
      PolygonPoints[1].Y = PixelPoints[idx][3];
      Err = stgfx_2dRasterDashedLine(Handle, 
                                     PolygonPoints[0].X, PolygonPoints[0].Y,
                                     PolygonPoints[1].X, PolygonPoints[1].Y,
                                     GC_p->LineStyle_p, GC_p->LineWidth,
                                     &StraightXYLNum[i], 
                                     StraightXYL[i]);
      if(Err != ST_NO_ERROR)
      {
        for(j=0; j<4; j++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
        }
        return(Err);
      }
    }
  }

  /*******************************/
  /* rendering is now  completed */
  /*******************************/

  /* first tiling analysis */
  if((GC_p->DrawTexture_p != NULL) || (GC_p->FillTexture_p != NULL))
  {
    Err = stgfx_InitTexture(Handle, GC_p, TargetBitmap_p, &MaskBitmap,
                            &BlitContext.WorkBuffer_p, &WorkBufferBH);
    if (Err != ST_NO_ERROR)
    {
      for(j=0; j<4; j++)
      {
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[j]);
        stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[j]);
      }
      return(Err);
    }
  }

  /* filling */
  if(GC_p->EnableFilling == TRUE && GC_p->LineStyle_p == NULL)
  {
    if(Orientation == 0 || Orientation == (360*64) ||
       Orientation == 90*64 || Orientation == 180*64||
       Orientation == 270 * 64)
    {
      TotOfSides = 8;
      /* sides preparation */
      /* corners */
      Sides[0].Up = 1;
      Sides[0].Spin = 0;
      Sides[0].xylData = CornerIntXYL[0];
      Sides[0].length = CornerIntXYLNum[0];
      Sides[2].xylData = CornerIntXYL[1];
      Sides[2].length = CornerIntXYLNum[1];
      Sides[4].xylData = CornerIntXYL[2];
      Sides[4].length = CornerIntXYLNum[2];
      Sides[4].Up = -1;
      Sides[4].Spin = 2;
      Sides[6].xylData = CornerIntXYL[3];
      Sides[6].length = CornerIntXYLNum[3];
      /* straight sides */
      Sides[1].xylData = StraightXYL[0];
      Sides[1].length = StraightXYLNum[0];
      Sides[1].Up = 0;
      Sides[1].Spin = 1;
      Sides[3].xylData = StraightXYL[1];
      Sides[3].length = StraightXYLNum[1];
      Sides[5].xylData = StraightXYL[2];
      Sides[5].length = StraightXYLNum[2];
      Sides[7].xylData = StraightXYL[3];
      Sides[7].length = StraightXYLNum[3];
      if(Orientation == 0 || Orientation == 360*64 || Orientation == 180*64)
      {
        Sides[1].Up = 0;
        Sides[1].Spin = 1;
        Sides[2].Up = -1;
        Sides[2].Spin = 2;
        Sides[3].Up = -1;
        Sides[3].Spin = 2;
        Sides[5].Up = 0;
        Sides[5].Spin = 3;
        Sides[6].Up = 1;
        Sides[6].Spin = 0;
        Sides[7].Up = 1;
        Sides[7].Spin = 0;
      }
      else if(Orientation == 90*64 || Orientation == 270*64)
      {
        Sides[1].Up = 1;
        Sides[1].Spin = 0;
        Sides[2].Up = 1;
        Sides[2].Spin = 0;
        Sides[3].Up = 0;
        Sides[3].Spin = 1;
        Sides[5].Up = -1;
        Sides[5].Spin = 2;
        Sides[6].Up = -1;
        Sides[6].Spin = 2;
        Sides[7].Up = 0;
        Sides[7].Spin = 3;
      }
      /* filling allocation */
      if(Orientation == 0 || Orientation == 360*64 || Orientation == 180*64)
      {
        FillXYLNum = FRACT_TO_UINT(Rectangle.Height) + 2*GC_p->LineWidth + 2;
      }
      else if(Orientation == 90*64 || Orientation == 270*64)
      {
        FillXYLNum = FRACT_TO_UINT(Rectangle.Width) + 2*GC_p->LineWidth + 2;
      }
      Err = stgfx_AVMEM_malloc(Handle, FillXYLNum*sizeof(STBLIT_XYL_t),
                               &FillXYLBlockHandle,
                               (void**) &FillXYL);
      if(Err != ST_NO_ERROR)
      {
        for(i=0; i<4; i++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[i]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[i]);
        }
        return(Err);
      }    
      /* compute filling */
      if(Orientation == 0 || Orientation == 360*64)
      {
        YMin = FRACT_TO_INT(Rectangle.PositionY);
        YMax = FRACT_TO_INT(Rectangle.PositionY + (S32)Rectangle.Height);
      }
      else if(Orientation == 180*64)
      {
        YMin = FRACT_TO_INT(Rectangle.PositionY - (S32)Rectangle.Height);
        YMax = FRACT_TO_INT(Rectangle.PositionY);
      }
      else if(Orientation == 90*64)
      {
        YMin = FRACT_TO_INT(Rectangle.PositionY - (S32)Rectangle.Width);
        YMax = FRACT_TO_INT(Rectangle.PositionY);
      }
      else if(Orientation == 270*64)
      {
        YMin = FRACT_TO_INT(Rectangle.PositionY);
        YMax = FRACT_TO_INT(Rectangle.PositionY + (S32)Rectangle.Width);
      }
    }
    else if( Orientation != 0 && Orientation != 360*64 && 
             Orientation != 180*64 && Orientation != 90*64 && 
             Orientation != 270*64)
    {
     TotOfSides = 8;
     Add1 = 0;
     Add2 = 0;
     /* sides preparation */
     if((Orientation > 0 && Orientation < 90*64)||
        (Orientation > 180*64 && Orientation < 270*64))
     {
       /* corners */
       
       Sides[0].xylData = CornerIntXYL[0];
       Sides[0].length = CornerIntXYLNum[0];
       Sides[0].Up = -1;
       Sides[0].Spin = 0;
       SplitArrayXYL( CornerIntXYL[3], CornerIntXYLNum[3], 
                      &PosMin, &PosMax, FALSE,
                      &Sides[2], &TotSides );
       Add1 = TotSides -1;
       TotOfSides += Add1;
       if(Orientation < 90*64)
       {
         XMax = CornerIntXYL[3][PosMax].PositionX;
         YMax = CornerIntXYL[3][PosMax].PositionY;
       }
       else
       {
         XMin = CornerIntXYL[3][PosMin].PositionX;
         YMin = CornerIntXYL[3][PosMin].PositionY;
       }
       if(TotSides == 2)
       {
         Sides[2].Up = -1;
         Sides[2].Spin = 0;
         Sides[3].Up = 1;
         Sides[3].Spin = 1;
       } 
       else /* TotSides == 1 */
       {
         if((( PosMin < PosMax ) && (Orientation < 90*64)) ||
            (( PosMin > PosMax ) && (Orientation > 90*64)))
         {
           Sides[2].Up = -1;
           Sides[2].Spin = 0;
         }
         else /* PosMin > PosMax */
         {
           Sides[2].Up = 1;
           Sides[2].Spin = 1;
         }
       }
       Sides[4+Add1].xylData = CornerIntXYL[2];
       Sides[4+Add1].length = CornerIntXYLNum[2];
       Sides[4+Add1].Up = 1;
       Sides[4+Add1].Spin = 1;
       SplitArrayXYL( CornerIntXYL[1], CornerIntXYLNum[1], 
                      &PosMin, &PosMax, FALSE,
                      &Sides[6+Add1], &TotSides );
       if(Orientation < 90*64)
       {
         XMin = CornerIntXYL[1][PosMin].PositionX;
         YMin = CornerIntXYL[1][PosMin].PositionY;
       }
       else
       {
         XMax = CornerIntXYL[1][PosMax].PositionX;
         YMax = CornerIntXYL[1][PosMax].PositionY;
       }
       if(TotSides == 2)
       {
         Sides[6+Add1].Up = 1;
         Sides[6+Add1].Spin = 1;
         Sides[7+Add1].Up = -1;
         Sides[7+Add1].Spin = 0;
       }
       else /* TotSides == 1 */
       {
         if(((PosMin > PosMax) && (Orientation < 90*64)) ||
           (( PosMin < PosMax ) && (Orientation > 90*64)))
         {
           Sides[6+Add1].Up = 1;
           Sides[6+Add1].Spin = 1;
         }
         else
         {
           Sides[6+Add1].Up = -1;
           Sides[6+Add1].Spin = 0;
         }
       }
       Add2 = (TotSides -1);
       TotOfSides += Add2;
       /* straight sides */
       Sides[1].xylData = StraightXYL[3];
       Sides[1].length = StraightXYLNum[3];
       Sides[1].Up = -1;
       Sides[1].Spin = 0;
       Sides[3+Add1].xylData = StraightXYL[2];
       Sides[3+Add1].length = StraightXYLNum[2];
       Sides[3+Add1].Up = 1;
       Sides[3+Add1].Spin = 1;
       Sides[5+Add1].xylData = StraightXYL[1];
       Sides[5+Add1].length = StraightXYLNum[1];
       Sides[5+Add1].Up = 1;
       Sides[5+Add1].Spin = 1;
       Sides[7+Add1+Add2].xylData = StraightXYL[0];
       Sides[7+Add1+Add2].length = StraightXYLNum[0];
       Sides[7+Add1+Add2].Up = -1;
       Sides[7+Add1+Add2].Spin = 0;
     }
     else if((Orientation > 90*64 && Orientation < 180*64)||
             (Orientation > 270*64 && Orientation < 360*64))
     {
       /* corners */
       SplitArrayXYL( CornerIntXYL[0], CornerIntXYLNum[0], 
                      &PosMin, &PosMax, FALSE,
                      &Sides[0], &TotSides );
       if(Orientation < 180*64)
       {
         XMax = CornerIntXYL[0][PosMax].PositionX;
         YMax = CornerIntXYL[0][PosMax].PositionY;
       }
       else
       {
         XMin = CornerIntXYL[0][PosMin].PositionX;
         YMin = CornerIntXYL[0][PosMin].PositionY;
       }
       Add1 = TotSides - 1;
       TotOfSides += Add1;
       if(TotSides == 2)
       {
         Sides[0].Up = -1;
         Sides[0].Spin = 0;
         Sides[1].Up = 1;
         Sides[1].Spin = 1;
       }
       else /* TotSides == 1 */
       {
       
         if(((PosMin < PosMax) && (Orientation < 180*64)) ||
            ((PosMin > PosMax) && (Orientation > 180*64))) 
         {
           Sides[0].Up = -1;
           Sides[0].Spin = 0;
         }
         else
         {
           Sides[0].Up = 1;
           Sides[0].Spin = 1;
         }
       }
       Sides[2+Add1].xylData = CornerIntXYL[3];
       Sides[2+Add1].length = CornerIntXYLNum[3];
       Sides[2+Add1].Up = 1;
       Sides[2+Add1].Spin = 1;
       SplitArrayXYL( CornerIntXYL[2], CornerIntXYLNum[2], 
                      &PosMin, &PosMax, FALSE,
                      &Sides[4+Add1], &TotSides );
       Add2 = TotSides -1;
       TotOfSides += Add2;
       if(Orientation < 180 *64)
       {
         XMin = CornerIntXYL[2][PosMin].PositionX;
         YMin = CornerIntXYL[2][PosMin].PositionY;
       }
       else
       {
         XMax = CornerIntXYL[2][PosMax].PositionX;
         YMax = CornerIntXYL[2][PosMax].PositionY;
       }
       if(TotSides == 2)
       {
         Sides[4+Add1].Up = 1;
         Sides[4+Add1].Spin = 1;
         Sides[5+Add1].Up = -1;
         Sides[5+Add1].Spin = 0;
       }
       else /* TotSides == 1 */
       {
           if(((PosMax < PosMin) && (Orientation < 180*64)) || 
              ((PosMax > PosMin) && (Orientation > 180*64)))
           {
             Sides[4+Add1].Up = 1;
             Sides[4+Add1].Spin = 1;
           }
           else
           {
             Sides[4+Add1].Up = -1;
             Sides[4+Add1].Spin = 0;
           }
       }
       Sides[6+Add1+Add2].xylData = CornerIntXYL[1];
       Sides[6+Add1+Add2].length = CornerIntXYLNum[1];
       Sides[6+Add1+Add2].Up = -1;
       Sides[6+Add1+Add2].Spin = 0;
       /* straight sides */
       Sides[1+Add1].xylData = StraightXYL[3];
       Sides[1+Add1].length = StraightXYLNum[3];
       Sides[1+Add1].Up = 1;
       Sides[1+Add1].Spin = 1;
       Sides[3+Add1].xylData = StraightXYL[2];
       Sides[3+Add1].length = StraightXYLNum[2];
       Sides[3+Add1].Up = 1;
       Sides[3+Add1].Spin = 1;
       Sides[5+Add1+Add2].xylData = StraightXYL[1];
       Sides[5+Add1+Add2].length = StraightXYLNum[1];
       Sides[5+Add1+Add2].Up = -1;
       Sides[5+Add1+Add2].Spin = 0;
       Sides[7+Add1+Add2].xylData = StraightXYL[0];
       Sides[7+Add1+Add2].length = StraightXYLNum[0];
       Sides[7+Add1+Add2].Up = -1;
       Sides[7+Add1+Add2].Spin = 0; 
     }
     /* filling allocation */
      FillXYLNum = 2* MaxSide + 2*GC_p->LineWidth + 2;
      Err = stgfx_AVMEM_malloc(Handle, FillXYLNum*sizeof(STBLIT_XYL_t),
                               &FillXYLBlockHandle,
                               (void**) &FillXYL);
      if(Err != ST_NO_ERROR)
      {
        for(i=0; i<4; i++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[i]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[i]);
        }
        return(Err);
      }    
      /* compute filling */
    }
    Err = stgfx_FillGenericPolygon(Bitmap_p->Height,
                                   YMin, YMax,
                                   Sides,
                                   TotOfSides,
                                   FillXYL,
                                   &FillXYLNum,
                                   STGFX_WINDING_FILL);
    if(Err != ST_NO_ERROR)
    {
      for(i=0; i<4; i++)
      {
        stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[i]);
        stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
      return(Err);
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
        for(i=0; i<4; i++)
        {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[i]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[i]);
        }
        stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
        stgfx_AVMEM_free(Handle, &WorkBufferBH);
            /* safe to free directly as not yet passed to STBLIT */
        return(Err);
      }

      Bitmap_p = &MaskBitmap;
    }

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

        MaxSpan = (S32)sqrt(((double)(Rectangle.Width)
                             *(double)(Rectangle.Width))
                             +(double)(Rectangle.Height)
                             *(double)(Rectangle.Height));
        MaxSpan = FRACT_TO_INT(MaxSpan);
        xMin = FRACT_TO_INT(Rectangle.PositionX) - MaxSpan
               - (GC_p->LineWidth/2);
        if(xMin<=0)
            xMin = 0;
        yMin = FRACT_TO_INT(Rectangle.PositionY) - MaxSpan
               - (GC_p->LineWidth/2);
        if(yMin<=0)
            yMin = 0;
        xMax = FRACT_TO_INT(Rectangle.PositionX) + MaxSpan
               + GC_p->LineWidth;
        yMax = FRACT_TO_INT(Rectangle.PositionY) + MaxSpan
               + GC_p->LineWidth;
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
  
#ifdef TRACE_ALLOC
  for(i=0; i<4; ++i)
  {
    TRACE_ALLOC(("Used %u elements StraightXYL[%i]\n",
                 StraightXYLNum[i], i));
  }
#endif

  /* remove overlaps if needed */
  if(GC_p->AluMode != STBLIT_ALU_COPY)
  {
    for(i=0; i<4; i++)
    {
      stgfx_RemoveXYLOverlaps(CornerIntXYL[i], &CornerIntXYLNum[i],
                              StraightXYL[i], &StraightXYLNum[i]);
      stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                              CornerIntXYL[(i==3) ? 0 : (i+1)],
                              &CornerIntXYLNum[(i==3) ? 0 : (i+1)]);
    }
    if(GC_p->LineWidth > 1)
    {
      for(i=0; i<4; i++)
      {
        stgfx_RemoveXYLOverlaps(CornerExtXYL[i], &CornerExtXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerExtXYL[(i==3) ? 0 : (i+1)],
                                &CornerExtXYLNum[(i==3) ? 0 : (i+1)]);
        
        stgfx_RemoveXYLOverlaps(CornerFillXYL[i], &CornerFillXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerFillXYL[(i==3) ? 0 : (i+1)],
                                &CornerFillXYLNum[(i==3) ? 0 : (i+1)]);
        
        stgfx_RemoveXYLOverlaps(CornerStartXYL[i], &CornerStartXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerEndXYL[(i==3) ? 0 : (i+1)],
                                &CornerEndXYLNum[(i==3) ? 0 : (i+1)]);
        
        stgfx_RemoveXYLOverlaps(CornerEndXYL[i], &CornerEndXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerStartXYL[(i==3) ? 0 : (i+1)],
                                &CornerStartXYLNum[(i==3) ? 0 : (i+1)]);
      }
      for(i=0; i<4; i++)
      {
        stgfx_RemoveXYLOverlaps(CornerExtXYL[i], &CornerExtXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerExtXYL[(i==3) ? 0 : (i+1)],
                                &CornerExtXYLNum[(i==3) ? 0 : (i+1)]);
        
        stgfx_RemoveXYLOverlaps(CornerFillXYL[i], &CornerFillXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerFillXYL[(i==3) ? 0 : (i+1)],
                                &CornerFillXYLNum[(i==3) ? 0 : (i+1)]);
        
        stgfx_RemoveXYLOverlaps(CornerStartXYL[i], &CornerStartXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerEndXYL[(i==3) ? 0 : (i+1)],
                                &CornerEndXYLNum[(i==3) ? 0 : (i+1)]);
        
        stgfx_RemoveXYLOverlaps(CornerEndXYL[i], &CornerEndXYLNum[i],
                                StraightXYL[i], &StraightXYLNum[i]);
        stgfx_RemoveXYLOverlaps(StraightXYL[i], &StraightXYLNum[i],
                                CornerStartXYL[(i==3) ? 0 : (i+1)],
                                &CornerStartXYLNum[(i==3) ? 0 : (i+1)]);
        
        stgfx_RemoveXYLOverlaps(CornerEndXYL[i], &CornerEndXYLNum[i],
                                CornerExtXYL[i], &CornerExtXYLNum[i]);
        stgfx_RemoveXYLOverlaps(CornerEndXYL[i], &CornerEndXYLNum[i],
                                CornerFillXYL[i], &CornerFillXYLNum[i]);
        stgfx_RemoveXYLOverlaps(CornerEndXYL[i], &CornerEndXYLNum[i],
                                CornerIntXYL[i], &CornerIntXYLNum[i]);

        stgfx_RemoveXYLOverlaps(CornerStartXYL[i], &CornerStartXYLNum[i],
                                CornerExtXYL[i], &CornerExtXYLNum[i]);
        stgfx_RemoveXYLOverlaps(CornerStartXYL[i], &CornerStartXYLNum[i],
                                CornerFillXYL[i], &CornerFillXYLNum[i]);
        stgfx_RemoveXYLOverlaps(CornerStartXYL[i], &CornerStartXYLNum[i],
                                CornerIntXYL[i], &CornerIntXYLNum[i]);
      }
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
    if (Err != ST_NO_ERROR)
    {
      for(i=0; i<4; i++)
      {
          stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[i]);
          stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[i]);
      }
      stgfx_AVMEM_free(Handle, &FillXYLBlockHandle);
      stgfx_AVMEM_free(Handle, &MaskDataBH);
      stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
          /* need to schedule as already in use by STBLIT */
      return(Err);
    }

    Bitmap_p = &MaskBitmap;
  }
  
  /* draw contour */
  for(i=0; i<4; i++)
  {
    /* straight */
    BlitContext.UserTag_p                 = (void*)StraightXYLBlockHandle[i];
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                  StraightXYLBlockHandle[i]));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, StraightXYL[i],
                              StraightXYLNum[i], &Color, &BlitContext);
    if(Err)
    {
      stgfx_AVMEM_free(Handle, &StraightXYLBlockHandle[i]);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }

    /* inner arcs */
    BlitContext.UserTag_p                 = (void*)CornerIntXYLBlockHandle[i];
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                  CornerIntXYLBlockHandle[i]));
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, CornerIntXYL[i],
                              CornerIntXYLNum[i], &Color, &BlitContext);
    if(Err)
    {
      stgfx_AVMEM_free(Handle, &CornerIntXYLBlockHandle[i]);
      LastErr = STGFX_ERROR_STBLIT_BLIT;
    }
    
    if(GC_p->LineWidth > 1)
    {
       /* ext arcs */
      BlitContext.UserTag_p              = (void*)CornerExtXYLBlockHandle[i];
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                    CornerExtXYLBlockHandle[i]));
      Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, CornerExtXYL[i],
                                CornerExtXYLNum[i], &Color, &BlitContext);
      if(Err)
      {
        stgfx_AVMEM_free(Handle, &CornerExtXYLBlockHandle[i]);
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }

      /* arcs fill */ 
      BlitContext.UserTag_p              = (void*)CornerFillXYLBlockHandle[i];
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                    CornerFillXYLBlockHandle[i]));
      Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, CornerFillXYL[i],
                                CornerFillXYLNum[i], &Color, &BlitContext);
      if(Err)
      {
        stgfx_AVMEM_free(Handle, &CornerFillXYLBlockHandle[i]);
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }
                                
      /* arcs start */
      BlitContext.UserTag_p              = (void*)CornerStartXYLBlockHandle[i];
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                    CornerStartXYLBlockHandle[i]));
      Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, CornerStartXYL[i],
                                CornerStartXYLNum[i], &Color, &BlitContext);
      if(Err)
      {
        stgfx_AVMEM_free(Handle, &CornerStartXYLBlockHandle[i]);
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }

      /* arcs end */
      BlitContext.UserTag_p              = (void*)CornerEndXYLBlockHandle[i];
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AVMEM blockHandle passed: %x\n",
                    CornerEndXYLBlockHandle[i]));
      Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, CornerEndXYL[i],
                                CornerEndXYLNum[i], &Color, &BlitContext);
      if(Err)
      {
        stgfx_AVMEM_free(Handle, &CornerEndXYLBlockHandle[i]);
        LastErr = STGFX_ERROR_STBLIT_BLIT;
      }
    }
  }
 
  /* if draw tiling is on */
  if(GC_p->DrawTexture_p != NULL)
  {
#if 0 /* old code used restricted span */
      S32 MaxSpan;
      S32 xMin, xMax, yMin, yMax;

      MaxSpan = (S32)sqrt(((double)(Rectangle.Width)
                           *(double)(Rectangle.Width))
                           +(double)(Rectangle.Height)
                           *(double)(Rectangle.Height));
      MaxSpan = FRACT_TO_INT(MaxSpan);
      xMin = FRACT_TO_INT(Rectangle.PositionX) - MaxSpan
             - (GC_p->LineWidth/2);
      if(xMin<=0)
          xMin = 0;
      yMin = FRACT_TO_INT(Rectangle.PositionY) - MaxSpan
             - (GC_p->LineWidth/2);
      if(yMin<=0)
          yMin = 0;
      xMax = FRACT_TO_INT(Rectangle.PositionX) + MaxSpan
             + GC_p->LineWidth;
      yMax = FRACT_TO_INT(Rectangle.PositionY) + MaxSpan
             + GC_p->LineWidth;
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
} /* end of STGFX_DrawRoundRectangle() */



/**************************************************************************\
Name        : RotoTranslation()
Description : Rotate the input point of the orientation angle 
              of the arc/ellipse
Parameters  : S32 Orientation, the ellipse rotation angle 
              S32* XRotated, the x coord of the returned point S
              S32* YRotated, the y coord of S
Assumptions : 
Limitations : 
Returns     : the coordinates of the point to be rotated
\****************************************************************************/
void RotoTranslation(
    S32 Orientation, 
    S32 RectangleOriginX,
    S32 RectangleOriginY,
    S32* XRT, 
    S32* YRT
)
{
   float X, Y;
   float XC, YC;
   float tmp;
   if((Orientation != 0) && (Orientation != (360*64)))
   {
     XC = (float)(RectangleOriginX /(float)(1<<STGFX_INTERNAL_ACCURACY));
     YC = (float)(RectangleOriginY /(float)(1<<STGFX_INTERNAL_ACCURACY));
     X = (float)(((float)*XRT - (float)RectangleOriginX)/
                (float)(1<<STGFX_INTERNAL_ACCURACY));
     Y = (float)(((float)*YRT - (float)RectangleOriginY)/
                (float)(1<<STGFX_INTERNAL_ACCURACY));
     tmp = X;
     X = (float)((double)tmp*cos(-TO_RAD(Orientation)) - 
         (double)(Y)*sin(-TO_RAD(Orientation)));
     Y = (float)((double)(Y)*cos(-TO_RAD(Orientation)) + 
         (double)tmp*sin(-TO_RAD(Orientation)));  
     *XRT = (S32)((X + XC) *(1<<STGFX_INTERNAL_ACCURACY));
     *YRT = (S32)((YC + Y) *(1<<STGFX_INTERNAL_ACCURACY));
   }
}
