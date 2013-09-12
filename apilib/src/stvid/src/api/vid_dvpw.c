/*******************************************************************************

File name   : vid_dvpw.c

Description : Window/Resize cmd for DVP+GFX layer config.

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
09 Jan 2002        Created                                      Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "api.h"

/* Value given by validation team corresponding to the hardware limit for SD stream width */
#define MAX_SD_OUTPUT_HORIZONTAL_SIZE       960

/* Private functions -------------------------------------------------------- */

static void ShareWindowsLayerVSDigitalInput(
                      STLAYER_ViewPortParams_t * const AdjustedLayerViewport_p,
                      STVID_DigitalInputWindows_t * const WindowsForDvp_p,
                      STLAYER_Layer_t         LayerType);

static ST_ErrorCode_t DVP_ChangeViewportParams(
                      VideoViewPort_t * const ViewPort_p,
                      STLAYER_ViewPortParams_t * const AdjustedLayerViewport_p);

static ST_ErrorCode_t ComputeOutputWindow(VideoViewPort_t * const ViewPort_p,
                      STGXOBJ_Rectangle_t * const OutputRectangle_p);

static ST_ErrorCode_t ComputeInputWindow(VideoViewPort_t * const ViewPort_p,
                      STGXOBJ_Rectangle_t * const InputRectangle_p);

#ifdef ST_display
#if defined (ST_diginput) && defined (ST_MasterDigInput)
/*******************************************************************************
Name        : stvid_DvpPictureCharacteristicsChange
Description : called on ' VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT.'
Parameters  : EVT API
Assumptions : EventData_p and SubscriberData_p shouldn't be null
Limitations :
Returns     :
*******************************************************************************/
void stvid_DvpPictureCharacteristicsChange(STEVT_CallReason_t Reason,
        const ST_DeviceName_t RegistrantName,
        STEVT_EventConstant_t Event,
        void *EventData_p,
        void *SubscriberData_p)
{
    VIDDISP_PictureCharacteristicsChangeParams_t *
                                                PictureCharacteristicsParams_p;
    STVID_PictureInfos_t *                      PictureInfos_p;
    VideoViewPort_t *                           ViewPort_p;
    U32                                         index;
    VideoDevice_t *                             Device_p
                                        = (VideoDevice_t *) SubscriberData_p;
    ST_ErrorCode_t                              ErrorCode = ST_NO_ERROR;
    STVID_DeviceType_t                          VideoType;
    STLAYER_ViewPortParams_t                    AdjustedLayerViewport;
    STVID_DigitalInputWindows_t                 WindowsForDvp;

	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    /* Ensure nobody else accesses layers/viewports. */
    semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    /* Retrieve the Decoder structure from the Video Handle. */
    ViewPort_p = (VideoViewPort_t *)
        Device_p->DeviceData_p->ViewportProperties_p;

    /* Store localy the parameters given in the event. */
    PictureCharacteristicsParams_p =
            (VIDDISP_PictureCharacteristicsChangeParams_t *) EventData_p;
    PictureInfos_p = PictureCharacteristicsParams_p->PictureInfos_p;

    VideoType= Device_p->DeviceData_p->DeviceType;

    /* In all the case, we need to find the opened Viewports. */
    for (index = 0; index < Device_p->DeviceData_p->MaxViewportNumber;
            index++)
    {
        if(ViewPort_p->Identity.ViewPortOpenedAndValidityCheck
                == VIDAPI_VALID_VIEWPORT_HANDLE)
        {
            /* Apply the actions to the opened viewports. */
            /* Retrieve the info given through the event about the stream.*/
            ViewPort_p->Params.StreamParameters.PanVector =  PictureCharacteristicsParams_p->PanAndScanIn16thPixel.FrameCentreHorizontalOffset;
            ViewPort_p->Params.StreamParameters.ScanVector = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.FrameCentreVerticalOffset;

            if (PictureInfos_p != NULL)
            {
                /* PictureInfos_p is NULL if only pan and scan ! */
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Pitch = PictureInfos_p->BitmapParams.Pitch;
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Width = PictureInfos_p->BitmapParams.Width;
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Height = PictureInfos_p->BitmapParams.Height;
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->AspectRatio = PictureInfos_p->BitmapParams.AspectRatio;
            }

            ViewPort_p->Params.StreamParameters.DisplayHorizontalSize = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.DisplayHorizontalSize;
            ViewPort_p->Params.StreamParameters.DisplayVerticalSize = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.DisplayVerticalSize;
            ViewPort_p->Params.StreamParameters.HasDisplaySizeRecommendation = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.HasDisplaySizeRecommendation;
            ViewPort_p->Params.StreamParameters.FrameCropInPixel = PictureCharacteristicsParams_p->FrameCropInPixel;


            switch (PictureCharacteristicsParams_p->CharacteristicsChanging)
            {
                case VIDDISP_CHANGING_ONLY_FRAME_CENTER_OFFSETS :
                    /* no break */
                case VIDDISP_CHANGING_EVERYTHING :
                    /* Process the parameters. */
                    ErrorCode = DVP_ChangeViewportParams(ViewPort_p,
                                                     &AdjustedLayerViewport);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "ChangeViewportParams failed. Error = 0x%x !",
                                    ErrorCode));
                    }
                    else
                    {
                        /* Apply to layer viewport */
                        ShareWindowsLayerVSDigitalInput(&AdjustedLayerViewport,
                                   &WindowsForDvp,
                                   ViewPort_p->Identity.Layer_p->LayerType);
                        ErrorCode = STLAYER_SetViewPortParams(
                                    ViewPort_p->Identity.LayerViewPortHandle,
                                    &AdjustedLayerViewport);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "STLAYER_SetViewPortParams failed. Error = 0x%x !",
                            ErrorCode));
                        }
                    }
                    break;
                default :
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                "No reason to change."));
                    break;
            }

            /* Check enable view port delayed. */
            if (ViewPort_p->Params.DisplayEnableDelayed)
            {
                ViewPort_p->Params.DisplayEnableDelayed = FALSE;

                /* Enable view port now that a new picture is coming */
                ErrorCode = STLAYER_EnableViewPort(
                                     ViewPort_p->Identity.LayerViewPortHandle);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "STLAYER_EnableViewPort() failed. Error = 0x%x !",
                            ErrorCode));
                    /* No error allowed for this function ! */
                    ErrorCode = ST_NO_ERROR;
                }
                else
                {
                    ViewPort_p->Params.DisplayEnable = TRUE;
                }
            }
        }
        ViewPort_p++;
    } /* End scan all viewports */

    /* Ensure nobody else accesses layers/viewports. */
   semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
} /* end of stvid_DvpPictureCharacteristicsChange() function */
#endif /* #ifdef ST_diginput && ST_MasterDigInput */

/*******************************************************************************
Name        : stvid_GraphicsPictureCharacteristicsChange
Description : called from the event function stvid_PictureCharacteristicsChange()
Parameters  : EVT API
Assumptions : EventData_p and SubscriberData_p shouldn't be null
Limitations :
Returns     :
*******************************************************************************/
void stvid_GraphicsPictureCharacteristicsChange(
        VideoViewPort_t * const ViewPort_p,
        VIDDISP_PictureCharacteristicsChangeParams_t * const PictureCharacteristicsParams_p)
{
    STVID_PictureInfos_t *                      PictureInfos_p;
    ST_ErrorCode_t                              ErrorCode = ST_NO_ERROR;
    STLAYER_ViewPortParams_t                    AdjustedLayerViewport;
    STVID_DigitalInputWindows_t                 WindowsForDvp;

    /* Retieving PictureInfos */
    PictureInfos_p = PictureCharacteristicsParams_p->PictureInfos_p;


    if (PictureInfos_p != NULL)
    {
        /* Retrieving all Bitmap source characteristics */
        *(ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p) = PictureInfos_p->BitmapParams;
    }

    switch (PictureCharacteristicsParams_p->CharacteristicsChanging)
    {
        case VIDDISP_CHANGING_ONLY_FRAME_CENTER_OFFSETS :
            /* no break */
        case VIDDISP_CHANGING_EVERYTHING :
            /* Process the parameters. */
            ErrorCode = DVP_ChangeViewportParams( ViewPort_p, &AdjustedLayerViewport);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DVP_ChangeViewportParams failed. Error = 0x%x !", ErrorCode));
            }
            else
            {
                /* Apply to layer viewport */
                ShareWindowsLayerVSDigitalInput( &AdjustedLayerViewport,
                                   &WindowsForDvp,
                                   ViewPort_p->Identity.Layer_p->LayerType);

                ErrorCode = STLAYER_SetViewPortParams( ViewPort_p->Identity.LayerViewPortHandle, &AdjustedLayerViewport);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetViewPortParams failed. Error = 0x%x !", ErrorCode));
                }
            }
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "No reason to change."));
            break;
    }

    /* Check enable view port delayed. */
    if (ViewPort_p->Params.DisplayEnableDelayed)
    {
        ViewPort_p->Params.DisplayEnableDelayed = FALSE;

        /* Enable view port now that a new picture is coming */
        ErrorCode = STLAYER_EnableViewPort( ViewPort_p->Identity.LayerViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableViewPort() failed. Error = 0x%x !", ErrorCode));
            /* No error allowed for this function ! */
            ErrorCode = ST_NO_ERROR;
        }
        else
        {
            ViewPort_p->Params.DisplayEnable = TRUE;
        }
    }

} /* end of stvid_GraphicsPictureCharacteristicsChange() function */

#endif /* ST_display */

#if defined (ST_diginput) && defined (ST_MasterDigInput)
/*******************************************************************************
Name        : stvid_DvpSetIOWindows
Description : Set the aligned IO Window
Parameters  :
Assumptions : ViewPortHandle should be a valid handle.
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stvid_DvpSetIOWindows(
        const STVID_ViewPortHandle_t ViewPortHandle,
        S32 const InputWinX,            S32 const InputWinY,
        U32 const InputWinWidth,        U32 const InputWinHeight,
        S32 const OutputWinX,           S32 const OutputWinY,
        U32 const OutputWinWidth,       U32 const OutputWinHeight)
{
    VideoViewPort_t       *ViewPort_p;
    STVID_PictureInfos_t  CurrentDisplayedPictureInfos;
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;
    STLAYER_ViewPortParams_t    AdjustedLayerViewport;
    STVID_DigitalInputWindows_t WindowsForDvp;

    /* Retrieve the local ViewPort structure from the ViewPortHandle.        */
    ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

    /* Ensure nobody else accesses layers/viewports. */
    semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionX
                        = InputWinX;
    ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionY
                        = InputWinY;
    ViewPort_p->Params.VideoViewportParameters.InputRectangle.Width
                        = InputWinWidth;
    ViewPort_p->Params.VideoViewportParameters.InputRectangle.Height
                        = InputWinHeight;
    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionX
                        = OutputWinX;
    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionY
                        = OutputWinY;
    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Width
                        = OutputWinWidth;
    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Height
                        = OutputWinHeight;

    ErrorCode = DVP_ChangeViewportParams(ViewPort_p,&AdjustedLayerViewport);
    /* apply to DVP */
    ShareWindowsLayerVSDigitalInput(&AdjustedLayerViewport,
                                &WindowsForDvp,
                                ViewPort_p->Identity.Layer_p->LayerType);
    STEVT_Notify(ViewPort_p->Identity.Device_p->DeviceData_p->EventsHandle,
                        ViewPort_p->Identity.Device_p->DeviceData_p->
                                    EventID[STVID_DIGINPUT_WIN_CHANGE_ID],
                        STEVT_EVENT_DATA_TYPE_CAST &WindowsForDvp);

    if (ErrorCode == ST_NO_ERROR)
    {
#ifdef ST_display
        /* Ask display if there is a picture on display.                         */
        ErrorCode = VIDDISP_GetDisplayedPictureInfos(
                        ViewPort_p->Identity.Device_p->DeviceData_p->DisplayHandle,
                        &CurrentDisplayedPictureInfos);
        if (ErrorCode == ST_NO_ERROR)
        {
            if ((WindowsForDvp.OutputWidth
                                == ViewPort_p->Params.CurrentOutputWidthForDVP)
                    &&(WindowsForDvp.OutputHeight
                                == ViewPort_p->Params.CurrentOutputHeightForDVP))
            {
                /* DVP won't produce a pic characteristic change evt */
                /* apply direct to layer gfx */
                ErrorCode = STLAYER_SetViewPortParams(
                            ViewPort_p->Identity.LayerViewPortHandle,
                            &AdjustedLayerViewport);
            }
            else
            {
                ViewPort_p->Params.CurrentOutputWidthForDVP
                                                    = WindowsForDvp.OutputWidth;
                ViewPort_p->Params.CurrentOutputHeightForDVP
                                                    = WindowsForDvp.OutputHeight;
            }
        }
        else
#endif /* ST_display */
        {
            /* This is not an error case.   */
            ErrorCode = ST_NO_ERROR;
        }
    }
    /* Ensure nobody else accesses layers/viewports. */
    semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    return(ErrorCode);
} /* End of stvid_DvpSetIOWindows() function */
#endif /* #ifdef ST_diginput && ST_MasterDigInput */

/*******************************************************************************
Name        : DVP_ChangeViewportParams
Description : 1. compute the input and output.
              2. calculate the scaling and adjust the input and output windows.
              3. center the output according to the WinParams.
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
                AdjustedLayerViewport_p should be not null
Limitations :
Returns     : ST_NO_ERROR, STVID_ERROR_NOT_AVAILABLE
*******************************************************************************/
static ST_ErrorCode_t DVP_ChangeViewportParams(
        VideoViewPort_t * const ViewPort_p,
        STLAYER_ViewPortParams_t  * const AdjustedLayerViewport_p)
{
    S32                       InputWinX;
    S32                       InputWinY;
    U32                       InputWinWidth;
    U32                       InputWinHeight;
    S32                       OutputWinX;
    S32                       OutputWinY;
    U32                       OutputWinWidth;
    U32                       OutputWinHeight;
    ST_ErrorCode_t            ErrorCode;
    STLAYER_ViewPortSource_t  Source;
    STGXOBJ_Bitmap_t          Bitmap;
    STGXOBJ_Palette_t         Palette;
    STLAYER_StreamingVideo_t  VideoStream;

    /* Store the parameters. */
    InputWinX      = ViewPort_p->Params.VideoViewportParameters.InputRectangle.
                            PositionX;
    InputWinY      = ViewPort_p->Params.VideoViewportParameters.InputRectangle.
                            PositionY;
    InputWinWidth  = ViewPort_p->Params.VideoViewportParameters.InputRectangle.
                            Width;
    InputWinHeight = ViewPort_p->Params.VideoViewportParameters.InputRectangle.
                            Height;
    OutputWinX     = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.
                            PositionX;
    OutputWinY     = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.
                            PositionY;
    OutputWinWidth = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.
                            Width;
    OutputWinHeight= ViewPort_p->Params.VideoViewportParameters.OutputRectangle.
                            Height;

    /* Input Window */
    /*--------------*/
    if (ViewPort_p->Params.InputWinAutoMode)
    {
        /* automatic Input */
        ErrorCode = ComputeInputWindow(ViewPort_p,
                                      &AdjustedLayerViewport_p->InputRectangle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "ComputeInputWindow failed. Error = 0x%x !",
                        ErrorCode));
            return( STVID_ERROR_NOT_AVAILABLE);
        }
    }
    else
    {
        /* manual Input */
        AdjustedLayerViewport_p->InputRectangle.PositionX    = InputWinX;
        AdjustedLayerViewport_p->InputRectangle.PositionY    = InputWinY;
        AdjustedLayerViewport_p->InputRectangle.Width        = InputWinWidth;
        AdjustedLayerViewport_p->InputRectangle.Height       = InputWinHeight;
    }
    /* Output Window */
    /*---------------*/
    if (ViewPort_p->Params.OutputWinAutoMode)
    {
        /* automatic Output */
        ErrorCode = ComputeOutputWindow(ViewPort_p,
                                     &AdjustedLayerViewport_p->OutputRectangle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "ComputeOutputWindow failed. Error = 0x%x !",
                        ErrorCode));
            return( STVID_ERROR_NOT_AVAILABLE);
        }
    }
    else
    {
        /* manual Output */
        AdjustedLayerViewport_p->OutputRectangle.PositionX   = OutputWinX;
        AdjustedLayerViewport_p->OutputRectangle.PositionY   = OutputWinY;
        AdjustedLayerViewport_p->OutputRectangle.Width       = OutputWinWidth;
        AdjustedLayerViewport_p->OutputRectangle.Height      = OutputWinHeight;
    }

    /* Now we have the parameters we intend to use. */
    /* Retrieve source from viewport */

    Source.Palette_p                            = &Palette;

    if (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP)
    {
        Source.Data.BitMap_p                        = &Bitmap;
    }
    else
    {
        Source.Data.VideoStream_p                   = &VideoStream;
    }

    STLAYER_GetViewPortSource(ViewPort_p->Identity.LayerViewPortHandle,&Source);

    AdjustedLayerViewport_p->Source_p       = ViewPort_p->Params.
                                               VideoViewportParameters.Source_p;

    if (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP)
    {
        AdjustedLayerViewport_p->Source_p->Data.BitMap_p->Data1_p  =
                                            Source.Data.BitMap_p->Data1_p;
        AdjustedLayerViewport_p->Source_p->Data.BitMap_p->Data2_p  =
                                            Source.Data.BitMap_p->Data2_p;
        AdjustedLayerViewport_p->Source_p->Data.BitMap_p->ColorType=
                                            Source.Data.BitMap_p->ColorType;
    }
    else /* STLAYER_STREAMING_VIDEO */
    {
        AdjustedLayerViewport_p->Source_p->Data.VideoStream_p->BitmapParams.Data1_p  =
                                        Source.Data.VideoStream_p->BitmapParams.Data1_p;
        AdjustedLayerViewport_p->Source_p->Data.VideoStream_p->BitmapParams.Data2_p  =
                                        Source.Data.VideoStream_p->BitmapParams.Data2_p;
        AdjustedLayerViewport_p->Source_p->Data.VideoStream_p->BitmapParams.ColorType=
                                        Source.Data.VideoStream_p->BitmapParams.ColorType;
    }

    /* Save the last I/O windows set to the video layer viewport              */
    /* before being adjusted.                                                 */
    ViewPort_p->Params.ViewportParametersAskedToLayerViewport
                                            = *AdjustedLayerViewport_p;

    return(ST_NO_ERROR);

} /* End of DVP_ChangeViewportParams() function */

/*******************************************************************************
Name        : ComputeOutputWindow
Description : Compute the Output Window
Parameters  :
Assumptions : Function calls are protected with  LayersViewportsLockingSemaphore_p
                OutputRectangle_p shouldn't be null
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t ComputeOutputWindow(VideoViewPort_t * const ViewPort_p,
                        STGXOBJ_Rectangle_t * const OutputRectangle_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STLAYER_LayerParams_t   LayerParams;
    U16                     Width, Height;
    STGXOBJ_AspectRatio_t   SourceAspectRatio;

    /* Affectations to facilitate the read.                                  */
    if (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP)
    {
        SourceAspectRatio = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->AspectRatio;
    }
    else /* STLAYER_STREAMING_VIDEO */
    {
        SourceAspectRatio =
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.AspectRatio;
    }
    /* Retrieve the Layer Parameters. Call to STLAYER_GetLayerParams         */
    /* =output fullscreen                                                    */
    ErrorCode = STLAYER_GetLayerParams(
            ViewPort_p->Identity.Layer_p->LayerHandle, &LayerParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STLAYER_GetLayerParams failed. Error = 0x%x !",
                        ErrorCode));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* default values */
    OutputRectangle_p->PositionX    = 0;
    OutputRectangle_p->PositionY    = 0;
    OutputRectangle_p->Width        = LayerParams.Width;
    OutputRectangle_p->Height       = LayerParams.Height;

    if (!                                                    /* if NOT :      */
        ((ViewPort_p->Params.InputWinAutoMode == 0)         /* input is manu  */
        || (SourceAspectRatio == LayerParams.AspectRatio)  /* in AR = out AR  */
        ||((SourceAspectRatio != STGXOBJ_ASPECT_RATIO_4TO3)  /* only 4/3 and  */
         &&(SourceAspectRatio != STGXOBJ_ASPECT_RATIO_16TO9))/* 16/9 are calc */
        ||((LayerParams.AspectRatio != STGXOBJ_ASPECT_RATIO_4TO3)
         &&(LayerParams.AspectRatio != STGXOBJ_ASPECT_RATIO_16TO9))))
    {
        /* perform the aspect ratio */
        /*--------------------------*/
        switch (ViewPort_p->Params.DispARConversion)
        {
            /* In case of letter box display mode only.*/
            case STVID_DISPLAY_AR_CONVERSION_LETTER_BOX:
                /* If 4/3 Stream on a 16/9 TV => Equal vertical bars.*/
                if (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3)
                {
                    /* the width displayed of the picture will be 3/4 of the  */
                    /* full layer width.*/
                    Width = (3 * LayerParams.Width) / 4;
                    OutputRectangle_p->PositionX = (LayerParams.Width-Width)/ 2;
                    OutputRectangle_p->Width = Width;
                    OutputRectangle_p->Height = LayerParams.Height;
                }
                /* If 16/9 Stream on a 4/3 TV => Equal horizontal bars.*/
                else
                {
                    Height = (3 * LayerParams.Height) / 4;
                    OutputRectangle_p->PositionY
                        = (LayerParams.Height - Height) / 2;
                    OutputRectangle_p->Height = Height;
                }
                break;

            /* In case of combined display mode only.*/
            case STVID_DISPLAY_AR_CONVERSION_COMBINED:
                /* If 4/3 Stream on a 16/9 TV => Equal vertical bars.*/
                if (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3)
                {
                    Width = (14 * LayerParams.Width) / 16;
                    OutputRectangle_p->PositionX = (LayerParams.Width-Width)/ 2;
                    OutputRectangle_p->Width = Width;
                    OutputRectangle_p->Height = LayerParams.Height;
                }
                /* If 16/9 Stream on a 4/3 TV => Equal horizontal bars.*/
                else
                {
                    Height = (14 * LayerParams.Height) / 16;
                    OutputRectangle_p->PositionY
                        = (LayerParams.Height - Height) / 2;
                    OutputRectangle_p->Height = Height;
                }
                break;

            default:
                /* Pan&Scan and Ignore modes => Full screen window.*/
                /* nothing to do */
                break;
        }
    }

    return(ErrorCode);
} /* End of ComputeOutputWindow() function */

/*******************************************************************************
Name        : ComputeInputWindow
Description : Compute the Input Windows
Parameters  :
Assumptions : Function calls are protected with  LayersViewportsLockingSemaphore_p
                InputRectangle_p shouldn't be null
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t ComputeInputWindow(VideoViewPort_t * const ViewPort_p,
                        STGXOBJ_Rectangle_t * const InputRectangle_p)
{
    STLAYER_LayerParams_t LayerParams;
    U32                   SourceWidth, SourceHeight;
    STGXOBJ_AspectRatio_t SourceAspectRatio;
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

    /* Affectations to facilitate the read.                                  */
    if (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP)
    {
        SourceWidth  = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Width;
        SourceHeight = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Height;
        SourceAspectRatio = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->AspectRatio;
    }
    else /* STLAYER_STREAMING_VIDEO */
    {
        SourceWidth  = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Width;
        SourceHeight = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Height;
        SourceAspectRatio = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.AspectRatio;
    }
    /* Retrieve the Layer Parameters. Call to STLAYER_GetLayerParams           */
    ErrorCode = STLAYER_GetLayerParams(
            ViewPort_p->Identity.Layer_p->LayerHandle,
            &LayerParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "STLAYER_GetLayerParams failed. Error = 0x%x !", ErrorCode));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* default values */
    InputRectangle_p->PositionX   = 0;
    InputRectangle_p->PositionY   = 0;
    InputRectangle_p->Width       = SourceWidth;
    InputRectangle_p->Height      = SourceHeight;
    /* patched values if req (pan & scan) */
    if (((SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) &&
        (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9))
     ||
        ((SourceAspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) &&
        (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3)))
    {
        /* Pan and Scan possible and requested.                          */
        switch(ViewPort_p->Params.DispARConversion)
        {
            case  STVID_DISPLAY_AR_CONVERSION_PAN_SCAN:
                /* 4/3 stream on 16/9 TV => Loose Up/Down horizontal bars.*/
                if (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3)
                {
                    InputRectangle_p->Height = (3 * SourceHeight) / 4;
                    InputRectangle_p->PositionY = 16 *
                        ((SourceHeight - InputRectangle_p->Height)/2);
                }
                else
                {
                    /* 16/9 stream on 4/3 TV => Loose l/r verti bars.  */
                    InputRectangle_p->Width = (3 * SourceWidth) / 4;
                    InputRectangle_p->PositionX = 16 *
                        ((SourceWidth - InputRectangle_p->Width) / 2);
                }
                break;

            case STVID_DISPLAY_AR_CONVERSION_COMBINED:
                /* 4/3 stream on 16/9 TV => Loose Up/Down horizontal bars.*/
                if (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3)
                {
                    InputRectangle_p->Height = (14 * SourceHeight) / 16;
                    InputRectangle_p->PositionY = 16 *
                        ((SourceHeight - InputRectangle_p->Height) / 2);
                }
                else
                {
                    /* 16/9 stream on 4/3 TV => Loose l/r vertical bars.  */
                    InputRectangle_p->Width = (14 * SourceWidth) / 16;
                    InputRectangle_p->PositionX = 16 *
                        ((SourceWidth - InputRectangle_p->Width) / 2);
                }
                break;

            default:
                /* Pan&Scan and Ignore modes => Full screen window.*/
                /* nothing to do */
                break;
        }/* end switch AR */
    }/*endif Pan&Scan req */
    return (ST_NO_ERROR);
} /* end of ComputeInputWindow() function */

/*******************************************************************************
Name        : ShareWindowsLayerVSDigitalInput
Description :
Parameters  :
Assumptions : fct called only if the layer is a graphic one
                AdjustedLayerViewport_p and WindowsForDvp shouldn't be null
Limitations :
Returns     : none
*******************************************************************************/
static void ShareWindowsLayerVSDigitalInput(
        STLAYER_ViewPortParams_t * const AdjustedLayerViewport_p,
        STVID_DigitalInputWindows_t * const WindowsForDvp_p,
        STLAYER_Layer_t         LayerType)
{

    /* Note : */
    /* AdjustedLayerViewport_p contains the requested windows.  */
    /* AdjustedLayerViewport_p need to be patched to be used to */
    /* set the layer windows                                    */
    /* WindowsForDvp_p contains the windows for the DVP           */

    /* 2: DVP input = API input */
    /* only the requested window is captured */
    WindowsForDvp_p->InputRectangle =
        AdjustedLayerViewport_p->InputRectangle;

    /* 3: Layer output = API output */
    /* already done */

    /* 4: DVP output = captured */
    /* (implicit : x=y=0 in the captured buffer) */
    if(AdjustedLayerViewport_p->InputRectangle.Width
            > AdjustedLayerViewport_p->OutputRectangle.Width)
    {
        /* zoom-out done by DVP */

        if ((AdjustedLayerViewport_p->InputRectangle.Width > MAX_SD_OUTPUT_HORIZONTAL_SIZE) &&
            (LayerType == STLAYER_HDDISPO2_VIDEO2))
        {
            WindowsForDvp_p->OutputWidth
                = MAX_SD_OUTPUT_HORIZONTAL_SIZE;
        }
        else
        {
            WindowsForDvp_p->OutputWidth
                = AdjustedLayerViewport_p->InputRectangle.Width;
        }
    }
    else
    {
        /* zoom-in done by layer */
        WindowsForDvp_p->OutputWidth
            = AdjustedLayerViewport_p->InputRectangle.Width;
    }
/* Vertical Zoom not supported on 7109 DVP */
    /* Initilalization, to avoid 0 value for 7109 */
    WindowsForDvp_p->OutputHeight = AdjustedLayerViewport_p->InputRectangle.Height;
#ifndef ST_7109
    if(AdjustedLayerViewport_p->InputRectangle.Height
            > AdjustedLayerViewport_p->OutputRectangle.Height)
    {
        /* zoom-out done by DVP */
        WindowsForDvp_p->OutputHeight
            = AdjustedLayerViewport_p->OutputRectangle.Height;
    }
    else
    {
        /* zoom-in done by layer */
        WindowsForDvp_p->OutputHeight
            = AdjustedLayerViewport_p->InputRectangle.Height;
    }
#endif /* ST_7109 */

    /* 5: layer Input = captured */
    AdjustedLayerViewport_p->InputRectangle.PositionX = 0; /* implicit */
    AdjustedLayerViewport_p->InputRectangle.PositionY = 0; /* implicit */
    AdjustedLayerViewport_p->InputRectangle.Width     =
                                                   WindowsForDvp_p->OutputWidth;
/* Vertical Zoom not supported on 7109 DVP */
#ifndef ST_7109
    AdjustedLayerViewport_p->InputRectangle.Height    =
                                                  WindowsForDvp_p->OutputHeight;
#endif /* ST_7109 */

}/* End of ShareWindowsLayerVSDigitalInput() function */


/* end of vid_dvpw.c */
