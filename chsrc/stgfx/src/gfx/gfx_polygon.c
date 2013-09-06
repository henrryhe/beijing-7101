/*******************************************************************************
File Name   : gfx_polygon.c

Description : STGFX polygon functions, exported and private

COPYRIGHT (C) STMicroelectronics 2001.
*******************************************************************************/
/* Includes ----------------------------------------------------------------- */

/*#define STTBX_PRINT*/

/*#define DONEPAUSE*/
#ifdef DONEPAUSE
#define DonePause(DescrStr) \
  if(GC_p->LineWidth > 1) \
  { \
    STTBX_Print((DescrStr " done. Press a key...\n")); \
    getchar(); \
  }
#else
#define DonePause(DescrStr)
#endif

#include <string.h>
#include "sttbx.h"
#include <limits.h>

#include "gfx_tools.h" /* includes stgfx_init.h, ... */

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* maximum number of xyl avmem blocks stgfx_DrawPoly can produce:
    closed polygon: MAX_POINTS sides and joins, fill
    open polygon: MAX_POINTS-1 sides and joins, fill, 2 caps */
#define MAX_POLY_XYL_BLOCKS (1 + 2*STGFX_MAX_NUM_POLY_POINTS)

#define TAN_PREC  10 /* tangent precision, a safe shift on real co-ords */
#define TAN_UNITY (1<<TAN_PREC)

/* Private Macros    -------------------------------------------------------- */

#define ERR_PROP(Err, LastErr) \
    if(Err != ST_NO_ERROR) \
        LastErr = Err;

#define TAN_TO_INT(a) (((a) >= 0) ? ((a) >> TAN_PREC) : -(-(a) >> TAN_PREC))
        
/* Private Variables -------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Static Functions' bodies ------------------------------------------------- */

/* Functions (API) ---------------------------------------------------------- */

/*******************************************************************************
Name        : STGFX_DrawDot()
Description : draws a dot according to a specified graphics context
Parameters  : Handle,
              Bitmap_p,
              GC_p,
              X,
              Y
Assumptions : The target bitmap, the GC and the parameters are correct
Limitations : 
Returns     : error if a parameter is not correct
*******************************************************************************/
ST_ErrorCode_t STGFX_DrawDot(
    STGFX_Handle_t            Handle,
    STGXOBJ_Bitmap_t*         TargetBitmap_p,
    const STGFX_GC_t*         GC_p,
    S32                       X,
    S32                       Y
)
{
    ST_ErrorCode_t         Err; /* error code */
    ST_ErrorCode_t         LastErr = ST_NO_ERROR; /* latest non-fatal error */
    STBLIT_BlitContext_t   BlitContext; /* blitting context */
    STBLIT_Handle_t        BlitHandle; /* blit handle */
    S32                    Shift; /* shift value */
    U32                    Radius; /* radius: used if circular */
    S32                    XInt, YInt; /* integer values for X and Y */
    U32                    i; /* counter */

    /* Xyl buffers: AV blockhandles, dimensions, addresses */
    STBLIT_XYL_t*          BorderXYL_p = NULL;
    U32                    BorderXYLNum;
    STBLIT_XYL_t*          FillXYL_p = NULL;
    U32                    FillXYLNum;
    STAVMEM_BlockHandle_t  XYLBlockHandle;
    U32                    AllocXYLNum;

    /* tiling bits */
    STGXOBJ_Color_t        DrawColor = GC_p->DrawColor;
    STGXOBJ_Bitmap_t       MaskBitmap;
    STGXOBJ_Bitmap_t*      Bitmap_p = TargetBitmap_p;
    STAVMEM_BlockHandle_t  MaskDataBH, WorkBufferBH;


    /* check handle validity */
    if (NULL == stgfx_HandleToUnit(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    
    BlitHandle = stgfx_GetBlitterHandle(Handle);

    /* copy blit params to blitting context and validate GC_p, Bitmap_p */
    Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
    if (Err != ST_NO_ERROR)
    {
        return (Err);
    }

    /* parameters checking */
    if((GC_p->LineWidth == 0) ||
       (GC_p->CapStyle != STGFX_RECTANGLE_CAP &&
        GC_p->CapStyle != STGFX_PROJECTING_CAP &&
        GC_p->CapStyle != STGFX_ROUND_CAP))
    {
        return(STGFX_ERROR_INVALID_GC);
    }

    /* adjust fractional precision (why, when we convert to int so soon?) */
    Shift = STGFX_INTERNAL_ACCURACY - stgfx_GetUserAccuracy(Handle);
    if(Shift > 0 )
    {
        X = X << Shift;
        Y = Y << Shift;
    }
    else if (Shift < 0)
    {
        U8 Sh = -Shift;
        X = X / (1<<Sh);
        Y = Y / (1<<Sh);
    }
    /* if Shift == 0 do nothing */

    /* adjust position if pixel aspect ratio at init time is !=1 */
    if(GC_p->XAspectRatio != GC_p->YAspectRatio)
    {
        X = (X/GC_p->XAspectRatio)*GC_p->YAspectRatio;
    }

    if((!(GC_p->LineWidth & 0x1)) && (GC_p->CapStyle == STGFX_ROUND_CAP))
    {
        X = X - (1<<(STGFX_INTERNAL_ACCURACY-1));
        Y = Y - (1<<(STGFX_INTERNAL_ACCURACY-1));
        Radius = ((GC_p->LineWidth/2)<<STGFX_INTERNAL_ACCURACY) -
                 (1<<(STGFX_INTERNAL_ACCURACY-1));
    }
    else
    {
        Radius = (GC_p->LineWidth/2)<<STGFX_INTERNAL_ACCURACY;
    }

    XInt = FRACT_TO_INT(X);
    YInt = FRACT_TO_INT(Y);


    /* allocation for capstyle != ROUND */
    AllocXYLNum = BorderXYLNum = GC_p->LineWidth;

    /* filling allocation (only for ROUND capstyle) */
    if((GC_p->LineWidth > 1) && (GC_p->CapStyle == STGFX_ROUND_CAP))
    {
        BorderXYLNum = 2*(GC_p->LineWidth+1);
        FillXYLNum   = GC_p->LineWidth;
        AllocXYLNum  = BorderXYLNum + FillXYLNum;
    }
    
    Err = stgfx_AVMEM_malloc(Handle, AllocXYLNum*sizeof(STBLIT_XYL_t),
                             &XYLBlockHandle, (void**) &BorderXYL_p);
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    FillXYL_p = BorderXYL_p + BorderXYLNum;


    /* rendering */
    if(GC_p->LineWidth==1)
    {
        BorderXYL_p[0].PositionX = XInt;
        BorderXYL_p[0].PositionY = YInt;
        BorderXYL_p[0].Length = 1;
        BorderXYLNum = 1;
    }
    else /* lineWidth >1 */
    {
        if(GC_p->CapStyle == STGFX_RECTANGLE_CAP)
        {
            YInt = YInt - (GC_p->LineWidth/2);
            for(i=0; i<GC_p->LineWidth; i++)
            {
                BorderXYL_p[i].PositionX = XInt;
                BorderXYL_p[i].PositionY = YInt + i;
                BorderXYL_p[i].Length = 1;        
            }
            BorderXYLNum = GC_p->LineWidth;
        }
        else if(GC_p->CapStyle == STGFX_PROJECTING_CAP)
        {
            XInt = XInt - (GC_p->LineWidth/2);
            YInt = YInt - (GC_p->LineWidth/2);
            for(i=0; i<GC_p->LineWidth; i++)
            {
                BorderXYL_p[i].PositionX = XInt;
                BorderXYL_p[i].PositionY = YInt + i;
                BorderXYL_p[i].Length = GC_p->LineWidth;
            }
            BorderXYLNum = GC_p->LineWidth;
        }
        else /* STGFX_ROUND_CAP */
        {
            Err = stgfx_2dRasterSimpleEllipse(Bitmap_p->Height, X, Y,
                                              Radius, Radius, 0, TRUE,
                                              &BorderXYLNum, BorderXYL_p,
                                              &FillXYLNum, FillXYL_p);
            if(Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &XYLBlockHandle);
                return(Err);
            }
        }
    }
    /*******************************/
    /* rendering is now  completed */
    /*******************************/

    /* tiling analysis */
    if(GC_p->DrawTexture_p != NULL)
    {
        /* draw to mask and then use that to apply texture */
        Err = stgfx_InitTexture(Handle, GC_p, TargetBitmap_p, &MaskBitmap,
                                &BlitContext.WorkBuffer_p, &WorkBufferBH);

        if (Err == ST_NO_ERROR)
        {
            Err = stgfx_PrepareTexture(Handle, &MaskBitmap, &MaskDataBH,
                                       &BlitContext, &DrawColor);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &WorkBufferBH);
                    /* safe to free directly as not yet passed to STBLIT */
            }
        }

        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_free(Handle, &XYLBlockHandle);
            return(Err);
        }

        Bitmap_p = &MaskBitmap;
    }

    /* draw filling if ROUND capstyle with linewidth>1 */
    if((GC_p->LineWidth > 1) && (GC_p->CapStyle == STGFX_ROUND_CAP))
    {
        /* free the single avmem block with the border draw below */
        BlitContext.NotifyBlitCompletion = FALSE;
        Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, FillXYL_p,
                                  FillXYLNum, &DrawColor, &BlitContext);
        if(Err != ST_NO_ERROR)
        {
            LastErr = STGFX_ERROR_STBLIT_BLIT;
        }
        
        BlitContext.NotifyBlitCompletion = TRUE; /* restore for below */
    }
 
    /* draw 'border' (the only part if not ROUND capstyle) */
    BlitContext.UserTag_p = (void*)XYLBlockHandle;
    Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, BorderXYL_p,
                              BorderXYLNum, &DrawColor, &BlitContext);
    if(Err != ST_NO_ERROR)
    {
        /* schedule rather than direct incase fill is pending from above */
        stgfx_ScheduleAVMEM_free(Handle, &XYLBlockHandle);
        LastErr = STGFX_ERROR_STBLIT_BLIT;
    }

    if(GC_p->DrawTexture_p != NULL)
    {
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

  return (LastErr);
} /* end of STGFX_DrawDot() */


/*******************************************************************************
ST_ErrorCode_t STGFX_DrawRectangle()
*******************************************************************************/
ST_ErrorCode_t STGFX_DrawRectangle(
    STGFX_Handle_t      Handle,
    STGXOBJ_Bitmap_t*   Bitmap_p,
    const STGFX_GC_t*   GC_p,
    STGXOBJ_Rectangle_t Rectangle,
    S32                 Orientation
)
{
    ST_ErrorCode_t Err;
    STGFX_Point_t  Points_p[4];
    U32            NumPoints;
    stgfx_PolyMode PolyMode;
    double         Alpha = (PI_FLOAT*(double)Orientation)/PI; /* why double? */
    double         r;
    double         AlphaZero;

    if (NULL == stgfx_HandleToUnit(Handle))
    {
      return(ST_ERROR_INVALID_HANDLE);
    }
    
    /* Bitmap_p, GC_p validated by stgfx_DrawPoly */

    Orientation = Orientation%(2 * PI);
    if(Orientation<0)
        Orientation = Orientation+(2*PI);
    
    /* check parameters */
    if (Rectangle.Width == 0 || Rectangle.Height == 0)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    /* special axis-aligned case using STBLIT_FillRectangle. LineWidth > 1
      Rectangle join could be handled readilly with a few extra instructions,
      as could texturing. There is no fill if LineStyle != NULL */
    if((GC_p->LineWidth == 1) && (GC_p->LineStyle_p == NULL) &&
       (Orientation % (90*64) == 0) && (GC_p->DrawTexture_p == NULL) &&
       ((GC_p->FillTexture_p == NULL) || !(GC_p->EnableFilling)) )
    {
      STBLIT_BlitContext_t   BlitContext;
      STBLIT_Handle_t        BlitHandle;
      ST_ErrorCode_t         LastErr = ST_NO_ERROR;
      U32                    Acc = stgfx_GetUserAccuracy(Handle);

      /* don't care about cap or join styles with LineWidth 1 */

      /* get blitter handle */
      BlitHandle = stgfx_GetBlitterHandle(Handle);

      /* copy blit params to blitting context */
      Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
      if (Err != ST_NO_ERROR)
      {
        return(Err);
      }
      BlitContext.NotifyBlitCompletion = FALSE; /* no memory to free */

      /* aspect ratio, accuracy */
      if(Acc > 0)
      {
        /* go straight to device co-ords as it's all axis-aligned */
        Rectangle.PositionX >>= Acc;
        Rectangle.PositionY >>= Acc;
        Rectangle.Width >>= Acc;
        Rectangle.Height >>= Acc;
      }

      if(GC_p->XAspectRatio != GC_p->YAspectRatio)
      {
        /* this is copied from stgfx_DrawPoly - note it only scales x and
          divides before multiplying */
        Rectangle.PositionX = (Rectangle.PositionX/GC_p->XAspectRatio)*
                                  GC_p->YAspectRatio;
        Rectangle.Width = (Rectangle.Width/GC_p->XAspectRatio)*
                              GC_p->YAspectRatio;
      }
      
      if((Rectangle.Width == 0) || (Rectangle.Height == 0))
      {
        return LastErr; /* nothing to do */
      }

      /* rotate rectangle co-ords if needed */
      switch(Orientation)
      {
      case 90*64:
          GFXSWAP(Rectangle.Width, Rectangle.Height);
          Rectangle.PositionY -= Rectangle.Height;
          break;

      case 180*64:
          Rectangle.PositionX -= Rectangle.Width;
          Rectangle.PositionY -= Rectangle.Height;
          break;

      case 270*64:
          Rectangle.PositionX -= Rectangle.Width;
          break;
      }
      
      if((Rectangle.Width < 3) || (Rectangle.Height < 3))
      {
        /* it's all outline */
        
        return STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                    (STGXOBJ_Color_t*)&(GC_p->DrawColor),
                                    &BlitContext);
      }

      if(!(GC_p->EnableFilling) ||
         (GC_p->FillColor.Type != GC_p->DrawColor.Type) ||
         !stgfx_AreColorsEqual(&(GC_p->FillColor), &(GC_p->DrawColor)))
      {
        /* draw outline separately */

        S32 c; /* temporaries as we permute Rectangle to give 4 sides */
        U32 d;

        /* one XYL operation might be quicker for top and bottom, but not
          when we move to LineWidth > 1 */

        /* top */
        d = Rectangle.Height;
        Rectangle.Height = 1;
        Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                   (STGXOBJ_Color_t*)&(GC_p->DrawColor),
                                   &BlitContext);

        if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;

        /* bottom */
        c = Rectangle.PositionY;
        Rectangle.PositionY += d-1;
        Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                   (STGXOBJ_Color_t*)&(GC_p->DrawColor),
                                   &BlitContext);

        if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;

        /* Restore slightly reduced so we don't write the corners twice.
          These are also the y values needed for the fill if used */
        Rectangle.PositionY = c+1;
        Rectangle.Height = d-2;

        /* left */
        d = Rectangle.Width;
        Rectangle.Width = 1;
        Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                   (STGXOBJ_Color_t*)&(GC_p->DrawColor),
                                   &BlitContext);

        if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;

        /* right */
        c = Rectangle.PositionX;
        Rectangle.PositionX += d-1;
        Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                   (STGXOBJ_Color_t*)&(GC_p->DrawColor),
                                   &BlitContext);

        if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;

        /* restore reduced to fill rect */
        Rectangle.PositionX = c+1;
        Rectangle.Width = d-2;
      }

      if(GC_p->EnableFilling)
      {
        Err = STBLIT_FillRectangle(BlitHandle, Bitmap_p, &Rectangle,
                                   (STGXOBJ_Color_t*)&(GC_p->FillColor),
                                   &BlitContext);

        if(Err != ST_NO_ERROR) LastErr = STGFX_ERROR_STBLIT_BLIT;
      }

      return LastErr;
    }
    
    /* general version */
    Points_p[0].X = Rectangle.PositionX;
    Points_p[0].Y = Rectangle.PositionY;
    
    /* degenerate rectangles */
    if (Rectangle.Width == 1 && Rectangle.Height == 1)
    {
       NumPoints = 1;
       PolyMode = Open;
    }
    else
    {
        switch (Orientation)
        {
            case (0): /* Orientation = 0 */
                if (Rectangle.Width == 1)
                {
                    Points_p[1].X = Rectangle.PositionX;
                    Points_p[1].Y = Rectangle.PositionY + Rectangle.Height - 1;
                    NumPoints = 2;
                    PolyMode = Open;
                } 
                else if(Rectangle.Height == 1)
                {
                    Points_p[1].X = Rectangle.PositionX + Rectangle.Width - 1;
                    Points_p[1].Y = Rectangle.PositionY;
                    NumPoints = 2;
                    PolyMode = Open;
                }
                else
                {
                    Points_p[1].X = Rectangle.PositionX;
                    Points_p[1].Y = Rectangle.PositionY + Rectangle.Height - 1;
                    Points_p[2].X = Rectangle.PositionX + Rectangle.Width  - 1;
                    Points_p[2].Y = Rectangle.PositionY + Rectangle.Height - 1;
                    Points_p[3].X = Rectangle.PositionX + Rectangle.Width  - 1;
                    Points_p[3].Y = Rectangle.PositionY;
                    NumPoints = 4;
                    PolyMode = Close;
                }
                break;
            
            case (5760): /* Orientation = 90 */
                if (Rectangle.Width == 1)
                {
                    Points_p[1].X = Rectangle.PositionX + Rectangle.Height - 1;
                    Points_p[1].Y = Rectangle.PositionY;
                    NumPoints = 2;
                    PolyMode = Open;
                } 
                else if(Rectangle.Height == 1)
                {
                    Points_p[1].X = Rectangle.PositionX;
                    Points_p[1].Y = Rectangle.PositionY - Rectangle.Width - 1;
                    NumPoints = 2;
                    PolyMode = Open;
                }
                else
                {
                    Points_p[1].X = Rectangle.PositionX + Rectangle.Height - 1;
                    Points_p[1].Y = Rectangle.PositionY;
                    Points_p[2].X = Rectangle.PositionX + Rectangle.Height - 1;
                    Points_p[2].Y = Rectangle.PositionY - Rectangle.Width -  1;
                    Points_p[3].X = Rectangle.PositionX;
                    Points_p[3].Y = Rectangle.PositionY - Rectangle.Width -  1;
                    NumPoints = 4;
                    PolyMode = Close;
                }
                break;

            case (11520): /* Orientation = 180 */
                if (Rectangle.Width == 1)
                {
                    Points_p[1].X = Rectangle.PositionX;
                    Points_p[1].Y = Rectangle.PositionY - Rectangle.Height - 1;
                    NumPoints = 2;
                    PolyMode = Open;
                } 
                else if(Rectangle.Height == 1)
                {
                    Points_p[1].X = Rectangle.PositionX - Rectangle.Width - 1;
                    Points_p[1].Y = Rectangle.PositionY;
                    NumPoints = 2;
                    PolyMode = Open;
                }
                else
                {
                    Points_p[1].X = Rectangle.PositionX;
                    Points_p[1].Y = Rectangle.PositionY - Rectangle.Height - 1;
                    Points_p[2].X = Rectangle.PositionX - Rectangle.Width  - 1;
                    Points_p[2].Y = Rectangle.PositionY - Rectangle.Height - 1;
                    Points_p[3].X = Rectangle.PositionX - Rectangle.Width  - 1;
                    Points_p[3].Y = Rectangle.PositionY;
                    NumPoints = 4;
                    PolyMode = Close;
                }
                break;
            
            case (17280): /* Orientation = 270 */
                if (Rectangle.Width == 1)
                {
                    Points_p[1].X = Rectangle.PositionX - Rectangle.Height - 1;
                    Points_p[1].Y = Rectangle.PositionY;
                    NumPoints = 2;
                    PolyMode = Open;
                } 
                else if(Rectangle.Height == 1)
                {
                    Points_p[1].X = Rectangle.PositionX;
                    Points_p[1].Y = Rectangle.PositionY + Rectangle.Width - 1;
                    NumPoints = 2;
                    PolyMode = Open;
                }
                else
                {
                    Points_p[1].X = Rectangle.PositionX - Rectangle.Height - 1;
                    Points_p[1].Y = Rectangle.PositionY;
                    Points_p[2].X = Rectangle.PositionX - Rectangle.Height - 1;
                    Points_p[2].Y = Rectangle.PositionY + Rectangle.Width -  1;
                    Points_p[3].X = Rectangle.PositionX;
                    Points_p[3].Y = Rectangle.PositionY + Rectangle.Width -  1;
                    NumPoints = 4;
                    PolyMode = Close;
                }
                break;
            
            default:
                if (Rectangle.Width == 1)
                {
                    r = (double)(U32)(Rectangle.Height-1);
                    AlphaZero = atan((double)(Rectangle.Height));
                    Points_p[1].X = Rectangle.PositionX + 
                                    (S32)(r*cos(AlphaZero-Alpha));
                    Points_p[1].Y = Rectangle.PositionY + 
                                    (S32)(r*sin(AlphaZero-Alpha));
                    NumPoints = 2;
                    PolyMode = Open;
                }
                else if (Rectangle.Height == 1)
                {
                    Points_p[1].X = Rectangle.PositionX +
                        (S32)((double)(U32)(Rectangle.Width-1)*cos(-Alpha));
                    Points_p[1].Y = Rectangle.PositionY +
                        (S32)((double)(U32)(Rectangle.Width-1)*sin(-Alpha));
                    NumPoints = 2;
                    PolyMode = Open;
                }
                else
                {
                    r = (double)(U32)(Rectangle.Height-1);
                    AlphaZero = atan((double)(Rectangle.Height));
                    Points_p[1].X = Rectangle.PositionX + 
                                    (S32)(r*cos(AlphaZero-Alpha));
                    Points_p[1].Y = Rectangle.PositionY + 
                                    (S32)(r*sin(AlphaZero-Alpha));
                    r = sqrt((double)(U32)(Rectangle.Width-1)*
                             (double)(U32)(Rectangle.Width-1) +
                             (double)(U32)(Rectangle.Height-1)*
                             (double)(U32)(Rectangle.Height-1));
                    AlphaZero = atan((double)(Rectangle.Height)/
                                     (double)(Rectangle.Width));
                    Points_p[2].X = Rectangle.PositionX + 
                                    (S32)(r*cos(AlphaZero-Alpha));
                    Points_p[2].Y = Rectangle.PositionY + 
                                    (S32)(r*sin(AlphaZero-Alpha));
                    Points_p[3].X = Rectangle.PositionX +
                        (S32)((double)(U32)(Rectangle.Width-1)*cos(-Alpha));
                    Points_p[3].Y = Rectangle.PositionY +
                        (S32)((double)(U32)(Rectangle.Width-1)*sin(-Alpha));
                    NumPoints = 4;
                    PolyMode = Close;
                }
        }
    }

#if 0    
    STTBX_Print(("STGFX_DrawRectangle: call stgfx_DrawPoly"));
    {
      int i;
      
      for (i = 0; i < NumPoints; ++i)
        STTBX_Print((" (%u, %u)", Points_p[i].X, Points_p[i].Y));
    
      STTBX_Print(("\n"));
    }
#endif

    Err = stgfx_DrawPoly(Handle, Bitmap_p, GC_p, NumPoints, Points_p, PolyMode);
        /* includes user accuracy->internal accuracy conversion */

    return(Err);
  
} /* end of STGFX_DrawRectangle() */


/*******************************************************************************
ST_ErrorCode_t STGFX_DrawPolygon()
*******************************************************************************/
ST_ErrorCode_t STGFX_DrawPolygon(
  STGFX_Handle_t            Handle,
  STGXOBJ_Bitmap_t*         Bitmap_p,
  const STGFX_GC_t*         GC_p,
  U32                       NumPoints,
  STGFX_Point_t*            Points_p
)
{
  ST_ErrorCode_t          Err;         /* Error code */
  stgfx_PolyMode          PolyMode;

  if (NULL == stgfx_HandleToUnit(Handle))
  {
    return(ST_ERROR_INVALID_HANDLE);
  }

  PolyMode = Close;

  Err = stgfx_DrawPoly(Handle, Bitmap_p, GC_p, NumPoints, Points_p, PolyMode);

  return(Err);
}


/******************************************************************************
Function Name : stgfx_IsPolygonConvex
  Description : Determine if a polygon with the supplied points is convex in y,
                meaning that it involves a maximum of one span and two 
                non-horizontal sides per y
   Parameters : Points array
******************************************************************************/
BOOL stgfx_IsPolygonConvex(U32 NumPoints, STGFX_Point_t* Points_p)
{
    STGFX_Point_t *p, *PointsLimit;
    U32 yDirCount = 0;
    S32 yPrev, yDir, yDirPrev = 0, yDirFirst = 0;
    
    /* we detect non-convexness as more than two changes of y direction */

    /* now is a good time to take account of the last-first transition */    
    yPrev = Points_p[NumPoints-1].Y;

    PointsLimit = Points_p + NumPoints;
    for(p = Points_p; (p < PointsLimit); ++p)
    {
        yDir = GFXSGN(p->Y - yPrev);
        yPrev = p->Y;
        
        if((yDir != 0) && (yDir != yDirPrev))
        {
            yDirPrev = yDir;
            
            if(yDirFirst == 0)
                yDirFirst = yDir;
            else
                ++yDirCount;
        }
    }
    
    if(yDir != yDirFirst)
        ++yDirCount;
        
    return (yDirCount <= 2);
}


/******************************************************************************
Function Name : stgfx_GetTopBotPoint
  Description : Obtains the index positions of the top and bottom entries in
                an STGFX_Point_t array
   Parameters : Points array, must have at least one element
******************************************************************************/
void stgfx_GetTopBotPoint(U32 NumPoints, STGFX_Point_t* Points_p,
                          U32* TopIdx_p, U32* BotIdx_p)
{
    STGFX_Point_t *p, *PointsLimit;
    S32 yMin, yMax;
    U32 yMinIdx, yMaxIdx;
    
    yMin = yMax = Points_p->Y;
    yMinIdx = yMaxIdx = 0;
    
    PointsLimit = Points_p + NumPoints;
    for(p = Points_p+1; (p < PointsLimit); ++p)
    {
        if(p->Y < yMin)
        {
            yMin = p->Y;
            yMinIdx = p - Points_p;
        }
        else if(p->Y > yMax)
        {
            yMax = p->Y;
            yMaxIdx = p - Points_p;
        }
    }
    
    *TopIdx_p = yMinIdx;
    *BotIdx_p = yMaxIdx;
}


/******************************************************************************
Function Name : stgfx_CalcSolidConvexPolygon
  Description : Calculate the spans for a solid polygon known to be y-convex.
                Always produces 1+yBot-yTop elements
   Parameters : Points array, xyl array, index of starting point (minimum y)
                Should cope with all NumPoints >= 1
******************************************************************************/
void stgfx_CalcSolidConvexPolygon(U32 NumPoints, STGFX_Point_t* Points_p,
                                  STBLIT_XYL_t* XYLArray, U32 yMinIdx)
{
    U32 PointsLeft;
    U32 Idx1, Idx2; /* points we're currently rastering towards */
    S32 Inc1, Inc2, x1Frac, x2Frac; /* fractional co-ordinates at TAN_PREC */
    S32 x1Int, x2Int, xPrev, xLeft, xRight, y; /* integer co-ordinates */
    STBLIT_XYL_t* XYL_p = XYLArray;
    
    y = Points_p[yMinIdx].Y;
    Idx1 = Idx2 = yMinIdx;
    
    x1Int = x2Int = Points_p[yMinIdx].X;
    
    /* most points will be selected once as a new target,
      the first never but the last twice */
    PointsLeft = NumPoints;
    
    do
    {
        /* initialise xLeft, xRight */
        xLeft = INT_MAX;
        xRight = INT_MIN;
        
        /* detect line 1 end */
        if(y == Points_p[Idx1].Y)
        {
            /* ensure line is exactly completed */
            xPrev = x1Int;
            x1Int = Points_p[Idx1].X;
            
            if(Inc1 >= 0)
            {
                if(xPrev != x1Int) ++xPrev;
                if(xPrev < xLeft) xLeft = xPrev;
                if(x1Int > xRight) xRight = x1Int;
            }
            else
            {
                if(xPrev != x1Int) --xPrev;
                if(x1Int < xLeft) xLeft = x1Int;
                if(xPrev > xRight) xRight = xPrev;
            }
            
            while(PointsLeft)
            {
                xPrev = x1Int;
            
                if(Idx1 == 0)
                    Idx1 = NumPoints-1;
                else
                    --Idx1;
                
                --PointsLeft;
                if(y != Points_p[Idx1].Y)
                {
                    /* start non-trivial line */
                    x1Frac = xPrev << TAN_PREC;
                    Inc1 = ((Points_p[Idx1].X - xPrev) << TAN_PREC)
                                 / (Points_p[Idx1].Y - y);

                    /* add half an increment less half a pixel */
                    if(Inc1 > TAN_UNITY/2)
                    {
                        x1Frac += (Inc1 - TAN_UNITY) / 2;
                    }
                    else if(Inc1 < -TAN_UNITY/2)
                    {
                        x1Frac += (Inc1 + TAN_UNITY) / 2;
                    }

                    x1Int = TAN_TO_INT(x1Frac);
                    if(x1Int < xLeft) xLeft = x1Int;
                    if(x1Int > xRight) xRight = x1Int;
                    /* only actually need one of these after earlier work */
                    
                    break;
                }
                
                /* process horizontal line */
                x1Int = Points_p[Idx1].X;
                if(x1Int < xLeft) xLeft = x1Int;
                if(x1Int > xRight) xRight = x1Int;
            }
            /* we drop out via the loop condition
              if the last segment is horizontal */
        }
        else
        {
            xPrev = x1Int;
        
            /* advance line 1 for new y */
            x1Frac += Inc1;

            x1Int = TAN_TO_INT(x1Frac);
            
            /* xPrev will track the inside edge of this line, x the outside */
            if(Inc1 >= 0)
            {
                if(xPrev != x1Int) ++xPrev;
                if(xPrev < xLeft) xLeft = xPrev;
                if(x1Int > xRight) xRight = x1Int;
            }
            else
            {
                if(xPrev != x1Int) --xPrev;
                if(x1Int < xLeft) xLeft = x1Int;
                if(xPrev > xRight) xRight = xPrev;
            }
        }
        
        /* detect line 2 end (replace 1/2 and direction of Idx change) */
        if(y == Points_p[Idx2].Y)
        {
            /* ensure line is exactly completed */
            xPrev = x2Int;
            x2Int = Points_p[Idx2].X;
            
            /* xPrev will track the inside edge of this line, x the outside */
            if(Inc2 >= 0)
            {
                if(xPrev != x2Int) ++xPrev;
                if(xPrev < xLeft) xLeft = xPrev;
                if(x2Int > xRight) xRight = x2Int;
            }
            else
            {
                if(xPrev != x2Int) --xPrev;
                if(x2Int < xLeft) xLeft = x2Int;
                if(xPrev > xRight) xRight = xPrev;
            }

            while(PointsLeft)
            {
                xPrev = x2Int;
            
                if(Idx2 == NumPoints-1)
                    Idx2 = 0;
                else
                    ++Idx2;
                
                --PointsLeft;
                if(y != Points_p[Idx2].Y)
                {
                    /* start non-trivial line */
                    x2Frac = xPrev << TAN_PREC;
                    Inc2 = ((Points_p[Idx2].X - xPrev) << TAN_PREC)
                                 / (Points_p[Idx2].Y - y);

                    /* add half an increment less half a pixel */
                    if(Inc2 > TAN_UNITY/2)
                    {
                        x2Frac += (Inc2 - TAN_UNITY) / 2;
                    }
                    else if(Inc2 < -TAN_UNITY/2)
                    {
                        x2Frac += (Inc2 + TAN_UNITY) / 2;
                    }

                    x2Int = TAN_TO_INT(x2Frac);
                    if(x2Int < xLeft) xLeft = x2Int;
                    if(x2Int > xRight) xRight = x2Int;
                    
                    break;
                }

                /* process horizontal line */
                x2Int = Points_p[Idx2].X;
                if(x2Int < xLeft) xLeft = x2Int;
                if(x2Int > xRight) xRight = x2Int;
            }
        }
        else
        {
            xPrev = x2Int;

            /* advance line 2 for new y */
            x2Frac += Inc2;

            x2Int = TAN_TO_INT(x2Frac);
            
            /* xPrev will track the LH edge of this line, x the RH edge */
            if(Inc2 >= 0)
            {
                if(xPrev != x2Int) ++xPrev;
                if(xPrev < xLeft) xLeft = xPrev;
                if(x2Int > xRight) xRight = x2Int;
            }
            else
            {
                if(xPrev != x2Int) --xPrev;
                if(x2Int < xLeft) xLeft = x2Int;
                if(xPrev > xRight) xRight = xPrev;
            }
        }
        
        /* emit XYL */
        XYL_p->PositionX = xLeft;
        XYL_p->PositionY = y;
        XYL_p->Length = 1 + xRight - xLeft;
        ++XYL_p;
        ++y;
    }
    while(PointsLeft || (y <= Points_p[Idx1].Y));
    /* drop out when we've actually done the final point */
}


/*******************************************************************************
Name        : stgfx_DrawPoly()
Description : this function is used to render any 2d graphic object that is
              made of a set of lines (lines, polyline, polygons, rectangles).
              It includes checks against NULL arguments, but not invalid handle
Parameters  : Handle:    Handle of the connection
              Bitmap_p:  pointer of the bitmap
              GC_p :     pointer of graphic context
              NumPoints:
              Points_p:  array of points
              PolyMode:  flag that indicates if 2d object is open or closed
Assumptions : Valid Handle & PolyMode
Limitations : 
Returns     : ST_NO_ERROR, STGFX_ERROR_INVALID_GC, ST_ERROR_BAD_PARAMETER,
              STGFX_ERROR_XYL_POOL_EXCEEDED, ST_ERROR_NO_MEMORY
*******************************************************************************/
ST_ErrorCode_t stgfx_DrawPoly(
  STGFX_Handle_t    Handle,
  STGXOBJ_Bitmap_t* TargetBitmap_p,
  const STGFX_GC_t* GC_p,
  U32               NumPoints,
  STGFX_Point_t*    Points_p,
  stgfx_PolyMode    PolyMode
)
{
    /* LastErr stores the most recent non-fatal error */
    ST_ErrorCode_t         Err, LastErr = ST_NO_ERROR;
    STBLIT_BlitContext_t   BlitContext;
    STBLIT_Handle_t        BlitHandle;
    STBLIT_XYL_t*          XYLCap_p[2];
    U32                    XYLCapNum[2];
    STAVMEM_BlockHandle_t  XYLCapBH[2];
    S32                    i, j;
    S32                    X1, Y1, X2, Y2;
    S32                    X1join, Y1join, X2join, Y2join, X3join, Y3join;
    S32                    Shift;
    stgfx_Side_t*          sides;
    STAVMEM_BlockHandle_t  XYLSideBH[STGFX_MAX_NUM_POLY_POINTS];
    U32                    SideAllocNum[STGFX_MAX_NUM_POLY_POINTS];
    stgfx_Side_t*          joins;
    STAVMEM_BlockHandle_t  XYLJoinBH[STGFX_MAX_NUM_POLY_POINTS];
    const stgfx_JoinSector JoinSector = Lines;
    STBLIT_XYL_t*          XYLFill_p;
    U32                    XYLFillNum;
    STAVMEM_BlockHandle_t  XYLFillBH;
    U8                     Spin;
    S32                    yTop, yBot;  /* top and bottom Y coordinates */
    U32                    NumP = 0, NumSides, NumJoins;
    STBLIT_XYL_t*          tmpXYL_p;
    STAVMEM_BlockHandle_t  tmpXYLBlockHandleH;
    S32                    Y[STGFX_MAX_NUM_POLY_POINTS];
    STGFX_Point_t          TmpPoints_p[STGFX_MAX_NUM_POLY_POINTS];
    U32                    NumXYLBlocks = 0;
    STAVMEM_BlockHandle_t  XYLAllocated[MAX_POLY_XYL_BLOCKS];
    BOOL                   HasCaps;

    /* tiling items */
    STGXOBJ_Color_t        Color;
    STGXOBJ_Bitmap_t       MaskBitmap;
    STGXOBJ_Bitmap_t*      Bitmap_p = TargetBitmap_p;
    STAVMEM_BlockHandle_t  MaskDataBH;
    STAVMEM_BlockHandle_t  WorkBufferBH = STAVMEM_INVALID_BLOCK_HANDLE;


    TIME_HERE("STGFX_DrawPoly entry");
    
    /* could let InitBlitContext do the first two checks if we move things */
    if((TargetBitmap_p == NULL) || (GC_p == NULL) || (Points_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    /* parameters checking */
    if ((GC_p->LineWidth == 0) ||
        (GC_p->CapStyle != STGFX_RECTANGLE_CAP &&
         GC_p->CapStyle != STGFX_PROJECTING_CAP &&
         GC_p->CapStyle != STGFX_ROUND_CAP) ||
        (GC_p->JoinStyle != STGFX_RECTANGLE_JOIN &&
         GC_p->JoinStyle != STGFX_FLAT_JOIN      &&
         GC_p->JoinStyle != STGFX_ROUNDED_JOIN))
    {
        return(STGFX_ERROR_INVALID_GC);
    }
    
    if (NumPoints == 0 || NumPoints > STGFX_MAX_NUM_POLY_POINTS)
        return(ST_ERROR_BAD_PARAMETER);

    if (GC_p->LineWidth > STGFX_MAX_LINE_WIDTH)
        return(ST_ERROR_BAD_PARAMETER);

    /* creates a copy of the Points vector removing any consecutive points */
    for(i=0, j=0; j < NumPoints; ++i)
    {
        TmpPoints_p[i] = Points_p[j++];
        while ((j < NumPoints) && (TmpPoints_p[i].X == Points_p[j].X)
                               && (TmpPoints_p[i].Y == Points_p[j].Y))
        {
          j++;
        }
    }
    /* -> i: number of unique points found */

    NumPoints = i;
    if ((PolyMode == Close) && (NumPoints > 1) &&
        (TmpPoints_p[0].X == TmpPoints_p[NumPoints-1].X) &&
        (TmpPoints_p[0].Y == TmpPoints_p[NumPoints-1].Y))
    {
        /* first and last point are the same - only need one */
        NumPoints--;
    }
       
    /* degenerate polygons */
    if (NumPoints == 1) /* degenerate polygon is a point */
    {
        return(STGFX_DrawDot(Handle, Bitmap_p, GC_p, TmpPoints_p[0].X,
                                                     TmpPoints_p[0].Y));
    }
    
    /* get blitter handle */
    BlitHandle = stgfx_GetBlitterHandle(Handle);
    
    /* copy blit params to blitting context */
    Err = stgfx_InitBlitContext(Handle, &BlitContext, GC_p, Bitmap_p);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }
    
    Shift = STGFX_INTERNAL_ACCURACY - stgfx_GetUserAccuracy(Handle);
    if (Shift > 0 )
    {
        for (i = 0; i < NumPoints; i++)
        {
            TmpPoints_p[i].X = TmpPoints_p[i].X << Shift;
            TmpPoints_p[i].Y = TmpPoints_p[i].Y << Shift;
        }
    }
    else if (Shift < 0)
    {
        U8 Sh = -Shift;
        for (i = 0; i < NumPoints; i++)
        {
            TmpPoints_p[i].X = (TmpPoints_p[i].X + 
                           (1<<(Sh-1))*((TmpPoints_p[i].X>0) ? 1 : -1))/(1<<Sh);
            TmpPoints_p[i].Y = (TmpPoints_p[i].Y + 
                           (1<<(Sh-1))*((TmpPoints_p[i].Y>0) ? 1 : -1))/(1<<Sh);
        }
    } 
    
    /* Aspect ratio geometric correction (divide first ?) */
    if(GC_p->XAspectRatio != GC_p->YAspectRatio)
    {
      for(i=0; i <NumPoints; i++)
        TmpPoints_p[i].X = (TmpPoints_p[i].X/GC_p->XAspectRatio)*
                                GC_p->YAspectRatio;
    }

    if ((PolyMode == Close) && (NumPoints == 2))
    {
        /* degenerates to a simple line */
        PolyMode = Open;
    }
    
    if((GC_p->LineWidth == 1) && (GC_p->LineStyle_p == NULL) &&
       (PolyMode == Close) && stgfx_IsPolygonConvex(NumPoints, TmpPoints_p) &&
       (GC_p->DrawTexture_p == NULL) && (GC_p->FillTexture_p == NULL) &&
       GC_p->EnableFilling && (GC_p->FillColor.Type == GC_p->DrawColor.Type) &&
       stgfx_AreColorsEqual(&(GC_p->FillColor), &(GC_p->DrawColor)))
    {
        U32 yTopIdx, yBotIdx;
    
        /* drop fractional part for interface to optimised routine */
        for (i = 0; i < NumPoints; i++)
        {
            TmpPoints_p[i].X = FRACT_TO_INT(TmpPoints_p[i].X);
            TmpPoints_p[i].Y = FRACT_TO_INT(TmpPoints_p[i].Y);
        }

        stgfx_GetTopBotPoint(NumPoints, TmpPoints_p, &yTopIdx, &yBotIdx);
    
        XYLFillNum = 1 + TmpPoints_p[yBotIdx].Y - TmpPoints_p[yTopIdx].Y;
        Err = stgfx_AVMEM_malloc(Handle, XYLFillNum*sizeof(STBLIT_XYL_t),
                                 &XYLFillBH, (void**) &XYLFill_p);
        if (Err != ST_NO_ERROR)
        {
            return Err;
        }
        
        TIME_HERE("Optimised polygon analysis and memory allocation done");
        
        stgfx_CalcSolidConvexPolygon(NumPoints, TmpPoints_p,
                                     XYLFill_p, yTopIdx);
        
        TIME_HERE("Optimised polygon xyl calculation done");
        
        BlitContext.UserTag_p = (void*)XYLFillBH;
        Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, XYLFill_p, XYLFillNum,
                                  (STGXOBJ_Color_t*) &GC_p->DrawColor,
                                  &BlitContext);
        DonePause("XYLFill_p");
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_free(Handle, &XYLFillBH);
            LastErr = STGFX_ERROR_STBLIT_BLIT;
        }
        
        TIME_HERE("Optimised polygon drawing done");
        return LastErr;
    }
    
    if (PolyMode == Open)
    {
        NumSides = NumPoints - 1;
        NumJoins = NumPoints - 2;
    }
    else /* (PolyMode == Close) */
    {
        NumSides = NumPoints;
        NumJoins = NumPoints;
        NumPoints++; /* count the first point twice */
    }
    
    if(GC_p->LineWidth == 1)
    {
        /* no joins to raster for linewidth 1 */
        NumJoins = 0;
    }
    
    /* sides and joins could almost be allocated on the stack using
      STGFX_MAX_NUM_POLY_POINTS, but would take ~600 bytes. Allocate
      them in one block, anyway, to ease later freeing */
      
    sides = stgfx_DEVMEM_malloc(Handle,
                                sizeof(stgfx_Side_t) * (NumSides+NumJoins));
    if (sides == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    joins = sides + NumSides;
    
    /* compute joins */
    if (GC_p->LineWidth > 1 && NumSides > 1)
    {
        for (i = 0; i < NumJoins; i++)
        {
            X1join = TmpPoints_p[i].X;   Y1join = TmpPoints_p[i].Y;
            X2join = TmpPoints_p[i+1].X; Y2join = TmpPoints_p[i+1].Y;
            X3join = TmpPoints_p[i+2].X; Y3join = TmpPoints_p[i+2].Y;
            if (PolyMode == Close && i == NumJoins - 2)
            {
                X3join = TmpPoints_p[0].X; Y3join = TmpPoints_p[0].Y;
            }
            if (PolyMode == Close && i == NumJoins - 1)
            {
                X1join = TmpPoints_p[NumPoints-2].X;
                Y1join = TmpPoints_p[NumPoints-2].Y;
                X2join = TmpPoints_p[0].X; 
                Y2join = TmpPoints_p[0].Y;
                X3join = TmpPoints_p[1].X; 
                Y3join = TmpPoints_p[1].Y;
            }

            Err = stgfx_2dRasterJoin(Handle, X1join, Y1join, X2join, Y2join,
                                     X3join, Y3join, 0, 0, 0, 0, JoinSector, 
                                     GC_p->LineWidth, GC_p->JoinStyle, 
                                     GC_p->LineStyle_p, &XYLJoinBH[i],
                                     &joins[i].xylData, &joins[i].length);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
                stgfx_DEVMEM_free(Handle, sides); 
                STTBX_Print(("stgfx_DrawPoly->stgfx_2dRasterJoin error 0x%08X\n", Err));
                return(Err);
            }
            XYLAllocated[NumXYLBlocks++] = XYLJoinBH[i];
        }
    }

    TIME_HERE("STGFX_DrawPoly before sides raster");
    
    /* sides rendering */
    for (i = 0; i < (NumPoints - 1); i++)
    {  
        X1 = TmpPoints_p[i].X;   Y1 = TmpPoints_p[i].Y;
        
        if (PolyMode == Close && i == NumPoints - 2)
        {
            X2 = TmpPoints_p[0].X; Y2 = TmpPoints_p[0].Y;
        }
        else
        {
            X2 = TmpPoints_p[i+1].X; Y2 = TmpPoints_p[i+1].Y;
        }
        
        if (X2 < X1)
        {
            GFXSWAP(X1, X2);
            GFXSWAP(Y1, Y2);
        }

        sides[i].length = GFXABS(FRACT_TO_INT(Y2)-FRACT_TO_INT(Y1)) + 1;
        
        if (GC_p->LineWidth == 1 && GC_p->LineStyle_p != NULL)
        {
            sides[i].length += GFXABS(FRACT_TO_INT(X2)-FRACT_TO_INT(X1));
        }
        else if (GC_p->LineWidth > 1)
        {
            if (GC_p->LineStyle_p == NULL)
            {
                sides[i].length += 2 * GC_p->LineWidth;
                if (NumPoints > 2)
                {
                    if (i == 0)
                    {
                        sides[i].length += joins[i].length;
                        if (PolyMode == Close)
                            sides[i].length += joins[NumJoins-1].length;
                    }
                    else if (i == NumPoints - 2)
                    {
                        sides[i].length += joins[i-1].length;
                        if (PolyMode == Close)
                            sides[i].length += joins[i].length;
                    }
                    else if (i > 0 && i < NumPoints - 2)
                    {
                        sides[i].length += joins[i-1].length + joins[i].length;
                    }
                }
            }
            else if (GC_p->LineStyle_p != NULL)
            {
                /* This call includes check that each segment has Length > 1 */
                Err = stgfx_CalcLimitDashedLine(X1, Y1, X2, Y2,
                                                GC_p->LineStyle_p,
                                                GC_p->LineWidth,
                                                &sides[i].length);
                if (Err != ST_NO_ERROR)
                {
                    stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
                    stgfx_DEVMEM_free(Handle, sides); 
                    return(Err);
                }
            }
        }
        
        Err = stgfx_AVMEM_malloc(Handle, sides[i].length*sizeof(STBLIT_XYL_t),
                                 &XYLSideBH[i], (void**) &sides[i].xylData);
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
            stgfx_DEVMEM_free(Handle, sides);
            return(Err);
        }
        XYLAllocated[NumXYLBlocks++] = XYLSideBH[i];
        SideAllocNum[i] = sides[i].length;

        TIME_HERE("STGFX_DrawPoly after side stgfx_AVMEM_malloc");

        if (GC_p->LineWidth == 1 && GC_p->LineStyle_p == NULL)
        { 
            /* simple polyline */
            Err = stgfx_2dRasterSimpleLine(X1, Y1, X2, Y2, &sides[i].length,
                                           sides[i].xylData);
        }
        
        else if (GC_p->LineStyle_p == NULL) /* && (GC_p->LineWidth > 1) */
        { 
            /* thick polyline */
            Err = stgfx_2dRasterThickLine(Handle, X1, Y1, X2, Y2, 
                                          GC_p->LineWidth,
                                          &sides[i].length, sides[i].xylData);
        }
        
        else /* (GC_p->LineWidth > 1) && (GC_p->LineStyle_p != NULL) */
        {
            /* dashed polyline */
            Err = stgfx_2dRasterDashedLine(Handle, X1, Y1, X2, Y2, 
                                           GC_p->LineStyle_p, GC_p->LineWidth, 
                                           &sides[i].length, sides[i].xylData);
        }
        
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
            stgfx_DEVMEM_free(Handle, sides);
            STTBX_Print(("stgfx_DrawPoly->line raster error 0x%08X\n", Err));
            return(Err);
        }

        TIME_HERE("STGFX_DrawPoly after side raster");
    }
    
    /* insert xylarray joins in xylarray sides */
    if (GC_p->LineWidth > 1 && NumSides > 1 && GC_p->LineStyle_p == NULL)
    {
        NumP = 4;
        for (i = 0; i < (NumPoints - 1); i++)
        {
            Y[0] = sides[i].xylData[0].PositionY;
            Y[1] = sides[i].xylData[sides[i].length - 1].PositionY;
            
            if (i == 0)
            {
                Y[2] = joins[i].xylData[0].PositionY;
                Y[3] = joins[i].xylData[joins[i].length - 1].PositionY;
                
                for (j = 0; j < joins[i].length; j++)
                {
                    sides[i].xylData[j+sides[i].length].PositionX =
                                                joins[i].xylData[j].PositionX;
                    sides[i].xylData[j+sides[i].length].PositionY =
                                                joins[i].xylData[j].PositionY;
                    sides[i].xylData[j+sides[i].length].Length =
                                                   joins[i].xylData[j].Length;
                }
                sides[i].length += joins[i].length;
                if (PolyMode == Close)
                {
                    NumP = 6;
                    Y[4] = joins[NumPoints-2].xylData[0].PositionY;
                    Y[5] = joins[NumPoints-2].xylData[joins[NumPoints-2].length - 1].PositionY;                
                    for (j = 0; j < joins[NumPoints-2].length; j++)
                    {
                        sides[i].xylData[j+sides[i].length].PositionX =
                                        joins[NumPoints-2].xylData[j].PositionX;
                        sides[i].xylData[j+sides[i].length].PositionY =
                                        joins[NumPoints-2].xylData[j].PositionY;
                        sides[i].xylData[j+sides[i].length].Length =
                                        joins[NumPoints-2].xylData[j].Length;
                    }
                    sides[i].length += joins[NumPoints-2].length;
                }
            }
            else if (i == NumPoints - 2)
            {
                Y[2] = joins[i-1].xylData[0].PositionY;
                Y[3] = joins[i-1].xylData[joins[i-1].length - 1].PositionY;
                for (j = 0; j < joins[i-1].length; j++)
                {
                    sides[i].xylData[j+sides[i].length].PositionX =
                                                joins[i-1].xylData[j].PositionX;
                    sides[i].xylData[j+sides[i].length].PositionY =
                                                joins[i-1].xylData[j].PositionY;
                    sides[i].xylData[j+sides[i].length].Length =
                                                   joins[i-1].xylData[j].Length;
                }
                sides[i].length += joins[i-1].length;
                if (PolyMode == Close)
                {
                    NumP = 6;
                    Y[4] = joins[i].xylData[0].PositionY;
                    Y[5] = joins[i].xylData[joins[i].length - 1].PositionY;
                    for (j = 0; j < joins[i].length; j++)
                    {
                        sides[i].xylData[j+sides[i].length].PositionX =
                                                joins[i].xylData[j].PositionX;
                        sides[i].xylData[j+sides[i].length].PositionY =
                                                joins[i].xylData[j].PositionY;
                        sides[i].xylData[j+sides[i].length].Length =
                                                   joins[i].xylData[j].Length;
                    }
                    sides[i].length += joins[i].length;
                }
            }            
            else if (i > 0 && i < NumPoints - 2)
            {
                for (j = 0; j < joins[i-1].length; j++)
                {
                    sides[i].xylData[j+sides[i].length].PositionX =
                                              joins[i-1].xylData[j].PositionX;
                    sides[i].xylData[j+sides[i].length].PositionY =
                                              joins[i-1].xylData[j].PositionY;
                    sides[i].xylData[j+sides[i].length].Length =
                                                 joins[i-1].xylData[j].Length;
                }
                sides[i].length += joins[i-1].length;
                for (j = 0; j < joins[i].length; j++)
                {
                    sides[i].xylData[j+sides[i].length].PositionX = 
                                                  joins[i].xylData[j].PositionX;
                    sides[i].xylData[j+sides[i].length].PositionY = 
                                                  joins[i].xylData[j].PositionY;
                    sides[i].xylData[j+sides[i].length].Length
                                                   = joins[i].xylData[j].Length;
                }
                sides[i].length += joins[i].length;
                Y[2] = joins[i-1].xylData[0].PositionY;
                Y[3] = joins[i-1].xylData[joins[i-1].length - 1].PositionY;
                Y[4] = joins[i].xylData[0].PositionY;
                Y[5] = joins[i].xylData[joins[i].length - 1].PositionY;
                NumP = 6;
            }
            
            if (sides[i].length > SideAllocNum[i])
            {
                stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
                stgfx_DEVMEM_free(Handle, sides);
                return(STGFX_ERROR_XYL_POOL_EXCEEDED);
            }

            /* create a temporary copy of the xylarray in order to write back
              a sanitised version (why?) */
              
            CalcyTopyBot(NumP, &yTop, &yBot, Y);
            
            Err = stgfx_AVMEM_malloc(Handle, 
                                     sides[i].length*sizeof(STBLIT_XYL_t),
                                     &tmpXYLBlockHandleH, (void**) &tmpXYL_p);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
                stgfx_DEVMEM_free(Handle, sides);
                return(Err);
            }
            
#if 0
            for (j = 0; j < sides[i].length; j++)
            {
                tmpXYL_p[j].PositionX = sides[i].xylData[j].PositionX;
                tmpXYL_p[j].PositionY = sides[i].xylData[j].PositionY;
                tmpXYL_p[j].Length    = sides[i].xylData[j].Length;
            }
#else
            memcpy(tmpXYL_p, sides[i].xylData,
                   sides[i].length*sizeof(STBLIT_XYL_t));
#endif

            CalcXYLArray(yTop, yBot, sides[i].length, tmpXYL_p, 
                         sides[i].xylData);

            sides[i].length = yBot - yTop + 1;
            stgfx_AVMEM_free(Handle, &tmpXYLBlockHandleH);
        }

        TIME_HERE("STGFX_DrawPoly after joins insert");
    }
    
    /* fill rendering */
    if (PolyMode == Close &&
        GC_p->LineStyle_p == NULL &&
        GC_p->EnableFilling == TRUE &&
        NumSides > 2)
    {
        /* prepare xylfillarray for polygons */
        for (i = 0; i < (NumPoints - 1); i++)
        {
            Y1 = TmpPoints_p[i].Y;
            
            if (i == NumPoints - 2)
            {
                Y2 = TmpPoints_p[0].Y;
            }
            else
            {
                Y2 = TmpPoints_p[i+1].Y;
            }
            
            if (Y1 < Y2)
            {
                sides[i].Up = -1;
            }
            else if (Y1 == Y2)
            {
                sides[i].Up = 0;
            }
            else
            {
                sides[i].Up = 1;
            }
        }
        NumPoints--; /* restored later */
        
        /* winding/spin information computations
          - appears to adjust Up=0 to next nonzero Up, where one exists */
        for(i = 0; i < NumPoints; i++)
        {
            if(sides[i].Up)
                continue;
            for(j = i; j < NumPoints; j++)
            {
                if(!sides[j].Up)
                    continue;
                sides[i].Up = sides[j].Up;
                break;
            }
        }

        /* spin setting - increment with every change in Up,
          continuing over the wrap if the first and last Up values are the same */
        Spin = 0;
        sides[0].Spin = 0;
        for(i = 1; i < NumPoints; i++)
        {
            if(sides[i].Up != sides[i-1].Up)
            {
                Spin++;
            }
            sides[i].Spin = Spin;
        }
        for(i = 0; i < (NumPoints - 1); i++)
        {
            if(sides[NumPoints-1].Up == sides[i].Up)
            {
                sides[i].Spin = sides[NumPoints - 1].Spin; 
            }
            else
            {
                break;
            }
        }
  
        /* find upper and downer vertices */
        for(i = 0; i < NumPoints; i++)
        {
            Y[i] = FRACT_TO_INT(TmpPoints_p[i].Y);
        }
        CalcyTopyBot(NumPoints, &yTop, &yBot, Y);
        XYLFillNum = (yBot - yTop + 1) * (NumPoints - 1);
        
        Err = stgfx_AVMEM_malloc(Handle, XYLFillNum*sizeof(STBLIT_XYL_t),
                                 &XYLFillBH, (void**) &XYLFill_p);
        if(Err == ST_NO_ERROR)
        {
            XYLAllocated[NumXYLBlocks++] = XYLFillBH;

            Err = stgfx_FillGenericPolygon(Bitmap_p->Height, yTop, yBot,
                                           sides, NumPoints, XYLFill_p, 
                                           &XYLFillNum, GC_p->PolygonFillMode);
        }
        
        if(Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
            stgfx_DEVMEM_free(Handle, sides);
            return(Err);
        }
        
        NumPoints++; /* restoring after earlier decrement */
        
        TIME_HERE("STGFX_DrawPoly after fill raster");
    }

    /* caps, when required */
    HasCaps = (PolyMode == Open) && (
        ((GC_p->CapStyle == STGFX_PROJECTING_CAP) && (GC_p->LineWidth > 1)) ||
        ((GC_p->CapStyle == STGFX_ROUND_CAP) && (GC_p->LineWidth > 2)));
    if (HasCaps)
    {
        XYLCapNum[0] = INCREASE_XYL_SIZE_CAP * GC_p->LineWidth;
        Err = stgfx_AVMEM_malloc(Handle, XYLCapNum[0]*sizeof(STBLIT_XYL_t),
                                 &XYLCapBH[0], (void**) &XYLCap_p[0]);
        
        if(Err == ST_NO_ERROR)
        {
            XYLAllocated[NumXYLBlocks++] = XYLCapBH[0];

            XYLCapNum[1] = INCREASE_XYL_SIZE_CAP * GC_p->LineWidth;
            Err = stgfx_AVMEM_malloc(Handle, XYLCapNum[1]*sizeof(STBLIT_XYL_t),
                                     &XYLCapBH[1], (void**) &XYLCap_p[1]);
        }
        
        if(Err == ST_NO_ERROR)
        {
            XYLAllocated[NumXYLBlocks++] = XYLCapBH[1];

            /* first point cap */
            Err = stgfx_2dLineRasterCap(Handle,
                                        TmpPoints_p[0].X, TmpPoints_p[0].Y,
                                        TmpPoints_p[1].X, TmpPoints_p[1].Y, 
                                        GC_p->CapStyle, GC_p->LineWidth, 
                                        &XYLCapNum[0], XYLCap_p[0]);
        }
        
        if(Err == ST_NO_ERROR)
        {
            /* final point cap */
            Err = stgfx_2dLineRasterCap(Handle, TmpPoints_p[NumPoints-1].X, 
                                        TmpPoints_p[NumPoints-1].Y,
                                        TmpPoints_p[NumPoints-2].X, 
                                        TmpPoints_p[NumPoints-2].Y,
                                        GC_p->CapStyle, GC_p->LineWidth, 
                                        &XYLCapNum[1], XYLCap_p[1]);
        }
        
        /* handle error from any of the above */
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
            stgfx_DEVMEM_free(Handle, sides);
            return(Err);
        }
        
        TIME_HERE("STGFX_DrawPoly after cap raster");
    }
    
    /**********************************************/
    /**********************************************/
    /* sides, joints and caps rendering completed */
    /**********************************************/
    /**********************************************/

    /* first tiling analysis */
    if((GC_p->DrawTexture_p != NULL) || (GC_p->FillTexture_p != NULL))
    {
        Err = stgfx_InitTexture(Handle, GC_p, TargetBitmap_p, &MaskBitmap,
                                &BlitContext.WorkBuffer_p, &WorkBufferBH);
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
            stgfx_DEVMEM_free(Handle, sides);
            return(Err);
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
            stgfx_AVMEM_FreeAll(Handle, NumXYLBlocks, XYLAllocated);
            stgfx_AVMEM_free(Handle, &WorkBufferBH);
                /* safe to free directly as not yet passed to STBLIT */
            stgfx_DEVMEM_free(Handle, sides);
            return(Err);
        }
        
        Bitmap_p = &MaskBitmap;
    }
 
    
    /* Overlap removal - must remove all overlaps on an XYL array before
      drawing it, because stgfx_RemoveXYLOverlaps modifies both inputs */
    if (GC_p->AluMode != STBLIT_ALU_COPY)
    {
        for (i = 0; i < (NumPoints - 1); i++)
        {
            for (j = 0; j < (NumPoints - 1); j++)
            {
                /* removes xyloverlaps between sides */
                if (j != i)
                stgfx_RemoveXYLOverlaps(sides[i].xylData, &sides[i].length,
                                        sides[j].xylData, &sides[j].length);
            }
            if (GC_p->LineWidth > 1 &&
                NumSides > 1       &&
                GC_p->LineStyle_p != NULL)
            {
                /* removes xyloverlaps between sides and joins if dashed line */
                for (j = 0; j < NumJoins; j++)
                    stgfx_RemoveXYLOverlaps(sides[i].xylData, &sides[i].length,
                                            joins[j].xylData, &joins[j].length);
            }
            if (HasCaps)
            {
                /* removes xyloverlaps between sides and caps */
                stgfx_RemoveXYLOverlaps(sides[i].xylData, &sides[i].length,
                                        XYLCap_p[0], &XYLCapNum[0]);
                stgfx_RemoveXYLOverlaps(sides[i].xylData, &sides[i].length,
                                        XYLCap_p[1], &XYLCapNum[1]);
            }
        }
    }
        
    if (GC_p->LineWidth > 1                   &&
        NumSides > 1                          &&
        GC_p->LineStyle_p != NULL             &&
        GC_p->AluMode != STBLIT_ALU_COPY)
    {
        /* removes xyloverlaps between joins if dashed line */
        for (i = 0; i < NumJoins; i++)
        {
            for (j = 0; j < NumJoins; j++)
            {
                if (j != i)
                    stgfx_RemoveXYLOverlaps(joins[i].xylData, &joins[i].length,
                                            joins[j].xylData, &joins[j].length);
            }
        }
    }
    
    if (HasCaps && (NumSides > 1) && (GC_p->LineStyle_p != NULL) &&
        (GC_p->AluMode != STBLIT_ALU_COPY))
    {
        /* removes xyloverlaps between joins and caps if PolyLine */
        for (j = 0; j < NumJoins; j++)
        {
            if (j != 0)
                stgfx_RemoveXYLOverlaps(joins[j].xylData, &joins[j].length,
                                        XYLCap_p[0], &XYLCapNum[0]);
            if (j != (NumPoints - 1))
                stgfx_RemoveXYLOverlaps(joins[j].xylData, &joins[j].length,
                                        XYLCap_p[1], &XYLCapNum[1]);
        }
    }
    
    if (HasCaps && (GC_p->AluMode != STBLIT_ALU_COPY))
    {
        /* removes xyloverlaps between caps */
        stgfx_RemoveXYLOverlaps(XYLCap_p[0], &XYLCapNum[0],
                                XYLCap_p[1], &XYLCapNum[1]);
    }
    
    TIME_HERE("STGFX_DrawPoly after overlap removal");

    /* draw sides: all drawing errors are considered non-fatal because they
      don't prevent us drawing other things; we just free the offending
      avmem block */
    for (i = 0; i < (NumPoints - 1); i++)
    {
        BlitContext.UserTag_p = (void*)XYLSideBH[i];
        Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                                  sides[i].xylData, sides[i].length,
                                  &Color, &BlitContext);
        DonePause("sides[i]");
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_free(Handle, &XYLSideBH[i]);
            LastErr = STGFX_ERROR_STBLIT_BLIT;
        }
    }
    
    /* plots joins if dashed line (otherwise they came with the sides above) */
    if (NumJoins > 0)
    {
        if(GC_p->LineStyle_p != NULL)
        {
            for (i = 0; i < NumJoins; i++)
            {
                BlitContext.UserTag_p = (void*)XYLJoinBH[i];
                Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p,
                                          joins[i].xylData, joins[i].length,
                                          &Color, &BlitContext);
                DonePause("joins[i]");
                if (Err != ST_NO_ERROR)
                {
                    stgfx_AVMEM_free(Handle, &XYLJoinBH[i]);
                    LastErr = STGFX_ERROR_STBLIT_BLIT;
                }
            }
        }
        else
        {
            for(i = 0; i < NumJoins; i++)
            {
                Err = stgfx_AVMEM_free(Handle, &XYLJoinBH[i]);
            }
        }
    }
    
    /* plots caps */
    if (HasCaps)
    {
        for(i=0; i<2; i++)
        {
            BlitContext.UserTag_p = (void*)XYLCapBH[i];
            Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, 
                                      XYLCap_p[i], XYLCapNum[i],
                                      &Color, &BlitContext);
            DonePause("XYLCap_p[i]");
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &XYLCapBH[i]);
                LastErr = STGFX_ERROR_STBLIT_BLIT;
            }
        }
    }
    
    TIME_SYNC(Handle);
    TIME_HERE("STGFX_DrawPoly after side drawing");
    
    /*****************************/ 
    /*****************************/ 
    /* contour drawing completed */
    /*****************************/ 
    /*****************************/ 
    
    /* if draw tiling is on */
    if(GC_p->DrawTexture_p != NULL)
    {
        Bitmap_p = TargetBitmap_p; /* go back to writing to real target */
        
        /* ApplyTexture takes care of freeing MaskDataBH */
        Err = stgfx_ApplyTexture(Handle, 0, 0, GC_p, GC_p->DrawTexture_p,
                                 &MaskBitmap, &MaskDataBH, Bitmap_p,
                                 &BlitContext);
        if(Err != ST_NO_ERROR)
        {
            LastErr = Err;
        }
    }
 
    
    
    /* fill drawing */
    if( PolyMode == Close
        && GC_p->LineStyle_p == NULL
        && GC_p->EnableFilling == TRUE
        && NumSides > 2 )
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
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &XYLFillBH); /* just this one left */
                stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
                    /* need to schedule as already in use by STBLIT */
                stgfx_DEVMEM_free(Handle, sides);
                return(Err);
            }

            Bitmap_p = &MaskBitmap;
        }
 
        /* plots xylarrayfilldata */
        BlitContext.UserTag_p = (void*)XYLFillBH;
        Err = STBLIT_DrawXYLArray(BlitHandle, Bitmap_p, XYLFill_p,
                                  XYLFillNum, &Color, &BlitContext);
        DonePause("XYLFill_p");
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_free(Handle, &XYLFillBH);
            LastErr = STGFX_ERROR_STBLIT_BLIT;
        }
        
        /* if fill tiling is on */
        if(GC_p->FillTexture_p != NULL)
        {
            Bitmap_p = TargetBitmap_p; /* go back to writing to real target */

            /* ApplyTexture takes care of freeing MaskDataBH */
            Err = stgfx_ApplyTexture(Handle, 0, 0, GC_p, GC_p->FillTexture_p,
                                     &MaskBitmap, &MaskDataBH, Bitmap_p,
                                     &BlitContext);
            if(Err != ST_NO_ERROR)
            {
                LastErr = Err;
            }
        }
        
        TIME_SYNC(Handle);
        TIME_HERE("STGFX_DrawPoly after fill drawing");
    }
    
    stgfx_DEVMEM_free(Handle, sides);
    
    if(WorkBufferBH != STAVMEM_INVALID_BLOCK_HANDLE)
    {
        stgfx_ScheduleAVMEM_free(Handle, &WorkBufferBH);
    }
    
    return(LastErr);

} /* End of stgfx_DrawPoly */


/*******************************************************************************
Name        : stgfx_AVMEM_FreeAll()
Description : free multiple av memory blocks (here because only gfx_DrawPoly
              uses it). Ignores handles that have been zeroed-out (eg because
              they have already been passed to DrawXYLArray)
Parameters  : Handle             :
              NumAVMEMAllocation : number of XYLarray allocated
              AVMXYLAllocated    : array of allocation handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR/STGFX_ERROR_STAVMEM (keeps going, though)
*******************************************************************************/
ST_ErrorCode_t stgfx_AVMEM_FreeAll(
    STGFX_Handle_t         Handle,
    U32                    NumAVMEMAllocation,
    STAVMEM_BlockHandle_t* AVMXYLAllocated
)
{
    U32 i;
    ST_ErrorCode_t Err, LastErr = ST_NO_ERROR;

    for (i = 0; (i < NumAVMEMAllocation); i++)
    {
        if(AVMXYLAllocated[i] != STAVMEM_INVALID_BLOCK_HANDLE)
        {
            Err = stgfx_AVMEM_free(Handle, &AVMXYLAllocated[i]);
            if(Err != ST_NO_ERROR)
                LastErr = Err;
        }
    }

    return LastErr;
} /* End of stgfx_AVMEM_FreeAll */

/* End of gfx_polygon.c */
