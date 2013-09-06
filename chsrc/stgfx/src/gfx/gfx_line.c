/*******************************************************************************

File name   : gfx_line.c

Description : Line drawing api source file

COPYRIGHT (C) STMicroelectronics 2001.
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "gfx_tools.h" /* includes stgfx_init.h ... */

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : STGFX_DrawLine()
Description : draws a line (straight segment) according to a specified graphics
              context
Parameters  : Bitmap_p,
              GC,
              X1,
              Y1,
              X2,
              Y2
Assumptions : The target bitmap, the GC and the parameters are correct
Limitations : 
Returns     : error if a parameter is not correct
*******************************************************************************/
ST_ErrorCode_t  STGFX_DrawLine(
    STGFX_Handle_t    Handle,
    STGXOBJ_Bitmap_t* Bitmap_p,
    const STGFX_GC_t* GC_p,
    S32               X1,
    S32               Y1,
    S32               X2,
    S32               Y2
)
{
    ST_ErrorCode_t Err;
    STGFX_Point_t  Points_p[2];
    U32            NumPoints;
    stgfx_PolyMode PolyMode;
    
    if(NULL == stgfx_HandleToUnit(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    NumPoints = 2;

    Points_p[0].X = X1; Points_p[0].Y = Y1;
    Points_p[1].X = X2; Points_p[1].Y = Y2;
    
    PolyMode = Open;
    
    Err = stgfx_DrawPoly(Handle, Bitmap_p, GC_p, NumPoints, Points_p, PolyMode);

    return(Err);

} /* end of STGFX_DrawLine() */


/*******************************************************************************
Name        : STGFX_DrawPolyline()
Description : draws a polyline according to a specified graphics context
Parameters  : Bitmap_p,
              GC,
              Points_p
Assumptions : The target bitmap, the GC and the parameters are correct
Limitations : 
Returns     : error if a parameter is not correct
*******************************************************************************/
ST_ErrorCode_t  STGFX_DrawPolyline(
    STGFX_Handle_t    Handle,
    STGXOBJ_Bitmap_t* Bitmap_p,
    const STGFX_GC_t* GC_p,
    U32               NumPoints,
    STGFX_Point_t*    Points_p
)
{
    ST_ErrorCode_t Err;
    stgfx_PolyMode PolyMode;
    
    if(NULL == stgfx_HandleToUnit(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    
    PolyMode = Open;
    
    Err = stgfx_DrawPoly(Handle, Bitmap_p, GC_p, NumPoints, Points_p, PolyMode);

    return(Err);
  
} /* End of STGFX_DrawPolyline */

/* End of gfx_line.c */
