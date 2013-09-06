/*******************************************************************************

File name   : gfx_tools.c

Description : STGFX internals source file (line/join/cap/fill/overlap removal)

COPYRIGHT (C) STMicroelectronics 2001.
*******************************************************************************/

/*#define STTBX_PRINT*/

/*#define XYLREM_PRINT*/
#ifdef XYLREM_PRINT /* RemoveXYLOverlaps() debugging */
#define STTBX_PRINT
#define XYLREM_Print(x) STTBX_Print(x)
#else
#define XYLREM_Print(x)
#endif

/* Includes ----------------------------------------------------------------- */

#include "gfx_tools.h" /* includes stgfx_init.h, ... */
#include <math.h>
#include <string.h>
#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

typedef struct
{
  BOOL  xflag;     /* TRUE = actually got an intersection, data below valid */
  S8    Up;        /* winding counter from side */
  S32   X1;        /* start x of xyl segment with desired y */
  S32   X2;        /* ending x of xyl segment with desired y */
  U8    Spin;      /* to catch vertices (from side) */
} gfxIntsect;


/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Extern functions */


/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t stgfx_CalcLimitJoin(
    S32               X1,
    S32               Y1,
    S32               Xjoin,
    S32               Yjoin,
    S32               X3,
    S32               Y3,
    S32               XintArc,
    S32               YintArc,
    S32               XextArc,
    S32               YextArc,
    stgfx_JoinSector  JoinSector,
    STGFX_JoinStyle_t JoinStyle,
    U16               LineWidth,
    float*            CheckJoin,
    U32               JoinPoints,
    STGFX_Point_t*    Points_p );

static ST_ErrorCode_t stgfx_CalcCapVertex(
  S32            Xcap,
  S32            Ycap,
  S32            X,
  S32            Y,
  U16            LineWidth,
  STGFX_Point_t* CapVertex_p );

static void stgfx_CalcVertex(
  S32            X1,
  S32            Y1,
  S32            X2,
  S32            Y2,
  U16            LineWidth,
  STGFX_Point_t* Vertex_p );


/* Static Functions' bodies ------------------------------------------------- */

/* This is the compare function which must be passed to qsort(). It puts valid
  elements before invalid ones, then those that start first before those
  that start later, finally those that end first before those that end later */
static int stgfx_compIsec(const void* a, const void* b)
{
  if( !(((gfxIntsect*)(a))->xflag) && (((gfxIntsect*)(b))->xflag))
    return( 1); /* a > b */
  if( (((gfxIntsect*)(a))->xflag) && !(((gfxIntsect*)(b))->xflag))
    return(-1); /* a < b */
  if( ((gfxIntsect*)(a))->X1 > ((gfxIntsect*)(b))->X1 )
    return( 1);
  if( ((gfxIntsect*)(a))->X1 < ((gfxIntsect*)(b))->X1 )
    return(-1);
  if( ((gfxIntsect*)(a))->X1 == ((gfxIntsect*)(b))->X1 )
  {
    if( ((gfxIntsect*)(a))->X2 > ((gfxIntsect*)(b))->X2 ) /* used to be >= */
     return( 1);
    if( ((gfxIntsect*)(a))->X2 < ((gfxIntsect*)(b))->X2 )
     return(-1);
  }
  return(0);
}


/* this function finds the intersection of a scan line with a segment */
static void stgfx_findIsec(
  S32           y,
  stgfx_Side_t* segment,
  gfxIntsect*   iSection
)
{
  int i;

  for(i=0; i<segment->length; i++)
  {
    if(segment->xylData[i].PositionY == y)
    {
      /* (nb y moves monatonically within sides,
        so we could search more efficiently) */
        
      iSection->X1 = segment->xylData[i].PositionX;
      iSection->X2 = segment->xylData[i].PositionX +
                     segment->xylData[i].Length - 1;
      iSection->xflag = TRUE;
      iSection->Up = segment->Up;
      iSection->Spin = segment->Spin;
      break;
    }
  }
  return;
}

/************************************************************************** 
   ResetDubValues()
   "this static recursive function is used to set correctly the spin value
   of consecutive sides" ??? - not recursive & doesn't set Spin. Rather it
   ensures that we don't count identical Up values belonging to the same
   thrust as changes of direction.
**************************************************************************/
static void ResetDubValues(gfxIntsect* Isecs, U32 num)
{
  U32 i;
  BOOL toggle = FALSE;
  
  if(Isecs[0].Up)
    toggle = TRUE;
  for(i=1; i<num; i++)
  {
    if(Isecs[0].Spin == Isecs[i].Spin)
    {
      if(toggle)
      {
        Isecs[i].Up = 0;
      }
      else /* toggle is still false */
      {
        if(Isecs[i].Up)
        {
          toggle = TRUE;
        }
      }
    } 
  }
}

/******************************************************************************/

/* Functions ---------------------------------------------------------------- */

/****************************************************************************\
Name        : stgfx_2dRasterSimpleLine()
Description : computes the xyl array which is the rendering
              of a 1-pixel thick segment between 2 given points.
              The output xyl segments are ordered ALWAYS in the positive verse
              of the x then y axes.
Parameters  : X1:
              Y1:
              X2:
              Y2:
              NumXYL: counter of the number of XYLArray elements
              XYLArray:
Assumptions : 
Limitations : 
Error codes : ST_NO_ERROR, STGFX_ERROR_XYL_POOL_EXCEEDED
\****************************************************************************/
ST_ErrorCode_t stgfx_2dRasterSimpleLine(
    S32            X1,
    S32            Y1,
    S32            X2,
    S32            Y2,
    U32*           NumXYL,
    STBLIT_XYL_t*  XYLArray
)
{
    S32  dx, dy, iE, iNE, d, x, y;
    BOOL newY;
    S32  i = -1;
    U32  XYLDim;
    U32  Size;
    
    XYLDim = GFXABS(FRACT_TO_INT(Y1) - FRACT_TO_INT(Y2)) + 1;
    Size = *NumXYL;
    
    if  (X1 > X2) /* sort so we are always going left to right */
    {
        GFXSWAP(X1, X2);
        GFXSWAP(Y1, Y2);
    }

    *NumXYL=0;
    if(FRACT_TO_INT(X1)==FRACT_TO_INT(X2) &&
       FRACT_TO_INT(Y1)==FRACT_TO_INT(Y2))
    {
      XYLArray[0].PositionX = FRACT_TO_INT(X1);
      XYLArray[0].PositionY = FRACT_TO_INT(Y1);
      XYLArray[0].Length = 1;
      *NumXYL = 1;
    }
    else if (FRACT_TO_INT(X1) == FRACT_TO_INT(X2))      /* vertical line | */
    {
        if  (Y1 > Y2)
            GFXSWAP(Y1, Y2);
        x = FRACT_TO_INT(X1);
        y = FRACT_TO_INT(Y1);
        for (i = 0; i < XYLDim; i++)
        {
            XYLArray[i].PositionX = x;
            XYLArray[i].PositionY = y;
            XYLArray[i].Length = 1;
            y += 1;
        }
        *NumXYL = XYLDim;
    }
    else if (FRACT_TO_INT(Y1) == FRACT_TO_INT(Y2))  /* horizontal line _____ */
    {
        /*if (X1 > X2)
            GFXSWAP(X1, X2); - already done above */
        x = FRACT_TO_INT(X1);
        XYLArray[0].PositionX = x;
        XYLArray[0].PositionY = FRACT_TO_INT(Y1);
        XYLArray[0].Length = 1 + FRACT_TO_INT(X2) - x;
        *NumXYL = 1;
    }
    else if ((X2 - X1) == (Y2 - Y1)) /* y=x+c line type \ (y +ve going down) */
    {
        x = FRACT_TO_INT(X1);
        y = FRACT_TO_INT(Y1);
        for (i = 0; i < XYLDim; i++)
        {
            XYLArray[i].PositionX = x;
            XYLArray[i].PositionY = y;
            XYLArray[i].Length = 1;
            
            x += 1;
            y += 1;
        }
        *NumXYL = XYLDim;
    }
    else if ((X2 - X1) == (Y1 - Y2)) /* y=-x+c line type / (y +ve going down) */
    {
        x = FRACT_TO_INT(X1);
        y = FRACT_TO_INT(Y1);
        for (i = 0; i < XYLDim; i++)
        {
            XYLArray[i].PositionX = x;
            XYLArray[i].PositionY = y;
            XYLArray[i].Length = 1;
            
            x += 1;
            y -= 1;
        }
        *NumXYL = XYLDim;
    }
    else if (((X2 - X1) > GFXABS((Y2 - Y1))) && (Y2 > Y1))
    {
        /* y=mx+c with 0 < m < 1 */
        dx = X2 - X1;
        dy = Y2 - Y1;
        d = (2 * dy) - dx;
        iE = 2 * dy;
        iNE = 2 * (dy - dx);
        x = X1;
        y = Y1;
        XYLArray[0].PositionX = FRACT_TO_INT(x);
        XYLArray[0].PositionY = FRACT_TO_INT(y);
        XYLArray[0].Length = 1;
        i = 0;
        *NumXYL = 1;
        while (x < X2)
        {
            if (d <= 0)
            {
                d += iE;
                x += (1<< STGFX_INTERNAL_ACCURACY);
                newY = FALSE;
            }
            else
            {
                d += iNE;
                x += (1<< STGFX_INTERNAL_ACCURACY);
                y += (1<< STGFX_INTERNAL_ACCURACY);
                newY = TRUE;
            }
            if (i == -1) /* the first xyl record */
            {
                i = 0;
                XYLArray[0].PositionX = FRACT_TO_INT(x);
                XYLArray[0].PositionY = FRACT_TO_INT(y);
                XYLArray[0].Length = 1;
                *NumXYL = 1;
                continue;
            }
            if (newY) /* new xyl record at a new y */
            {
                if(*NumXYL==XYLDim)
                {
                  break;
                }
                i++;
                XYLArray[i].PositionX = FRACT_TO_INT(x);
                XYLArray[i].PositionY = FRACT_TO_INT(y);
                XYLArray[i].Length = 1;
                (*NumXYL)++;
            }
            else /* current xyl record is NumXYL */
            {
                XYLArray[i].Length++;
            }
        }
    }
    else if (((X2 - X1) > GFXABS((Y2 - Y1))) && (Y1 > Y2))
    {
        /* y=mx+c with -1 < m < 0 */
        dx = X2 - X1;
        dy = Y1 - Y2;
        d = (2 * dy) - dx;
        iE = 2 * dy;
        iNE = 2 * (dy - dx);
        x = X1;
        y = Y1;
        XYLArray[0].PositionX = FRACT_TO_INT(x);
        XYLArray[0].PositionY = FRACT_TO_INT(y);
        XYLArray[0].Length = 1;
        i = 0;
        *NumXYL = 1;
        while (x < X2)
        {
            if (d <= 0)
            {
                d += iE;
                x += (1<< STGFX_INTERNAL_ACCURACY);
                newY = FALSE;
            }
            else
            {
                d += iNE;
                x += (1<< STGFX_INTERNAL_ACCURACY);
                y -= (1<< STGFX_INTERNAL_ACCURACY);
                newY = TRUE;
            }
            if (newY) /* new xyl record at a new y */
            {
                if(*NumXYL==XYLDim)
                {
                  break;
                }
                i++;
                XYLArray[i].PositionX = FRACT_TO_INT(x);
                XYLArray[i].PositionY = FRACT_TO_INT(y);
                XYLArray[i].Length = 1;
                (*NumXYL)++;
            }
            else /* current xyl record Length */
            {
                XYLArray[i].Length++;
            }
        }
    }
    else if (((X2 - X1) < GFXABS((Y2 - Y1))) && (Y2 > Y1))
    {
        /* y=mx+c with 1 < m < +inf */
        dx = X2 - X1;
        dy = Y2 - Y1;
        d = (2 * dx) - dy;
        iE = 2 * dx;
        iNE = 2 * (dx - dy);
        x = X1;
        y = Y1;
        XYLArray[0].PositionX = FRACT_TO_INT(x);
        XYLArray[0].PositionY = FRACT_TO_INT(y);
        XYLArray[0].Length = 1;
        i = 0;
        *NumXYL = 1;
        while (y < Y2)
        {
            if (d <= 0)
            {
                d += iE;
                y += (1<< STGFX_INTERNAL_ACCURACY);
            }
            else
            {
                d += iNE;
                x += (1<< STGFX_INTERNAL_ACCURACY);
                y += (1<< STGFX_INTERNAL_ACCURACY);
            }
            i++;
            XYLArray[i].PositionX = FRACT_TO_INT(x);
            XYLArray[i].PositionY = FRACT_TO_INT(y);
            XYLArray[i].Length = 1;
            (*NumXYL)++;
          if(*NumXYL==XYLDim)
          {
            break;
          }
        }
    }
    else if (((X2 - X1) < GFXABS((Y1 - Y2))) && (Y1 > Y2))
    {
        /* y=-mx+c with 1<m<+inf */
        dx = X2 - X1;
        dy = Y1 - Y2;
        d = (2 * dx) - dy;
        iE = 2 * dx;
        iNE = 2 * (dx - dy);
        x = X1;
        y = Y1;
        XYLArray[0].PositionX = FRACT_TO_INT(x);
        XYLArray[0].PositionY = FRACT_TO_INT(y);
        XYLArray[0].Length = 1;
        i = 0;
        *NumXYL = 1;
        while (*NumXYL<XYLDim)
        {
            if (d <= 0)
            {
                d += iE;
                y -= (1<< STGFX_INTERNAL_ACCURACY);
            }
            else
            {
                d += iNE;
                x += (1<< STGFX_INTERNAL_ACCURACY);
                y -= (1<< STGFX_INTERNAL_ACCURACY);
            }
            i++;
            XYLArray[i].PositionX = FRACT_TO_INT(x);
            XYLArray[i].PositionY = FRACT_TO_INT(y);
            XYLArray[i].Length = 1;
          if(*NumXYL==XYLDim)
          {
            break;
          }
            (*NumXYL)++;
        }
    }
    
    if ((*NumXYL) > Size)
        return (STGFX_ERROR_XYL_POOL_EXCEEDED);
    else    
        return(ST_NO_ERROR);
    
}   /* end of stgfx_2dRasterSimpleLine() */


/******************************************************************************
Name        : stgfx_2dRasterThickLine()
Description : computes the xyl array which is the rendering
              of a n-pixel (n>1) thick segment between 2 given points.
              The output xyl segments are ordered ALWAYS in the positive verse
              of the x then y axes.
Parameters  : X1:
              Y1:
              X2:
              Y2:
              LineWidth:
              NumXYL: counter of the number of XYLArray elements
              XYLArray:
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR, STGFX_ERROR_XYL_POOL_EXCEEDED
******************************************************************************/
ST_ErrorCode_t stgfx_2dRasterThickLine(
    STGFX_Handle_t Handle,
    S32            X1,
    S32            Y1,
    S32            X2,
    S32            Y2,
    U16            LineWidth,
    U32*           NumXYL,
    STBLIT_XYL_t*  XYLArray
)
{
    ST_ErrorCode_t Err;
    STGFX_Point_t  Vertex_p[4];
    U32            Size, tmpSize;
    U32            NumPoints = 4;
    U16            i;
    
    Size = *NumXYL;
    tmpSize = *NumXYL;
    *NumXYL = 0;

    if (X1 == X2 && Y1 == Y2)
    {
      S32 YInt  = FRACT_TO_INT(Y1) - (LineWidth/2);
      S32 XInt  = FRACT_TO_INT(X1);
      
      if(Size < LineWidth) /* XYL array is too small */
      {
        return(STGFX_ERROR_XYL_POOL_EXCEEDED);
      }
      *NumXYL = LineWidth;
      for(i=0; i<LineWidth; i++)
      {
        XYLArray[i].PositionX = XInt;
        XYLArray[i].PositionY = YInt + i;
        XYLArray[i].Length = 1;
      }
    }
    else
    {
        if  (X1 > X2)
        {
            GFXSWAP(X1, X2);
            GFXSWAP(Y1, Y2);
        }
        else if (X1 == X2 && Y1 > Y2)
            GFXSWAP(Y1, Y2);

        stgfx_CalcVertex(X1, Y1, X2, Y2, LineWidth, Vertex_p);

        GFXSWAP(Vertex_p[2].X, Vertex_p[3].X);
        GFXSWAP(Vertex_p[2].Y, Vertex_p[3].Y);

        Err = stgfx_FillSimplePolygon(Handle, NumPoints, Vertex_p, &Size, 
                                      XYLArray);
    
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }
    
        *NumXYL = Size;
    }
    
    if ((*NumXYL) > tmpSize)
        return(STGFX_ERROR_XYL_POOL_EXCEEDED);
    else
        return(ST_NO_ERROR);

}   /* End of stgfx_2dRasterThickLine() */


/*******************************************************************************
Name        : stgfx_2dRasterDashedLine()
Description : computes the xyl array which is the rendering
              of a n-pixel (n>=1) thick-dashed segment between 2 given points.
              The output xyl segments are ordered ALWAYS in the positive verse
              of the x then y axes.
Parameters  : X1,
              Y1,
              X2,
              Y2,
              LineStyle, pointer to an array ended with a zero
              NumXYL, counter of the number of XYLArray elements
              XYLArray 
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR, STGFX_ERROR_XYL_POOL_EXCEEDED
*******************************************************************************/
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
)
{
    ST_ErrorCode_t Err;
    S32            DeltaX, DeltaY;
    double         Length;
    S32            XA;
    S32            YA;
    S32            XB = 0;
    S32            YB = 0;
    S32            D1 = 0;
    S32            D2 = 0;
    S32            i = 0;
    U32            Size, tmpSize;


    if  (X1 > X2)
    {
        GFXSWAP(X1, X2);
        GFXSWAP(Y1, Y2);
    }
    else if (X1 == X2 && Y1 > Y2)
        GFXSWAP(Y1, Y2);
    
    tmpSize = *NumXYL;
    Size = *NumXYL;
    *NumXYL = 0;
    
    DeltaX = X2 - X1;
    DeltaY = Y2 - Y1;
    Length = (sqrt(((double)DeltaX * (double)DeltaX) +
                  ((double)DeltaY * (double)DeltaY)));
    
    while (D1 <= (S32)Length)
    {
        D2 = D1 + ((LineStyle[i] - 1) << STGFX_INTERNAL_ACCURACY);
        XA = (S32)(X1 + D1 * (DeltaX / Length));
        YA = (S32)(Y1 + D1 * (DeltaY / Length));
        XB = (S32)(X1 + D2 * (DeltaX / Length));
        YB = (S32)(Y1 + D2 * (DeltaY / Length));

        if ((XB > X2) || ((XB == X2) && (YB > Y2)))
        {
            XB = X2;
            YB = Y2;
        }
        Size = GFXABS(FRACT_TO_INT(YB) - FRACT_TO_INT(YA)) + 1
               + (2 * LineWidth);

        if (LineWidth == 1)
        {
            Err = stgfx_2dRasterSimpleLine(XA, YA, XB, YB, &Size, XYLArray);
            if (Err != ST_NO_ERROR)
            {
                return(Err);
            }
        }
        else
        {
            Err = stgfx_2dRasterThickLine(Handle, XA, YA, XB, YB,
                                          LineWidth, &Size, XYLArray);
            if (Err != ST_NO_ERROR)
            {
                return(Err);
            }
        }

        XYLArray += Size;
        *NumXYL += Size;

        /* advance over filled segment and gap segment, if any */
        D1 += ((LineStyle[i] + LineStyle[i + 1]) << STGFX_INTERNAL_ACCURACY);
        if ((LineStyle[i + 1] == 0) || (LineStyle[i + 2] == 0))
        {
            i = 0; /* end of array reached; restart from beginning */
        }
        else
        {
            i += 2; /* next filled segment */
        }
    }

    if ((*NumXYL) > tmpSize)
        return(STGFX_ERROR_XYL_POOL_EXCEEDED);
    else
        return(ST_NO_ERROR);
    
}   /* End of 2dRasterDashedLine */


/******************************************************************************
Name        : stgfx_2dRasterJoin()
Description : computes the xyl array which is the rendering
              of the joint between two consecutive lines with
              linewidth > 1;
              this array is allocated inside the function because the 
              CalcLimitJoin function compute exactly the length of the join xyl;
              The output xyl segments are ordered ALWAYS in the positive verse
              of the y axis.
Parameters  : X1, Y1:    extreme 1st line
              X2, Y2:    join point
              X3, Y3:    extreme 2nd line
              LineWidth:
              JoinStyle:
              LineStyle:
              JoinBlockHandle: handle of the xylarray allocated
              NumXYL:    counter of the number of XYLArray elements
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
*******************************************************************************/
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
    STAVMEM_BlockHandle_t* JoinBlockHandle_p,
    STBLIT_XYL_t**         XYL_p_p,
    U32*                   NumXYL
)
{
    ST_ErrorCode_t        Err;
    U32                   Size, j, i;
    float                 CheckJoin;
    U32                   JoinPointsNum;
    STGFX_Point_t         JoinPoints[5];
    STGFX_Point_t         DashJoinPoints[4];
    U8                    flag = 0;
    S32                   yTop, yBot;
    STBLIT_XYL_t*         XYLArray;
    U32                   Hand;
    STBLIT_XYL_t*         ArcXYLArray;
    U32                   ArcSize = 0;
    STAVMEM_BlockHandle_t ArcBlockHandle;
    STBLIT_XYL_t*         tmpXYLArray;
    STAVMEM_BlockHandle_t tmpBlockHandle;
    S32                   YY[6];
    float                 Check_c1;
    S32                   Xa, Ya, Xb, Yb, Xc, Yc, Xc1, Yc1, Xc2, Yc2;
    float                 XP = 0;
    float                 YP = 0;
    float                 XD;
    float                 YD;
    float                 XCenter;
    float                 YCenter;
    float                 Radius;
    float                 XS, YS, XE, YE;
    float                 A, B, C, D, E, F;
    S32                   InP[4], OutP[4];
    S32                   Orientation = 0;
    U16                   LineW;
    U8                    ExtraFlag = 0;
    U32                   ExtraSize = 0;
    STBLIT_XYL_t          ExtraXYLArray[10];
    U32                   NumYY = 4;


    Size = *NumXYL = 0;

    Hand = *JoinBlockHandle_p;

    if ((LineWidth <= 3) && (JoinStyle == STGFX_ROUNDED_JOIN))
        JoinStyle = STGFX_FLAT_JOIN;

    if ((X1 == Xjoin && Xjoin == X3) || /* lines | to the bitmap and //
                                           between them                 */
        (Y1 == Yjoin && Yjoin == Y3) || /* lines - to the bitmap and //
                                           between them                 */
        (((X1 == Xjoin) && (Xjoin == X3)) && ((Y1 == Yjoin) && (Yjoin == Y3))))
    {
        flag = 1;
        *NumXYL = 1;
    }
    else
    {
        if (JoinStyle == STGFX_FLAT_JOIN)
            JoinPointsNum = 4;
        else if (JoinStyle == STGFX_ROUNDED_JOIN)
            JoinPointsNum = 4;
        else if (JoinStyle == STGFX_RECTANGLE_JOIN)
            JoinPointsNum = 5;

        Err = stgfx_CalcLimitJoin(X1, Y1, Xjoin, Yjoin, X3, Y3,
                                  XintArc, YintArc, XextArc, YextArc,
                                  JoinSector, JoinStyle,
                                  LineWidth, &CheckJoin, JoinPointsNum,
                                  JoinPoints);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }
    }


    if (CheckJoin == 0.F)               /* lines // between them        */
    {
        flag = 1;
        *NumXYL = 1;
    }
    else
    {
        for(j = 0; j < JoinPointsNum; j++)
        {
            YY[j] = FRACT_TO_INT(JoinPoints[j].Y);
        }
        CalcyTopyBot(JoinPointsNum, &yTop, &yBot, YY);
        *NumXYL = (yBot - yTop + 1) * LineWidth + 4 * LineWidth;
    }

    Err = stgfx_AVMEM_malloc(Handle, (*NumXYL)*sizeof(STBLIT_XYL_t),
                             &Hand, (void**) &XYLArray);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }

    if (flag == 1)
    {
        XYLArray[0].PositionX = FRACT_TO_INT(Xjoin);
        XYLArray[0].PositionY = FRACT_TO_INT(Yjoin);
        XYLArray[0].Length    = 0;
        *NumXYL = 1;
    }
    else
    {
        Size = *NumXYL;
        if (LineStyle == NULL)
        {
            Err = stgfx_FillSimplePolygon(Handle, JoinPointsNum, JoinPoints,
                                          &Size, XYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &Hand);
                return(Err);
            }
        }
        else
        {
            JoinPointsNum--;
            DashJoinPoints[0].X = Xjoin; DashJoinPoints[0].Y = Yjoin;
            DashJoinPoints[1].X = JoinPoints[2].X;
            DashJoinPoints[1].Y = JoinPoints[2].Y;
            DashJoinPoints[2].X = JoinPoints[3].X;
            DashJoinPoints[2].Y = JoinPoints[3].Y;
            if (JoinStyle == STGFX_RECTANGLE_JOIN)
            {
                DashJoinPoints[3].X = JoinPoints[4].X;
                DashJoinPoints[3].Y = JoinPoints[4].Y;
            }
            Err = stgfx_FillSimplePolygon(Handle, JoinPointsNum,
                                          DashJoinPoints, &Size, XYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &Hand);
                return(Err);
            }
        }
        *NumXYL = Size;

        if (JoinStyle == STGFX_ROUNDED_JOIN)
        {
            LineW = LineWidth;
            ArcSize = 4 * LineWidth;
            if ((JoinSector != Lines) &&
                (((X1==Xjoin) && (Yjoin>=Y1)) || ((X3<=Xjoin) && (Yjoin==Y3))))
            {
                Xa = InP[0] = JoinPoints[0].X; Ya = InP[1] = JoinPoints[0].Y;
                Xb = InP[2] = JoinPoints[1].X; Yb = InP[3] = JoinPoints[1].Y;
            }
            else
            {
                if ((JoinSector == Lines)                &&
                    (JoinPoints[0].Y == JoinPoints[2].Y) &&
                    (JoinPoints[1].Y == JoinPoints[3].Y))
                {
                        Xb = InP[0] = JoinPoints[0].X;
                        Yb = InP[1] = JoinPoints[0].Y;
                        Xa = InP[2] = JoinPoints[1].X;
                        Ya = InP[3] = JoinPoints[1].Y;
                }
                else
                {
                    Xa = InP[0] = JoinPoints[2].X;
                    Ya = InP[1] = JoinPoints[2].Y;
                    Xb = InP[2] = JoinPoints[3].X;
                    Yb = InP[3] = JoinPoints[3].Y;
                }
            }

            FindPointOnNormal(InP, OutP,
                              ((LineWidth << STGFX_INTERNAL_ACCURACY) / 2));
            Xc1 = OutP[0]; Yc1 = OutP[1];
            Xc2 = OutP[2]; Yc2 = OutP[3];

            if (Xb != Xa)
            {
                CheckJoin = (float)Yjoin - (float)Yb -
                            (((float)Yb - (float)Ya) / ((float)Xb - (float)Xa))
                            * ((float)Xjoin - (float)Xb);
                Check_c1  = (float)Yc1 - (float)Yb -
                            (((float)Yb - (float)Ya) / ((float)Xb - (float)Xa))
                                * ((float)Xc1 - (float)Xb);
            }
            else  /* vertical line */
            {
                CheckJoin = Check_c1 = (float)Yb - (float)Ya;
            }

            if (((CheckJoin < 0.F) && (Check_c1 < 0.F)) ||
                ((CheckJoin > 0.F) && (Check_c1 > 0.F)) ||
                ((CheckJoin == 0.F) && (Check_c1 == 0.F)))
            {
                Xc = Xc1; Yc = Yc1;
            }
            else
            {
                Xc = Xc2; Yc = Yc2;
            }
            if ((CheckJoin == Check_c1) || ((Ya == Yb) && (Xa > Xb)))
            {
                Xc = Xjoin; Yc = Yjoin;
            }
            XCenter = (float)Xc / (1 << STGFX_INTERNAL_ACCURACY);
            YCenter = (float)Yc / (1 << STGFX_INTERNAL_ACCURACY);
            LineWidth = (LineWidth << STGFX_INTERNAL_ACCURACY) / 2;
            Radius = (float)LineWidth / (1 << STGFX_INTERNAL_ACCURACY);

            if ((Ya > Yc) || ((Ya == Yc) && (Xa < Xc)))
                Orientation = 180*64;

            CalcConicCoefficients(Radius, Radius, Orientation,
                                  &A, &B, &C, &D, &E, &F, &XP, &YP, &XD, &YD);

            XS = (float)Xa / (1 << STGFX_INTERNAL_ACCURACY);
            YS = (float)Ya / (1 << STGFX_INTERNAL_ACCURACY);
            XE = (float)Xb / (1 << STGFX_INTERNAL_ACCURACY);
            YE = (float)Yb / (1 << STGFX_INTERNAL_ACCURACY);

            Err = stgfx_AVMEM_malloc(Handle, ArcSize*sizeof(STBLIT_XYL_t),
                                     &ArcBlockHandle, (void**) &ArcXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &Hand);
                return(Err);
            }

            XS = XS - XCenter;
            YS = YCenter - YS;
            XE = XE - XCenter;
            YE = YCenter - YE;
            XS = XS + Radius;
            XE = XE + Radius;

            if (Orientation)
            {
                XS = XS -2*Radius;
                XE = XE -2*Radius;
            }

            /*if((IS_ODD((U32)(YCenter*2.F))) && (IS_ODD((U32)(Radius*2.F))))
            {
              if(YP >= 0)
              {
                YP += 0.5F;
              }
              else
              {
                YP -= 0.5F;
              }
            }*/

            Err = Conic(XP, YP, XS, YS, XE, YE, A, B, C, D,
                        E, F, (XCenter-XD), (YCenter+YD), XD, YD,
                        &ArcSize, ArcXYLArray, FALSE, NULL, TRUE);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &Hand);
                stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                return(Err);
            }

            if (((ArcXYLArray[0].PositionX+ArcXYLArray[0].Length - 1) !=
                  FRACT_TO_INT(Xa)) ||
                (ArcXYLArray[0].PositionY != FRACT_TO_INT(Ya)))
            {
                ExtraFlag = 1;
                ExtraSize = GFXABS(ArcXYLArray[0].PositionY - FRACT_TO_INT(Ya))
                            +1;
                Err = stgfx_2dRasterSimpleLine(Xa, Ya,
                      (ArcXYLArray[0].PositionX + ArcXYLArray[0].Length - 1)
                                                     << STGFX_INTERNAL_ACCURACY,
                            ArcXYLArray[0].PositionY << STGFX_INTERNAL_ACCURACY,
                            &ExtraSize, ExtraXYLArray);
                if (Err != ST_NO_ERROR)
                {
                    stgfx_AVMEM_free(Handle, &Hand);
                    stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                    return(Err);
                }
                if (ExtraSize > sizeof(ExtraXYLArray)/sizeof(ExtraXYLArray[0]))
                {
                    stgfx_AVMEM_free(Handle, &Hand);
                    stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
                }
            }

            YY[0] = YY[1] = XYLArray[0].PositionY;
            for (i = 1; i < Size; i++)
            {
                YY[0] = GFXMIN(YY[0], XYLArray[i].PositionY);
                YY[1] = GFXMAX(YY[1], XYLArray[i].PositionY);
            }
            YY[2] = YY[3] = ArcXYLArray[0].PositionY;
            for (i = 1; i < ArcSize; i++)
            {
                YY[2] = GFXMIN(YY[2], ArcXYLArray[i].PositionY);
                YY[3] = GFXMAX(YY[3], ArcXYLArray[i].PositionY);
            }
            if (ExtraFlag)
            {
                /* NumYY = 6 ? */
                YY[4] = YY[5] = ExtraXYLArray[0].PositionY;
                for (i = 1; i < ExtraSize; i++)
                {
                    YY[4] = GFXMIN(YY[4], ExtraXYLArray[i].PositionY);
                    YY[5] = GFXMAX(YY[5], ExtraXYLArray[i].PositionY);
                }
            }
            CalcyTopyBot(NumYY, &yTop, &yBot, YY);

            Size += ArcSize;
            if (ExtraFlag)
            {
                Size += ExtraSize;
            }

            Err = stgfx_AVMEM_malloc(Handle, Size*sizeof(STBLIT_XYL_t),
                                     &tmpBlockHandle, (void**) &tmpXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &Hand);
                stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                return(Err);
            }
            
            for (i = 0; i < (*NumXYL); i++)
            {
                tmpXYLArray[i].PositionX = XYLArray[i].PositionX;
                tmpXYLArray[i].PositionY = XYLArray[i].PositionY;
                tmpXYLArray[i].Length    = XYLArray[i].Length;
            }
            for (i = 0; i < ArcSize; i++)
            {
                tmpXYLArray[i + (*NumXYL)].PositionX = ArcXYLArray[i].PositionX;
                tmpXYLArray[i + (*NumXYL)].PositionY = ArcXYLArray[i].PositionY;
                tmpXYLArray[i + (*NumXYL)].Length    = ArcXYLArray[i].Length;
            }
            if (ExtraFlag)
            {
                for (i = 0; i < ExtraSize; i++)
                {
                    tmpXYLArray[i + (*NumXYL) + ArcSize].PositionX =
                                                     ExtraXYLArray[i].PositionX;
                    tmpXYLArray[i + (*NumXYL) + ArcSize].PositionY =
                                                     ExtraXYLArray[i].PositionY;
                    tmpXYLArray[i + (*NumXYL) + ArcSize].Length    =
                                                        ExtraXYLArray[i].Length;
                }
            }

            CalcXYLArray(yTop, yBot, Size, tmpXYLArray, XYLArray);

            stgfx_AVMEM_free(Handle, &ArcBlockHandle);
            stgfx_AVMEM_free(Handle, &tmpBlockHandle);

            (*NumXYL) = yBot - yTop + 1;
        }
    }

    *JoinBlockHandle_p = Hand;
    *XYL_p_p            = XYLArray;

    return(ST_NO_ERROR);

} /* End of stgfx_2dRasterJoin() */

/******************************************************************************
Name        : stgfx_2dLineRasterCap()
Description : computes the xyl array which is the rendering
              of the cap of a line with linewidth > 1
              The output xyl segments are ordered ALWAYS in the positive verse
              of the y axis.
Parameters  : Xcap, Ycap: cap coordinates
              X, Y:       extreme coordinates
              CapStyle:
              LineWidth:
              NumXYL:  counter of the number of XYLArray elements
              XYLArray: 
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR, STGFX_ERROR_XYL_POOL_EXCEEDED
*******************************************************************************/
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
)
{
    ST_ErrorCode_t        Err;
    STGFX_Point_t         CapVertex_p;
    U32                   Size, tmpSize, i;
    STGFX_Point_t         Vertex[4];
    STBLIT_XYL_t*         LineXYLArray;
    U32                   LineSize = 0;
    STAVMEM_BlockHandle_t LineBlockHandle;
    STBLIT_XYL_t*         ArcXYLArray;
    U32                   ArcSize = 0;
    STAVMEM_BlockHandle_t ArcBlockHandle;
    STBLIT_XYL_t*         tmpXYLArray;
    STAVMEM_BlockHandle_t tmpBlockHandle;
    S32                   yTop, yBot, YY[4];
    S32                   Orientation = 0;
    double                Beta;
    float                 M;
    U32                   Radius;
    S32                   PixelPoints[4];


    tmpSize = Size = *NumXYL;
    *NumXYL = 0;

    if (CapStyle == STGFX_PROJECTING_CAP)
    {
        Err = stgfx_CalcCapVertex(Xcap, Ycap, X, Y, LineWidth, &CapVertex_p);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }

        Err = stgfx_2dRasterThickLine(Handle, Xcap, Ycap,
                                      CapVertex_p.X, CapVertex_p.Y,
                                      LineWidth, &Size, XYLArray);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }
        *NumXYL = Size;
    }
    else if (CapStyle == STGFX_ROUND_CAP)
    {
        if (LineWidth == 2)
        {
            XYLArray[0].PositionX = FRACT_TO_INT(Xcap);
            XYLArray[0].PositionY = FRACT_TO_INT(Ycap);
            XYLArray[0].Length    = 0;
            /* *NumXYL = 0; above */
        }
        else
        {
            stgfx_CalcVertex(Xcap, Ycap, X, Y, LineWidth, Vertex);

            LineSize = GFXABS(FRACT_TO_INT(Vertex[1].Y) - 
                              FRACT_TO_INT(Vertex[0].Y)) + 1;
            Err = stgfx_AVMEM_malloc(Handle, LineSize*sizeof(STBLIT_XYL_t),
                                     &LineBlockHandle, (void**) &LineXYLArray);
            if (Err != ST_NO_ERROR)
            {
                return(Err);
            }
            
            Err = stgfx_2dRasterSimpleLine(Vertex[0].X, Vertex[0].Y,
                                           Vertex[1].X, Vertex[1].Y,
                                           &LineSize, LineXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle);
                return(Err);
            }
            YY[0] = FRACT_TO_INT(Vertex[0].Y);
            YY[1] = FRACT_TO_INT(Vertex[1].Y);
        
            if (Xcap != X)
            {
                M = ((float)Ycap - (float)Y) / ((float)Xcap - (float)X);
                Beta = atan(M);
                Beta = Beta * PI / PI_FLOAT;
            }
            else
            {    
                if (Ycap <= Y)
                    Beta = 90 * 64;
                else
                    Beta = -90 * 64;
            }
            Orientation = 90 * 64 - (S32)Beta;
            if (Xcap > X)
                Orientation += 180*64;
        
            Radius = (LineWidth / 2) << STGFX_INTERNAL_ACCURACY;
        
            if (!IS_ODD(LineWidth))
            {
                Radius -= (1 << (STGFX_INTERNAL_ACCURACY - 1));
                Xcap = (Vertex[0].X + Vertex[1].X) / 2;
                Ycap = (Vertex[0].Y + Vertex[1].Y) / 2;
            }
            ArcSize = LineWidth + 2; /* 1 */
            Err = stgfx_AVMEM_malloc(Handle, ArcSize*sizeof(STBLIT_XYL_t),
                                     &ArcBlockHandle, (void**) &ArcXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle);
                return(Err);
            }
            Err = stgfx_2dRasterSimpleArc(Xcap, Ycap, Radius, Radius, 0, 180*64,
                                          Orientation, &ArcSize, ArcXYLArray,
                                          PixelPoints);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle);
                stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                STTBX_Print(("stgfx_2dRasterLineCap->stgfx_2dRasterSimpleArc"
                             " error 0x%08X\n", Err));
                return(Err);
            }

            YY[2] = YY[3] = ArcXYLArray[0].PositionY;
            for (i = 1; i < ArcSize; i++)
            {
                YY[2] = GFXMIN(YY[2], ArcXYLArray[i].PositionY);
                YY[3] = GFXMAX(YY[3], ArcXYLArray[i].PositionY);
            }

            CalcyTopyBot(4, &yTop, &yBot, YY);

            Size = LineSize + ArcSize;
            Err = stgfx_AVMEM_malloc(Handle, Size*sizeof(STBLIT_XYL_t),
                                     &tmpBlockHandle, (void**) &tmpXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle);
                stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                return(Err);
            }

            for (i = 0; i < LineSize; i++)
            {
                tmpXYLArray[i].PositionX = LineXYLArray[i].PositionX;
                tmpXYLArray[i].PositionY = LineXYLArray[i].PositionY;
                tmpXYLArray[i].Length    = LineXYLArray[i].Length;
            }
            for (i = 0; i < ArcSize; i++)
            {
                tmpXYLArray[i + LineSize].PositionX = ArcXYLArray[i].PositionX;
                tmpXYLArray[i + LineSize].PositionY = ArcXYLArray[i].PositionY;
                tmpXYLArray[i + LineSize].Length    = ArcXYLArray[i].Length;
            }

            CalcXYLArray(yTop, yBot, Size, tmpXYLArray, XYLArray);

            stgfx_AVMEM_free(Handle, &LineBlockHandle);
            stgfx_AVMEM_free(Handle, &ArcBlockHandle);
            stgfx_AVMEM_free(Handle, &tmpBlockHandle);

            (*NumXYL) = yBot - yTop + 1;
        }
    }
    if ((*NumXYL) > tmpSize)
        return(STGFX_ERROR_XYL_POOL_EXCEEDED);
    else
        return(ST_NO_ERROR);
    
}   /* end of stgfx_2dLineRasterCap() */


/******************************************************************************
Name        : stgfx_2dArcRasterCap()
Description : computes the xyl array which is the rendering
              of the cap of an arc with linewidth > 1
              The output xyl segments are ordered ALWAYS in the positive verse
              of the y axis.
Parameters  : Xcap, Ycap: cap coordinates
              X, Y:       extreme coordinates
              CapStyle:
              LineWidth:
              NumXYL:  counter of the number of XYLArray elements
              XYLArray: 
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR, STGFX_ERROR_XYL_POOL_EXCEEDED
*******************************************************************************/
ST_ErrorCode_t stgfx_2dArcRasterCap(
                                   STGFX_Handle_t   Handle,
                                   S32              Xs,
                                   S32              Ys,
                                   S32              Xe,
                                   S32              Ye,
                                   STGFX_CapStyle_t CapStyle,
                                   U16              LineWidth,
                                   U32*             NumXYL,
                                   STBLIT_XYL_t*    XYLArray,
                                   BOOL             IsStart
                                   )
{
    ST_ErrorCode_t        Err;
    U32                   Size, tmpSize, i;
    STGFX_Point_t         Vertex[4];
    STBLIT_XYL_t*         LineXYLArray;
    U32                   LineSize = 0;
    STAVMEM_BlockHandle_t LineBlockHandle;
    STBLIT_XYL_t*         ArcXYLArray;
    U32                   ArcSize = 0;
    STAVMEM_BlockHandle_t ArcBlockHandle;
    STBLIT_XYL_t*         tmpXYLArray;
    STAVMEM_BlockHandle_t tmpBlockHandle;
    S32                   yTop, yBot, YY[6];
    float                 XP = 0;
    float                 YP = 0;
    float                 XD;
    float                 YD;
    float                 XCenter;
    float                 YCenter;
    float                 Radius;
    float                 XS, YS, XE, YE;
    float                 A, B, C, D, E, F;
    S32                   Orientation = 0;
    S32                   SRadius;
    S32                   Points[8];
    S32                   PointsOnNormal[4];
    S32                   DirectionX, DirectionY, Xc, Yc;
    STGFX_Point_t         ProjectingPoints[4];
    float                 CheckDirection, CheckPoint;
    BOOL                  IsRadiusEven = FALSE;
    U16                   ExtraFlag = 0;
    U32                   ExtraSize = 0;
    STBLIT_XYL_t          ExtraXYLArray[10];
    U32                   NumYY = 4;


    tmpSize = Size = *NumXYL;
    *NumXYL = 0;

    if (CapStyle == STGFX_PROJECTING_CAP)
    {
        stgfx_CalcVertex(Xs, Ys, Xe, Ye, LineWidth, Vertex);

        Points[0] = Xs; Points[1] = Ys;
        Points[2] = Xe; Points[3] = Ye;

        FindPointOnNormal(Points, PointsOnNormal,
                          LineWidth << STGFX_INTERNAL_ACCURACY);
        if (!IsStart)
        {
            Points[4] = Xs; Points[5] = Ys;
            Points[6] = Xe; Points[7] = Ye;
        }
        ChooseThePoint(Points, PointsOnNormal, &DirectionX, &DirectionY,
                       IsStart);

        if (Xe != Xs)
        {
            CheckDirection = (float)DirectionY - (float)Ye -
                (((float)Ye - (float)Ys) / ((float)Xe - (float)Xs))
                     * ((float)DirectionX - (float)Xe);
            CheckPoint  = (float)Vertex[0].Y - (float)Ye -
                (((float)Ye - (float)Ys) / ((float)Xe - (float)Xs))
                     * ((float)Vertex[0].X - (float)Xe);
        }
        else  /* vertical line */
        {
            CheckDirection = CheckPoint = (float)Ye - (float)Ys;
            if (!IsStart)
            {
                GFXSWAP(Xe,Xs);
                GFXSWAP(Ye,Ys);
                CheckPoint = -CheckPoint;
            }
        }

        if (((CheckDirection < 0.F) && (CheckPoint < 0.F)) ||
            ((CheckDirection > 0.F) && (CheckPoint > 0.F)) ||
            ((CheckDirection == 0.F) && (CheckPoint == 0.F)))
        {
            ProjectingPoints[0].X = Xs; ProjectingPoints[0].Y = Ys;
            ProjectingPoints[1].X = Xe; ProjectingPoints[1].Y = Ye;
            ProjectingPoints[2].X = Vertex[3].X; ProjectingPoints[2].Y = Vertex[3].Y;
            ProjectingPoints[3].X = Vertex[1].X; ProjectingPoints[3].Y = Vertex[1].Y;
        }
        else
        {
            ProjectingPoints[0].X = Xs; ProjectingPoints[0].Y = Ys;
            ProjectingPoints[1].X = Xe; ProjectingPoints[1].Y = Ye;
            ProjectingPoints[2].X = Vertex[2].X; ProjectingPoints[2].Y = Vertex[2].Y;
            ProjectingPoints[3].X = Vertex[0].X; ProjectingPoints[3].Y = Vertex[0].Y;
        }

        Err = stgfx_FillSimplePolygon(Handle, 4, ProjectingPoints, &Size,
                                      XYLArray);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }

        *NumXYL = Size;

    }
    else if (CapStyle == STGFX_ROUND_CAP)
    {
        if (LineWidth == 2)
        {
            XYLArray[0].PositionX = FRACT_TO_INT(Xs);
            XYLArray[0].PositionY = FRACT_TO_INT(Ys);
            XYLArray[0].Length    = 0;
            /* *NumXYL = 0; above */
        }
        else
        {
            LineSize = GFXABS(FRACT_TO_INT(Ys) - FRACT_TO_INT(Ye)) + 1;

            Err = stgfx_AVMEM_malloc(Handle, LineSize*sizeof(STBLIT_XYL_t),
                                     &LineBlockHandle, (void**) &LineXYLArray);
            if (Err != ST_NO_ERROR)
            {
                return(Err);
            }

            Err = stgfx_2dRasterSimpleLine(Xs, Ys, Xe, Ye, &LineSize,
                                           LineXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle);
                return(Err);
            }
            YY[0] = FRACT_TO_INT(Ys);
            YY[1] = FRACT_TO_INT(Ye);

            ArcSize = 2 * LineWidth;

            SRadius = (LineWidth / 2) << STGFX_INTERNAL_ACCURACY;

            if (!IS_ODD(LineWidth))
            {
                SRadius -= (1 << (STGFX_INTERNAL_ACCURACY - 1));
            }
            Xc = (Xs + Xe) / 2;
            Yc = (Ys + Ye) / 2;

            XCenter = (float)Xc / (1 << STGFX_INTERNAL_ACCURACY);
            YCenter = (float)Yc / (1 << STGFX_INTERNAL_ACCURACY);
            Radius = (float)SRadius / (1 << STGFX_INTERNAL_ACCURACY);
            if(IS_ODD((U32)(Radius*2.F)))
            {
                IsRadiusEven = FALSE;
            }
            else
            {
                IsRadiusEven = TRUE;
            }
            if ((Ys > Yc) || ((Ys == Yc) && (Xs < Xc)))
                Orientation = 180*64;

            CalcConicCoefficients(Radius, Radius, Orientation,
                                  &A, &B, &C, &D, &E, &F, &XP, &YP, &XD, &YD);
            XS = (float)Xs / (1 << STGFX_INTERNAL_ACCURACY);
            YS = (float)Ys / (1 << STGFX_INTERNAL_ACCURACY);
            XE = (float)Xe / (1 << STGFX_INTERNAL_ACCURACY);
            YE = (float)Ye / (1 << STGFX_INTERNAL_ACCURACY);

            XS = XS - XCenter;
            YS = YCenter - YS;
            XE = XE - XCenter;
            YE = YCenter - YE;
            XS = XS + Radius;
            XE = XE + Radius;

            if (Orientation)
            {
                XS = XS -2*Radius;
                XE = XE -2*Radius;
            }
            if(IS_ODD((U32)(YCenter*2.F)))
            {
                if(YP >= 0.F)
                {
                    YP += 0.5F;
                }
                else
                {
                    YP -= 0.5F;
                }
            }
            if(IS_ODD((U32)(XCenter*2.F))||(IS_ODD((U32)(Radius*2.F)))) {
                if(XP >= 0.F)
                {
                    if(Orientation &&
                       ((IsRadiusEven == 0) &&
                        IS_ODD((U32)(XCenter*2.F))))
                    {
                        XP += 0.5F;
                        XS -= 0.5F;
                    }
                    else
                    {
                        XP -= 0.5F;
                        if((!IS_ODD((U32)(XS*2.F)))&&((XS - 0.5F) == XP)) {
                            XS -= 0.5F;
                        }
                    }
                }
                else
                {

                    if(Orientation &&
                       ((IsRadiusEven == 0) &&
                        IS_ODD((U32)(XCenter*2.F))))
                    {
                        XP -= 0.5F;
                        XS -= 0.5F;
                    }
                    else if(Orientation &&
                            ((IsRadiusEven == 1) &&
                             IS_ODD((U32)(XCenter*2.F))))
                    {
                        XP -= 0.5F;
                    }
                    else
                    {
                        XP += 0.5F;
                        if((!IS_ODD((U32)(XS*2.F)))&&((XS - 0.5F) == XP)) {
                            XS += 0.5F;
                        }
                    }
                }
            }
            if(XS == 0.F && ((YS + 0.5F) == 0 ||(YS - 0.5F) == 0)) {
                YS = YP;
                XS = XP;
            }
            if (YS == YE)
            {
                XP = XS;
                YP = YS;
            }
            Err = stgfx_AVMEM_malloc(Handle, ArcSize*sizeof(STBLIT_XYL_t),
                                     &ArcBlockHandle,
                                     (void**) &ArcXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle); 
                return(Err);
            }
            
            Err = Conic(XP, YP, XS, YS, XE, YE, A, B, C, D,
                        E, F, (XCenter-XD), (YCenter+YD), XD, YD,
                        &ArcSize, ArcXYLArray, FALSE, NULL, TRUE);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle);
                stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                return(Err);
            }
            if ((ArcXYLArray[0].PositionX != FRACT_TO_INT(Xs)) ||
                (ArcXYLArray[0].PositionY != FRACT_TO_INT(Ys)))
            {
                ExtraFlag = 1;
                ExtraSize = GFXABS(ArcXYLArray[0].PositionY - FRACT_TO_INT(Ys))
                    +1;
                Err = stgfx_2dRasterSimpleLine(Xs, Ys,
                                               ArcXYLArray[0].PositionX << STGFX_INTERNAL_ACCURACY,
                                               ArcXYLArray[0].PositionY << STGFX_INTERNAL_ACCURACY,
                                               &ExtraSize, ExtraXYLArray);
                if (Err != ST_NO_ERROR)
                {
                    stgfx_AVMEM_free(Handle, &LineBlockHandle);
                    stgfx_AVMEM_free(Handle, &ArcBlockHandle);
                    return(Err);
                }
            }
            YY[2] = YY[3] = ArcXYLArray[0].PositionY; for (i = 1; i < ArcSize; i++)
            {
                YY[2] = GFXMIN(YY[2], ArcXYLArray[i].PositionY);
                YY[3] = GFXMAX(YY[3], ArcXYLArray[i].PositionY);
            }
            if (ExtraFlag)
            {
                YY[4] = YY[5] = ExtraXYLArray[0].PositionY;
                for (i = 1; i < ExtraSize; i++)
                {
                    YY[4] = GFXMIN(YY[4], ExtraXYLArray[i].PositionY);
                    YY[5] = GFXMAX(YY[5], ExtraXYLArray[i].PositionY);
                }
                NumYY = 6;
            }
            CalcyTopyBot(NumYY, &yTop, &yBot, YY);

            Size = LineSize + ArcSize;
            if (ExtraFlag)
            {
                Size += ExtraSize;
            }
            Err = stgfx_AVMEM_malloc(Handle, Size*sizeof(STBLIT_XYL_t),
                                     &tmpBlockHandle,
                                     (void**) &tmpXYLArray);
            if (Err != ST_NO_ERROR)
            {
                stgfx_AVMEM_free(Handle, &LineBlockHandle); 
                stgfx_AVMEM_free(Handle, &ArcBlockHandle); 
                return(Err);
            }
            
            for (i = 0; i < LineSize; i++)
            {
                tmpXYLArray[i].PositionX = LineXYLArray[i].PositionX;
                tmpXYLArray[i].PositionY = LineXYLArray[i].PositionY;
                tmpXYLArray[i].Length    = LineXYLArray[i].Length;
            }
            for (i = 0; i < ArcSize; i++)
            {
                tmpXYLArray[i + LineSize].PositionX = ArcXYLArray[i].PositionX;
                tmpXYLArray[i + LineSize].PositionY = ArcXYLArray[i].PositionY;
                tmpXYLArray[i + LineSize].Length    = ArcXYLArray[i].Length;
            }
            if (ExtraFlag)
            {
                for (i = 0; i < ExtraSize; i++)
                {
                    tmpXYLArray[i + LineSize + ArcSize].PositionX =
                        ExtraXYLArray[i].PositionX;
                    tmpXYLArray[i + LineSize + ArcSize].PositionY =
                        ExtraXYLArray[i].PositionY;
                    tmpXYLArray[i + LineSize + ArcSize].Length    =
                        ExtraXYLArray[i].Length;
                }
            }

            CalcXYLArray(yTop, yBot, Size, tmpXYLArray, XYLArray);

            stgfx_AVMEM_free(Handle, &LineBlockHandle);
            stgfx_AVMEM_free(Handle, &ArcBlockHandle);
            stgfx_AVMEM_free(Handle, &tmpBlockHandle);

            (*NumXYL) = yBot - yTop + 1;
        }
    }
    if ((*NumXYL) > tmpSize)
        return(STGFX_ERROR_XYL_POOL_EXCEEDED);
    else
        return(ST_NO_ERROR);

}   /* end of stgfx_2dArcRasterCap() */

/******************************************************************************
Name        : stgfx_CalcCapVertex()
Description : compute vertex for capstyle projecting
Parameters  : X1,
              Y1,
              X2,
              Y2,
              LineWidth,
              CapVertex
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR
******************************************************************************/
static ST_ErrorCode_t stgfx_CalcCapVertex(
    S32            Xcap,
    S32            Ycap,
    S32            X,
    S32            Y,
    U16            LineWidth,
    STGFX_Point_t* CapVertex
)
{
    S32   dx, dy, iE, iNE, d, x, y;
    S32   YIncr = 0;
    S32   DeltaXCap, DeltaYCap;
    U8    InvCoord = 0;
    BOOL  Odd; /* set to 0 if Width is even, 1 if odd */
    
    if (Xcap >= X)
            {
                GFXSWAP(Xcap, X);
                GFXSWAP(Ycap, Y);
                InvCoord = 1;
            }
    else if (Xcap == X && Ycap > Y)
    {
        GFXSWAP(Ycap, Y);
        InvCoord = 1;
    }   

    Odd = ((LineWidth % 2)== 0) ? FALSE : TRUE;
  
    LineWidth = LineWidth << STGFX_INTERNAL_ACCURACY;
  
    dx = X - Xcap;
    dy = GFXABS(Ycap - Y);
    if (Y > Ycap)
        YIncr = 1;
    else
        YIncr = -1;
        
    if (dx >= dy)
    {
        d = 2 * dy - dx;
        iE = 2 * dy; 
        iNE = 2 * (dy - dx);
    }
    else
    {
        d = 2 * dx - dy;
        iE = 2 * dx; 
        iNE = 2 * (dx - dy);
    }
    
    x = Xcap;
    y = Ycap;
    
    while (((4*(x - Xcap) * (x - Xcap)) + (4*(y - Ycap) * (y - Ycap))) <
          (((LineWidth - (1<<STGFX_INTERNAL_ACCURACY)) *
           (LineWidth - (1<<STGFX_INTERNAL_ACCURACY)))))
    {
        if (d <= 0)
        {
            d += iE;
            if (dx >= dy)
                x += (1<<STGFX_INTERNAL_ACCURACY);
            else
                y += (YIncr<<STGFX_INTERNAL_ACCURACY);
        }
        else
        {
            d += iNE;
            x += (1<<STGFX_INTERNAL_ACCURACY);
            y += (YIncr<<STGFX_INTERNAL_ACCURACY);
        } 
    } /* end while loop */

    CapVertex->X = Xcap - (x - Xcap);
    CapVertex->Y = Ycap - (y - Ycap);
    
    if (InvCoord == 1)
    {
        DeltaXCap = x - Xcap;
        DeltaYCap = Ycap - y;
        CapVertex->X = X + DeltaXCap;
        CapVertex->Y = Y - DeltaYCap;
    }

    return(ST_NO_ERROR);

}   /* End of stgfx_CalcCapVertex */


/******************************************************************************
Name        : stgfx_CalcVertex()
Description : compute the 4 vertices of the thick line between two points
Parameters  : X1:
              Y1:
              X2:
              Y2:
              LineWidth,
              Vertex_p
Assumptions : 
Limitations : 
Returns     : void
******************************************************************************/
static void stgfx_CalcVertex(
    S32            X1,
    S32            Y1,
    S32            X2,
    S32            Y2,
    U16            LineWidth,
    STGFX_Point_t* Vertex_p
)
{
    S32   dx, dy, iE, iNE, d, x, y;
    S32   YIncr = 0;
    S32   i = -1;
    S32   XA1, YA1, XB1, YB1, XA2, YA2, XB2, YB2;
    S32   DeltaX1, DeltaY1, DeltaX2, DeltaY2;
    S32   xyArray[ 3 * YRES ];
    U8    InvCoord = 0;
    BOOL  Odd; /* set to 0 if Width is even, 1 if odd */

    if  (X1 > X2)
    {
        GFXSWAP(X1, X2);
        GFXSWAP(Y1, Y2);
        InvCoord = 1;
    }
    else if (X1 == X2 && Y1 > Y2)
    {
        GFXSWAP(Y1, Y2);
        InvCoord = 1;
    }
    
    Odd = ((LineWidth % 2) == 0) ? FALSE : TRUE;

    LineWidth = LineWidth << STGFX_INTERNAL_ACCURACY;
  
    dx = X2 - X1;
    dy = GFXABS(Y1 - Y2);
    if (Y2 > Y1)
        YIncr = 1;
    else
        YIncr = -1;
        
    if (dx >= dy)
    {
        d = 2 * dy - dx;
        iE = 2 * dy; 
        iNE = 2 * (dy - dx);
    }
    else
    {
        d = 2 * dx - dy;
        iE = 2 * dx; 
        iNE = 2 * (dx - dy);
    }
    
    /* the first element of xyArray is Xjoin, Yjoin */
    x = X1;
    y = Y1;
    i = 0;
    xyArray[i] = x;
    xyArray[i + 1] = y;

    while (((4*(x - X1)*(x - X1)) + 4*(y - Y1)*(y - Y1)) <
          (((LineWidth - (1<<STGFX_INTERNAL_ACCURACY)) *
           (LineWidth - (1<<STGFX_INTERNAL_ACCURACY)))))
    {
        if (d <= 0)
        {
            d += iE;
            if (dx >= dy)    
                x += (1<<STGFX_INTERNAL_ACCURACY);
            else
                y += (YIncr<<STGFX_INTERNAL_ACCURACY);
        }
        else
        {
            d += iNE;      
            x += (1<<STGFX_INTERNAL_ACCURACY);
            y += (YIncr<<STGFX_INTERNAL_ACCURACY);
        } 
    
        /* all the points must be recorded in xyArray because if only x
        is incremented, rotating of 90 degrees, y always is incremented!
        firstly storage the rotated point through a anticlockwise angle 
        of 90 degrees */
        i += 2;
        xyArray[i] = X1 + (y - Y1);
        xyArray[i + 1] = Y1 - (x - X1);
    
        /* now storage the rotated point through an clockwise of 90 degrees */
        i += 2;
        xyArray[i] = X1 - ( y - Y1);  
        xyArray[i + 1] = Y1 + (x - X1);
    
    } /* end while loop */
  
    if (Odd == FALSE) 
    {
        i -= 2; /* the lower boundary does not have the last pixel line;
                record the coordinates of the last loop because they are
                the coordinates of the edge in the first endpoint */
        XA2 = xyArray[i];
        YA2 = xyArray[i + 1];
        XA1 = xyArray[i - 2];
        YA1 = xyArray[i - 1];
    } 
    else 
    {
        XA1 = xyArray[i];
        YA1 = xyArray[i + 1];
        XA2 = xyArray[i - 2];
        YA2 = xyArray[i - 1];
    }

    DeltaX1 = X1 - XA1;
    DeltaY1 = YA1 - Y1;
    DeltaX2 = XA2 - X1;
    DeltaY2 = Y1 - YA2;
  
    /* now it is possible to calculate also the coordinates of the edge
    in the other endpoint */
    XB1 = X2 - DeltaX1;
    YB1 = Y2 + DeltaY1;
    XB2 = X2 + DeltaX2;
    YB2 = Y2 - DeltaY2;

    Vertex_p[0].X = XA2; /* M or M' */
    Vertex_p[0].Y = YA2; /*   //   */
    Vertex_p[1].X = XA1; /* N or N' */
    Vertex_p[1].Y = YA1; /*   //   */ 
    Vertex_p[2].X = XB2; /* P or P' */
    Vertex_p[2].Y = YB2; /*   //   */
    Vertex_p[3].X = XB1; /* O or O' */
    Vertex_p[3].Y = YB1; /*   //   */
    
    if (InvCoord == 1)
    {
        Vertex_p[0].X = XB1; /* M or M' */
        Vertex_p[0].Y = YB1; /*   //   */
        Vertex_p[1].X = XB2; /* N or N' */
        Vertex_p[1].Y = YB2; /*   //   */
        Vertex_p[2].X = XA1; /* P or P' */
        Vertex_p[2].Y = YA1; /*   //   */
        Vertex_p[3].X = XA2; /* O or O' */
        Vertex_p[3].Y = YA2; /*   //   */
    } 

}   /* End of stgfx_CalcVertex */

/*******************************************************************************
ST_ErrorCode_t stgfx_FillGenericPolygon()
*******************************************************************************/
ST_ErrorCode_t stgfx_FillGenericPolygon(
  U32                       BmHeight,    /* bitmap height */
  S32                       yTop,        /* topY of the sides */
  S32                       yBot,        /* bottom Y of the sides */
  stgfx_Side_t*             Sides_p,     /* sides data */
  U32                       SidesNum,    /* number of sides */
  STBLIT_XYL_t*             XYLFillData, /* where to place filling XYL data */
  U32*                      XYLLength,   /* length of the XYL array */
  STGFX_PolygonFillMode_t   PolygonFillMode /* filling mode */
)
{
  gfxIntsect   Isecs[STGFX_MAX_NUM_POLY_POINTS];
                 /* intersections between sides and current scanline */
  U32          IsecNum;     /* number of intersections of a given scanline */
  S16          ScanLineUp;  /* "Up" count of a scanline */
  U16          Cnt, idx;    /* counters */
  S32          y;           /* y coordinate of a scanline */
  U32          i;           /* counter */
  S32          k;           /* another counter */
  S32          EndY;        /* lower y coordinate */
  U32          Size;        /* size of the XYL array given to the function
                                by its caller, it cannot be exceeded */
  Size = *XYLLength;
  (*XYLLength) = 0;

  if (yBot < 0)
  {
    return (ST_NO_ERROR); /* nothing to do */
  }

  EndY = GFXMIN((BmHeight-1), yBot);

  for(y = yTop; y <= EndY; y++)
  {
    IsecNum = 0;
    for(i=0; i<SidesNum; i++) /* loop on segments */
    {
      Isecs[i].xflag = FALSE;
      stgfx_findIsec(y, &Sides_p[i], &Isecs[i]);
      if(Isecs[i].xflag)
        IsecNum++;
    }
    if(IsecNum == 0)
      continue;

    qsort(Isecs, SidesNum, sizeof(gfxIntsect), stgfx_compIsec);

    if(PolygonFillMode == STGFX_EVENODD_FILL)
    {
      ScanLineUp = 0;
      for(i=0; i<IsecNum-1; i++)
      {
        if(Isecs[i].X2 < Isecs[i+1].X1)
        {
          /* segment i ends before segment i+1 begins */
          ScanLineUp += Isecs[i].Up; /* (moving by 1 each time is all that's needed) */
          
          /* second condition below ensures nonzero length (important for GX1) */
          if((ScanLineUp & 0x1) && (Isecs[i+1].X1-Isecs[i].X2-1 > 0))
          {
            XYLFillData[*XYLLength].PositionX = Isecs[i].X2 + 1;
            XYLFillData[*XYLLength].PositionY = y;
            XYLFillData[*XYLLength].Length = Isecs[i+1].X1-Isecs[i].X2-1;
            (*XYLLength)++;
          }
        }
        else
        {
          /* segments i and i+1 overlap - skip past any more overlaps */
          for(idx=i; idx < IsecNum-1; idx++)
          {
            if(Isecs[idx].X2 < Isecs[idx+1].X1)
            {
              break;
            }
            if(Isecs[idx].X2 > Isecs[idx+1].X2)
            {
              Isecs[idx+1].X2 = Isecs[idx].X2;
            }
          }

          for(k=i; k<=idx; k++)
            ResetDubValues(&(Isecs[k]), idx+1-k);

          for(Cnt=i; Cnt<idx; Cnt++)
          {
            ScanLineUp += Isecs[Cnt].Up;
          }
          i = idx-1;
        }
      }
    }
    else if(PolygonFillMode == STGFX_WINDING_FILL)
    {
      ScanLineUp = 0;
      for(i=0; i<IsecNum-1; i++)
      {
        if(Isecs[i].X2 < Isecs[i+1].X1)
        {
          /* segment i ends before segment i+1 begins */
          ScanLineUp += Isecs[i].Up;

          /* second condition below ensures nonzero length (important for GX1) */
          if(ScanLineUp && (Isecs[i+1].X1-Isecs[i].X2-1 > 0))
          {
            XYLFillData[*XYLLength].PositionX = Isecs[i].X2 + 1;
            XYLFillData[*XYLLength].PositionY = y;
            XYLFillData[*XYLLength].Length = Isecs[i+1].X1-Isecs[i].X2-1;
            
#if 0       /* TEMPORARY DEBUG: look for overlong Thickness elements */
            if(XYLFillData[*XYLLength].Length > 100)
            {
              printf("stgfx_FillGenericPolygon: length %u element generated:\n",
                     XYLFillData[*XYLLength].Length);
              for(idx = 0; idx < IsecNum; ++idx)
              {
                printf("  Up %i X1 %i X2 %i Spin %u\n", (S32) Isecs[idx].Up,
                       Isecs[idx].X1, Isecs[idx].X2, (U32) Isecs[idx].Spin);
              }
            }
#endif            
            (*XYLLength)++;
          }
        }
        else
        {
          /* segments i and i+1 overlap */
          for(idx=i; idx < IsecNum-1; idx++)
          {
            if(Isecs[idx].X2 < Isecs[idx+1].X1)
            {
              break;
            }
            if(Isecs[idx].X2 > Isecs[idx+1].X2)
            {
              Isecs[idx+1].X2 = Isecs[idx].X2;
            }
          }
          
          for(k=i; k<=idx; k++)
            ResetDubValues(&(Isecs[k]), idx+1-k);

          for(Cnt=i; Cnt<idx; Cnt++)
          {
            ScanLineUp += Isecs[Cnt].Up;
          }
          i = idx-1;
        }
      }
    }
  }
  
  if(*XYLLength > Size)
  {
    return(STGFX_ERROR_XYL_POOL_EXCEEDED);
  }

  return(ST_NO_ERROR);
} /* end of stgfx_FillGenericPolygon() */



/*******************************************************************************
Name        : stgfx_FillSimplePolygon()
Description : this function is used to fill simple polygons like lines, caps 
              and joins
Parameters  : Handle   : Handle of the connection
              NumPoints: 
              Points_p : array of points
              NumXYL   : counter of the number of XYLArray elements
              XYLArray :
Assumptions : 
Limitations : 
Returns     : ST_NO_ERROR, STGFX_ERROR_XYL_POOL_EXCEEDED, ST_ERROR_NO_MEMORY
*******************************************************************************/
ST_ErrorCode_t stgfx_FillSimplePolygon(
  STGFX_Handle_t Handle,
  U32            NumPoints,
  STGFX_Point_t* Points_p,
  U32*           NumXYL ,
  STBLIT_XYL_t*  XYLArray
)
{
    ST_ErrorCode_t        Err;
    U32                   i;
    S32                   yTop, yBot;  /* top and bottom Y coordinates */ 
    U32                   Size, tmpSize;
    STBLIT_XYL_t*         tmpXYLArray;
    STAVMEM_BlockHandle_t tmpXYLBlockHandle;
    S32                   X1, Y1, X2, Y2;
    U32                   Length;
    S32                   YY[6];
    

    tmpSize = Size = *NumXYL;
    *NumXYL = 0; 
    
    for(i = 0; i < NumPoints; i++)
    {
        YY[i] = FRACT_TO_INT(Points_p[i].Y);
    }
    CalcyTopyBot(NumPoints, &yTop, &yBot, YY);

    
    Length = (yBot - yTop + 1) * (NumPoints - 1);
    Err = stgfx_AVMEM_malloc(Handle, Length*sizeof(STBLIT_XYL_t),
                             &tmpXYLBlockHandle, (void**) &tmpXYLArray);
    if (Err != ST_NO_ERROR)
    {
        return(Err);
    }
    
    for(i = 0; i <= (NumPoints - 1); i++)
    {        
        X1 = Points_p[i].X; Y1 = Points_p[i].Y;

        if (i == (NumPoints - 1))
        {
            X2 = Points_p[0].X; Y2 = Points_p[0].Y;
        }
        else
        {
            X2 = Points_p[i+1].X; Y2 = Points_p[i+1].Y;
        }

        Size = GFXABS(FRACT_TO_INT(Y2) - FRACT_TO_INT(Y1)) + 10;        
        Err = stgfx_2dRasterSimpleLine(X1, Y1, X2, Y2, &Size,
                                       (tmpXYLArray + (*NumXYL)));
        if (Err != ST_NO_ERROR)
        {
            stgfx_AVMEM_free(Handle, &tmpXYLBlockHandle);
            return(Err);
        } 
        (*NumXYL) += Size;
    }

    CalcXYLArray(yTop, yBot, (*NumXYL), tmpXYLArray, XYLArray);

    (*NumXYL) = yBot - yTop + 1;
    
    stgfx_AVMEM_free(Handle, &tmpXYLBlockHandle);

    if ((*NumXYL) > tmpSize)
        return(STGFX_ERROR_XYL_POOL_EXCEEDED);
    else
        return(ST_NO_ERROR);
}
/* End of stgfx_FillSimplePolygon() */


/******************************************************************************
Name        : stgfx_RemoveXYLOverlaps()
Description : this function is used to remove any possible pixel overlap if
              alumode != copy
Parameters  : XYLArray1 :
              NumXYL1   : counter of the number of XYLArray1 elements
              XYLArray2 : this is the one we modify - we may reduce the number
                          of elements on GX1 in order to avoid 0 length ones
              NumXYL2   : counter of the number of XYLArray2 elements
Assumptions : No overlaps within the two arrays separately, only between the two
              Optimised for XYLArray2 being the shorter
Limitations :
Returns     :
******************************************************************************/
void stgfx_RemoveXYLOverlaps(
   STBLIT_XYL_t* XYLArray1,
   U32*          NumXYL1,
   STBLIT_XYL_t* XYLArray2,
   U32*          NumXYL2
)
{
  /* Where the input ranges are monatonic in y, a faster merge would be
    possible. Only dashed arcs violate this, I believe */

  STBLIT_XYL_t *Src1_p, *Src2_p; /* source positions in XYLArray1, XYLArray2 */
  STBLIT_XYL_t *Src1Limit_p, *Src2Limit_p;
  STBLIT_XYL_t *Dst1_p, *Dst2_p; /* destination posn's in XYLArray1, XYLArray2 */
  S32 Diff;
  
  XYLREM_Print(("RemoveXYLOverlaps: %u %u\n", *NumXYL1, *NumXYL2));
  
  /* setup initial loop limits */
  Src1Limit_p = XYLArray1 + *NumXYL1;
  Src2Limit_p = XYLArray2 + *NumXYL2;
  
  /* fake inner loop 'keep all' end condition incase
    outer loop never executes (*NumXYL2 == 0) */
  Dst1_p = Src1Limit_p;
  
  for (Src2_p = Dst2_p = XYLArray2; (Src2_p < Src2Limit_p); ++Src2_p)
  {
    /* if (Src2_p->Length == 0) continue; - without ++Dst2_p */
    /* would be an optimisation, but isn't because it should never occur */
    
    if(Src2_p != Dst2_p)
    {
      /* copy basic element 2 values (except Reserved field) */
      Dst2_p->PositionX = Src2_p->PositionX;
      Dst2_p->PositionY = Src2_p->PositionY;
      Dst2_p->Length    = Src2_p->Length;
      /* work on Dst2_p from now on to keep changes between src 1 iterations */
    }

    for (Src1_p = Dst1_p = XYLArray1; (Src1_p < Src1Limit_p); ++Src1_p)
    {
      /* copy basic element 1 values */
      if(Src1_p != Dst1_p)
      {
        Dst1_p->PositionX = Src1_p->PositionX;
        Dst1_p->PositionY = Src1_p->PositionY;
        Dst1_p->Length    = Src1_p->Length;
        /* work on Dst1_p from now on for consistency */
      }
      
      if((Dst1_p->PositionY == Dst2_p->PositionY) &&
         (Dst1_p->PositionX < (Dst2_p->PositionX + (S32)Dst2_p->Length)) &&
         (Dst2_p->PositionX < (Dst1_p->PositionX + (S32)Dst1_p->Length)))
      {
        /* They overlap. It is important that we do not add length to either
          side, so as to not invalidate prior removal of overlaps with other
          arrays, or the requirement that each array is free of self-overlaps.
          We may have to take an element from either array, if it is
          completely covered by one from the other */
          
        Diff = Dst2_p->PositionX - Dst1_p->PositionX;
        XYLREM_Print(("Overlap Diff %i ", Diff));
        if(Diff >= 0) /* both start together, or element 1 starts first */
        {
          /* keep element 1 as it is and start element 2 later */
          /* (S32) shouldn't matter as old Diff >= 0 for them to overlap */
          Diff = (S32)Dst1_p->Length - Diff;
          if(Dst2_p->Length < Diff)
          {
            /* element from array 2 completely subsumed */
            Dst2_p->Length = 0; /* needed to avoid ++Dst2_p below */
            
            /* we could leave it here and loop over the remaining src1 - a zero-
              length src2 will never pass the overlap test above. But it's a bit
              quicker to break, before which we must memmove all remaining src1
              and update Dst1_p to give the correct Src1Limit_p later */
#if 0
            if(Src1_p != Dst1_p)
            {
              ++Src1_p; ++Dst1_p; /* already copied, shouldn't be 0 length */
              memmove(Dst1_p, Src1_p, (U32)Src1Limit_p - (U32)Src1_p);
            }
            
            /* update Dst1_p for correct Src1Limit_p update below */
            /* note no (U32) cast here - we need to count elements not bytes */
            Dst1_p += Src1Limit_p - Src1_p;
            break;
#endif
          }
          else
          {
            /* retain both elements */
            Dst2_p->PositionX += Diff;
            Dst2_p->Length -= Diff;
          }
          
          XYLREM_Print(("Diff' %i, element 2 new length %u\n",
                        Diff, Dst2_p->Length));
        }
        else /* element 2 starts first */
        {
          /* keep element 2 as it is and start element 1 later */
          /* (S32) matters here because old Diff < 0 */
          Diff = (S32)Dst2_p->Length + Diff;
          if(Dst1_p->Length < Diff)
          {
            /* element from array 1 completely subsumed */
            Dst1_p->Length = 0;
          }
          else
          {
            /* retain both elements */
            Dst1_p->PositionX += Diff;
            Dst1_p->Length -= Diff;
          }
          
          XYLREM_Print(("Diff' %i, element 1 new length %u\n",
                        Diff, Dst1_p->Length));
        }
      }
      
      /* preserve XYLArray1 segment if appropriate */
      if(Dst1_p->Length != 0) ++Dst1_p;
    }
    
    /* revise upper limit for array 1 */
    Src1Limit_p = Dst1_p;
    
    /* preserve XYLArray2 segment if appropriate */
    if(Dst2_p->Length != 0) ++Dst2_p;
  }
  
  /* store number of elements that now remain in the arrays */
  *NumXYL1 = Dst1_p - XYLArray1;
  *NumXYL2 = Dst2_p - XYLArray2;
  
  XYLREM_Print(("... complete %u %u\n", *NumXYL1, *NumXYL2));
}

#if 0 /* not currently used */
/******************************************************************************
Name        : stgfx_RemoveZeroXYL()
Description : Shortens an XYL array, removing zero-length elements
Parameters  : XYLArray : XYL array to process
              NumXYL   : points to count of elements, which we will update
Assumptions : 
Limitations :
Returns     :
******************************************************************************/
void stgfx_RemoveZeroXYL(
   STBLIT_XYL_t* XYLArray,
   U32*          NumXYL
)
{
    int i, j = 0; /* i is source index, j next destination */
    
    for(i = 0; i < *NumXYL; ++i)
    {
        if(XYLArray[i].Length != 0)
        {
            if(i != j) XYLArray[j] = XYLArray[i];
            ++j;
        }
        else
        {
            printf("stgfx_RemoveZeroXYL:"
                   " zero length element found at index %i\n", i);
        }
    }
    
    *NumXYL = j;
}
#endif

/*******************************************************************************
Name        : stgfx_CalcLimitJoin()
Description : this function is used to compute the exact limit join
Parameters  : X1        :
              Y1        :
              X2        :
              Y2        :
              X3        :
              Y3        :
              JoinStyle :
              LineWidth :
              CheckJoin : verify if lines are / to the bitmap and // between 
                          them
              Points_p  : array of points
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t stgfx_CalcLimitJoin(
                                   S32               X1,
                                   S32               Y1,
                                   S32               Xjoin,
                                   S32               Yjoin,
                                   S32               X3,
                                   S32               Y3,
                                   S32               XintArc,
                                   S32               YintArc,
                                   S32               XextArc,
                                   S32               YextArc,
                                   stgfx_JoinSector  JoinSector,
                                   STGFX_JoinStyle_t JoinStyle,
                                   U16               LineWidth,
                                   float*            CheckJoin,
                                   U32               JoinPoints,
                                   STGFX_Point_t*    Points_p
                                   )
{
    STGFX_Point_t  JoinVertex_p[4];
    S32            Mx, My, Nx, Ny, Ox, Oy, Px, Py;
    S32            M1x, M1y, N1x, N1y, O1x, O1y, P1x, P1y;
    S32            Ax, Ay, Bx, By;
    float          m, m1;
    S32            DeltaInt, DeltaExt, DeltaM1x, DeltaM1y,
                   DeltaN1x, DeltaN1y;

    stgfx_CalcVertex(Xjoin, Yjoin, X1, Y1, LineWidth, JoinVertex_p);

    Mx = JoinVertex_p[0].X; My = JoinVertex_p[0].Y; /* M */
    Nx = JoinVertex_p[1].X; Ny = JoinVertex_p[1].Y; /* N */
    if (JoinStyle == STGFX_RECTANGLE_JOIN)
    {
        Px = JoinVertex_p[2].X; Py = JoinVertex_p[2].Y; /* P */
        Ox = JoinVertex_p[3].X; Oy = JoinVertex_p[3].Y; /* O */
    }

    stgfx_CalcVertex(Xjoin, Yjoin, X3, Y3, LineWidth, JoinVertex_p);

    M1x = JoinVertex_p[0].X; M1y = JoinVertex_p[0].Y; /* M' */
    N1x = JoinVertex_p[1].X; N1y = JoinVertex_p[1].Y; /* N' */
    if (JoinStyle == STGFX_RECTANGLE_JOIN)
    {
        P1x = JoinVertex_p[2].X; P1y = JoinVertex_p[2].Y; /* P' */
        O1x = JoinVertex_p[3].X; O1y = JoinVertex_p[3].Y; /* O' */
    }

    if (JoinSector==ArcLine)
    {
        if ((M1x==XintArc && M1y==YintArc) && (N1x==XextArc && N1y==YextArc) ||

            (M1x==XextArc && M1y==YextArc) && (N1x==XintArc && N1y==YintArc))
        {
            M1x = M1x; M1y = M1y;
            N1x = N1x; N1y = N1y;
        }
        else
        {
            DeltaInt=((M1x-XintArc)*(M1x-XintArc)+(M1y-YintArc)*(M1y-YintArc));

            DeltaExt=((M1x-XextArc)*(M1x-XextArc)+(M1y-YextArc)*(M1y-YextArc));

            if (DeltaInt > DeltaExt)
            {
                DeltaM1x = XextArc - M1x;
                DeltaM1y = YextArc - M1y;
                DeltaN1x = XintArc - N1x;
                DeltaN1y = YintArc - N1y;
                M1x = XextArc; M1y = YextArc;
                N1x = XintArc; N1y = YintArc;
            }
            else
            {
                DeltaM1x = XintArc - M1x;
                DeltaM1y = YintArc - M1y;
                DeltaN1x = XextArc - N1x;
                DeltaN1y = YextArc - N1y;
                M1x = XintArc; M1y = YintArc;
                N1x = XextArc; N1y = YextArc;
            }
            if (JoinStyle == STGFX_RECTANGLE_JOIN) {
                P1x += DeltaM1x; P1y += DeltaM1y;
                O1x += DeltaN1x; O1y += DeltaN1y;
            }
        }
    }

    if (Xjoin != X1)
        *CheckJoin = (float)Y3 - (float)Yjoin -
            (((float)Y1 - (float)Yjoin) / ((float)X1 - (float)Xjoin))
                 * ((float)X3 - (float)Xjoin);
    else  /* vertical line */
    {
        *CheckJoin = (float)Y3 - (float)Y1;
        if (Y3 == Y1 && X3 != X1)
        {
            if (X3 > Xjoin)
                *CheckJoin += 1;
            else
                *CheckJoin -= 1;
        }
    }

    if (*CheckJoin < 0.F)
    {
        if (Xjoin >= X1)
        {
            if (Xjoin==X1 && Yjoin < Y1 && X3 > X1) {
                Points_p[0].X = N1x; Points_p[0].Y = N1y;
                Points_p[1].X = Mx; Points_p[1].Y = My;
                Points_p[2].X = M1x; Points_p[2].Y = M1y;
                Points_p[3].X = Nx; Points_p[3].Y = Ny;
                if (JoinStyle == STGFX_RECTANGLE_JOIN)
                {
                    Ax = P1x; Ay = P1y;
                    Bx = Ox; By = Oy;
                }
            }
            else
            {
                Points_p[0].X = Nx; Points_p[0].Y = Ny;
                Points_p[1].X = M1x; Points_p[1].Y = M1y;
                Points_p[2].X = Mx; Points_p[2].Y = My;
                Points_p[3].X = N1x; Points_p[3].Y = N1y;
                if (JoinStyle == STGFX_RECTANGLE_JOIN)
                {
                    Ax = Px; Ay = Py;
                    Bx = O1x; By = O1y;
                }
            }
        }
        else
        {
            Points_p[0].X = N1x; Points_p[0].Y = N1y;
            Points_p[1].X = Mx; Points_p[1].Y = My;
            Points_p[2].X = M1x; Points_p[2].Y = M1y;
            Points_p[3].X = Nx; Points_p[3].Y = Ny;
            if (JoinStyle == STGFX_RECTANGLE_JOIN)
            {
                Ax = P1x; Ay = P1y;
                Bx = Ox; By = Oy;
            }
        }
    }
    if (*CheckJoin > 0.F)
    {
        if (Xjoin > X1)
        {
            Points_p[0].X = N1x; Points_p[0].Y = N1y;
            Points_p[1].X = Mx; Points_p[1].Y = My;
            Points_p[2].X = M1x; Points_p[2].Y = M1y;
            Points_p[3].X = Nx; Points_p[3].Y = Ny;
            if (JoinStyle == STGFX_RECTANGLE_JOIN)
            {
                Ax = P1x; Ay = P1y;
                Bx = Ox; By = Oy;
            }
        }
        else
        {
            Points_p[0].X = Nx; Points_p[0].Y = Ny;
            Points_p[1].X = M1x; Points_p[1].Y = M1y;
            Points_p[2].X = Mx; Points_p[2].Y = My;
            Points_p[3].X = N1x; Points_p[3].Y = N1y;
            if (JoinStyle == STGFX_RECTANGLE_JOIN)
            {
                Ax = Px; Ay = Py;
                Bx = O1x; By = O1y;
            }
        }
    }

    if (JoinStyle == STGFX_RECTANGLE_JOIN && *CheckJoin != 0) {
        if ((Points_p[2].X == Ax) && (Points_p[3].Y == By)) {
            if ((X1 == Xjoin) && (Y1 < Yjoin) && (X3 < Xjoin) && (Y3 == Yjoin))

            {
                Points_p[4].X = Points_p[0].X;
                Points_p[4].Y = Points_p[1].Y;
                GFXSWAP(Points_p[0].X, Points_p[3].X);
                GFXSWAP(Points_p[0].Y, Points_p[3].Y);
                GFXSWAP(Points_p[1].X, Points_p[2].X);
                GFXSWAP(Points_p[1].Y, Points_p[2].Y);
            }
            else
            {
                Points_p[4].X = Points_p[2].X;
                Points_p[4].Y = Points_p[3].Y;
            }
        }
        else if (Points_p[2].Y == Ay && Points_p[3].X == Bx) {
            Points_p[4].X = Points_p[3].X;
            Points_p[4].Y = Points_p[2].Y;
        }
        else if (Points_p[2].X == Ax && Points_p[3].X != Bx) {
            Points_p[4].X = Ax;
            m1 = ((float)Points_p[3].Y - (float)By) /
                ((float)Points_p[3].X - (float)Bx);
            Points_p[4].Y = (S32)(m1 * ((float)Points_p[4].X -
                                        (float)Bx) + (float)By);
        }
        else if (Points_p[2].X != Ax && Points_p[3].X == Bx) {
            Points_p[4].X = Bx;
            m  = ((float)Points_p[2].Y - (float)Ay) /
                ((float)Points_p[2].X - (float)Ax);
            Points_p[4].Y = (S32)(m * ((float)Points_p[4].X -
                                       (float)Ax) + (float)Ay);
        }
        else
        {
            if ((GFXABS(FRACT_TO_INT(Points_p[0].X) -
                        FRACT_TO_INT(Points_p[1].X) == 0) &&
                 GFXABS(FRACT_TO_INT(Points_p[0].Y) -
                        FRACT_TO_INT(Points_p[1].Y) == 1)) ||
                (GFXABS(FRACT_TO_INT(Points_p[0].X) -
                        FRACT_TO_INT(Points_p[1].X) == 1) &&
                 GFXABS(FRACT_TO_INT(Points_p[0].Y) -
                        FRACT_TO_INT(Points_p[1].Y) == 0))
                )
            {
                Points_p[4].X = (Points_p[2].X + Points_p[3].X) / 2;
                Points_p[4].Y = (Points_p[2].Y + Points_p[3].Y) / 2;
            }
            else
            {
                m  = ((float)Points_p[2].Y - (float)Ay) /
                    ((float)Points_p[2].X - (float)Ax);
                m1 = ((float)Points_p[3].Y - (float)By) /
                    ((float)Points_p[3].X - (float)Bx);

                Points_p[4].X = (S32)((m * (float)Points_p[2].X -
                                       m1 * (float)Points_p[3].X -
                                       (float)Points_p[2].Y + (float)Points_p[3].Y) / (m - m1));
                Points_p[4].Y = (S32)(m * ((float)Points_p[4].X -
                                           (float)Points_p[2].X) + (float)Points_p[2].Y);
            }
        }
        GFXSWAP(Points_p[3].X, Points_p[4].X); GFXSWAP(Points_p[3].Y, Points_p[4].Y);
    }
    return(ST_NO_ERROR);

} /* End of stgfx_CalcLimitJoin() */


/*******************************************************************************
FindPointOnNormal()

Input  data order: X1, Y1, X2, Y2
Output data order: X1, Y1, X2, Y2
*******************************************************************************/
void FindPointOnNormal(S32* InP, S32* OutP, U32 R)
{
  if(InP[1]==InP[3]) /* vertical */
  {
    double C;
    
    C = (double)InP[1]*(double)InP[1] +
        ((double)InP[0]-(double)InP[2])*((double)InP[0]-(double)InP[2])/4. -
        (double)R*(double)R;
    C = sqrt((double)InP[3]*(double)InP[3] - C);
    OutP[0] = OutP[2] = (InP[0]+InP[2])/2;
    OutP[1] = InP[1] + (S32)C;
    OutP[3] = InP[1] - (S32)C;
    return;
  }
  else /* any other case */
  {
    double m, q, A, B, C;
   
    m = ((double)InP[0]-(double)InP[2])/((double)InP[3]-(double)InP[1]);
    q = (((double)InP[1] + (double)InP[3]) -
         m * ((double)InP[0] + (double)InP[2]))/2.;
    A = 1. + m*m;
    B = m*(q - (double)InP[3]) - (double)InP[2];
    C = (double)InP[2]*(double)InP[2] +
        (q - (double)InP[3])*(q - (double)InP[3]) - (double)R*(double)R;
    C = sqrt(B*B - A*C);
    OutP[0] = (S32)((-B + C)/A);
    OutP[1] = (S32)(m*OutP[0] + q);
    OutP[2] = (S32)((-B - C)/A);
    OutP[3] = (S32)(m*OutP[2] + q);
    return;      
  }
}


/*******************************************************************************
Name        : stgfx_CalcLimitDashedLine()
Description : this function is used to compute the exact length of a dashed line
              with LineWidth > 1
Parameters  : X1        :
              Y1        :
              X2        :
              Y2        :
              LineStyle :
              LineWidth :
              NumXYL    :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stgfx_CalcLimitDashedLine(
    S32            X1,
    S32            Y1,
    S32            X2,
    S32            Y2,
    U8*            LineStyle,
    U16            LineWidth,
    U32*           NumXYL
)
{
    S32            DeltaX, DeltaY;
    double         Length;
    S32            XA;
    S32            YA;
    S32            XB = 0;
    S32            YB = 0;
    S32            D1 = 0;
    S32            D2 = 0;
    S32            i = 0, j = 0;
    U32            Size;
    STGFX_Point_t  Vertex[4];
    S32            yTop = 0, yBot = 0;
    S32            YY[4];
    

    if (LineWidth > 1)
    {
        while(LineStyle[j] != 0)
        {
            j++;
        }
        for (i = 0; i < (j-1); i+=2)
        {
            if (LineStyle[i] == 1)
            {
                return(STGFX_ERROR_INVALID_GC);
            }
        }
    }
    i = 0;
    j = 0;
    
    if  (X1 > X2)
    {
        GFXSWAP(X1, X2);
        GFXSWAP(Y1, Y2);
    }
    else if (X1 == X2 && Y1 > Y2)
        GFXSWAP(Y1, Y2);
    
    *NumXYL = 0;
    
    DeltaX = X2 - X1;
    DeltaY = Y2 - Y1;
    Length = (sqrt(((double)DeltaX * (double)DeltaX) +
                  ((double)DeltaY * (double)DeltaY)));
           
    while (D1 <= (S32)Length)
    {
        D2 = D1 + ((LineStyle[i] - 1) << STGFX_INTERNAL_ACCURACY);
        XA = (S32)(X1 + D1 * (DeltaX / Length));
        YA = (S32)(Y1 + D1 * (DeltaY / Length));
        XB = (S32)(X1 + D2 * (DeltaX / Length));
        YB = (S32)(Y1 + D2 * (DeltaY / Length));

        if ((XB > X2) || ((XB == X2) && (YB > Y2)))
        {
            XB = X2;
            YB = Y2;
        }
        
        stgfx_CalcVertex(XA, YA, XB, YB, LineWidth, Vertex);

        for(j = 0; j < 4; j++)
        {
            YY[j] = FRACT_TO_INT(Vertex[j].Y);
        }
        CalcyTopyBot(4, &yTop, &yBot, YY);

        
        Size = yBot - yTop + 1;
        (*NumXYL) += Size;
        
        /* advance over filled segment and gap segment, if any */
        D1 += ((LineStyle[i] + LineStyle[i + 1]) << STGFX_INTERNAL_ACCURACY);
        if ((LineStyle[i + 1] == 0) || (LineStyle[i + 2] == 0))
        {
            i = 0; /* end of array reached; restart from beginning */
        }
        else
        {
            i += 2; /* next filled segment */
        }
    }

    return(ST_NO_ERROR);
    
}   /* End of CalcLimitDashedLine */


/*******************************************************************************
Name        : CalcyTopyBot()
Description : Find the top and bottom y values in the supplied array
Parameters  : 
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void CalcyTopyBot(U32 NumPoints, S32* yTop, S32* yBot, S32* YY)
{
    U32 i;

    *yTop = *yBot = YY[0];
    for (i = 1; i < NumPoints; i++)
    {
        *yTop = GFXMIN(*yTop, YY[i]);
        *yBot = GFXMAX(*yBot, YY[i]);
    }
}


/*******************************************************************************
Name        : CalcXYLArray()
Description : Find the XYLArray that covers the supplied simple boundary
              and the region within it. Always writes exactly 1+yBot-yTop
              elements
Parameters  : 
Assumptions : At least one element in tmpXYLArray for each y, yTop <= y <= yBot
Limitations :
Returns     :
*******************************************************************************/
void CalcXYLArray(
    S32           yTop, 
    S32           yBot, 
    U32           Size, 
    STBLIT_XYL_t* tmpXYLArray, 
    STBLIT_XYL_t* XYLArray
)
{
    U32 i, flag = 0;
    S32 y, xMin, x1, x2, leng;
    
    for (y = yTop; y <= yBot; y++)
    {
        flag = 0;
        for (i = 0; i < Size; i++)
        {
            /* go through boundary aggregating segments to end with one
              that goes from the leftmost to rightmost point encountered */

            if (y == tmpXYLArray[i].PositionY)
            {
                if (flag == 0)
                {
                    xMin = tmpXYLArray[i].PositionX;
                    leng = tmpXYLArray[i].Length;
                    flag = 1;
                }
                else
                {
                    if(flag == 2)
                    {
                        /* expected, eg, where a segment is axis-aligned,
                          but appears to occur in other places too */
                        /*STTBX_Print(("CalcXYLArray: > 2 boundaries encountered for"
                            " y = %u (yTop = %u, yBot = %u)\n", y, yTop, yBot));*/
                    }
                    
                    x1   = xMin;
                    x2   = tmpXYLArray[i].PositionX;
                    xMin = GFXMIN(x1, x2);
                    /*leng = 1 + GFXMAX(GFXABS(x1-xMin+leng - 1),
                           GFXABS(x2-xMin+(S32)tmpXYLArray[i].Length - 1));*/
                    leng = GFXMAX(x1-xMin+leng,
                                  x2-xMin+(S32)tmpXYLArray[i].Length);
                    flag = 2;
                }
            }
        }
        
        if (flag == 0)
        {
            /* y not represented in XYLArray - repeat tmpXYLArray[0]
              for safety, but produces visual artifact eg for STBLIT_ALU_XOR */
            STTBX_Print(("CalcXYLArray: no element to write for y = %u"
                " (yTop = %u, yBot = %u)\n", y, yTop, yBot));
                
            xMin = tmpXYLArray[0].PositionX;
            leng = tmpXYLArray[0].Length;
        }
        
        XYLArray[y - yTop].PositionX = xMin;
        XYLArray[y - yTop].PositionY = y;
        XYLArray[y - yTop].Length    = leng;
        if(leng == 0)
        {
            /* will cause problems on GX1 */
            STTBX_Print(("CalcXYLArray: leng == 0\n"));
        }
    }
}

/* EOF gfx_tools.c */
