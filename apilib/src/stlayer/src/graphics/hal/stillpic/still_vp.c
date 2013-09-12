/*******************************************************************************

File name   : still_vp.c

Description : It provides the STILL viewport functions

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-01-04         Created                                    Michel Bruant


*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "lay_stil.h"
#include "still_cm.h"
#include "still_vp.h"
#include "stillhal.h"

/* Macros ------------------------------------------------------------------- */
#define VP_HANDLE   0x95687423
#define TestViewportHandle(VpHandle)\
{\
    if(VpHandle != VP_HANDLE)\
    {\
        layerstill_Defense(ST_ERROR_INVALID_HANDLE);\
        return(ST_ERROR_INVALID_HANDLE);\
    }\
}

#define TestLayHandle(LayerHandle) \
{\
    if(LayerHandle != 0)\
    {\
        layerstill_Defense(ST_ERROR_INVALID_HANDLE);\
        return(ST_ERROR_INVALID_HANDLE);\
    }\
}

#define LayerOpen() \
{\
    if(stlayer_LayOpen(0) == 0)\
    {\
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);\
        return(ST_ERROR_BAD_PARAMETER);\
    }\
}

/* Functions protos---------------------------------------------------------- */

static ST_ErrorCode_t StillTestParams(STGXOBJ_Rectangle_t * InputRectangle_p,
                               STGXOBJ_Rectangle_t * OutputRectangle_p,
                               STGXOBJ_Bitmap_t    * Bitmap_p);

/* Functions ---------------------------------------------------------------- */

ST_ErrorCode_t LAYERSTILL_OpenViewPort(STLAYER_Handle_t LayerHandle,
                                        STLAYER_ViewPortParams_t*   Params,
                                        STLAYER_ViewPortHandle_t*   VPHandle)
{
    ST_ErrorCode_t Error;

    LayerOpen();
    TestLayHandle(LayerHandle);

    if (stlayer_still_context.ViewPort.Open)
    {
        /* already open */
        *VPHandle = VP_HANDLE + 456;
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = StillTestParams(&Params->InputRectangle,
                    &Params->OutputRectangle,
                    Params->Source_p->Data.BitMap_p);
    if (Error != ST_NO_ERROR)
    {
        layerstill_Defense(Error);
        return(Error);
    }

    semaphore_wait(stlayer_still_context.AccessProtect_p);
    *VPHandle = VP_HANDLE;
    /* store viewport params */
    stlayer_still_context.ViewPort.Enabled          = FALSE;
    stlayer_still_context.ViewPort.Open             = TRUE;
    stlayer_still_context.ViewPort.InputRectangle   = Params->InputRectangle;
    stlayer_still_context.ViewPort.OutputRectangle  = Params->OutputRectangle;
    stlayer_still_context.ViewPort.Bitmap           = *Params->Source_p->Data.BitMap_p;
    stlayer_still_context.ViewPort.Alpha            = 128; /* = opaque */
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (stlayer_still_context.ViewPort.Enabled)
    {
        LAYERSTILL_DisableViewPort(VPHandle);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.Open = FALSE;
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    LayerOpen();
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (stlayer_still_context.ViewPort.Enabled)
    {
        return(ST_NO_ERROR);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.Enabled = TRUE;
    stlayer_StillHardInit();
    stlayer_StillHardUpdate();
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle)
{
    if(!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if(stlayer_still_context.ViewPort.Enabled == FALSE)
    {
        return(ST_NO_ERROR);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.Enabled = FALSE;
    stlayer_StillHardUpdate();
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_SetViewPortSource(STLAYER_ViewPortHandle_t VPHandle,
                                            STLAYER_ViewPortSource_t* VPSource)
{
    ST_ErrorCode_t Error;

    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = StillTestParams(&stlayer_still_context.ViewPort.InputRectangle,
                            &stlayer_still_context.ViewPort.OutputRectangle,
                            VPSource->Data.BitMap_p);
    if (Error != ST_NO_ERROR)
    {
        layerstill_Defense(Error);
        return(Error);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.Bitmap= *VPSource->Data.BitMap_p;
    if (stlayer_still_context.ViewPort.Enabled)
    {
        stlayer_StillHardUpdate();
    }
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}
/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_GetViewPortSource(STLAYER_ViewPortHandle_t VPHandle,
                    STLAYER_ViewPortSource_t*   VPSource)
{
    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (VPSource == 0)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((VPSource->Palette_p == 0) || (VPSource->Data.BitMap_p == 0))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    VPSource->SourceType = STLAYER_GRAPHIC_BITMAP;
    /* *(VPSource->Palette_p) is not affected */
    *(VPSource->Data.BitMap_p) = stlayer_still_context.ViewPort.Bitmap;
    semaphore_signal(stlayer_still_context.AccessProtect_p);

    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_SetViewPortIORectangle(
                STLAYER_ViewPortHandle_t VPHandle,
                STGXOBJ_Rectangle_t*        InputRectangle_p,
                STGXOBJ_Rectangle_t*        OutputRectangle_p)
{
    ST_ErrorCode_t Error;

    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = StillTestParams(InputRectangle_p,
                            OutputRectangle_p,
                            &stlayer_still_context.ViewPort.Bitmap);
    if (Error != ST_NO_ERROR)
    {
        layerstill_Defense(Error);
        return(Error);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.InputRectangle   = *InputRectangle_p;
    stlayer_still_context.ViewPort.OutputRectangle  = *OutputRectangle_p;
    if (stlayer_still_context.ViewPort.Enabled)
    {
        stlayer_StillHardUpdate();
    }
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}
/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_AdjustViewPortParams(STLAYER_Handle_t Handle,
                                           STLAYER_ViewPortParams_t * Params_p)
{
    ST_ErrorCode_t              ErrorSource, ErrorRect;
    STGXOBJ_Bitmap_t            *BitMap_p;
    STGXOBJ_Rectangle_t         InputRectangle;
    STGXOBJ_Rectangle_t         OutputRectangle;

    BitMap_p        = Params_p->Source_p->Data.BitMap_p;
    InputRectangle  = Params_p->InputRectangle;
    OutputRectangle = Params_p->OutputRectangle;

    ErrorSource = ST_NO_ERROR;
    /* Selection test */
    if (InputRectangle.PositionX  > ((S32)BitMap_p->Width))
    {
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }
    if (InputRectangle.PositionY + InputRectangle.Height > BitMap_p->Height)
    {
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    /* Adjust to the source */
    if (InputRectangle.PositionX + InputRectangle.Width > BitMap_p->Width)
    {
        InputRectangle.Width = BitMap_p->Width - InputRectangle.PositionX;
        ErrorSource = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle.PositionY + InputRectangle.Height > BitMap_p->Height)
    {
        InputRectangle.Height = BitMap_p->Height - InputRectangle.PositionY;
        ErrorSource = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust the io rectangles */
    ErrorRect = LAYERSTILL_AdjustIORectangle(Handle,&InputRectangle,
                                                    &OutputRectangle);
    if ((ErrorSource == STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE)
            || (ErrorRect == STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE))
    {
        layerstill_Defense(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    Params_p->InputRectangle  = InputRectangle;
    Params_p->OutputRectangle = OutputRectangle;
    return(ErrorRect | ErrorSource);

}
/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                    STLAYER_ViewPortParams_t * Params_p)
{
    ST_ErrorCode_t           Error;
    STGXOBJ_Rectangle_t      * InputRectangle;
    STGXOBJ_Rectangle_t      * OutputRectangle;
    STLAYER_ViewPortSource_t * Source_p;

    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* test the new params */
    InputRectangle  = &(Params_p->InputRectangle);
    OutputRectangle = &(Params_p->OutputRectangle);
    Source_p        = Params_p->Source_p;
    Error = StillTestParams(InputRectangle,OutputRectangle,
                            Source_p->Data.BitMap_p);
    if (Error != ST_NO_ERROR)
    {
        layerstill_Defense(Error);
        return(Error);
    }
    /* Set the source without test i/o rectangles */
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.Bitmap= *(Source_p->Data.BitMap_p);
    semaphore_signal(stlayer_still_context.AccessProtect_p);

    /* The set the i/o rectangles */
    LAYERSTILL_SetViewPortIORectangle(VPHandle,InputRectangle,OutputRectangle);
    return(ST_NO_ERROR);

}
/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                      STLAYER_ViewPortParams_t * Params_p)
{
    ST_ErrorCode_t Error;

    if (Params_p == 0)
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = LAYERSTILL_GetViewPortSource(VPHandle,Params_p->Source_p);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }
    else
    {
        Error = LAYERSTILL_GetViewPortIORectangle(VPHandle,
                                                &(Params_p->InputRectangle),
                                                &(Params_p->OutputRectangle));
    }
    return(Error);
}


/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_GetViewPortIORectangle(
                STLAYER_ViewPortHandle_t VPHandle,
                STGXOBJ_Rectangle_t*        InputRectangle_p,
                STGXOBJ_Rectangle_t*        OutputRectangle_p)
{
    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    * InputRectangle_p  = stlayer_still_context.ViewPort.InputRectangle;
    * OutputRectangle_p = stlayer_still_context.ViewPort.OutputRectangle;
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return (ST_NO_ERROR);
}

/******************************************************************************/
ST_ErrorCode_t LAYERSTILL_SetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                                S32 XPosition,
                                                S32 YPosition)
{
    ST_ErrorCode_t Error;
    STGXOBJ_Rectangle_t OutputRectangle;

    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    OutputRectangle           = stlayer_still_context.ViewPort.OutputRectangle;
    OutputRectangle.PositionX = XPosition;
    OutputRectangle.PositionY = YPosition;
    Error = StillTestParams(&stlayer_still_context.ViewPort.InputRectangle,
                            &OutputRectangle,
                            &stlayer_still_context.ViewPort.Bitmap);
    if (Error != ST_NO_ERROR)
    {
        layerstill_Defense(Error);
        return(Error);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.OutputRectangle  = OutputRectangle;
    if (stlayer_still_context.ViewPort.Enabled == TRUE)
    {
        stlayer_StillHardUpdate();
    }
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}
/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_GetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                                S32  * XPosition,
                                                S32  * YPosition)
{
    LayerOpen();
    TestViewportHandle(VPHandle);
    if (!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    * XPosition = stlayer_still_context.ViewPort.OutputRectangle.PositionX;
    * YPosition = stlayer_still_context.ViewPort.OutputRectangle.PositionY;
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_SetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_GlobalAlpha_t*    Alpha)
{
    LayerOpen();
    TestViewportHandle(VPHandle);
    if(!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (Alpha->A0 > 128 )
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    stlayer_still_context.ViewPort.Alpha = Alpha->A0;
    if (stlayer_still_context.ViewPort.Enabled == TRUE)
    {
        stlayer_StillHardUpdate();
    }
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}

/******************************************************************************/

ST_ErrorCode_t LAYERSTILL_GetViewPortAlpha(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_GlobalAlpha_t*    Alpha)
{
    LayerOpen();
    TestViewportHandle(VPHandle);
    if(!(stlayer_still_context.ViewPort.Open))
    {
        layerstill_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    semaphore_wait(stlayer_still_context.AccessProtect_p);
    Alpha->A0 = stlayer_still_context.ViewPort.Alpha;
    Alpha->A1 = 0;
    semaphore_signal(stlayer_still_context.AccessProtect_p);
    return(ST_NO_ERROR);
}

/******************************************************************************/
static ST_ErrorCode_t StillTestParams(STGXOBJ_Rectangle_t * InputRectangle_p,
                               STGXOBJ_Rectangle_t * OutputRectangle_p,
                               STGXOBJ_Bitmap_t    * Bitmap_p)
{
    ST_ErrorCode_t Ret = ST_NO_ERROR;

    /* test input rectangle */
    /*----------------------*/
    if(InputRectangle_p->PositionX < 0)
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : negative X on bitmap ");
    }
    if(InputRectangle_p->PositionX + InputRectangle_p->Width > Bitmap_p->Width )
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : overflow X on bitmap ");
    }
     if(InputRectangle_p->PositionY < 0)
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : negative Y on bitmap ");
    }
    if(InputRectangle_p->PositionY + InputRectangle_p->Height
                    > Bitmap_p->Height )
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : overflow Y on bitmap ");
    }

    /* test output windows */
    /*---------------------*/
    if(OutputRectangle_p->PositionX < 0)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : negative X on screen ");
    }
    if(OutputRectangle_p->PositionX + OutputRectangle_p->Width >
            stlayer_still_context.LayerParams.Width)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : overflow X on screen");
    }
    if(OutputRectangle_p->PositionY < 0)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : negative Y on screen ");
    }
    if(OutputRectangle_p->PositionY %2 != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Odd Y on screen ");
    }
    if(OutputRectangle_p->PositionY + OutputRectangle_p->Height >
            stlayer_still_context.LayerParams.Height)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerstill_Trace("LAYER - Still : overflow Y on screen");
    }
    /* Test resize */
    /*-------------*/
    if((OutputRectangle_p->Height != InputRectangle_p->Height)
        &&(OutputRectangle_p->Height != 2 * InputRectangle_p->Height)
        &&(2 * OutputRectangle_p->Height != InputRectangle_p->Height))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Bad Y resize ");
    }
    if((InputRectangle_p->Width - 4) * 256
           / (OutputRectangle_p->Width - 1) > 0x1ff)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Bad X resize ");
    }
    /* bitmap storage test */
    /*---------------------*/
    if(Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Bad Color format ");
    }
    if((Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
        &&(Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Bad storage format ");
    }
    if(((U32)Bitmap_p->Data1_p % 128 ) != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Bad storage align for data1 ");
    }
    if((((U32)Bitmap_p->Data2_p % 128 ) != 0)
            &&(Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Bad storage align for data2 ");
    }
    if((Bitmap_p->Pitch % 128 ) != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Bad pitch align  ");
    }
#ifdef HW_5510
    if((Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE)
       && (Bitmap_p->Pitch > 1008))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerstill_Trace("LAYER - Still : Pitch overflow register size ");
    }
#endif
    return(Ret);
}


/* end of still_vp.c */
