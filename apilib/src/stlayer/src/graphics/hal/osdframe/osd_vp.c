/*******************************************************************************

File name   : osd_vp.c

Description :It provides the OSD frame viewport functions

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-01-04         Created                                    Michel Bruant


*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include "stlayer.h"
#include "stddefs.h"
#include "osd_vp.h"
#include "osd_cm.h"
#include "layerosd.h"
#include "osd_list.h"
#include "stsys.h"

/* Macros ------------------------------------------------------------------- */

#define TestViewportHandle(Handle) \
{                                                                           \
    /* test add range */                                                    \
    if (((U32)Handle < (U32)stlayer_osd_context.OSD_ViewPort)               \
            || ((U32)Handle > (U32)stlayer_osd_context.OSD_ViewPort         \
              + stlayer_osd_context.MaxViewPorts * sizeof(stosd_ViewportDesc)))\
    {                                                                       \
        layerosd_Defense(ST_ERROR_INVALID_HANDLE);                          \
        return(ST_ERROR_INVALID_HANDLE);                                    \
    }                                                                       \
    /* test open */                                                         \
    if (!(((stosd_ViewportDesc *)Handle)->Open))                            \
    {                                                                       \
        layerosd_Defense(ST_ERROR_INVALID_HANDLE);                          \
        return(ST_ERROR_INVALID_HANDLE);                                    \
    }                                                                       \
}                                                                           \


/* Functions ---------------------------------------------------------------- */
static ST_ErrorCode_t TestOSDParams (STGXOBJ_Rectangle_t * InputRectangle_p,
                               STGXOBJ_Rectangle_t * OutputRectangle_p,
                               STGXOBJ_Rectangle_t * ClippedIn_p,
                               STGXOBJ_Rectangle_t * ClippedOut_p,
                               BOOL                * TotalClipped,
                               STGXOBJ_Bitmap_t    * Bitmap_p);
static void UpdatePalette(STLAYER_ViewPortHandle_t);
ST_ErrorCode_t LAYEROSD_ShowViewPort(STLAYER_ViewPortHandle_t Handle);
ST_ErrorCode_t LAYEROSD_HideViewPort(STLAYER_ViewPortHandle_t Handle);

/*******************************************************************************
Name        : LAYEROSD_OpenViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_OpenViewPort(STLAYER_Handle_t            LayerHandle,
                                     STLAYER_ViewPortParams_t*   Params_p,
                                     STLAYER_ViewPortHandle_t*   Handle_p )
{
    U32             VP;
    ST_ErrorCode_t  Error;
    STGXOBJ_Rectangle_t ClippedIn,ClippedOut;
    BOOL            TotalClipped;

    if((Handle_p == NULL) || (Params_p == NULL) || (Params_p->Source_p == NULL)
     ||(Params_p->Source_p->Data.BitMap_p == NULL))
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = TestOSDParams(&Params_p->InputRectangle,
                    &Params_p->OutputRectangle,
                    &ClippedIn,&ClippedOut,&TotalClipped,
                    Params_p->Source_p->Data.BitMap_p);
    if (Error != ST_NO_ERROR)
    {
        layerosd_Defense(Error);
        return(Error);
    }

    /* Init descriptor  */
    VP = stlayer_osd_IndexNewViewport();
    if (VP == NO_NEW_VP)
    {
        layerosd_Defense(STLAYER_ERROR_NO_FREE_HANDLES);
        return(STLAYER_ERROR_NO_FREE_HANDLES);
    }
    /* The viewport handle is its address : */
    *Handle_p = (STLAYER_ViewPortHandle_t)
                                      (&(stlayer_osd_context.OSD_ViewPort[VP]));
    stlayer_osd_context.OSD_ViewPort[VP].OutputRectangle
                                            = Params_p->OutputRectangle;
    stlayer_osd_context.OSD_ViewPort[VP].InputRectangle
                                            = Params_p->InputRectangle;
    stlayer_osd_context.OSD_ViewPort[VP].ClippedIn      = ClippedIn;
    stlayer_osd_context.OSD_ViewPort[VP].ClippedOut     = ClippedOut;
    stlayer_osd_context.OSD_ViewPort[VP].TotalClipped   = TotalClipped;
    stlayer_osd_context.OSD_ViewPort[VP].Bitmap
                                           = *Params_p->Source_p->Data.BitMap_p;
    stlayer_osd_context.OSD_ViewPort[VP].Open           = TRUE;
    stlayer_osd_context.OSD_ViewPort[VP].Alpha.A1       = 0;
    stlayer_osd_context.OSD_ViewPort[VP].Alpha.A0       = 128;
    stlayer_osd_context.OSD_ViewPort[VP].Next_p         = 0;
    stlayer_osd_context.OSD_ViewPort[VP].Prev_p         = 0;
    stlayer_osd_context.OSD_ViewPort[VP].Recordable     = TRUE;
    if((Params_p->Source_p->Data.BitMap_p->ColorType
                    == STGXOBJ_COLOR_TYPE_CLUT2)
      ||(Params_p->Source_p->Data.BitMap_p->ColorType
                    == STGXOBJ_COLOR_TYPE_CLUT4)
      ||(Params_p->Source_p->Data.BitMap_p->ColorType
                    == STGXOBJ_COLOR_TYPE_CLUT8))
    {
        if(Params_p->Source_p->Palette_p == NULL)
        {
            layerosd_Defense(ST_ERROR_BAD_PARAMETER);
            return(ST_ERROR_BAD_PARAMETER);
        }
        stlayer_osd_context.OSD_ViewPort[VP].Palette
                                            = *Params_p->Source_p->Palette_p;
        UpdatePalette(*Handle_p);
    }
    /* else : no palette */
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYEROSD_CloseViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_CloseViewPort(STLAYER_ViewPortHandle_t Handle)
{

    TestViewportHandle(Handle);
    if ( ((stosd_ViewportDesc *)Handle)->Enabled)
    {
        LAYEROSD_DisableViewPort(Handle);
    }
    ((stosd_ViewportDesc *)Handle)->Open = FALSE;
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_ShowViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_ShowViewPort(STLAYER_ViewPortHandle_t Handle)
{
    ST_ErrorCode_t Err;

    /* manage the list */
    Err = stlayer_osd_InsertViewport(Handle);
    return (Err);
}
/*******************************************************************************
Name        : LAYEROSD_EnableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_EnableViewPort(STLAYER_ViewPortHandle_t Handle)
{
    ST_ErrorCode_t Err;

    TestViewportHandle(Handle);
    if (((stosd_ViewportDesc *)Handle)->Enabled)
    {
        return(ST_NO_ERROR);
    }
    if(stlayer_osd_context.NumViewPortEnabled != 0)
    {
        /* need to test if color match the rgb16 mode */
        if((((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                == STGXOBJ_COLOR_TYPE_ARGB1555)
        || (((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                == STGXOBJ_COLOR_TYPE_ARGB4444)
        || (((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                == STGXOBJ_COLOR_TYPE_RGB565))
        {
            if(!stlayer_osd_context.EnableRGB16Mode)
            {
                layerosd_Trace("LAYER - OSD : VP Color must be Clut+YCbCr\n");
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
        else /* VP color is YCbCr+clut */
        {
            if(stlayer_osd_context.EnableRGB16Mode)
            {
                layerosd_Trace("LAYER - OSD : VP Color must be RGB16\n");
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }

    }
    else /* first vp enabled : set the mode rgb16 and enable */
    {
        /* need to set/reset the rgb16 mode */
        if((((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                == STGXOBJ_COLOR_TYPE_ARGB1555)
        || (((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                == STGXOBJ_COLOR_TYPE_ARGB4444)
        || (((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                == STGXOBJ_COLOR_TYPE_RGB565))
        {
            stlayer_osd_context.EnableRGB16Mode = TRUE;
        }
        else /* color type is a clut one */
        {
            stlayer_osd_context.EnableRGB16Mode = FALSE;
        }
        if(stlayer_osd_context.OutputEnabled)
        {
            stlayer_HardEnableDisplay();
        }
    }

    Err = ST_NO_ERROR;
    if (!(((stosd_ViewportDesc *)Handle)->TotalClipped))
    {
        Err = LAYEROSD_ShowViewPort(Handle);
    }
    if (Err == ST_NO_ERROR)
    {
        semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
        stlayer_BuildHardImage();
        ((stosd_ViewportDesc *)Handle)->Enabled = TRUE;
        stlayer_osd_context.NumViewPortEnabled ++;
        semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
    }
    return (Err);
}
/*******************************************************************************
Name        : LAYEROSD_HideViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_HideViewPort(STLAYER_ViewPortHandle_t Handle)
{
    /* manage the list */
    stlayer_osd_ExtractViewport(Handle);
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_DisableViewPort
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_DisableViewPort(STLAYER_ViewPortHandle_t Handle)
{

    TestViewportHandle(Handle);
    if (!(((stosd_ViewportDesc *)Handle)->Enabled))
    {
        return(ST_NO_ERROR);
    }
    if (!(((stosd_ViewportDesc *)Handle)->TotalClipped))
    {
        LAYEROSD_HideViewPort(Handle);
        semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
        stlayer_BuildHardImage();
        stlayer_osd_context.NumViewPortEnabled --;
        semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
    }
    ((stosd_ViewportDesc *)Handle)->Enabled = FALSE;
    if(stlayer_osd_context.NumViewPortEnabled == 0)
    {
        stlayer_HardDisableDisplay();
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYEROSD_SetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_SetViewPortSource(STLAYER_ViewPortHandle_t Handle,
                                          STLAYER_ViewPortSource_t* VPSource_p)
{
    ST_ErrorCode_t  Error;
    STGXOBJ_Rectangle_t ClippedIn,ClippedOut;
    BOOL        TotalClipped;

    if((VPSource_p == NULL) || (VPSource_p->Data.BitMap_p == NULL))
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    Error = TestOSDParams(&((stosd_ViewportDesc *)Handle)->InputRectangle,
                          &((stosd_ViewportDesc *)Handle)->OutputRectangle,
                          &ClippedIn,&ClippedOut,&TotalClipped,
                            VPSource_p->Data.BitMap_p);

    if (Error != ST_NO_ERROR)
    {
        layerosd_Defense(Error);
        return(Error);
    }

    if(((stosd_ViewportDesc *)Handle)->Enabled)
    {
        if((VPSource_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2)
         ||(VPSource_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4)
         ||(VPSource_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8))
        {
            if(stlayer_osd_context.EnableRGB16Mode)
            {
                layerosd_Trace("LAYER - OSD : VP Color must be a RGB16\n");
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
        else
        {
            if(!stlayer_osd_context.EnableRGB16Mode)
            {
                layerosd_Trace("LAYER - OSD : VP Color must be Clut+YCbCr\n");
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
    }

    if((VPSource_p->Data.BitMap_p->ColorType  == STGXOBJ_COLOR_TYPE_CLUT2)
     ||(VPSource_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4)
     ||(VPSource_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8))
    {
        if(VPSource_p->Palette_p == NULL)
        {
            layerosd_Defense(ST_ERROR_BAD_PARAMETER);
            return(ST_ERROR_BAD_PARAMETER);
        }
        ((stosd_ViewportDesc *)Handle)->Palette = *VPSource_p->Palette_p;
        UpdatePalette(Handle);
    }/* else : no palette */

    ((stosd_ViewportDesc *)Handle)->Bitmap  = *VPSource_p->Data.BitMap_p;


    if(((stosd_ViewportDesc *)Handle)->Enabled)
    {
        if((((stosd_ViewportDesc *)Handle)->TotalClipped)
             && (!(TotalClipped)))
        {
            LAYEROSD_ShowViewPort(Handle);
        }
        if((!(((stosd_ViewportDesc *)Handle)->TotalClipped))
             && (TotalClipped))
        {
            LAYEROSD_HideViewPort(Handle);
        }
        ((stosd_ViewportDesc *)Handle)->TotalClipped = TotalClipped;
        semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
        stlayer_BuildHardImage();
        semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
    }
    return (Error);
}

/*******************************************************************************
Name        : LAYEROSD_GetViewPortSource
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortSource(STLAYER_ViewPortHandle_t Handle,
                    STLAYER_ViewPortSource_t*   VPSource)
{
    TestViewportHandle(Handle);
    if(VPSource == 0)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if((VPSource->Palette_p == 0)||(VPSource->Data.BitMap_p == 0))
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    VPSource->SourceType       = STLAYER_GRAPHIC_BITMAP;
    *(VPSource->Palette_p)     = ((stosd_ViewportDesc *)Handle)->Palette;
    *(VPSource->Data.BitMap_p) = ((stosd_ViewportDesc *)Handle)->Bitmap;

    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_AdjustViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_AdjustViewPortParams(STLAYER_Handle_t Handle,
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

    /* Adjust the io rectangles */
    ErrorRect = LAYEROSD_AdjustIORectangle(Handle,&InputRectangle,
                                                    &OutputRectangle);
    if ((ErrorSource == STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE)
            || (ErrorRect == STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE))
    {
        layerosd_Defense(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    Params_p->InputRectangle  = InputRectangle;
    Params_p->OutputRectangle = OutputRectangle;
    return(ErrorRect | ErrorSource);

}
/*******************************************************************************
Name        : LAYEROSD_SetViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p)
{
    ST_ErrorCode_t           Error;
    STGXOBJ_Rectangle_t      * InputRectangle;
    STGXOBJ_Rectangle_t      * OutputRectangle;
    STGXOBJ_Rectangle_t     ClippedIn,ClippedOut;
    BOOL                    TotalClipped;
    STLAYER_ViewPortSource_t * Source_p;

    /* test the new params */
    InputRectangle  = &(Params_p->InputRectangle);
    OutputRectangle = &(Params_p->OutputRectangle);
    Source_p        = Params_p->Source_p;
    Error = TestOSDParams(InputRectangle,OutputRectangle,
                            &ClippedIn,&ClippedOut,&TotalClipped,
                            Source_p->Data.BitMap_p);
    if (Error != ST_NO_ERROR)
    {
        layerosd_Defense(Error);
        return(Error);
    }
    if(((stosd_ViewportDesc *)VPHandle)->Enabled)
    {
        if((Source_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT2)
         ||(Source_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4)
         ||(Source_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8))
        {
            if(stlayer_osd_context.EnableRGB16Mode)
            {
                layerosd_Trace("LAYER - OSD : VP Color must be a RGB16\n");
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
        else
        {
            if(!stlayer_osd_context.EnableRGB16Mode)
            {
                layerosd_Trace("LAYER - OSD : VP Color must be Clut+YCbCr\n");
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
        }
    }

    if((Source_p->Data.BitMap_p->ColorType  == STGXOBJ_COLOR_TYPE_CLUT2)
     ||(Source_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT4)
     ||(Source_p->Data.BitMap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT8))
    {
        if(Source_p->Palette_p == NULL)
        {
            layerosd_Defense(ST_ERROR_BAD_PARAMETER);
            return(ST_ERROR_BAD_PARAMETER);
        }
        ((stosd_ViewportDesc *)VPHandle)->Palette = *Source_p->Palette_p;
        UpdatePalette(VPHandle);
    }/* else : no palette */

    /* Set the source without test i/o rectangles */
    ((stosd_ViewportDesc *)VPHandle)->Bitmap  = *(Source_p->Data.BitMap_p);


    /* The set the i/o rectangles */
    LAYEROSD_SetViewPortIORectangle(VPHandle,InputRectangle,OutputRectangle);
    return(ST_NO_ERROR);

}
/*******************************************************************************
Name        : LAYEROSD_GetViewPortParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p)
{
    ST_ErrorCode_t Error;

    if(Params_p == 0)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = LAYEROSD_GetViewPortSource(VPHandle,Params_p->Source_p);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }
    else
    {
        Error = LAYEROSD_GetViewPortIORectangle(VPHandle,
                                                &(Params_p->InputRectangle),
                                                &(Params_p->OutputRectangle));
    }
    return(Error);
}


/*******************************************************************************
Name        : LAYEROSD_SetViewPortIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_SetViewPortIORectangle(STLAYER_ViewPortHandle_t Handle,
                                          STGXOBJ_Rectangle_t * InputRectangle,
                                          STGXOBJ_Rectangle_t * OutputRectangle)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    STGXOBJ_Rectangle_t SaveInput, SaveOutput;
    STGXOBJ_Rectangle_t SaveClipIn, SaveClipOut;
    STGXOBJ_Rectangle_t ClippedIn,ClippedOut;
    BOOL                TotalClipped;

    TestViewportHandle(Handle);
    Error = TestOSDParams(InputRectangle,OutputRectangle,
                            &ClippedIn,&ClippedOut,&TotalClipped,
                            &((stosd_ViewportDesc *)Handle)->Bitmap);
    if(Error != ST_NO_ERROR)
    {
        layerosd_Defense(Error);
        return(Error);
    }
    SaveInput  = ((stosd_ViewportDesc *)Handle)->InputRectangle;
    SaveOutput = ((stosd_ViewportDesc *)Handle)->OutputRectangle;
    SaveClipIn = ((stosd_ViewportDesc *)Handle)->ClippedIn;
    SaveClipOut= ((stosd_ViewportDesc *)Handle)->ClippedOut;
    semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
    ((stosd_ViewportDesc *)Handle)->InputRectangle   = *InputRectangle;
    ((stosd_ViewportDesc *)Handle)->OutputRectangle  = *OutputRectangle;
    ((stosd_ViewportDesc *)Handle)->ClippedIn        = ClippedIn;
    ((stosd_ViewportDesc *)Handle)->ClippedOut       = ClippedOut;
    if (((stosd_ViewportDesc *)Handle)->Enabled)
    {
        if (TotalClipped) /* VP is now unvisible */
        {
            if (!(((stosd_ViewportDesc*)Handle)->TotalClipped))
            {
                LAYEROSD_HideViewPort(Handle);
                stlayer_BuildHardImage();
            }
            ((stosd_ViewportDesc*)Handle)->TotalClipped = TRUE;
            semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
            return(ST_NO_ERROR);
        }
        /* TotalClipped is FALSE : VP is now visible */
        if (((stosd_ViewportDesc*)Handle)->TotalClipped)
        {
            Error = LAYEROSD_ShowViewPort(Handle);
            stlayer_BuildHardImage();
            if (Error != ST_NO_ERROR)
            {
                ((stosd_ViewportDesc *)Handle)->InputRectangle   = SaveInput;
                ((stosd_ViewportDesc *)Handle)->OutputRectangle  = SaveOutput;
                ((stosd_ViewportDesc *)Handle)->ClippedIn   = SaveClipIn;
                ((stosd_ViewportDesc *)Handle)->ClippedOut  = SaveClipOut;
                semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
                return(STLAYER_ERROR_OVERLAP_VIEWPORT);
            }
            ((stosd_ViewportDesc*)Handle)->TotalClipped = FALSE;
            semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
            return(ST_NO_ERROR);
        }
        /* VP is now visible and was already visible : this is a real move */
        LAYEROSD_HideViewPort(Handle);
        Error = LAYEROSD_ShowViewPort(Handle);
        if (Error != ST_NO_ERROR)
        {
            ((stosd_ViewportDesc *)Handle)->InputRectangle   = SaveInput;
            ((stosd_ViewportDesc *)Handle)->OutputRectangle  = SaveOutput;
            ((stosd_ViewportDesc *)Handle)->ClippedIn        = SaveClipIn;
            ((stosd_ViewportDesc *)Handle)->ClippedOut       = SaveClipOut;
            Error = LAYEROSD_ShowViewPort(Handle);
            stlayer_BuildHardImage();
            semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
            return(STLAYER_ERROR_OVERLAP_VIEWPORT);
        }
        stlayer_BuildHardImage();
    }
    semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : LAYEROSD_GetViewPortIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortIORectangle(STLAYER_ViewPortHandle_t Handle,
                                STGXOBJ_Rectangle_t*           InputRectangle,
                                STGXOBJ_Rectangle_t*           OutputRectangle)
{
    TestViewportHandle(Handle);
    * InputRectangle  = ((stosd_ViewportDesc *)Handle)->InputRectangle;
    * OutputRectangle = ((stosd_ViewportDesc *)Handle)->OutputRectangle;

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYEROSD_SetViewPortPosition
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_SetViewPortPosition(STLAYER_ViewPortHandle_t Handle,
                                                S32                 XPosition,
                                                S32                 YPosition)
{
    STGXOBJ_Rectangle_t OutputRectangle,InputRectangle;
    ST_ErrorCode_t Error;

    TestViewportHandle(Handle);
    /* copy actual rectangles */
    OutputRectangle           = ((stosd_ViewportDesc *)Handle)->OutputRectangle;
    InputRectangle            = ((stosd_ViewportDesc *)Handle)->InputRectangle;
    /* patch the output position */
    OutputRectangle.PositionX = XPosition;
    OutputRectangle.PositionY = YPosition;
    Error = LAYEROSD_SetViewPortIORectangle( Handle,&InputRectangle,
                                                &OutputRectangle);
    return(Error);
}

/*******************************************************************************
Name        : LAYEROSD_GetViewPortPosition
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortPosition(STLAYER_ViewPortHandle_t Handle,
                                            S32 * XPosition,
                                            S32 * YPosition)
{
    TestViewportHandle(Handle);
    * XPosition = ((stosd_ViewportDesc *)Handle)->OutputRectangle.PositionX;
    * YPosition = ((stosd_ViewportDesc *)Handle)->OutputRectangle.PositionY;
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYEROSD_SetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_SetViewPortAlpha(STLAYER_ViewPortHandle_t  Handle,
                                           STLAYER_GlobalAlpha_t*    Alpha_p)
{
    TestViewportHandle(Handle);

    if((Alpha_p->A0 <= 128) && (Alpha_p->A1 <= 128))
    {
        ((stosd_ViewportDesc *)Handle)->Alpha = *Alpha_p;
    }
    else
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    if((((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                    == STGXOBJ_COLOR_TYPE_CLUT2)
      ||(((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                    == STGXOBJ_COLOR_TYPE_CLUT4)
      ||(((stosd_ViewportDesc *)Handle)->Bitmap.ColorType
                    == STGXOBJ_COLOR_TYPE_CLUT8))
    {   /* Update the Palette */
        UpdatePalette(Handle);
    }
    else
    {   /* Udate the alpha in the header: */
        stlayer_BuildHardImage();
    }


    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_GetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortAlpha(STLAYER_ViewPortHandle_t  Handle,
                                           STLAYER_GlobalAlpha_t*    Alpha_p)
{
    TestViewportHandle(Handle);

    *Alpha_p = ((stosd_ViewportDesc *)Handle)->Alpha;
    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_SetViewPortRecordable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_SetViewPortRecordable(STLAYER_ViewPortHandle_t  Handle,
                                              BOOL Recordable)
{
    TestViewportHandle(Handle);

    semaphore_wait(stlayer_osd_context.VPDecriptorAccess_p);
    ((stosd_ViewportDesc *)Handle)->Recordable = Recordable;
    /* Update the OSD */
    stlayer_BuildHardImage();
    semaphore_signal(stlayer_osd_context.VPDecriptorAccess_p);

    return(ST_NO_ERROR);
}
/*******************************************************************************
Name        : LAYEROSD_GetViewPortRecordable
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortRecordable(STLAYER_ViewPortHandle_t  Handle,
                                         BOOL * Recordable_p)
{
    TestViewportHandle(Handle);

    * Recordable_p = ((stosd_ViewportDesc *)Handle)->Recordable;
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : TestOSDParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t TestOSDParams(STGXOBJ_Rectangle_t * InputRect_p,
                             STGXOBJ_Rectangle_t * OutputRect_p,
                             STGXOBJ_Rectangle_t * ClippedIn_p,
                             STGXOBJ_Rectangle_t * ClippedOut_p,
                             BOOL                * TotalClipped,
                             STGXOBJ_Bitmap_t    * BitMap_p)
{
    ST_ErrorCode_t  Ret = ST_NO_ERROR;
    U32             PixelPerByte;
    S32 ClipedInStart;
    S32 ClipedInStop;
    S32 ClipedOutStart;
    S32 ClipedOutStop;
    U32 Resize;
    U32 StartTrunc,StopTrunc;
    S32 InStop, OutStop;
    * TotalClipped = FALSE;

    switch (BitMap_p->ColorType)
    {
        case STGXOBJ_COLOR_TYPE_CLUT2:
            PixelPerByte = 4;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT4:
            PixelPerByte = 2;
            break;
        case STGXOBJ_COLOR_TYPE_CLUT8:
            PixelPerByte = 1;
            break;
        default:
            PixelPerByte = 1;
            break;
    }
    if(InputRect_p->PositionX % (8*PixelPerByte) != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad align for X on bitmap \n");
    }
    /* Test resize */
    /*-------------*/
    if((OutputRect_p->Height != InputRect_p->Height)
        && (2 * OutputRect_p->Height != InputRect_p->Height)
        &&(OutputRect_p->Height != 2 * InputRect_p->Height))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad Y resize \n");
    }
    if((OutputRect_p->Width != InputRect_p->Width)
        &&(OutputRect_p->Width != 2 * InputRect_p->Width))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad X resize \n");
    }
    /* bitmap storage test */
    /*---------------------*/
#if defined HW_5514 || defined HW_5516
    if((BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
     &&(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT4)
     &&(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT2))
#else /* this version: 'else' means HW_5517 */
    if((BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
     &&(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT4)
     &&(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT2)
     &&(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB1555)
     &&(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_RGB565)
     &&(BitMap_p->ColorType != STGXOBJ_COLOR_TYPE_ARGB4444))
#endif
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad Color format \n");
    }
    if((BitMap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad storage format \n");
    }
    if(((U32)BitMap_p->Data1_p % 8 ) != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad storage align for data1 \n");
    }
    if((BitMap_p->Pitch % 8 ) != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad pitch align  \n");
    }

    if (Ret != ST_NO_ERROR)
    {
        layerosd_Defense(Ret);
        return(Ret);
    }


    /* Clipping Horizontal Computing */
    /*-------------------------------*/
    InStop  = InputRect_p->PositionX + InputRect_p->Width;
    OutStop = OutputRect_p->PositionX + OutputRect_p->Width;
    Resize =  256 * OutputRect_p->Width / InputRect_p->Width;

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if((InStop < 0 ) || (InputRect_p->PositionX > (S32)BitMap_p->Width))
    {
        * TotalClipped = TRUE;
    }
    if(InputRect_p->PositionX  < 0 )
    {
        StartTrunc = -InputRect_p->PositionX;
        ClipedInStart = 0;
    }
    else
    {
        StartTrunc = 0;
        ClipedInStart = InputRect_p->PositionX;
    }
    if(InStop > ((S32)BitMap_p->Width))
    {
        StopTrunc = InStop - BitMap_p->Width;
        ClipedInStop = BitMap_p->Width;
    }
    else
    {
        StopTrunc = 0;
        ClipedInStop = InStop;
    }
    /* screen feedback clipping */
    /*--------------------------*/
    ClipedOutStart = OutputRect_p->PositionX + (StartTrunc * Resize / 256 );
    ClipedOutStop = OutStop - (StopTrunc * Resize / 256);

    /* easy clipping in output selection */
    /*-----------------------------------*/
    if((OutStop < 0) || (OutputRect_p->PositionX
           > (S32)stlayer_osd_context.LayerParams.Width))
    {
        * TotalClipped = TRUE;
    }
    if(ClipedOutStart < 0)
    {
        StartTrunc = -ClipedOutStart;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc = 0;
    }
    if(ClipedOutStop > ((S32)stlayer_osd_context.LayerParams.Width))
    {
        StopTrunc = ClipedOutStop -
            stlayer_osd_context.LayerParams.Width;
        ClipedOutStop =
            stlayer_osd_context.LayerParams.Width;
    }
    else
    {
        StopTrunc = 0;
    }
    /* bitmap feedback clipping */
    /*--------------------------*/
    ClipedInStart = ClipedInStart + (256 * StartTrunc / Resize);
    ClipedInStop = ClipedInStop - (256 * StopTrunc / Resize);

    ClippedIn_p->PositionX  = ClipedInStart;
    ClippedIn_p->Width      = ClipedInStop - ClipedInStart;
    ClippedOut_p->PositionX = ClipedOutStart;
    ClippedOut_p->Width     = Resize * ClippedIn_p->Width / 256;

    /* Clipping Vertical Computing */
    /*-----------------------------*/

    InStop  = InputRect_p->PositionY + InputRect_p->Height;
    OutStop = OutputRect_p->PositionY + OutputRect_p->Height;
    Resize =  256 * OutputRect_p->Height / InputRect_p->Height;

    /* easy clipping in bitmap selection */
    /*-----------------------------------*/
    if((InStop < 0 ) || (InputRect_p->PositionY > ((S32)BitMap_p->Height)))
    {
        * TotalClipped = TRUE;
    }
    if(InputRect_p->PositionY  < 0 )
    {
        StartTrunc = -InputRect_p->PositionY;
        ClipedInStart = 0;
    }
    else
    {
        StartTrunc = 0;
        ClipedInStart = InputRect_p->PositionY;
    }
    if(InStop > ((S32)BitMap_p->Height))
    {
        StopTrunc = InStop - BitMap_p->Height;
        ClipedInStop = BitMap_p->Height;
    }
    else
    {
        StopTrunc = 0;
        ClipedInStop = InStop;
    }
    /* screen feedback clipping */
    /*--------------------------*/
    ClipedOutStart = OutputRect_p->PositionY + (StartTrunc * Resize / 256 );
    ClipedOutStop = OutStop - (StopTrunc * Resize / 256);

    /* easy clipping in output selection */
    /*-----------------------------------*/
    if((OutStop < 0) || (OutputRect_p->PositionY
           > (S32)stlayer_osd_context.LayerParams.Height))
    {
        * TotalClipped = TRUE;
    }
    if(ClipedOutStart < 0)
    {
        StartTrunc = -ClipedOutStart;
        ClipedOutStart = 0;
    }
    else
    {
        StartTrunc = 0;
    }
    if(ClipedOutStop > ((S32)stlayer_osd_context.LayerParams.Height))
    {
        StopTrunc = ClipedOutStop - stlayer_osd_context.LayerParams.Height;
        ClipedOutStop = stlayer_osd_context.LayerParams.Height;
    }
    else
    {
        StopTrunc = 0;
    }
    /* bitmap feedback clipping */
    /*--------------------------*/
    ClipedInStart = ClipedInStart + (256 * StartTrunc / Resize );
    ClipedInStop = ClipedInStop - (256 * StopTrunc / Resize );

    ClippedIn_p->PositionY  = ClipedInStart;
    ClippedIn_p->Height     = ClipedInStop - ClipedInStart;
    ClippedOut_p->PositionY = ClipedOutStart;
    ClippedOut_p->Height    = Resize * ClippedIn_p->Height / 256;

    if(  (ClippedOut_p->Height <= 1)
       ||(ClippedOut_p->Width  <= 1)
       ||(ClippedIn_p->Height  <= 1)
       ||(ClippedIn_p->Width   <= 1))
    {
        *TotalClipped = TRUE;
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : UpdatePalette
Description : Recopy user palette in hw header-palette + alpha
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void UpdatePalette(STLAYER_ViewPortHandle_t Handle )
{
    U32                * DstPointer1;
    U32                * DstPointer2;
    U32                * SrcPointer;
    U32                  Value32,GlobalAlpha, NbColors, i;

    NbColors    = 1 << ((stosd_ViewportDesc *)Handle)->Palette.ColorDepth;
    GlobalAlpha = ((stosd_ViewportDesc *)Handle)->Alpha.A0;

    SrcPointer   = (U32*)(((stosd_ViewportDesc *)Handle)->Palette.Data_p);
    DstPointer1  = (U32*)(((stosd_ViewportDesc *)Handle)->Top_header + 1);
    DstPointer2  = (U32*)(((stosd_ViewportDesc *)Handle)->Bot_header + 1);
    for(i=0; i < NbColors; i++)
    {
        /* fast but maybe not portable */
        Value32 = STSYS_ReadRegMem32LE((void*)SrcPointer);
        Value32 = (Value32 & 0xffffff00)
              | (((Value32 & 0x0000003f)*GlobalAlpha)>>7);
        STSYS_WriteRegMem32LE((void*)DstPointer1,Value32);
        STSYS_WriteRegMem32LE((void*)DstPointer2,Value32);
        DstPointer1 ++;
        DstPointer2 ++;
        SrcPointer  ++;
    }
}

/* end of osd_vp.c */
