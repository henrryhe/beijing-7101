/*****************************************************************************

File name: drawing.c

Description: drawing utilities

COPYRIGHT (C) 2004 STMicroelectronics

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "blt_wrapper.h"
#include "blt_engine.h"
#include "blt_drawing.h"

STGXOBJ_Color_t         Color;

/*----------------------------------------------------------------------
 * Function : DemoDraw_SnakeColor()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
 void DemoDraw_SnakeColor()
{
    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8888 )
    {
        Color.Value.ARGB8888.Alpha = 0x80 ;
        Color.Value.ARGB8888.R = 1 ;
        Color.Value.ARGB8888.G = 28 ;
        Color.Value.ARGB8888.B = 12 ;
  }

    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8565 )
    {
        Color.Value.ARGB8565.Alpha = 0x80 ;
        Color.Value.ARGB8565.R = 1 ;
        Color.Value.ARGB8565.G = 28 ;
        Color.Value.ARGB8565.B = 12 ;
  }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB565 )
    {
        Color.Value.RGB565.R = 1 ;
        Color.Value.RGB565.G = 28 ;
        Color.Value.RGB565.B = 12 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB888 )
    {
        Color.Value.RGB888.R = 1 ;
        Color.Value.RGB888.G = 28 ;
        Color.Value.RGB888.B = 12 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_CLUT8 )
    {
        Color.Value.CLUT8 = 10 ;
    }

}
/*----------------------------------------------------------------------
 * Function : DemoDraw_DstBitmapColor()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
 void DemoDraw_DstBitmapColor()
{
    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8888 )
    {
        Color.Value.ARGB8888.Alpha = 0x80 ;
        Color.Value.ARGB8888.R = 11 ;
        Color.Value.ARGB8888.G = 19 ;
        Color.Value.ARGB8888.B = 17 ;
  }

    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8565 )
    {
        Color.Value.ARGB8565.Alpha = 0x80 ;
        Color.Value.ARGB8565.R = 11 ;
        Color.Value.ARGB8565.G = 19 ;
        Color.Value.ARGB8565.B = 17 ;
  }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB565 )
    {
        Color.Value.RGB565.R = 11 ;
        Color.Value.RGB565.G = 19 ;
        Color.Value.RGB565.B = 17 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB888 )
    {
        Color.Value.RGB888.R = 19 ;
        Color.Value.RGB888.G = 11 ;
        Color.Value.RGB888.B = 17 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_CLUT8 )
    {
        Color.Value.CLUT8 = 5 ;
    }

}

/*----------------------------------------------------------------------
 * Function : DemoDraw_BodyColor()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
void DemoDraw_BodyColor()
{
    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8888 )
    {
        Color.Value.ARGB8888.Alpha = 0x80 ;
        Color.Value.ARGB8888.R = 24 ;
        Color.Value.ARGB8888.G = 44 ;
        Color.Value.ARGB8888.B = 1 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8565 )
    {
        Color.Value.ARGB8565.Alpha = 0x80 ;
        Color.Value.ARGB8565.R = 24 ;
        Color.Value.ARGB8565.G = 44 ;
        Color.Value.ARGB8565.B = 1 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB565 )
    {
        Color.Value.RGB565.R = 24 ;
        Color.Value.RGB565.G = 44 ;
        Color.Value.RGB565.B = 1 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB888 )
    {
        Color.Value.RGB888.R = 24 ;
        Color.Value.RGB888.G = 44 ;
        Color.Value.RGB888.B = 1 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_CLUT8 )
    {
        Color.Value.CLUT8 = 30 ;
    }

}

  /*----------------------------------------------------------------------
 * Function : wall_draw()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
void DemoDraw_WallColor()
{
    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8888 )
    {
        Color.Value.ARGB8888.Alpha = 0xFF ;
        Color.Value.ARGB8888.R = 19 ;
        Color.Value.ARGB8888.G = 0 ;
        Color.Value.ARGB8888.B = 5 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8565 )
    {
        Color.Value.ARGB8565.Alpha = 0x80 ;
        Color.Value.ARGB8565.R = 19 ;
        Color.Value.ARGB8565.G = 0 ;
        Color.Value.ARGB8565.B = 5 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB565 )
    {
        Color.Value.RGB565.R = 19 ;
        Color.Value.RGB565.G = 0 ;
        Color.Value.RGB565.B = 5 ;
     }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB888 )
    {
        Color.Value.RGB888.R = 19 ;
        Color.Value.RGB888.G = 0 ;
        Color.Value.RGB888.B = 5 ;
      }

    if(Color.Type == STGXOBJ_COLOR_TYPE_CLUT8 )
    {
        Color.Value.CLUT8 = 50 ;
    }
}

  /*----------------------------------------------------------------------
 * Function : wall_draw()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
void DemoDraw_ObstacleColor()
{
    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8888 )
    {
        Color.Value.ARGB8888.Alpha = 0x80 ;
        Color.Value.ARGB8888.R = 4 ;
        Color.Value.ARGB8888.G = 12 ;
        Color.Value.ARGB8888.B = 18 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8565 )
    {
        Color.Value.ARGB8565.Alpha = 0x80 ;
        Color.Value.ARGB8565.R = 4 ;
        Color.Value.ARGB8565.G = 12 ;
        Color.Value.ARGB8565.B = 18 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB565 )
    {
        Color.Value.RGB565.R = 4 ;
        Color.Value.RGB565.G = 12 ;
        Color.Value.RGB565.B = 18 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB888 )
    {
        Color.Value.RGB888.R = 4 ;
        Color.Value.RGB888.G = 12 ;
        Color.Value.RGB888.B = 18 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_CLUT8 )
    {
        Color.Value.CLUT8 = 80 ;
    }
}

  /*----------------------------------------------------------------------
 * Function : DemoDraw_TargetColor()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
void DemoDraw_TargetColor()
{
    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8888 )
    {
        Color.Value.ARGB8888.Alpha = 0x80 ;
        Color.Value.ARGB8888.R = 24;
        Color.Value.ARGB8888.G = 33 ;
        Color.Value.ARGB8888.B = 22 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8565 )
    {
        Color.Value.ARGB8565.Alpha = 0x80 ;
        Color.Value.ARGB8565.R = 24;
        Color.Value.ARGB8565.G = 33 ;
        Color.Value.ARGB8565.B = 22 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB565 )
    {
        Color.Value.RGB565.R = 24;
        Color.Value.RGB565.G = 33 ;
        Color.Value.RGB565.B = 22 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB888 )
    {
        Color.Value.RGB888.R = 24;
        Color.Value.RGB888.G = 33 ;
        Color.Value.RGB888.B = 22 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_CLUT8 )
    {
        Color.Value.CLUT8 = 90 ;
    }
}

  /*----------------------------------------------------------------------
 * Function : DemoDraw_TargetColor()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
void DemoDraw_BackgroundColor()
{
    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8888 )
    {
        Color.Value.ARGB8888.Alpha = 0x80 ;
        Color.Value.ARGB8888.R = 31 ;
        Color.Value.ARGB8888.G = 57 ;
        Color.Value.ARGB8888.B = 24 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_ARGB8565 )
    {
        Color.Value.ARGB8565.Alpha = 0x80 ;
        Color.Value.ARGB8565.R = 31 ;
        Color.Value.ARGB8565.G = 57 ;
        Color.Value.ARGB8565.B = 24 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB565 )
    {
        Color.Value.RGB565.R = 31 ;
        Color.Value.RGB565.G = 57 ;
        Color.Value.RGB565.B = 24 ;
     }

    if(Color.Type == STGXOBJ_COLOR_TYPE_RGB888 )
    {
        Color.Value.RGB888.R = 31 ;
        Color.Value.RGB888.G = 57 ;
        Color.Value.RGB888.B = 24 ;
    }

    if(Color.Type == STGXOBJ_COLOR_TYPE_CLUT8 )
    {
        Color.Value.CLUT8 = 100 ;
    }
}

 /*----------------------------------------------------------------------
 * Function : DemoDraw_ClearOSDBitmap()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
ST_ErrorCode_t DemoDraw_ClearOSDBitmap(STGXOBJ_Bitmap_t *DstBitmap)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Set Color type*/
    Color.Type = (*DstBitmap).ColorType;

    /* Set Color value*/
    DemoDraw_DstBitmapColor();

    /* Fill DstBitmap with color*/
    ErrorCode = DemoBlit_FillRectangle(DstBitmap,0,0,(*DstBitmap).Width,(*DstBitmap).Height,Color);
    if (ErrorCode != ST_NO_ERROR)
    {
        return( ErrorCode );
    }

    return (ErrorCode);
}

 /*----------------------------------------------------------------------
 * Function : DemoDraw_DrawMatrix()
 *  Input    :
 *  Output   :
 *  Return   : ST_ErrorCode
 * --------------------------------------------------------------------*/
ST_ErrorCode_t DemoDraw_DrawMatrix(STGXOBJ_Bitmap_t *DstBitmap)
{
    ST_ErrorCode_t ErrorCode;
    int i,j;
    int CellPositionX,CellPositionY,CellWidth,CellHeight;


    /* Set the MatrixCell size*/
    CellWidth = SnakeDemoParameters.Width / SnakeDemoParameters.Complexity;
    CellHeight = SnakeDemoParameters.Height / SnakeDemoParameters.Complexity;

    /* Set Color type*/
    Color.Type = (*DstBitmap).ColorType;


    for(i=0;i<SnakeDemoParameters.Complexity;i++)
    {
        for(j=0;j<SnakeDemoParameters.Complexity;j++)
        {
            /* set the MatrixCell position */
            CellPositionX = SnakeDemoParameters.OffsetX + j * CellWidth;
            CellPositionY = SnakeDemoParameters.OffsetY + i * CellHeight;

            /* switch MatrixCell type, Fill rectangle with a specific color */
            switch ( DemoEngine_Matrix[i][j])
            {
                case 's':
                    /* Set Color value*/
                    DemoDraw_SnakeColor();

                    ErrorCode = DemoBlit_FillRectangle(DstBitmap,CellPositionX,CellPositionY,CellWidth,CellHeight,Color);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return( ErrorCode );
                    }
                    break;

                case 'x':
                   /* Set Color value*/
                    DemoDraw_BodyColor();

                    ErrorCode = DemoBlit_FillRectangle(DstBitmap,CellPositionX,CellPositionY,CellWidth,CellHeight,Color);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return( ErrorCode );
                    }
                    break;
                case 'w':
                   /* Set Color value*/
                    DemoDraw_WallColor();

                    ErrorCode = DemoBlit_FillRectangle(DstBitmap,CellPositionX,CellPositionY,CellWidth,CellHeight,Color);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return( ErrorCode );
                    }
                    break;

                case 'o':
                   /* Set Color value*/
                    DemoDraw_ObstacleColor();

                    ErrorCode = DemoBlit_FillRectangle(DstBitmap,CellPositionX,CellPositionY,CellWidth,CellHeight,Color);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return( ErrorCode );
                    }
                    break;

                case 't':
                   /* Set Color value*/
                    DemoDraw_TargetColor();

                    ErrorCode = DemoBlit_FillRectangle(DstBitmap,CellPositionX,CellPositionY,CellWidth,CellHeight,Color);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return( ErrorCode );
                    }
                    break;

                case 'b':
                   /* Set Color value*/
                    DemoDraw_BackgroundColor();

                    ErrorCode = DemoBlit_FillRectangle(DstBitmap,CellPositionX,CellPositionY,CellWidth,CellHeight,Color);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return( ErrorCode );
                    }
                    break;
            }/*switch*/
        }
    }

    return( ErrorCode );
}

/* EOF --------------------------------------------------------------------- */
































































































