/*******************************************************************************

File name   : vid_comp.c

Description : Video composition functions commands source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                            Name
----               ------------                                            ----
 23 Jun 2000        Created                                                 AN
 23 Jan 2001        Fix the bugs found as severity 1 in the code review     AN
 29 Jun 2001        For multi viewport success: don't subscribe events all
                    the time: now subscribe to characteristics change event
                    at init, and subscribe to LAYER update params event when
                    first opening a view port on layer (unsubs. when close) HG
 21 Aug 2001        Add generic digital input support                       TM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Define to add debug info and traces                                        */
/*#define TRACE_COMPOSITION*/

#ifdef TRACE_COMPOSITION
#include "trace.h"
#define COMP_TraceBuffer(x) TraceBuffer(x)
#else  /* TRACE_COMPOSITION */
#define COMP_TraceBuffer(x) {}
#endif /* TRACE_COMPOSITION */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "api.h"
#include "vid_err.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
#define BAD_LAYER_INDEX         0xffff
#define ENABLE_SLAVE_DISPLAY_SEQUENCER      TRUE
#define DISABLE_SLAVE_DISPLAY_SEQUENCER     FALSE
#define ZOOM_RATIO_CLOSE_TO_UNITY 103   /* 3% is the limit for disabling resizing (it's above 720/704 and below action safe area overscan)*/

/* Private Variables (static)------------------------------------------------ */

const STLAYER_QualityOptimizations_t DefaultQualityOptimizations = { TRUE, TRUE };

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */
/* Passing a (STVID_ViewPortHandle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidViewPortHandle(Handle)                                       \
  ((((VideoViewPort_t *) (Handle)) != NULL) &&                              \
   (((VideoViewPort_t *) (Handle))->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE) && \
   ((((VideoViewPort_t *) (Handle))->Identity.Device_p) != NULL) &&         \
   (((((VideoViewPort_t *) (Handle))->Identity.Device_p)->DeviceData_p) != NULL) && \
   ((VideoViewPort_t *) (Handle) >= ((VideoViewPort_t *) ((VideoViewPort_t *) (Handle))->Identity.Device_p->DeviceData_p->ViewportProperties_p)) && \
   ((VideoViewPort_t *) (Handle) <  ((VideoViewPort_t *) ((VideoViewPort_t *) (Handle))->Identity.Device_p->DeviceData_p->ViewportProperties_p) + ((VideoViewPort_t *) (Handle))->Identity.Device_p->DeviceData_p->MaxViewportNumber))

/* Function prototypes ------------------------------------------------------ */


/* Private Function prototypes ---------------------------------------------- */

#ifdef ST_display
static void ActionSetIOWindows(VideoViewPort_t * ViewPort_p,
                    S32 InputWinX, S32 InputWinY, U32 InputWinWidth, U32 InputWinHeight,
                    S32 OutputWinX, S32 OutputWinY, U32 OutputWinWidth, U32 OutputWinHeight);
#endif /* ST_display */
#ifdef ST_producer
static ST_ErrorCode_t AffectDecodedPicturesAndTheBuffersFreeingStrategy(VideoDevice_t * const Device_p, const U8 FrameBufferHoldTime);
#endif /* ST_producer */
static void UpdateDecimationFromLayer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void UpdateParamsFromLayer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static ST_ErrorCode_t SetVTGParams(VideoDevice_t * const Device_p,
                                    STLAYER_VTGParams_t * const VTGParams_p,
                                    const ST_DeviceName_t LayerName);
static ST_ErrorCode_t ComputeOutputWindow(VideoViewPort_t * const ViewPort_p,
                        STGXOBJ_Rectangle_t * const OutputRectangle_p);
static ST_ErrorCode_t ComputeInputWindow(VideoViewPort_t * const ViewPort_p,
                        STGXOBJ_Rectangle_t * const InputRectangle_p);
static ST_ErrorCode_t CenterOutput(VideoViewPort_t * const ViewPort_p,
        const STGXOBJ_Rectangle_t * const OutputRectangle_p,
        STGXOBJ_Rectangle_t * const AdjustedOutputRectangle_p);
#ifdef ST_display
static ST_ErrorCode_t ChangeViewportInputParams(VideoViewPort_t * const ViewPort_p);
#endif /* ST_display */
static ST_ErrorCode_t ChangeViewportParams(VideoViewPort_t * const ViewPort_p);

static void ConvertDecimationValues (STVID_DecimationFactor_t  VideoDecimationFactor,
                                    STLAYER_DecimationFactor_t *LayerHorizontalDecimationFactor,
                                    STLAYER_DecimationFactor_t *LayerVerticalDecimationFactor);

static ST_ErrorCode_t CloseLayerForOldViewport(VideoDevice_t * const Device_p, ST_DeviceName_t LayerName, const BOOL FinishOpenedNotClosedInCaseOfProblem);
static ST_ErrorCode_t OpenLayerForNewViewport(VideoDevice_t * const Device_p,
    ST_DeviceName_t LayerName, VideoLayer_t ** const VideoLayer_p);
static U32 IdentOfNamedLayer(VideoDeviceData_t * DeviceData_p,
                             const ST_DeviceName_t LayerName);
static void vidcomp_CheckAndAdjustManualInputParams(
        STGXOBJ_Rectangle_t     * const InputRectangle_p,
        VideoViewPort_t         * const ViewPort_p);

/* Private functions -------------------------------------------------------- */

static U32 IdentOfNamedLayer(VideoDeviceData_t * DeviceData_p,
                             const  ST_DeviceName_t LayerName)
{
    U32 Id;
    Id = 0;
    while (Id < DeviceData_p->MaxLayerNumber)
    {
        if(strcmp(LayerName,DeviceData_p->LayerProperties_p[Id].LayerName) == 0)
        {
            /* layer is found */
            return(Id);
        }
        Id++;
    }
    return(BAD_LAYER_INDEX);
}

#ifdef ST_display
/*******************************************************************************
Name        : ActionSetIOWindows
Description : Actions to IOWindowsview port
Parameters  : viewport pointer

Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ActionSetIOWindows(VideoViewPort_t * ViewPort_p,
                    S32 InputWinX, S32 InputWinY, U32 InputWinWidth, U32 InputWinHeight,
                    S32 OutputWinX, S32 OutputWinY, U32 OutputWinWidth, U32 OutputWinHeight)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    /* Store the SetIOWindows parameters. They could be used in case of a      */
    /* STVID_ShowPicture.                                                      */
    (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.PositionX   = (InputWinX);  /* 1/16 pixel unit used when needed */
    (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.PositionY   = (InputWinY);  /* 1/16 pixel unit used when needed */
    (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.Width       = (InputWinWidth);
    (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.Height      = (InputWinHeight);
    (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.PositionX  = (OutputWinX);
    (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.PositionY  = (OutputWinY);
    (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.Width      = (OutputWinWidth);
    (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.Height     = (OutputWinHeight);

    /* Process the parameters. */
    ErrorCode = ChangeViewportParams((ViewPort_p));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ChangeViewportParams() failed. Error = 0x%x !", ErrorCode));
    }
    else
    {
        /* Restore the SetIOWindows parameters. They could be asked by STVID_GetIOWindows */
        (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.PositionX   = (InputWinX); /* 1/16 pixel unit used when needed */
        (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.PositionY   = (InputWinY); /* 1/16 pixel unit used when needed */
        (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.Width       = (InputWinWidth);
        (ViewPort_p)->Params.VideoViewportParameters.InputRectangle.Height      = (InputWinHeight);
        (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.PositionX  = (OutputWinX);
        (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.PositionY  = (OutputWinY);
        (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.Width      = (OutputWinWidth);
        (ViewPort_p)->Params.VideoViewportParameters.OutputRectangle.Height     = (OutputWinHeight);
    }
} /* end of ActionSetIOWindows() function */
#endif /* ST_display */

/*******************************************************************************
Name        : stvid_ActionCloseViewPort
Description : Actions to close view port
Parameters  : viewport pointer
              parameter to re-open viewport if problem or NULL to force close.
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t stvid_ActionCloseViewPort(VideoViewPort_t * const ViewPort_p, STLAYER_ViewPortParams_t * const ViewportParamsForReOpenOrNullToForceClose_p)
{
    VideoDevice_t *     Device_p;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    Device_p = ViewPort_p->Identity.Device_p;

    ViewPort_p->Identity.ViewPortOpenedAndValidityCheck = 0 ;
    /* Call to STLAYER_CloseViewPort. */
    ErrorCode = STLAYER_CloseViewPort(ViewPort_p->Identity.LayerViewPortHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_CloseViewPort() failed. Error = 0x%x !", ErrorCode));
        if (ViewportParamsForReOpenOrNullToForceClose_p == NULL)
        {
            /* Force close: continue close... */
            ErrorCode = ST_NO_ERROR;
        }
        else
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        if (ViewportParamsForReOpenOrNullToForceClose_p != NULL)
        {
            /* try to close with possibility to re-open */
            ErrorCode = CloseLayerForOldViewport(Device_p, ViewPort_p->Identity.Layer_p->LayerName, TRUE);
        }
        else
        {
            /* Force close: close layer with final state closed */
            ErrorCode = CloseLayerForOldViewport(Device_p, ViewPort_p->Identity.Layer_p->LayerName, FALSE);
        }
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CloseLayerForOldViewport() failed. Error = 0x%x !", ErrorCode));
            if (ViewportParamsForReOpenOrNullToForceClose_p == NULL)
            {
                /* Force close: continue close... */
                ErrorCode = ST_NO_ERROR;
            }
            else
            {
                /* Un-do actions closed. */
                ErrorCode = STLAYER_OpenViewPort(ViewPort_p->Identity.Layer_p->LayerHandle,
                        ViewportParamsForReOpenOrNullToForceClose_p, &ViewPort_p->Identity.LayerViewPortHandle);
                        /* temporary cast while STLAYER doesn't use cast's */
/*                        ViewportParamsForReOpen_p, &ViewPort_p->Identity.LayerViewPortHandle);*/
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_OpenViewPort() failed. Error = 0x%x !", ErrorCode));
                }
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Clear */
            /* Don't allow access to the viewoort through STVID calls any more. */
            ViewPort_p->Identity.ViewPortOpenedAndValidityCheck = 0; /* not VIDAPI_VALID_VIEWPORT_HANDLE */
            ViewPort_p->Identity.Layer_p = NULL;
        } /* end of CloseLayerForOldViewport() success */
        else
        {
            ViewPort_p->Identity.ViewPortOpenedAndValidityCheck = VIDAPI_VALID_VIEWPORT_HANDLE;
        }
    } /* end of STLAYER_CloseViewPort() success */

    return(ErrorCode);
} /* end of stvid_ActionCloseViewPort() function */

/*******************************************************************************
Name        : stvid_ActionEnableOutputWindow
Description : Actions to enable output window
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stvid_ActionEnableOutputWindow(VideoViewPort_t * const ViewPort_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoDevice_t *         Device_p;
#ifdef ST_display
    STVID_PictureInfos_t    CurrentDisplayedPictureInfos;
#endif /* ST_display */

    /* Retrieve the Decoder structure from the local ViewPort structure.       */
    Device_p = ViewPort_p->Identity.Device_p;

    /* If the ViewPort display is disable then call STLAYER_EnableViewPort     */
    /* in order to enable it.                                                  */
    if (!(ViewPort_p->Params.DisplayEnable))
    {
        /* Ask display if there is a picture on display */
#ifdef ST_display
        ErrorCode = VIDDISP_GetDisplayedPictureInfos(Device_p->DeviceData_p->DisplayHandle, &CurrentDisplayedPictureInfos);
        if (ErrorCode == ST_NO_ERROR)
        {
            /* There's a picture on display: enable view port */
            ErrorCode = STLAYER_EnableViewPort(ViewPort_p->Identity.LayerViewPortHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableViewPort() failed. Error = 0x%x !", ErrorCode));
                ErrorCode = STVID_ERROR_NOT_AVAILABLE;
            }
            else
            {
                ViewPort_p->Params.DisplayEnable             = TRUE;
            }
        }
        else
        {
#endif /* ST_display */
            ErrorCode = ST_NO_ERROR;
            /* There's no picture on display: delay the enable view port */
            ViewPort_p->Params.DisplayEnableDelayed = TRUE;

#ifdef ST_display
        } /* end must delay view port enable */
#endif /* ST_display */
    } /* End view port disabled */
    return(ErrorCode);
} /* end of stvid_ActionEnableOutputWindow() function */

/*******************************************************************************
Name        : stvid_ActionDisableOutputWindow
Description : Actions to disable output window
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stvid_ActionDisableOutputWindow(VideoViewPort_t * const ViewPort_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* If the ViewPort display is enable then call STLAYER_DisableViewPort     */
    /* in order to disable it.                                                 */
    if (ViewPort_p->Params.DisplayEnable)
    {
        ErrorCode = STLAYER_DisableViewPort(ViewPort_p->Identity.LayerViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_DisableViewPort() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
        else
        {
            ViewPort_p->Params.DisplayEnable = FALSE;
        }
    }
    else if (ViewPort_p->Params.DisplayEnableDelayed)
    {
        /* Enable flag delay was set: still no picture on display and viewport was enabled: disable it */
        ViewPort_p->Params.DisplayEnableDelayed = FALSE;
    }

    return(ErrorCode);
} /* end of stvid_ActionDisableOutputWindow() function */


/*******************************************************************************
Name        : SetVTGParams
Description : Set the VTG Name and frame rate to Display, Trickmode and Error Recovery.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetVTGParams(VideoDevice_t * const Device_p,
                                    STLAYER_VTGParams_t * const VTGParams_p,
                                   const  ST_DeviceName_t LayerName)
{
    BOOL            MainNotAux = TRUE;
    U32             LayerIndex;
    ST_ErrorCode_t  ReturnErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Ensure nobody else accesses VTG name or params */
    semaphore_wait(Device_p->DeviceData_p->VTGParamsChangeLockingSemaphore_p);

    /* Check inconsistencies: if frame rate is outside incredible range, use default value instead (just take care: value 0 is OK).
    This happened when application was given FrameRate in Hz instead of mHz. So FrameRate was 1000 times
    smaller than the reality, leading to calculations of delays 1000 time longer: no more synchro was taking place ! */
    if (((VTGParams_p->VTGFrameRate < STVID_MIN_VALID_VTG_FRAME_RATE) || (VTGParams_p->VTGFrameRate > STVID_MAX_VALID_VTG_FRAME_RATE)) &&
         (VTGParams_p->VTGFrameRate != 0))
    {
        /* Check if Hz were not given instead of mHz ... */
        VTGParams_p->VTGFrameRate *= 1000;
        if ((VTGParams_p->VTGFrameRate < STVID_MIN_VALID_VTG_FRAME_RATE) || (VTGParams_p->VTGFrameRate > STVID_MAX_VALID_VTG_FRAME_RATE))
        {
            /* VTGFrameRate is bad and 1000*VTGFrameRate also: better pass a default and
            imprecise value to internal functions,rather than having incredible calculations... */
            VTGParams_p->VTGFrameRate = STVID_DEFAULT_VALID_VTG_FRAME_RATE;
        }
    }
    LayerIndex = IdentOfNamedLayer(Device_p->DeviceData_p,LayerName);

    /* In the case if display on auxiliary and secondary (specific 7200) */
    if (( Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
          Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
          (Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
          Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ))
    {
        /* The Main layer have been chosen arbitrary on VIDEO4 */
        if (Device_p->DeviceData_p->LayerProperties_p[LayerIndex].LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
            MainNotAux = FALSE;
        else if (Device_p->DeviceData_p->LayerProperties_p[LayerIndex].LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
            MainNotAux = TRUE;
    } else
    {
        switch(Device_p->DeviceData_p->LayerProperties_p[LayerIndex].LayerType)
        {
            case STLAYER_OMEGA2_VIDEO1:
            case STLAYER_7020_VIDEO1:
            case STLAYER_HDDISPO2_VIDEO1 :
            case STLAYER_SDDISPO2_VIDEO1 :
            case STLAYER_DISPLAYPIPE_VIDEO1 :
            case STLAYER_DISPLAYPIPE_VIDEO2 :  /* Main display of 7200 */
            case STLAYER_GAMMA_GDP :
                MainNotAux = TRUE;
            break;

            case STLAYER_OMEGA2_VIDEO2:
            case STLAYER_7020_VIDEO2:
            case STLAYER_SDDISPO2_VIDEO2 :
            case STLAYER_HDDISPO2_VIDEO2 :
            case STLAYER_DISPLAYPIPE_VIDEO3 : /* Auxiliary display of 7200 */
            case STLAYER_DISPLAYPIPE_VIDEO4 : /* Secondary display of 7200 */
                MainNotAux = FALSE;
            break;

            default: /* cases 55xx */
                MainNotAux = TRUE;
            break;
        }
    }
#ifdef ST_display
    ErrorCode = VIDDISP_NewVTG(Device_p->DeviceData_p->DisplayHandle,
                               MainNotAux, VTGParams_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        ReturnErrorCode = ErrorCode;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"VIDDISP_NewVTG() failed. Error = 0x%x !", ErrorCode));
    }
#endif /* ST_display */

    /* Store new VTG frame rate. */
    Device_p->DeviceData_p->VTGFrameRate = VTGParams_p->VTGFrameRate;

    ErrorCode = stvid_NewVTGErrorRecovery(Device_p, VTGParams_p->VTGName);
    if (ErrorCode != ST_NO_ERROR)
    {
        ReturnErrorCode = ErrorCode;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"stvid_NewVTGErrorRecovery() failed. Error = 0x%x !", ErrorCode));
    }

    /* Ensure nobody else accesses VTG name or params */
    semaphore_signal(Device_p->DeviceData_p->VTGParamsChangeLockingSemaphore_p);

    return(ReturnErrorCode);
} /* End of SetVTGParams() function */


/*******************************************************************************
Name        : ComputeOutputWindow
Description : Compute the Output Window
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ComputeOutputWindow(VideoViewPort_t * const ViewPort_p,
                        STGXOBJ_Rectangle_t * const OutputRectangle_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STLAYER_LayerParams_t   LayerParams;
    U16                     Width, Height;
    U32                     SourceWidth /*, SourceHeight*/;
    STGXOBJ_AspectRatio_t   SourceAspectRatio;
    VideoDevice_t *         Device_p;

    /* retrive Device structure */
    Device_p = ((VideoUnit_t *)(ViewPort_p->Identity.VideoHandle))->Device_p;

    /* Affectations to facilitate the read.                                  */
    if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)
            ||(ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP))
    {
        SourceWidth = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Width;
        SourceAspectRatio = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->AspectRatio;
    }
    else
    {
        SourceWidth = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Width;
        SourceAspectRatio = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.AspectRatio;
    }

    /* Retrieve the Layer Parameters. Call to STLAYER_GetLayerParams         */
    ErrorCode = STLAYER_GetLayerParams(
            ViewPort_p->Identity.Layer_p->LayerHandle, &LayerParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetLayerParams failed. Error = 0x%x !", ErrorCode));
        return(ST_ERROR_BAD_PARAMETER);
    }


    /* In case of letter box display mode only.                               */
    if (ViewPort_p->Params.DispARConversion ==
            STVID_DISPLAY_AR_CONVERSION_LETTER_BOX)
    {
        /* If 4/3 Stream on a 16/9 TV => Equal vertical bars.                 */
        if ((SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) &&
            (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9))
        {
            /* The width displayed of the picture will be 3 quarters of the full layer width */
            Width = (3 * LayerParams.Width) / 4;

            OutputRectangle_p->PositionX = (LayerParams.Width - Width) / 2;
            OutputRectangle_p->PositionY = 0;
            OutputRectangle_p->Width = Width;
            OutputRectangle_p->Height = LayerParams.Height;
        }
        /* If 16/9 Stream on a 4/3 TV => Equal horizontal bars.              */
        else
        {
            if ((SourceAspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) &&
                (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3))
            {
                Height = 3 * LayerParams.Height / 4;

                OutputRectangle_p->PositionX = 0;
                OutputRectangle_p->PositionY = (LayerParams.Height - Height) / 2;
                OutputRectangle_p->Width = LayerParams.Width;
                OutputRectangle_p->Height = Height;
            }
            /* 16/9 Stream on a 16/9 TV  or  4/3 Stream on a 4/3 TV => Full Screen. */
            else
            {
                OutputRectangle_p->PositionX = 0;
                OutputRectangle_p->PositionY = 0;
                OutputRectangle_p->Width = LayerParams.Width;
                OutputRectangle_p->Height = LayerParams.Height;
            }
        }
    }
    /* In case of combined display mode only.                                */
    else if (ViewPort_p->Params.DispARConversion ==
            STVID_DISPLAY_AR_CONVERSION_COMBINED)
    {
        /* If 4/3 Stream on a 16/9 TV => Equal vertical bars.                */
        if ((SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) &&
            (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9))
        {
            Width = (14 * LayerParams.Width) / 16;

            OutputRectangle_p->PositionX = (LayerParams.Width - Width) / 2;
            OutputRectangle_p->PositionY = 0;
            OutputRectangle_p->Width = Width;
            OutputRectangle_p->Height = LayerParams.Height;
        }
        /* If 16/9 Stream on a 4/3 TV => Equal horizontal bars.              */
        else
        {
            if ((SourceAspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) &&
                (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3))
            {
                Height = 14 * LayerParams.Height / 16;

                OutputRectangle_p->PositionX = 0;
                OutputRectangle_p->PositionY = (LayerParams.Height - Height) / 2;
                OutputRectangle_p->Width = LayerParams.Width;
                OutputRectangle_p->Height = Height;
            }
            /* 16/9 Stream on a 16/9 TV or 4/3 Stream on a 4/3 TV => Full Screen. */
            else
            {
                OutputRectangle_p->PositionX = 0;
                OutputRectangle_p->PositionY = 0;
                OutputRectangle_p->Width = LayerParams.Width;
                OutputRectangle_p->Height = LayerParams.Height;
            }
        }
    }
    /* Pan&Scan and Ignore modes => Full screen window.                      */
    else
    {
        OutputRectangle_p->PositionX = 0;
        OutputRectangle_p->PositionY = 0;
        OutputRectangle_p->Width =  LayerParams.Width;
        OutputRectangle_p->Height = LayerParams.Height;
    }

    return(ErrorCode);
} /* End of ComputeOutputWindow() function */


/*******************************************************************************
Name        : ComputeInputWindow
Description : Compute the Input Windows
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ComputeInputWindow(VideoViewPort_t * const ViewPort_p,
                        STGXOBJ_Rectangle_t * const InputRectangle_p)
{
  STLAYER_LayerParams_t LayerParams;
    BOOL                  AspectRatioConversionIsRequired;
    U32                   SourceWidth, SourceHeight, CroppedSourceWidth, CroppedSourceHeight;
  U32                   PositionX = 0;
  U32                   DisplayHorizontalSize = 0;
  STGXOBJ_AspectRatio_t SourceAspectRatio;
  ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;
  VideoDevice_t*          Device_p;

    /* retrive Device structure */
    Device_p = ViewPort_p->Identity.Device_p;

    /* Affectations to facilitate the read.                                  */
    if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)
            ||(ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP))
    {
        SourceWidth = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Width;
        SourceHeight = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Height;
        SourceAspectRatio = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->AspectRatio;
    }
    else
    {
        SourceWidth = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Width;
        SourceHeight = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Height;
        SourceAspectRatio = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.AspectRatio;
    }

    if (ViewPort_p->Params.StreamParameters.HasDisplaySizeRecommendation)
    {
        DisplayHorizontalSize = ViewPort_p->Params.StreamParameters.DisplayHorizontalSize;
    }

    /* Test special case for ARIB broadcast profile. */
    if (Device_p->DeviceData_p->StartParams.BrdCstProfile == STVID_BROADCAST_ARIB)
    {
        /* Case input vertical size is 1088, but real source    */
        /* vertical size is 1080. So exclude last 8 lines.      */
        if (SourceHeight == 1088) {SourceHeight = 1080;}

        /* Case input horizontal size is 544, but it has to be  */
        /* managed as 540 so that 2 blacks pixels are added at  */
        /* each end of displayed picture.                       */
        if (SourceWidth == 544)
        {
            SourceWidth = 540;
            PositionX   = 2 * 16;
        }

        /* Case display horizontal size is 544 pixel. reduce    */
        /* it to 540 pixels.                                    */
        if (DisplayHorizontalSize == 544)
        {
            DisplayHorizontalSize = 540;
        }
    }

  /* Retrieve the Layer Parameters. Call to STLAYER_GetLayerParams           */
  ErrorCode = STLAYER_GetLayerParams(
            ViewPort_p->Identity.Layer_p->LayerHandle,
            &LayerParams);
  if (ErrorCode != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetLayerParams failed. Error = 0x%x !", ErrorCode));
      return(ST_ERROR_BAD_PARAMETER);
  }

    /* Take into account input window cropping */
    CroppedSourceWidth = SourceWidth - ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset -
            ViewPort_p->Params.StreamParameters.FrameCropInPixel.RightOffset;
    CroppedSourceHeight = SourceHeight - ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset -
            ViewPort_p->Params.StreamParameters.FrameCropInPixel.BottomOffset;

    InputRectangle_p->PositionX = PositionX + 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset;
    InputRectangle_p->PositionY = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset;
    InputRectangle_p->Width = CroppedSourceWidth;
    InputRectangle_p->Height = CroppedSourceHeight;

    AspectRatioConversionIsRequired = (
    (
    /* 16/9 stream on 4/3 TV.                                                */
        (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) &&
        (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9)
    ) ||
    (
    /* 4/3 stream on 16/9 TV.                                                */
        (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) &&
        (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3)
    )
    );

    if ((AspectRatioConversionIsRequired && ViewPort_p->Params.StreamParameters.HasDisplaySizeRecommendation &&
        (DisplayHorizontalSize != SourceWidth)) &&
        ((ViewPort_p->Params.DispARConversion == STVID_DISPLAY_AR_CONVERSION_PAN_SCAN) ||
        (ViewPort_p->Params.DispARConversion == STVID_DISPLAY_AR_CONVERSION_COMBINED)) )
    {
        /* There are some display information attached, use them instead of aspect ratio calculation */
        InputRectangle_p->PositionX = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset +
                (16 * (S32)(CroppedSourceWidth - DisplayHorizontalSize)) / 2
                - ViewPort_p->Params.StreamParameters.PanVector;
        InputRectangle_p->PositionY = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset +
                (16 * (S32)(CroppedSourceHeight - ViewPort_p->Params.StreamParameters.DisplayVerticalSize)) / 2
                - ViewPort_p->Params.StreamParameters.ScanVector;
        InputRectangle_p->Width = DisplayHorizontalSize;
        InputRectangle_p->Height = ViewPort_p->Params.StreamParameters.DisplayVerticalSize;
    }
    else
    {
        /* Pan and Scan mode requested */
        if  (AspectRatioConversionIsRequired && (ViewPort_p->Params.DispARConversion == STVID_DISPLAY_AR_CONVERSION_PAN_SCAN))
        {
            /* 4/3 stream on 16/9 TV => Loose Up/Down horizontal bars.       */
            if ((LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) && (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3))
            {
                InputRectangle_p->Width = CroppedSourceWidth;
                InputRectangle_p->Height = (3 * CroppedSourceHeight) / 4;
                InputRectangle_p->PositionX = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset;
                InputRectangle_p->PositionY = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset +
                        (16 * (CroppedSourceHeight - InputRectangle_p->Height)) / 2;
            }
            else
            {
                /* 16/9 stream on 4/3 TV => Loose left/right vertical bars.  */
                InputRectangle_p->Width = (3 * CroppedSourceWidth) / 4;
                InputRectangle_p->Height = CroppedSourceHeight;
                InputRectangle_p->PositionX = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset +
                         (16 * (CroppedSourceWidth - InputRectangle_p->Width)) / 2;
                InputRectangle_p->PositionY = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset;
            }
        }
        /* combined mode requested */
        else if (AspectRatioConversionIsRequired && (ViewPort_p->Params.DispARConversion == STVID_DISPLAY_AR_CONVERSION_COMBINED))
        {
            /* 4/3 stream on 16/9 TV => Loose Up/Down horizontal bars.       */
            if ((LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9)
            && (SourceAspectRatio == STGXOBJ_ASPECT_RATIO_4TO3))
            {
                InputRectangle_p->Width = CroppedSourceWidth;
                InputRectangle_p->Height = (14 * CroppedSourceHeight) / 16;
                InputRectangle_p->PositionX = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset;
                InputRectangle_p->PositionY = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset +
                        (16 * (CroppedSourceHeight - InputRectangle_p->Height)) / 2;
            }
            else
            {
                /* 16/9 stream on 4/3 TV => Loose left/right vertical bars.  */
                InputRectangle_p->Width = (14 * CroppedSourceWidth) / 16;
                InputRectangle_p->Height = CroppedSourceHeight;
                InputRectangle_p->PositionX = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset +
                        (16 * (CroppedSourceWidth - InputRectangle_p->Width)) / 2;
                InputRectangle_p->PositionY = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset;
            }
        }
    }

    return (ST_NO_ERROR);
} /* End of ComputeInputWindow() function */


/*******************************************************************************
Name        : CenterOutput
Description : Center the Output Window with the Windows Parameters. Can change
              the Output Positions OutputX and OutputY.
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CenterOutput(
        VideoViewPort_t * const ViewPort_p,
        const STGXOBJ_Rectangle_t * const OutputRectangle_p,
        STGXOBJ_Rectangle_t * const AdjustedOutputRectangle_p)
{
  if (OutputRectangle_p->Width > (*AdjustedOutputRectangle_p).Width)
  {
    switch(ViewPort_p->Params.OutputWinParams.Align)
    {
      case STVID_WIN_ALIGN_TOP_LEFT :
      case STVID_WIN_ALIGN_VCENTRE_LEFT :
      case STVID_WIN_ALIGN_BOTTOM_LEFT :
        break; /* Nothing to do, requested X will be used */

      case STVID_WIN_ALIGN_TOP_RIGHT :
      case STVID_WIN_ALIGN_VCENTRE_RIGHT :
      case STVID_WIN_ALIGN_BOTTOM_RIGHT :
        (*AdjustedOutputRectangle_p).PositionX +=
                (OutputRectangle_p->Width - (*AdjustedOutputRectangle_p).Width );
        break;

      case STVID_WIN_ALIGN_TOP_HCENTRE :
      case STVID_WIN_ALIGN_VCENTRE_HCENTRE :
      case STVID_WIN_ALIGN_BOTTOM_HCENTRE :
      default :
        (*AdjustedOutputRectangle_p).PositionX +=
                (OutputRectangle_p->Width - (*AdjustedOutputRectangle_p).Width ) / 2;
        break;
    }
  }
  if (OutputRectangle_p->Height > (*AdjustedOutputRectangle_p).Height)
  {
    switch(ViewPort_p->Params.OutputWinParams.Align)
    {
      case STVID_WIN_ALIGN_TOP_LEFT :
      case STVID_WIN_ALIGN_TOP_HCENTRE :
      case STVID_WIN_ALIGN_TOP_RIGHT :
        break; /* Nothing to do, requested X will be used */

      case STVID_WIN_ALIGN_BOTTOM_LEFT :
      case STVID_WIN_ALIGN_BOTTOM_HCENTRE :
      case STVID_WIN_ALIGN_BOTTOM_RIGHT :
        (*AdjustedOutputRectangle_p).PositionY +=
                (OutputRectangle_p->Height - (*AdjustedOutputRectangle_p).Height );
        break;

      case STVID_WIN_ALIGN_VCENTRE_LEFT :
      case STVID_WIN_ALIGN_VCENTRE_HCENTRE :
      case STVID_WIN_ALIGN_VCENTRE_RIGHT :
      default :
        (*AdjustedOutputRectangle_p).PositionY +=
                (OutputRectangle_p->Height - (*AdjustedOutputRectangle_p).Height) / 2;
        break;
    }
  }

  return(ST_NO_ERROR);
} /* End of CenterOutput() function */


#ifdef ST_display
/*******************************************************************************
Name        : ChangeViewportInputParams
Description :
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ChangeViewportInputParams(VideoViewPort_t * const ViewPort_p)
{
  STLAYER_ViewPortParams_t  AdjustedLayerViewport;
  STGXOBJ_Rectangle_t       InputRectangle;
  ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

  /* Retrieve the previous viewport parameters sent to layer.                */
  AdjustedLayerViewport = ViewPort_p->Params.ViewportParametersAskedToLayerViewport;

  /* Apply th Pan & Scan parameters.                                         */
  ErrorCode = ComputeInputWindow(ViewPort_p, &InputRectangle);

  if (ErrorCode != ST_NO_ERROR)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ComputeInputWindow failed. Error = 0x%x !", ErrorCode));
    return( STVID_ERROR_NOT_AVAILABLE);
  }

  AdjustedLayerViewport.InputRectangle = InputRectangle;
  ErrorCode = STLAYER_SetViewPortParams(
        ViewPort_p->Identity.LayerViewPortHandle,
        &AdjustedLayerViewport);
  if (ErrorCode != ST_NO_ERROR)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetViewPortParams failed. Error = 0x%x !", ErrorCode));
    return( STVID_ERROR_NOT_AVAILABLE);
  }

  /* Save the last I windows set to the video layer viewport.              */
  ViewPort_p->Params.ViewportParametersAskedToLayerViewport.InputRectangle = InputRectangle;
  return(ErrorCode);
} /* End of ChangeViewportInputParams() function */
#endif /* ST_display */


/*******************************************************************************
Name        : ChangeViewportParams
Description : do :
              1. compute the input and output.
              2. calculate the scaling and adjust the input and output windows.
              3. center the output according to the WinParams.
              4. set the scaling, re-calculate input and output and set them.
              5. calculate and set the pan&scan vectors.
Parameters  : ViewPort parameters.
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ChangeViewportParams(VideoViewPort_t * const ViewPort_p)
{
    STLAYER_ViewPortParams_t  AdjustedLayerViewport;
    STLAYER_LayerParams_t     LayerParams;
    STGXOBJ_Rectangle_t       InputRectangle;
    STGXOBJ_Rectangle_t       OutputRectangle;
    S32                       InputWinX;
    S32                       InputWinY;
    U32                       InputWinWidth;
    U32                       InputWinHeight;
    S32                       OutputWinX;
    S32                       OutputWinY;
    U32                       OutputWinWidth;
    U32                       OutputWinHeight;
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    STLAYER_DecimationFactor_t PreviousLayerVerticalDecimationFactor;
    STLAYER_DecimationFactor_t PreviousLayerHorizontalDecimationFactor;
    STLAYER_DecimationFactor_t CurrentLayerHorizontalDecimationFactor;
    STLAYER_DecimationFactor_t CurrentLayerVerticalDecimationFactor;
    VideoDevice_t *         Device_p;

    /* retrieve Device structure */
    Device_p = ((VideoUnit_t *)(ViewPort_p->Identity.VideoHandle))->Device_p;

    do
    {
        /* Restore the parameters. They can come from the SetIOWindows functions. */
        InputWinX      = ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionX;
        InputWinY      = ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionY;
        InputWinWidth  = ViewPort_p->Params.VideoViewportParameters.InputRectangle.Width;
        InputWinHeight = ViewPort_p->Params.VideoViewportParameters.InputRectangle.Height;
        OutputWinX     = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionX;
        OutputWinY     = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionY;
        OutputWinWidth = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Width;
        OutputWinHeight= ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Height;

        /* The application asks for a full automatic scaling : the stream is      */
        /* automatically adjusted in the ViewPort.                                  */
        if (ViewPort_p->Params.InputWinAutoMode && ViewPort_p->Params.OutputWinAutoMode)
        {
            /* Adjust Input and Output from the Stream and the layer parameters.  */
            ErrorCode = ComputeOutputWindow(ViewPort_p, &OutputRectangle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ComputeOutputWindow failed. Error = 0x%x !", ErrorCode));
                return( STVID_ERROR_NOT_AVAILABLE);
            }

            ErrorCode = ComputeInputWindow(ViewPort_p, &InputRectangle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ComputeInputWindow failed. Error = 0x%x !", ErrorCode));
                return( STVID_ERROR_NOT_AVAILABLE);
            }
        }
        else
        {
            if (ViewPort_p->Params.OutputWinAutoMode)
            {
                /* The application asks for an automatic scaling only for the     */
                /* output (a manual scaling for the input) : the Output parameters*/
                /* given to the function are not taken into account. We use the   */
                /* Layer  dimensions instead.                                     */
                /* We use the Input parameters given to the function.             */
                /* Retrieve the Layer Params to get the layer dimensions.         */
                ErrorCode = STLAYER_GetLayerParams(ViewPort_p->Identity.Layer_p->LayerHandle, &LayerParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetLayerParams failed. Error = 0x%x !", ErrorCode));
                    return( STVID_ERROR_NOT_AVAILABLE);
                }
                OutputRectangle.PositionX = 0;
                OutputRectangle.PositionY = 0;
                OutputRectangle.Width  = LayerParams.Width;
                OutputRectangle.Height = LayerParams.Height;

                /* Usage of the parameters of the functions.                      */
                InputRectangle.PositionX = InputWinX;
                InputRectangle.PositionY = InputWinY;
                InputRectangle.Width  = InputWinWidth;
                InputRectangle.Height = InputWinHeight;
                /* Limit the input window within the cropped window source if applicable - only needed with H264 codec */
                if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_H264) ||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_H264) ||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264) ||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_H264))
                {
                    vidcomp_CheckAndAdjustManualInputParams(&InputRectangle, ViewPort_p);
                }
            }
            else
            {
                /* The application asks for a manual scaling for the output :    */
                /* we use the Output parameters given to the function.            */
                OutputRectangle.PositionX = OutputWinX;
                OutputRectangle.PositionY = OutputWinY;
                OutputRectangle.Width  = OutputWinWidth;
                OutputRectangle.Height = OutputWinHeight;

                /* The Input parameters are automatic (coming from the stream).   */
                if (ViewPort_p->Params.InputWinAutoMode)
                {
                    ErrorCode = ComputeInputWindow(ViewPort_p, &InputRectangle);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ComputeInputWindow failed. Error = 0x%x !", ErrorCode));
                        return( STVID_ERROR_NOT_AVAILABLE);
                    }
                }
                else
                {
                    /* The application asks for a manual scaling for the input : */
                    /* we use the input parameters given to the function.        */
                    InputRectangle.PositionX = InputWinX;
                    InputRectangle.PositionY = InputWinY;
                    InputRectangle.Width  = InputWinWidth;
                    InputRectangle.Height = InputWinHeight;
                    /* Limit the input window within the cropped window source if applicable - only needed with H264 codec */
                    if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_H264) ||
                        (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_H264) ||
                        (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264) ||
                        (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_H264))
                    {
                        vidcomp_CheckAndAdjustManualInputParams(&InputRectangle, ViewPort_p);
                    }
                }
            }
        }

        if(ViewPort_p->Params.QualityOptimizations.DoNotRescaleForZoomCloseToUnity == TRUE)
        {
            /* Zooming in Input */
            if( (InputRectangle.Width != 0) && (OutputRectangle.Width > InputRectangle.Width) &&
                (OutputRectangle.Width*100/InputRectangle.Width) < ZOOM_RATIO_CLOSE_TO_UNITY)
            {
                /* Input width is very close to output width (ex 704 vs 720)=> it's better not to resize */
                /* Output width is set to input width and centered */
                OutputRectangle.PositionX += (OutputRectangle.Width - InputRectangle.Width)/2;
                OutputRectangle.Width = InputRectangle.Width;
            }
            if( (InputRectangle.Height != 0) && (OutputRectangle.Height > InputRectangle.Height) &&
                (OutputRectangle.Height*100/InputRectangle.Height) < ZOOM_RATIO_CLOSE_TO_UNITY)
            {
                /* Input Height is very close to output Height => it's better not to resize */
                /* Output Height is set to input Height and centered */
                OutputRectangle.PositionY += (OutputRectangle.Height - InputRectangle.Height)/2;
                OutputRectangle.Height = InputRectangle.Height;
            }

            /* Zooming out Input */
            if( (OutputRectangle.Height != 0) && (InputRectangle.Height > OutputRectangle.Height) &&
                (InputRectangle.Height*100/OutputRectangle.Height) < ZOOM_RATIO_CLOSE_TO_UNITY)
            {
                /* Output Height is very close to input Height (ex 1088 vs 1080)=> it's better not to resize */
                /* Input Height is set to output Height and centered */
                InputRectangle.PositionY += 16*(InputRectangle.Height - OutputRectangle.Height)/2;
                InputRectangle.Height = OutputRectangle.Height;
            }
            if( (OutputRectangle.Width != 0) && (InputRectangle.Width > OutputRectangle.Width) &&
                (InputRectangle.Width*100/OutputRectangle.Width) < ZOOM_RATIO_CLOSE_TO_UNITY)
            {
                /* Output width is very close to input width => it's better not to resize */
                /* Input width is set to output width and centered */
                InputRectangle.PositionX += 16*(InputRectangle.Width - OutputRectangle.Width)/2;
                InputRectangle.Width = OutputRectangle.Width;
            }
        }

        /* Now we have the parameters we intend to use. Adjust them with the      */
        /* hardware capabilities : call to STLAYER_AdjustIORectangle.             */
        AdjustedLayerViewport.InputRectangle                    = InputRectangle;
        AdjustedLayerViewport.OutputRectangle                   = OutputRectangle;

        AdjustedLayerViewport.Source_p = ViewPort_p->Params.VideoViewportParameters.Source_p;
        AdjustedLayerViewport.Source_p->Palette_p = NULL;
        /* Here, get the decimation used. It's called "previous" because the       */
        /* function STLAYER_SetViewPortParams() may change it.                     */
        PreviousLayerHorizontalDecimationFactor =
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->HorizontalDecimationFactor;
        PreviousLayerVerticalDecimationFactor =
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->VerticalDecimationFactor;

        /* Save the last I/O windows set to the video layer viewport               */
        ViewPort_p->Params.ViewportParametersAskedToLayerViewport = AdjustedLayerViewport;

        ErrorCode = STLAYER_AdjustViewPortParams(ViewPort_p->Identity.Layer_p->LayerHandle, &AdjustedLayerViewport);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_AdjustViewPortParams failed. Error = 0x%x !", ErrorCode));
            return( STVID_ERROR_NOT_AVAILABLE);
        }

        /* Recenter with the Output Windows Parameters asked if necessary (Only in  */
        /* Automatic output window mode).                                           */
        if (ViewPort_p->Params.OutputWinAutoMode)
        {
            CenterOutput(ViewPort_p,&OutputRectangle, &AdjustedLayerViewport.OutputRectangle);
        }

        if(ViewPort_p->Params.QualityOptimizations.DoForceStartOnEvenLine == TRUE)
        {
            BOOL IsInputOnEvenLine = (AdjustedLayerViewport.InputRectangle.PositionY/16) %2;
            BOOL IsOutputOnEvenLine = AdjustedLayerViewport.OutputRectangle.PositionY %2 ;

            /* Only if Input or Output (exclusive) is on even line we must correct  */
            if((IsInputOnEvenLine || IsOutputOnEvenLine) && !(IsInputOnEvenLine && IsOutputOnEvenLine))
            {
                if(IsInputOnEvenLine)
                {
                    AdjustedLayerViewport.InputRectangle.PositionY &= ~0x1F;  /* Input is in 1/16th of pixel so mask last 4 bits */
                }
                else
                {
                     AdjustedLayerViewport.OutputRectangle.PositionY &= ~0x01;
                }
            }
        }

        /* *********************************  DEBUG  traces.  ********************************* */
        COMP_TraceBuffer(("\r\n    ChangeViewportParams (H%d/V%d)(%d/%d to %d/%d)",
                PreviousLayerHorizontalDecimationFactor,
                PreviousLayerVerticalDecimationFactor,
                AdjustedLayerViewport.InputRectangle.Width,
                AdjustedLayerViewport.InputRectangle.Height,
                AdjustedLayerViewport.OutputRectangle.Width,
                AdjustedLayerViewport.OutputRectangle.Height ));
        /* ******************************  DEBUG  traces (end).  ****************************** */
        /* Apply these parameters : call to STLAYER_SetViewPortParams.       */
        if(ViewPort_p->Identity.Layer_p->LayerType != STLAYER_COMPOSITOR)
        {
            ErrorCode = STLAYER_SetViewPortParams(ViewPort_p->Identity.LayerViewPortHandle, &AdjustedLayerViewport);
        }
        else
        {
            /* I don't understand why the other chips set all the params (= io rect + source) !!! */
            /* We need only to update the rectangles.!! For my chip I update the rectangles only    */
            ErrorCode = STLAYER_SetViewPortIORectangle(ViewPort_p->Identity.LayerViewPortHandle,
                            &AdjustedLayerViewport.InputRectangle,&AdjustedLayerViewport.OutputRectangle);
        }
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetViewPortParams failed. Error = 0x%x !", ErrorCode));
            return( STVID_ERROR_NOT_AVAILABLE);
        }

        /* Get the current decimation applied for this viewport.             */
        CurrentLayerHorizontalDecimationFactor =
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->HorizontalDecimationFactor;
        CurrentLayerVerticalDecimationFactor =
                ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->VerticalDecimationFactor;

    } while ( (CurrentLayerHorizontalDecimationFactor != PreviousLayerHorizontalDecimationFactor) ||
              (CurrentLayerVerticalDecimationFactor   != PreviousLayerVerticalDecimationFactor  ) );

    return(ErrorCode);
} /* End of ChangeViewportParams() function */


/*******************************************************************************
Name        : ConvertDecimationValues
Description : Convert decimation types from video driver to layer driver.
Parameters  :  IN : Video decimation VideoDecimationFactor
               OUT: LayerHorizontalDecimationFactor and LayerHorizontalDecimationFactor
Assumptions : VideoDecimationFactor is correctly set.
              Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     : EVT API
*******************************************************************************/
static void ConvertDecimationValues (STVID_DecimationFactor_t  VideoDecimationFactor,
                                    STLAYER_DecimationFactor_t *LayerHorizontalDecimationFactor,
                                    STLAYER_DecimationFactor_t *LayerVerticalDecimationFactor)
{
    switch (VideoDecimationFactor & ~(STVID_DECIMATION_FACTOR_H2 | STVID_DECIMATION_FACTOR_H4))
    {
        case STVID_DECIMATION_FACTOR_V2 :
            *LayerVerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_2;
            break;

        case STVID_DECIMATION_FACTOR_V4 :
            *LayerVerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_4;
            break;

        default :
            *LayerVerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
            break;
    }

    switch (VideoDecimationFactor & ~(STVID_DECIMATION_FACTOR_V2 | STVID_DECIMATION_FACTOR_V4))
    {
        case STVID_DECIMATION_FACTOR_H2 :
            *LayerHorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_2;
            break;

        case STVID_DECIMATION_FACTOR_H4 :
            *LayerHorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_4;
            break;

        default :
            *LayerHorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
            break;
    }

} /* End of ConvertDecimationValues function. */


/*******************************************************************************
Name        : CloseLayerForOldViewport
Description : Given a layer name, eventually close the associated layer handle.
Parameters  : device, layer name, parameter to say the state to restore in case of failure: closed or opened
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
static ST_ErrorCode_t CloseLayerForOldViewport(VideoDevice_t * const Device_p, ST_DeviceName_t LayerName, const BOOL FinishOpenedNotClosedInCaseOfProblem)
{
    U32                             Id;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    BOOL                            MainNotAux = TRUE;
    STLAYER_VTGParams_t             VTGParams;
    U32                             k;
#ifdef ST_display
    VIDDISP_LayerConnectionParams_t LayerConnectionParams;
#endif /* ST_display */

    /* Look for the name */
    Id = IdentOfNamedLayer(Device_p->DeviceData_p,LayerName);
    if (Id != BAD_LAYER_INDEX)
    {
        /* Found layer name: close it if no more viewports ! */
        Device_p->DeviceData_p->LayerProperties_p[Id].NumberVideoViewportsAssociated --;

        if (Device_p->DeviceData_p->LayerProperties_p[Id].NumberVideoViewportsAssociated == 0)
        {
            /* No more viewports, close layer */
            ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, LayerName, STLAYER_UPDATE_PARAMS_EVT);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_UnSubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                if (FinishOpenedNotClosedInCaseOfProblem)
                {
                    /* Must go to opened state: stop closing */
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    /* Must go to close state: close as much as possible without error */
                    ErrorCode = ST_NO_ERROR;
                }
            } /* end error unsubscribe */

            ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, LayerName, STLAYER_UPDATE_DECIMATION_EVT);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_UnSubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                if (FinishOpenedNotClosedInCaseOfProblem)
                {
                    /* Must go to opened state: stop closing */
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    /* Must go to close state: close as much as possible without error */
                    ErrorCode = ST_NO_ERROR;
                }
            } /* end error unsubscribe */

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Try to close handle. If failing: go on anyway */
                ErrorCode = STLAYER_Close(Device_p->DeviceData_p->LayerProperties_p[Id].LayerHandle);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_Close() failed. Error = 0x%x !", ErrorCode));
                    if (FinishOpenedNotClosedInCaseOfProblem)
                    {
                        /* Must go to opened state: re-do things closed */
                        STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) UpdateParamsFromLayer;
                        STEVT_SubscribeParams.SubscriberData_p = (void *) Device_p;
                        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, LayerName, STLAYER_UPDATE_PARAMS_EVT, &STEVT_SubscribeParams);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* undo actions done. */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                        }

                        STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) UpdateDecimationFromLayer;
                        STEVT_SubscribeParams.SubscriberData_p = (void *) Device_p;
                        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, LayerName, STLAYER_UPDATE_DECIMATION_EVT, &STEVT_SubscribeParams);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* undo actions done. */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                        }

                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        /* Must go to close state: close as much as possible without error */
                        ErrorCode = ST_NO_ERROR;
                    }
                } /* end error layer close */

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Disconnect display From VTG */
                    VTGParams.VTGName[0] = '\0';

                    /* In the case if display on auxiliary and secondary (specific 7200) */

                    if (( Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
                          Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
                          (Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
                          Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ))
                    {
                        if (Device_p->DeviceData_p->LayerProperties_p[Id].LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                            MainNotAux = FALSE;
                        else if (Device_p->DeviceData_p->LayerProperties_p[Id].LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                            MainNotAux = TRUE;
                    } else
                    {

                        switch(Device_p->DeviceData_p->LayerProperties_p[Id].LayerType)
                        {
                            case STLAYER_OMEGA2_VIDEO1:
                            case STLAYER_7020_VIDEO1:
                            case STLAYER_SDDISPO2_VIDEO1 :
                            case STLAYER_HDDISPO2_VIDEO1 :
                            case STLAYER_DISPLAYPIPE_VIDEO1 :
                            case STLAYER_DISPLAYPIPE_VIDEO2 :
                            case STLAYER_GAMMA_GDP :
                                MainNotAux = TRUE;
                            break;

                            case STLAYER_OMEGA2_VIDEO2:
                            case STLAYER_7020_VIDEO2:
                            case STLAYER_SDDISPO2_VIDEO2 :
                            case STLAYER_HDDISPO2_VIDEO2 :
                            case STLAYER_DISPLAYPIPE_VIDEO3 :
                            case STLAYER_DISPLAYPIPE_VIDEO4 :
                                MainNotAux = FALSE;
                            break;

                            default: /* cases 55xx */
                                MainNotAux = TRUE;
                            break;
                        }
                    }
#ifdef ST_display
                    VIDDISP_NewVTG(Device_p->DeviceData_p->DisplayHandle,
                          MainNotAux ,
                          &VTGParams);

                    /* Inform the Display that a layer is disconnected. */
                    LayerConnectionParams.LayerType = Device_p->DeviceData_p->LayerProperties_p[Id].LayerType;
                    /* Set default values for framebuffer display latency and frame buffer hold time */
                    LayerConnectionParams.DisplayLatency      = 1;
                    LayerConnectionParams.FrameBufferHoldTime = 1;
                    LayerConnectionParams.LayerHandle = Device_p->DeviceData_p->LayerProperties_p[Id].LayerHandle;
                    VIDDISP_LayerConnection(Device_p->DeviceData_p->DisplayHandle,
                                            &LayerConnectionParams,
                                            FALSE);
#endif /* ST_display */

                    /* Check now if the mode for passing picture to display
                     * needs to be changed */
                    Device_p->DeviceData_p->LayerProperties_p[Id].DisplayLatency = 1;
                    Device_p->DeviceData_p->UpdateDisplayQueueMode = VIDPROD_UPDATE_DISPLAY_QUEUE_DEFAULT_MODE;
                    for(k = 0; k < Device_p->DeviceData_p->MaxLayerNumber; k++)
                    {
                        if((Device_p->DeviceData_p->LayerProperties_p[k].IsOpened)
                           &&(Device_p->DeviceData_p->LayerProperties_p[k].LayerType == STLAYER_COMPOSITOR)
                           &&(Device_p->DeviceData_p->LayerProperties_p[k].DisplayLatency == 2))
                        {
                            /* Change is needed only if one connected of the
                               still contected layers has a latency of 2 */
                            Device_p->DeviceData_p->UpdateDisplayQueueMode = VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_START;
                            break;
                        }
                    }

                    /* Clear  */
                    Device_p->DeviceData_p->LayerProperties_p[Id].
                                LayerName[0] = '\0';
                    Device_p->DeviceData_p->LayerProperties_p[Id].
                                NumberVideoViewportsAssociated = 0;
                    Device_p->DeviceData_p->LayerProperties_p[Id].
                                IsOpened = FALSE;
                } /* end ready to initialise variables */
            } /* end ready to close layer */
        } /* end no more viewports in layer */
        return(ErrorCode);
    }

    /* Name not found ! */
    return(ST_ERROR_BAD_PARAMETER);

} /* End of CloseLayerForOldViewport() function */

/*******************************************************************************
Name        : OpenLayerForNewViewport
Description : Given a new layer name, returns a pointer to the layer structure associated.
Parameters  :
Assumptions : Function calls are protected with LayersViewportsLockingSemaphore_p
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_NO_MEMORY if no free memory
              ST_ERROR_BAD_PARAMETER if parameter problem
*******************************************************************************/
static ST_ErrorCode_t OpenLayerForNewViewport(VideoDevice_t * const Device_p, ST_DeviceName_t LayerName, VideoLayer_t ** const VideoLayer_p)
{
    U32                     Id;
    STLAYER_OpenParams_t    LayerOpenParams;
    STLAYER_Capability_t    LayerCapability;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    BOOL                    Found;
#ifdef ST_display
    VIDDISP_LayerConnectionParams_t LayerConnectionParams;
#endif /* ST_display */

    /* Look if the name already exists */
    Id = IdentOfNamedLayer(Device_p->DeviceData_p,LayerName);
    if (Id != BAD_LAYER_INDEX)
    {
        /* Found same layer name: layer already in use, just take it if max viewport number not reached ! */
        if ( Device_p->DeviceData_p->LayerProperties_p[Id].NumberVideoViewportsAssociated
                < Device_p->DeviceData_p->MaxViewportNumberPerLayer)
        {
            *VideoLayer_p = &Device_p->DeviceData_p->LayerProperties_p[Id];
             Device_p->DeviceData_p->LayerProperties_p[Id].NumberVideoViewportsAssociated ++;
            return(ST_NO_ERROR);
        }
        else
        {
            return(ST_ERROR_NO_MEMORY);
        }
    }

    /* the layer name is not found : it's a new layer */
    Id=0;Found = FALSE;
    do
    {
        if (!(Device_p->DeviceData_p->LayerProperties_p[Id].IsOpened))
        {
            Found = TRUE;
        }
        else
        {
            Id ++;
        }
    } while((Id <= Device_p->DeviceData_p->MaxLayerNumber)
        &&(!Found));

    if (Id >= Device_p->DeviceData_p->MaxLayerNumber)
    {
        /* No more layers available for this instance ! */
        return(ST_ERROR_NO_MEMORY);
    }

    ErrorCode = STLAYER_GetCapability(LayerName, &LayerCapability);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetCapability() failed. Error = 0x%x !", ErrorCode));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Could not find layer: this is a new layer name, take it ! */
    ErrorCode = STLAYER_Open(LayerName, &LayerOpenParams, &(Device_p->DeviceData_p->LayerProperties_p[Id].LayerHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_Open failed. Error = 0x%x !", ErrorCode));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        /* Layer open successfull */
        /* Subscribe to STLAYER_UPDATE_PARAMS_EVT of this layer instance. */
        STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) UpdateParamsFromLayer;
        STEVT_SubscribeParams.SubscriberData_p = (void *) Device_p;
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, LayerName, STLAYER_UPDATE_PARAMS_EVT, &STEVT_SubscribeParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* undo actions done. */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STLAYER_Close(Device_p->DeviceData_p->LayerProperties_p[Id].LayerHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"STLAYER_Close() failed. Error = 0x%x !", ErrorCode));
            }
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            /* Subscribe to STLAYER_UPDATE_DECIMATION_EVT of this layer instance. */
            STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)UpdateDecimationFromLayer ;
            STEVT_SubscribeParams.SubscriberData_p = (void *) Device_p;
            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, LayerName, STLAYER_UPDATE_DECIMATION_EVT, &STEVT_SubscribeParams);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* undo actions done. */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                ErrorCode = STLAYER_Close(Device_p->DeviceData_p->LayerProperties_p[Id].LayerHandle);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"STLAYER_Close() failed. Error = 0x%x !", ErrorCode));
                }
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Layer open and subscribe successfull */
            Device_p->DeviceData_p->LayerProperties_p[Id].IsOpened = TRUE;
            strcpy(Device_p->DeviceData_p->LayerProperties_p[Id].LayerName, LayerName);
            Device_p->DeviceData_p->LayerProperties_p[Id].NumberVideoViewportsAssociated = 1;
            Device_p->DeviceData_p->LayerProperties_p[Id].LayerType = LayerCapability.LayerType;
            Device_p->DeviceData_p->LayerProperties_p[Id].DisplayLatency = LayerCapability.FrameBufferDisplayLatency;

            *VideoLayer_p = &Device_p->DeviceData_p->LayerProperties_p[Id];

#ifdef ST_display
            /* Inform the Display that a layer is connected. */
            LayerConnectionParams.LayerType           = Device_p->DeviceData_p->LayerProperties_p[Id].LayerType;
            LayerConnectionParams.DisplayLatency      = LayerCapability.FrameBufferDisplayLatency;
            LayerConnectionParams.FrameBufferHoldTime = LayerCapability.FrameBufferHoldTime;
            /* According to the composition mode used on the blitter based
               platforms, it might be important to inform the display about a
               new picture decoded as soon as possible, to allow compositor
               anticipate the nodes generation */
            if((LayerCapability.FrameBufferDisplayLatency == 2)
               &&(LayerCapability.LayerType == STLAYER_COMPOSITOR))
            {
                Device_p->DeviceData_p->UpdateDisplayQueueMode = VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_START;
            }

            LayerConnectionParams.LayerHandle         = Device_p->DeviceData_p->LayerProperties_p[Id].LayerHandle;

            VIDDISP_LayerConnection(Device_p->DeviceData_p->DisplayHandle,
                &LayerConnectionParams,
                   TRUE);
#endif /* ST_display */
        }
    }

    return(ErrorCode);
} /* End of OpenLayerForNewViewport() function */



/* Functions for events callback -------------------------------------------- */


#ifdef ST_display
/*******************************************************************************
Name        : stvid_PictureCharacteristicsChange
Description : Function to subscribe to the video display event :
              VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_PictureCharacteristicsChange(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VIDDISP_PictureCharacteristicsChangeParams_t *  PictureCharacteristicsParams_p;
    STVID_PictureInfos_t *                          PictureInfos_p;
            /* is NULL if there is no sequence_display_extension */
    VideoViewPort_t *                               ViewPort_p;
    STLAYER_StreamingVideo_t *                      VideoStream_p;
    U32                                             index;
    VIDCOM_InternalProfile_t                        MemoryProfile;
    VideoDevice_t *                                 Device_p = (VideoDevice_t *) SubscriberData_p;
    ST_ErrorCode_t                                  ErrorCode = ST_NO_ERROR;

#if defined (ST_diginput) && defined (ST_MasterDigInput)
    if(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)
    {
        stvid_DvpPictureCharacteristicsChange(Reason,RegistrantName, Event, EventData_p, SubscriberData_p);
        return;
    }
#else /* #ifdef ST_diginput && ST_MasterDigInput */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
#endif/* #ifdef ST_diginput && ST_MasterDigInput */

    if (!Device_p->DeviceData_p->AreViewportParametersInitialized)
    {
        /* Ignore the picture characteristics change event as long
         * as the viewport parameters are not initialized correctly */
        return;
    }

    /* Ensure nobody else accesses layers/viewports. */
    semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    /* *********************************  DEBUG  traces.  ********************************* */
        COMP_TraceBuffer(("\r\nstvid_PictureCharacteristicsChange :"));
    /* ******************************  DEBUG  traces (end).  ****************************** */


    /* Store locally the parameters given in the event.                         */
    PictureCharacteristicsParams_p = (VIDDISP_PictureCharacteristicsChangeParams_t *) EventData_p;
    PictureInfos_p = PictureCharacteristicsParams_p->PictureInfos_p;

    /* Retrieve the memory profile.     */
    VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &MemoryProfile);

    /* In all the cases, we need to find the opened Viewports.                  */
    ViewPort_p = Device_p->DeviceData_p->ViewportProperties_p;
    for (index = 0; index < Device_p->DeviceData_p->MaxViewportNumber; index++)
    {
        if((ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == 0)
       ||(PictureCharacteristicsParams_p->LayerType
                != ViewPort_p->Identity.Layer_p->LayerType))
        {
            /* this viewport is not concerned ... */
            ViewPort_p++;
            continue;
        }
        /* Apply the actions to the opened viewports.                          */
        /* Retrieve the informations given through the event about the stream. */
        ViewPort_p->Params.StreamParameters.PanVector =  PictureCharacteristicsParams_p->PanAndScanIn16thPixel.FrameCentreHorizontalOffset;
        ViewPort_p->Params.StreamParameters.ScanVector = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.FrameCentreVerticalOffset;
        ViewPort_p->Params.StreamParameters.HasDisplaySizeRecommendation = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.HasDisplaySizeRecommendation;
        ViewPort_p->Params.StreamParameters.DisplayHorizontalSize = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.DisplayHorizontalSize / 16;
        ViewPort_p->Params.StreamParameters.DisplayVerticalSize = PictureCharacteristicsParams_p->PanAndScanIn16thPixel.DisplayVerticalSize / 16;
        ViewPort_p->Params.StreamParameters.FrameCropInPixel = PictureCharacteristicsParams_p->FrameCropInPixel;

        /* Case of Graphics layer type */
        if (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP)
        {
            stvid_GraphicsPictureCharacteristicsChange( ViewPort_p, PictureCharacteristicsParams_p);
            ViewPort_p++;
            continue;
        }

        /* Reset the ShowingPicture flag if it's set !!!   */
        if (ViewPort_p->Params.ShowingPicture)
        {
            /* Restore the reconstruction mode and the decimation factor that was available before first STVID_ShowPicture. */
            ErrorCode = VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                                    ViewPort_p->Identity.Layer_p->LayerType,
                                    ViewPort_p->Params.PictureSave.ReconstructionMode);
            PictureInfos_p->VideoParams.DecimationFactors = ViewPort_p->Params.PictureSave.FrameBuffer.DecimationFactor;

            ViewPort_p->Params.StreamParameters         = ViewPort_p->Params.PictureSave.StreamParameters;
            ViewPort_p->Params.VideoViewportParameters  = ViewPort_p->Params.PictureSave.LayerViewport;
            *ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p
                    = ViewPort_p->Params.PictureSave.VideoStreaming;

            ViewPort_p->Params.ShowingPicture = FALSE;
        }
        /* TODO_OB : Shouldn't we use PictureInfos_p->VideoParams.DecimationFactors below ? */
#ifdef ST_deblock
        if (MemoryProfile.ApplicableDecimationFactor & STVID_MPEG_DEBLOCK_RECONSTRUCTION)
        {
            if (PictureInfos_p != NULL)
            {
                if ((PictureInfos_p->BitmapParams.Width <= 720)&&(PictureInfos_p->BitmapParams.Height <= 576)) /* SD */
                {
                    ErrorCode = VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                                            ViewPort_p->Identity.Layer_p->LayerType,
                                            VIDBUFF_SECONDARY_RECONSTRUCTION);
                    /* Not available anymore
                    ErrorCode = VIDPROD_SetDisplayedReconstruction(
                                            Device_p->DeviceData_p->ProducerHandle,
                                            VIDPROD_DISPLAYED_SECONDARY_ONLY);
                    */
               }
            }
        }
#endif /* ST_deblock */

        if (MemoryProfile.ApplicableDecimationFactor & STVID_POSTPROCESS_RECONSTRUCTION)
        {
                ErrorCode = VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                                        ViewPort_p->Identity.Layer_p->LayerType,
                                        VIDBUFF_SECONDARY_RECONSTRUCTION);
        }

        if (PictureInfos_p != NULL)
        {
            /* PictureInfos_p is NULL if only pan and scan ! (use old params) */
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT:
                    ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Pitch = PictureInfos_p->BitmapParams.Pitch;
                    ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Width = PictureInfos_p->BitmapParams.Width;
                    ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Height = PictureInfos_p->BitmapParams.Height;
                    ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->AspectRatio = PictureInfos_p->BitmapParams.AspectRatio;
                    break;

                default :
                    /* *********************************  DEBUG  traces.  ********************************* */
                        COMP_TraceBuffer(("\r\n(VP%d) - Source [%d/%d%s %s Dec(%d)]",
                                index,
                                PictureInfos_p->BitmapParams.Width,
                                PictureInfos_p->BitmapParams.Height,
                                (PictureInfos_p->VideoParams.ScanType     == STGXOBJ_INTERLACED_SCAN  ? "I"  :"P"   ),
                                (PictureInfos_p->BitmapParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3? "4:3":"16:9"),
                                PictureInfos_p->VideoParams.DecimationFactors ));
                    /* ******************************  DEBUG  traces (end).  ****************************** */

                    /* Retrieve all source characteristics, and save it in Viewport properties field.       */
                    ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams =
                            PictureInfos_p->BitmapParams;

                    ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->ScanType =
                            PictureInfos_p->VideoParams.ScanType;

                    if((ViewPort_p->Identity.Layer_p->LayerType == STLAYER_COMPOSITOR) ||
                       (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT))
                    {
                        if (ViewPort_p->Params.DisplayEnableDelayed)
                        {
                            ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionX = 0;
                            ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionY = 0;
                        }
                        ViewPort_p->Params.VideoViewportParameters.InputRectangle.Width
                                            = PictureInfos_p->BitmapParams.Width;
                        ViewPort_p->Params.VideoViewportParameters.InputRectangle.Height
                                            = PictureInfos_p->BitmapParams.Height;

                        STLAYER_SetViewPortParams(ViewPort_p->Identity.LayerViewPortHandle,
                            &(ViewPort_p->Params.VideoViewportParameters));
                    }

                    /* Retrieve viewport video bitstream parameters.                    */
                    VideoStream_p = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p;

                    /* Test if this ViewPort's decimation has to be changed.            */
                    if (
                         /* The viewport is already using decimation. Update its value. */
                         ((VideoStream_p->HorizontalDecimationFactor != STLAYER_DECIMATION_FACTOR_NONE) ||
                          (VideoStream_p->VerticalDecimationFactor   != STLAYER_DECIMATION_FACTOR_NONE))
                         ||
                         /* The source is only decimated (no main reconstruction allowed*/
                         /* so force the viewport to use the decimation.                */
                         ((PictureInfos_p->VideoParams.DecimationFactors != STVID_DECIMATION_FACTOR_NONE) &&
                          ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT)||
                           (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)||
                           (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT)||
                           (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT)||
                           (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT)||
                           (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT)))
                         ||
                         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
                         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
                    {
                        /* Yes, so update the current used decimation.              */
                        /* Set the actual decimation used for the current picture.   */
                        ConvertDecimationValues (PictureInfos_p->VideoParams.DecimationFactors,
                                &(VideoStream_p->HorizontalDecimationFactor),
                                &(VideoStream_p->VerticalDecimationFactor));
                        if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                            (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                            (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
                                     {
                            /* Force decimation mode for MPEG4-P2 */
                            VideoStream_p->HorizontalDecimationFactor = STLAYER_DECIMATION_MPEG4_P2;
                            VideoStream_p->VerticalDecimationFactor   = STLAYER_DECIMATION_MPEG4_P2;
                        }
                        else if (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) /* PLE_TO_DO : STLAYER_DECIMATION_MPEG4_P2 is not a well named flag. Needs to be reviewed and modified */
                        {
                            /* Force decimation mode for MPEG4-P2 */
                            /* Force decimation mode for MPEG4-P2 */
                            VideoStream_p->HorizontalDecimationFactor = STLAYER_DECIMATION_AVS;
                            VideoStream_p->VerticalDecimationFactor   = STLAYER_DECIMATION_AVS;
                        }



                         /* *********************************  DEBUG  traces.  ********************************* */
                            COMP_TraceBuffer((" -VP Decimation changed to :%d",
                                    PictureInfos_p->VideoParams.DecimationFactors));
                         /* ******************************  DEBUG  traces (end).  ****************************** */
                     }
#ifdef ST_crc
                    VideoStream_p->CRCCheckMode = PictureCharacteristicsParams_p->CRCCheckMode;
                    VideoStream_p->CRCScanType = PictureCharacteristicsParams_p->CRCScanType;
#endif /* ST_crc */
                break;
            }
        }

        switch (PictureCharacteristicsParams_p->CharacteristicsChanging)
        {
            case VIDDISP_CHANGING_ONLY_FRAME_CENTER_OFFSETS :
                /* We take into account the input parameters coming from the     */
                /* stream in automatic mode only.                                */
                if (ViewPort_p->Params.InputWinAutoMode)
                {
                    /* Process the parameters.                                   */
                    /* Each time we have Pan&Scan informations, it means we have */
                    /* Sequence Display Extension.                               */
                    ViewPort_p->Params.StreamParameters.HasDisplaySizeRecommendation = TRUE;

                    /* Apply changes to the view port...                         */
                    ErrorCode = ChangeViewportInputParams(ViewPort_p);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"ChangeViewportInputParams failed. Error = 0x%x !", ErrorCode));
                    }
                }
                break;

            case VIDDISP_CHANGING_EVERYTHING :
                /* Process the parameters.                                       */
                ErrorCode = ChangeViewportParams(ViewPort_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"ChangeViewportParams failed. Error = 0x%x !", ErrorCode));
                }
                break;
            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,  "No reason to change."));
                break;
        }

        /* Check enable view port delayed. */
        if (ViewPort_p->Params.DisplayEnableDelayed)
        {
            ViewPort_p->Params.DisplayEnableDelayed = FALSE;

            /* Enable view port now that a new picture is coming */
            ErrorCode = STLAYER_EnableViewPort(ViewPort_p->Identity.LayerViewPortHandle);
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
        ViewPort_p++;
    } /* End scan all opened viewports */

    /* Ensure nobody else accesses layers/viewports. */
    semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

} /* End of stvid_PictureCharacteristicsChange() function */
#endif /* ST_display */


/*******************************************************************************
Name        : UpdateDecimationFromLayer
Description : Function called when occurs the layer video event :
              STLAYER_UPDATE_DECIMATION_EVT.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void UpdateDecimationFromLayer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    STLAYER_UpdateParams_t *    UpdateParams_p;
    VideoDevice_t *             Device_p = (VideoDevice_t *) SubscriberData_p;
    VIDBUFF_AvailableReconstructionMode_t ReconstructionMode;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STLAYER_Layer_t             LayerType;
    STVID_MemoryProfile_t       APIMemoryProfile;
    VIDCOM_InternalProfile_t    InternalMemoryProfile;
    STLAYER_DecimationFactor_t  LayerHorizontalDecimationFactor;
    STLAYER_DecimationFactor_t  LayerVerticalDecimationFactor;
    U32                         k;
    U32                         LayerId = 0;
    STVID_DecimationFactor_t    DecimationFactor;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(Event);

    /* Store locally the parameters given in the event. */
    UpdateParams_p = (STLAYER_UpdateParams_t *) EventData_p;

    LayerId = IdentOfNamedLayer(Device_p->DeviceData_p,RegistrantName);
    if(LayerId == BAD_LAYER_INDEX)
    {
        /* it's an error case */
        return;
    }

    LayerType = Device_p->DeviceData_p->LayerProperties_p[LayerId].LayerType;

    /* Find the opened viewport on this layer.  */
    for (k = 0; k < Device_p->DeviceData_p->MaxViewportNumber; k ++)
    {
        if((Device_p->DeviceData_p->ViewportProperties_p[k].Identity.ViewPortOpenedAndValidityCheck
                    == VIDAPI_VALID_VIEWPORT_HANDLE)
       && (Device_p->DeviceData_p->ViewportProperties_p[k].Identity.Layer_p->LayerType
                    == LayerType))
        {
            /* Viewport is in this layer: Check if showing picture. */
            if (!Device_p->DeviceData_p->ViewportProperties_p[k].Params.ShowingPicture)
            {

                /* If we come here, it means that we are calling by STLAYER. And STLAYER has been called by */
                /* vid_comp.c ChangeViewportParams, so we are already protected.                            */

                /* *********************************  DEBUG  traces.  ********************************* */
                    COMP_TraceBuffer(("\r\nSTLAYER_DECIMATION_NEED_REASON event : Decimation %s Needed for %s",
                        (UpdateParams_p->IsDecimationNeeded ?"is":"no more"),
                        ((LayerType==STLAYER_7020_VIDEO1)||(LayerType==STLAYER_OMEGA2_VIDEO1))?"MAIN DISPLAY":"AUX DISPLAY"));
                /* ******************************  DEBUG  traces (end).  ****************************** */

                /* Retrieve the memory profile.     */
                VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &InternalMemoryProfile);
                APIMemoryProfile=InternalMemoryProfile.ApiProfile;
                /* TODO_OB : Shouldn't we use PictureInfos_p->VideoParams.DecimationFactors below ? */
#ifdef ST_deblock
                if (!(InternalMemoryProfile.ApplicableDecimationFactor & STVID_MPEG_DEBLOCK_RECONSTRUCTION))
#endif /* ST_deblock */
                {
                    /* Retrieve the action to performed. */
                    if(InternalMemoryProfile.ApplicableDecimationFactor == STVID_POSTPROCESS_RECONSTRUCTION)
                    {
                        ReconstructionMode = VIDBUFF_SECONDARY_RECONSTRUCTION;
                    }
                    else
                    {
                        ReconstructionMode = (UpdateParams_p->IsDecimationNeeded ?
                                VIDBUFF_SECONDARY_RECONSTRUCTION :
                                VIDBUFF_MAIN_RECONSTRUCTION);
                    }
                    DecimationFactor = InternalMemoryProfile.ApplicableDecimationFactor;
                }
#ifdef ST_deblock
                else
                {
                    if ((UpdateParams_p->StreamWidth > 720)||(UpdateParams_p->StreamHeight > 576)) /* HD */
                    {
                        /* Do not take into account STVID_MPEG_DEBLOCK_RECONSTRUCTION for HD streams*/
                        DecimationFactor = InternalMemoryProfile.ApplicableDecimationFactor &~ STVID_MPEG_DEBLOCK_RECONSTRUCTION;

                        /* Retrieve the action to performed. */
                        if(InternalMemoryProfile.ApplicableDecimationFactor == STVID_POSTPROCESS_RECONSTRUCTION)
                        {
                            ReconstructionMode = VIDBUFF_SECONDARY_RECONSTRUCTION;
                        }
                        else
                        {
                            ReconstructionMode = (UpdateParams_p->IsDecimationNeeded ?
                                    VIDBUFF_SECONDARY_RECONSTRUCTION :
                                VIDBUFF_MAIN_RECONSTRUCTION);
                        }
                    }
                    else
                    {
                        DecimationFactor = STVID_MPEG_DEBLOCK_RECONSTRUCTION;
                        ReconstructionMode = VIDBUFF_SECONDARY_RECONSTRUCTION;
                    }
                }
#endif /* ST_deblock */

                /* Test if decimation is needed, and decimation possible (allowed by memory profile).   */
                if ( (ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)
                   &&(InternalMemoryProfile.ApplicableDecimationFactor == STVID_DECIMATION_FACTOR_NONE)
                   &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG4P2)
                   &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG4P2)
                   &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_AVS)
                   &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7200_MPEG4P2))
                {
                    /* *********************************  DEBUG  traces.  ********************************* */
                        COMP_TraceBuffer(("\r\n    Not allowed : Secondary non-existent"));
                    /* ******************************  DEBUG  traces (end).  ****************************** */

                    /* The layer inform that a decimation is needed, BUT no decimation was allowed by   */
                    /* the memory profile. Set the MAIN_RECONSTRUCTION active.                          */
                    ReconstructionMode = VIDBUFF_MAIN_RECONSTRUCTION;
#ifdef ST_display
                    /* Inform the display module to allowed or forbid the decimation to display.        */
                    ErrorCode = VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle, LayerType,
                            VIDBUFF_MAIN_RECONSTRUCTION);
#endif /* ST_display */
                    /* And consider the result is ST_ERROR_ALREADY_INITIALIZED. The driver should send  */
                    /* an event to application to inform decimation is needed.                          */
                    ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
                }
                else if ( (ReconstructionMode == VIDBUFF_MAIN_RECONSTRUCTION) &&
                        (InternalMemoryProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE) &&
                        ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT)||
                        (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)) )
                {
                    /* *********************************  DEBUG  traces.  ********************************* */
                        COMP_TraceBuffer(("\r\n    Not allowed : Primary non-existent"));
                    /* ******************************  DEBUG  traces (end).  ****************************** */
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
#ifdef ST_display
                else
                {
                    /* Default case :                                                                   */
                    /* Inform the display module to allow or forbid the decimation to display. */
                    ErrorCode = VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle, LayerType, ReconstructionMode);
                }
#endif /* ST_display */
                if (ErrorCode == ST_NO_ERROR)
                {
                    /* *********************************  DEBUG  traces.  ********************************* */
                        COMP_TraceBuffer(("\r\n    Allowed : %s RECONSTRUCTION",
                                (ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION ?
                                "SECONDARY":"MAIN") ));
                    /* ******************************  DEBUG  traces (end).  ****************************** */
                    /* Update informations because they will be used by the layer. */
                    if (ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)
                    {

                        if ((DecimationFactor & STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2) == STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2)
                        {
                            DecimationFactor&=~(STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2);
                            if (UpdateParams_p->StreamWidth <= 1280)
                            {
                                DecimationFactor |= STVID_DECIMATION_FACTOR_H2;
                            }
                            else
                            {
                                DecimationFactor |= STVID_DECIMATION_FACTOR_H4;
                            }
                        }
                        ConvertDecimationValues(DecimationFactor,
                            &LayerHorizontalDecimationFactor,
                            &LayerVerticalDecimationFactor);
#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
                        /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
                         * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum) */
                        if (UpdateParams_p->StreamWidth <= 720)
                        {
                            LayerHorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
                        }
                        if (UpdateParams_p->StreamHeight <= 576)
                        {
                            LayerVerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
                        }
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
                        if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                            (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                            (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
                        {
                            /* Force decimation mode for MPEG4-P2 */
                            LayerVerticalDecimationFactor   = STLAYER_DECIMATION_MPEG4_P2;
                            LayerHorizontalDecimationFactor = STLAYER_DECIMATION_MPEG4_P2;
                        }
                        else if (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) /* PLE_TO_DO : STLAYER_DECIMATION_MPEG4_P2 is not a well named flag. Needs to be reviewed and modified */
                        {
                          /* Force decimation mode for MPEG4-P2 */
                          LayerVerticalDecimationFactor   = STLAYER_DECIMATION_AVS;
                          LayerHorizontalDecimationFactor = STLAYER_DECIMATION_AVS;
                        }
                    }
                    else
                    {
                        LayerHorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
                        LayerVerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
                    }

                    /* Update all viewports opened in this layer. */
                    for (k = 0; k < Device_p->DeviceData_p->MaxViewportNumber; k ++)
                    {
                        if((Device_p->DeviceData_p->ViewportProperties_p[k].Identity.ViewPortOpenedAndValidityCheck
                                    == VIDAPI_VALID_VIEWPORT_HANDLE )
                        && (Device_p->DeviceData_p->ViewportProperties_p[k].Identity.Layer_p->LayerType
                                    == LayerType))
                        {
                            Device_p->DeviceData_p->ViewportProperties_p[k].Params.ViewportParametersAskedToLayerViewport.Source_p->Data.VideoStream_p->HorizontalDecimationFactor
                                = LayerHorizontalDecimationFactor;
                            Device_p->DeviceData_p->ViewportProperties_p[k].Params.ViewportParametersAskedToLayerViewport.Source_p->Data.VideoStream_p->VerticalDecimationFactor
                                = LayerVerticalDecimationFactor;
                        }
                    }
                }

                /* Check if the event STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT is to be sent : if VIDDISP_SetReconstructionMode    */
                /* returns ST_ERROR_ALREADY_INITIALIZED, it means that we were already in SECONDARY. So the current Decimation  */
                /* is not enough. Ask the recommended decimation.                                                               */
                if ( ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
                {
                    /* *********************************  DEBUG  traces.  ********************************* */
                        COMP_TraceBuffer(("\r\n   Impossible (%s activated)",
                                (ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION ?
                                "SECOND":"MAIN") ));
                        COMP_TraceBuffer(("\r\n   Event with (H%d/V%d)",
                                UpdateParams_p->RecommendedHorizontalDecimation << 1,
                                UpdateParams_p->RecommendedVerticalDecimation   << 1 ));
                    /* ******************************  DEBUG  traces (end).  ****************************** */
                    /* Send event Impossible with current memory profile. */
                    APIMemoryProfile.DecimationFactor = 0;

                    switch (UpdateParams_p->RecommendedVerticalDecimation)
                    {
                        case STLAYER_DECIMATION_FACTOR_2 :
                            APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_V2;
                            break;

                        case STLAYER_DECIMATION_FACTOR_4 :
                            APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_V4;
                            break;
                        default:
                            /* Nothing to do for other cases. */
                            break;
                    }
                    switch (UpdateParams_p->RecommendedHorizontalDecimation)
                    {
                        case STLAYER_DECIMATION_FACTOR_2 :
                            APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_H2;
                            break;

                        case STLAYER_DECIMATION_FACTOR_4 :
                            APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_H4;
                            break;
                        default:
                            /* Nothing to do for other cases. */
                            break;
                    }
                    /* Notify the event.    */
                    STEVT_Notify(Device_p->DeviceData_p->EventsHandle,
                            Device_p->DeviceData_p->EventID[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID],
                            STEVT_EVENT_DATA_TYPE_CAST &APIMemoryProfile);
                }
                else if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Set of ReconstructionMode. Error = 0x%x !", ErrorCode));
                }
            }
        }
    }
}

/*******************************************************************************
Name        : UpdateParamsFromLayer
Description : Function called when occurs the layer video event :
              STLAYER_UPDATE_PARAMS_EVT.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void UpdateParamsFromLayer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    STLAYER_UpdateParams_t *    UpdateParams_p;
    VideoDevice_t *             Device_p = (VideoDevice_t *) SubscriberData_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STLAYER_Layer_t             LayerType;
    STLAYER_VTGParams_t         NullVTGParams;
    STVID_MemoryProfile_t       APIMemoryProfile;
    VIDCOM_InternalProfile_t    InternalMemoryProfile;
    U32                         k;
    U32                         LayerId = 0;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(Event);

    NullVTGParams.VTGName[0] = '\0';
    NullVTGParams.VTGFrameRate = 0;

    /* Store locally the parameters given in the event. */
    UpdateParams_p = (STLAYER_UpdateParams_t *) EventData_p;

    LayerId = IdentOfNamedLayer(Device_p->DeviceData_p,RegistrantName);
    if(LayerId == BAD_LAYER_INDEX)
    {
        /* it's an error case */
        return;
    }

    LayerType = Device_p->DeviceData_p->LayerProperties_p[LayerId].LayerType;
    /* There may be many reasons at one time, so process all of them */

    if ((UpdateParams_p->UpdateReason & STLAYER_DECIMATION_NEED_REASON) != 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"ERROR: Decimation is no more managed in UpdateParamsFromLayer"));
        return;
    }

    if ((UpdateParams_p->UpdateReason & STLAYER_CONNECT_REASON) != 0)
    {
#ifdef ST_producer
        /* Check if with the new framebuffer hold time, we need to change the decoded pictures */
        ErrorCode = AffectDecodedPicturesAndTheBuffersFreeingStrategy(Device_p, UpdateParams_p->DisplayParamsForVideo.FrameBufferHoldTime);
#endif /* ST_producer */
#ifdef ST_display
        VIDDISP_NewMixerConnection(Device_p->DeviceData_p->DisplayHandle,
                                UpdateParams_p->DisplayParamsForVideo.FrameBufferDisplayLatency,
                                UpdateParams_p->DisplayParamsForVideo.FrameBufferHoldTime);
#endif /* ST_display */
        /* Update the DisplayLatency parameter and update the mode for inserting
           pictures to the display */
        Device_p->DeviceData_p->LayerProperties_p[LayerId].DisplayLatency = UpdateParams_p->DisplayParamsForVideo.FrameBufferDisplayLatency;
        if((UpdateParams_p->DisplayParamsForVideo.FrameBufferDisplayLatency == 2)
           &&(LayerType == STLAYER_COMPOSITOR))
        {
            Device_p->DeviceData_p->UpdateDisplayQueueMode = VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_START;
        }
    }

    if ((UpdateParams_p->UpdateReason & STLAYER_LAYER_PARAMS_REASON) != 0)
    {
        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        /* Update all viewports opened in this layer. */
        for (k = 0; k < Device_p->DeviceData_p->MaxViewportNumber; k ++)
        {
            if((Device_p->DeviceData_p->ViewportProperties_p[k].Identity.ViewPortOpenedAndValidityCheck
                        == VIDAPI_VALID_VIEWPORT_HANDLE)
           && (Device_p->DeviceData_p->ViewportProperties_p[k].Identity.Layer_p->LayerType
                        == LayerType))
            {
                /* Viewport is in this layer: apply changes. */
                ErrorCode = ChangeViewportParams(&Device_p->DeviceData_p->ViewportProperties_p[k]);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"ChangeViewportParams() failed. Error = 0x%x !", ErrorCode));
                }
            }
        }

        /* Fill-in layer params info */
        VIDDISP_UpdateLayerParams(Device_p->DeviceData_p->DisplayHandle, UpdateParams_p->LayerParams_p, LayerId);

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    } /* end STLAYER_LAYER_PARAMS_REASON */

    if ((UpdateParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
    {
        /* Informs all the module of a new VTG. */
        ErrorCode = SetVTGParams(Device_p, &(UpdateParams_p->VTGParams),
                                RegistrantName);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"SetVTGParams() failed. Error = 0x%x !", ErrorCode));
        }
    } /* end STLAYER_VTG_REASON */

    if ((UpdateParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
    {
        /* Informs all the module of a new VTG. */
        ErrorCode = SetVTGParams(Device_p, &NullVTGParams,RegistrantName);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"SetVTGParams() failed. Error = 0x%x !", ErrorCode));
        }
#ifdef ST_display
        /* Set default values for framebuffer display lateny and frame buffer hold time */
        VIDDISP_NewMixerConnection(Device_p->DeviceData_p->DisplayHandle, 1, 1);
#endif /* ST_display */
    } /* end STLAYER_DISCONNECT_REASON */

    if ((UpdateParams_p->UpdateReason & STLAYER_IMPOSSIBLE_WITH_PROFILE) != 0)
    {
        /* *********************************  DEBUG  traces.  ********************************* */
            COMP_TraceBuffer(("\r\nImpossible with profile (H%d/V%d)",
                    UpdateParams_p->RecommendedVerticalDecimation   << 1,
                    UpdateParams_p->RecommendedHorizontalDecimation << 1 ));
        /* ******************************  DEBUG  traces (end).  ****************************** */    /* Retrieve the memory profile.     */
        VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &InternalMemoryProfile);
        APIMemoryProfile=InternalMemoryProfile.ApiProfile;
        APIMemoryProfile.DecimationFactor = 0;

        switch (UpdateParams_p->RecommendedVerticalDecimation)
        {
            case STLAYER_DECIMATION_FACTOR_2 :
                APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_V2;
                break;

            case STLAYER_DECIMATION_FACTOR_4 :
            default:
                APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_V4;
                break;
        }
        switch (UpdateParams_p->RecommendedHorizontalDecimation)
        {
            case STLAYER_DECIMATION_FACTOR_2 :
                APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_H2;
                break;

            case STLAYER_DECIMATION_FACTOR_4 :
            default:
                APIMemoryProfile.DecimationFactor |= STVID_DECIMATION_FACTOR_H4;
                break;
        }

        /* Notify the event.    */
        STEVT_Notify(Device_p->DeviceData_p->EventsHandle,
                Device_p->DeviceData_p->EventID[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID],
                STEVT_EVENT_DATA_TYPE_CAST &APIMemoryProfile);
    } /* end STLAYER_IMPOSSIBLE_WITH_PROFILE */

/*    if ((UpdateParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)*/
/*    if ((UpdateParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)*/
/*    if ((UpdateParams_p->UpdateReason & STLAYER_CHANGE_ID_REASON) != 0)*/
/*    if ((UpdateParams_p->UpdateReason & STLAYER_DISPLAY_REASON) != 0)*/

} /* End of UpdateParamsFromLayer() function */

#ifdef ST_display
/*******************************************************************************
Name        : UpdateBlitterDisplay
Description : Function called when occurs the Display Hal event :
              VIDDISP_UPDATE_GFX_LAYER_SRC_EVT. Ask the blitter-display to
              refresh the displayed picture pointers
Parameters  : EVT API
Assumptions : Limitations :
Returns     : None
*******************************************************************************/
static void UpdateBlitterDisplay(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    void *EventData_p, void *SubscriberData_p)
{
    STLAYER_ViewPortHandle_t                VPHandle ;
    VIDDISP_BlitterDisplayUpdateParams_t *  UpdateParams_p;
    VideoDeviceData_t *                     DeviceData_p;
    VideoViewPort_t *                       ViewPort_p;
    STAVMEM_SharedMemoryVirtualMapping_t *  VirtualMapping_p;
    U32                                     Id;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Retrieve the current device data pointer */
    DeviceData_p    = (VideoDeviceData_t*) SubscriberData_p;
    /* Retrieve event data */
    UpdateParams_p  = (VIDDISP_BlitterDisplayUpdateParams_t*) EventData_p;

    /* Current Implementation: all the viewports of this instance are updated at the same time      */
    /* -possible because each time a layer is connected, the associated vtg replace the existing    */
    /* see SetVTGParams(): it calls VIDDISP_NewVTG() with 'MainNotAux' = TRUE                       */
    /* --> So the display task works in single display mode (one Vsync callback, synchronized on    */
    /* the latest connected) whatever the number of VP/Layer connected on this instance             */

    /* -no pb if VPs are connected on displays with the same VTG (always true on st5100 single vtg) */

    for(Id = 0; Id < DeviceData_p->MaxViewportNumber; Id++)
    {
        if(DeviceData_p->ViewportProperties_p[Id].Identity.ViewPortOpenedAndValidityCheck
                == VIDAPI_VALID_VIEWPORT_HANDLE)
        {
            ViewPort_p = &DeviceData_p->ViewportProperties_p[Id];
            VirtualMapping_p= &(ViewPort_p->Identity.Device_p->DeviceData_p
                                    ->VirtualMapping);
        }
        else
        {
            continue; /* no VP is stored at this index: take next */
        }
        /* Update Data1 field (Luma) in bitmap */
        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.
            VideoStream_p->BitmapParams.Data1_p
                    = (void *)((U32) UpdateParams_p->LumaBuff_p
                    + (U32) (VirtualMapping_p->VirtualBaseAddress_p)
                    - (U32) (VirtualMapping_p->PhysicalAddressSeenFromDevice_p));
        /* Update Data2 field (Chroma) in bitmap */
        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.
            VideoStream_p->BitmapParams.Data2_p
                = (void *)((U32) UpdateParams_p->ChromaBuff_p
                + (U32) (VirtualMapping_p->VirtualBaseAddress_p)
                - (U32) (VirtualMapping_p->PhysicalAddressSeenFromDevice_p));

        /* Update the field to be presented in bitmap */
        if(ViewPort_p->Params.VideoViewportParameters.Source_p->Data.
                VideoStream_p->ScanType == STGXOBJ_INTERLACED_SCAN)
        {
            ViewPort_p->Params.VideoViewportParameters.Source_p->Data.
                VideoStream_p->PresentedFieldInverted
                                        = UpdateParams_p->FieldInverted;
        }
        else /* scan-type is progr so it is not good to ask an inversion */
        {
            ViewPort_p->Params.VideoViewportParameters.Source_p->Data.
                VideoStream_p->PresentedFieldInverted = FALSE;
        }

        /* Apply updated params */
        VPHandle = ViewPort_p->Identity.LayerViewPortHandle;
        STLAYER_SetViewPortSource(VPHandle,
                (ViewPort_p->Params.VideoViewportParameters.Source_p));
    }
} /* End of UpdateBlitterDisplay() function */

/*******************************************************************************
Name        : UpdateVideoDisplay
Description : Function called when occurs the Display Hal event :
              VIDDISP_UPDATE_GFX_LAYER_SRC_EVT.
Parameters  : EVT API
Assumptions : Layer potential errors not tested!
Limitations :
Returns     : None
*******************************************************************************/
static void UpdateVideoDisplay(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    void *EventData_p, void *SubscriberData_p)
{
    STLAYER_ViewPortHandle_t        VPHandle ;
    VIDDISP_LayerUpdateParams_t*    UpdateParams_p;
    STLAYER_ViewPortSource_t        Source;
    STLAYER_StreamingVideo_t        VideoStream;
    VideoLayer_t*                   VideoLayer_p;
    U32                             Id;
    VideoDeviceData_t * DeviceData_p    = (VideoDeviceData_t*) SubscriberData_p;
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_TransformParam_t         *VIDFGTparams_p;
    STLAYER_XVPParam_t              XVPParams;
    U8                              Index;
#endif /* ST_XVP_ENABLE_FGT */

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Retrieve event data */
    UpdateParams_p = (VIDDISP_LayerUpdateParams_t*) EventData_p;

    for(Id = 0; Id < DeviceData_p->MaxViewportNumber; Id++)
    {
        if(DeviceData_p->ViewportProperties_p[Id].Identity.ViewPortOpenedAndValidityCheck
                == VIDAPI_VALID_VIEWPORT_HANDLE)
        {
            /* A valid viewport found. Test if it's the good layer. */
            VideoLayer_p = DeviceData_p->ViewportProperties_p[Id].Identity.Layer_p;

            if ( (VideoLayer_p != NULL) && (VideoLayer_p->LayerType == UpdateParams_p->LayerType)
                && (UpdateParams_p->LayerType != STLAYER_GAMMA_GDP) )
            {
                VPHandle = DeviceData_p->ViewportProperties_p[Id].Identity.LayerViewPortHandle;
            }
            else
            {
                continue;
            }
        }
        else
        {
            continue; /* no VP is stored at this index: take next */
        }

#ifdef ST_XVP_ENABLE_FGT
        /* send parameters to FLEXVP */
        VIDFGTparams_p = (VIDFGT_TransformParam_t *)VIDFGT_GetFlexVpParamsPointer(DeviceData_p->FGTHandle);
        if (VIDFGTparams_p)
        {
            XVPParams.FGTParams.State                   = TRUE;
            XVPParams.FGTParams.Flags                   = VIDFGTparams_p->Flags;
            XVPParams.FGTParams.LumaAccBufferAddr       = VIDFGTparams_p->LumaAccBufferAddr;
            XVPParams.FGTParams.ChromaAccBufferAddr     = VIDFGTparams_p->ChromaAccBufferAddr;
            XVPParams.FGTParams.FieldType               = VIDFGTparams_p->FieldType;
            XVPParams.FGTParams.PictureSize             = VIDFGTparams_p->PictureSize;
            XVPParams.FGTParams.UserZoomOut             = VIDFGTparams_p->UserZoomOut;
            XVPParams.FGTParams.LutAddr                 = VIDFGTparams_p->LutAddr;
            XVPParams.FGTParams.LumaSeed                = VIDFGTparams_p->LumaSeed;
            XVPParams.FGTParams.CbSeed                  = VIDFGTparams_p->CbSeed;
            XVPParams.FGTParams.CrSeed                  = VIDFGTparams_p->CrSeed;
            XVPParams.FGTParams.Log2ScaleFactor         = VIDFGTparams_p->Log2ScaleFactor;
            XVPParams.FGTParams.GrainPatternCount       = VIDFGTparams_p->GrainPatternCount;
            XVPParams.FGTParams.ViewportOutX            = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.OutputRectangle.PositionX;
            XVPParams.FGTParams.ViewportOutY            = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.OutputRectangle.PositionY;
            XVPParams.FGTParams.ViewportOutWidth        = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.OutputRectangle.Width;
            XVPParams.FGTParams.ViewportOutHeight       = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.OutputRectangle.Height;
            XVPParams.FGTParams.ViewportInX             = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.InputRectangle.PositionX;
            XVPParams.FGTParams.ViewportInY             = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.InputRectangle.PositionY;
            XVPParams.FGTParams.ViewportInWidth         = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.InputRectangle.Width;
            XVPParams.FGTParams.ViewportInHeight        = DeviceData_p->ViewportProperties_p[Id].Params.VideoViewportParameters.InputRectangle.Height;

            for (Index = 0; Index < MAX_GRAIN_PATTERN; Index++)
            {
                XVPParams.FGTParams.GrainPatternAddrArray[Index] = VIDFGTparams_p->GrainPatternAddrArray[Index];
            }
        }
        else
        {
            XVPParams.FGTParams.State = FALSE;
        }
        STLAYER_XVPSetParam(VPHandle, &XVPParams);
#endif /* ST_XVP_ENABLE_FGT */

        Source.Data.VideoStream_p = &VideoStream;
        /* Retrieving source */
        Source.Data.VideoStream_p->SourceNumber = DeviceData_p->ViewportProperties_p[Id].Identity.Device_p->DecoderNumber;

        Source.Data.VideoStream_p->PresentedFieldInverted = UpdateParams_p->PresentedFieldInverted;

        Source.Data.VideoStream_p->PreviousField.FieldAvailable = UpdateParams_p->PreviousField.FieldAvailable;
        Source.Data.VideoStream_p->PreviousField.FieldType = UpdateParams_p->PreviousField.FieldType;
        Source.Data.VideoStream_p->PreviousField.PictureIndex = UpdateParams_p->PreviousField.PictureIndex;

        Source.Data.VideoStream_p->CurrentField.FieldAvailable = UpdateParams_p->CurrentField.FieldAvailable;
        Source.Data.VideoStream_p->CurrentField.FieldType = UpdateParams_p->CurrentField.FieldType;
        Source.Data.VideoStream_p->CurrentField.PictureIndex = UpdateParams_p->CurrentField.PictureIndex;

        Source.Data.VideoStream_p->NextField.FieldAvailable = UpdateParams_p->NextField.FieldAvailable;
        Source.Data.VideoStream_p->NextField.FieldType = UpdateParams_p->NextField.FieldType;
        Source.Data.VideoStream_p->NextField.PictureIndex = UpdateParams_p->NextField.PictureIndex;

        Source.Data.VideoStream_p->AdvancedHDecimation = UpdateParams_p->AdvancedHDecimation;
        ConvertDecimationValues (UpdateParams_p->DecimationFactors,
                &(Source.Data.VideoStream_p->HorizontalDecimationFactor),
                &(Source.Data.VideoStream_p->VerticalDecimationFactor));

        Source.Data.VideoStream_p->IsDisplayStopped = UpdateParams_p->IsDisplayStopped;
        Source.Data.VideoStream_p->PeriodicFieldInversionDueToFRC = UpdateParams_p->PeriodicFieldInversionDueToFRC;

        /* then apply updated params */
        STLAYER_SetViewPortSource(VPHandle, &Source);

#ifdef ST_XVP_ENABLE_FGT
        STLAYER_XVPCompute(VPHandle);
#endif /* ST_XVP_ENABLE_FGT */
    }

} /* End of UpdateVideoDisplay() function */


/*******************************************************************************
Name        : UpdateGfxLayerSrc
Description : Function called when occurs the Display Hal event :
              VIDDISP_UPDATE_GFX_LAYER_SRC_EVT.
Parameters  : EVT API
Assumptions : Layer potential errors not tested!
              Meaningfull for Gfx Layer
Limitations :
Returns     : None
*******************************************************************************/
static void UpdateGfxLayerSrc(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    void *EventData_p, void *SubscriberData_p)
{
    STLAYER_ViewPortHandle_t        VPHandle ;
    VIDDISP_LayerUpdateParams_t*    UpdateParams_p;
    STLAYER_ViewPortSource_t        Source;
    STGXOBJ_Bitmap_t                Bitmap;
    STGXOBJ_Palette_t               Palette;
    VideoViewPort_t *               ViewPort_p = (VideoViewPort_t *) SubscriberData_p;
    STAVMEM_SharedMemoryVirtualMapping_t * VirtualMapping_p = &(ViewPort_p->Identity.Device_p->DeviceData_p->VirtualMapping);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Retrieve ViewPort Handle */
    VPHandle = ViewPort_p->Identity.LayerViewPortHandle;

    /* Retrieve event data */
    UpdateParams_p = (VIDDISP_LayerUpdateParams_t*) EventData_p;

    /* Retrieve source from viewport */
    Source.Data.BitMap_p = &Bitmap;
    Source.Palette_p     = &Palette;

    STLAYER_GetViewPortSource(VPHandle, &Source);

    /* Update bitmap */
    if (((UpdateParams_p->UpdateReason) & VIDDISP_LAYER_UPDATE_TOP_FIELD_ADDRESS) != 0)
    {
        /* Update Data1 field (Top) in bitmap */
        Bitmap.Data1_p = (void *)((U32) UpdateParams_p->Data1_p
                                + (U32) (VirtualMapping_p->VirtualBaseAddress_p)
                                - (U32) (VirtualMapping_p->PhysicalAddressSeenFromDevice_p));
    }
    if (((UpdateParams_p->UpdateReason) & VIDDISP_LAYER_UPDATE_BOTTOM_FIELD_ADDRESS) != 0)
    {
        /* Update Data2 field (Bottom) in bitmap */
        Bitmap.Data2_p = (void *)((U32)UpdateParams_p->Data2_p
                                + (U32) (VirtualMapping_p->VirtualBaseAddress_p)
                                - (U32) (VirtualMapping_p->PhysicalAddressSeenFromDevice_p));
    }

    /* then apply updated params */
    STLAYER_SetViewPortSource(VPHandle, &Source);

} /* End of UpdateGfxLayerSrc() function */

/*******************************************************************************
Name        : UpdateGdpLayerSrc
Description : Function called when occurs the Display Hal event :
              VIDDISP_UPDATE_GFX_LAYER_SRC_EVT.
Parameters  : EVT API
Assumptions : Layer potential errors not tested!
              Meaningfull for Graphics Layer
Limitations :
Returns     : None
*******************************************************************************/
static void UpdateGdpLayerSrc(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    void *EventData_p, void *SubscriberData_p)
{
    STLAYER_ViewPortHandle_t        VPHandle ;
    VIDDISP_LayerUpdateParams_t*    UpdateParams_p;
    STLAYER_ViewPortSource_t        Source;
    STGXOBJ_Bitmap_t                Bitmap;
    STGXOBJ_Palette_t               Palette;
    VideoLayer_t*                   VideoLayer_p;
    VideoViewPort_t *               ViewPort_p;
    U32                             Id;
    VideoDeviceData_t * DeviceData_p    = (VideoDeviceData_t*) SubscriberData_p;
    STAVMEM_SharedMemoryVirtualMapping_t * VirtualMapping_p = &(DeviceData_p->VirtualMapping);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Retrieve event data */
    UpdateParams_p = (VIDDISP_LayerUpdateParams_t*) EventData_p;

    for(Id = 0; Id < DeviceData_p->MaxViewportNumber; Id++)
    {
        if(DeviceData_p->ViewportProperties_p[Id].Identity.ViewPortOpenedAndValidityCheck
                == VIDAPI_VALID_VIEWPORT_HANDLE)
        {
            /* A valid viewport found. Test if it's the good layer. */
            VideoLayer_p = DeviceData_p->ViewportProperties_p[Id].Identity.Layer_p;

            if ( (VideoLayer_p != NULL) && (VideoLayer_p->LayerType == UpdateParams_p->LayerType)
                    && (UpdateParams_p->LayerType == STLAYER_GAMMA_GDP) )
            {   /* Retrieve ViewPort Handle */
                VPHandle = DeviceData_p->ViewportProperties_p[Id].Identity.LayerViewPortHandle;
            }
            else
            {
                continue;
            }
        }
        else
        {
            continue; /* no VP is stored at this index: take next */
        }

        ViewPort_p = &(DeviceData_p->ViewportProperties_p[Id]);

        /* Retrieve source from viewport */
        Source.Data.BitMap_p = &Bitmap;
        Source.Palette_p     = &Palette;

        STLAYER_GetViewPortSource(VPHandle, &Source);

        /* Update bitmap */
        if (((UpdateParams_p->UpdateReason) & VIDDISP_LAYER_UPDATE_TOP_FIELD_ADDRESS) != 0)
        {
            /* Update Data1 field (Top) in bitmap */
            Bitmap.Data1_p = (void *)((U32) UpdateParams_p->Data1_p
                       + (U32) (VirtualMapping_p->VirtualBaseAddress_p)
                       - (U32) (VirtualMapping_p->PhysicalAddressSeenFromDevice_p));
        }
        if (((UpdateParams_p->UpdateReason) & VIDDISP_LAYER_UPDATE_BOTTOM_FIELD_ADDRESS) != 0)
        {
            /* Update Data2 field (Bottom) in bitmap */
           Bitmap.Data2_p = (void *)((U32)UpdateParams_p->Data2_p
                      + (U32) (VirtualMapping_p->VirtualBaseAddress_p)
                      - (U32) (VirtualMapping_p->PhysicalAddressSeenFromDevice_p));
        }

        /* To avoid a Null Bitmap address; */
        /* this case could happen at the starting, when the second fiels not yet assigned */
        if(Bitmap.Data2_p == 0)
        {
            Bitmap.Data2_p = Bitmap.Data1_p;
        }
        if(Bitmap.Data1_p == 0)
        {
            Bitmap.Data1_p = Bitmap.Data2_p;
        }

        /* then apply updated params */
        STLAYER_SetViewPortSource(VPHandle, &Source);

    }

} /* End of UpdateGdpLayerSrc() function */


/*******************************************************************************
Name        : UpdateBlitterWithNextPictureToBeDecoded
Description : Function called when occurs the Parse event :
              VIDDEC_NEW_PICTURE_PARSED_EVT. Ask the blitter-display to
              update the blitter with next picture to be decoded type
Parameters  : EVT API
Assumptions : Limitations :
Returns     : None
*******************************************************************************/
static void UpdateBlitterWithNextPictureToBeDecoded(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    void *EventData_p, void *SubscriberData_p)
{
    STLAYER_ViewPortHandle_t                VPHandle ;
    STVID_PictureInfos_t *                  UpdateParams_p;
    STGXOBJ_PictureInfos_t                  PictureInfos;
    VideoDeviceData_t *                     DeviceData_p;
    VideoViewPort_t *                       ViewPort_p;
    U32                                     Id;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Retrieve the current device data pointer */
    DeviceData_p    = (VideoDeviceData_t*) SubscriberData_p;
    /* Retrieve event data */
    UpdateParams_p  = (STVID_PictureInfos_t*) EventData_p;

    for(Id = 0; Id < DeviceData_p->MaxViewportNumber; Id++)
    {
        if(DeviceData_p->ViewportProperties_p[Id].Identity.ViewPortOpenedAndValidityCheck
                == VIDAPI_VALID_VIEWPORT_HANDLE)
        {
            ViewPort_p = &DeviceData_p->ViewportProperties_p[Id];
        }
        else
        {
            continue; /* no VP is stored at this index: take next */
        }

        /* Apply updated params */
        VPHandle = ViewPort_p->Identity.LayerViewPortHandle;
        PictureInfos.VideoParams.MPEGFrame = UpdateParams_p->VideoParams.MPEGFrame;
        STLAYER_InformPictureToBeDecoded(VPHandle, &PictureInfos);
    }
} /* End of UpdateBlitterWithNextPictureToBeDecoded() function */

/*******************************************************************************
Name        : UpdateBlitterCommit
Description : Function called when occurs the Parse event :
              VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT. Info blitter-display that
              video display parameters are committed
Parameters  : EVT API
Assumptions : Limitations :
Returns     : None
*******************************************************************************/
static void UpdateBlitterCommit(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    void *EventData_p, void *SubscriberData_p)
{
    STLAYER_ViewPortHandle_t                VPHandle ;
    VideoDeviceData_t *                     DeviceData_p;
    VideoViewPort_t *                       ViewPort_p;
    U32                                     Id;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    /* Retrieve the current device data pointer */
    DeviceData_p    = (VideoDeviceData_t*) SubscriberData_p;

    for(Id = 0; Id < DeviceData_p->MaxViewportNumber; Id++)
    {
        if(DeviceData_p->ViewportProperties_p[Id].Identity.ViewPortOpenedAndValidityCheck
                == VIDAPI_VALID_VIEWPORT_HANDLE)
        {
            ViewPort_p = &DeviceData_p->ViewportProperties_p[Id];
        }
        else
        {
            continue; /* no VP is stored at this index: take next */
        }

        /* Apply updated params */
        VPHandle = ViewPort_p->Identity.LayerViewPortHandle;
        STLAYER_CommitViewPortParams(VPHandle);
    }
} /* End of UpdateBlitterCommit() function */

#endif /* ST_display */



/* Exported functions ------------------------------------------------------- */



/*******************************************************************************
Name        : STVID_CloseViewPort
Description : Close a ViewPort
Paraeters  :  ViewPortHandle
Assumptions :
Limitations :
Returns     :  STVID_ERROR_NOT_AVAILABLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_CloseViewPort(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoDevice_t *               Device_p;
    VideoViewPort_t *             ViewPort_p;
    STLAYER_ViewPortParams_t      ViewportParams;
    STGXOBJ_Rectangle_t           InputRectangle, OutputRectangle;
    BOOL                          ViewportParamsDisplayEnable;
#ifdef ST_display
    U32                           Id;
#endif /* ST_display */
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = (((VideoViewPort_t *) ViewPortHandle)->Identity).Device_p;

    /* Ensure nobody else accesses layers/viewports. */
    semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

    /* Get ViewPort parameter to be used to undo in case of failure */
    ErrorCode = STLAYER_GetViewPortIORectangle(ViewPort_p->Identity.LayerViewPortHandle, &InputRectangle, &OutputRectangle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Impossible to undo actions done. */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetViewPortIORectangle() failed. Error = %d !", ErrorCode));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        ViewportParams.InputRectangle   = InputRectangle;
        ViewportParams.OutputRectangle  = OutputRectangle;
        ViewportParams.Source_p         = ViewPort_p->Params.VideoViewportParameters.Source_p;
        ViewportParamsDisplayEnable     = ViewPort_p->Params.DisplayEnable;

        /* Disable view port first */
        ErrorCode = stvid_ActionDisableOutputWindow(ViewPort_p);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"stvid_ActionDisableOutputWindow() failed. Error = %d !", ErrorCode));
            /* ??? Don't fail function for that only ! */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            /* To avoid possible deadlock with stvid_PictureCharacteristicsChange callback function */
            Device_p->DeviceData_p->AreViewportParametersInitialized = FALSE;

            /* Call to STLAYER_CloseViewPort. */
            ErrorCode = stvid_ActionCloseViewPort(ViewPort_p, &ViewportParams);
            if (ErrorCode != ST_NO_ERROR)
            {
                if (ViewportParamsDisplayEnable)
                {
                    ErrorCode = STLAYER_EnableViewPort(ViewPort_p->Identity.LayerViewPortHandle);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableViewPort() failed. Error = 0x%x !", ErrorCode));
                    }
                }
                else
                {
                    ErrorCode = STLAYER_DisableViewPort(ViewPort_p->Identity.LayerViewPortHandle);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_DisableViewPort() failed. Error = 0x%x !", ErrorCode));
                    }
                }
            }

#ifdef ST_display
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_5528_MPEG :
                case STVID_DEVICE_TYPE_5100_MPEG :
                case STVID_DEVICE_TYPE_5525_MPEG :
                case STVID_DEVICE_TYPE_5105_MPEG :
                case STVID_DEVICE_TYPE_5301_MPEG :
                    ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,Device_p->DeviceName,
                                                            VIDDISP_UPDATE_GFX_LAYER_SRC_EVT);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_UnsubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    } /* end error unsubscribe */

                    ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,Device_p->DeviceName,
                                                            VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_UnsubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    } /* end error unsubscribe */

                    break;
                case STVID_DEVICE_TYPE_7710_MPEG :
                case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7100_MPEG :
                case STVID_DEVICE_TYPE_7100_MPEG4P2 :
                case STVID_DEVICE_TYPE_7100_H264 :
                case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_ZEUS_MPEG :
                case STVID_DEVICE_TYPE_ZEUS_H264 :
                case STVID_DEVICE_TYPE_ZEUS_VC1  :
                case STVID_DEVICE_TYPE_7109_MPEG :
                case STVID_DEVICE_TYPE_7109D_MPEG :
                case STVID_DEVICE_TYPE_7109_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_AVS :
                case STVID_DEVICE_TYPE_7109_H264 :
                case STVID_DEVICE_TYPE_7109_VC1 :
                case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7200_MPEG :
                case STVID_DEVICE_TYPE_7200_MPEG4P2 :
                case STVID_DEVICE_TYPE_7200_H264 :
                case STVID_DEVICE_TYPE_7200_VC1 :
                case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
                    /* Check if there's a connected layer.      */
                    for(Id = 0; Id < Device_p->DeviceData_p->MaxViewportNumber; Id++)
                    {
                        if(Device_p->DeviceData_p->ViewportProperties_p[Id].Identity.ViewPortOpenedAndValidityCheck
                                == VIDAPI_VALID_VIEWPORT_HANDLE)
                        {
                            /* At least one Layer connection. Exit from loop */
                            break;
                        }
                    }
                    if (Id == Device_p->DeviceData_p->MaxViewportNumber)
                    {
                        /* No more ViewPort opened on this device : unsubscribe to update event. */
                        ErrorCode = STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,Device_p->DeviceName,
                                                            VIDDISP_UPDATE_GFX_LAYER_SRC_EVT);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                    "STEVT_UnsubscribeDeviceEvent() failed. Error = 0x%x !",
                                    ErrorCode));
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                        /* We can now ignore the picture characteristics change event*/
                        Device_p->DeviceData_p->AreViewportParametersInitialized = FALSE;
                    }
                    else
                    {   /* Do not Ignore the picture characteristics change event because another viewport can use it */
                        Device_p->DeviceData_p->AreViewportParametersInitialized = TRUE;
                    }
                    break;
                default :
                    break;
            }
#endif /* ST_display */
        } /* end of stvid_ActionDisableOutputWindow() success */
    } /* end STLAYER_GetViewPortIORectangle() success */

    /* Ensure nobody else accesses layers/viewports. */
    semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

    return(ErrorCode);
} /* End of STVID_CloseViewPort() function */


/*******************************************************************************
Name        : STVID_DisableBorderAlpha
Description : Disable border alpha of a viewport
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_DisableBorderAlpha(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t *ViewPort_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_DisableBorderAlpha(ViewPort_p->Identity.LayerViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_DisableBorderAlpha() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }

    return(ErrorCode);
} /* End of STVID_DisableBorderAlpha() function */


/*******************************************************************************
Name        : STVID_DisableColorKey
Description : Disable color Key of a ViewPort
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_DisableColorKey(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t *ViewPort_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_DisableColorKey(ViewPort_p->Identity.LayerViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_DisableColorKey() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }

    return(ErrorCode);
} /* End of STVID_DisableColorKey() function */


/*******************************************************************************
Name        : STVID_DisableOutputWindow
Description : Disable a ViewPort
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_DisableOutputWindow(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t *   ViewPort_p;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *)ViewPortHandle;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        ErrorCode = stvid_ActionDisableOutputWindow(ViewPort_p);

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    } /* end no error */

    return(ErrorCode);
} /* End of STVID_DisableOutputWindow() function */


/*******************************************************************************
Name        : STVID_EnableBorderAlpha
Description : Enable border alpha of a ViewPort
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_EnableBorderAlpha(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t *ViewPort_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_EnableBorderAlpha(ViewPort_p->Identity.LayerViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableBorderAlpha() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }

    return(ErrorCode);
} /* End of STVID_EnableBorderAlpha() function */


/*******************************************************************************
Name        : STVID_EnableColorKey
Description : Enable color key of a ViewPort
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_EnableColorKey(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t *ViewPort_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_EnableColorKey(ViewPort_p->Identity.LayerViewPortHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableColorKey() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }

    return(ErrorCode);
} /* End of STVID_EnableColorKey() function */


/*******************************************************************************
Name        : STVID_EnableOutputWindow
Description : Enable a ViewPort
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_EnableOutputWindow(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t *       ViewPort_p;
    VideoDevice_t *         Device_p;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Retrieve the Decoder structure from the local ViewPort structure.       */
        Device_p = ViewPort_p->Identity.Device_p;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        /*ErrorCode =*/ stvid_ActionEnableOutputWindow(ViewPort_p);

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    } /* end no error */

    return(ErrorCode);
} /* End of STVID_EnableOutputWindow() function */


/*******************************************************************************
Name        : STVID_GetAlignIOWindows
Description : Return the aligned IO Window
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE   if the given handle is invalid.
              ST_ERROR_BAD_PARAMETER    if any of the input parameters is invalid.
              STVID_ERROR_NOT_AVAILABLE if there is no decoded picture on display
                                        (information not available).
              ST_NO_ERROR               if informations are available.
*******************************************************************************/
ST_ErrorCode_t STVID_GetAlignIOWindows(
        const STVID_ViewPortHandle_t    ViewPortHandle,
        S32 * const InputWinX_p,        S32 * const InputWinY_p,
        U32 * const InputWinWidth_p,    U32 * const InputWinHeight_p,
        S32 * const OutputWinX_p,       S32 * const OutputWinY_p,
        U32 * const OutputWinWidth_p,   U32 * const OutputWinHeight_p)
{
    VideoViewPort_t           *ViewPort_p;
    VideoDevice_t *           Device_p;
#ifdef ST_display
    STGXOBJ_Rectangle_t       InputRectangle, OutputRectangle;
    STVID_PictureInfos_t      CurrentDisplayedPictureInfos;
#endif /* ST_display */
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;

    if ((InputWinX_p == NULL) || (InputWinY_p == NULL) ||
        (InputWinWidth_p == NULL) || (InputWinHeight_p == NULL) ||
        (OutputWinX_p == NULL) || (OutputWinY_p == NULL) ||
        (OutputWinWidth_p == NULL) || (OutputWinHeight_p == NULL))
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!IsValidViewPortHandle(ViewPortHandle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Retrieve the Decoder structure from the local ViewPort structure.       */
        Device_p = ViewPort_p->Identity.Device_p;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

#ifdef ST_display
        /* Ask display if there is a picture on display.                            */
        ErrorCode = VIDDISP_GetDisplayedPictureInfos(Device_p->DeviceData_p->DisplayHandle, &CurrentDisplayedPictureInfos);
        if (ErrorCode == ST_NO_ERROR)
        {
            /* There's a picture on display: enable view port, so we can retrieve   */
            /* the informations concerning the aligned windows.                     */
            ErrorCode = STLAYER_GetViewPortIORectangle( ViewPort_p->Identity.LayerViewPortHandle, &InputRectangle, &OutputRectangle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,  "STLAYER_GetViewPortIORectangle failed. Error = %d !", ErrorCode));
                ErrorCode = STVID_ERROR_NOT_AVAILABLE;
            }
            else
            {
                *InputWinX_p        = InputRectangle.PositionX / 16;
                *InputWinY_p        = InputRectangle.PositionY / 16;
                *InputWinWidth_p    = InputRectangle.Width;
                *InputWinHeight_p   = InputRectangle.Height;
                *OutputWinX_p       = OutputRectangle.PositionX;
                *OutputWinY_p       = OutputRectangle.PositionY;
                *OutputWinWidth_p   = OutputRectangle.Width;
                *OutputWinHeight_p  = OutputRectangle.Height;
            }
        }
        else
        {
#endif /* ST_display */
            /* There's no picture on display: can't retrive aligned windows data.   */
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
#ifdef ST_display
        }
#endif /* ST_display */

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of STVID_GetAlignIOWindows() function */


/*******************************************************************************
Name        : STVID_GetDisplayAspectRatioConversion
Description : Get the Display Aspect Ratio
              Returns the user prefered mode to view 16/9 on 4/3 or 4/3 on 16/9
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetDisplayAspectRatioConversion(
        const STVID_ViewPortHandle_t ViewPortHandle,
        STVID_DisplayAspectRatioConversion_t * const Conversion_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    if (Conversion_p == NULL)
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Return the current Display Aspect Ratio Conversion stored in our        */
        /* ViewPort local structure.                                               */
        *Conversion_p = ViewPort_p->Params.DispARConversion;
    }

    return(ErrorCode);
} /* End of STVID_GetDisplayAspectRatioConversion() function */


/*******************************************************************************
Name        : STVID_GetInputWindowMode
Description : Get the Input Window Mode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetInputWindowMode(
        const STVID_ViewPortHandle_t ViewPortHandle,
        BOOL * const AutoMode_p, STVID_WindowParams_t * const WinParams_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    if ((AutoMode_p == NULL) || (WinParams_p == NULL))
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        /* Return the current Input Windows Mode (automatic or not) and the        */
        /* Input Windows Parameters stored in our ViewPort local structure.        */
        *AutoMode_p = ViewPort_p->Params.InputWinAutoMode;
        *WinParams_p = ViewPort_p->Params.InputWinParams;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of STVID_GetInputWindowMode() function */


/*******************************************************************************
Name        : STVID_GetIOWindows
Description : Get the IO Window
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetIOWindows(
        const STVID_ViewPortHandle_t    ViewPortHandle,
        S32 * const InputWinX_p,        S32 * const InputWinY_p,
        U32 * const InputWinWidth_p,    U32 * const InputWinHeight_p,
        S32 * const OutputWinX_p,       S32 * const OutputWinY_p,
        U32 * const OutputWinWidth_p,   U32 * const OutputWinHeight_p)
{
    VideoViewPort_t       *ViewPort_p;
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

    if ((InputWinX_p == NULL) || (InputWinY_p == NULL) ||
        (InputWinWidth_p == NULL) || (InputWinHeight_p == NULL) ||
        (OutputWinX_p == NULL) || (OutputWinY_p == NULL) ||
        (OutputWinWidth_p == NULL) || (OutputWinHeight_p == NULL))
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        *InputWinX_p      = ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionX / 16;
        *InputWinY_p      = ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionY / 16;
        *InputWinWidth_p  = ViewPort_p->Params.VideoViewportParameters.InputRectangle.Width;
        *InputWinHeight_p = ViewPort_p->Params.VideoViewportParameters.InputRectangle.Height;
        *OutputWinX_p     = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionX;
        *OutputWinY_p     = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionY;
        *OutputWinWidth_p = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Width;
        *OutputWinHeight_p = ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Height;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of STVID_GetIOWindows() function */


/*******************************************************************************
Name        : STVID_GetOutputWindowMode
Description : Get the Output Window Mode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetOutputWindowMode(
        const STVID_ViewPortHandle_t ViewPortHandle,
        BOOL * const AutoMode_p, STVID_WindowParams_t * const WinParams_p)
{
    VideoViewPort_t * ViewPort_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if ((WinParams_p == NULL) || (AutoMode_p == NULL))
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        /* Return the current Output Windows Mode (automatic or not) and the       */
        /* Output Windows Parameters stored in our ViewPort local structure.       */
        *AutoMode_p = ViewPort_p->Params.OutputWinAutoMode;
        *WinParams_p = ViewPort_p->Params.OutputWinParams;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of STVID_GetOutputWindowMode() function */


/*******************************************************************************
Name        : STVID_GetViewPortAlpha
Description : Get the Viewport global alpha
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortAlpha(const STVID_ViewPortHandle_t ViewPortHandle,
        STLAYER_GlobalAlpha_t * const GlobalAlpha_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (GlobalAlpha_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_GetViewPortAlpha(ViewPort_p->Identity.LayerViewPortHandle, GlobalAlpha_p);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetViewPortAlpha() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_GetViewPortAlpha() function */


/*******************************************************************************
Name        : STVID_GetViewPortColorKey
Description : Get the Viewport Color Key
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortColorKey(const STVID_ViewPortHandle_t ViewPortHandle,
        STGXOBJ_ColorKey_t * const ColorKey_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (ColorKey_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_GetViewPortColorKey(ViewPort_p->Identity.LayerViewPortHandle, ColorKey_p);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetViewPortColorKey() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_GetViewPortColorKey() function */


/*******************************************************************************
Name        : STVID_GetViewPortPSI
Description : Get the Viewport PSI
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortPSI(const STVID_ViewPortHandle_t ViewPortHandle,
        STLAYER_PSI_t * const VPPSI_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (VPPSI_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_GetViewPortPSI(ViewPort_p->Identity.LayerViewPortHandle, VPPSI_p);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetViewPortPSI() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_GetViewPortPSI() function */

/*******************************************************************************
Name        : STVID_GetViewPortSpecialMode
Description : Get special mode parameters.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortSpecialMode (const STVID_ViewPortHandle_t ViewPortHandle,
        STLAYER_OutputMode_t * const OuputMode_p,
        STLAYER_OutputWindowSpecialModeParams_t * const Params_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ( (OuputMode_p == NULL) || (Params_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_GetViewPortSpecialMode(ViewPort_p->Identity.LayerViewPortHandle,
                (STLAYER_OutputMode_t *) OuputMode_p,
                (STLAYER_OutputWindowSpecialModeParams_t *) Params_p);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetViewportSpecialMode() failed. Error = 0x%x !", ErrorCode));
        }
    }
    return(ErrorCode);

} /* End of STVID_GetViewPortSpecialMode() function. */

/*******************************************************************************
Name        : STVID_HidePicture
Description : Hide the picture
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_HidePicture(const STVID_ViewPortHandle_t ViewPortHandle)
{
#ifdef ST_display
    VideoDevice_t *             Device_p;
    VideoViewPort_t *           ViewPort_p;
    VideoStatus_t               Status;
    VIDBUFF_PictureBuffer_t     Picture;
    VideoLayer_t *              Layer_p;
    U32                         Layer;
#endif /* ST_display */
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
#ifdef ST_display
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Retrieve the Decoder structure from the local ViewPort structure.       */
        Device_p = ViewPort_p->Identity.Device_p;
        Layer_p     = ViewPort_p->Identity.Layer_p;
        Layer = 0;

        /* In the case if display on auxiliary and secondary (specific 7200) */

        if (( Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
              Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
              (Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
              Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ) )
        {

            if (( Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
              Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ))
            {
                if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                    Layer = 0;
                else if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                    Layer = 1;
            } else
            {
                if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                    Layer = 1;
                else if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                    Layer = 0;
            }
        } else
        {

            if ( (Layer_p->LayerType == STLAYER_7020_VIDEO1)
              || (Layer_p->LayerType == STLAYER_SDDISPO2_VIDEO1)
              || (Layer_p->LayerType == STLAYER_HDDISPO2_VIDEO1)
              || (Layer_p->LayerType == STLAYER_OMEGA2_VIDEO1)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO1)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO2)
              || (Layer_p->LayerType == STLAYER_GAMMA_GDP))
            {
                Layer = 0;
            }
            if ( (Layer_p->LayerType == STLAYER_7020_VIDEO2)
              || (Layer_p->LayerType == STLAYER_SDDISPO2_VIDEO2)
              || (Layer_p->LayerType == STLAYER_HDDISPO2_VIDEO2)
              || (Layer_p->LayerType == STLAYER_OMEGA2_VIDEO2)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4))
            {
                Layer = 1;
            }
        }
        /* Enter in critical section to avoid multi access of the DEVICE's properties at the same time.*/
        stvid_EnterCriticalSection(Device_p);
        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        /* Check if the decoder is not running. */
        Status = GET_VIDEO_STATUS(Device_p);
        if (Status != VIDEO_STATUS_STOPPED)
        {
            ErrorCode = STVID_ERROR_DECODER_RUNNING;
        }
        else
        {
            /* Nothing to do if STVID_ShowPicture has never been executed.             */
            if (ViewPort_p->Params.ShowingPicture)
            {
                /* Restore the reconstruction mode that was available before first STVID_ShowPicture. */
                ErrorCode = VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle
                        , ViewPort_p->Identity.Layer_p->LayerType
                        , ViewPort_p->Params.PictureSave.ReconstructionMode);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDDISP_SetReconstructionMode() failed. Error = 0x%x !", ErrorCode));
                    ErrorCode = STVID_ERROR_NOT_AVAILABLE;
                }
                if (ViewPort_p->Params.PictureSave.WasViewPortEnabledDuringShow)
                {
                    /* Viewport was displaying nothing before the STVID_ShowPicture         */
                    /* function call. Disable it again.                                     */
                    ErrorCode = STLAYER_DisableViewPort(ViewPort_p->Identity.LayerViewPortHandle);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_DisableViewPort() failed. Error = 0x%x !", ErrorCode));
                        /* No error allowed for this function !     */
                        ErrorCode = ST_NO_ERROR;

                    }
                    /* Whatever the return code, reset the flag.    */
                    ViewPort_p->Params.PictureSave.WasViewPortEnabledDuringShow = FALSE;
                }
                else
                {
                    /* Restore viewport informations.           */
                    ViewPort_p->Params.StreamParameters         = ViewPort_p->Params.PictureSave.StreamParameters;
                    ViewPort_p->Params.VideoViewportParameters  = ViewPort_p->Params.PictureSave.LayerViewport;
                    *ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p
                            = ViewPort_p->Params.PictureSave.VideoStreaming;

                    /* Restore viewport's source informations.  */
                    Picture                 = ViewPort_p->Params.PictureSave.PictureBuffer;
                    *(Picture.FrameBuffer_p)= ViewPort_p->Params.PictureSave.FrameBuffer;

                    ErrorCode = VIDDISP_ShowPicture(Device_p->DeviceData_p->DisplayHandle, &Picture, Layer,VIDDISP_RESTORE_ORIGINAL_PICTURE);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDDISP_ShowPicture failed. Error = 0x%x !", ErrorCode));
                        ErrorCode = STVID_ERROR_NOT_AVAILABLE;
                    }

                    /* Call to STVID_SetIOWindows in order to center the Windows.            */
                    ActionSetIOWindows(ViewPort_p,
                                    ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionX,
                                    ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionY,
                                    ViewPort_p->Params.VideoViewportParameters.InputRectangle.Width,
                                    ViewPort_p->Params.VideoViewportParameters.InputRectangle.Height,
                                    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionX,
                                    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionY,
                                    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Width,
                                    ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Height);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ActionSetIOWindows() failed. Error = 0x%x !", ErrorCode));
                        ErrorCode = STVID_ERROR_NOT_AVAILABLE;
                    }
                    else
                    {
                        /* Restore previous freeze mode */
                        /* TO DO */
                    }
                }
            }
        }

        ViewPort_p->Params.ShowingPicture = FALSE;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
        stvid_LeaveCriticalSection(Device_p);
    }
#else /* ST_display */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_display */

    return(ErrorCode);
} /* End of STVID_HidePicture() function */


/*******************************************************************************
Name        : STVID_OpenViewPort
Description : Open a ViewPort
Parameters  :
Assumptions : Layer_Init is called by the apps :
              The Layer DeviceName is in the ViewPort OpenParam.
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_OpenViewPort(
         const STVID_Handle_t                   VideoHandle,
         const STVID_ViewPortParams_t * const   ViewPortParams_p,
         STVID_ViewPortHandle_t * const         ViewPortHandle_p)
{
    STLAYER_LayerParams_t         LayerParams;
    STLAYER_ViewPortParams_t      OpenViewPortParams;
    VideoDevice_t *               Device_p;
    VideoViewPort_t *             ViewPort_p;
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;
    ST_DeviceName_t               LayerName;
    STLAYER_VTGParams_t           VTGParams;
#ifdef ST_display
    STVID_PictureInfos_t          CurrentDisplayedPictureInfos;
    STEVT_DeviceSubscribeParams_t SubscribeParams;
#endif /* ST_display */
    BOOL                           IsInputOutputComputingNeeded;
    BOOL                            Found;
    VideoLayer_t *                VideoLayer_p;
    U32                           k;
#ifdef ST_producer
    STLAYER_Capability_t                    LayerCapability;
#endif /* ST_producer */
#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
    VIDCOM_InternalProfile_t      MemoryProfile;
#endif

    if ((ViewPortParams_p == NULL) || (ViewPortHandle_p == NULL))
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* The Layer name is an inout parameter of the function.                   */
    if ((strlen(ViewPortParams_p->LayerName) > ST_MAX_DEVICE_NAME - 1) ||
        ((ViewPortParams_p->LayerName)[0] == '\0'))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_Init the size of the Device Name is not permitted"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* The Layer name is an inout parameter of the function.                   */
    strcpy(LayerName, ViewPortParams_p->LayerName);

    /* Check the validity of the video handle. */
    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return( ST_ERROR_INVALID_HANDLE );
    }

    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

#ifdef ST_producer
    ErrorCode = STLAYER_GetCapability(LayerName, &LayerCapability);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetCapability() failed. Error = 0x%x !", ErrorCode));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        /* Check if with the new framebuffer hold time, we need to change the decoded pictures */
        ErrorCode = AffectDecodedPicturesAndTheBuffersFreeingStrategy(Device_p, LayerCapability.FrameBufferHoldTime);
    }
#endif /* ST_producer */

    /* Boolean set to TRUE at the end of the function, to ensure that the viewport parameters
     * only get used once they are correctly initialized */
    Device_p->DeviceData_p->AreViewportParametersInitialized = FALSE;

    /* Disable slave sequencer until all viewport parameters are set */
    VIDDISP_SetSlaveDisplayUpdate(Device_p->DeviceData_p->DisplayHandle, DISABLE_SLAVE_DISPLAY_SEQUENCER);

    if(ErrorCode == ST_NO_ERROR)
    {
        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        /* Get layer associated with viewport */
        ErrorCode = OpenLayerForNewViewport(Device_p, LayerName, &VideoLayer_p);
        if (ErrorCode == ST_NO_ERROR)
        {
            /* Take first free viewport structure */
            k=0;Found = FALSE;
            do
            {
                ViewPort_p = &Device_p->DeviceData_p->ViewportProperties_p[k];
                if(ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == 0)
                {
                    Found = TRUE;
                }
                else
                {
                    k ++;
                }
                } while((k < Device_p->DeviceData_p->MaxViewportNumber) && (!Found));

            if(k < Device_p->DeviceData_p->MaxViewportNumber)
            {
                ViewPort_p->Identity.Layer_p = VideoLayer_p;

                /* Give the VTG Name to all the modules !!! To be done properly !!! */
                ErrorCode = STLAYER_GetVTGParams(VideoLayer_p->LayerHandle, &VTGParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"STLAYER_GetVTGParams failed. Error = 0x%x !", ErrorCode));
                    /* Don't fail for that ? */
                    ErrorCode = ST_NO_ERROR;
                }
                else
                {
                    /* DDTS GNBvd8800 correction : Be sure that the just opened layer is really connected */
                    /*    to a VTG (VTGName[0] != '\0'), otherwise, the current valid VTG connected to the*/
                    /*    display, ... will be removed !!!                                                */
                    if (VTGParams.VTGName[0] != '\0')
                    {
                        ErrorCode = SetVTGParams(Device_p, &VTGParams,LayerName);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"SetVTGParams() failed. Error = 0x%x !", ErrorCode));
                            /* Don't fail for that ? */
                            ErrorCode = ST_NO_ERROR;
                        }
                    }
                    /* else, nothing to do. Let the current VTG in activity. */
                }

                /* Retrieve the Layer Params because by default, the ViewPort will have    */
                /* the dimensions of its layer.                                            */
                ErrorCode = STLAYER_GetLayerParams(VideoLayer_p->LayerHandle, &LayerParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_GetLayerParams() failed. Error = 0x%x !", ErrorCode));
                    ErrorCode = CloseLayerForOldViewport(Device_p, LayerName, FALSE);
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
                    /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
                    * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum) */
                    VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &MemoryProfile);
                    if ((MemoryProfile.NbSecondaryFrameStore != 0) && (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_HDDISPO2_VIDEO2))
                    {
                        VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                            ViewPort_p->Identity.Layer_p->LayerType,
                            VIDBUFF_SECONDARY_RECONSTRUCTION);
                    }
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
                    if ( (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS))
                    {
                        VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                            ViewPort_p->Identity.Layer_p->LayerType,
                            VIDBUFF_SECONDARY_RECONSTRUCTION);
                    }

                    /* Call to STLAYER_OpenViewPort and get a ViewPortHandle from STLAYER. */
                    OpenViewPortParams.Source_p = ViewPort_p->Params.VideoViewportParameters.Source_p;

    #ifdef ST_display
                    /* See if there is a picture available on display:
                        -if yes, take its parameters
                            -if no, take default dummy parameters. When a picture comes, it will update the view port through CharacteristicsChange event */

                    ErrorCode = VIDDISP_GetDisplayedPictureInfos(Device_p->DeviceData_p->DisplayHandle, &CurrentDisplayedPictureInfos);

                    if (ErrorCode == ST_NO_ERROR)
                    {
                        IsInputOutputComputingNeeded = TRUE;

                        if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT) ||
                            (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP))
                        {
                            OpenViewPortParams.Source_p->SourceType         = STLAYER_GRAPHIC_BITMAP;
                            *(OpenViewPortParams.Source_p->Data.BitMap_p)   = CurrentDisplayedPictureInfos.BitmapParams;
                        }
                        else
                        {
                            OpenViewPortParams.Source_p->SourceType                             = STLAYER_STREAMING_VIDEO;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->SourceNumber       = Device_p->DecoderNumber;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->BitmapParams       = CurrentDisplayedPictureInfos.BitmapParams;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->ScanType           = CurrentDisplayedPictureInfos.VideoParams.ScanType;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->CompressionLevel   = STLAYER_COMPRESSION_LEVEL_NONE;  /* Layer types versus video types ! */
                        }

                        OpenViewPortParams.Source_p->Palette_p = NULL;
                        OpenViewPortParams.InputRectangle.PositionX = 0;
                        OpenViewPortParams.InputRectangle.PositionY = 0;
                        OpenViewPortParams.InputRectangle.Width = CurrentDisplayedPictureInfos.BitmapParams.Width;
                        OpenViewPortParams.InputRectangle.Height = CurrentDisplayedPictureInfos.BitmapParams.Height;
                        OpenViewPortParams.OutputRectangle.PositionX = 0;
                        OpenViewPortParams.OutputRectangle.PositionY = 0;
                        OpenViewPortParams.OutputRectangle.Width = LayerParams.Width;
                        OpenViewPortParams.OutputRectangle.Height = LayerParams.Height;
                    }
                    else
    #endif /* ST_display */
                    {   /* No picture displayed */
                        BitmapDefaultFillParams_t   BitmapDefaultFillParams;

                        IsInputOutputComputingNeeded = FALSE;
                        /* Prepare structure to get default params for the source bitmap */
                        BitmapDefaultFillParams.DeviceType      = Device_p->DeviceData_p->DeviceType;
                        BitmapDefaultFillParams.BrdCstProfile   = Device_p->DeviceData_p->StartParams.BrdCstProfile;
                        BitmapDefaultFillParams.VTGFrameRate    = Device_p->DeviceData_p->VTGFrameRate;

                        /* Fill the viewport source parameters with default values */
                        if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT) ||
                            (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP))
                        {
                            OpenViewPortParams.Source_p->SourceType = STLAYER_GRAPHIC_BITMAP;
                            /* Get default params for the source bitmap */
                            vidcom_FillBitmapWithDefaults(&BitmapDefaultFillParams, OpenViewPortParams.Source_p->Data.BitMap_p);

                            /* Initializing Bitmap Width and Height */
                            if (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP)
                            {
                                OpenViewPortParams.Source_p->Data.BitMap_p->Width = LayerParams.Width;
                                OpenViewPortParams.Source_p->Data.BitMap_p->Height = LayerParams.Height;
                            }
                        }
                        else
                        {
                            /* Get default params for the source bitmap */
                            vidcom_FillBitmapWithDefaults(&BitmapDefaultFillParams, &OpenViewPortParams.Source_p->Data.VideoStream_p->BitmapParams);
                            OpenViewPortParams.Source_p->SourceType  = STLAYER_STREAMING_VIDEO;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->SourceNumber = Device_p->DecoderNumber;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->ScanType = STGXOBJ_INTERLACED_SCAN;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->CompressionLevel = STLAYER_COMPRESSION_LEVEL_NONE;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->HorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
                            OpenViewPortParams.Source_p->Data.VideoStream_p->VerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;

                        }
                        OpenViewPortParams.Source_p->Palette_p          = NULL;
                        OpenViewPortParams.InputRectangle.PositionX     = 0;
                        OpenViewPortParams.InputRectangle.PositionY     = 0;
                        OpenViewPortParams.InputRectangle.Width         = LayerParams.Width;
                        OpenViewPortParams.InputRectangle.Height        = LayerParams.Height;
                        OpenViewPortParams.OutputRectangle.PositionX    = 0;
                        OpenViewPortParams.OutputRectangle.PositionY    = 0;
                        OpenViewPortParams.OutputRectangle.Width        = LayerParams.Width;
                        OpenViewPortParams.OutputRectangle.Height       = LayerParams.Height;
                    }

                    /* Be carefull, make sure the graphic layer does not return error because of invalid parameters for bitmap (size NULL, data NULL...) */
                    ErrorCode = STLAYER_OpenViewPort(VideoLayer_p->LayerHandle, &OpenViewPortParams, &(ViewPort_p->Identity.LayerViewPortHandle));
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"STLAYER_OpenViewPort() failed. Error = 0x%x !", ErrorCode));

                        ErrorCode = CloseLayerForOldViewport(Device_p, LayerName, FALSE);
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        /* Affectations(initialization) in the local structure ViewPort. */
                        ViewPort_p->Identity.VideoHandle        = VideoHandle;
                        ViewPort_p->Identity.Device_p           = Device_p;
                        ViewPort_p->Params.DisplayEnable        = FALSE;
                        ViewPort_p->Params.ShowingPicture       = FALSE;
                        ViewPort_p->Params.DisplayEnableDelayed = FALSE;
                        ViewPort_p->Params.InputWinAutoMode     = TRUE;
                        ViewPort_p->Params.OutputWinAutoMode    = TRUE;
                        ViewPort_p->Params.DispARConversion     = STVID_DISPLAY_AR_CONVERSION_LETTER_BOX;
                        ViewPort_p->Params.InputWinParams.Align = STVID_WIN_ALIGN_VCENTRE_HCENTRE;
                        ViewPort_p->Params.InputWinParams.Size  = STVID_WIN_SIZE_DONT_CARE;
                        ViewPort_p->Params.OutputWinParams.Align = STVID_WIN_ALIGN_VCENTRE_HCENTRE;
                        ViewPort_p->Params.OutputWinParams.Size = STVID_WIN_SIZE_DONT_CARE;
                        ViewPort_p->Params.StreamParameters.HasDisplaySizeRecommendation = FALSE;
                        ViewPort_p->Params.StreamParameters.DisplayHorizontalSize = 0;
                        ViewPort_p->Params.StreamParameters.DisplayVerticalSize   = 0;
                        ViewPort_p->Params.StreamParameters.PanVector   = 0;
                        ViewPort_p->Params.StreamParameters.ScanVector  = 0;
                        ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset = 0;
                        ViewPort_p->Params.StreamParameters.FrameCropInPixel.RightOffset = 0;
                        ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset = 0;
                        ViewPort_p->Params.StreamParameters.FrameCropInPixel.BottomOffset = 0;
                        ViewPort_p->Params.PictureSave.StreamParameters = ViewPort_p->Params.StreamParameters;
                        ViewPort_p->Params.PictureSave.LayerViewport = ViewPort_p->Params.VideoViewportParameters;
                        ViewPort_p->Params.PictureSave.WasViewPortEnabledDuringShow = FALSE;
                        ViewPort_p->Params.VideoViewportParameters.InputRectangle = OpenViewPortParams.InputRectangle;
                        ViewPort_p->Params.VideoViewportParameters.OutputRectangle = OpenViewPortParams.OutputRectangle;
                        ViewPort_p->Params.QualityOptimizations = DefaultQualityOptimizations;

                        /* The handle returned by the STVID function is a pointer on our local structure. */
                        *ViewPortHandle_p = (STVID_ViewPortHandle_t) ViewPort_p;

                        /* Now allow access to the viewport through STVID calls. */
                        ViewPort_p->Identity.ViewPortOpenedAndValidityCheck = VIDAPI_VALID_VIEWPORT_HANDLE;

                        /* Default state : disabled. */
                        ViewPort_p->Params.DisplayEnable = TRUE;/* way to force the video to disable the layer display. */
                        if((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)
                                ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG)
                                ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5525_MPEG)
                                ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5105_MPEG)
                                ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG)
                                ||(ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP))
                        {
                            /* In this case : layer is a graphic one : */
                            /* stvid_ActionDisableOutputWindow must not call STLAYER_DisableVP ( it is already disabled) */
                            ViewPort_p->Params.DisplayEnable = FALSE;
                            ErrorCode = ST_NO_ERROR;
                        }
                        ErrorCode = stvid_ActionDisableOutputWindow(ViewPort_p);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "stvid_ActionDisableOutputWindow() failed. Error = 0x%x !", ErrorCode));
                        }
                        if (IsInputOutputComputingNeeded)
                        {
                            ChangeViewportParams(ViewPort_p);
                        }
    #ifdef ST_display
                        if (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_GAMMA_GDP)
                        {
                            /* Subscribe to event from Hal display to update layer source pointer */
                            SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t) UpdateGdpLayerSrc;
                            SubscribeParams.SubscriberData_p    = (void*)(Device_p->DeviceData_p);
                             /* viewport handle of the associated LAYER (not viewport handle of the video */
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                            Device_p->DeviceName,
                                                            VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                                            &SubscribeParams);
                            if (ErrorCode == STEVT_ERROR_ALREADY_SUBSCRIBED)
                            {
                                /* Multi-viewport application. This is not an error.    */
                                ErrorCode = ST_NO_ERROR;

                            } else if (ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }
                        }  /* end type STLAYER_GAMMA_GDP */
                        else if (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)
                        {
                            /* Subscribe to event from Hal display to update layer source pointer */
                            SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t) UpdateGfxLayerSrc;
                            SubscribeParams.SubscriberData_p    = (void*)(ViewPort_p);
                                /* viewport handle of the associated LAYER (not viewport handle of the video) */
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                            Device_p->DeviceName,
                                                            VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                                            &SubscribeParams);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }
                        } /* end type digital input generic */
                        else if(  (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG)
                                ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5525_MPEG)
                                ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5105_MPEG)
                                ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG))
                        {
                            /* Subscribe to event from Hal display to update layer source pointer   */
                            SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t) UpdateBlitterDisplay;
                            SubscribeParams.SubscriberData_p    = (void*)(Device_p->DeviceData_p);
                            /* viewport handle of the associated LAYER (not viewport handle of the video */
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                            Device_p->DeviceName,
                                                            VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                                            &SubscribeParams);
                            if (ErrorCode == STEVT_ERROR_ALREADY_SUBSCRIBED)
                            {
                                /* Multi-viewport application. This is not an error.    */
                                ErrorCode = ST_NO_ERROR;

                            } else if (ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }

                            /* Subscribe to event from Hal display to update layer source pointer   */
                            SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t) UpdateBlitterWithNextPictureToBeDecoded;
                            SubscribeParams.SubscriberData_p    = (void*)(Device_p->DeviceData_p);
                            /* viewport handle of the associated LAYER (not viewport handle of the video */
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                            Device_p->DeviceName,
                                                            VIDPROD_NEW_PICTURE_PARSED_EVT,
                                                            &SubscribeParams);
                            if (ErrorCode == STEVT_ERROR_ALREADY_SUBSCRIBED)
                            {
                                /* Multi-viewport application. This is not an error.    */
                                ErrorCode = ST_NO_ERROR;


                            } else if (ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }

                            /* Subscribe to event from Hal display to update layer source pointer   */
                            SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t) UpdateBlitterCommit;
                            SubscribeParams.SubscriberData_p    = (void*)(Device_p->DeviceData_p);
                            /* viewport handle of the associated LAYER (not viewport handle of the video */
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                            Device_p->DeviceName,
                                                            VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT,
                                                            &SubscribeParams);
                            if (ErrorCode == STEVT_ERROR_ALREADY_SUBSCRIBED)
                            {
                                /* Multi-viewport application. This is not an error.    */
                                ErrorCode = ST_NO_ERROR;


                            } else if (ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }

                        }
                        else if (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5528_MPEG)

                        {
                            /* Subscribe to event from Hal display to update layer source pointer */
                            SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t) UpdateVideoDisplay;
                            SubscribeParams.SubscriberData_p    = (void*)(Device_p->DeviceData_p);
                                /* viewport handle of the associated LAYER (not viewport handle of the video) */
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                            Device_p->DeviceName,
                                                            VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                                            &SubscribeParams);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }
                        }
                        else if( (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7710_MPEG) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_H264) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_H264) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_MPEG) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_ZEUS_VC1 ) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109D_MPEG) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_H264) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_VC1) ||
                                 (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT))
                        {
                            /* Subscribe to event from Hal display to update layer source pointer */
                            SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t) UpdateVideoDisplay;
                            SubscribeParams.SubscriberData_p    = (void*)(Device_p->DeviceData_p);
                             /* viewport handle of the associated LAYER (not viewport handle of the video */
                            ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle,
                                                            Device_p->DeviceName,
                                                            VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                                            &SubscribeParams);
                            if (ErrorCode == STEVT_ERROR_ALREADY_SUBSCRIBED)
                            {
                                /* Multi-viewport application. This is not an error.    */
                                ErrorCode = ST_NO_ERROR;

                            } else if (ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_SubscribeDeviceEvent() failed. Error = 0x%x !", ErrorCode));
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                            }
                        }
    #endif /* ST_display */
                    } /* end STLAYER_OpenViewPort() succeeded */
                } /* end STLAYER_GetLayerParams() succeeded */

            } /* end found layer structure */
            else
            {
                /* No free viewport structure */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_OpenViewPort : No free handle : all the ViewPort are affected"));
                ErrorCode = ST_ERROR_NO_MEMORY;
            }
        } /* end if Get layer associated with viewport OK */
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_OpenViewPort() : problem while finding layer structure !"));
        }

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }

    /* Allow slave display sequencer to run now that all the parameters of the viewport are set. */
    if (ErrorCode == ST_NO_ERROR)
    {
        Device_p->DeviceData_p->AreViewportParametersInitialized = TRUE;
        VIDDISP_SetSlaveDisplayUpdate(Device_p->DeviceData_p->DisplayHandle, ENABLE_SLAVE_DISPLAY_SEQUENCER);
    }

    return(ErrorCode);
} /* End of STVID_OpenViewPort() function */


/*******************************************************************************
Name        : STVID_SetDisplayAspectRatioConversion
Description : Set the Display Aspect Ratio
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetDisplayAspectRatioConversion(
        const STVID_ViewPortHandle_t ViewPortHandle,
        const STVID_DisplayAspectRatioConversion_t Mode)
{
    VideoViewPort_t         *ViewPort_p;
#ifdef ST_display
    STVID_PictureInfos_t    CurrentDisplayedPictureInfos;
#endif /* ST_display */
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        switch(Mode)
        {
            case STVID_DISPLAY_AR_CONVERSION_PAN_SCAN :
            case STVID_DISPLAY_AR_CONVERSION_LETTER_BOX :
            case STVID_DISPLAY_AR_CONVERSION_COMBINED :
            case STVID_DISPLAY_AR_CONVERSION_IGNORE :

                /* Affectation of the Display Aspect Ratio Conversion stored in our    */
                /* ViewPort local structure.                                           */
                ViewPort_p->Params.DispARConversion = Mode;

#ifdef ST_display
                /* Ask display if there is a picture on display.                            */
                ErrorCode = VIDDISP_GetDisplayedPictureInfos(ViewPort_p->Identity.Device_p->DeviceData_p->DisplayHandle, &CurrentDisplayedPictureInfos);
                if (ErrorCode == ST_NO_ERROR)
                {
                    /* There's already a picture on display. The SetIOWindows has a sense. Perform it.  */

                    /* Apply the change by calling ChangeViewportParams.                   */
                    ErrorCode = ChangeViewportParams(ViewPort_p);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"ChangeViewportParams failed. Error = 0x%x !", ErrorCode));
                    }
                }
                else
                {
                    /* There's no picture on display. This function has no sense.       */
                    /* The mode have just been updated, so nothing else to store.       */
                    /* This is not an error case.   */
                    ErrorCode = ST_NO_ERROR;
                }
#endif /* ST_display */
                break;

            default :
                /* The asked Display Aspect Ratio Conversion is not recognized.        */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetDisplayAspectRatioConversion : Invalid mode"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of STVID_SetDisplayAspectRatioConversion() function */


/*******************************************************************************
Name        : STVID_SetInputWindowMode
Description : Set the Input Window Mode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetInputWindowMode(
        const STVID_ViewPortHandle_t ViewPortHandle,
        const BOOL AutoMode, const STVID_WindowParams_t * const WinParams_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    if (WinParams_p == NULL)
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (WinParams_p->Align)
    {
        case STVID_WIN_ALIGN_TOP_LEFT :
        case STVID_WIN_ALIGN_VCENTRE_LEFT :
        case STVID_WIN_ALIGN_BOTTOM_LEFT :
        case STVID_WIN_ALIGN_TOP_RIGHT :
        case STVID_WIN_ALIGN_VCENTRE_RIGHT :
        case STVID_WIN_ALIGN_BOTTOM_RIGHT :
        case STVID_WIN_ALIGN_BOTTOM_HCENTRE :
        case STVID_WIN_ALIGN_TOP_HCENTRE :
        case STVID_WIN_ALIGN_VCENTRE_HCENTRE :
            break;

        default :
            /* The asked align is not recognized. */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetInputWindowMode : Invalid align"));
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    switch (WinParams_p->Size)
    {
        case STVID_WIN_SIZE_FIXED :
        case STVID_WIN_SIZE_DONT_CARE :
        case STVID_WIN_SIZE_INCREASE :
        case STVID_WIN_SIZE_DECREASE :
            break;

        default :
            /* The asked window size is not recognized. */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetInputWindowMode : Invalid size"));
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        switch (WinParams_p->Size)
        {
            case STVID_WIN_SIZE_DONT_CARE :
                break;

            case STVID_WIN_SIZE_FIXED :
            case STVID_WIN_SIZE_INCREASE :
            case STVID_WIN_SIZE_DECREASE :
            default :
                /* The asked window size is not supported. */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetInputWindowMode : not supported."));
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Retrieve the local ViewPort structure from the ViewPortHandle. */
            ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

            /* Ensure nobody else accesses layers/viewports. */
            semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

            /* Affectation of the Input Windows Mode (automatic or not) and the        */
            /* Input Windows Parameters in our ViewPort local structure.               */
            ViewPort_p->Params.InputWinAutoMode = AutoMode;
            ViewPort_p->Params.InputWinParams = *WinParams_p;

            /* Ensure nobody else accesses layers/viewports. */
            semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
        }
    }

    return(ErrorCode);
} /* End of STVID_SetInputWindowMode() function */


/*******************************************************************************
Name        : STVID_SetIOWindows
Description : Set the aligned IO Window
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetIOWindows(
        const STVID_ViewPortHandle_t    ViewPortHandle,
        S32 const InputWinX,            S32 const InputWinY,
        U32 const InputWinWidth,        U32 const InputWinHeight,
        S32 const OutputWinX,           S32 const OutputWinY,
        U32 const OutputWinWidth,       U32 const OutputWinHeight)
{
    VideoViewPort_t       *ViewPort_p;
#ifdef ST_display
    STVID_PictureInfos_t  CurrentDisplayedPictureInfos;
#endif /* ST_display */
    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

#if defined (ST_diginput) && defined (ST_MasterDigInput)
        if((ViewPort_p->Identity.Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT) ||
           (ViewPort_p->Identity.Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT))
        {
            return (stvid_DvpSetIOWindows(ViewPortHandle,
                                        InputWinX, InputWinY,
                                        InputWinWidth, InputWinHeight,
                                        OutputWinX, OutputWinY,
                                        OutputWinWidth, OutputWinHeight));
        }
#endif /* #ifdef ST_diginput && ST_MasterDigInput */

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

#ifdef ST_display
        /* Ask display if there is a picture on display.                            */
        ErrorCode = VIDDISP_GetDisplayedPictureInfos(ViewPort_p->Identity.Device_p->DeviceData_p->DisplayHandle, &CurrentDisplayedPictureInfos);
        if (ErrorCode == ST_NO_ERROR)
        {
            /* There's already a picture on display. The SetIOWindows has a sense. Perform it.  */
            /* Need 1/16 pixel unit to be used */
            ActionSetIOWindows(ViewPort_p, InputWinX << 4, InputWinY << 4, InputWinWidth, InputWinHeight, OutputWinX, OutputWinY, OutputWinWidth, OutputWinHeight);
        }
        else
#endif /* ST_display */
        {
            /* There's no picture on display. The SetIOWindows has no sense.    */
            /* Just store the viewport parameters, and they'll be applied when  */
            /* the very first picture will be displayed.                        */
            ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionX   = InputWinX * 16;
            ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionY   = InputWinY * 16;
            ViewPort_p->Params.VideoViewportParameters.InputRectangle.Width       = InputWinWidth;
            ViewPort_p->Params.VideoViewportParameters.InputRectangle.Height      = InputWinHeight;
            ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionX  = OutputWinX;
            ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionY  = OutputWinY;
            ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Width      = OutputWinWidth;
            ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Height     = OutputWinHeight;

            /* This is not an error case.   */
            ErrorCode = ST_NO_ERROR;
        }

        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of STVID_SetIOWindows() function */


/*******************************************************************************
Name        : STVID_SetOutputWindowMode
Description : Set the Output Window Mode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetOutputWindowMode(
        const STVID_ViewPortHandle_t ViewPortHandle,
        const BOOL AutoMode, const STVID_WindowParams_t * const WinParams_p)
{
  VideoViewPort_t   *ViewPort_p;
  ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    if (WinParams_p == NULL)
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (WinParams_p->Align)
    {
        case STVID_WIN_ALIGN_TOP_LEFT :
        case STVID_WIN_ALIGN_VCENTRE_LEFT :
        case STVID_WIN_ALIGN_BOTTOM_LEFT :
        case STVID_WIN_ALIGN_TOP_RIGHT :
        case STVID_WIN_ALIGN_VCENTRE_RIGHT :
        case STVID_WIN_ALIGN_BOTTOM_RIGHT :
        case STVID_WIN_ALIGN_BOTTOM_HCENTRE :
        case STVID_WIN_ALIGN_TOP_HCENTRE :
        case STVID_WIN_ALIGN_VCENTRE_HCENTRE :
            break;

        default :
            /* The asked align is not recognized. */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetInputWindowMode : Invalid align"));
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    switch (WinParams_p->Size)
    {
        case STVID_WIN_SIZE_FIXED :
        case STVID_WIN_SIZE_DONT_CARE :
        case STVID_WIN_SIZE_INCREASE :
        case STVID_WIN_SIZE_DECREASE :
            break;

        default :
            /* The asked window size is not recognized. */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetInputWindowMode : Invalid size"));
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        switch (WinParams_p->Size)
        {
            case STVID_WIN_SIZE_DONT_CARE :
                break;

            case STVID_WIN_SIZE_FIXED :
            case STVID_WIN_SIZE_INCREASE :
            case STVID_WIN_SIZE_DECREASE :
            default :
                /* The asked window size is not supported. */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetInputWindowMode : not supported."));
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
            ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

            /* Ensure nobody else accesses layers/viewports. */
            semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

            /* Affectation of the Output Windows Mode (automatic or not) and the        */
            /* Output Windows Parameters in our ViewPort local structure.               */
            ViewPort_p->Params.OutputWinAutoMode = AutoMode;
            ViewPort_p->Params.OutputWinParams = *WinParams_p;

            /* Ensure nobody else accesses layers/viewports. */
            semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
        }
    }

    return(ErrorCode);
} /* End of STVID_SetOutputWindowMode() function */


/*******************************************************************************
Name        : STVID_SetViewPortAlpha
Description : Set global alpha of the Viewport
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortAlpha(const STVID_ViewPortHandle_t ViewPortHandle,
        const STLAYER_GlobalAlpha_t * const GlobalAlpha_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    const STLAYER_GlobalAlpha_t*  New_GlobalAlpha_p = (const STLAYER_GlobalAlpha_t*) GlobalAlpha_p;

    /* Exit now if parameters are bad */
    if (GlobalAlpha_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_SetViewPortAlpha(ViewPort_p->Identity.LayerViewPortHandle,
                CAST_CONST_HANDLE_TO_DATA_TYPE(STLAYER_GlobalAlpha_t * , New_GlobalAlpha_p));

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetViewPortAlpha() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_SetViewPortAlpha() function */


/*******************************************************************************
Name        : STVID_SetViewPortColorKey
Description : Set the Viewport Color Key
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortColorKey(const STVID_ViewPortHandle_t ViewPortHandle, const STGXOBJ_ColorKey_t * const ColorKey_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    const STGXOBJ_ColorKey_t*  New_ColorKey_p = (const STGXOBJ_ColorKey_t*) ColorKey_p;

    /* Exit now if parameters are bad */
    if (ColorKey_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_SetViewPortColorKey(ViewPort_p->Identity.LayerViewPortHandle,
                CAST_CONST_HANDLE_TO_DATA_TYPE(STGXOBJ_ColorKey_t * , New_ColorKey_p));

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetViewPortColorKey() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_SetViewPortColorKey() function */


/*******************************************************************************
Name        : STVID_SetViewPortPSI
Description : Set the Viewport PSI
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortPSI(const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_PSI_t * const VPPSI_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    const STLAYER_PSI_t*  New_VPPSI_p = (const STLAYER_PSI_t*) VPPSI_p;

    /* Exit now if parameters are bad */
    if (VPPSI_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_SetViewPortPSI(ViewPort_p->Identity.LayerViewPortHandle,
                CAST_CONST_HANDLE_TO_DATA_TYPE(STLAYER_PSI_t* , New_VPPSI_p));

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetViewPortPSI() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_SetViewPortPSI() function */

#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
/*******************************************************************************
Name        : STVID_GetVideoDisplayParams
Description : This function retreive params used for VHSRC.
Parameters  : VPHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated VHSRCParams data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetVideoDisplayParams(const STVID_ViewPortHandle_t ViewPortHandle,
                                  STLAYER_VideoDisplayParams_t * Params_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_GetVideoDisplayParams(ViewPort_p->Identity.LayerViewPortHandle, Params_p);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetVHSRCParams() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
}
#endif /* STVID_DEBUG_GET_DISPLAY_PARAMS */

/*******************************************************************************
Name        : STVID_SetViewPortSpecialMode
Description : Set special mode parameters.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortSpecialMode (const STVID_ViewPortHandle_t ViewPortHandle,
        const STLAYER_OutputMode_t OuputMode,
        const STLAYER_OutputWindowSpecialModeParams_t * const Params_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    const STLAYER_OutputWindowSpecialModeParams_t*  New_Params_p = (const STLAYER_OutputWindowSpecialModeParams_t*) Params_p;

    /* Exit now if parameters are bad */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_SetViewPortSpecialMode(ViewPort_p->Identity.LayerViewPortHandle,
                (STLAYER_OutputMode_t) OuputMode,
                CAST_CONST_HANDLE_TO_DATA_TYPE(STLAYER_OutputWindowSpecialModeParams_t *, New_Params_p));

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetViewportSpecialMode() failed. Error = 0x%x !", ErrorCode));
        }
    }
    return(ErrorCode);

} /* End of STVID_SetViewPortSpecialMode() function. */

/*******************************************************************************
Name        : STVID_ShowPicture
Description : Show the picture given as parameter.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_ShowPicture(
        const STVID_ViewPortHandle_t    ViewPortHandle,
        STVID_PictureParams_t           * const Params_p,
        const STVID_Freeze_t            * const Freeze_p)
{
#ifdef ST_display
    VideoViewPort_t *                       ViewPort_p;
    VideoDevice_t *                         Device_p;
    STVID_PictureInfos_t *                  PictureInfos_p = NULL;
    VIDBUFF_PictureBuffer_t                 Picture;
    VIDBUFF_FrameBuffer_t                   FrameBuffer;
    VideoStatus_t                           Status;
    STLAYER_Layer_t                         LayerType;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    VideoLayer_t *                          Layer_p;
    U32                                     Layer;
#endif /* ST_display */
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    if ((Params_p == NULL) || (Freeze_p == NULL))
    {
        /* Bad parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
#ifdef ST_display
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle. */
        ViewPort_p  = (VideoViewPort_t *) ViewPortHandle;
        Device_p    = ViewPort_p->Identity.Device_p;
        LayerType   = Device_p->DeviceData_p->LayerProperties_p[0].LayerType;
        VirtualMapping = Device_p->DeviceData_p->VirtualMapping;

        Layer_p     = ViewPort_p->Identity.Layer_p;
        Layer = 0;

        /* In the case of display on auxiliary and secondary (specific 7200) */
        if (( Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
              Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
              (Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
              Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ) )
        {
            if (( Device_p->DeviceData_p->LayerProperties_p[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
              Device_p->DeviceData_p->LayerProperties_p[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ))
            {
                if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                    Layer = 0;
                else if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                    Layer = 1;
            } else
            {
                if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                    Layer = 1;
                else if (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                    Layer = 0;
            }
        } else
        {
            if ( (Layer_p->LayerType == STLAYER_7020_VIDEO1)
              || (Layer_p->LayerType == STLAYER_SDDISPO2_VIDEO1)
              || (Layer_p->LayerType == STLAYER_HDDISPO2_VIDEO1)
              || (Layer_p->LayerType == STLAYER_OMEGA2_VIDEO1)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO1)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO2)
              || (Layer_p->LayerType == STLAYER_GAMMA_GDP))
            {
                Layer = 0;
            }
            if ( (Layer_p->LayerType == STLAYER_7020_VIDEO2)
              || (Layer_p->LayerType == STLAYER_SDDISPO2_VIDEO2)
              || (Layer_p->LayerType == STLAYER_HDDISPO2_VIDEO2)
              || (Layer_p->LayerType == STLAYER_OMEGA2_VIDEO2)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
              || (Layer_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4))
            {
                Layer = 1;
            }
        }
        /* Enter in critical section to avoid multi access of the DEVICE's properties at the same time.*/
        stvid_EnterCriticalSection(Device_p);
        /* Ensure nobody else accesses layers/viewports. */
        semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

        /* Check if the decoder is stopped. */
        Status = GET_VIDEO_STATUS(Device_p);
        if (Status != VIDEO_STATUS_STOPPED)
        {
            ErrorCode = STVID_ERROR_DECODER_RUNNING;
        }
        else
        {
            /* First time we gonna show a picture => save current global structure to  */
            /* allow go back to previous state with the STVID_Hide function.           */
            if (!(ViewPort_p->Params.ShowingPicture))
            {
                /* Get current parameters of displayed picture. */
                ErrorCode = VIDDISP_GetDisplayedPictureExtendedInfos(Device_p->DeviceData_p->DisplayHandle, &Picture);
                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Save current viewport informations.  */
                    ViewPort_p->Params.PictureSave.StreamParameters = ViewPort_p->Params.StreamParameters;
                    ViewPort_p->Params.PictureSave.LayerViewport    = ViewPort_p->Params.VideoViewportParameters;
                    ViewPort_p->Params.PictureSave.VideoStreaming   = (*(ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p));

                    /* Save viewport's source informations. */
                    ViewPort_p->Params.PictureSave.PictureBuffer = Picture;
                    ViewPort_p->Params.PictureSave.FrameBuffer   = *Picture.FrameBuffer_p;
                }
                ErrorCode = VIDDISP_GetReconstructionMode(Device_p->DeviceData_p->DisplayHandle, LayerType,
                            &ViewPort_p->Params.PictureSave.ReconstructionMode);
            }

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Remember that we are showing a picture, in order to remove it when STVID_HidePicture is called */
                ViewPort_p->Params.ShowingPicture = TRUE;
                /* Setup the display without decimation (STVID_DECIMATION_FACTOR_NONE for Horizontal and Vertical). */
                ErrorCode = VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle, LayerType, VIDBUFF_MAIN_RECONSTRUCTION);
                if (ErrorCode == ST_NO_ERROR)
                {
                    PictureInfos_p = &(Picture.ParsedPictureInformation.PictureGenericData.PictureInfos);
                    PictureInfos_p->BitmapParams.Width    = Params_p->Width;
                    PictureInfos_p->BitmapParams.Height   = Params_p->Height;
                    PictureInfos_p->VideoParams.ScanType =
                            (Params_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE ?
                            STGXOBJ_PROGRESSIVE_SCAN :
                            STGXOBJ_INTERLACED_SCAN);
                    Picture.FrameBuffer_p = &FrameBuffer;
                    if (Params_p->ColorType == STVID_COLOR_TYPE_YUV422)
                    {
                        Picture.FrameBuffer_p->Allocation.BufferType = VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL;
                        if (Params_p->pChromaOffset == 0)
                        {
                            /* Impossible, error recovery case */
                            Params_p->pChromaOffset = Params_p->Size / 2;
                        }
                    }
                    else if (Params_p->ColorType == STVID_COLOR_TYPE_YUV444)
                    {
                        /* ??? not sure of the size in 4:4:4, no chip has that for the moment ! */
                        Picture.FrameBuffer_p->Allocation.BufferType = VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL;
                        if (Params_p->pChromaOffset == 0)
                        {
                            /* Impossible, error recovery case */
                            Params_p->pChromaOffset = Params_p->Size / 2;
                        }
                    }
                    else
                    {
                        /* Generally decoders output 4:2:0 */
                        Picture.FrameBuffer_p->Allocation.BufferType = VIDBUFF_BUFFER_FRAME_12BITS_PER_PIXEL;
                        if (Params_p->pChromaOffset == 0)
                        {
                            /* Impossible, error recovery case */
                            Params_p->pChromaOffset = (2 * Params_p->Size) / 3;
                        }
                    }
                    Picture.FrameBuffer_p->Allocation.TotalSize = Params_p->Size;
                    Picture.FrameBuffer_p->Allocation.Size2 = Params_p->Size - Params_p->pChromaOffset;
                    Picture.FrameBuffer_p->Allocation.AllocationMode = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY;
                    Picture.FrameBuffer_p->Allocation.AvmemBlockHandle = (STAVMEM_BlockHandle_t) 0;
                    Picture.FrameBuffer_p->AllocationIsDone = TRUE;
                    Picture.FrameBuffer_p->AvailableReconstructionMode = VIDBUFF_MAIN_RECONSTRUCTION;
                    Picture.FrameBuffer_p->NextAllocated_p = NULL;
                    Picture.FrameBuffer_p->FrameOrFirstFieldPicture_p = &Picture;
                    Picture.FrameBuffer_p->NothingOrSecondFieldPicture_p = NULL;
                    Picture.FrameBuffer_p->CompressionLevel  = STVID_COMPRESSION_LEVEL_NONE;   /* No compression supported           */
                    /* Convert addresses according to STAVMEM mapping :                     */
                    /*  Params_p->Data is a virtual address, so convert it to device one.   */
                    Picture.FrameBuffer_p->Allocation.Address_p = VirtualToDevice(Params_p->Data, VirtualMapping);
                    Picture.FrameBuffer_p->Allocation.Address2_p = VirtualToDevice( ((U32)Params_p->Data + Params_p->pChromaOffset), VirtualMapping);

                    /* Update the Stream parameters with parameters of the picture. Need by    */
                    /* the STVID_SetIOWindows function.                                        */
                    ViewPort_p->Params.StreamParameters.HasDisplaySizeRecommendation = FALSE;
                    if (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)
                    {
                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Width  = Params_p->Width;
                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Height = Params_p->Height;
                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Pitch  = Params_p->Width;

                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->ScanType =
                                (Params_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE ?
                                STGXOBJ_PROGRESSIVE_SCAN :
                                STGXOBJ_INTERLACED_SCAN );
                        /* Set non decimation picture.  */
                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->HorizontalDecimationFactor
                                = STLAYER_DECIMATION_FACTOR_NONE;
                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->VerticalDecimationFactor
                                = STLAYER_DECIMATION_FACTOR_NONE;
                    }
                    else
                    {
                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Width = Params_p->Width;
                        ViewPort_p->Params.VideoViewportParameters.Source_p->Data.BitMap_p->Height = Params_p->Height;
                    }
                    ViewPort_p->Params.StreamParameters.PanVector = 0;
                    ViewPort_p->Params.StreamParameters.ScanVector = 0;

                    ErrorCode = VIDDISP_ShowPicture(Device_p->DeviceData_p->DisplayHandle, &Picture, Layer,VIDDISP_SHOW_PICTURE);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDDISP_ShowPicture failed. Error = 0x%x !", ErrorCode));
                        ErrorCode = STVID_ERROR_NOT_AVAILABLE;
                    }
                    else
                    {
                        /* Call STVID_SetIOWindows with the SetIOWindows parameters stored during  */
                        /* the latest call to STVID_SetIOWindows.                                  */
                        ActionSetIOWindows(ViewPort_p,
                                        ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionX,
                                        ViewPort_p->Params.VideoViewportParameters.InputRectangle.PositionY,
                                        ViewPort_p->Params.VideoViewportParameters.InputRectangle.Width,
                                        ViewPort_p->Params.VideoViewportParameters.InputRectangle.Height,
                                        ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionX,
                                        ViewPort_p->Params.VideoViewportParameters.OutputRectangle.PositionY,
                                        ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Width,
                                        ViewPort_p->Params.VideoViewportParameters.OutputRectangle.Height);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ActionSetIOWindows() failed. Error = 0x%x !", ErrorCode));
                            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
                        }
                        else
                        {
                            /* Now, check if viewport is enable or not.                                 */
                            /* If the Video viewport is enabled and delayed, then we can call           */
                            /* STLAYER_EnableViewPort in order to enable it.                            */
                            if (ViewPort_p->Params.DisplayEnableDelayed)
                            {
                                /* Viewport is opened but with no picture in display queue. If we want  */
                                /* to se the just showed picture, we must enable it.                    */
                                ErrorCode = STLAYER_EnableViewPort(ViewPort_p->Identity.LayerViewPortHandle);
                                if (ErrorCode != ST_NO_ERROR)
                                {
                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableViewPort() failed. Error = 0x%x !", ErrorCode));
                                    /* No error allowed for this function ! */
                                    ErrorCode = ST_NO_ERROR;
                                }
                                else
                                {
                                    /* Remember STVID_ShowPicture() enabled the viewport.   */
                                    ViewPort_p->Params.PictureSave.WasViewPortEnabledDuringShow = TRUE;
                                }
                            }
                        }
                    }   /* end of VIDDISP_ShowPicture */
                }  /* end of VIDDISP_SetReconstructionMode */
            }  /* end of VIDDISP_GetReconstructionMode */
        }
        /* Ensure nobody else accesses layers/viewports. */
        semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
        stvid_LeaveCriticalSection(Device_p);
    }
#else /* ST_display */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_display */

    return(ErrorCode);
} /* End of STVID_ShowPicture() function */

#ifdef ST_XVP_ENABLE_FLEXVP
/*******************************************************************************
Name        : STVID_ActivateXVPProcess
Description : Activate a FlexVP's Process.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_ActivateXVPProcess(    const STVID_ViewPortHandle_t ViewPortHandle,
                                            const STLAYER_ProcessId_t ProcessID)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t          ErrorCodeTmp;
    VideoViewPort_t         *ViewPort_p;
    VideoDevice_t           *Device_p;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;
        Device_p = ViewPort_p->Identity.Device_p;

        switch (ProcessID)
        {
#ifdef ST_XVP_ENABLE_FGT
        case STLAYER_PROCESSID_FGT :
            {
                VIDCOM_InternalProfile_t        InternalProfile;
                VIDDISP_DisplayState_t          DisplayState;
                /* get memory profile : FGT activation is forbidden if DECIMATION is activated */
                ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &InternalProfile);

                if ((ErrorCode == ST_NO_ERROR) &&
                    (Device_p->DeviceData_p->FGTHandleIsValid == TRUE))
                {
                    /* get display state : FGT activation is forbidden if DISPLAY is not frozen */
                    VIDDISP_GetState(Device_p->DeviceData_p->DisplayHandle, &DisplayState);

                    if (    (InternalProfile.ApplicableDecimationFactor == STVID_DECIMATION_FACTOR_NONE) &&
                            (DisplayState != VIDDISP_DISPLAY_STATE_DISPLAYING))
                    {
                        ErrorCode = VIDFGT_Activate(Device_p->DeviceData_p->FGTHandle);

                        if (    (ErrorCode == ST_NO_ERROR) ||
                                (ErrorCode == ST_ERROR_ALREADY_INITIALIZED))
                        {
                            /* init of LAYERFGT */
                            ErrorCodeTmp = STLAYER_XVPInit(ViewPort_p->Identity.LayerViewPortHandle, ProcessID);
                        }

                        if (    (ErrorCode != ST_ERROR_ALREADY_INITIALIZED) ||
                                (ErrorCodeTmp != ST_NO_ERROR))
                            ErrorCode = ErrorCodeTmp;
                    }
                    else ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
            }
            break;
#endif /* ST_XVP_ENABLE_FGT */

        default :
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVID_DeactivateXVPProcess
Description : Deactivate a FlexVP's Process.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_DeactivateXVPProcess(  const STVID_ViewPortHandle_t ViewPortHandle,
                                            const STLAYER_ProcessId_t ProcessID)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    VideoViewPort_t         *ViewPort_p;
    VideoDevice_t           *Device_p;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;
        Device_p = ViewPort_p->Identity.Device_p;

        switch (ProcessID)
        {
#ifdef ST_XVP_ENABLE_FGT
        case STLAYER_PROCESSID_FGT :
            {
                VIDDISP_DisplayState_t  DisplayState;
                VIDDISP_GetState(Device_p->DeviceData_p->DisplayHandle, &DisplayState);

                if (    (Device_p->DeviceData_p->FGTHandleIsValid == TRUE) &&
                        (DisplayState != VIDDISP_DISPLAY_STATE_DISPLAYING))
                {
                    /* !!! VIDFGT_Deactivate is not called here to preserve the pattern database !!! */
                    /* VIDFGT_Deactivate is done during STVID_Term */

                    /* init of LAYERFGT */
                    ErrorCode = STLAYER_XVPTerm(ViewPort_p->Identity.LayerViewPortHandle);
                }
                else ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            break;
#endif /* ST_XVP_ENABLE_FGT */

        default :
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVID_ShowXVPProcess
Description : Enable / Disable a FlexVP's Process.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_ShowXVPProcess(     const STVID_ViewPortHandle_t ViewPortHandle,
                                         const STLAYER_ProcessId_t ProcessID)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STLAYER_XVPExtraParam_t     ExtraParam;
    VideoViewPort_t             *ViewPort_p;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        switch (ProcessID)
        {
#ifdef ST_XVP_ENABLE_FGT
        case STLAYER_PROCESSID_FGT :
            ExtraParam.FGTExtraParams.Type  = STLAYER_FGT_EXTRAPARAM_STATE;
            ExtraParam.FGTExtraParams.State = STLAYER_FGT_ENABLED_STATE;
            ErrorCode = STLAYER_XVPSetExtraParam(ViewPort_p->Identity.LayerViewPortHandle, &ExtraParam);
            break;
#endif /* ST_XVP_ENABLE_FGT */

        default :
            break;
        }
    }
    return ErrorCode;
}

/*******************************************************************************
Name        : STVID_HideXVPProcess
Description : Enable / Disable a FlexVP's Process.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_HideXVPProcess(     const STVID_ViewPortHandle_t ViewPortHandle,
                                         const STLAYER_ProcessId_t ProcessID)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STLAYER_XVPExtraParam_t     ExtraParam;
    VideoViewPort_t             *ViewPort_p;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        switch (ProcessID)
        {
#ifdef ST_XVP_ENABLE_FGT
        case STLAYER_PROCESSID_FGT :
            ExtraParam.FGTExtraParams.Type  = STLAYER_FGT_EXTRAPARAM_STATE;
            ExtraParam.FGTExtraParams.State = STLAYER_FGT_DISABLED_STATE;
            ErrorCode = STLAYER_XVPSetExtraParam(ViewPort_p->Identity.LayerViewPortHandle, &ExtraParam);
            break;
#endif /* ST_XVP_ENABLE_FGT */

        default :
            break;
        }
    }
    return ErrorCode;
}
#endif /* ST_XVP_ENABLE_FLEXVP */


/*******************************************************************************
Name        : STVID_SetViewPortQualityOptimizations
Description : This function set the quality optimizations used in viewport.
Parameters  : VPHandle IN: Handle of the viewport, Params_p IN: Pointer
              to an allocated STLAYER_QualityOptimizations_t data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetViewPortQualityOptimizations(const STVID_ViewPortHandle_t ViewPortHandle,
                                                     const STLAYER_QualityOptimizations_t * Params_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ViewPort_p->Params.QualityOptimizations = *Params_p;
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_SetViewPortQualityOptimizations
Description : This function retreive the quality optimizations used in viewport.
Parameters  : VPHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated STLAYER_QualityOptimizations_t data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetViewPortQualityOptimizations(const STVID_ViewPortHandle_t ViewPortHandle,
                                                     STLAYER_QualityOptimizations_t * const Params_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        *Params_p = ViewPort_p->Params.QualityOptimizations;
    }
    return(ErrorCode);
}

#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
/*******************************************************************************
Name        : STVID_GetRequestedDeinterlacingMode
Description : This function can be used to know the currently requested Deinterlacing mode.
Parameters  : The Requested mode can be one of the following values:
              0: Off (nothing displayed)
              1: Reserved for future use
              2: Bypass
              3: Vertical Intperolation
              4: Directional Intperolation
              5: Field Merging
              6: Median Filter
              7: MLD Mode (=3D Mode) = Default value
              8: LMU Mode
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetRequestedDeinterlacingMode(const STVID_ViewPortHandle_t ViewPortHandle,
        STLAYER_DeiMode_t * const RequestedMode_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_GetRequestedDeinterlacingMode(ViewPort_p->Identity.LayerViewPortHandle, RequestedMode_p);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_GetRequestedDeinterlacingMode() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_GetRequestedDeinterlacingMode() function */

/*******************************************************************************
Name        : STVID_SetRequestedDeinterlacingMode
Description : This function can be used to request a Deinterlacing mode.

              This function is provided FOR TEST PURPOSE ONLY. By default the requested
              DEI mode should not be changed.

Parameters  : The Requested mode can be one of the following values:
              0: Off (nothing displayed)
              1: Reserved for future use
              2: Bypass
              3: Vertical Intperolation
              4: Directional Intperolation
              5: Field Merging
              6: Median Filter
              7: MLD Mode (=3D Mode) = Default value
              8: LMU Mode
Assumptions :
Limitations : The requested mode will be applied only if the necessary conditions are met
              (for example the availability of the Previous and Current fields)
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetRequestedDeinterlacingMode(const STVID_ViewPortHandle_t ViewPortHandle,
        const STLAYER_DeiMode_t RequestedMode)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_SetRequestedDeinterlacingMode(ViewPort_p->Identity.LayerViewPortHandle, RequestedMode);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetRequestedDeinterlacingMode() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_SetRequestedDeinterlacingMode() function */
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */

#ifdef STVID_USE_FMD
/*******************************************************************************
Name        : STVID_SetFMDParams
Description : This function set params used for FMD.
Parameters  : VPHandle IN: Handle of the viewport, Params_p IN: Pointer
              to an allocated FMDParams data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetFMDParams(const STVID_ViewPortHandle_t ViewPortHandle,
                                  const STLAYER_FMDParams_t * Params_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_SetFMDParams(ViewPort_p->Identity.LayerViewPortHandle, Params_p);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetFMDParams() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetFMDParams
Description : This function retreive params used for FMD.
Parameters  : VPHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated FMDParams data type
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetFMDParams(const STVID_ViewPortHandle_t ViewPortHandle,
                                  STLAYER_FMDParams_t * Params_p)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_GetFMDParams(ViewPort_p->Identity.LayerViewPortHandle, Params_p);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_SetFMDParams() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_EnableFMDReporting
Description : This function can be used to enable the FMD reporting.
Parameters  : VPHandle IN: Handle of the viewport
Assumptions :
Limitations : FMD is working only if DEI is enabled
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_EnableFMDReporting(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_EnableFMDReporting(ViewPort_p->Identity.LayerViewPortHandle);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableFMDReporting() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_EnableFMDReporting() function */

/*******************************************************************************
Name        : STVID_DisableFMDReporting
Description : This function can be used to disable the FMD reporting.
Parameters  : VPHandle IN: Handle of the viewport
Assumptions :
Limitations : FMD is working only if DEI is enabled
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_DisableFMDReporting(const STVID_ViewPortHandle_t ViewPortHandle)
{
    VideoViewPort_t   *ViewPort_p;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;

    /* Check the ViewPort Handle is valid. */
    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;

        ErrorCode = STLAYER_DisableFMDReporting(ViewPort_p->Identity.LayerViewPortHandle);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STLAYER_EnableFMDReporting() failed. Error = 0x%x !", ErrorCode));
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
    return(ErrorCode);
} /* End of STVID_DisableFMDReporting() function */

#endif

/*******************************************************************************
Name        : vidcomp_CheckAndAdjustManualInputParams
Description : Limit the input window within the cropped window source if applicable
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vidcomp_CheckAndAdjustManualInputParams(
        STGXOBJ_Rectangle_t     * const InputRectangle_p,
        VideoViewPort_t         * const ViewPort_p)
{
    U32     SourceWidth, SourceHeight; /* in pixels */
    U32     CroppedSourceWidth, CroppedSourceHeight;  /* in pixels */

    SourceWidth = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Width;
    SourceHeight = ViewPort_p->Params.VideoViewportParameters.Source_p->Data.VideoStream_p->BitmapParams.Height;

    CroppedSourceWidth = SourceWidth - ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset -
            ViewPort_p->Params.StreamParameters.FrameCropInPixel.RightOffset;
    CroppedSourceHeight = SourceHeight - ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset -
            ViewPort_p->Params.StreamParameters.FrameCropInPixel.BottomOffset;

    if (InputRectangle_p->Width > CroppedSourceWidth)
    {
        InputRectangle_p->Width = CroppedSourceWidth;
    }
    if (InputRectangle_p->Height > CroppedSourceHeight)
    {
        InputRectangle_p->Height = CroppedSourceHeight;
    }

    if (InputRectangle_p->PositionX < (S32)(16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset))
    {
        InputRectangle_p->PositionX = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset;
    }
    if (InputRectangle_p->PositionY < (S32)(16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset))
    {
        InputRectangle_p->PositionY = 16 * ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset;
    }

    /* Check if the Input rectangle width fits in the cropped source */
    if ((InputRectangle_p->PositionX / 16 + InputRectangle_p->Width) >
        (ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset + CroppedSourceWidth) )
    {
        /* The input rectangle width exceeds the size of the source picture: It should be truncated. */
        if ( ( (U32) InputRectangle_p->PositionX / 16) > (ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset + CroppedSourceWidth) )
        {
            /* PositionX is over the cropped picture width */
            InputRectangle_p->Width = 0;
        }
        else
        {
            InputRectangle_p->Width = ViewPort_p->Params.StreamParameters.FrameCropInPixel.LeftOffset + CroppedSourceWidth
                        - (InputRectangle_p->PositionX / 16);
        }
    }

    /* Check if the Input rectangle height fits in the cropped source */
    if ((InputRectangle_p->PositionY / 16 + InputRectangle_p->Height) >
        (ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset + CroppedSourceHeight) )
    {
        /* The input rectangle height exceeds the size of the source picture: It should be truncated. */
        if ( ( (U32) InputRectangle_p->PositionY / 16) > (ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset + CroppedSourceHeight) )
        {
            /* PositionY is over the cropped picture height */
            InputRectangle_p->Height = 0;
        }
        else
        {
            InputRectangle_p->Height = ViewPort_p->Params.StreamParameters.FrameCropInPixel.TopOffset + CroppedSourceHeight
                                    - (InputRectangle_p->PositionY / 16);
        }
    }

} /* End of vidcomp_CheckAndAdjustManualInputParams() function */

#ifdef ST_producer
/*******************************************************************************
Name        : AffectDecodedPicturesAndTheBuffersFreeingStrategy
Description : Tries to change the decoded pictures according to a new
              framebuffer hold time
Parameters  : A pointer on a device, the framebuffer hold time
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if the decoded pictures was changed successfully
*******************************************************************************/
static ST_ErrorCode_t AffectDecodedPicturesAndTheBuffersFreeingStrategy(VideoDevice_t * const Device_p, const U8 FrameBufferHoldTime)
{
    VIDCOM_InternalProfile_t                MemoryProfile;
    STVID_DecodedPictures_t                 DecodedPictures;
    VIDBUFF_StrategyForTheBuffersFreeing_t  StrategyForTheBuffersFreeing;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    stvid_EnterCriticalSection(Device_p);
    /* Check if with the new framebuffer hold time, we need to change the decoded pictures */

    /* Get the momory profile */
    ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &MemoryProfile);
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"VIDBUFF_GetMemoryProfile() failed. Error = 0x%x !", ErrorCode));
    }
    else
    {
        if (Device_p->DeviceData_p->ProducerHandleIsValid)
        {
            /* Get the decoded pictures */
            ErrorCode = VIDPROD_GetDecodedPictures(Device_p->DeviceData_p->ProducerHandle, &DecodedPictures);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetDecodedPictures() failed. Error = 0x%x !", ErrorCode));
            }
            else
            {
                /* Change the decoded pictures according to the new framebuffer hold time */
                ErrorCode = AffectDecodedPictures(Device_p,
                                                &MemoryProfile.ApiProfile,
                                                FrameBufferHoldTime,
                                                &DecodedPictures);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AffectDecodedPictures() failed. Error = 0x%x !", ErrorCode));
                }
                else
                {
                    /* Set again the buffers freeing strategy, if the decoded pictures has changed */
                    AffectTheBuffersFreeingStrategy(
                            Device_p->DeviceData_p->DeviceType,
                            MemoryProfile.NbMainFrameStore,
                            DecodedPictures,
                            &StrategyForTheBuffersFreeing);

                    /* Inform buffer which needs to know it to choose the buffers freeing strategy. */
                    ErrorCode = VIDBUFF_SetStrategyForTheBuffersFreeing(Device_p->DeviceData_p->BuffersHandle,
                            &StrategyForTheBuffersFreeing);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_SetStrategyForTheBuffersFreeing failed"));
                    }
                } /* End of AffectDecodedPictures is OK */
            } /* End of producer handle is valid */
        } /* End of producer handle is valid */
    }
    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of function AffectDecodedPicturesAndTheBuffersFreeing() */
#endif /* ST_producer */

#if 1  /*wy add*/
ST_ErrorCode_t STVID_GetDisplayedPictureInfos(const STVID_ViewPortHandle_t ViewPortHandle,STVID_PictureInfos_t *PictureInfos)
{
    VideoViewPort_t       *ViewPort_p;
   ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

    if (!(IsValidViewPortHandle(ViewPortHandle)))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        /* Retrieve the local ViewPort structure from the ViewPortHandle.          */
        ViewPort_p = (VideoViewPort_t *) ViewPortHandle;
        semaphore_wait(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
	ErrorCode = VIDDISP_GetDisplayedPictureInfos(ViewPort_p->Identity.Device_p->DeviceData_p->DisplayHandle, PictureInfos);
        semaphore_signal(ViewPort_p->Identity.Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
    }
	return ErrorCode;
}
#endif

ST_ErrorCode_t	STVID_ComputeOutWindow(const STVID_ViewPortHandle_t ViewPortHandle, STGXOBJ_Rectangle_t *OutRec)
{
	VideoViewPort_t       *ViewPort_p;
	ST_ErrorCode_t  ErrorCode;

	if (!(IsValidViewPortHandle(ViewPortHandle)))
	{
		ErrorCode = ST_ERROR_INVALID_HANDLE;
	}
	else
	{
		/* Retrieve the local ViewPort structure from the ViewPortHandle.          */
		ViewPort_p = (VideoViewPort_t *) ViewPortHandle;
		ErrorCode = ComputeOutputWindow(ViewPort_p, OutRec);
	}

	return ErrorCode;
}

ST_ErrorCode_t	STVID_ComputeInWindow(const STVID_ViewPortHandle_t ViewPortHandle,STGXOBJ_Rectangle_t *InRec)
{
	VideoViewPort_t       *ViewPort_p;
	ST_ErrorCode_t  ErrorCode;

	if (!(IsValidViewPortHandle(ViewPortHandle)))
	{
		ErrorCode = ST_ERROR_INVALID_HANDLE;
	}
	else
	{
		/* Retrieve the local ViewPort structure from the ViewPortHandle.          */
		ViewPort_p = (VideoViewPort_t *)ViewPortHandle;
		ComputeInputWindow(ViewPort_p, InRec);
	}

	return ErrorCode;

}


/* End of vid_comp.c */
