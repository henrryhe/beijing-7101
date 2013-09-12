/*******************************************************************************

File name   : osd_vp.c

Description : OSD Viewports Manager

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-03-09          Creation                                Michel Bruant

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include "stlayer.h"
#include "stddefs.h"
#include "osd_vp.h"
#include "osd_hal.h"
#include "layerosd.h"
#include "osd_list.h"
#include "osd_task.h"
#include "stsys.h"

/* Private functions -------------------------------------------------------- */

static void OSDFieldSetViewportDesc (stosd_ViewportDesc * VP,
                               STGXOBJ_Rectangle_t * InputRectangle_p,
                               STGXOBJ_Rectangle_t * OutputRectangle_p,
                               STGXOBJ_Bitmap_t    * Bitmap_p);

static ST_ErrorCode_t TestOSDFieldParams(STGXOBJ_Rectangle_t * InputRectangle_p,
                               STGXOBJ_Rectangle_t * OutputRectangle_p,
                               STGXOBJ_Bitmap_t    * Bitmap_p,
                               STGXOBJ_Palette_t   * Palette_p);

static void UpdatePalette(STLAYER_ViewPortHandle_t Handle );

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
    U32 VP;
    ST_ErrorCode_t Error;

    if ((Handle_p == NULL)
    || (Params_p == NULL))
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = TestOSDFieldParams(&Params_p->InputRectangle,
                                &Params_p->OutputRectangle,
                                Params_p->Source_p->Data.BitMap_p,
                                Params_p->Source_p->Palette_p );
    if(Error != ST_NO_ERROR)
    {
        layerosd_Defense(Error);
        return (Error);
    }

    /* Get a VP descriptor */
    semaphore_wait(stlayer_osd_context.ContextAcess_p);
    VP = stlayer_IndexNewViewport();
    if( VP == NO_NEW_VP)
    {
        layerosd_Defense(STLAYER_ERROR_NO_FREE_HANDLES);
        return(STLAYER_ERROR_NO_FREE_HANDLES);
    }
    /* The viewport handle is its address : */
    *Handle_p = (STLAYER_ViewPortHandle_t)
        (&(stlayer_osd_context.OSD_ViewPort[VP]));

    /* Update VP descriptor */
    OSDFieldSetViewportDesc((stosd_ViewportDesc *)(*Handle_p),
                                &Params_p->InputRectangle,
                                &Params_p->OutputRectangle,
                                Params_p->Source_p->Data.BitMap_p);
    stlayer_osd_context.OSD_ViewPort[VP].Open            = TRUE;
    stlayer_osd_context.OSD_ViewPort[VP].Bitmap          = *(Params_p->Source_p
                                                        ->Data.BitMap_p);
    stlayer_osd_context.OSD_ViewPort[VP].Palette         = *(Params_p->Source_p
                                                         ->Palette_p);
    stlayer_osd_context.OSD_ViewPort[VP].Alpha           = 128;
    stlayer_osd_context.OSD_ViewPort[VP].TopHeaderUpdate = FALSE;
    stlayer_osd_context.OSD_ViewPort[VP].BotHeaderUpdate = FALSE;
    stlayer_osd_context.OSD_ViewPort[VP].Next_p          = 0;
    stlayer_osd_context.OSD_ViewPort[VP].Prev_p          = 0;
    stlayer_osd_context.OSD_ViewPort[VP].Recordable      = TRUE;
    UpdatePalette(*Handle_p);
    semaphore_signal(stlayer_osd_context.ContextAcess_p);

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
    ST_ErrorCode_t  Err;
    if ( ((stosd_ViewportDesc *)Handle)->Enabled)
    {
        Err = LAYEROSD_DisableViewPort(Handle);
    }
    else
    {
        Err = ST_NO_ERROR;
    }
    if(Err == 0)
    {
        ((stosd_ViewportDesc *)Handle)->Open = FALSE;
    }
    return(Err);
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

    if (((stosd_ViewportDesc *)Handle)->Enabled)
    {
        return(ST_NO_ERROR);
    }
    /* manage the list */
    Err = stlayer_InsertViewport(Handle);
    if (Err == ST_NO_ERROR)
    {
        semaphore_wait(stlayer_osd_context.ContextAcess_p);
        ((stosd_ViewportDesc *)Handle)->Enabled         = TRUE;
        /* set the flag to update on vsync */
        ((stosd_ViewportDesc *)Handle)->TopHeaderUpdate = TRUE;
        ((stosd_ViewportDesc *)Handle)->BotHeaderUpdate = TRUE;
        semaphore_signal(stlayer_osd_context.ContextAcess_p);
    }
    return (Err);
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
    ST_ErrorCode_t Err;

    if (((stosd_ViewportDesc *)Handle)->Enabled == FALSE)
    {
        return(ST_NO_ERROR);
    }
    /* manage the list */
    semaphore_wait(stlayer_osd_context.ContextAcess_p);
    Err = stlayer_ExtractViewport(Handle);
    {
        ((stosd_ViewportDesc *)Handle)->Enabled         = FALSE;
        /* set the flag to update on vsync */
        if(((stosd_ViewportDesc *)Handle)->Prev_p != NULL)
        {
            ((stosd_ViewportDesc *)Handle)->Prev_p->TopHeaderUpdate = TRUE;
            ((stosd_ViewportDesc *)Handle)->Prev_p->BotHeaderUpdate = TRUE;
        }
        else if(((stosd_ViewportDesc *)Handle)->Next_p != NULL)
        {
            ((stosd_ViewportDesc *)Handle)->Next_p->TopHeaderUpdate = TRUE;
            ((stosd_ViewportDesc *)Handle)->Next_p->BotHeaderUpdate = TRUE;
        }
        else
        {
            /* prev = next = null : no more viewport */
            stlayer_HardDisable();
        }
    }
    semaphore_signal(stlayer_osd_context.ContextAcess_p);
    return (Err);
}

/*******************************************************************************
Name        : LAYEROSD_GetViewPortIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortIORectangle(STLAYER_ViewPortHandle_t VPHdl,
                                STGXOBJ_Rectangle_t*           InputRectangle,
                                STGXOBJ_Rectangle_t*           OutputRectangle)
{
    OutputRectangle->PositionX = ((stosd_ViewportDesc *)VPHdl)->PositionX;
    OutputRectangle->PositionY = ((stosd_ViewportDesc *)VPHdl)->PositionY;
    OutputRectangle->Height    = ((stosd_ViewportDesc *)VPHdl)->Height;
    OutputRectangle->Width     = ((stosd_ViewportDesc *)VPHdl)->Width;

    InputRectangle->PositionX  = 0;
    InputRectangle->PositionY  = ((stosd_ViewportDesc *)VPHdl)->BitmapY;
    InputRectangle->Height     = ((stosd_ViewportDesc *)VPHdl)->Height;
    InputRectangle->Width      = ((stosd_ViewportDesc *)VPHdl)->Width;
    return(ST_NO_ERROR);
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
    ST_ErrorCode_t Err;
    stosd_ViewportDesc * OldPrev;
    stosd_ViewportDesc * OldNext;

    Err = TestOSDFieldParams(InputRectangle,OutputRectangle,
                    &((stosd_ViewportDesc *)Handle)->Bitmap,
                    &((stosd_ViewportDesc *)Handle)->Palette);

    if( Err != ST_NO_ERROR)
    {
        layerosd_Defense(Err);
        return (Err);
    }
    OldPrev = ((stosd_ViewportDesc *)Handle)->Prev_p;
    OldNext = ((stosd_ViewportDesc *)Handle)->Next_p;
    semaphore_wait(stlayer_osd_context.ContextAcess_p);
    if(((stosd_ViewportDesc *)Handle)->Enabled)
    {
        Err = stlayer_TestMove(Handle,OutputRectangle);
        if (Err == OSD_OK) /* Move without any inversion in the list */
        {
            OSDFieldSetViewportDesc((stosd_ViewportDesc *)Handle,
                     InputRectangle,OutputRectangle,
                    &((stosd_ViewportDesc *)Handle)->Bitmap);
            /* set the flag to update on vsync */
            ((stosd_ViewportDesc *)Handle)->TopHeaderUpdate = TRUE;
            ((stosd_ViewportDesc *)Handle)->BotHeaderUpdate = TRUE;
        }
        else if(Err == OSD_MOVE_INVERSION_LIST)
        {
            OSDFieldSetViewportDesc((stosd_ViewportDesc *)Handle,
                     InputRectangle,OutputRectangle,
                    &((stosd_ViewportDesc *)Handle)->Bitmap);
            /* set the flag to update all the list on vsync */
            if(OldPrev != NULL)
            {
                OldPrev->TopHeaderUpdate = TRUE;
                OldPrev->BotHeaderUpdate = TRUE;
            }
            else if(OldNext != NULL)
            {
                OldNext->TopHeaderUpdate = TRUE;
                OldNext->BotHeaderUpdate = TRUE;
            }
            ((stosd_ViewportDesc *)Handle)->TopHeaderUpdate = TRUE;
            ((stosd_ViewportDesc *)Handle)->BotHeaderUpdate = TRUE;
        }
        else /* Err == OSD_IMPOSSIBLE_RESIZE_MOVE */
        {
            semaphore_signal(stlayer_osd_context.ContextAcess_p);
            layerosd_Defense(STLAYER_ERROR_OVERLAP_VIEWPORT);
            return(STLAYER_ERROR_OVERLAP_VIEWPORT);
        }
    }
    else
    {
        OSDFieldSetViewportDesc((stosd_ViewportDesc *)Handle,
                 InputRectangle,OutputRectangle,
                &((stosd_ViewportDesc *)Handle)->Bitmap);

    }
    semaphore_signal(stlayer_osd_context.ContextAcess_p);
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
ST_ErrorCode_t LAYEROSD_SetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                                S32                 XPosition,
                                                S32                 YPosition)
{
    ST_ErrorCode_t Error;
    STGXOBJ_Rectangle_t Input,Output;

    LAYEROSD_GetViewPortIORectangle(VPHandle,&Input,&Output);
    Output.PositionX = XPosition;
    Output.PositionY = YPosition;
    Error = LAYEROSD_SetViewPortIORectangle(VPHandle,&Input,&Output);
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
ST_ErrorCode_t LAYEROSD_GetViewPortPosition(STLAYER_ViewPortHandle_t VPHandle,
                                            S32 * XPosition,
                                            S32 * YPosition)
{
    * XPosition = ((stosd_ViewportDesc *)VPHandle)->PositionX;
    * YPosition = ((stosd_ViewportDesc *)VPHandle)->PositionY;
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYEROSD_AdjustIORectangle
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_AdjustIORectangle(STLAYER_Handle_t Handle,
                                    STGXOBJ_Rectangle_t*   InputRectangle,
                                    STGXOBJ_Rectangle_t*   OutputRectangle)
{
    ST_ErrorCode_t               Error;

    Error = ST_NO_ERROR;

    /* Selection tests */
    if(OutputRectangle->PositionX > ((S32)stlayer_osd_context.LayerParams.Width))
    {
       return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }
    if(OutputRectangle->PositionY > ((S32)stlayer_osd_context.LayerParams.Height))
    {
       return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    /* Adjust Input Rectangle */
    if (InputRectangle->PositionX != 0)
    {
        InputRectangle->PositionX = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle->PositionY < 0)
    {
        InputRectangle->PositionY = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust Output Rectangle */
    if (OutputRectangle->PositionX < 0)
    {
        OutputRectangle->PositionX = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (OutputRectangle->PositionY < 0)
    {
        OutputRectangle->PositionY = 0;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if(OutputRectangle->PositionX + OutputRectangle->Width
                                > stlayer_osd_context.LayerParams.Width)
    {
        OutputRectangle->Width = stlayer_osd_context.LayerParams.Width
                                                - OutputRectangle->PositionX ;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if(OutputRectangle->PositionY + OutputRectangle->Height
                                > stlayer_osd_context.LayerParams.Height)
    {
        OutputRectangle-> Height = stlayer_osd_context.LayerParams.Height
                                                - OutputRectangle->PositionY ;
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust Resize */
    /* test Vresize */
    if (InputRectangle->Height != OutputRectangle->Height)
    {
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        if (InputRectangle->Height > OutputRectangle->Height)
        {
            InputRectangle->Height = OutputRectangle->Height;
        }
        else
        {
            OutputRectangle->Height = InputRectangle->Height;
        }
    }
    /* Test Hresize */
    if (InputRectangle->Width != OutputRectangle->Width)
    {
        Error = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
        if (InputRectangle->Width > OutputRectangle->Width)
        {
            InputRectangle->Width = OutputRectangle->Width;
        }
        else
        {
            OutputRectangle->Width = InputRectangle->Width;
        }
    }
    return(Error);
}


/*******************************************************************************
Name        : LAYEROSD_GetViewPortAlpha
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_GetViewPortAlpha(  STLAYER_ViewPortHandle_t  VPHandle,
                                            STLAYER_GlobalAlpha_t*    Alpha)
{
    Alpha->A0 = ((stosd_ViewportDesc *)VPHandle)->Alpha;
    Alpha->A1 = 0;
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
ST_ErrorCode_t LAYEROSD_SetViewPortAlpha(  STLAYER_ViewPortHandle_t  VPHandle,
                                    STLAYER_GlobalAlpha_t*    Alpha)
{
    if(Alpha->A0 <= 128)
    {
        ((stosd_ViewportDesc *)VPHandle)->Alpha = Alpha->A0;
    }
    else
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Update the Palette */
    UpdatePalette(VPHandle);
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
ST_ErrorCode_t LAYEROSD_SetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                              BOOL Recordable)
{
    semaphore_wait(stlayer_osd_context.ContextAcess_p);
    ((stosd_ViewportDesc *)VPHandle)->Recordable = Recordable;
    /* Update the OSD */
    if(((stosd_ViewportDesc *)VPHandle)->Enabled)
    {
        /* set the flag to update on vsync */
        ((stosd_ViewportDesc *)VPHandle)->TopHeaderUpdate = TRUE;
        ((stosd_ViewportDesc *)VPHandle)->BotHeaderUpdate = TRUE;
    }
    semaphore_signal(stlayer_osd_context.ContextAcess_p);

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
ST_ErrorCode_t LAYEROSD_GetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                         BOOL * Recordable_p)
{
    * Recordable_p = ((stosd_ViewportDesc *)VPHandle)->Recordable;
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
ST_ErrorCode_t LAYEROSD_SetViewPortSource(  STLAYER_ViewPortHandle_t Handle,
                                STLAYER_ViewPortSource_t*   VPSource_p)

{
    ST_ErrorCode_t  Error;
    STGXOBJ_Rectangle_t PrevInput;
    STGXOBJ_Rectangle_t PrevOutput;

    /*  io rectangles */
    PrevInput.PositionY     = ((stosd_ViewportDesc *)Handle)->BitmapY;
    PrevInput.PositionX     = 0;
    PrevInput.Width         = ((stosd_ViewportDesc *)Handle)->Width;
    PrevInput.Height        =((stosd_ViewportDesc *)Handle)->Height;
    PrevOutput.PositionX    = ((stosd_ViewportDesc *)Handle)->PositionX;
    PrevOutput.PositionY    = ((stosd_ViewportDesc *)Handle)->PositionY;
    PrevOutput.Width        = ((stosd_ViewportDesc *)Handle)->Width;
    PrevOutput.Height       = ((stosd_ViewportDesc *)Handle)->Height;

    Error = TestOSDFieldParams(&PrevInput,&PrevOutput,
                          VPSource_p->Data.BitMap_p,VPSource_p->Palette_p);

    if (Error != ST_NO_ERROR)
    {
        layerosd_Defense(Error);
        return(Error);
    }
    ((stosd_ViewportDesc *)Handle)->Bitmap  = *VPSource_p->Data.BitMap_p;
    ((stosd_ViewportDesc *)Handle)->Palette = *VPSource_p->Palette_p;

    /* Update the Palette */
    UpdatePalette(Handle);

    if(((stosd_ViewportDesc *)Handle)->Enabled)
    {
        /* set the flag to update on vsync */
        ((stosd_ViewportDesc *)Handle)->TopHeaderUpdate = TRUE;
        ((stosd_ViewportDesc *)Handle)->BotHeaderUpdate = TRUE;
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
ST_ErrorCode_t LAYEROSD_GetViewPortSource(  STLAYER_ViewPortHandle_t Handle,
                             STLAYER_ViewPortSource_t*   VPSource)

{
    if(VPSource == 0)
    {
        layerosd_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }
    if((VPSource->Palette_p == 0)||(VPSource->Data.BitMap_p == 0))
    VPSource->SourceType    = STLAYER_GRAPHIC_BITMAP;
    *(VPSource->Palette_p)     = ((stosd_ViewportDesc *)Handle)->Palette;
    *(VPSource->Data.BitMap_p) = ((stosd_ViewportDesc *)Handle)->Bitmap;

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : LAYEROSD_UpdateFromMixer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t LAYEROSD_UpdateFromMixer(STLAYER_Handle_t         LayerHandle,
                                       STLAYER_OutputParams_t * OutputParams_p)
{
    stosd_ViewportDesc *    ViewPort_p;

    /* Connect a VTG synchro */
    if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
    {
        stlayer_SubscribeEvents(OutputParams_p->VTGName);
    }
    /* Disonnect a VTG synchro */
    if ((OutputParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
    {
        stlayer_SubscribeEvents(0);
    }
    /* Update Offset params */
    if ((OutputParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)
    {
       stlayer_osd_context.XOffset = OutputParams_p->XOffset;
       stlayer_osd_context.YOffset = OutputParams_p->YOffset;
    }
    /* Update screen params */
    if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
    {
       if (OutputParams_p->Width < stlayer_osd_context.LayerParams.Width)
       {
            layerosd_Trace("LAYER - OSD : Output Width is too small. \n");
            layerosd_Defense(STLAYER_ERROR_INSIDE_LAYER);
            return(STLAYER_ERROR_INSIDE_LAYER);
       }
       if (OutputParams_p->Height < stlayer_osd_context.LayerParams.Height)
       {
            layerosd_Trace("LAYER - OSD : Output Height is too small. \n");
            layerosd_Defense(STLAYER_ERROR_INSIDE_LAYER);
            return(STLAYER_ERROR_INSIDE_LAYER);
       }
       stlayer_osd_context.Width  = OutputParams_p->Width;
       stlayer_osd_context.Height = OutputParams_p->Height;
       stlayer_osd_context.XStart = OutputParams_p->XStart;
       stlayer_osd_context.YStart = OutputParams_p->YStart  /* top lines */
                                  + OutputParams_p->YStart; /* bot lines */
    }
    /* Change the pipeline */
    if ((OutputParams_p->UpdateReason & STLAYER_CHANGE_ID_REASON) != 0)
    {
        /* dont care */
    }
    /* Enable / Disable the display */
    if ((OutputParams_p->UpdateReason & STLAYER_DISPLAY_REASON) != 0)
    {
        stlayer_osd_context.OutputEnabled = OutputParams_p->DisplayEnable;
        if (stlayer_osd_context.OutputEnabled)
        {
            stlayer_EnableDisplay();
        }
        else
        {
            stlayer_DisableDisplay();
        }
    }

    /* dynamic output params change */
    ViewPort_p = stlayer_osd_context.FirstViewPortEnabled;
    while(ViewPort_p != NULL) /* Scan the list of enabled VP */
    {
        ViewPort_p->TopHeaderUpdate = TRUE;
        ViewPort_p->BotHeaderUpdate = TRUE;
        ViewPort_p = ViewPort_p->Next_p;
    }

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
    if (InputRectangle.PositionY + InputRectangle.Height > BitMap_p->Height)
    {
        return(STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE);
    }

    if(InputRectangle.PositionX != 0)
    {
        InputRectangle.PositionX = 0;
        ErrorSource = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    /* Adjust to the source */
    if (InputRectangle.Width > BitMap_p->Width)
    {
        InputRectangle.Width = BitMap_p->Width;
        ErrorSource = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
    if (InputRectangle.PositionY + InputRectangle.Height > BitMap_p->Height)
    {
        InputRectangle.Height = BitMap_p->Height - InputRectangle.PositionY;
        ErrorSource = STLAYER_SUCCESS_IORECTANGLES_ADJUSTED;
    }
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
    STLAYER_ViewPortSource_t * Source_p;

    /* test the new params */
    InputRectangle  = &(Params_p->InputRectangle);
    OutputRectangle = &(Params_p->OutputRectangle);
    Source_p        = Params_p->Source_p;
    Error = TestOSDFieldParams(InputRectangle,OutputRectangle,
                            Source_p->Data.BitMap_p,Source_p->Palette_p);
    if (Error != ST_NO_ERROR)
    {
        layerosd_Defense(Error);
        return(Error);
    }
    /* Set the source without test i/o rectangles */
    ((stosd_ViewportDesc *)VPHandle)->Bitmap  = *(Source_p->Data.BitMap_p);
    ((stosd_ViewportDesc *)VPHandle)->Palette = *(Source_p->Palette_p);

    /* Update the Palette */
    UpdatePalette(VPHandle);

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
Name        : TestOSDParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t TestOSDFieldParams(STGXOBJ_Rectangle_t * InputRectangle_p,
                               STGXOBJ_Rectangle_t * OutputRectangle_p,
                               STGXOBJ_Bitmap_t    * Bitmap_p,
                               STGXOBJ_Palette_t   * Palette_p)
{
    ST_ErrorCode_t  Ret = ST_NO_ERROR;

    /* test input rectangle */
    /*----------------------*/
    if(InputRectangle_p->PositionX != 0)
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : no X offset on bitmap \n");
    }
    if(InputRectangle_p->PositionX + InputRectangle_p->Width > Bitmap_p->Width )
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : overflow X on bitmap \n");
    }
     if(InputRectangle_p->PositionY < 0)
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : negative Y on bitmap \n");
    }
    if(InputRectangle_p->PositionY + InputRectangle_p->Height
                    > Bitmap_p->Height )
    {
        Ret |= STLAYER_ERROR_INVALID_INPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : overflow Y on bitmap \n");
    }

    /* test output windows */
    /*---------------------*/
    if(OutputRectangle_p->PositionX < 0)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : negative X on screen \n");
    }
    if(OutputRectangle_p->PositionX + OutputRectangle_p->Width >
            stlayer_osd_context.LayerParams.Width)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : overflow X on screen\n");
    }
    if(OutputRectangle_p->PositionY < 0)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : negative Y on screen \n");
    }
    if(OutputRectangle_p->PositionY + OutputRectangle_p->Height >
            stlayer_osd_context.LayerParams.Height)
    {
        Ret |= STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE;
        layerosd_Trace("LAYER - OSD : overflow Y on screen\n");
    }
    /* Test resize */
    /*-------------*/
    if(OutputRectangle_p->Height != InputRectangle_p->Height)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad Y resize \n");
    }
    if(OutputRectangle_p->Width != InputRectangle_p->Width)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad X resize \n");
    }
    /* bitmap storage test */
    /*---------------------*/
    if((Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT8)
            &&(Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT4)
            &&(Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_CLUT2))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad bitmap format \n");
    }
    if((Bitmap_p->BitmapType != STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad storage format \n");
    }
    if(((U32)Bitmap_p->Data1_p % 128 ) != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad storage align for data1 \n");
    }
    if((Bitmap_p->Pitch % 128 ) != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad pitch align  \n");
    }
    /* Palette storage test */
    /*----------------------*/
    if(((U32)Palette_p->Data_p) % 256 != 0)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad palette align  \n");
    }
    if(Palette_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444)
    {
        Ret |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        layerosd_Trace("LAYER - OSD : Bad color format in the palette  \n");
    }
    return(Ret);
}
/*******************************************************************************
Name        : OSDFieldSetViewportDesc
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void OSDFieldSetViewportDesc (stosd_ViewportDesc * VP,
                               STGXOBJ_Rectangle_t * InputRectangle_p,
                               STGXOBJ_Rectangle_t * OutputRectangle_p,
                               STGXOBJ_Bitmap_t    * Bitmap_p)
{

    /* First line bitmap header */
    VP->BitmapFirstHeader_p = (osd_RegionHeader_t *)((U32) (Bitmap_p->Data1_p)
                            + Bitmap_p->Pitch * InputRectangle_p->PositionY);
    /* Second line bitmap header */
    VP->BitmapSecondHeader_p = (osd_RegionHeader_t *)((U32)
                                (VP->BitmapFirstHeader_p)+ Bitmap_p->Pitch);
    /* Last line bitmap header */
    VP->BitmapLastHeader_p = (osd_RegionHeader_t *)((U32)
                                (VP->BitmapFirstHeader_p)
                           + Bitmap_p->Pitch * (OutputRectangle_p->Height -1));
    /* Antelast line bitmap header */
    VP->BitmapPrevLastHeader_p = (osd_RegionHeader_t *)((U32)
                                (VP->BitmapLastHeader_p)- Bitmap_p->Pitch);

    /* Viewport param init */
    VP->PositionX = OutputRectangle_p->PositionX;
    VP->PositionY = OutputRectangle_p->PositionY;
    VP->Width     = OutputRectangle_p->Width;
    VP->Height    = OutputRectangle_p->Height;
    VP->BitmapY   = InputRectangle_p->PositionY;
    VP->Pitch     = Bitmap_p->Pitch;
}

/*******************************************************************************
Name        : UpdatePalette
Description : Recopy user palette in hw header-palette + alpha
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void UpdatePalette(STLAYER_ViewPortHandle_t Handle )
{
    STGXOBJ_Color_t     UserValue;
    STGXOBJ_Palette_t   UserPalette;
    U32                 * DstPointer;
    U32                 * SrcPointer;
    U32                 NewAlpha,GlobalAlpha, NbColors, i;
    U32                 HardwareData;

    NbColors    = 1 << ((stosd_ViewportDesc *)Handle)->Palette.ColorDepth;
    UserPalette = ((stosd_ViewportDesc *)Handle)->Palette;
    GlobalAlpha = ((stosd_ViewportDesc *)Handle)->Alpha;
    DstPointer  = (U32*)(((stosd_ViewportDesc *)Handle)->PaletteFirstHeader_p + 1);
    for(i=0; i < NbColors; i++)
    {
        STGXOBJ_GetPaletteColor(&UserPalette,i,&UserValue);
        NewAlpha = UserValue.Value.UnsignedAYCbCr6888_444.Alpha
                    * GlobalAlpha / 128;
        UserValue.Value.UnsignedAYCbCr6888_444.Alpha = NewAlpha;
        HardwareData =
                (U32)(UserValue.Value.UnsignedAYCbCr6888_444.Cr    & 0xFF)
               |(U32)(UserValue.Value.UnsignedAYCbCr6888_444.Cb    & 0xFF) << 8
               |(U32)(UserValue.Value.UnsignedAYCbCr6888_444.Y     & 0xFF) <<16
               |(U32)(UserValue.Value.UnsignedAYCbCr6888_444.Alpha & 0xFF) <<24;
        STSYS_WriteRegMem32BE((void*)DstPointer,HardwareData );
        DstPointer ++;
    }
    /* Bottom header */
    SrcPointer  =(U32*)(((stosd_ViewportDesc *)Handle)->PaletteFirstHeader_p + 1);
    DstPointer  =(U32*)(((stosd_ViewportDesc *)Handle)->PaletteSecondHeader_p + 1);
    STAVMEM_CopyBlock1D(SrcPointer,DstPointer,NbColors*sizeof(U32));
}

/* end of osd_vp.c */
