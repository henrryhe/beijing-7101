/*******************************************************************************

File name   : hv_filt.c

Description : Omega2 Filter management file (PSI, ...).

Notes       : File content...
                    *** Exported functions ***
                    InitFilter()
                    InitFilterEvt()
                    GetViewportPsiParameters()
                    SetViewportPsiParameters()
                    UpdateViewportPsiParameters()
                    TermFilter()

                    *** Private  functions ***
                    SetAutoFleshMode()
                    SetEdgeReplacementMode()
                    SetGreenBoostMode()
                    SetPeakingMode()
                    SetPSIMode()
                    SetSaturationMode()
                    SetTintMode()

                    VideoDisplayInterruptHandler()

                    ***   DCI  functions   ***
                    DCIParameterComputation()
                    DCIProcessEndOfAnalysis()
                    DCIDisableInterrupt()
                    DCIEnableInterrupt()
                    SetDCIAnalysisWindow()
                    SetDCI_ABAMode()
                    SetDCICoringLevel()
                    SetDCI_DSAMode()
                    SetDCIFilterMode()
                    SetDCIMode()
                    SetDCITransferFunction()
                    SetDCIPeakDetectionMode()

                    **************************

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
16 Dec 2002        Created                                          GGi
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

/* Additional trace for debug. */
/* #define TRACE_HAL_VIDEO_LAYER_FILTER*/
#ifdef TRACE_HAL_VIDEO_LAYER_FILTER
 extern void trace (const char *format,...);
 #define TraceBuffer(x)  trace x
#else
 #define TraceBuffer(x)
#endif /* TRACE_HAL_VIDEO_LAYER_FILTER */


#include <stdio.h>
#include <string.h>

#include "stddefs.h"

#include "stlayer.h"
#include "stgxobj.h"

#include "halv_lay.h"
#include "layer2.h"

/* Private Constants -------------------------------------------------------- */
/* Magic number to set dynamicly parameters.                                  */
#define SELF_SETTING 0x12345678

/* Peaking filtering selection array.                                         */
const U32 FilterToReg [DIS_PK_FILT_MASK+1] = {50, 25, 18, 17, 12, 9, 8, 7};

/* DCI default peak thresholds.                                               */
const U8 DCIPeakThresholds[] = {188, 192, 195, 199, 202, 206, 209, 213, \
                                216, 219, 223, 226, 230, 233, 237, 240};

/* Default automatic parameters settings.                                     */

const STLAYER_AutoFleshParameters_t DefaultAutoFleshSettings[4] =
    {       /*  STLAYER_VIDEO_CHROMA_AUTOFLESH          */
            /* Chroma Autoflesh default parameters.     */
    {00, MEDIUM_WIDTH, AXIS_121_0},         /* Disable  */
    {50, MEDIUM_WIDTH, AXIS_116_6},         /* AutoMode1*/
    {50, MEDIUM_WIDTH, AXIS_121_0},         /* AutoMode2*/
    {50, MEDIUM_WIDTH, AXIS_125_5}          /* AutoMode3*/
    };

const STLAYER_GreenBoostParameters_t DefaultGreenBoostSettings[4] =
    {       /*  STLAYER_VIDEO_CHROMA_GREENBOOST         */
            /* Chroma Green Boost default parameters.   */
    {00},                                   /* Disable  */
    {25},                                   /* AutoMode1*/
    {50},                                   /* AutoMode2*/
    {75}                                    /* AutoMode3*/
    };
const STLAYER_TintParameters_t  DefaultTintSettings[4] =
    {       /*  STLAYER_VIDEO_CHROMA_TINT               */
            /* Chroma Tint default parameters.          */
    {00},                                   /* Disable  */
    {-10},                                  /* AutoMode1*/
    {+10},                                  /* AutoMode2*/
    {+20}                                   /* AutoMode3*/
    };
const STLAYER_SatParameters_t DefaultSatSettings[4] =
    {       /*  STLAYER_VIDEO_CHROMA_SAT                */
            /* Chroma Saturation default parameters.    */
    {00},                                   /* Disable  */
    {-33},                                  /* AutoMode1*/
    {+33},                                  /* AutoMode2*/
    {+66}                                   /* AutoMode3*/
    };
const STLAYER_EdgeReplacementParameters_t DefaultEdgeReplacementSettings[4] =
    {       /*  STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT     */
    {00, MEDIUM_FREQ_FILTER},               /* Disable  */
    {25, SELF_SETTING},                     /* AutoMode1*/
    {50, SELF_SETTING},                     /* AutoMode2*/
    {75, SELF_SETTING}                      /* AutoMode3*/
    };
const STLAYER_PeakingParameters_t DefaultPeakingSettings[4] =
    {
            /*  STLAYER_VIDEO_LUMA_PEAKING              */
    {0,0,0,0,0,FALSE},                    /* Disable  */
    {-33,20,-33,20,SELF_SETTING,TRUE},      /* AutoMode1*/
    {+33,20,+33,20,SELF_SETTING,TRUE},      /* AutoMode2*/
    {+66,20,+66,20,SELF_SETTING,TRUE}       /* AutoMode3*/
    };
const STLAYER_DCIParameters_t DefaultDCISettings[4] =
    {
            /*  STLAYER_VIDEO_LUMA_DCI                  */
    {0,0,0,0,0},    /* Disable  */
    {25,SELF_SETTING,SELF_SETTING,SELF_SETTING,SELF_SETTING},    /* AutoMode1*/
    {50,SELF_SETTING,SELF_SETTING,SELF_SETTING,SELF_SETTING},    /* AutoMode2*/
    {75,SELF_SETTING,SELF_SETTING,SELF_SETTING,SELF_SETTING}     /* AutoMode3*/
    };

/* Private Types ------------------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Determine the register gain value according to gain signed percentage.     */
#define GetPkRegisterGainValue(Gain) \
    (((Gain) <= 0) ? (((6*(Gain))+600)*8)/300 : (((8*(Gain)*47)+12800)/100)/8)

/* Macro that if the layer type is corresponding to a STi7020 chips one.      */
#define IsLayerTypeMainNotAux(Handle)                                          \
  ((((HALLAYER_LayerProperties_t *)Handle)->LayerType == STLAYER_OMEGA2_VIDEO1)\
  || (((HALLAYER_LayerProperties_t *)Handle)->LayerType == STLAYER_7020_VIDEO1))

/* Private Variables (static)------------------------------------------------ */

/* Private Functions -------------------------------------------------------- */

static ST_ErrorCode_t SetAutoFleshMode(const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);

static ST_ErrorCode_t SetEdgeReplacementMode(const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
static ST_ErrorCode_t SetGreenBoostMode (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
static ST_ErrorCode_t SetPeakingMode    (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
static ST_ErrorCode_t SetPSIMode        (const STLAYER_Handle_t LayerHandle,
                        BOOL isPSIEnable);
static ST_ErrorCode_t SetSaturationMode (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
static ST_ErrorCode_t SetTintMode       (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
static void VideoDisplayInterruptHandler(STEVT_CallReason_t     Reason,
                        const ST_DeviceName_t  RegistrantName,
                        STEVT_EventConstant_t  Event,
                        const void*            EventData_p,
                        const void*            SubscriberData_p);

static ST_ErrorCode_t DCIParameterComputation (const STLAYER_Handle_t LayerHandle,
                        U32 *dis_dci_tf_dpp,
                        U32 *dis_dci_gain_seg1_frc,
                        U32 *dis_dci_gain_seg2_frc);
static ST_ErrorCode_t DCIDisableInterrupt(const STLAYER_Handle_t LayerHandle);
static ST_ErrorCode_t DCIEnableInterrupt(const STLAYER_Handle_t LayerHandle);

static ST_ErrorCode_t SetDCIAnalysisWindow (const STLAYER_Handle_t LayerHandle,
                        U32 StartPixel, U32 EndPixel,
                        U32 StartLine, U32 EndLine);
static ST_ErrorCode_t SetDCI_ABAMode (const STLAYER_Handle_t LayerHandle,
                        U32 Sensitivity, U32 LightSampleThreshold,
                        U32 DarkSampleThreshold, U32 LightSampleWeight);
static ST_ErrorCode_t SetDCICoringLevel (const STLAYER_Handle_t LayerHandle,
                        U32 CoringLevel);
static ST_ErrorCode_t SetDCI_DSAMode (const STLAYER_Handle_t LayerHandle,
                        U32 Sensitivity, U32 DarkAreaSize,
                        U32 DarkLevelThreshold);
#if 0
static ST_ErrorCode_t SetDCIFilterMode  (const STLAYER_Handle_t LayerHandle,
                        BOOL IsHDFilter);
#endif
static ST_ErrorCode_t SetDCIMode        (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);
#if 0 /* Not used for the moment. */
static ST_ErrorCode_t SetDCITransferFunction (const STLAYER_Handle_t LayerHandle,
                        U32 PivotPoint, U32 Gain1, U32 Gain2);
#endif /* 0 */
static ST_ErrorCode_t SetDCIPeakDetectionMode (const STLAYER_Handle_t LayerHandle,
                        U32 PeakSize, U8 *PeakThresholds_p);

/* -------------------------------------------------------------------------- */
/*******************************************************************************
Name        : InitFilter
Description : Initialize the filtering access device.
Parameters  : Layerhandle
Assumptions : LayerHandle is valid.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t InitFilter (const STLAYER_Handle_t LayerHandle)
{
    U32                             Count;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   =
            (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Initialize default Array parameters.     */
    for (Count = 0; Count < STLAYER_MAX_VIDEO_FILTER_POSITION; Count ++)
    {
        LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[Count].VideoFiltering
            = Count;
        LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[Count].VideoFilteringControl
            = STLAYER_DISABLE;
    }

    /* Initialize overall activity mask for this video layer.   */
    LayerCommonData_p->PSI_Parameters.ActivityMask = 0;

    if (IsLayerTypeMainNotAux(LayerHandle))
    {
        /* Initialize all display filter with default or disable values.    */
        SetPSIMode(LayerHandle, FALSE);

        /* Luma filtering.      */
        SetEdgeReplacementMode(LayerHandle,
                (STLAYER_VideoFilteringParameters_t *) &DefaultEdgeReplacementSettings[STLAYER_DISABLE]);
        memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT].VideoFilteringParameters.EdgeReplacementParameters),
               &(DefaultEdgeReplacementSettings[STLAYER_DISABLE]),
               sizeof(STLAYER_EdgeReplacementParameters_t));

        SetPeakingMode(LayerHandle,
                (STLAYER_VideoFilteringParameters_t *) &DefaultPeakingSettings[STLAYER_DISABLE]);
        memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_PEAKING].VideoFilteringParameters.PeakingParameters),
               &(DefaultPeakingSettings[STLAYER_DISABLE]),
               sizeof(STLAYER_PeakingParameters_t));

        /* DCI filtering.       */
        SetDCIMode (LayerHandle,
                (STLAYER_VideoFilteringParameters_t *) &DefaultDCISettings[STLAYER_DISABLE]);
        memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_DCI].VideoFilteringParameters.DCIParameters),
               &(DefaultDCISettings[STLAYER_DISABLE]),
               sizeof(STLAYER_DCIParameters_t));
        SetDCIPeakDetectionMode(LayerHandle,
                            DIS_DCI_PEAK_SIZE_DEFAULT, (U8*)DCIPeakThresholds);

        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iBsgf   = 0;
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iAvrgbf = 0;
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iYpf    = 0;

        /* Chroma filtering :    */
        SetAutoFleshMode(LayerHandle,
                (STLAYER_VideoFilteringParameters_t *) &DefaultAutoFleshSettings[STLAYER_DISABLE]);
        memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_CHROMA_AUTOFLESH].VideoFilteringParameters.AutoFleshParameters),
               &(DefaultAutoFleshSettings[STLAYER_DISABLE]),
                sizeof(STLAYER_AutoFleshParameters_t));

        SetGreenBoostMode(LayerHandle,
                (STLAYER_VideoFilteringParameters_t *) &DefaultGreenBoostSettings[STLAYER_DISABLE]);
        memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_CHROMA_GREENBOOST].VideoFilteringParameters.GreenBoostParameters),
               &(DefaultGreenBoostSettings[STLAYER_DISABLE]),
                sizeof(STLAYER_GreenBoostParameters_t));
    } /* Main layer, not aux. */

    /* Both layer init. */
    SetSaturationMode(LayerHandle,
                (STLAYER_VideoFilteringParameters_t *) &DefaultSatSettings[STLAYER_DISABLE]);
    memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_CHROMA_SAT].VideoFilteringParameters.SatParameters),
           &(DefaultSatSettings[STLAYER_DISABLE]),
           sizeof(STLAYER_SatParameters_t));

    SetTintMode(LayerHandle,
                (STLAYER_VideoFilteringParameters_t *) &DefaultTintSettings[STLAYER_DISABLE]);
    memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_CHROMA_TINT].VideoFilteringParameters.TintParameters),
           &(DefaultTintSettings[STLAYER_DISABLE]),
           sizeof(STLAYER_TintParameters_t));

        /* No error to report. */
    return(ErrorCode);

} /* End of InitFilter() function */

/*******************************************************************************
Name        : InitFilterEvt
Description : Initialize the evt acces.
Parameters  : - Layerhandle
              - Layer init parameters
Assumptions : LayerHandle is valid.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t InitFilterEvt (const STLAYER_Handle_t LayerHandle,
        const STLAYER_InitParams_t * const  LayerInitParams_p)
{
    STEVT_OpenParams_t              OpenParams;
    STEVT_DeviceSubscribeParams_t   SubscribeParams;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   =
            (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);


    if (IsLayerTypeMainNotAux(LayerHandle))
    {
        /* Main layer : Install display interrupt.  */
        /* Open Event handler.              */
        ErrorCode = STEVT_Open(LayerInitParams_p->EventHandlerName,
                &OpenParams, &(LayerCommonData_p->EvtHandle));

        if (ErrorCode != ST_NO_ERROR)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
        SubscribeParams.NotifyCallback      = VideoDisplayInterruptHandler;
        SubscribeParams.SubscriberData_p    = (void*)LayerHandle;
        ErrorCode = STEVT_SubscribeDeviceEvent(LayerCommonData_p->EvtHandle,
                (char*)LayerInitParams_p->InterruptEventName,
                LayerInitParams_p->VideoDisplayInterrupt,&SubscribeParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            ErrorCode = STEVT_Close(LayerCommonData_p->EvtHandle);
            return(ST_ERROR_BAD_PARAMETER);
        }
    }
    return(ErrorCode);

} /* End of InitFilterEvt() function */

/*******************************************************************************
Name        : GetViewportPsiParameters
Description : Get all required PSI parameters.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t GetViewportPsiParameters(const STLAYER_Handle_t LayerHandle,
        STLAYER_PSI_t * const ViewportPSI_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    if (ViewportPSI_p->VideoFiltering >= STLAYER_MAX_VIDEO_FILTER_POSITION)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Test special case for AUX display.   */
    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        if ( (ViewportPSI_p->VideoFiltering != STLAYER_VIDEO_CHROMA_TINT) &&
             (ViewportPSI_p->VideoFiltering != STLAYER_VIDEO_CHROMA_SAT) )
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
    *ViewportPSI_p =LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering];

    return(ST_NO_ERROR);

} /* End of GetViewportPsiParameters() function */

/*******************************************************************************
Name        : SetViewportPsiParameters
Description : Set all required PSI parameters (if enable) and store its
              parameters.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t SetViewportPsiParameters(const STLAYER_Handle_t LayerHandle,
        const STLAYER_PSI_t * const ViewportPSI_p)
{
    HALLAYER_LayerProperties_t *            LayerProperties_p;
    HALLAYER_LayerCommonData_t *            LayerCommonData_p;
    STLAYER_VideoFilteringParameters_t *    FilterParamsToApply_p;
    BOOL                                    AreParametersUpdated = FALSE;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    ST_ErrorCode_t(*FilterFct_p)(const STLAYER_Handle_t FilterFct_LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p);


    /* init Layer and viewport properties.  */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Test input parameters.               */
    if ( (ViewportPSI_p->VideoFiltering >= STLAYER_MAX_VIDEO_FILTER_POSITION) ||
         (ViewportPSI_p->VideoFilteringControl > STLAYER_ENABLE_MANUAL) )
    {
        /* Bad parameters. Return immediately. */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (ViewportPSI_p->VideoFilteringControl > STLAYER_ENABLE_MANUAL)
    {
        /* Bad parameters. Return immediately. */
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (ViewportPSI_p->VideoFiltering)
    {
        case STLAYER_VIDEO_CHROMA_AUTOFLESH :
            FilterFct_p = SetAutoFleshMode;
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *) &DefaultAutoFleshSettings[ViewportPSI_p->VideoFilteringControl];
            break;
        case STLAYER_VIDEO_CHROMA_GREENBOOST :
            FilterFct_p = SetGreenBoostMode;
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *) &DefaultGreenBoostSettings[ViewportPSI_p->VideoFilteringControl];
            break;
        case STLAYER_VIDEO_CHROMA_TINT :
            FilterFct_p = SetTintMode;
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *) &DefaultTintSettings[ViewportPSI_p->VideoFilteringControl];
            break;
        case STLAYER_VIDEO_CHROMA_SAT :
            FilterFct_p = SetSaturationMode;
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *) &DefaultSatSettings[ViewportPSI_p->VideoFilteringControl];
            break;
        case STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT :
            FilterFct_p = SetEdgeReplacementMode;
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *) &DefaultEdgeReplacementSettings[ViewportPSI_p->VideoFilteringControl];
            AreParametersUpdated = TRUE;
            break;
        case STLAYER_VIDEO_LUMA_PEAKING :
            FilterFct_p = SetPeakingMode;
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *) &DefaultPeakingSettings[ViewportPSI_p->VideoFilteringControl];
            AreParametersUpdated = TRUE;
            break;
        case STLAYER_VIDEO_LUMA_DCI :
            FilterFct_p = SetDCIMode;
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *) &DefaultDCISettings[ViewportPSI_p->VideoFilteringControl];
            AreParametersUpdated = TRUE;
            break;
        default :
            /* Unknown case. Return corresponding error.    */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    if (ViewportPSI_p->VideoFilteringControl == STLAYER_ENABLE_MANUAL)
    {
        FilterParamsToApply_p =
            (STLAYER_VideoFilteringParameters_t *) &ViewportPSI_p->VideoFilteringParameters;
    }

    ErrorCode = FilterFct_p (LayerHandle, FilterParamsToApply_p);
    if (ErrorCode == ST_NO_ERROR)
    {
        /* OK, remember new parameters, and return.     */
        if (AreParametersUpdated)
        {
            FilterParamsToApply_p =
                    (STLAYER_VideoFilteringParameters_t *)&(LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters);

        }
        /* Don't know which field was filled in the video filtering parameters union, so copy all */
        memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering].VideoFilteringParameters.AutoFleshParameters),
               FilterParamsToApply_p,
               sizeof(STLAYER_VideoFilteringParameters_t));

        LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering].VideoFilteringControl
                = ViewportPSI_p->VideoFilteringControl;

        /* Update global activity flag.                 */
        if (ViewportPSI_p->VideoFilteringControl == STLAYER_DISABLE)
        {
            LayerCommonData_p->PSI_Parameters.ActivityMask &= ~(1 << ViewportPSI_p->VideoFiltering);
            /* De-Activate PSI feature if necessary.    */
            if (LayerCommonData_p->PSI_Parameters.ActivityMask == 0)
            {
                SetPSIMode(LayerHandle, FALSE);
            }
        }
        else
        {
            /* Activate PSI feature if necessary.       */
            if (LayerCommonData_p->PSI_Parameters.ActivityMask == 0)
            {
                SetPSIMode(LayerHandle, TRUE);
            }
            LayerCommonData_p->PSI_Parameters.ActivityMask |= (1 << ViewportPSI_p->VideoFiltering);
        }
    }
    else
    {
        /* Wrong operation. Restore previous parameters.*/
        FilterParamsToApply_p =
        (STLAYER_VideoFilteringParameters_t*) &LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[ViewportPSI_p->VideoFiltering].VideoFilteringParameters;
        FilterFct_p (LayerHandle, FilterParamsToApply_p);
    }

    /* Return current error code. */
    return(ErrorCode);

} /* End of SetViewportPsiParameters() function */
/*******************************************************************************
Name        : TermFilter
Description : Terminate the filtering access device.
Parameters  : - Layerhandle
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER,
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t TermFilter (const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (IsLayerTypeMainNotAux(LayerHandle))
    {
        /* Main layer : Uninstall EOW interrupt */

        /* Un-subscribe and un-register events: this is done automatically by */
        /* STEVT_Close() Close instances opened at init.                      */
        ErrorCode = STEVT_Close(LayerCommonData_p->EvtHandle);
    }

    /* No error to report. */
    return(ErrorCode);

} /* End of TermFilter function */

/*******************************************************************************
Name        : UpdateViewportPsiParameters
Description : Update filters currently enabled.
Parameters  : - Layerhandle
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER,
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t UpdateViewportPsiParameters (const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    STLAYER_VideoFilteringControl_t TmpFilterControl;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (IsLayerTypeMainNotAux(LayerHandle))
    {
        /* Test currently enable filters that need to be updated.   */
        /* if input/output window parameter is changing.            */
        if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1 << STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT)) != 0)
        {
            /* Luma Edge Replacement needs to be updated if automatic mode. */
            TmpFilterControl =
                    LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT].VideoFilteringControl;
            if (TmpFilterControl != STLAYER_ENABLE_MANUAL)
            {
                SetEdgeReplacementMode(LayerHandle,
                    (STLAYER_VideoFilteringParameters_t *) &DefaultEdgeReplacementSettings[TmpFilterControl]);
                /* Don't know which field was filled in the video filtering parameters union, so copy all */
                memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT].VideoFilteringParameters.AutoFleshParameters),
                       &(LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.AutoFleshParameters),
                       sizeof(STLAYER_VideoFilteringParameters_t));
            }
        }
        if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1 << STLAYER_VIDEO_LUMA_PEAKING)) != 0)
        {
            /* Luma peaking needs to be updated if automatic mode.          */
            TmpFilterControl =
                    LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_PEAKING].VideoFilteringControl;
            if (TmpFilterControl != STLAYER_ENABLE_MANUAL)
            {
                SetPeakingMode(LayerHandle,
                    (STLAYER_VideoFilteringParameters_t *) &DefaultPeakingSettings[TmpFilterControl]);
                /* Don't know which field was filled in the video filtering parameters union, so copy all */
                memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_PEAKING].VideoFilteringParameters.AutoFleshParameters),
                       &(LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.AutoFleshParameters),
                       sizeof(STLAYER_VideoFilteringParameters_t));
            }
        }
        if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1 << STLAYER_VIDEO_LUMA_DCI)) != 0)
        {
            /* Luma DCI needs to be updated if automatic mode.              */
            TmpFilterControl =
                    LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_DCI].VideoFilteringControl;
            if (TmpFilterControl != STLAYER_ENABLE_MANUAL)
            {
                SetDCIMode(LayerHandle,
                    (STLAYER_VideoFilteringParameters_t *) &DefaultDCISettings[TmpFilterControl]);
                /* Don't know which field was filled in the video filtering parameters union, so copy all */
                memcpy(&(LayerCommonData_p->PSI_Parameters.CurrentPSIParameters[STLAYER_VIDEO_LUMA_DCI].VideoFilteringParameters.AutoFleshParameters),
                       &(LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.AutoFleshParameters),
                       sizeof(STLAYER_VideoFilteringParameters_t));
            }
        }
    }

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of UpdateViewportPsiParameters() function */

/* -------------------------------------------------------------------------- */
/*******************************************************************************
Name        : SetAutoFleshMode
Description : Set the AutoFlesh Mode.
Parameters  : - Layerhandle
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations : Function available only for MAIN Display process
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetAutoFleshMode(const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /*  - DIS_AF_OFF : Autoflesh gain control 0=AF disabled.max is 127  */
    /*  - DIS_AF_AXIS : Autoflesh axis control with ->                  */
    /*                0(116.60),1(121.00),2(125.50) or 3(130.20)        */
    /*  - DIS_AF_QWIDTH : Autoflesh quadrature width control with ->    */
    /*                0(+-150), 1(+-110) or 2(+-80)                     */

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_AF_OFF,
        ((FilterParams_p->AutoFleshParameters.AutoFleshControl * DIS_AF_OFF_MASK) / 100) & DIS_AF_OFF_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_AF_AXIS,
            FilterParams_p->AutoFleshParameters.AutoFleshAxisControl & DIS_AF_AXIS_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_AF_QWIDTH,
            FilterParams_p->AutoFleshParameters.QuadratureFleshWidthControl & DIS_AF_QWIDTH_MASK);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetAutoFleshMode() function */

/*******************************************************************************
Name        : SetEdgeReplacementMode
Description : Set the Edge Replacement Mode.
Parameters  : - Layerhandle
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations : Function available only for MAIN Display process
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetEdgeReplacementMode(const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    U32 FrequencyControl;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* - DIS_ER_DELAY : frequency control with ->       */
    /*                  0=3 (Low), 1 (High), 2 (Medium).*/
    /*                  Depending on Hout/Hin ratio.    */
    /* - DIS_ER_CTRL :  Gain Control with ->            */
    /*                  0=ER disabled, Max is 63.       */

    /* Check for frequency control to apply...          */

    /*      - Hout/Hin < 2 : High Freq filter           */
    /*      - Hout/Hin < 3 : Med. Freq filter           */
    /*      - Hout/Hin > 3 : Low Freq filter            */
    if (FilterParams_p->EdgeReplacementParameters.FrequencyControl == SELF_SETTING)
    {
        if (LayerCommonData_p->ViewPortParams.InputRectangle.Width != 0)
        {
            if (LayerCommonData_p->ViewPortParams.OutputRectangle.Width /
                    LayerCommonData_p->ViewPortParams.InputRectangle.Width < 2)
            {
                FrequencyControl = DIS_ER_DELAY_HF;
            }
            else if (LayerCommonData_p->ViewPortParams.OutputRectangle.Width /
                    LayerCommonData_p->ViewPortParams.InputRectangle.Width < 3)
            {
                FrequencyControl = DIS_ER_DELAY_MF;
            }
            else
            {
                FrequencyControl = DIS_ER_DELAY_LF;
            }
        }
        else
        {
            /* Calculation not available. Apply default case.   */
            FrequencyControl = DIS_ER_DELAY_MF;
        }
    }
    else
    {
        FrequencyControl = FilterParams_p->EdgeReplacementParameters.FrequencyControl;
    }

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_ER_DELAY,
            FrequencyControl & DIS_ER_DELAY_MASK);

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_ER_CTRL,
            ((FilterParams_p->EdgeReplacementParameters.GainControl * DIS_ER_CTRL_MASK)/100) & DIS_ER_CTRL_MASK);

    /* Update just set filter parameters.       */
    LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFiltering =
            STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT;
    /* Don't know which field was filled in the video filtering parameters union, so copy all */
    memcpy(&(LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters),
           FilterParams_p,
           sizeof(STLAYER_VideoFilteringParameters_t));
    LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.EdgeReplacementParameters.FrequencyControl =
            FrequencyControl;

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetEdgeReplacementMode() function. */

/*******************************************************************************
Name        : SetGreenBoostMode
Description : Set the Green boost parameters.
Parameters  : - Layerhandle
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations : Function available only for MAIN Display process
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetGreenBoostMode (const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    S32 RegValue;

    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* - DIS_GB_OFF : Green boost gain control with ->              */
    /*                0=GB disabled. max effect : -32               */
    /*                max revers effect : 31 (0x1f)                 */
    RegValue = (S32)((S32)(FilterParams_p->GreenBoostParameters.GreenBoostControl) * DIS_GB_MIN_VALUE)/100;

    if (RegValue > DIS_GB_MAX_VALUE)
    {
        RegValue = DIS_GB_MAX_VALUE;
    }
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_GB_OFF,
            RegValue & DIS_GB_OFF_MASK);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetGreenBoostMode() function */

/*******************************************************************************
Name        : SetPeakingMode
Description : Set the Peaking (Horizontal and Vertical) and SinX/X compensation
              parameters.
Parameters  : - Layerhandle,
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations : Function available only for MAIN Display process
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetPeakingMode (const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{

    U32 Count;
    U32 FilterRatio;
    U32 FilterRegisterValue = DIS_PK_FILT_MASK;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Default case:  take the ratio given as input parameter.      */
    FilterRatio =
            FilterParams_p->PeakingParameters.HorizontalPeakingFilterSelection;

    /*    - DIS_PK_V_PEAK : Vertical peaking gain control with ->   */
    /*                    0=-6dB, 16=0dB, 48=+6dB, 63=+8dB.         */
    /*    - DIS_PK_V_CORE : Vertical peaking core with ->           */
    /*                    0=coring disabled, max is 15.             */
    /*    - DIS_PK_H_PEAK : Horizontal peaking gain control with -> */
    /*                    0=-6dB, 16=0dB, 48=+6dB, 63=+8dB.         */
    /*    - DIS_PK_H_CORE : Horizontal peaking core with ->         */
    /*                    0=coring disabled, max is 15.             */
    /*    - DIS_PK_FILT : Horizontal peaking filter selection Fs/Fc */
    /*                    from 0 (0.5) to 7 (0.07)                  */
    /*    - DIS_PK_SIEN : Sine(x)/x compensation enable             */

    /* Filter selection management. */
    if (FilterParams_p->PeakingParameters.HorizontalPeakingFilterSelection == SELF_SETTING)
    {
        /* Compute the optimum ratio given by formula :     */
        /* Fc = (1/4) * (InWidth/OutWidth)                  */
        /* e.g. SD to SD, (1/4).(720/720)  = 0.25           */
        /* e.g. SD to HD, (1/4).(720/1920) = 0.09           */
        if (LayerCommonData_p->ViewPortParams.OutputRectangle.Width != 0)
        {
            FilterRatio =
                ((LayerCommonData_p->ViewPortParams.InputRectangle.Width * 100) /
                LayerCommonData_p->ViewPortParams.OutputRectangle.Width) / 4;
        }
        else
        {
            /* Default case, apply highest filter frequency filter effect.  */
            FilterRatio = 50;
        }
    }

    /* Get filter selection register value acoording to wanted ratio.*/
    for (Count=0; Count <= DIS_PK_FILT_MASK; Count++)
    {
        if (FilterRatio >= FilterToReg[Count])
        {
            FilterRegisterValue = Count;
            break;
        }
    }

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_PK_FILT,  FilterRegisterValue & DIS_PK_FILT_MASK);


    /* Peaking gain control calculation.                */
    /* Two cases :  - GainControl < 0                   */
    /*              - GainControl > 0                   */
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_PK_H_PEAK,
            (GetPkRegisterGainValue (FilterParams_p->PeakingParameters.HorizontalPeakingGainControl)) & DIS_PK_H_PEAK_MASK);

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_PK_H_CORE,
            ((FilterParams_p->PeakingParameters.CoringForHorizontalPeaking * DIS_PK_H_CORE_MASK) / 100) & DIS_PK_H_CORE_MASK);


    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_PK_V_PEAK,
            (GetPkRegisterGainValue (FilterParams_p->PeakingParameters.VerticalPeakingGainControl)) & DIS_PK_V_PEAK_MASK);

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_PK_V_CORE,
            ((FilterParams_p->PeakingParameters.CoringForVerticalPeaking * DIS_PK_V_CORE_MASK) / 100) & DIS_PK_V_CORE_MASK);


    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_PK_SIEN,
            (FilterParams_p->PeakingParameters.SINECompensationEnable ? DIS_PK_SIEN_ON : DIS_PK_SIEN_OFF));


    LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFiltering = STLAYER_VIDEO_LUMA_PEAKING;
    /* Don't know which field was filled in the video filtering parameters union, so copy all */
    memcpy(&(LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.AutoFleshParameters),
           FilterParams_p,
           sizeof(STLAYER_VideoFilteringParameters_t));
    LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.PeakingParameters.HorizontalPeakingFilterSelection
            = FilterRatio;

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetPeakingMode() function */

/*******************************************************************************
Name        : SetPSIMode
Description : Set the Saturation boost value.
Parameters  : - Layerhandle,
              - PSIEnable : Flag that is set to enable PSI filters.
                TRUE=enabled, FALSE=disabled.
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetPSIMode (const STLAYER_Handle_t LayerHandle,
        BOOL isPSIEnable)
{
    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_BYPASS,
            isPSIEnable ? DIS_DCI_BYPASS_DISABLE_VALUE : DIS_DCI_BYPASS_ENABLE_VALUE);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetPSIMode() function. */

/*******************************************************************************
Name        : SetSaturationMode
Description : Set the Saturation boost value. If Saturation is disabled, default
              value will be loaded.
Parameters  : - Layerhandle,
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetSaturationMode (const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (IsLayerTypeMainNotAux(LayerHandle))
    {
        /* - DIS_DCI_SAT : Saturation value to be applied to picture */
        /*                 with -> min=0, 32=OFF, MAX=63.            */
        HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_SAT,
            (DIS_DCI_SAT_DISABLE_VALUE +
            (FilterParams_p->SatParameters.SaturationGainControl * (DIS_DCI_SAT_MASK>>1)) / 100) & DIS_DCI_SAT_MASK);
    }
    else
    {
        if (FilterParams_p->SatParameters.SaturationGainControl != 0)
        {
            /* Saturation filter enable.                            */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_SAT,
            (DIP_TS_SAT_DISABLE_VALUE +
            (FilterParams_p->SatParameters.SaturationGainControl * (DIP_TS_SAT_MASK>>1)) / 100) & DIP_TS_SAT_MASK);
            /* Enable overall Tint/Saturation filter.               */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_BYPASS, 0);
        }
        else
        {
            /* Saturation filter disable.                           */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_SAT, DIP_TS_SAT_DISABLE_VALUE);
            /* Disable overall Tint/Saturation filter if necessary. */
            if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1<<STLAYER_VIDEO_CHROMA_TINT)) == 0)
            {
                /* Tint is not activated. Disable TS filter.        */
                HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_BYPASS, 1);
            }
        }
    }

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetSaturationMode() function */


/*******************************************************************************
Name        : SetTintMode
Description : Set the Peaking (Horizontal and Vertical) and SinX/X compensation
              parameters.
Parameters  : - Layerhandle,
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t SetTintMode (const STLAYER_Handle_t LayerHandle,
        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (IsLayerTypeMainNotAux(LayerHandle))
    {
        /* - DIS_AF_TINT : Tint correction control value with ->            */
        /*                 0=OFF. -32=min and +31=MAX (signed with 6 bits)  */

        HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_AF_TINT,
                (DIS_AF_TINT_DISABLE_VALUE +
                (FilterParams_p->TintParameters.TintRotationControl * (DIS_AF_TINT_MASK >> 1)) / 100) & DIS_AF_TINT_MASK);
    }
    else
    {
        if (FilterParams_p->TintParameters.TintRotationControl != 0)
        {
            /* Tint filter enable.  */

            /* - DIP_TS_TINT : Tint correction control value with ->            */
            /*                 0=OFF. -32=min and +31=MAX (signed with 6 bits)  */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_TINT,
                    (DIP_TS_TINT_DISABLE_VALUE +
                    (FilterParams_p->TintParameters.TintRotationControl * (DIP_TS_TINT_MASK>>1)) / 100) & DIP_TS_TINT_MASK);
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_BYPASS, 0);
        }
        else
        {
            /* Tint filter disable.                                 */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_TINT, DIP_TS_TINT_DISABLE_VALUE);
            /* Disable overall Tint/Saturation filter if necessary. */
            if ((LayerCommonData_p->PSI_Parameters.ActivityMask & (1<<STLAYER_VIDEO_CHROMA_SAT)) == 0)
            {
                /* Sat. is not activated. Disable TS filter.        */
                HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_TS_BYPASS, 1);
            }
        }
    }

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetTintMode() function */


/*******************************************************************************
Name        : VideoDisplayInterruptHandler
Description : Video display interrupt handler. Should be as short as possible.
Parameters  : ...
Assumptions : .
Limitations : .
Returns     : .
*******************************************************************************/
static void VideoDisplayInterruptHandler(STEVT_CallReason_t     Reason,
        const ST_DeviceName_t  RegistrantName,
        STEVT_EventConstant_t  Event,
        const void*            EventData_p,
        const void*            SubscriberData_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    U32                             RegITS;
    BOOL                            IsPendingUpdate;

    LayerProperties_p = (HALLAYER_LayerProperties_t *)SubscriberData_p;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);


    RegITS = (U32)HAL_Read32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_ITS)
            & LayerCommonData_p->PSI_Parameters.InterruptMask;

    if (RegITS & DIS_STA_EAW)
    {
        /* multi-task protection */
        STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);

        IsPendingUpdate = LayerCommonData_p->RegToUpdate != 0 ? TRUE : FALSE;

        /* As semaphore signal is done, the flag may no be set. */
        /* LayerCommonData_p->IsUpdateNeeded = FALSE;*/
        LayerCommonData_p->RegToUpdate    |= DCI_FILTER;

        /* multi-task protection */
        STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);

        if (!IsPendingUpdate)
        {
            /* Wake up the task.    */
            STOS_SemaphoreSignal(LayerProperties_p->HALOrderSemaphore_p);
        }
    }

} /* End of VideoDisplayInterruptHandler */

/* ************************************************************************** */
/* *******************  Specific DCI access function ************************ */
/* ************************************************************************** */

/*******************************************************************************
Name        : DCIParameterComputation
Description : Get from hardware following parameters :
                - dis_dci_avrg_br result of ABA (from DIS_DCI_AVRG_BR)
                - dis_dci_drks_dist result of DSDA (from DIS_DCI_DRKS_DIST)
                - dis_dci_cbcr_peak result of Chroma peak analysis
                  (from DIS_DCI_CBCR_PEAK)
                - dis_dci_peak_n peak size (from DIS_DCI_PEAK_N)
                - dis_dci_peak_size peak area size (from DIS_DCI_PEAK_SIZE)
                - dis_dci_fr_peak, 16 peak counters (from DIS_DCI_FR_PEAK[0..15])

              and return parameters to set the dual segment transfert function :

                - *dis_dci_tf_dpp returns pivot point
                  (to be written in DIS_DCI_TF_DPP)
                - *dis_dci_gain_seg1_frc returns segment 1 gain (to be written
                  in DIS_DCI_GAIN_SEG1_FRC)
                - *dis_dci_gain_seg2_frc returns segment 2 gain (to be written
                  in DIS_DCI_GAIN_SEG2_FRC)

Parameters  : - Layerhandle,
              - *dis_dci_tf_dpp         pivot point (DIS_DCI_TF_DPP)
              - *dis_dci_gain_seg1_frc  segment 1 gain (DIS_DCI_GAIN_SEG1_FRC)
              - *dis_dci_gain_seg2_frc  segment 2 gain (DIS_DCI_GAIN_SEG2_FRC)

Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED,
              ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t DCIParameterComputation (const STLAYER_Handle_t LayerHandle,
                                U32 *dis_dci_tf_dpp,
                                U32 *dis_dci_gain_seg1_frc,
                                U32 *dis_dci_gain_seg2_frc)

{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    int  dis_dci_avrg_br;
    int  dis_dci_drks_dist;
    U8   dis_dci_peak_size;

    int iAvrgbl;            /* Limited average brightness value */
    int iBsgl;              /* Limited dark dample distribution value */

    int iYp;                /* Sum of peak samples */
    int iYpl;               /* Limited sum of peak samples */

    int iIl, iAdrs, iTf_Dpp, iTf_Dppin, iMinBp, iGain_Seg1, iGain_Seg2;

    int PP_LIM = 0;         /* Pivot point lower limitation [0..74]     */
    int AP_LIM = 0;         /* Peak lower value limitation [0..55]      */
    int BS_GAIN = 255;      /* Black stretch gain (0: min, 255: max)    */

    /* Average Brightness Filter Coefficients.  */
    int AB_ATT0 = MAX(1, AB_ATT);   /* Attack time coeff._0 */
    int AB_ATT1 = (32 - AB_ATT0);   /* Attack time coeff._1 */
    int AB_DEC0 = MAX(1, AB_DEC);   /* Decay time coeff._0  */
    int AB_DEC1 = (32 - AB_DEC0);   /* Decay time coeff._1  */

    /* Dark Sample Distribution Filter Coefficients.    */
    int DS_ATT0 = MAX(1, DS_ATT);   /* Attack time coeff._0 */
    int DS_ATT1 = (32 - DS_ATT0);   /* Attack time coeff._1 */
    int DS_DEC0 = MAX(1, DS_DEC);   /* Decay time coeff._0  */
    int DS_DEC1 = (32 - DS_DEC0);   /* Decay time coeff._1  */

    /* Frame Peak value Filter Coefficients.    */
    int PK_ATT0 = MAX(1, PK_ATT);   /* Attack time coeff._0 */
    int PK_ATT1 = (32 - PK_ATT0);   /* Attack time coeff._1 */
    int PK_DEC0 = MAX(1, PK_DEC);   /* Decay time coeff._0  */
    int PK_DEC1 = (32 - PK_DEC0);   /* Decay time coeff._1  */

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Get needed^hardware parameters.      */
    /* - dis_dci_avrg_br result of ABA (from DIS_DCI_AVRG_BR)       */
    dis_dci_avrg_br =
         (int)(HAL_Read16(VideoBaseAddress(LayerHandle) + DIS_DCI_AVRG_BR) & DIS_DCI_AVRG_BR_MASK);
    /* - dis_dci_drks_dist result of DSDA (from DIS_DCI_DRKS_DIST)  */
    dis_dci_drks_dist =
            (int)(HAL_Read16(VideoBaseAddress(LayerHandle) + DIS_DCI_DRKS_DIST) & DIS_DCI_DRKS_DIST_MASK);
    dis_dci_peak_size =
            (U8)(HAL_Read8(VideoBaseAddress(LayerHandle) + DIS_DCI_PEAK_SIZE) & DIS_DCI_PEAK_SIZE_MASK);


/*    TraceBuffer(("\r\n DCI Param. Comp. "));*/
/*    TraceBuffer(("\r\nIn :avrg_br(%d), drks_dist(%d).", (int)dis_dci_avrg_br, (int)dis_dci_drks_dist ));*/

    /* Dynamic Pivot Point Computation. */
	iMinBp = (PP_LIM + 32-128) * 4; 					/* Pivot P. Lim <10,9,t>*/
	iAvrgbl = (int)LIM(dis_dci_avrg_br, 0, 148); 			/* Limit average bright.*/

#ifndef WS_MODE      /* White stretch OFF */
    iAvrgbl = 148;
#endif

    if((LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iAvrgbf/32) < iAvrgbl)
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iAvrgbf =
        iAvrgbl * AB_ATT0 + (AB_ATT1 * LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iAvrgbf/32);
	else
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iAvrgbf =
        iAvrgbl * AB_DEC0 + (AB_DEC1 * LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iAvrgbf/32);

    iTf_Dppin = 848 - (LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iAvrgbf/8); /* Pivot point internal */
	iTf_Dpp = (iTf_Dppin - 1024)/2 ;
	iTf_Dpp = MAX(iTf_Dpp, iMinBp); 					/* Pivot point <10,9,t> */

    /* Lower_Segment Gain Computation.  */

	iBsgl = LIM(dis_dci_drks_dist, 0, 255);
	iBsgl = MIN(iBsgl, BS_GAIN);

#ifndef BS_MODE     /* Black stretch OFF*/
    iBsgl = 0;
#endif

    if((LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iBsgf/32) < iBsgl)
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iBsgf =
        iBsgl * DS_ATT0 + (DS_ATT1 * LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iBsgf/32);
	else
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iBsgf =
        iBsgl * DS_DEC0 + (DS_DEC1 * LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iBsgf/32);

    iGain_Seg1 = LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iBsgf/4;  /* Gain fract. <12,0,u>*/

    /* Upper_Segment Gain Computation.  */
	AP_LIM = MIN(AP_LIM, 55);
	iYp =0;

	for(iIl = 0; iIl < 16 ; iIl++)
        iYp +=
        (int)HAL_Read16(VideoBaseAddress(LayerHandle) + DIS_DCI_FR_PEAK0 + iIl * 4) & DIS_DCI_FR_PEAK_MASK;

	iYp = iYp >> dis_dci_peak_size;
    iYp = (iYp * 55 ) >> 8;                     /* Relative peak value */
	iYpl = MAX(iYp, AP_LIM);
    iYpl += 185;                                /* Frame peak value */

#ifndef AP_MODE
    iYpl = 240;     /* Auto-Pix OFF */
#endif

    if((LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iYpf/32) < iYpl)
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iYpf =
        iYpl * PK_DEC0 + (PK_DEC1 * LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iYpf/32);
	else
        LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iYpf =
        iYpl * PK_ATT0 + (PK_ATT1 * LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iYpf/32);

    iAdrs = ((LayerCommonData_p->PSI_Parameters.DCI_SpecificParameters.iYpf/4) - iTf_Dppin);
	iGain_Seg2 = (1920 - iTf_Dppin) << 12;
	iGain_Seg2 = iGain_Seg2 / iAdrs;
	iGain_Seg2 = iGain_Seg2 - 4096;
	iGain_Seg2 = LIM(iGain_Seg2, 0, 2864); 		/* Gain fract. <12,0,u>*/

    /* Return results.  */
	*dis_dci_tf_dpp = iTf_Dpp;
	*dis_dci_gain_seg1_frc = iGain_Seg1;
	*dis_dci_gain_seg2_frc = iGain_Seg2;

/*    TraceBuffer((" Result : GAIN1:%4x, GAIN2:%4x DPP:%4x",*/
/*            (iGain_Seg1, iGain_Seg2, iTf_Dpp ));*/
/*    TraceBuffer(("Out :Tf_dpp: %d (%d IRE), Gain1: %d (%d%%), Gain2: %d (%d%%)\n",*/
/*                    (int)iTf_Dpp, ((iTf_Dpp+512)*100)/1024,*/
/*                                (int)iGain_Seg1, 100+(((int)iGain_Seg1*69)/2864),*/
/*                                (int)iGain_Seg2, 100+(((int)iGain_Seg2*69)/2864)));*/

    TraceBuffer(("%d, %d%%, %d%%\r\n",
                    ((iTf_Dpp+512)*100)/1024,
                    100+(((int)iGain_Seg1*69)/2864),
                    100+(((int)iGain_Seg2*69)/2864)));


    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of DCIParameterComputation() function. */

/*******************************************************************************
*******************************************************************************/
void DCIProcessEndOfAnalysis(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    U32     dis_dci_tf_dpp;
    U32     dis_dci_gain_seg1_frc;
    U32     dis_dci_gain_seg2_frc;

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)
                   (((HALLAYER_LayerProperties_t *)LayerHandle)->PrivateData_p);

    /* Calculate transfert function parameters.     */
    DCIParameterComputation(LayerHandle, &dis_dci_tf_dpp,
            &dis_dci_gain_seg1_frc, &dis_dci_gain_seg2_frc);

    /* Set transfert function parameters.           */
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_TF_DPP,
            (U32)dis_dci_tf_dpp /*& DIS_DCI_TF_DPP_MASK*/);

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_GAIN_SEG1_FRC,
            (U32)dis_dci_gain_seg1_frc /*& DIS_DCI_GAIN_SEG1_FRC_MASK*/);

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_GAIN_SEG2_FRC,
            (U32)dis_dci_gain_seg2_frc /*& DIS_DCI_GAIN_SEG2_FRC_MASK*/);

} /* End of DCIProcessEndOfAnalysis() function. */

/*******************************************************************************
*******************************************************************************/
static ST_ErrorCode_t DCIDisableInterrupt(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)
                          (((HALLAYER_LayerProperties_t *)LayerHandle)->PrivateData_p);

    LayerCommonData_p->PSI_Parameters.InterruptMask = 0;

    HAL_SetRegister32DefaultValue(VideoBaseAddress(LayerHandle) + DIS_ITM,
            0, HALLAYER_DISABLE);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of DCIDisableInterrupt() function. */

/*******************************************************************************
*******************************************************************************/
static ST_ErrorCode_t DCIEnableInterrupt(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    U32                             Tmp32;

    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)
                          (((HALLAYER_LayerProperties_t *)LayerHandle)->PrivateData_p);

    Tmp32 = HAL_Read32(VideoBaseAddress(LayerHandle) + DIS_ITS);

    LayerCommonData_p->PSI_Parameters.InterruptMask = DIS_STA_EAW;

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_ITM, DIS_STA_EAW);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of DCIEnableInterrupt() function. */


/*******************************************************************************
Name        : SetDCIAnalysisWindow
Description : Initialize parameters of analysis window
Parameters  : - Layerhandle,
              - StartPixel : Start pixel position of analysis window
              - EndPixel : Last pixel position of analysis window
              - StartLine : First line position of analysis window
              - EndLine : Last line position of analysis window
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCIAnalysisWindow (const STLAYER_Handle_t LayerHandle,
        U32 StartPixel, U32 EndPixel, U32 StartLine, U32 EndLine)
{
    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_SPIXEL,
            (U32)StartPixel & DIS_DCI_SPIXEL_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_EPIXEL,
            (U32)EndPixel & DIS_DCI_EPIXEL_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_SLINE,
            (U32)StartLine & DIS_DCI_SLINE_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_ELINE,
            (U32)EndLine & DIS_DCI_ELINE_MASK);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCIAnalysisWindow() function */

/*******************************************************************************
Name        : SetDCI_ABAMode
Description : Set the average brightness analysis mode.
                - Sensitivity (SENSWS)
                - Light & dark thresholds (LSTHR, DSTHR)
                - Light samples weight (LSWF)
Parameters  :
        - Layerhandle,
        - Sensitivity,          recommended value: Analysis window size / 141.
        - LightSampleThreshold, samples with a value superior are considered as
                                light samples recommended: 106
        - DarkSampleThreshold,  samples with a value inferior are considered as
                                dark samples recommended: 122
        - LightSampleWeight     contribution of light samples
                                0: null contribution, 15: max (recommended: 2).
Assumptions : LayerHandle is valid and corresponds to a video MAIN LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCI_ABAMode (const STLAYER_Handle_t LayerHandle,
        U32 Sensitivity, U32 LightSampleThreshold,
        U32 DarkSampleThreshold, U32 LightSampleWeight)
{

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_SENSWS,
            ((U32)Sensitivity & DIS_DCI_SENSWS_MASK));
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_LSTHR,
            ((U32)LightSampleThreshold & DIS_DCI_LSTHR_MASK));
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_DSTHR,
            ((U32)DarkSampleThreshold & DIS_DCI_DSTHR_MASK));
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_LSWF,
            ((U32)LightSampleWeight & DIS_DCI_LSWF_MASK));

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCI_ABAMode() function */

/*******************************************************************************
Name        : SetDCICoringLevel
Description : Set the Adaptative signal splitter (DCI filter) mode.
              as DCI_COR and SNR_COR are redundant, select DCI_COR in COR_MOD
              (value 0) and don't use SNR_COR.
Parameters  : - Layerhandle,
              - CoringLevel : Coring level ro set.
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCICoringLevel (const STLAYER_Handle_t LayerHandle,
        U32 CoringLevel)
{
    /* Set the coring mod to 0 so that the DCI uses a fixed coring value    */
    /* stored in register DIS_DCI_COR.                                      */
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_COR_MOD, 0);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_COR,
            (CoringLevel & DIS_DCI_COR_MASK));

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCICoringLevel() function */

/*******************************************************************************
Name        : SetDCI_DSAMode
Description : Set the Dark Sample Analysis mode.
Parameters  : - Layerhandle,
              - Sensitivity             sensitivity of the DSDA
              - DarkAreaSize            sensitivity to small dark areas
              - DarkLevelThreshold      dark sample threshold level
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCI_DSAMode (const STLAYER_Handle_t LayerHandle,
        U32 Sensitivity, U32 DarkAreaSize, U32 DarkLevelThreshold)
{
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_SENSBS,
            (U32)Sensitivity & DIS_DCI_SENSBS_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_DYTC,
            (U32)DarkAreaSize & DIS_DCI_DYTC_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_ERR_COMP,
            (U32)(Sensitivity/DarkAreaSize) & DIS_DCI_ERR_COMP_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_DLTHR,
            (U32)DarkLevelThreshold & DIS_DCI_DLTHR_MASK);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCI_DSAMode() function */


#if 0
/*******************************************************************************
Name        : SetDCIFilterMode
Description : Set the Adaptative signal splitter (DCI filter) mode.
Parameters  : - Layerhandle,
              - IsHDFilter : Flag that is set for HD stream management.
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCIFilterMode (const STLAYER_Handle_t LayerHandle,
        BOOL IsHDFilter)
{

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_FILTER,
            IsHDFilter ? TRUE : FALSE);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCIFilterMode() function */
#endif

/*******************************************************************************
Name        : SetDCIMode
Description : Enable/disable DCI display filter
Parameters  : - Layerhandle
              - FilterParams_p
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCIMode (const STLAYER_Handle_t LayerHandle,
                        STLAYER_VideoFilteringParameters_t * FilterParams_p)
{
    #define ANALYSIS_WINDOW_MARGING  16

    HALLAYER_LayerCommonData_t    * LayerCommonData_p = (HALLAYER_LayerCommonData_t *)
                          (((HALLAYER_LayerProperties_t *)LayerHandle)->PrivateData_p);

    U32 ScanRatio           = (LayerCommonData_p->LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN ? 2 : 1);

    U32 analysisWindowWidth , HorAnalysisStart;
    U32 analysisWindowHeight, VertAnalysisStart;
    U32 analysisWindowSize;

    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /*    - DCIEnable : Flag that is set to enable DCI filters.             */
    /*    - AnalysisEnable : Flag that is set to enable the DCI analysis    */
    /*    - ColorSatCompensationEnable : same for color saturation          */
    /*    compensation.                                                     */
    /*    TRUE=enabled, FALSE=disabled.                                     */

    LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFiltering = STLAYER_VIDEO_LUMA_DCI;
    /* Don't know which field was filled in the video filtering parameters union, so copy all */
    memcpy(&(LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.AutoFleshParameters),
           FilterParams_p,
           sizeof(STLAYER_VideoFilteringParameters_t));

    /* Check for DCI activity.                                              */
    if ( (FilterParams_p->DCIParameters.FirstPixelAnalysisWindow == 0) &&
         (FilterParams_p->DCIParameters.LastPixelAnalysisWindow  == 0) &&
         (FilterParams_p->DCIParameters.FirstLineAnalysisWindow  == 0) &&
         (FilterParams_p->DCIParameters.LastLineAnalysisWindow   == 0) )
    {
        /* Unavailable analysis window. Consider it as disable condition.   */
        DCIDisableInterrupt(LayerHandle);
        HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_DCI_ON, FALSE);
    }
    else
    {
        /* Calculate analysis window.                                       */
        if (FilterParams_p->DCIParameters.FirstPixelAnalysisWindow == SELF_SETTING)
        {
            /* At least one analysis window parameter is automatic.         */
            /* Consider all are automativly set.                            */
            analysisWindowWidth = (LayerCommonData_p->ViewPortParams.OutputRectangle.Width != 0 ?
                        LayerCommonData_p->ViewPortParams.OutputRectangle.Width - 2 * ANALYSIS_WINDOW_MARGING : 0);
            HorAnalysisStart    = ANALYSIS_WINDOW_MARGING;
            analysisWindowHeight= (LayerCommonData_p->ViewPortParams.OutputRectangle.Height != 0 ?
                    (LayerCommonData_p->ViewPortParams.OutputRectangle.Height - 2 * ANALYSIS_WINDOW_MARGING) / ScanRatio : 0);
            VertAnalysisStart   = ANALYSIS_WINDOW_MARGING / ScanRatio;

        }
        else
        {
            analysisWindowWidth =  FilterParams_p->DCIParameters.LastPixelAnalysisWindow -
                    FilterParams_p->DCIParameters.FirstPixelAnalysisWindow;
            HorAnalysisStart    = FilterParams_p->DCIParameters.FirstPixelAnalysisWindow;
            analysisWindowHeight= (FilterParams_p->DCIParameters.LastLineAnalysisWindow -
                    FilterParams_p->DCIParameters.FirstLineAnalysisWindow) / ScanRatio;
            VertAnalysisStart   = FilterParams_p->DCIParameters.FirstLineAnalysisWindow / ScanRatio;
        }
        analysisWindowSize = analysisWindowWidth * analysisWindowHeight;

        SetDCIAnalysisWindow(LayerHandle, HorAnalysisStart,  analysisWindowWidth  + HorAnalysisStart,
                                          VertAnalysisStart, analysisWindowHeight + VertAnalysisStart);

        LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.DCIParameters.FirstPixelAnalysisWindow
                = HorAnalysisStart;
        LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.DCIParameters.LastPixelAnalysisWindow
                = analysisWindowWidth  + HorAnalysisStart;
        LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.DCIParameters.FirstLineAnalysisWindow
                = VertAnalysisStart * ScanRatio;
        LayerCommonData_p->PSI_Parameters.JustUpdatedParameters.VideoFilteringParameters.DCIParameters.LastLineAnalysisWindow
                = (analysisWindowHeight + VertAnalysisStart) * ScanRatio;

        SetDCI_ABAMode(LayerHandle, analysisWindowSize/141,
                DIS_DCI_LSTHR_DEFAULT, DIS_DCI_DSTHR_DEFAULT, DIS_DCI_LSWF_DEFAULT);

        SetDCICoringLevel(LayerHandle,
                (FilterParams_p->DCIParameters.CoringLevelGainControl * DIS_DCI_COR_MASK) / 100);

        SetDCI_DSAMode(LayerHandle, analysisWindowSize/278,
                DIS_DCI_DYTC_DEFAULT, DIS_DCI_DLTHR_DEFAULT);

        if (LayerCommonData_p->ViewPortParams.OutputRectangle.Width >= 1280)
        {
            /* HD filter.   */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_FILTER, 1);
        }
        else
        {
            /* SD, filter.  */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_FILTER, 0);
        }

/*        Action performed at initialization for ever. No need to update it     */
/*        SetDCIPeakDetectionMode(LayerHandle,*/
/*                            DIS_DCI_PEAK_SIZE_DEFAULT, (U8*)DCIPeakThresholds);*/

        DCIEnableInterrupt(LayerHandle);
        HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_DCI_ON, TRUE);
        HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_FREEZE_ANLY, FALSE);
        HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_CSC_ON, TRUE);
    }

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCIMode() function. */

/*******************************************************************************
Name        : SetDCIPeakDetectionMode
Description : Sets DCI peak detection mode:
                - Contribution of small size peaks (PEAK_SIZE)
                - Initialize peak detect threshold ranges (PEAK_THR[0..15])
Parameters  : - Layerhandle,
              - PeakSize,   Peak area size
              - PeakThresholds_p discrete peak detect threshold range
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCIPeakDetectionMode (const STLAYER_Handle_t LayerHandle,
        U32 PeakSize, U8 *PeakThresholds_p)
{
    int Count;

    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_PEAK_SIZE,
            (U32)PeakSize & DIS_DCI_PEAK_SIZE_MASK);

    for (Count=0; Count<16; Count++)
        HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_PEAK_THR0 + Count*4,
        (U32)PeakThresholds_p[Count]);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCIPeakDetectionMode() function */

#if 0 /* Not used for the moment. */
/*******************************************************************************
Name        : SetDCITransferFunction
Description : Set transfer function pivot point (TF_DPP), and gains
              (GAIN_SEG1_FRC, GAIN_SEG2_FRC).
Parameters  : - Layerhandle,
              - PivotPoint, pivot point location, between 7 and 409
              - Gain1,      fractional part of the gain below pivot point
              - Gain2       fractional part of the gain above pivot point
                            0=>gain=1, 2864=>gain=1.69
Assumptions : LayerHandle is valid and corresponds to a video LayerType.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetDCITransferFunction (const STLAYER_Handle_t LayerHandle,
        U32 PivotPoint, U32 Gain1, U32 Gain2)
{
    if (!IsLayerTypeMainNotAux(LayerHandle))
    {
        /* This feature is not supported for those types of layer.  */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_TF_DPP,
            (U32)PivotPoint & DIS_DCI_TF_DPP_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_GAIN_SEG1_FRC,
            (U32)Gain1 & DIS_DCI_GAIN_SEG1_FRC_MASK);
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIS_DCI_GAIN_SEG2_FRC,
            (U32)Gain2 & DIS_DCI_GAIN_SEG2_FRC_MASK);

    /* No error to report. */
    return(ST_NO_ERROR);

} /* End of SetDCITransferFunction() function */
#endif /* 0 */

/* End of file Hv_filt.c. */
