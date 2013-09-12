/*******************************************************************************

File name   : hv_vdplay.c

Description : Layer Video HAL (Hardware Abstraction Layer) for displaypipe chips
              access to hardware source file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
18 Oct 2005        Created                                          DG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* #define VIDEO_HALLAYER_DEBUG */      /* Internal flag to check error cases.        */
/* #define HAL_VIDEO_LAYER_REG_DEBUG */ /* Internal flag to debug Display registers */

/* To disable filters (i.e. set unitary filters) */
/* #define HAL_VIDEO_LAYER_FILTER_DISABLE */

/* Includes ----------------------------------------------------------------- */

#ifndef ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stcommon.h"
#include "sttbx.h"

#ifdef ST_OSLINUX
     #include "stlayer_core.h"
#endif

#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "halv_lay.h"
#include "hv_vdplay.h"
#include "hv_vdptools.h"
#include "hv_vdpdei.h"
#include "hv_vdpfmd.h"
#include "hv_vdpfilt.h"

#if defined(DVD_SECURED_CHIP)
#include "stmes.h"
#endif

#if defined DBG_USE_MANUAL_VHSRC_PARAMS || defined DBG_CHECK_VDP_CONSISTENCY
    #define TRACE_UART
#endif

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART*/

/* Select the Traces you want */
#define TrMain                 TraceOff
#define TrApi                  TraceOff
#define TrDebug                TraceOff         /* Additional trace for debug */
#define TrLinux                TraceOff
#define TrVsync                TraceOff
#define TrViewport             TraceOff
#define TrBandwidth            TraceOff
#define TrHorizontalCoeff      TraceOff
#define TrVerticalCoeff        TraceOff
#define TrHorizontalFilter     TraceOn
#define TrVerticalFilter       TraceOn
#define TrLayerPosition        TraceOff
#define TrFieldInversions      TraceOff

#define TrWarning              TraceOn
/* TrError is now defined in layer_trace.h */

/* layer_trace.h" should be included AFTER the definition of the traces wanted */
#include "layer_trace.h"


/* Private Constants -------------------------------------------------------- */

#if defined(DVD_SECURED_CHIP)
/* Size necessary to hold all the filters coefficients in memory */
#define HALLAYER_SECURE_AREA_SIZE   (8*1024)
#define HALLAYER_ALIGNMENT_128BYTES (128)
#endif /* defined(DVD_SECURED_CHIP) */

/* Enable/Disable */
#define HALLAYER_DISABLE                                        0
#define HALLAYER_ENABLE                                         1

/* Number Max of Viewports */
#define MAX_MAIN_LAYER_VIEWPORTS_MAX_NB                         1       /* Because of hardware limitations, the architecture */
                                                                        /* of the API doesn't allow more than 1 viewport.    */
#define MAX_AUX_LAYER_VIEWPORTS_MAX_NB                          1

#define MAX_SEC_LAYER_VIEWPORTS_MAX_NB                          1

/* Max zoom out factors */
#define VERTICAL_MAX_ZOOM_OUT                                   4       /* Max zoom out is 4 */
#define HORIZONTAL_MAX_ZOOM_OUT                                 4       /* Max zoom out is 4 */

/* Maximum values due to hardware limitation */
#define SOURCE_HEIGHT_MINIMUM_VALUE         16     /* The display is corrupted when less than 16 pixel in height are read */
#define SOURCE_WIDTH1_MINIMUM_VALUE         34
#define SOURCE_WIDTH2_MINIMUM_VALUE         16

#define DEFAULT_LINE_DURATION               60      /* 60 us */


/* Private Types ------------------------------------------------------------ */

typedef struct
{
    /*************************      Video LAYER parameters      *************************/
    STLAYER_Handle_t            AssociatedLayerHandle;          /* Associated layer handle, to retrieve its characteristics  */
    BOOL                        Opened;                         /* Viewport's status (Open/Close)                            */
    U8                          ViewportId;                     /* Viewport Id                                               */
    U32                         SourceNumber;                   /* Source number linked to it                                */

} HALLAYER_ViewportProperties_t;

#if defined(DVD_SECURED_CHIP)
/* Structure for controling the initialization of the secure data         */
/* On secure platforms, the filters are loaded only once in a secure area */
/* Once allocated, the secure area cannot be deallocated anymore and is   */
/* re-used at each 'Init' call using the following 'secure control info'  */
typedef struct HALLAYER_SecureFiltersControl_s
{
    BOOL                  SecureFilters_InitDone;
    STAVMEM_BlockHandle_t SecureFilters_BlockHandle;
    void *                SecureFilters_StartAddress;
    void *                SecureFilters_StopAddress;
} HALLAYER_SecureDataControl_t;
#endif /* defined(DVD_SECURED_CHIP) */

/* Private Macros ----------------------------------------------------------- */

#define Max(Val1, Val2) (((Val1) > (Val2))?(Val1):(Val2))

/* Private Variables (static)------------------------------------------------ */
static BOOL IsDisp1Initialised = FALSE;
static BOOL IsDisp2Initialised = FALSE;
#if defined (HW_7200)
static BOOL IsDisp3Initialised = FALSE;
#endif

#if defined(DVD_SECURED_CHIP)
static HALLAYER_SecureDataControl_t HALLAYER_SecureDataControl =
{
    /* SecureFilters_InitDone     */     FALSE,
    /* SecureFilters_Handle       */     (STAVMEM_BlockHandle_t)NULL,
    /* SecureFilters_StartAddress */     (void*)NULL,
    /* SecureFilters_StopAddress  */     (void*)NULL
};
#endif /* defined(DVD_SECURED_CHIP) */

#ifdef TRACE_UART
/* String array used for debug */
static const char * DbgFilterType[] =
    {
        "FILTER_NO_SET",
        "FILTER_NULL",
        "FILTER_A",
        "FILTER_B",
        "FILTER_Y_C",
        "FILTER_C_C",
        "FILTER_Y_D",
        "FILTER_C_D",
        "FILTER_Y_E",
        "FILTER_C_E",
        "FILTER_Y_F",
        "FILTER_C_F",
        "FILTER_H_CHRX2",
        "FILTER_V_CHRX2"
    };
#endif /* TRACE_UART */

/* Functions ---------------------------------------------------------------- */
static ST_ErrorCode_t AdjustViewportParams(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_ViewPortParams_t * const   Params_p);
static ST_ErrorCode_t SetViewportParams(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p);
static ST_ErrorCode_t SetViewportPSI(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_PSI_t * const        ViewportPSI_p);
static ST_ErrorCode_t Init(
                         const STLAYER_Handle_t              LayerHandle,
                         const STLAYER_InitParams_t * const  LayerInitParams_p);
static ST_ErrorCode_t OpenViewport(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_ViewPortParams_t * const    Params_p,
                         STLAYER_ViewPortHandle_t *          ViewportHandle_p);
static ST_ErrorCode_t CloseViewport(
                         const STLAYER_ViewPortHandle_t ViewportHandle);
static ST_ErrorCode_t Term(
                         const STLAYER_Handle_t              LayerHandle,
                         const STLAYER_TermParams_t *        TermParams_p);
static void GetCapability(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_Capability_t *              Capability_p);
static void EnableVideoViewport(
                         const STLAYER_ViewPortHandle_t      ViewportHandle);
static void DisableVideoViewport(
                         const STLAYER_ViewPortHandle_t      ViewportHandle);
static void SetViewportPriority(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         const HALLAYER_ViewportPriority_t   Priority);
static ST_ErrorCode_t SetViewportSource(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         const STLAYER_ViewPortSource_t *    Source_p);
static void GetViewportSource(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         U32 *                               SourceNumber_p);
static ST_ErrorCode_t SetViewportSpecialMode(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_OutputMode_t         OuputMode,
                         const STLAYER_OutputWindowSpecialModeParams_t * const Params_p);
static ST_ErrorCode_t GetViewportSpecialMode (
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_OutputMode_t *             const OuputMode_p,
                         STLAYER_OutputWindowSpecialModeParams_t * const Params_p);
static void SetLayerParams(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_LayerParams_t * const      LayerParams_p) ;
static void GetLayerParams(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_LayerParams_t *            LayerParams_p);
static void SetViewportPosition(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         const U32                           ViewportXPosition,
                         const U32                           ViewportYPosition);
static void GetViewportPosition(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         U32 *                               XPos_p,
                         U32 *                               YPos_p);
static ST_ErrorCode_t GetViewportParams(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p);
static ST_ErrorCode_t GetViewportPSI(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_PSI_t * const              ViewportPSI_p);

static ST_ErrorCode_t GetViewportIORectangle(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STGXOBJ_Rectangle_t * InputRectangle_p,
                         STGXOBJ_Rectangle_t * OutputRectangle_p);
static void UpdateFromMixer(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_OutputParams_t * const      OutputParams_p);
static void SetViewportAlpha(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         STLAYER_GlobalAlpha_t*              Alpha_p);
static void GetViewportAlpha(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         STLAYER_GlobalAlpha_t*              Alpha_p);
static BOOL IsUpdateNeeded(
                         const STLAYER_Handle_t             LayerHandle);
static void SynchronizedUpdate(
                         const STLAYER_Handle_t             LayerHandle);
static ST_ErrorCode_t GetRequestedDeinterlacingMode (
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_DeiMode_t * const          RequestedMode_p);
static ST_ErrorCode_t SetRequestedDeinterlacingMode (
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_DeiMode_t            RequestedMode);
static ST_ErrorCode_t DisableFMDReporting (
                         const STLAYER_ViewPortHandle_t ViewportHandle);
static ST_ErrorCode_t EnableFMDReporting (
                         const STLAYER_ViewPortHandle_t ViewportHandle);
static ST_ErrorCode_t GetFMDParams (
                         const STLAYER_ViewPortHandle_t ViewportHandle,
                         STLAYER_FMDParams_t * Params_p);
static ST_ErrorCode_t SetFMDParams (
                         const STLAYER_ViewPortHandle_t ViewportHandle,
                         STLAYER_FMDParams_t * Params_p);
static ST_ErrorCode_t GetVideoDisplayParams (
                         const STLAYER_ViewPortHandle_t ViewportHandle,
                         STLAYER_VideoDisplayParams_t * Params_p);

/* private ERROR management functions --------------------------------------- */

#ifdef VIDEO_HALLAYER_DEBUG
static void HALLAYER_Error(void);
#else
static void HALLAYER_Error(void){}
#endif

#ifdef DBG_FILTER_COEFFS
    static void DBG_CheckHorizontalFilterPointer(const STLAYER_Handle_t LayerHandle);
    static void DBG_CheckVerticalFilterPointer(const STLAYER_Handle_t LayerHandle);
#endif

/* private DISPLAY management functions ---------------------- */

static void InitializeVideoProcessor(const STLAYER_Handle_t LayerHandle);

static void AdjustInputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p);
static void AdjustOutputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p);
static void ApplyViewPortParams(const STLAYER_ViewPortHandle_t ViewportHandle, STLAYER_ViewPortParams_t * const Params_p);
static void CenterNegativeValues(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p);

static ST_ErrorCode_t CheckForAvailableScaling (const STLAYER_Handle_t LayerHandle,
        const STLAYER_ViewPortParams_t  *ViewPortParams_p,
        STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
        STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation,
        STLAYER_DecimationFactor_t      *RecommendedHorizontalDecimation_p,
        STLAYER_DecimationFactor_t      *RecommendedVerticalDecimation_p,
        U32                             *RecommendedHorizontalSize_p,
        U32                             *RecommendedVerticalSize_p);
static void SetViewportOutputRectangle(const HALLAYER_LayerProperties_t * LayerProperties_p);
static void ComputeAndApplyThePolyphaseFilters(const STLAYER_Handle_t LayerHandle,
                    STLAYER_ViewPortParams_t *      ViewPortParams_p,
                    STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
                    STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation);
static ST_ErrorCode_t SetColorSpaceConversionMode(const HALLAYER_LayerProperties_t * const LayerProperties_p,
        const STGXOBJ_ColorSpaceConversionMode_t ColorSpaceConversionMode);

static U32 GetOutputRectangleDisplayTime(const STLAYER_Handle_t     LayerHandle,
                                         U32                        OutputVerticalSize,
                                         U32                        DurationOfOneLine);
static U32 GetInputLumaDataSize(const STLAYER_Handle_t   LayerHandle,
                           U32                      InputHorizontalSize,
                           U32                      InputVerticalSize,
                           BOOL                     IsInputProgressive);
static U32 GetDurationOfOneLine(ST_DeviceName_t VTGName);

#ifdef DBG_CHECK_VDP_CONSISTENCY
    static void CheckVdpConsistency(STLAYER_Handle_t        LayerHandle,
                        HALLAYER_VDPVhsrcRegisterMap_t *    VhsrcTopRegister_p,
                        HALLAYER_VDPVhsrcRegisterMap_t *    VhsrcBottomRegister_p,
                        HALLAYER_VDPDeiRegisterMap_t *      DeiRegister_p,
                        HALLAYER_VDPIncrement_t *           Increment_p);
#endif

#if 0
    static void DumpFilters(void);
    static void DumpFilterCoeffs(const char * coeffs_name, const S8 * coeff_p, U32 coeff_length);
#endif

/* private MAIN DISPLAY management functions -------------------------------- */




/* Global Functions --------------------------------------------------------- */

#ifdef ST_OSLINUX
extern STAVMEM_PartitionHandle_t STAVMEM_GetPartitionHandle( U32 PartitionID );
#endif

/* Global Variables --------------------------------------------------------- */

const HALLAYER_FunctionsTable_t HALLAYER_DisplayPipeFunctions =
{
    AdjustViewportParams,
    CloseViewport,
    DisableFMDReporting,
    DisableVideoViewport,
    EnableFMDReporting,
    EnableVideoViewport,
    GetCapability,
    GetFMDParams,
    GetLayerParams,
    GetRequestedDeinterlacingMode,
    GetVideoDisplayParams,
    GetViewportAlpha,
    GetViewportParams,
    GetViewportPSI,
    GetViewportIORectangle,
    GetViewportPosition,
    GetViewportSource,
    GetViewportSpecialMode,
    Init,
    IsUpdateNeeded,
    OpenViewport,
    SetFMDParams,
    SetLayerParams,
    SetRequestedDeinterlacingMode,
    SetViewportAlpha,
    SetViewportParams,
    SetViewportPSI,
    SetViewportPosition,
    SetViewportPriority,
    SetViewportSource,
    SetViewportSpecialMode,
    SynchronizedUpdate,
    UpdateFromMixer,
    Term
};

#ifdef HAL_VIDEO_LAYER_REG_DEBUG
    BOOL                UseLocalDisplayRegisters = FALSE;
    HALLAYER_VDPVhsrcRegisterMap_t      VhsrcTopDebugRegister, VhsrcBottomDebugRegister;
    HALLAYER_VDPDeiRegisterMap_t         DeiDebugRegister;
#endif


/*******************************************************************************
Name        : AdjustInputWindow
Description : Adjust the input window parameters according to the viewport
              characteristics and the hardware constraints.
Parameters  : IN        - HAL layer manager handle.
              IN/OUT    - Init viewport params.
Assumptions : The decimation factor must be applied before calling this function.
              Otherwise they won't be taken into account.
              The function CenterNegativeValues must be called before this one.
Limitations :
Returns     :
*******************************************************************************/
static void AdjustInputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t    * LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    S32                             XOffsetInPixelUnit, YOffsetInPixelUnit;
    U32                             SourcePictureHorizontalSize, SourcePictureVerticalSize;

    TrApi(("\r\nAdjustInputWindow"));

    /* Avoid null pointer for input parameter. */
    if (Params_p == NULL)
    {
        return;
    }

    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
    if (ViewportProperties_p == NULL)
    {
        return;
    }

    /* Retrieve the parameters of the source. */
    SourcePictureHorizontalSize = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
    SourcePictureVerticalSize   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;

    /* Test width against minimum values accepted by the hardware */
    if (SourcePictureHorizontalSize >= SOURCE_WIDTH1_MINIMUM_VALUE)
    {
        /* The source picture has more than 34 pixels in width */

        /* The input rectangle width can not be lower than 34 pixels */
        if (Params_p->InputRectangle.Width < SOURCE_WIDTH1_MINIMUM_VALUE)
        {
            Params_p->InputRectangle.Width = SOURCE_WIDTH1_MINIMUM_VALUE;
        }
        /* Thus the PositionX should not exceed "source picture witdh - 34"
           (Warning: The input position is in 1/16 of pixel) */
        if ( (U32) Params_p->InputRectangle.PositionX > (SourcePictureHorizontalSize-SOURCE_WIDTH1_MINIMUM_VALUE) * 16)
        {
            Params_p->InputRectangle.PositionX = (SourcePictureHorizontalSize-SOURCE_WIDTH1_MINIMUM_VALUE) * 16;
        }
    }
    else
    {
        /* The source picture doesn't have 34 pixels in width: It can not be displayed by the Video Display Pipe */
    }

    /* Test height against minimum values accepted by the hardware */
    if (SourcePictureVerticalSize >= SOURCE_HEIGHT_MINIMUM_VALUE)
    {
        /* The source picture is more than 5 pixels high */

        /* The input rectangle width can not be lower than 5 pixels high */
        if (Params_p->InputRectangle.Height < SOURCE_HEIGHT_MINIMUM_VALUE)
        {
            Params_p->InputRectangle.Height = SOURCE_HEIGHT_MINIMUM_VALUE;
        }
        /* Thus the PositionY should not exceed "source picture witdh - SOURCE_HEIGHT_MINIMUM_VALUE"
           (Warning: The input position is in 1/16 of pixel) */
        if ( (U32) Params_p->InputRectangle.PositionY > (SourcePictureVerticalSize-SOURCE_HEIGHT_MINIMUM_VALUE) * 16)
        {
            Params_p->InputRectangle.PositionY = (SourcePictureVerticalSize-SOURCE_HEIGHT_MINIMUM_VALUE) * 16;
        }
    }
    else
    {
        /* The source picture is not high enough: It can not be displayed by the Video Display Pipe */
    }

    /* Tests to avoid any division by zero !!!  */
    if ( (Params_p->InputRectangle.Width != 0) && (Params_p->InputRectangle.Height != 0) )
    {
        /* XOffset and YOffset tests...                             */
        /* PS: the input rectangle position unit is 1/16 pixel !!!  */
        XOffsetInPixelUnit = Params_p->InputRectangle.PositionX / 16;
        YOffsetInPixelUnit = Params_p->InputRectangle.PositionY / 16;

        if ((U32)XOffsetInPixelUnit > SourcePictureHorizontalSize)
        {
            XOffsetInPixelUnit = SourcePictureHorizontalSize;
        }

        if ((U32)YOffsetInPixelUnit > SourcePictureVerticalSize)
        {
            YOffsetInPixelUnit = SourcePictureVerticalSize;
        }

        /* Make sure to have the edge of the displayed video image falling into the display window. */
        if ((XOffsetInPixelUnit + Params_p->InputRectangle.Width) > SourcePictureHorizontalSize)
        {
            /* Adjust the width of output window, to keep a constant ratio. */
            Params_p->OutputRectangle.Width  = ( (SourcePictureHorizontalSize - XOffsetInPixelUnit) * Params_p->OutputRectangle.Width)
                                                     / Params_p->InputRectangle.Width;
            Params_p->InputRectangle.Width   = SourcePictureHorizontalSize - XOffsetInPixelUnit ;
        }

        if ((YOffsetInPixelUnit + Params_p->InputRectangle.Height) > SourcePictureVerticalSize)
        {
            /* Adjust the height of output window, to keep a constant ratio. */
            Params_p->OutputRectangle.Height  = ( (SourcePictureVerticalSize - YOffsetInPixelUnit) * Params_p->OutputRectangle.Height)
                                                    / Params_p->InputRectangle.Height;
            Params_p->InputRectangle.Height   = SourcePictureVerticalSize - YOffsetInPixelUnit ;
        }
    }
} /* End of AdjustInputWindow() function. */


/*******************************************************************************
Name        : AdjustOutputWindow
Description : Adjust the output window parameters according to the output
              dimensions.
Parameters  : IN   - HAL layer manager handle.
              IN/OUT - Init viewport params.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AdjustOutputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t    * LayerCommonData_p;
    U32                             NonVisibleWidth, NonVisibleHeight;

    TrApi(("\r\nAdjustOutputWindow"));

    /* Avoid null pointer for input parameter.          */
    if (Params_p == NULL)
    {
        return;
    }

    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Tests to avoid any division by zero !!!  */
    if ( (Params_p->OutputRectangle.Width != 0) && (Params_p->OutputRectangle.Height != 0) &&
         (Params_p->InputRectangle.Width   != 0) && (Params_p->InputRectangle.Height  != 0) )
    {
        /* Check the ViewPort output parameters with the Layer dimensions.          */
        if (Params_p->OutputRectangle.PositionX <= (S32)(LayerCommonData_p->LayerParams.Width ))
        {
            if ((Params_p->OutputRectangle.PositionX + Params_p->OutputRectangle.Width) > LayerCommonData_p->LayerParams.Width)
            {
                NonVisibleWidth = Params_p->OutputRectangle.Width -
                        (LayerCommonData_p->LayerParams.Width - Params_p->OutputRectangle.PositionX);
                Params_p->InputRectangle.Width     -= ((NonVisibleWidth * Params_p->InputRectangle.Width)
                        / (Params_p->OutputRectangle.Width));
                Params_p->OutputRectangle.Width    -= NonVisibleWidth;
            }
        }
        else
        {
            Params_p->OutputRectangle.Width = 0;
        }

        if (Params_p->OutputRectangle.PositionY <= (S32)(LayerCommonData_p->LayerParams.Height ))
        {
            if ((Params_p->OutputRectangle.PositionY + Params_p->OutputRectangle.Height) > LayerCommonData_p->LayerParams.Height)
            {
                NonVisibleHeight = Params_p->OutputRectangle.Height -
                        (LayerCommonData_p->LayerParams.Height - Params_p->OutputRectangle.PositionY);
                Params_p->InputRectangle.Height    -= ((NonVisibleHeight * Params_p->InputRectangle.Height)
                        / Params_p->OutputRectangle.Height);
                Params_p->OutputRectangle.Height   -= NonVisibleHeight;
            }
        }
        else
        {
            Params_p->OutputRectangle.Height = 0;
        }
    }
} /* End of AdjustOutputWindow() function */

/*******************************************************************************
Name        : ApplyViewPortParams
Description : Apply the viewport new parameters.
Parameters  : HAL layer manager handle.
              ViewPort params.
Assumptions : The viewport parameters have to be correct. The function
              AdjustInputWindow and AdjustOutputWindow must be called before.
Limitations :
Returns     :
*******************************************************************************/
static void ApplyViewPortParams(const STLAYER_ViewPortHandle_t ViewportHandle, STLAYER_ViewPortParams_t * const Params_p)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;
    U32                             RecommendedOutputVerticalSize;
    U32                             RecommendedOutputHorizontalSize;
    STLAYER_DecimationFactor_t      RecommendedHorizontalDecimation;
    STLAYER_DecimationFactor_t      RecommendedVerticalDecimation;
    STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation;
    STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation;
    ST_ErrorCode_t                  ErrorCode;
    STLAYER_UpdateParams_t          UpdateParams;

    /* Avoid null pointer for input parameter.          */
    if (Params_p == NULL)
    {
        return;
    }

    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;
    if (ViewportProperties_p == NULL)
    {
        return;
    }

    LayerProperties_p    = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p    = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    TrViewport(("\r\nApplyViewPortParams"));

    /* Set local variables CurrentPictureHorizontalDecimation and CurrentPictureVerticalDecimation. */
    CurrentPictureHorizontalDecimation = (((Params_p->Source_p)->Data).VideoStream_p)->HorizontalDecimationFactor ;
    CurrentPictureVerticalDecimation = (((Params_p->Source_p)->Data).VideoStream_p)->VerticalDecimationFactor ;

    /* Check for viewport parameters. If correct, then apply this viewport. */
    /* Otherwise, it's an error case. Disable the current viewport.         */
    if ( (LayerCommonData_p->ViewPortParams.InputRectangle.Width  == 0) ||
         (LayerCommonData_p->ViewPortParams.InputRectangle.Height == 0) ||
         (LayerCommonData_p->ViewPortParams.OutputRectangle.Width == 0) ||
         (LayerCommonData_p->ViewPortParams.OutputRectangle.Height== 0))
    {
        /* Special case to display nothing !!!! */
        /* We must apply an invisible area parameters. */
        /* GAM_VIDn_VPO : Viewport Origin.   */
        LayerCommonData_p->ViewportOffset = 0;
        LayerCommonData_p->ViewportSize = 0;
        /* The Viewport will be updated at next VSync */
        LayerCommonData_p->UpdateViewport = TRUE;

    }
    else
    {
    /**************************************************************************************************************************************/
    /*                     Mecanism to switch between DECIMATION / NO DECIMATION / NEW DECIMATION NEEDED                                  */
    /**************************************************************************************************************************************/
    /*                                            Check(No Decimation)                                                                    */
    /*    ________________________________________________|___________________________________________________________________            */
    /*   |                     |                              |                                       |                       |           */
    /*   V                     V                              V                                       V                       V           */
    /* NO_ERROR             NO_ERROR                   FEATURE_NOT_SUPPORTED                 FEATURE_NOT_SUPPORTED       BAD_PARAMETER    */
    /*   |(Cur Dec = None)     |(Cur Dec != None)             | (Cur Dec != None)                     | (Cur Dec = None)      |           */
    /*   V                     V                              V                                       V                       V           */
    /*  Apply (No Decim)    Send EVT(Don't need Dec)    Check(Cur Dec)                        Send EVT(Need New Dec)  Nothing done: exit  */
    /*                         |                      ________|___________________________            |                                   */
    /*                         V                     |              |                     |           V                                   */
    /*                   Apply (No Decim)            V              V                     V       Apply (No Decim)                        */
    /*                                            NO_ERROR   FEATURE_NOT_SUPPORTED   BAD_PARAMETER                                        */
    /*                                               |              |                     |                                               */
    /*                                               V              V                     V                                               */
    /*                                      Apply (Cur Dec) Send EVT(Need New Dec)  Nothing done: exit                                    */
    /*                                                              |                                                                     */
    /*                                                              V                                                                     */
    /*                                                        Apply (Cur Dec)                                                             */
    /**************************************************************************************************************************************/

        /* First of all, try without decimation. It always gives the best quality. */
        ErrorCode = CheckForAvailableScaling (ViewportProperties_p->AssociatedLayerHandle, Params_p,
                STLAYER_DECIMATION_FACTOR_NONE,     STLAYER_DECIMATION_FACTOR_NONE ,
                &RecommendedHorizontalDecimation,   &RecommendedVerticalDecimation,
                &RecommendedOutputHorizontalSize,   &RecommendedOutputVerticalSize);

        if (ErrorCode == ST_NO_ERROR)
        {
            /* ST_NO_ERROR means that the scaling is possible without decimation. */
            /* Force video display to present non-decimated picture in the case   */
            /* when the previous picture displayed was decimated.                 */
            if ((CurrentPictureHorizontalDecimation != STLAYER_DECIMATION_FACTOR_NONE) ||
                (CurrentPictureVerticalDecimation   != STLAYER_DECIMATION_FACTOR_NONE) )
            {
                UpdateParams.UpdateReason                       = STLAYER_DECIMATION_NEED_REASON;
                UpdateParams.IsDecimationNeeded                 = FALSE;
                if (CurrentPictureHorizontalDecimation == STLAYER_DECIMATION_MPEG4_P2)
                {
                    UpdateParams.IsDecimationNeeded             = TRUE;
                }
                if (CurrentPictureHorizontalDecimation == STLAYER_DECIMATION_AVS)
                {
                    UpdateParams.IsDecimationNeeded             = TRUE;
                }
                UpdateParams.RecommendedHorizontalDecimation    = RecommendedHorizontalDecimation;
                UpdateParams.RecommendedVerticalDecimation      = RecommendedVerticalDecimation;
                UpdateParams.StreamWidth  = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
                UpdateParams.StreamHeight = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;

                STEVT_Notify(LayerProperties_p->EventsHandle,
                        LayerProperties_p->RegisteredEventsID[HALLAYER_UPDATE_DECIMATION_EVT_ID],
                        (const void *) (&UpdateParams));
            }

            /* Adjust the Output according to the new hardware constraints. */
            AdjustOutputWindow(ViewportProperties_p->AssociatedLayerHandle, Params_p);

            /* Store the Viewport params applied.                               */
            LayerCommonData_p->ViewPortParams = *Params_p;

            LayerCommonData_p->RecommendedVerticalDecimation = RecommendedVerticalDecimation;
            LayerCommonData_p->RecommendedHorizontalDecimation = RecommendedHorizontalDecimation;
        } /* end of 1st check is NO_ERROR */
        else if (ErrorCode == ST_ERROR_FEATURE_NOT_SUPPORTED)
        {
            /* Working without decimation is not possible. Try with the current decimation if existing. */
            if ((CurrentPictureHorizontalDecimation != STLAYER_DECIMATION_FACTOR_NONE) ||
                (CurrentPictureVerticalDecimation   != STLAYER_DECIMATION_FACTOR_NONE))
            {
                /* A current decimation exists : try it. */
                ErrorCode = CheckForAvailableScaling (ViewportProperties_p->AssociatedLayerHandle, Params_p,
                        CurrentPictureHorizontalDecimation, CurrentPictureVerticalDecimation,
                        &RecommendedHorizontalDecimation,   &RecommendedVerticalDecimation,
                        &RecommendedOutputHorizontalSize,   &RecommendedOutputVerticalSize);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Adjust the Output according to the new hardware constraints. */
                    AdjustOutputWindow(ViewportProperties_p->AssociatedLayerHandle, Params_p);

                    /* Store the Viewport params applied. */
                    LayerCommonData_p->ViewPortParams = *Params_p;
                    LayerCommonData_p->RecommendedVerticalDecimation = RecommendedVerticalDecimation;
                    LayerCommonData_p->RecommendedHorizontalDecimation = RecommendedHorizontalDecimation;
                } /* end of 1st check is NO_ERROR */
            }
            /* else : there is no current decimation. */

            if (ErrorCode == ST_ERROR_FEATURE_NOT_SUPPORTED)
            {
                /* Use the Recommended parameters and send an event to ask the decimation. */
                /* Force video display to present decimated picture if possible.    */
                /* This change also requires the re-calculation of all.             */
                UpdateParams.UpdateReason       = STLAYER_DECIMATION_NEED_REASON;
                UpdateParams.IsDecimationNeeded = TRUE;
                UpdateParams.RecommendedVerticalDecimation = RecommendedVerticalDecimation;
                UpdateParams.RecommendedHorizontalDecimation = RecommendedHorizontalDecimation;
                UpdateParams.StreamWidth  = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
                UpdateParams.StreamHeight = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;

                STEVT_Notify(LayerProperties_p->EventsHandle,
                        LayerProperties_p->RegisteredEventsID[HALLAYER_UPDATE_DECIMATION_EVT_ID],
                        (const void *) (&UpdateParams));

                /* Apply the new recommended output window.                     */
                Params_p->OutputRectangle.Width  = RecommendedOutputHorizontalSize;
                Params_p->OutputRectangle.Height = RecommendedOutputVerticalSize;

                /* Adjust the Output according to the new hardware constraints. */
                AdjustOutputWindow(ViewportProperties_p->AssociatedLayerHandle, Params_p);

                LayerCommonData_p->RecommendedVerticalDecimation = RecommendedVerticalDecimation;
                LayerCommonData_p->RecommendedHorizontalDecimation = RecommendedHorizontalDecimation;

                /* Store the Viewport params applied.                               */
                LayerCommonData_p->ViewPortParams = *Params_p;
            }
        } /* end of 1st check is FEATURE_NOT_SUPPORTED */

        /* Here is the apply part : */
        if (ErrorCode != ST_ERROR_BAD_PARAMETER)
        {
            /* Store the Viewport params applied.                                   */
            LayerCommonData_p->ViewPortParams = *Params_p;

            /* Check for viewport parameters. If correct, then apply this viewport. */
            /* Otherwise, it's an error case. Disable the current viewport.         */
            if ( (LayerCommonData_p->ViewPortParams.InputRectangle.Width  == 0) ||
                (LayerCommonData_p->ViewPortParams.InputRectangle.Height == 0) ||
                (LayerCommonData_p->ViewPortParams.OutputRectangle.Width == 0) ||
                (LayerCommonData_p->ViewPortParams.OutputRectangle.Height== 0))
            {
                LayerCommonData_p->ViewportOffset = 0;
                LayerCommonData_p->ViewportSize = 0;
                LayerCommonData_p->UpdateViewport = TRUE;
            }
            else
            {
                /* Compute and apply viewport position and size. */
                SetViewportOutputRectangle(LayerProperties_p);

                SetColorSpaceConversionMode(LayerProperties_p,
                    ((Params_p->Source_p)->Data.VideoStream_p)->BitmapParams.ColorSpaceConversion);
            }
        } /* end of if 1st or 2nd Check != BAD_PARAMETER */
    }
} /* End of ApplyViewPortParams() function */

/*******************************************************************************
Name        : CenterNegativeValues
Description : Check for negative value of InputRectangle and OutputRectangle,
              and transform them to manage only positive ones.
Parameters  :   IN   - HAL layer manager handle.
              IN/OUT - viewport params.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void CenterNegativeValues(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p)

{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t    * LayerCommonData_p;

    S32 InputPositionX, OutputPositionX ;
    U32 InputWidth,     OutputWidth ;
    S32 InputPositionY, OutputPositionY ;
    U32 InputHeight,    OutputHeight ;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* InputPositionX < 0 */
    if (Params_p->InputRectangle.PositionX < 0)
    {
        InputPositionX  = -Params_p->InputRectangle.PositionX / 16;
        InputWidth      = Params_p->InputRectangle.Width;
        OutputPositionX = Params_p->OutputRectangle.PositionX;
        OutputWidth     = Params_p->OutputRectangle.Width;

        Params_p->InputRectangle.PositionX = 0;

        if ( InputWidth < (U32)(InputPositionX))
        {
            Params_p->InputRectangle.Width = 0;
        }
        else
        {
            Params_p->InputRectangle.Width -= InputPositionX;
        }
        if (InputWidth != 0)
        {
            Params_p->OutputRectangle.PositionX += ((InputPositionX * Params_p->OutputRectangle.Width) / InputWidth);
        }
        Params_p->OutputRectangle.Width     -= (Params_p->OutputRectangle.PositionX - OutputPositionX);
    }

    /* InputPositionY < 0 */
    if (Params_p->InputRectangle.PositionY < 0)
    {
        InputPositionY  = -Params_p->InputRectangle.PositionY / 16;
        InputHeight     = Params_p->InputRectangle.Height;
        OutputPositionY = Params_p->OutputRectangle.PositionY;
        OutputHeight    = Params_p->OutputRectangle.Height;

        Params_p->InputRectangle.PositionY = 0;

        if ( InputHeight < (U32)(InputPositionY) )
        {
            Params_p->InputRectangle.Height = 0;
        }
        else
        {
            Params_p->InputRectangle.Height -= InputPositionY;
        }
        if (InputHeight != 0)
        {
            Params_p->OutputRectangle.PositionY += (( InputPositionY * Params_p->OutputRectangle.Height) / InputHeight);
        }
        Params_p->OutputRectangle.Height    -= (Params_p->OutputRectangle.PositionY - OutputPositionY);

    }

    /* OutputPositionX < 0 */
    if (Params_p->OutputRectangle.PositionX < 0)
    {
        InputPositionX  = Params_p->InputRectangle.PositionX / 16;
        InputWidth      = Params_p->InputRectangle.Width;
        OutputPositionX = -Params_p->OutputRectangle.PositionX;
        OutputWidth     = Params_p->OutputRectangle.Width;
        if (OutputWidth != 0) /* HG: Quick patch to avoid division by 0 ! */
        {
            Params_p->InputRectangle.PositionX   = (OutputPositionX * Params_p->InputRectangle.Width) / OutputWidth;
        }
        Params_p->InputRectangle.Width      -= (Params_p->InputRectangle.PositionX - InputPositionX);
        Params_p->InputRectangle.PositionX  *= 16;  /* Unit is 1/16th of pixels */
        Params_p->OutputRectangle.PositionX  = 0;
        Params_p->OutputRectangle.Width     -= OutputPositionX;
    }

    /* OutputPositionY < 0 */
    if (Params_p->OutputRectangle.PositionY < 0)
    {
        InputPositionY  = Params_p->InputRectangle.PositionY / 16;
        InputHeight     = Params_p->InputRectangle.Height;
        OutputPositionY = -Params_p->OutputRectangle.PositionY;
        OutputHeight    = Params_p->OutputRectangle.Height;
        if (OutputHeight != 0) /* HG: Quick patch to avoid division by 0 ! */
        {
            Params_p->InputRectangle.PositionY   = (S32)(OutputPositionY * Params_p->InputRectangle.Height) / OutputHeight;
        }
        Params_p->InputRectangle.Height     -= (Params_p->InputRectangle.PositionY - InputPositionY);
        Params_p->InputRectangle.PositionY  *= 16;  /* Unit is 1/16th of pixels */
        Params_p->OutputRectangle.PositionY  = 0;
        Params_p->OutputRectangle.Height    -= OutputPositionY ;
    }
} /* End of CenterNegativeValues() function */

/*******************************************************************************
Name        : Init
Description : Allocate and initialize the HAL_LAYER properties value.
Parameters  : HAL layer manager handle, init layer params.
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no problem during init.
                - ST_ERROR_NO_MEMORY, if memory allocation problem.
                - ST_ERROR_BAD_PARAMETER, if bad Init parameters.
                - ST_ERROR_ALREADY_INITIALIZED if already initialised.
*******************************************************************************/
static ST_ErrorCode_t Init(const STLAYER_Handle_t       LayerHandle,
                    const STLAYER_InitParams_t * const  LayerInitParams_p)
{
    STAVMEM_AllocBlockParams_t              AvmemAllocParams;

#if defined(DVD_SECURED_CHIP)
    STAVMEM_BlockHandle_t                   SecureFiltersBufferBlockHandle;
#else /* defined(DVD_SECURED_CHIP) */
    STAVMEM_BlockHandle_t                   HorizontalFilterBufferBlockHandle;
    STAVMEM_BlockHandle_t                   VerticalFilterBufferBlockHandle;

    STAVMEM_BlockHandle_t                   AlternateHorizontalFilterBufferBlockHandle;
    STAVMEM_BlockHandle_t                   AlternateVerticalFilterBufferBlockHandle;
#endif /*  defined(DVD_SECURED_CHIP) */
    STAVMEM_FreeBlockParams_t               AvmemFreeParams;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;     /* Avmem mapping */

#if defined(DVD_SECURED_CHIP)
    void *                                  SecureFiltersBufferVirtualBaseAddress;
    void *                                  SecureFiltersBufferVirtualStopAddress;
#else /* defined(DVD_SECURED_CHIP) */
    void *                                  HorizontalFilterBufferVirtualAddress;
    void *                                  VerticalFilterBufferVirtualAddress;
    void *                                  HorizontalFilterBufferAddress;
    void *                                  VerticalFilterBufferAddress;

    void *                                  AlternateHorizontalFilterBufferVirtualAddress;
    void *                                  AlternateVerticalFilterBufferVirtualAddress;
    void *                                  AlternateHorizontalFilterBufferAddress;
    void *                                  AlternateVerticalFilterBufferAddress;
#endif /*  defined(DVD_SECURED_CHIP) */

    HALLAYER_LayerProperties_t              * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t           * LayerCommonData_p;
    HALLAYER_ViewportProperties_t           * ViewportProperties_p;
    STLAYER_LayerParams_t                   * LayerParams_p;
    U8                                      ViewportId;
    U8                                      DisplayInstanceIndex;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    TrApi(("\r\nInit Video layer"));

    /* init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Avoid null pointer for input parameters.          */
    if ( (LayerInitParams_p == NULL) || (LayerInitParams_p->LayerParams_p == NULL) || (LayerProperties_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* memory allocation for common layer data structure */
    LayerProperties_p->PrivateData_p = (void *)STOS_MemoryAllocate(LayerProperties_p->CPUPartition_p, sizeof(HALLAYER_VDPLayerCommonData_t));
    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check if the memory allocation is successful.    */
    if (LayerCommonData_p == NULL)
    {
        return (ST_ERROR_NO_MEMORY);
    }

#if defined(DVD_SECURED_CHIP)
    /* If not yet done, allocate memory and copy the secure filters */
    if(HALLAYER_SecureDataControl.SecureFilters_InitDone == FALSE)
    {
        /* Get AV Mem mapping */
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

        /* Horizontal filters. */
        AvmemAllocParams.Alignment                = HALLAYER_ALIGNMENT_128BYTES;
        AvmemAllocParams.PartitionHandle          = LayerInitParams_p->AVMEM_Partition;
        AvmemAllocParams.Size                     = HALLAYER_SECURE_AREA_SIZE;
        AvmemAllocParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AvmemAllocParams.NumberOfForbiddenRanges  = 0;
        AvmemAllocParams.ForbiddenRangeArray_p    = NULL;
        AvmemAllocParams.NumberOfForbiddenBorders = 0;
        AvmemAllocParams.ForbiddenBorderArray_p   = NULL;

        ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &SecureFiltersBufferBlockHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error handling */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate buffer for the secure filters!"));
            return ST_ERROR_NO_MEMORY;
        }

        ErrorCode = STAVMEM_GetBlockAddress(SecureFiltersBufferBlockHandle, &SecureFiltersBufferVirtualBaseAddress);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error handling */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of Secure Filters Buffer !"));
            return ST_ERROR_NO_MEMORY;
        }

        /* Compute the end of secure filters area */
        SecureFiltersBufferVirtualStopAddress = (void*)((U32)SecureFiltersBufferVirtualBaseAddress+HALLAYER_SECURE_AREA_SIZE-1);

        /* Load the filters into the secure area */
        ErrorCode = stlayer_VDPLoadCoefficientsFilters( (U32*)SecureFiltersBufferVirtualBaseAddress,
                                                        (U32)HALLAYER_SECURE_AREA_SIZE,
                                                        HALLAYER_ALIGNMENT_128BYTES);
        if(ErrorCode != ST_NO_ERROR)
        {
            /* Failed to load filters */
            AvmemFreeParams.PartitionHandle = LayerInitParams_p->AVMEM_Partition;
            STAVMEM_FreeBlock(&AvmemFreeParams, &SecureFiltersBufferBlockHandle);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot load the secure filters!"));
            return(ErrorCode);
        }

       /* Turn the area to secure */
       ErrorCode = STMES_SetSecureRange( STAVMEM_VirtualToDevice(SecureFiltersBufferVirtualBaseAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                         STAVMEM_VirtualToDevice(SecureFiltersBufferVirtualStopAddress,  STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                         SID_SH4_CPU);
       if(ErrorCode != ST_NO_ERROR)
       {
            /* Failed to make the filters as a 'secure' area */
            AvmemFreeParams.PartitionHandle = LayerInitParams_p->AVMEM_Partition;
            STAVMEM_FreeBlock(&AvmemFreeParams, &SecureFiltersBufferBlockHandle);
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to make the filters as a 'secure' area!"));
            return(ErrorCode);
       }

       /* All coef filters are loaded */
       HALLAYER_SecureDataControl.SecureFilters_BlockHandle = SecureFiltersBufferBlockHandle;
       HALLAYER_SecureDataControl.SecureFilters_StartAddress = SecureFiltersBufferVirtualBaseAddress;
       HALLAYER_SecureDataControl.SecureFilters_StopAddress = SecureFiltersBufferVirtualStopAddress;
       HALLAYER_SecureDataControl.SecureFilters_InitDone = TRUE;
    }
#endif /* defined(DVD_SECURED_CHIP) */

    /* Get the layer parameters from init. parameters.  */
    LayerParams_p = LayerInitParams_p->LayerParams_p;

    switch (LayerProperties_p->LayerType)
    {
        case MAIN_DISPLAY :
            /* Check if the layer is not already initialised. */
            if (IsDisp1Initialised)
            {
                ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
            }
            else
            {
                /* memory allocation */
                LayerCommonData_p->ViewportProperties_p = (void *)STOS_MemoryAllocate(LayerProperties_p->CPUPartition_p,
                                                                        MAX_MAIN_LAYER_VIEWPORTS_MAX_NB * sizeof(HALLAYER_ViewportProperties_t));
                /* Check if the memory allocation is successful.    */
                if (LayerCommonData_p->ViewportProperties_p == NULL)
                {
                    /* De-allocate common layer Properties */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerProperties_p->PrivateData_p);
                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    /* Init maximum allowed viewport.   */
                    LayerCommonData_p->MaxViewportNumber = MAX_MAIN_LAYER_VIEWPORTS_MAX_NB;

                    /* Update flag to avoid initialising the same layer */
                    IsDisp1Initialised = TRUE;
                }
            } /* end of not IsDisp1Initialised  */
            break;

        case AUX_DISPLAY :
            /* Check if the layer is not already initialised. */
            if (IsDisp2Initialised)
            {
                ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
            }
            else
            {
                /* memory allocation */
                LayerCommonData_p->ViewportProperties_p = (void *)STOS_MemoryAllocate(LayerProperties_p->CPUPartition_p,
                                                                        MAX_AUX_LAYER_VIEWPORTS_MAX_NB * sizeof(HALLAYER_ViewportProperties_t));
                /* Check if the memory allocation is successful.    */
                if (LayerCommonData_p->ViewportProperties_p == NULL)
                {
                    /* De-allocate common layer Properties */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerProperties_p->PrivateData_p);
                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    /* Init maximum allowed viewport.   */
                    LayerCommonData_p->MaxViewportNumber = MAX_AUX_LAYER_VIEWPORTS_MAX_NB;

                    /* Update flag to avoid initialising the same layer */
                    IsDisp2Initialised = TRUE;
                }
            }
            break;

#if defined (HW_7200)
        case SEC_DISPLAY :
            /* Check if the layer is not already initialised. */
            if (IsDisp3Initialised)
            {
                ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
            }
            else
            {
                /* memory allocation */
                LayerCommonData_p->ViewportProperties_p = (void *)STOS_MemoryAllocate(LayerProperties_p->CPUPartition_p,
                                                                        MAX_SEC_LAYER_VIEWPORTS_MAX_NB * sizeof(HALLAYER_ViewportProperties_t));
                /* Check if the memory allocation is successful.    */
                if (LayerCommonData_p->ViewportProperties_p == NULL)
                {
                    /* De-allocate common layer Properties */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerProperties_p->PrivateData_p);
                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    /* Init maximum allowed viewport.   */
                    LayerCommonData_p->MaxViewportNumber = MAX_SEC_LAYER_VIEWPORTS_MAX_NB;

                    /* Update flag to avoid initialising the same layer */
                    IsDisp3Initialised = TRUE;
                }
            }
            break;
#endif /* HW_7200 */

        default :
            HALLAYER_Error();
            break;
    }
    /* Check for previous setting result.    */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Store the layer dimensions.      */
        LayerCommonData_p->LayerParams = *LayerParams_p;

        /* Store the AVMEM Partition Handle */
#ifdef ST_OSLINUX
        LayerCommonData_p->AvmemPartitionHandle = STAVMEM_GetPartitionHandle( LAYER_PARTITION_AVMEM );
#else
        LayerCommonData_p->AvmemPartitionHandle = LayerInitParams_p->AVMEM_Partition;
#endif
        /* Initialization of all allowed viewports. */
        ViewportProperties_p = (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
        for (ViewportId = 0; ViewportId < ((HALLAYER_VDPLayerCommonData_t *)(LayerCommonData_p))->MaxViewportNumber; ViewportId++)
        {
            ViewportProperties_p[ViewportId].Opened     = FALSE;
            ViewportProperties_p[ViewportId].ViewportId = ViewportId;
        }

        /* Allocate space in SDRAM for the Horizontal and Vertical filters. */
        for (DisplayInstanceIndex = 0; DisplayInstanceIndex < MAX_AVAILABLE_DISPLAY ; DisplayInstanceIndex++)
        {
#if !defined(DVD_SECURED_CHIP)
            /* Get AV Mem mapping  */
            #ifdef ST_OSLINUX
                STAVMEM_GetSharedMemoryVirtualMapping2(LAYER_PARTITION_AVMEM,&VirtualMapping );
            #else
                STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
            #endif

            /* Horizontal filters. */
            AvmemAllocParams.Alignment                  = 128;
            AvmemAllocParams.PartitionHandle            = LayerCommonData_p->AvmemPartitionHandle;
            AvmemAllocParams.Size                       = HFC_NB_COEFS*4*2;
            AvmemAllocParams.AllocMode                  = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AvmemAllocParams.NumberOfForbiddenRanges    = 0;
            AvmemAllocParams.ForbiddenRangeArray_p      = NULL;
            AvmemAllocParams.NumberOfForbiddenBorders   = 0;
            AvmemAllocParams.ForbiddenBorderArray_p     = NULL;

            ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &HorizontalFilterBufferBlockHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate buffer for the horizontal filter!"));

                /* De-allocate Viewport Properties. */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                /* De-allocate last: data inside cannot be used after ! */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                ErrorCode = ST_ERROR_NO_MEMORY;
            }
            else
            {
                ErrorCode = STAVMEM_GetBlockAddress(HorizontalFilterBufferBlockHandle, &HorizontalFilterBufferVirtualAddress);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of Horizontal Filter Buffer !"));

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &HorizontalFilterBufferBlockHandle);

                    /* De-allocate Viewport Properties. */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                    /* De-allocate last: data inside cannot be used after ! */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    #ifdef ST_OSLINUX
                            TrLinux(("HF Virtual Buffer:%x\n",(U32)HorizontalFilterBufferVirtualAddress));
                            TrLinux(("HF CPU Buffer:%x\n",(U32)HorizontalFilterBufferAddress));
                            TrLinux(("HF Device Buffer:%x\n",(U32)STAVMEM_VirtualToDevice(HorizontalFilterBufferVirtualAddress,(U32)&VirtualMapping)));
                    #endif /* ST_OSLINUX */

                    HorizontalFilterBufferAddress = STAVMEM_VirtualToDevice(HorizontalFilterBufferVirtualAddress,&VirtualMapping);

                    /* Store values. */
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddress_p   = HorizontalFilterBufferAddress;
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalBlockHandle = HorizontalFilterBufferBlockHandle;
                }
            }

            /* Alternate Horizontal Buffer */
            AvmemAllocParams.Alignment       = 128;
            AvmemAllocParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
            AvmemAllocParams.Size            = HFC_NB_COEFS*4*2;
            AvmemAllocParams.AllocMode       = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AvmemAllocParams.NumberOfForbiddenRanges    = 0;
            AvmemAllocParams.ForbiddenRangeArray_p      = NULL;
            AvmemAllocParams.NumberOfForbiddenBorders   = 0;
            AvmemAllocParams.ForbiddenBorderArray_p     = NULL;

            ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &AlternateHorizontalFilterBufferBlockHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate alternate buffer for the horizontal filter!"));

                /* Try to free block before leaving */
                AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                STAVMEM_FreeBlock(&AvmemFreeParams, &HorizontalFilterBufferBlockHandle);

                /* De-allocate Viewport Properties. */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                /* De-allocate last: data inside cannot be used after ! */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                ErrorCode = ST_ERROR_NO_MEMORY;
            }
            else
            {
                ErrorCode = STAVMEM_GetBlockAddress(AlternateHorizontalFilterBufferBlockHandle, &AlternateHorizontalFilterBufferVirtualAddress);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of Alternate Horizontal Filter Buffer !"));

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &HorizontalFilterBufferBlockHandle);

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &AlternateHorizontalFilterBufferBlockHandle);

                    /* De-allocate Viewport Properties. */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                    /* De-allocate last: data inside cannot be used after ! */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    #ifdef ST_OSLINUX
                            TrLinux(("HF2 Virtual Buffer:%x\n",(U32)AlternateHorizontalFilterBufferVirtualAddress));
                            TrLinux(("HF2 CPU Buffer:%x\n",(U32)AlternateHorizontalFilterBufferAddress));
                            TrLinux(("HF2 Device Buffer:%x\n",(U32)STAVMEM_VirtualToDevice(AlternateHorizontalFilterBufferVirtualAddress,
                                                            (U32)&VirtualMapping)));
                    #endif /* ST_OSLINUX */

                    AlternateHorizontalFilterBufferAddress = STAVMEM_VirtualToDevice(AlternateHorizontalFilterBufferVirtualAddress,&VirtualMapping);

                    /* Store values. */
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateHorizontalAddress_p   = AlternateHorizontalFilterBufferAddress;
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateHorizontalBlockHandle = AlternateHorizontalFilterBufferBlockHandle;
                }
            }

            /* Vertical filters */
            AvmemAllocParams.Alignment       = 128;
            AvmemAllocParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
            AvmemAllocParams.Size            = VFC_NB_COEFS*4*2;
            AvmemAllocParams.AllocMode       = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AvmemAllocParams.NumberOfForbiddenRanges    = 0;
            AvmemAllocParams.ForbiddenRangeArray_p      = NULL;
            AvmemAllocParams.NumberOfForbiddenBorders   = 0;
            AvmemAllocParams.ForbiddenBorderArray_p     = NULL;

            ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &VerticalFilterBufferBlockHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate buffer for the Vertical filter!"));

                /* Try to free block before leaving */
                AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                STAVMEM_FreeBlock(&AvmemFreeParams, &HorizontalFilterBufferBlockHandle);

                /* Try to free block before leaving */
                AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                STAVMEM_FreeBlock(&AvmemFreeParams, &AlternateHorizontalFilterBufferBlockHandle);

                /* De-allocate Viewport Properties. */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                /* De-allocate last: data inside cannot be used after ! */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                ErrorCode = ST_ERROR_NO_MEMORY;
            }
            else
            {
                ErrorCode = STAVMEM_GetBlockAddress(VerticalFilterBufferBlockHandle, &VerticalFilterBufferVirtualAddress);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of Vertical Filter Buffer !"));

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &VerticalFilterBufferBlockHandle);

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &HorizontalFilterBufferBlockHandle);

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &AlternateHorizontalFilterBufferBlockHandle);

                    /* De-allocate Viewport Properties. */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                    /* De-allocate last: data inside cannot be used after ! */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    #ifdef ST_OSLINUX
                            TrLinux(("VF Virtual Buffer:%x\n",(U32)VerticalFilterBufferVirtualAddress));
                            TrLinux(("VF CPU Buffer:%x\n",(U32)VerticalFilterBufferAddress));
                            TrLinux(("VF Device Buffer:%x\n",(U32)STAVMEM_VirtualToDevice(VerticalFilterBufferVirtualAddress,(U32)&VirtualMapping)));
                    #endif /* ST_OSLINUX */

                    VerticalFilterBufferAddress = STAVMEM_VirtualToDevice(VerticalFilterBufferVirtualAddress,&VirtualMapping);

                    /* Store values. */
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddress_p   = VerticalFilterBufferAddress;
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalBlockHandle = VerticalFilterBufferBlockHandle;
                }
            }

            /* Alternate Vertical filters */
            AvmemAllocParams.Alignment                  = 128;
            AvmemAllocParams.PartitionHandle            = LayerCommonData_p->AvmemPartitionHandle;
            AvmemAllocParams.Size                       = VFC_NB_COEFS*4*2;
            AvmemAllocParams.AllocMode                  = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AvmemAllocParams.NumberOfForbiddenRanges    = 0;
            AvmemAllocParams.ForbiddenRangeArray_p      = NULL;
            AvmemAllocParams.NumberOfForbiddenBorders   = 0;
            AvmemAllocParams.ForbiddenBorderArray_p     = NULL;

            ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &AlternateVerticalFilterBufferBlockHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate Alternate buffer for the Vertical filter!"));

                /* Try to free block before leaving */
                AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                STAVMEM_FreeBlock(&AvmemFreeParams, &AlternateHorizontalFilterBufferBlockHandle);

                /* Try to free block before leaving */
                AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                STAVMEM_FreeBlock(&AvmemFreeParams, &HorizontalFilterBufferBlockHandle);

                /* De-allocate Viewport Properties. */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                /* De-allocate last: data inside cannot be used after ! */
                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                ErrorCode = ST_ERROR_NO_MEMORY;
            }
            else
            {
                ErrorCode = STAVMEM_GetBlockAddress(AlternateVerticalFilterBufferBlockHandle, &AlternateVerticalFilterBufferVirtualAddress);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of Alternate Vertical Filter Buffer !"));

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &VerticalFilterBufferBlockHandle);

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &AlternateVerticalFilterBufferBlockHandle);

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &HorizontalFilterBufferBlockHandle);

                    /* Try to free block before leaving */
                    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
                    STAVMEM_FreeBlock(&AvmemFreeParams, &AlternateHorizontalFilterBufferBlockHandle);

                    /* De-allocate Viewport Properties. */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

                    /* De-allocate last: data inside cannot be used after ! */
                    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

                    ErrorCode = ST_ERROR_NO_MEMORY;
                }
                else
                {
                    #ifdef ST_OSLINUX
                            TrLinux(("VF2 Virtual Buffer:%x\n",(U32)AlternateVerticalFilterBufferVirtualAddress));
                            TrLinux(("VF2 CPU Buffer:%x\n",(U32)AlternateVerticalFilterBufferAddress));
                            TrLinux(("VF2 Device Buffer:%x\n",(U32)STAVMEM_VirtualToDevice(AlternateVerticalFilterBufferVirtualAddress,
                                                            (U32)&VirtualMapping)));
                    #endif /* ST_OSLINUX */

                    AlternateVerticalFilterBufferAddress = STAVMEM_VirtualToDevice(AlternateVerticalFilterBufferVirtualAddress,&VirtualMapping);

                    /* Store values. */
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateVerticalAddress_p   = AlternateVerticalFilterBufferAddress;
                    LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateVerticalBlockHandle = AlternateVerticalFilterBufferBlockHandle;
                }
            }
#endif /* !defined(DVD_SECURED_CHIP) */

            /* Init */
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalLumaFilter    = VDP_FILTER_NO_SET;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalChromaFilter  = VDP_FILTER_NO_SET;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalLumaFilter      = VDP_FILTER_NO_SET;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalChromaFilter    = VDP_FILTER_NO_SET;
#if defined(DVD_SECURED_CHIP)
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddress_p = HALLAYER_SecureDataControl.SecureFilters_StartAddress;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateHorizontalAddress_p = HALLAYER_SecureDataControl.SecureFilters_StartAddress;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddress_p = HALLAYER_SecureDataControl.SecureFilters_StartAddress;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateVerticalAddress_p = HALLAYER_SecureDataControl.SecureFilters_StartAddress;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalBlockHandle = (STAVMEM_BlockHandle_t)NULL;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateHorizontalBlockHandle = (STAVMEM_BlockHandle_t)NULL;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalBlockHandle = (STAVMEM_BlockHandle_t)NULL;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateVerticalBlockHandle = (STAVMEM_BlockHandle_t)NULL;
#endif /* defined(DVD_SECURED_CHIP) */

            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressUsedByHw_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddress_p;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressAvailForWritting_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateHorizontalAddress_p;

            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressUsedByHw_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddress_p;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressAvailForWritting_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateVerticalAddress_p;
        } /* end of the for loop for Display1 and Display2 */

        /* Default initializations of private data. */
        LayerCommonData_p->LayerPosition.XStart  = 0;
        LayerCommonData_p->LayerPosition.YStart  = 0;
        LayerCommonData_p->LayerPosition.XOffset = 0;
        LayerCommonData_p->LayerPosition.YOffset = 0;

        LayerCommonData_p->VTGFrameRate          = 0;
        strcpy(LayerCommonData_p->VTGName, "");

        LayerCommonData_p->LumaDataRateInBypassMode = 0;
        LayerCommonData_p->VSyncCounter = 0;

        LayerCommonData_p->ViewPortParams.InputRectangle.PositionX  = 0;
        LayerCommonData_p->ViewPortParams.InputRectangle.PositionY  = 0;
        LayerCommonData_p->ViewPortParams.InputRectangle.Width      = 0;
        LayerCommonData_p->ViewPortParams.InputRectangle.Height     = 0;

        LayerCommonData_p->ViewPortParams.OutputRectangle.PositionX = 0;
        LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY = 0;
        LayerCommonData_p->ViewPortParams.OutputRectangle.Width     = LayerParams_p->Width;
        LayerCommonData_p->ViewPortParams.OutputRectangle.Height    = LayerParams_p->Height;

        LayerCommonData_p->CurrentPictureHorizontalDecimationFactor = 1;
        LayerCommonData_p->CurrentPictureVerticalDecimationFactor = 1;

        LayerCommonData_p->IsViewportEnabled                        = FALSE;
        LayerCommonData_p->ViewportOffset                           = 0;
        LayerCommonData_p->ViewportSize                             = 0;
        LayerCommonData_p->UpdateViewport                           = FALSE;

        LayerCommonData_p->SourcePictureDimensions.Width            = 0;
        LayerCommonData_p->SourcePictureDimensions.Height           = 0;

        LayerCommonData_p->IsPeriodicalUpdateNeeded     = FALSE;
        LayerCommonData_p->ReloadOfHorizontalFiltersInProgress = FALSE;
        LayerCommonData_p->ReloadOfVerticalFiltersInProgress = FALSE;
        LayerCommonData_p->FilterCoeffsReloaded = FALSE;

#ifdef DBG_FILTER_COEFFS
        LayerCommonData_p->DbgLastU32OfHorizontalLumaBuffer = 0;
        LayerCommonData_p->DbgLastU32OfVerticalLumaBuffer = 0;
#endif
        LayerCommonData_p->AdvancedHDecimation                      = FALSE;

#ifdef DBG_USE_MANUAL_VHSRC_PARAMS
        LayerCommonData_p->ManualParams = FALSE;
#endif

#ifdef DBG_FIELD_INVERSIONS
        LayerCommonData_p->OldCurrentField.PictureIndex = 0;
        LayerCommonData_p->FieldsWithFieldInversion = 0;
        LayerCommonData_p->FieldsSetOnOddInputPosition = 0;
        LayerCommonData_p->FieldsSetOnOddOutputPosition = 0;
#endif /* DBG_FIELD_INVERSIONS */

        /* Init VDP DEI Layer */
        vdp_dei_Init (LayerHandle);
#ifdef STLAYER_USE_FMD
        /* Init FMD */
        vdp_fmd_Init(LayerHandle);
#endif
        /* Init PSI regs */
        ErrorCode = VDPInitFilter(LayerHandle);
    }

    /* Return the current error code.   */
    return(ErrorCode);

} /* end of Init() function */


/*******************************************************************************
Name        : GetCapability
Description : Gets the capabilities of the HAL layer.
Parameters  : IN  - HAL layer manager handle
              OUT - Capability to return.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetCapability(const STLAYER_Handle_t    LayerHandle,
                         STLAYER_Capability_t *     Capability_p)
{
    HALLAYER_LayerProperties_t *  LayerProperties_p;

    /* init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Check if any input parameter is unavailable */
    if ((LayerProperties_p != NULL) && (Capability_p != NULL))
    {
        Capability_p->MultipleViewPorts  = FALSE;
        Capability_p->HorizontalResizing = TRUE;
        Capability_p->AlphaBorder = FALSE;
        Capability_p->GlobalAlpha = FALSE;
        Capability_p->ColorKeying = FALSE;
        Capability_p->MultipleViewPortsOnScanLineCapable = FALSE;
        Capability_p->LayerType = LayerProperties_p->LayerType;
    }

} /* End of GetCapability() function */


/*******************************************************************************
Name        : OpenViewport
Description : Opens a new Viewport of the specified layer.
Parameters  : IN  - HAL layer manager handle
              IN  - Viewport params
              OUT - Handle of the opened viewport.
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - STLAYER_ERROR_NO_FREE_HANDLES, if no more viewport available.
                - ST_ERROR_BAD_PARAMETER, if one parameter is false.
                - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t OpenViewport(const STLAYER_Handle_t   LayerHandle,
                            STLAYER_ViewPortParams_t *      const Params_p,
                            STLAYER_ViewPortHandle_t *      ViewportHandle_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    STGXOBJ_Rectangle_t             InputRectangle, OutputRectangle;
    U32                             SourcePictureHorizontalSize, SourcePictureVerticalSize, PanVector, ScanVector;
    U32                             InputHorizontalSize, InputVerticalSize;
    U32                             OutputPositionX, OutputPositionY, OutputHorizontalSize, OutputVerticalSize;
    STGXOBJ_ColorType_t             ColorType;
    U8                              ViewportId = 0;

    TrApi(("\r\nOpenViewport"));

    /* Check if any input parameter is unavailable */
    if ((ViewportHandle_p == NULL) || (Params_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve the first free viewport */
    while ((ViewportProperties_p[ViewportId].Opened) && (ViewportId < LayerCommonData_p->MaxViewportNumber))
    {
        ViewportId ++;
    }

    /* Test if a new viewport can be opened */
    if (ViewportId >= LayerCommonData_p->MaxViewportNumber)
    {
        /* No more free handles found */
        return (STLAYER_ERROR_NO_FREE_HANDLES);
    }

    /*- Input Window Verifications -------------------------------------------*/
    /*------------------------------------------------------------------------*/
    InputRectangle  = Params_p->InputRectangle;
    OutputRectangle = Params_p->OutputRectangle;
    PanVector       = InputRectangle.PositionX / 16;
    ScanVector      = InputRectangle.PositionY / 16;

    SourcePictureHorizontalSize = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
    SourcePictureVerticalSize   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;
    ColorType                   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.ColorType;

    /* PanVector can not be greater than decoded image horizontal size & ScanVector can not be greater than decoded image vertical size */
    if ((PanVector > SourcePictureHorizontalSize) || (ScanVector > SourcePictureVerticalSize))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    InputHorizontalSize = InputRectangle.Width;
    InputVerticalSize   = InputRectangle.Height;

    /*- Output Window Verifications ------------------------------------------*/
    /*------------------------------------------------------------------------*/
    OutputHorizontalSize= OutputRectangle.Width;
    OutputVerticalSize  = OutputRectangle.Height;

    OutputPositionX     = OutputRectangle.PositionX;
    OutputPositionY     = OutputRectangle.PositionY;

    /* Test if output Window Position out of Layer Dimensions, or if risk of division by 0 */
    if ((OutputPositionX > LayerCommonData_p->LayerParams.Width) || (OutputPositionY > LayerCommonData_p->LayerParams.Height) ||
         (InputHorizontalSize == 0) || (InputVerticalSize == 0))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* --------- Viewport initializations --------- */
    /* Viewport private parameters.                 */
    ViewportProperties_p[ViewportId].AssociatedLayerHandle  = LayerHandle;
    ViewportProperties_p[ViewportId].Opened                 = TRUE;
    ViewportProperties_p[ViewportId].ViewportId             = ViewportId;
    /* And its handle is available.                 */
    *ViewportHandle_p = (STLAYER_ViewPortHandle_t)(&(ViewportProperties_p[ViewportId]));

    /* Viewport Parameters are OK : memorize them.  */
    LayerCommonData_p->ViewPortParams = *Params_p;

    /* Layer Parameters are OK : memorize them.     */
    LayerCommonData_p->SourcePictureDimensions.Width  = SourcePictureHorizontalSize;
    LayerCommonData_p->SourcePictureDimensions.Height = SourcePictureVerticalSize;
    LayerCommonData_p->ColorType                      = ColorType;

    /* Initialize the main registers to work !      */
    InitializeVideoProcessor (LayerHandle);

    /* Sets the source of the viewport. */
    /* Remember the viewport ID.             */
    ViewportProperties_p[ViewportId].SourceNumber = Params_p->Source_p->Data.VideoStream_p->SourceNumber;

    /* Init of STLAYER_StreamingVideo_t PresentedFieldInverted parameter */
    LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->PresentedFieldInverted = 0;

    /* Sets the color space conversion mode */
    if (SetColorSpaceConversionMode(LayerProperties_p, Params_p->Source_p->Data.VideoStream_p->BitmapParams.ColorSpaceConversion)!=ST_NO_ERROR)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Update Viewport parameters                   */
    SetViewportParams(*ViewportHandle_p, Params_p);

    /* No error to return.                          */
    return (ST_NO_ERROR);
} /* End of OpenViewport() function */

/*******************************************************************************
Name        : CloseViewport
Description : Closes a Viewport
Parameters  : HAL Viewport manager handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t CloseViewport(const STLAYER_ViewPortHandle_t ViewportHandle)
{
    HALLAYER_ViewportProperties_t * ViewportProperties;

    TrApi(("\r\nCloseViewport"));

    /* init viewport properties.    */
    ViewportProperties = (HALLAYER_ViewportProperties_t *)(ViewportHandle);

    /* Check NULL parameters.                                           */
    if (ViewportProperties == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Just close the viewport.     */
    ViewportProperties->Opened = FALSE;

    /* No error to return.          */
    return (ST_NO_ERROR);

} /* End of CloseViewport() function */


/*******************************************************************************
Name        : Term
Description : Deallocate the HAL_LAYER properties value.
Parameters  : HAL Layer manager handle
              Terms Params
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_ERROR_OPEN_HANDLE, if one viewport is still open.
                - ST_ERROR_BAD_PARAMETER if null pointer.
                - ST_ERROR_UNKNOWN_DEVICE if layer not initialized.
                - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t Term(const STLAYER_Handle_t       LayerHandle,
                    const STLAYER_TermParams_t*         TermParams_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    STLAYER_Layer_t                 LayerType;
    U8                              ViewPortId;
#if !defined(DVD_SECURED_CHIP)
    U8                              DisplayInstanceIndex;
    STAVMEM_FreeBlockParams_t       AvmemFreeParams;
#endif /* !defined(DVD_SECURED_CHIP) */

    TrApi(("\r\nTerm Video layer"));

    /* Check if any input parameter is unavailable */
    if (TermParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* init Layer and viewport properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerType = LayerProperties_p->LayerType;

    /* Check if layer is initialized correctly. */
    if (((LayerType == MAIN_DISPLAY) && (!IsDisp1Initialised)) ||
#if defined (HW_7200)
    ((LayerType == SEC_DISPLAY) && (!IsDisp3Initialised)) ||
#endif /* HW_7200 */
        ((LayerType == AUX_DISPLAY)  && (!IsDisp2Initialised)))
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Check if parameter is correct. */
    if (TermParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    for (ViewPortId = 0; ViewPortId < LayerCommonData_p->MaxViewportNumber; ViewPortId ++)
    {
         /* if TempParams_p == ForceTerm then close all the viewports */
         if (TermParams_p->ForceTerminate)
         {
             CloseViewport((STLAYER_ViewPortHandle_t)(&ViewportProperties_p[ViewPortId]));
         }
         /* else verify that all the viewports are closed */
         else
         {
              if (ViewportProperties_p[ViewPortId].Opened)
              {
                  return (ST_ERROR_OPEN_HANDLE);
              }
         }
    }

#ifdef STLAYER_USE_FMD
    vdp_fmd_Term (LayerHandle);
#endif
    vdp_dei_Term(LayerHandle);

#if !defined(DVD_SECURED_CHIP)
    /* Free memory used by the filters. */
    AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
    for (DisplayInstanceIndex = 0; DisplayInstanceIndex < MAX_AVAILABLE_DISPLAY ; DisplayInstanceIndex++)
    {
        STAVMEM_FreeBlock(&AvmemFreeParams,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalBlockHandle);
        STAVMEM_FreeBlock(&AvmemFreeParams,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalBlockHandle);
        STAVMEM_FreeBlock(&AvmemFreeParams,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateHorizontalBlockHandle);
        STAVMEM_FreeBlock(&AvmemFreeParams,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.AlternateVerticalBlockHandle);
     }
#else /* DVD_SECURED_CHIP */
    /* On secure platforms, the filters have been loaded in a secure area and cannot */
    /* be deallocated anymore: the filers area is kept as is for future 'Init' calls */
#endif /* DVD_SECURED_CHIP */

    /* De-allocate Viewport Properties. */
    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

    /* De-allocate last: data inside cannot be used after ! */
    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

    /* Update flag to allow a new init for the layer type. */
    switch (LayerType)
    {
        case MAIN_DISPLAY :
            IsDisp1Initialised = FALSE;
            break;

        case AUX_DISPLAY :
            IsDisp2Initialised = FALSE;
            break;

#if defined (HW_7200)
        case SEC_DISPLAY :
            IsDisp3Initialised = FALSE;
            break;
#endif /* HW_7200 */

        default :
            break;
    }

    /* No error to return. */
    return(ST_NO_ERROR);

} /* End of Term() function */


/*******************************************************************************
Name        : EnableVideoViewport
Description : Enables a viewport in the main display window.
Parameters  : HAL Viewport manager handle.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableVideoViewport(const STLAYER_ViewPortHandle_t   ViewportHandle)
{
   HALLAYER_LayerProperties_t * LayerProperties_p;
   HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    TrApi(("\r\nEnableVideoViewport"));

    /* Check null pointer                               */
    if ((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL)
    {
        return;
    }

    /* init Layer properties.       */
   LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    /* init Layer properties.                   */
    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    TrViewport(("\r\nEnableVideoViewport"));

    /* Save this state.     */
    LayerCommonData_p->IsViewportEnabled = TRUE;
    LayerCommonData_p->UpdateViewport = TRUE;

    /* Set the viewport rectangle dimensions */
    SetViewportOutputRectangle(LayerProperties_p);

    /* Now enable the VHSRC module */
    HAL_SetRegister32DefaultValue( (void *) (((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p) + VDP_VHSRC_CTL),
                                   ((U32)1 << 5),
                                   ((U32)1 << 5));

} /* End of EnableVideoViewport() function */

/*******************************************************************************
Name        : DisableVideoViewport
Description : Disables a viewport in the main display window.
Parameters  : HAL viewport manager handle.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableVideoViewport(const STLAYER_ViewPortHandle_t   ViewportHandle)
{
    HALLAYER_LayerProperties_t *        LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *     LayerCommonData_p;
    U32                                 VpoRegisterValue;

    TrApi(("\r\nDisableVideoViewport"));

    /* Check null pointer                               */
    if ((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL)
    {
        return;
    }

    /* init Layer properties.       */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    /* init Layer properties.                   */
    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    TrViewport(("\r\nDisableVideoViewport"));

    /* Save this state.     */
    LayerCommonData_p->IsViewportEnabled = FALSE;
    LayerCommonData_p->UpdateViewport = TRUE;

    /* Set the viewport rectangle dimension */
    SetViewportOutputRectangle(LayerProperties_p);

    /* Disable Video layer in Compositor:
       The output viewport should be disabled immediately because no VSync will happen anymore.
       This is done by setting a null video viewport size in the Compositor (Viewport Origin = Viewport Stop) */
    VpoRegisterValue = HAL_ReadGam32(GAM_VIDn_VPO);
    HAL_WriteGam32(GAM_VIDn_VPS, VpoRegisterValue);

    /* Disable DEI */
    vdp_dei_SetDeiMode(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle, VDP_DEI_SET_MODE_OFF);

    /* Disable VHSRC */
    HAL_WriteDisp32(VDP_VHSRC_CTL, 0);

} /* End of DisableVideoViewport() function */

/*******************************************************************************
Name        : SetViewportPriority
Description : Sets the viewport priority display in the layer.
Parameters  : HAL viewport manager handle
              viewport priority
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetViewportPriority(const STLAYER_ViewPortHandle_t  ViewportHandle,
                         const HALLAYER_ViewportPriority_t      Priority)
{
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(Priority);

    return;
} /* End of SetViewportPriority() function */

/*******************************************************************************
Name        : SetViewportSource
Description : Sets the characteristics of the source for the specified viewport
              (Source dimensions, source (Video 1,..., SD1, ...)).
              Calculate repeat and phase each when used as callback from VIDDISP
Parameters  : HAL viewport manager handle.
              Data of the source.
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no error occurs.
                - ST_ERROR_FEATURE_NOT_SUPPORTED if wrong color space conversion
*******************************************************************************/
static ST_ErrorCode_t SetViewportSource(const STLAYER_ViewPortHandle_t  ViewportHandle,
                       const STLAYER_ViewPortSource_t *                 Source_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t    * LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;

    /* Check null pointer                               */
    if ((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Init Layer/Viewport properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
    /* Check null pointer                               */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve STLAYER_StreamingVideo_t PresentedFieldInverted parameter */
    /* Warning: Most of the variables in "Source_p->Data.VideoStream_p" contains a dummy content.
                       That's why we don't do a memcpy but only copy the necessary variables */
    LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->PresentedFieldInverted = Source_p->Data.VideoStream_p->PresentedFieldInverted;

    /* Set The Advanced decimation mode */
    LayerCommonData_p->AdvancedHDecimation = Source_p->Data.VideoStream_p->AdvancedHDecimation;

    STOS_memcpy( &LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->PreviousField,
                    &Source_p->Data.VideoStream_p->PreviousField,
                    sizeof(STLAYER_Field_t) );

    STOS_memcpy( &LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField,
                    &Source_p->Data.VideoStream_p->CurrentField,
                    sizeof(STLAYER_Field_t) );

    STOS_memcpy( &LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->NextField,
                    &Source_p->Data.VideoStream_p->NextField,
                    sizeof(STLAYER_Field_t) );

    /* Calculating Polyphase */
    ComputeAndApplyThePolyphaseFilters(ViewportProperties_p->AssociatedLayerHandle,
                    &LayerCommonData_p->ViewPortParams,
                    Source_p->Data.VideoStream_p->HorizontalDecimationFactor,
                    Source_p->Data.VideoStream_p->VerticalDecimationFactor);

    /* No error to return.          */
    return (ST_NO_ERROR);

} /* End of SetViewportSource() function */


/*******************************************************************************
Name        : SetColorSpaceConversionMode
Description : Set color space conversion settings.
Parameters  : HAL layer properties : LayerProperties_p
              Color space conversion mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED or ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t SetColorSpaceConversionMode(
        const HALLAYER_LayerProperties_t * const LayerProperties_p,
        const STGXOBJ_ColorSpaceConversionMode_t ColorSpaceConversionMode)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    #if defined (HW_7109)
    U32 Temp32;
    U32 CutId;
    #endif

    /* To differentiate between 7109 cut2.0 and cut3.0 as we have to support both cuts simultaneously */
    #if defined (HW_7109)
    CutId = ST_GetCutRevision();
    #endif

    switch (ColorSpaceConversionMode)
    {
        case STGXOBJ_ITU_R_BT470_2_M:
        case STGXOBJ_ITU_R_BT470_2_BG:
        case STGXOBJ_SMPTE_170M:
        case STGXOBJ_FCC:
        case STGXOBJ_ITU_R_BT601 :
            #if defined (HW_7109)
            if (CutId < 0xC0)
            {
                /* Embedded conversion matrix for 7109 cut 2.0: just reset bit 25 of GAM_VIDn_CTL */
                Temp32 = HAL_Read32((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_CTL);
                HAL_WriteGam32(GAM_VIDn_CTL, Temp32 & (~GAM_VIDn_CTL_709NOT601));
            }
            else
            {
            #endif
                /* 7109 cut 3.0 and above */
                /* HAL_SetRegister32DefaultValue does not change the reserved bits */
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR1,
                                    GAM_VIDn_MPR1_MASK,
                                    0x0a800000);
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR2,
                                    GAM_VIDn_MPR2_MASK,
                                    0x0aaf0000);
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR3,
                                    GAM_VIDn_MPR3_MASK,
                                    0x094e0754);
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR4,
                                    GAM_VIDn_MPR4_MASK,
                                    0x00000add);
            #if defined (HW_7109)
            }
            #endif
            break;

        case STGXOBJ_SMPTE_240M:
        case STGXOBJ_ITU_R_BT709 :
            #if defined (HW_7109)
            if (CutId < 0xC0)
            {
                /* Embedded conversion matrix for 7109 cut 2.0: just set bit 25 of GAM_VIDn_CTL */
                Temp32 = HAL_Read32((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_CTL);
                HAL_WriteGam32(GAM_VIDn_CTL, Temp32 | GAM_VIDn_CTL_709NOT601);
            }
            else
            {
            #endif
                /* 7109 cut 3.0 and above */
                /* HAL_SetRegister32DefaultValue does not change the reserved bits */
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR1,
                                    GAM_VIDn_MPR1_MASK,
                                    0x0a800000);
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR2,
                                    GAM_VIDn_MPR2_MASK,
                                    0x0ac50000);
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR3,
                                    GAM_VIDn_MPR3_MASK,
                                    0x07160545);
                HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_MPR4,
                                    GAM_VIDn_MPR4_MASK,
                                    0x00000ae8);
               #if defined (HW_7109)
            }
            #endif
            break;

        case STGXOBJ_CONVERSION_MODE_UNKNOWN:
            /* Do not change the default matrix (set to CCIR 709) */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;

        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    /* OK, return a good status */
    return(ErrorCode);

} /* End of SetColorSpaceConversionMode function */


/*******************************************************************************
Name        : GetViewportSource
Description : Gets the source of the specified viewport (Video 1,..., SD1, ...).
Parameters  : IN  - HAL viewport manager handle.
              OUT - Source number (compatible with register).
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetViewportSource(const STLAYER_ViewPortHandle_t    ViewportHandle,
                               U32 *                            SourceNumber_p)
{
    /* Check null pointer                               */
    if ((SourceNumber_p == NULL) || ((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL))
    {
        return;
    }

    /* Retrieve source number */
    *SourceNumber_p = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->SourceNumber;

} /* End of GetViewportSource() function */

/*******************************************************************************
Name        : SetViewportSpecialMode
Description : Sets special mode parameters.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t SetViewportSpecialMode(
                         const STLAYER_ViewPortHandle_t                   ViewportHandle,
                         const STLAYER_OutputMode_t                       OuputMode,
                         const STLAYER_OutputWindowSpecialModeParams_t *  const Params_p)
{
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(OuputMode);
    UNUSED_PARAMETER(Params_p);

    /* Not supported for the moment */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

} /* End of SetViewportSpecialMode() function. */

/*******************************************************************************
Name        : GetViewportSpecialMode
Description : Get special mode parameters.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetViewportSpecialMode (
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_OutputMode_t *             const OuputMode_p,
                         STLAYER_OutputWindowSpecialModeParams_t * const Params_p)
{
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(OuputMode_p);
    UNUSED_PARAMETER(Params_p);

    /* Not supported for the moment */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

} /* End of GetViewportSpecialMode() function. */

/*******************************************************************************
Name        : SetLayerParams
Description : Sets layer parameters.
Parameters  : HAL layer manager handle, Layer parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetLayerParams(const STLAYER_Handle_t      LayerHandle,
                    STLAYER_LayerParams_t * const      LayerParams_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t    * LayerCommonData_p;
    STLAYER_UpdateParams_t          UpdateParams;
    STLAYER_LayerParams_t           LayerParams;

    /* init Layer properties.  */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p != NULL)
    {
        LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
        /* Check null pointer                               */
        if (LayerCommonData_p != NULL)
        {
            /* Check that the pointer is not null. */
            if (LayerParams_p == NULL)
            {
                HALLAYER_Error();
            }
            else
            {
                /* Check if at least, one parameter changed */
                if ((LayerCommonData_p->LayerParams.AspectRatio  != LayerParams_p->AspectRatio) ||
                    (LayerCommonData_p->LayerParams.ScanType     != LayerParams_p->ScanType)    ||
                    (LayerCommonData_p->LayerParams.Width        != LayerParams_p->Width)       ||
                    (LayerCommonData_p->LayerParams.Height       != LayerParams_p->Height))
                {

                    /* Store the Layer Parameters in our local structure */
                    LayerCommonData_p->LayerParams          = *LayerParams_p;

                    /* Notify event HALLAYER_UPDATE_PARAMS_EVT_ID           */
                    LayerParams                             = *LayerParams_p;
                    UpdateParams.LayerParams_p              = &LayerParams;
                    UpdateParams.UpdateReason               = STLAYER_LAYER_PARAMS_REASON;
                    STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle,
                            ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID],
                            (const void *) (&UpdateParams));
                }
            }
        }
    }
} /* End of SetLayerParams() function */

/*******************************************************************************
Name        : GetLayerParams
Description : Gets layer parameters.
Parameters  : IN  - HAL Layer manager handle.
              OUT - Pointer on Layer Params structure.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetLayerParams(const STLAYER_Handle_t       LayerHandle,
                           STLAYER_LayerParams_t *      LayerParams_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;

    /* Check null pointer                               */
    if (LayerParams_p == NULL)
    {
        return;
    }

    /* init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    *LayerParams_p = LayerCommonData_p->LayerParams;

} /* End of GetLayerParams() function */

/*******************************************************************************
Name        : SetViewportPosition
Description : Sets the viewport position.
Parameters  : ViewportHandle
              XPos            the horizontal position of this source
              YPos            the vertical position of this source
Assumptions :
Limitations : Only available for the main display window.
Returns     :
*******************************************************************************/
static void SetViewportPosition(const STLAYER_ViewPortHandle_t   ViewportHandle,
                                const U32                        ViewportXPosition,
                                const U32                        ViewportYPosition)
{
    HALLAYER_LayerProperties_t * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    /* Init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

#ifdef VIDEO_HALLAYER_DEBUG
    if ( (ViewportYPosition != 0) || (ViewportXPosition != 0) )
    {
        HALLAYER_Error();
    }
#endif

    LayerCommonData_p->ViewPortParams.OutputRectangle.PositionX = ViewportXPosition;
    LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY = ViewportYPosition;
} /* end of SetViewportPosition() function */

/*******************************************************************************
Name        : GetViewportPosition
Description : Gets the position of a given viewport
Parameters  : HAL viewport manager handle  : ViewportHandle
              XPos            the horizontal position of this source
              YPos            the vertical position of this source
Assumptions :
Limitations : Only available for the main display window.
Returns     :
*******************************************************************************/
static void GetViewportPosition(const STLAYER_ViewPortHandle_t   ViewportHandle,
                                U32 *                            XPos_p,
                                U32 *                            YPos_p)
{
    HALLAYER_LayerProperties_t * LayerProperties_p;

    /* Check null pointer                               */
    if ((XPos_p == NULL) || (YPos_p == NULL))
    {
        return;
    }

    /* Check null pointer                               */
    if ((HALLAYER_ViewportProperties_t *)(ViewportHandle))
    {
        return;
    }

    /* Init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    *XPos_p *= 16 ;
    *YPos_p *= 16;
} /* End of GetViewportPosition() function */

/*******************************************************************************
Name        : SetViewportPSI
Description : Set the PSI parameters.
Parameters  : ViewportHandle
              Params
Assumptions :
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED.
*******************************************************************************/
static ST_ErrorCode_t SetViewportPSI(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_PSI_t * const        ViewportPSI_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;
    U32                             ActivityMask;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Check NULL parameters                                            */
    if (((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL) || (ViewportPSI_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ActivityMask = LayerCommonData_p->VDPPSI_Parameters.ActivityMask;
    ErrorCode = VDPSetViewportPsiParameters(LayerHandle, ViewportPSI_p);
    if ((ActivityMask != LayerCommonData_p->VDPPSI_Parameters.ActivityMask) && (ErrorCode == ST_NO_ERROR))
    {
        SetColorSpaceConversionMode(LayerProperties_p,
                     LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->BitmapParams.ColorSpaceConversion);
    }

    /* Return the current error status.     */
    return(ErrorCode);

} /* End of SetViewportPSI() function. */

/*******************************************************************************
Name        : GetViewportParams
Description : Retrieve the ViewPort parameters.
Parameters  : HAL viewport manager handle  : ViewportHandle
              Parameters to be returned : Params
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t GetViewportParams(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;

    /* Check null pointer                               */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;
    /* Check null pointer                               */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *Params_p = LayerCommonData_p->ViewPortParams;

    /* No error to return */
    return(ST_NO_ERROR);

} /* End of GetViewportParams() function */

/*******************************************************************************
Name        : GetViewportPSI
Description : Retrieve the PSI parameters.
Parameters  : ViewportHandle
              Params
Assumptions :
Limitations :
Returns     : ST_ERROR_FEATURE_NOT_SUPPORTED.
*******************************************************************************/
static ST_ErrorCode_t GetViewportPSI(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_PSI_t * const              ViewportPSI_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Check NULL parameters                                            */
    if (((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL) || (ViewportPSI_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get the current layer PSI parameters. */
    ErrorCode = VDPGetViewportPsiParameters(LayerHandle, ViewportPSI_p);

    return(ErrorCode);
} /* End of GetViewportPSI() function. */

/*******************************************************************************
Name        : GetViewportIORectangle
Description : Retrieve the ViewPort IO rectangle parameters.
Parameters  : HAL viewport manager handle  : ViewportHandle
              Parameters to be returned : InputRectangle and OutputRectangle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t GetViewportIORectangle( const STLAYER_ViewPortHandle_t     ViewportHandle,
                                             STGXOBJ_Rectangle_t * InputRectangle_p,
                                             STGXOBJ_Rectangle_t * OutputRectangle_p)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;

    /* Check null pointer                               */
    if ((InputRectangle_p == NULL) || (OutputRectangle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;
    /* Check null pointer                               */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *InputRectangle_p  = LayerCommonData_p->ViewPortParams.InputRectangle;
    *OutputRectangle_p = LayerCommonData_p->ViewPortParams.OutputRectangle;

    /* No error to return */
    return(ST_NO_ERROR);

} /* End of GetViewportIORectangle() function */


/*******************************************************************************
Name        : AdjustViewportParams
Description : Adjust viewport parameters according to the hardware constraints.
Parameters  :   IN   - HAL layer manager handle
              IN/OUT - Viewport parameters.
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no error occurs.
                - ST_ERROR_BAD_PARAMETER, if one of parameters is bad.
*******************************************************************************/
static ST_ErrorCode_t AdjustViewportParams(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_ViewPortParams_t * const   Params_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Check null pointer                               */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if any input parameter is unavailable */
    if ( (Params_p->InputRectangle.Width == 0) || (Params_p->InputRectangle.Height  == 0) ||
         (Params_p->OutputRectangle.Width== 0) || (Params_p->OutputRectangle.Height == 0))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Re-align the negative values for (xi,yi,xo,yo) */
    CenterNegativeValues(LayerHandle, Params_p);

    /* Set the Input/Output windows according to the hardware                  */
    /* constraints (PS: Order input then output has to be respected) */
    AdjustInputWindow(LayerHandle, Params_p);
    AdjustOutputWindow(LayerHandle, Params_p);

    /* No error to return */
    return (ErrorCode);

} /* End of AdjustViewportParams() function */

/*******************************************************************************
Name        : SetViewportParams
Description : Apply the ViewPort parameters.
Parameters  : HAL viewport manager handle.
              Viewport parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetViewportParams(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t    * LayerCommonData_p;

    TrApi(("\r\nSetViewportParams"));

    /* Init Layer/Viewport properties.                                  */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;

    /* Check if any input parameters is unavailable.                    */
    if ((ViewportProperties_p == NULL) || (Params_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerProperties_p    = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    LayerCommonData_p    = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters                                            */
    if ((LayerProperties_p == NULL) || (LayerCommonData_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Re-align the negative values for (xi,yi,xo,yo).                  */
    CenterNegativeValues(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle, Params_p);

    /* Set the Input/Output windows according to the hardware           */
    /* constraints (PS: Order input then output has to be respected).   */
    AdjustInputWindow(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle, Params_p);
    AdjustOutputWindow(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle, Params_p);

    /* Store the Viewport params applied.                               */
    LayerCommonData_p->ViewPortParams = *Params_p;
    /* And its source characteristics.                                  */
    LayerCommonData_p->SourcePictureDimensions.Width    = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
    LayerCommonData_p->SourcePictureDimensions.Height   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;

    /* Apply the ViewPort Dimensions.                                   */
    ApplyViewPortParams(ViewportHandle, Params_p);

    /* No error to return.                                              */
    return (ST_NO_ERROR);

} /* End of SetViewportParams() function */


/*******************************************************************************
Name        : UpdateFromMixer
Description : Update parameters from mixer called by STMIXER.
Parameters  : HAL layer manager handle.
              New mixer data to manage.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void UpdateFromMixer(const STLAYER_Handle_t      LayerHandle,
                     STLAYER_OutputParams_t * const     OutputParams_p)
{

    HALLAYER_LayerProperties_t *LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *LayerCommonData_p;
    STLAYER_LayerParams_t       LayerParams;
    STLAYER_UpdateParams_t      UpdateParams;

    TrApi(("\r\nUpdateFromMixer"));

    /* Check NULL parameters                                            */
    if (OutputParams_p == NULL)
    {
        return;
    }

    /* Init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p) ;
    /* Check NULL parameters                                            */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    TrViewport(("\r\nUpdateFromMixer"));

    /* Retrieve the screen parameters.          */
    if ((OutputParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)
    {
        /* Save new layer offset parameters.    */
        LayerCommonData_p->LayerPosition.XOffset = OutputParams_p->XOffset;
        LayerCommonData_p->LayerPosition.YOffset = OutputParams_p->YOffset;
        TrLayerPosition(("\r\n XOffset=%d, YOffset=%d", OutputParams_p->XOffset, OutputParams_p->YOffset));

        /* This change need also the re-calculation of all. */
        GetLayerParams(LayerHandle, &LayerParams);
        UpdateParams.LayerParams_p = &LayerParams;
        UpdateParams.UpdateReason = STLAYER_LAYER_PARAMS_REASON;
        STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle, ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID], (const void *) (&UpdateParams));

        /* Set the viewport rectangle dimensions and offset*/
        SetViewportOutputRectangle(LayerProperties_p);
    }

    if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
    {
        /* Save new layer start position.           */
        LayerCommonData_p->LayerPosition.XStart = OutputParams_p->XStart;
        LayerCommonData_p->LayerPosition.YStart = OutputParams_p->YStart;
        TrLayerPosition(("\r\n XStart=%d, YStart=%d", OutputParams_p->XStart, OutputParams_p->YStart));

        LayerCommonData_p->LayerParams.Width        = OutputParams_p->Width;
        LayerCommonData_p->LayerParams.Height       = OutputParams_p->Height;
        LayerCommonData_p->LayerParams.ScanType     = OutputParams_p->ScanType;
        LayerCommonData_p->LayerParams.AspectRatio  = OutputParams_p->AspectRatio;
        LayerCommonData_p->VTGFrameRate             = OutputParams_p->FrameRate;

        LayerParams.Width       = OutputParams_p->Width;
        LayerParams.Height      = OutputParams_p->Height;
        LayerParams.ScanType    = OutputParams_p->ScanType;
        LayerParams.AspectRatio = OutputParams_p->AspectRatio;

        UpdateParams.LayerParams_p = &LayerParams;
        UpdateParams.UpdateReason = STLAYER_LAYER_PARAMS_REASON;
        STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle, ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID], (const void *) (&UpdateParams));

        /* Set the viewport rectangle dimensions and offset*/
        SetViewportOutputRectangle(LayerProperties_p);
    }

    if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
    {
        /* Retrieve the VTGFrameRate of output. */
        LayerCommonData_p->VTGFrameRate = OutputParams_p->FrameRate;
        strcpy(LayerCommonData_p->VTGName, OutputParams_p->VTGName);
    }
    if ((OutputParams_p->UpdateReason & STLAYER_CHANGE_ID_REASON) != 0)
    {
        /* Only available for ST40 GX1 */
    }
    if ((OutputParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
    {
    }
    if ((OutputParams_p->UpdateReason & STLAYER_DISPLAY_REASON) != 0)
    {
        /* Only available for Omega 1 cells. */
    }
} /* End of UpdateFromMixer() callback function. */


/*******************************************************************************
Name        : SetViewportAlpha
Description : Set the global alpha value for gamma video output.
Parameters  : HAL viewport manager handle : ViewportHandle.
              Alpha value to set : Alpha.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetViewportAlpha(const STLAYER_ViewPortHandle_t     ViewportHandle,
                             STLAYER_GlobalAlpha_t*             Alpha_p)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;

    /* Check NULL parameters                                            */
    if (Alpha_p == NULL)
    {
        return;
    }

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;
    /* Check NULL parameters                                            */
    if (ViewportProperties_p == NULL)
    {
        return;
    }

    LayerProperties_p    = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    /* Set the global alpha value for this video gamma. */
    HAL_WriteGam32(GAM_VIDn_ALP, Alpha_p->A0 & GAM_VIDn_ALP_MASK);

} /* End of SetViewportAlpha() function. */

/*******************************************************************************
Name        : GetViewportAlpha
Description : Gets the global alpha value for gamma video output.
Parameters  : HAL viewport manager handle : ViewportHandle.
              Alpha value to set : Alpha.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetViewportAlpha(const STLAYER_ViewPortHandle_t     ViewportHandle,
                             STLAYER_GlobalAlpha_t*             Alpha_p)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;

    /* Check NULL parameters                                            */
    if (Alpha_p == NULL)
    {
        return;
    }

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;
    /* Check NULL parameters                                            */
    if (ViewportProperties_p == NULL)
    {
        return;
    }

    LayerProperties_p    = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    /* Get the global alpha value for this video gamma. */
    Alpha_p->A0 =  ( (U32)HAL_Read32((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_ALP) & GAM_VIDn_ALP_MASK);

} /* End of GetViewportAlpha() function. */

/* private MAIN/AUXILIARY DISPLAY management functions ---------------------- */

/*******************************************************************************
Name        : InitializeVideoProcessor
Description : Initialize main and auxiliary processor registers in order to allow
              a normal display.
Parameters  : HAL layer manager handle : LayerHandle.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void InitializeVideoProcessor (const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;

    /* init Layer properties.               */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters                                            */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Initialization of the video gamma compositor registers */
    /* GAM_VIDn_CTL : control */
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_CTL,
                                  GAM_VIDn_CTL_MASK,
                                  HALLAYER_DISABLE);

    /* GAM_VIDn_ALP : Global Alpha Value              */
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_ALP,
                                  GAM_VIDn_ALP_MASK,
                                  0x00000080);

    /* GAM_VIDn_KEY1 : lower limit of the color range */
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_KEY1,
                                  GAM_VIDn_KEY1_MASK,
                                  HALLAYER_DISABLE);

    /* GAM_VIDn_KEY2 : upper limit of the color range */
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + GAM_VIDn_KEY2,
                                  GAM_VIDn_KEY2_MASK,
                                  HALLAYER_DISABLE);

} /* End of InitializeVideoProcessor() function */

/*******************************************************************************
Name        : CheckForAvailableScaling
Description : Check if the wanted scaling is available.
Parameters  : IN :  layer handle : LayerHandle
                    viewport parameters : ViewPortParams_p
              OUT : recommended output sizes  (horizontal & vertical)
Assumptions : Those parameters have already been tested.
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no error occurs. The scaling is possible,
                  output parameter are then available.
                - ST_ERROR_BAD_PARAMETER, one parameter is invalid.
                - ST_ERROR_FEATURE_NOT_SUPPORTED, the scaling is impossible.
*******************************************************************************/
static ST_ErrorCode_t CheckForAvailableScaling (
        const STLAYER_Handle_t          LayerHandle,
        const STLAYER_ViewPortParams_t  *ViewPortParams_p,
        STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
        STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation,
        STLAYER_DecimationFactor_t      *RecommendedHorizontalDecimation_p,
        STLAYER_DecimationFactor_t      *RecommendedVerticalDecimation_p,
        U32                             *RecommendedHorizontalSize_p,
        U32                             *RecommendedVerticalSize_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t    * LayerCommonData_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Input parameters.                */
    U32                 InputHorizontalSize;
    U32                 InputVerticalSize;

    /* Output parameters.               */
    U32                 OutputHorizontalSize;
    U32                 OutputVerticalSize;

        /* Bandwidth claculation parameters.*/
    BOOL                IsEndOfScalingTestReached = FALSE;
    U32                 DurationOfOneLine;
    U32                 HorizontalZoomFactorCoef;
    U32                 VerticalZoomFactorCoef;
    U32                 NumberOfPixelsLuma,NumberOfPixelsChroma;
    U32                 RecommendedHDecimation;
    U32                 RecommendedVDecimation;
    U32                 RecommendedOutputHorizontalSize;
    U32                 RecommendedOutputVerticalSize;
    STGXOBJ_ColorType_t ColorType;
    U32                 DisplayTimeInUs;
    U32                 LumaDataSize;
    BOOL                IsInputScantypeProgressive;

    /* Init Layer/Viewport properties.  */
    LayerProperties_p       = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p       = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    RecommendedHDecimation  = stlayer_GetDecimationFactorFromDecimation(CurrentPictureHorizontalDecimation);
    RecommendedVDecimation  = stlayer_GetDecimationFactorFromDecimation(CurrentPictureVerticalDecimation);

    InputHorizontalSize     = ViewPortParams_p->InputRectangle.Width;
    InputVerticalSize       = ViewPortParams_p->InputRectangle.Height;
    OutputHorizontalSize    = ViewPortParams_p->OutputRectangle.Width;
    OutputVerticalSize      = ViewPortParams_p->OutputRectangle.Height;

    /* Default : recommended output window is the original one. */
    RecommendedOutputVerticalSize   = OutputVerticalSize;
    RecommendedOutputHorizontalSize = OutputHorizontalSize;

    TrDebug(("\r\n In CheckForAvailableScaling, with IV=%d, IH=%d, OV=%d, OH=%d, VD=%d & HD=%d",InputVerticalSize,
            InputHorizontalSize,OutputVerticalSize,OutputHorizontalSize,RecommendedVDecimation,RecommendedHDecimation));

    /* Avoid division by zero.  */
    if ( (InputVerticalSize         == 0) || (InputHorizontalSize       == 0) ||
         (OutputHorizontalSize      == 0) || (OutputVerticalSize        == 0) )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        ColorType = (((ViewPortParams_p->Source_p)->Data).VideoStream_p)->BitmapParams.ColorType;

        /* Hardware consumption for the Horizontal zoom */
        InputHorizontalSize = InputHorizontalSize / RecommendedHDecimation;
        HorizontalZoomFactorCoef = ((InputHorizontalSize - 1) / OutputHorizontalSize) + 1; /* (575 / 576) + 1 = 1 */
        if (HorizontalZoomFactorCoef > HORIZONTAL_MAX_ZOOM_OUT)
        {
            /* Hardware horizontal zoom limit */
            RecommendedOutputHorizontalSize = 1 + (InputHorizontalSize / HORIZONTAL_MAX_ZOOM_OUT);
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
        while ((HorizontalZoomFactorCoef > HORIZONTAL_MAX_ZOOM_OUT) && (RecommendedHDecimation < 4))
        {
            InputHorizontalSize = InputHorizontalSize * RecommendedHDecimation;
            RecommendedHDecimation <<= 1;
            /* Hardware consumtion for the horizontal zoom */
            InputHorizontalSize = InputHorizontalSize / RecommendedHDecimation;
            HorizontalZoomFactorCoef = ((InputHorizontalSize - 1) / OutputHorizontalSize) + 1; /* (575 / 576) + 1 = 1 */
        }

        /* Hardware consumption for the vertical zoom */
        OutputVerticalSize  = RecommendedOutputVerticalSize;
        InputVerticalSize   = InputVerticalSize / RecommendedVDecimation;
        VerticalZoomFactorCoef      = ((InputVerticalSize - 1) / OutputVerticalSize) + 1; /* (575 / 576) + 1 = 1 */

        if (VerticalZoomFactorCoef > VERTICAL_MAX_ZOOM_OUT)
        {
            /* Hardware vertical zoom limit */
            RecommendedOutputVerticalSize = 1 + (InputVerticalSize / VERTICAL_MAX_ZOOM_OUT);
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
        while ((VerticalZoomFactorCoef > VERTICAL_MAX_ZOOM_OUT) && (RecommendedVDecimation < 4))
        {
            InputVerticalSize = InputVerticalSize * RecommendedVDecimation;
            RecommendedVDecimation <<= 1;
            /* Hardware consumtion for the vertical zoom */
            InputVerticalSize = InputVerticalSize / RecommendedVDecimation;
            VerticalZoomFactorCoef = ((InputVerticalSize - 1) / OutputVerticalSize) + 1; /* (575 / 576) + 1 = 1 */
        }

        NumberOfPixelsLuma = InputHorizontalSize;
        NumberOfPixelsChroma = NumberOfPixelsLuma / ( (ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420)? 2:1);

        DurationOfOneLine = GetDurationOfOneLine(LayerCommonData_p->VTGName);

        if ((((ViewPortParams_p->Source_p)->Data).VideoStream_p)->ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        {
            IsInputScantypeProgressive = TRUE;
        }
        else
        {
            IsInputScantypeProgressive = FALSE;
        }
        TrBandwidth(("\r\nIsInputScantypeProgressive=%d", IsInputScantypeProgressive));

        /* NB: Evaluation of Decimation is now based on Luma Data Rate only.
               This evalution is done with DEI considered as Bypassed.
               This is the function vdp_dei_IsBypassModeNecessary() that will decide if there is
                enough Bandwidth to activate the DEI. */

        /********************************************/
        /* Loop until the right decimation is found */
        /********************************************/
        while (!IsEndOfScalingTestReached)
        {
            /* Compute the LumaDataRate corresponding to current H and V decimation:

               LumaDataRate = (size of Luma data going through the pipe) / (time to display the output rectangle)
            */
            LumaDataSize = GetInputLumaDataSize(LayerHandle, InputHorizontalSize, InputVerticalSize, IsInputScantypeProgressive);

            DisplayTimeInUs = GetOutputRectangleDisplayTime(LayerHandle, OutputVerticalSize, DurationOfOneLine);

            LayerCommonData_p->LumaDataRateInBypassMode = LumaDataSize / DisplayTimeInUs;
            TrBandwidth(("\r\n=> LumaDataRateInBypassMode=%d MB/s", LayerCommonData_p->LumaDataRateInBypassMode));

            if (LayerCommonData_p->LumaDataRateInBypassMode <= MAX_LUMA_DATA_RATE)
            {
                /* Ok, global bandwidth respected. */
                IsEndOfScalingTestReached = TRUE;
            }
            else
            {
                /* Not enough bandwidth => The picture should be decimated */

                /* Change parameters to answer bandwidth limitation by appling these actions consecutively:
                 * 1 - * Modify the horizontal decimation
                 *     * Use the new number of pixels to compute the bandwidth
                 * 2 - * Modify the vertical decimation
                 *     * Use the new factor to compute the bandwidth
                 * 3 - * Calculate RecommendedOutputVerticalSize using MAX_LUMA_DATA_RATE
                 */
                /* Try to increase the horizontal decimation, then vertical one.*/
                if (RecommendedHDecimation < 4)
                {
                    InputHorizontalSize = InputHorizontalSize * RecommendedHDecimation;
                    /* Ok, increase the horizontal decimation.  */
                    RecommendedHDecimation <<= 1;
                    InputHorizontalSize = InputHorizontalSize / RecommendedHDecimation;
                    NumberOfPixelsLuma = InputHorizontalSize;
                    NumberOfPixelsChroma = NumberOfPixelsLuma / ( (ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420)? 2:1);
                }
                else
                {
                    if (RecommendedVDecimation < 4)
                    {
                        InputVerticalSize = InputVerticalSize * RecommendedVDecimation;
                        /* Max horizontal decimation is reached.    */
                        /* Try to increase the vertical one.      */
                        RecommendedVDecimation <<= 1;
                        /* Hardware consumtion for the vertical zoom */
                        InputVerticalSize = InputVerticalSize / RecommendedVDecimation;
                    }
                    else
                    {
                        TrWarning(("\r\nImpossible to perform the wanted scaling!"));
                        /* In that case we recommend to use a higher vertical output size. This will increase the Display time and thus decrease the bitrate.

                           LumaDataRateInBypassMode = LumaDataSize / DisplayTimeInUs
                                                    = LumaDataSize / (OutputVerticalSize * DurationOfOneLine);

                           => OutputVerticalSize = LumaDataSize / (LumaDataRateInBypassMode * DurationOfOneLine)

                           So if we want that the bitrate doesn't exceed MAX_LUMA_DATA_RATE:
                           RecommendedOutputVerticalSize = LumaDataSize / (MAX_LUMA_DATA_RATE * DurationOfOneLine)
                        */
                        RecommendedOutputVerticalSize = LumaDataSize / (MAX_LUMA_DATA_RATE * DurationOfOneLine);
                        TrBandwidth(("\r\nRecommendedOutputVerticalSize=%d", RecommendedOutputVerticalSize));
                        IsEndOfScalingTestReached = TRUE;
                    }
                }
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            } /* if (LayerCommonData_p->LumaDataRateInBypassMode <= MAX_LUMA_DATA_RATE) */
        } /* while (!IsEndOfScalingTestReached) */
    }

    /* Set the recommended size for the output sizes.   */
    *RecommendedHorizontalSize_p = RecommendedOutputHorizontalSize;
    *RecommendedVerticalSize_p   = RecommendedOutputVerticalSize;

    /* And update in/out recommended decimation parameters. */
    *RecommendedHorizontalDecimation_p = stlayer_GetDecimationFromDecimationFactor(RecommendedHDecimation);
    *RecommendedVerticalDecimation_p = stlayer_GetDecimationFromDecimationFactor(RecommendedVDecimation);
    TrBandwidth(("\r\nRecommended decim: H:%d, V:%d", *RecommendedHorizontalDecimation_p, *RecommendedVerticalDecimation_p));

    TrDebug(("\r\n Out CheckForAvailableScaling, with IV=%d, IH=%d, OV=%d, OH=%d, VD=%d & HD=%d",InputVerticalSize,
            InputHorizontalSize,OutputVerticalSize,OutputHorizontalSize,RecommendedVDecimation,RecommendedHDecimation));

    return(ErrorCode);
} /* End of CheckForAvailableScaling function */


/*******************************************************************************
Name        : GetInputLumaDataSize
Description : This function computes the amount of Luma data going through the pipe at each VSync.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 GetInputLumaDataSize(const STLAYER_Handle_t   LayerHandle,
                           U32                      InputHorizontalSize,
                           U32                      InputVerticalSize,
                           BOOL                     IsInputProgressive)
{
    U32     LumaDataSize;
    UNUSED_PARAMETER(LayerHandle);

    if (!IsInputProgressive)
    {
        /* We read a FIELD in memory so only half of the lines will do through the pipe during a VSync */
        InputVerticalSize /= 2;
    }

    LumaDataSize = (InputHorizontalSize * InputVerticalSize);
    TrBandwidth(("\r\nLumaDataSize=%d", LumaDataSize));

    return (LumaDataSize);
}


/*******************************************************************************
Name        : GetOutputRectangleDisplayTime
Description : This function computes the time necessary to display the output rectangle.
              It corresponds to the time between the top left corner and bottom right corner.
              This is approximately equal to: Output Height * Duration of one line
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 GetOutputRectangleDisplayTime(const STLAYER_Handle_t     LayerHandle,
                                         U32                        OutputVerticalSize,
                                         U32                        DurationOfOneLine)
{
    HALLAYER_LayerProperties_t    *     LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t *     LayerCommonData_p;
    U32                                 DisplayTimeInUs;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (LayerCommonData_p->LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN)
    {
        OutputVerticalSize /= 2;
    }

    DisplayTimeInUs = (OutputVerticalSize*DurationOfOneLine);
    TrBandwidth(("\r\nDisplayTimeInUs=%d", DisplayTimeInUs));

    return (DisplayTimeInUs);
}

/*******************************************************************************
Name        : GetDurationOfOneLine
Description : Duration of one output line (in us)
              This duration will be used to compute the STBus Bitrate.
              We should take the minimum line duration in order to get the value of the max bitrate.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static U32 GetDurationOfOneLine(ST_DeviceName_t VTGName)
{
    STVTG_Handle_t       VtgHandle;
    STVTG_OpenParams_t   VtgOpenParams;
    STVTG_SyncParams_t   syncParams;
    ST_ErrorCode_t       Error=ST_NO_ERROR;
    U32                  LineDurationInUs = DEFAULT_LINE_DURATION;

    Error = STVTG_Open(VTGName, &VtgOpenParams, &VtgHandle);

    if (Error == ST_NO_ERROR)
    {
        Error = STVTG_GetModeSyncParams(VtgHandle, &syncParams);

        if (Error == ST_NO_ERROR && syncParams.PixelClock !=0 )
        {
            LineDurationInUs = (syncParams.PixelsPerLine * 1000000) / syncParams.PixelClock;
        }

        Error = STVTG_Close(VtgHandle);
    }

    return(LineDurationInUs);
}

#ifdef STLAYER_USE_CRC
/*******************************************************************************
Name        : SetMemoryAccessForCRC
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void SetMemoryAccessForCRC(STLAYER_ViewPortParams_t *ViewPortParams_p,   HALLAYER_VDPDeiMemoryAccess_t   *MemoryAccessParams_p,
                                                                    U32 SourcePictureHorizontalSize, U32 SourcePictureVerticalSize )
{
    U32     VideoWidthInMB;
    U32     VideoHeightInMB;

    if (ViewPortParams_p->Source_p->Data.VideoStream_p->CRCScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        VideoWidthInMB       = ((SourcePictureHorizontalSize + 15) / 16);       /* 'n' in App Note   */
        VideoHeightInMB      = ((SourcePictureVerticalSize + 31) / 32);           /* 'p' in app note   */

        MemoryAccessParams_p->YFOffsetL0= 64*VideoWidthInMB-53;        MemoryAccessParams_p->YFCeilL0=  VideoHeightInMB;
        MemoryAccessParams_p->YFOffsetL1= 11;                          MemoryAccessParams_p->YFCeilL1= 2;
        MemoryAccessParams_p->YFOffsetL2=-13;                          MemoryAccessParams_p->YFCeilL2= 4;
        MemoryAccessParams_p->YFOffsetL3=-17;                          MemoryAccessParams_p->YFCeilL3= 2;
        MemoryAccessParams_p->YFOffsetL4= 16;                          MemoryAccessParams_p->YFCeilL4= 2;

        MemoryAccessParams_p->CFOffsetL0= 32*VideoWidthInMB-41;        MemoryAccessParams_p->CFCeilL0 = VideoHeightInMB;
        MemoryAccessParams_p->CFOffsetL1= 23;                          MemoryAccessParams_p->CFCeilL1 = 2;
        MemoryAccessParams_p->CFOffsetL2= -5;                          MemoryAccessParams_p->CFCeilL2 = 2;
        MemoryAccessParams_p->CFOffsetL3= -9;                          MemoryAccessParams_p->CFCeilL3 = 2;
        MemoryAccessParams_p->CFOffsetL4=  8;                          MemoryAccessParams_p->CFCeilL4 = 2;
    }
    else
    {
        VideoWidthInMB       = ((SourcePictureHorizontalSize + 15) / 16);
        VideoHeightInMB      = ((SourcePictureVerticalSize + 31) / 32);

        MemoryAccessParams_p->YFOffsetL0= 64*VideoWidthInMB-37;        MemoryAccessParams_p->YFCeilL0=  VideoHeightInMB;
        MemoryAccessParams_p->YFOffsetL1= 27;                          MemoryAccessParams_p->YFCeilL1= 2;
        MemoryAccessParams_p->YFOffsetL2=  3;                          MemoryAccessParams_p->YFCeilL2= 4;
        MemoryAccessParams_p->YFOffsetL3= -1;                          MemoryAccessParams_p->YFCeilL3= 2;
        MemoryAccessParams_p->YFOffsetL4=  0;                          MemoryAccessParams_p->YFCeilL4= 0;

        MemoryAccessParams_p->CFOffsetL0=  32*VideoWidthInMB-33;        MemoryAccessParams_p->CFCeilL0 = VideoHeightInMB;
        MemoryAccessParams_p->CFOffsetL1=  31;                          MemoryAccessParams_p->CFCeilL1 = 2;
        MemoryAccessParams_p->CFOffsetL2=   3;                          MemoryAccessParams_p->CFCeilL2 = 2;
        MemoryAccessParams_p->CFOffsetL3=  -1;                          MemoryAccessParams_p->CFCeilL3 = 2;
        MemoryAccessParams_p->CFOffsetL4=   0;                          MemoryAccessParams_p->CFCeilL4 = 0;
    }
}
#endif /* STLAYER_USE_CRC */

/*******************************************************************************
Name        : ComputeAndApplyThePolyphaseFilters
Description : Get VhsrcTopRegister and VhsrcBottomRegister from the input
              parameters (InputRectangle, OutputRectangle, InputFormat, ...).
              DeiRegister, VhsrcTopRegister and VhsrcBottomRegister will be written
              during the next VSync Top and Bottom.
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void ComputeAndApplyThePolyphaseFilters(const STLAYER_Handle_t LayerHandle,
                    STLAYER_ViewPortParams_t *      ViewPortParams_p,
                    STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
                    STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation)
{
    HALLAYER_VDPIncrement_t             Increment;
    HALLAYER_VDPPositionDefinition_t    PositionDefinition;
    STGXOBJ_ColorType_t                 ColorType;
    BOOL                                IsFirstLineTopNotBot;
    HALLAYER_VDPPhase_t                 Phase;
    HALLAYER_VDPFirstPixelRepetition_t  FirstPixelRepetition;
    HALLAYER_VDPMemorySize_t            MemorySize;
    HALLAYER_VDPVhsrcRegisterMap_t      VhsrcTopRegister, VhsrcBottomRegister;
    U32                                 IncrementForCoeffSelectionChromaVertical, IncrementForCoeffSelectionLumaVertical;
    /* Register values */
    U8                              EnaCHF, EnaYHF;
    U8                              EnaVHSRC;
    U8                              EnaHFilterUpdate = 0;
    U8                              EnaVFilterUpdate = 0;
    U16                             TargetHeightTop, TargetHeightBottom;
    U32                             CoefLoadLine, PixLoadLine;
    BOOL                            FiltersNeedReload = FALSE;           /* Boolean used to force filter coeff reloading */
    HALLAYER_VDPFilter_t            HorizontalLumaFilter;
    HALLAYER_VDPFilter_t            HorizontalChromaFilter;
    HALLAYER_VDPFilter_t            VerticalLumaFilter;
    HALLAYER_VDPFilter_t            VerticalChromaFilter;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;
    STGXOBJ_Rectangle_t             InputRectangle, OutputRectangle;
    U32                             VerticalFilterPointer, HorizontalFilterPointer;
    U8                              DisplayInstanceIndex = 0;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    U32                             DecimXOffset;       /* XOffset due to decimation rounding (in 32nd of pixels) */
    U32                             DecimYOffset;       /* YOffset due to decimation rounding (in 32nd of pixels) */
    BOOL                            IsDeiActivated = FALSE;
    BOOL                            IsPseudo422FromTopField = FALSE;
    BOOL                            IsSourceFirstLineEven;
    BOOL                            IsOutputFirstLineEven;
#ifdef DBG_FIELD_INVERSIONS
    BOOL                            SameFieldRepeated;
#endif
#if defined(HAL_VIDEO_LAYER_FILTER_DISABLE)
    BOOL                            ForceNullCoefficients = FALSE;
#endif
    /* Local variables specific to VDP */
    HALLAYER_VDPDeiRegisterMap_t *  DeiRegisterMap_p;
    HALLAYER_VDPDeiMemoryAccess_t   MemoryAccessParams;
    U32                             SourcePictureHorizontalSize, SourcePictureVerticalSize;

    TrVsync(("|"));

    /* Init Layer/Viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    if (LayerProperties_p->LayerType == MAIN_DISPLAY)
    {
        DisplayInstanceIndex = 0;
    }
    else if (LayerProperties_p->LayerType == AUX_DISPLAY)
    {
        DisplayInstanceIndex = 1;
    }
#if defined(HW_7200)
    else if (LayerProperties_p->LayerType == SEC_DISPLAY)
    {
        DisplayInstanceIndex = 2;
    }
#endif /* HW_7200 */
    TrDebug(("Index: %d\r\n",DisplayInstanceIndex));

    /************************************************************************************************/
    /* Apply decimation factors.                                                                    */
    LayerCommonData_p->CurrentPictureHorizontalDecimationFactor = stlayer_GetDecimationFactorFromDecimation(CurrentPictureHorizontalDecimation);
    LayerCommonData_p->CurrentPictureVerticalDecimationFactor = stlayer_GetDecimationFactorFromDecimation(CurrentPictureVerticalDecimation);

    if ((((ViewPortParams_p->Source_p)->Data).VideoStream_p)->HorizontalDecimationFactor == STLAYER_DECIMATION_MPEG4_P2)
    {
        SourcePictureHorizontalSize  = ( (((ViewPortParams_p->Source_p->Data.VideoStream_p->BitmapParams.Width+ 15)/16 + 1) / 2) * 32);
    }
    else
    {
        SourcePictureHorizontalSize  = ViewPortParams_p->Source_p->Data.VideoStream_p->BitmapParams.Width;
        SourcePictureHorizontalSize /= LayerCommonData_p->CurrentPictureHorizontalDecimationFactor;
    }
    SourcePictureVerticalSize    = ViewPortParams_p->Source_p->Data.VideoStream_p->BitmapParams.Height;
    SourcePictureVerticalSize   /= LayerCommonData_p->CurrentPictureVerticalDecimationFactor;

    InputRectangle = ViewPortParams_p->InputRectangle;
    InputRectangle.PositionX /= LayerCommonData_p->CurrentPictureHorizontalDecimationFactor;
    InputRectangle.PositionY /= LayerCommonData_p->CurrentPictureVerticalDecimationFactor;
    InputRectangle.Height /= LayerCommonData_p->CurrentPictureVerticalDecimationFactor;
    InputRectangle.Width /= LayerCommonData_p->CurrentPictureHorizontalDecimationFactor;

    OutputRectangle = ViewPortParams_p->OutputRectangle;

    if (LayerCommonData_p->UpdateViewport)
    {
        if (LayerCommonData_p->IsViewportEnabled == FALSE)
        {
            TrViewport(("\r\n VPO=0 VPS=0"));
            /* GAM_VIDn_VPO : Viewport Origin.   */
            HAL_WriteGam32(GAM_VIDn_VPO, 0);
            /* GAM_VIDn_VPS : Viewport Stop.     */
            HAL_WriteGam32(GAM_VIDn_VPS, 0);
        }
        else
        {
            TrViewport(("\r\n VPO=0x%x VPS=0x%x", LayerCommonData_p->ViewportOffset, LayerCommonData_p->ViewportOffset + LayerCommonData_p->ViewportSize));
            /* GAM_VIDn_VPO : Viewport Origin.   */
            HAL_WriteGam32(GAM_VIDn_VPO, LayerCommonData_p->ViewportOffset);
            /* GAM_VIDn_VPS : Viewport Stop.     */
            HAL_WriteGam32(GAM_VIDn_VPS, LayerCommonData_p->ViewportOffset + LayerCommonData_p->ViewportSize);

            /* The Viewport has been enabled. We reset the information about the currently loaded filters in order to force a reload */
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalLumaFilter   = VDP_FILTER_NO_SET;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalChromaFilter = VDP_FILTER_NO_SET;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalLumaFilter     = VDP_FILTER_NO_SET;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalChromaFilter   = VDP_FILTER_NO_SET;
        }
        LayerCommonData_p->UpdateViewport = FALSE;
    }

    /* "ViewPortParams_p->InputRectangle" corresponds to the size and position in the non decimated frame.
       "InputRectangle" corresponds to the size and position in the decimated frame. */

    /* Compute position offsets due to decimation rounding (these offsets are in 32nd of pixels) */
    DecimXOffset = ( (ViewPortParams_p->InputRectangle.PositionX / 16) % LayerCommonData_p->CurrentPictureHorizontalDecimationFactor)  * 32 / LayerCommonData_p->CurrentPictureHorizontalDecimationFactor; /* PositionX is in 16th of pixels */
    DecimYOffset = ( (ViewPortParams_p->InputRectangle.PositionY / 16) % LayerCommonData_p->CurrentPictureVerticalDecimationFactor)  * 32 / LayerCommonData_p->CurrentPictureVerticalDecimationFactor;    /* PositionY is in 16th of pixels */

#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2(LAYER_PARTITION_AVMEM, &VirtualMapping );
#else
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif

    ColorType = (((ViewPortParams_p->Source_p)->Data).VideoStream_p)->BitmapParams.ColorType;
    if (ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420)
    {
        LayerCommonData_p->SourceFormatInfo.IsFormat422Not420 = FALSE;
    }
    else /* if 4:2:0 else 4:2:2 ( if (ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422) )*/
    {
        LayerCommonData_p->SourceFormatInfo.IsFormat422Not420 = TRUE;
    }

    if ((((ViewPortParams_p->Source_p)->Data).VideoStream_p)->ScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive = TRUE;
    }
    else
    {
        LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive = FALSE;
    }

#ifdef STLAYER_USE_CRC
    /* Overwrite parameters with CRC ones */
    if((ViewPortParams_p->Source_p)->Data.VideoStream_p->CRCCheckMode == TRUE)
    {
        InputRectangle.Height=SourcePictureVerticalSize;
        if ((ViewPortParams_p->Source_p->Data.VideoStream_p)->CRCScanType != STGXOBJ_PROGRESSIVE_SCAN)
        {
            LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive = FALSE;
        }
        else
        {
            LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive = TRUE;
        }

        LayerCommonData_p->RequestedDeiMode = STLAYER_DEI_MODE_BYPASS;
    }
#endif /* STLAYER_USE_CRC */

    /* Special case for Progressive Source with Interlaced Output:
       The source can be considered as interlaced in order to achieve a bigger zoom factor */
    if ( (LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive) &&
         (!LayerCommonData_p->IsOutputScantypeProgressive) )
    {
        if (OutputRectangle.Height != 0)
        {
            U32   OutputFieldHeight;

            OutputFieldHeight = OutputRectangle.Height  / 2;

            if (InputRectangle.Height > (VERTICAL_MAX_ZOOM_OUT * OutputFieldHeight) )
            {
                /* The needed Zoom factor exceeds the limit: The source can be considered as interlaced to decrease the zoom factor by 2 */
                LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive = FALSE;
                TrWarning(("\r\nSrc considered as interlaced"));
            }
        }
    }

    if (LayerCommonData_p->LayerParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        LayerCommonData_p->IsOutputScantypeProgressive = TRUE;
    }
    else
    {
        LayerCommonData_p->IsOutputScantypeProgressive = FALSE;
    }

    if ( (CurrentPictureHorizontalDecimation != STLAYER_DECIMATION_FACTOR_NONE) ||
          (CurrentPictureVerticalDecimation != STLAYER_DECIMATION_FACTOR_NONE) )
    {
        LayerCommonData_p->IsDecimationActivated = TRUE;
    }
    else
    {
        LayerCommonData_p->IsDecimationActivated = FALSE;
    }

    /* Raster format or macroblock? */
    switch ((ViewPortParams_p->Source_p->Data).VideoStream_p->BitmapParams.BitmapType)
    {
        case STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE:
        case STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM:
            LayerCommonData_p->SourceFormatInfo.IsFormatRasterNotMacroblock = TRUE;
            break;

        case STGXOBJ_BITMAP_TYPE_MB:
        case STGXOBJ_BITMAP_TYPE_MB_RANGE_MAP:
        case STGXOBJ_BITMAP_TYPE_MB_HDPIP:
        case STGXOBJ_BITMAP_TYPE_MB_TOP_BOTTOM:
        default:
            LayerCommonData_p->SourceFormatInfo.IsFormatRasterNotMacroblock = FALSE;
    }

    if (LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive)
    {
        LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->PreviousField.FieldType = STLAYER_TOP_FIELD;
        LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.FieldType = STLAYER_TOP_FIELD;
        LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->NextField.FieldType = STLAYER_TOP_FIELD;
    }

    /******************************************************************************/
    /* "SourceFormatInfo" are set so we can now decide if Deinterlacing is needed */
    vdp_dei_SetDeinterlacer(LayerHandle);

    IsDeiActivated = vdp_dei_IsDeiActivated(LayerHandle);

    if  (IsDeiActivated)
    {
        /* Deinterlacer activated so the VHSRC input is progressive "pseudo 4:2:2"  */
        LayerCommonData_p->PostDeiFormatInfo.IsFormat422Not420 = TRUE;
        LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive = TRUE;

        if (LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.FieldType == STLAYER_TOP_FIELD)
        {
            IsPseudo422FromTopField = TRUE;
        }
        else
        {
            IsPseudo422FromTopField = FALSE;
        }
    }
    else
    {
        /* Deinterlacer Bypassed or Inactive */
        LayerCommonData_p->PostDeiFormatInfo.IsFormat422Not420 = LayerCommonData_p->SourceFormatInfo.IsFormat422Not420;
        LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive = LayerCommonData_p->SourceFormatInfo.IsInputScantypeProgressive;
    }

    /* "IsFormatRasterNotMacroblock" is not changed by the deinterlacer */
    LayerCommonData_p->PostDeiFormatInfo.IsFormatRasterNotMacroblock = LayerCommonData_p->SourceFormatInfo.IsFormatRasterNotMacroblock;

    /******************************************************************************/
    /* "PostDeiFormatInfo" are now set                                            */

    /* If the VHSRC input or the output is progressive then PresentedFieldInverted has no meaning */
    if ( (LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive) ||
         (LayerCommonData_p->IsOutputScantypeProgressive) )
    {
        ViewPortParams_p->Source_p->Data.VideoStream_p->PresentedFieldInverted = FALSE;
    }

#ifdef DBG_FIELD_INVERSIONS
    if ( (LayerCommonData_p->OldCurrentField.PictureIndex == LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.PictureIndex) &&
        (LayerCommonData_p->OldCurrentField.FieldType == LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.FieldType) )
    {
        SameFieldRepeated = TRUE;
    }
    else
    {
        SameFieldRepeated = FALSE;
    }
    LayerCommonData_p->OldCurrentField.PictureIndex = LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.PictureIndex;
    LayerCommonData_p->OldCurrentField.FieldType = LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.FieldType;

    if (ViewPortParams_p->Source_p->Data.VideoStream_p->PresentedFieldInverted)
    {
        TrFieldInversions(("\r\nField Inv display %d", DisplayInstanceIndex));
        LayerCommonData_p->FieldsWithFieldInversion++;
    }
#endif /* DBG_FIELD_INVERSIONS */

    /**********************************************************************************/
    /* Determine what kind of Field has been presented by the display (Top or Bottom) */
    if (LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->PresentedFieldInverted)
    {
        /*TrWarning(("\r\nInv MAIN"));*/
        /* Next field is presented inverted */
        if (LayerProperties_p->VTGSettings.VSyncType == STVTG_TOP)
        {
            /* Next VSync is Bot, next field to be presented is Top */
           LayerCommonData_p->PresentTopRegisters = TRUE;
           /*TrWarning(("TOP field presented on BOT VSync!\r\n"));*/
        }
        else
        {
            /* Next VSync is Top, next field to be presented is Bot */
            LayerCommonData_p->PresentTopRegisters = FALSE;
            /*TrWarning(("BOT field presented on TOP VSync!\r\n"));*/
        }
    }
    else
    {
        /* Next field is presented with good polarity */
        if (LayerProperties_p->VTGSettings.VSyncType == STVTG_BOTTOM)
        {
            /* Next VSync is Top, next field to be presented is Top */
            LayerCommonData_p->PresentTopRegisters = TRUE;
            TrDebug(("TOP field on output next field \r\n"));
        }
        else
        {
            /* Next VSync is Bot, next field to be presented is Bot */
            LayerCommonData_p->PresentTopRegisters = FALSE;
            TrDebug(("BOTTOM field on output next field \r\n"));
        }
    }

    if ( (!LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive) &&
         (LayerCommonData_p->IsOutputScantypeProgressive) )
    {
        /* Special case when VHSRC input is interlaced and output is progressive:
           We should present alternatively Top and Bot registers */
       if (LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->CurrentField.FieldType == STLAYER_TOP_FIELD)
       {
            LayerCommonData_p->PresentTopRegisters = TRUE;
       }
       else
       {
            LayerCommonData_p->PresentTopRegisters = FALSE;
       }
    }

    /**************************************************************************************/
    IsSourceFirstLineEven = (((InputRectangle.PositionY / 16) + 1) % 2);
    IsOutputFirstLineEven = ((OutputRectangle.PositionY + 1) % 2);

#ifdef DBG_FIELD_INVERSIONS
    if (!IsSourceFirstLineEven)
    {
        /* Source first line is odd: This is not normal */
        LayerCommonData_p->FieldsSetOnOddInputPosition++;
    }

    if (!IsOutputFirstLineEven)
    {
        /* Output first line is odd: This is not normal */
        LayerCommonData_p->FieldsSetOnOddOutputPosition++;
    }
#endif /* DBG_FIELD_INVERSIONS */

    /* Decide if display is done on opposite field */
    if (  (ViewPortParams_p->Source_p->Data.VideoStream_p->PresentedFieldInverted && !IsOutputFirstLineEven)
        ||(!ViewPortParams_p->Source_p->Data.VideoStream_p->PresentedFieldInverted && IsOutputFirstLineEven))
    {
        /* This is the usual case: The source is presented at the normal position. No compensation is necessary */
        IsFirstLineTopNotBot = TRUE;
    }
    else
    {
        /* Top and Bottom positions should be inverted by the VHSRC */
        IsFirstLineTopNotBot = FALSE;
    }

    /**************************************************************************************/
    stlayer_VDPGetIncrement(&InputRectangle,
                            &OutputRectangle,
                            LayerCommonData_p->PostDeiFormatInfo.IsFormat422Not420,
                            LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive,
                            LayerCommonData_p->IsOutputScantypeProgressive,
                            &Increment);

    /* As this function may have modified the output window properties, store it again */
    ViewPortParams_p->OutputRectangle = OutputRectangle;

    /* Set the Type of the Current Field */
    vdp_dei_SetCvfType(LayerHandle);

    /**************************************************************************************/
    stlayer_VDPGetPositionDefinition(&InputRectangle, &Increment,
                                     LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive,
                                     LayerCommonData_p->IsOutputScantypeProgressive,
                                     LayerCommonData_p->PostDeiFormatInfo.IsFormat422Not420,
                                     LayerCommonData_p->PostDeiFormatInfo.IsFormatRasterNotMacroblock,
                                     IsFirstLineTopNotBot,
                                     IsDeiActivated,
                                     IsPseudo422FromTopField,
                                     DecimXOffset,
                                     DecimYOffset,
                                     CurrentPictureHorizontalDecimation,
                                     CurrentPictureVerticalDecimation,
                                     LayerCommonData_p->AdvancedHDecimation,
                                     &PositionDefinition);

    Phase = PositionDefinition.Phase;
    FirstPixelRepetition = PositionDefinition.FirstPixelRepetition;

    MemorySize.Width1 = InputRectangle.Width;
    MemorySize.Width2 = InputRectangle.Width / 2;

    /**************************************************************************************/
    /* Input and Output scan types have an impact on Filter coeff selection */
    IncrementForCoeffSelectionChromaVertical = Increment.ChromaVertical;
    IncrementForCoeffSelectionLumaVertical = Increment.LumaVertical;
    /* luma */
    if (LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive && !LayerCommonData_p->IsOutputScantypeProgressive)
    {
        /* Progressive VHSRC Input and Interlaced Ouput */
        IncrementForCoeffSelectionLumaVertical = IncrementForCoeffSelectionLumaVertical >> 1;
    }
    /* chroma */
    if (LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive && !LayerCommonData_p->IsOutputScantypeProgressive)
    {
        /* Progressive VHSRC Input and Interlaced Ouput */
        IncrementForCoeffSelectionChromaVertical = IncrementForCoeffSelectionChromaVertical >> 1;
    }

    /**************************************************************************************/
    /* filter enable */
    if ((Increment.ChromaHorizontal != NO_ZOOM_FACTOR ) || (Phase.ChromaHorizontal != 0))
    {
        EnaCHF = 0x1;
    }
    else
    {
        EnaCHF = 0x0;
    }

    if ((Increment.LumaHorizontal != LUMA_VERTICAL_NO_ZOOM) || (Phase.LumaHorizontal != 0))
    {
        EnaYHF = 0x1;
    }
    else
    {
        EnaYHF = 0x0;
    }

    /**************************************************************************************/
    if ( (LayerCommonData_p->ReloadOfHorizontalFiltersInProgress) &&
         (LayerCommonData_p->FilterCoeffsReloaded) )
    {
        /* The horizontal coeff have been reloaded */
        void * TempBufferAddress_p;

#ifdef DBG_FILTER_COEFFS
        /* Check that the content has not been changed meantime. */
        DBG_CheckHorizontalFilterPointer(LayerHandle);
#endif

        /* The buffer "available" becomes the buffer "used by Hw" and vice versa */
        TempBufferAddress_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressUsedByHw_p;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressUsedByHw_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressAvailForWritting_p;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressAvailForWritting_p = TempBufferAddress_p;
        TrHorizontalCoeff(("\r\n HorizontalAddressUsedByHw_p = 0x%x", LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressUsedByHw_p));
        TrHorizontalCoeff(("\r\n HorizontalAddressAvailForWritting_p = 0x%x", LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressAvailForWritting_p));

        LayerCommonData_p->ReloadOfHorizontalFiltersInProgress = FALSE;
    }


    if ( (LayerCommonData_p->ReloadOfVerticalFiltersInProgress) &&
         (LayerCommonData_p->FilterCoeffsReloaded) )
    {
        /* The vertical coeff have been reloaded */
        void * TempBufferAddress_p;

#ifdef DBG_FILTER_COEFFS
        /* Check that the content has not been changed meantime. */
        DBG_CheckVerticalFilterPointer(LayerHandle);
#endif

        /* The buffer "available" becomes the buffer "used by Hw" and vice versa */
        TempBufferAddress_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressUsedByHw_p;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressUsedByHw_p = LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressAvailForWritting_p;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressAvailForWritting_p = TempBufferAddress_p;
        TrVerticalCoeff(("\r\n VerticalAddressUsedByHw_p = 0x%x", LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressUsedByHw_p));
        TrVerticalCoeff(("\r\n VerticalAddressAvailForWritting_p = 0x%x", LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressAvailForWritting_p));

        LayerCommonData_p->ReloadOfVerticalFiltersInProgress = FALSE;
    }

    /**************************************************************************************/
    /* Get addresses of buffers available for writting */
    HorizontalFilterPointer = (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressAvailForWritting_p;
    VerticalFilterPointer   = (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressAvailForWritting_p;

#ifdef HAL_VIDEO_LAYER_FILTER_DISABLE
    ForceNullCoefficients = TRUE;
#endif

#if defined(HAL_VIDEO_LAYER_FILTER_DISABLE)
    if (ForceNullCoefficients)
    {
        /* Special case with no filters */
        TrDebug(("Null Coefficient Filters\r\n"));

        /* Horizontal */
        stlayer_VDPWriteHorizontalNullCoefficientsFilterToMemory(STAVMEM_DeviceToCPU((U32 *)HorizontalFilterPointer, &VirtualMapping));
        stlayer_VDPWriteHorizontalNullCoefficientsFilterToMemory(STAVMEM_DeviceToCPU((U32 *)(HorizontalFilterPointer + HFC_NB_COEFS*sizeof(U32)), &VirtualMapping));
        HorizontalLumaFilter = VDP_FILTER_NULL;
        HorizontalChromaFilter = VDP_FILTER_NULL;
        EnaHFilterUpdate = 0x1;
        /* Save the new filter in use */
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalLumaFilter = HorizontalLumaFilter;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalChromaFilter = HorizontalChromaFilter;

        /* Vertical */
        stlayer_VDPWriteVerticalNullCoefficientsFilterToMemory(STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping));
        stlayer_VDPWriteVerticalNullCoefficientsFilterToMemory(STAVMEM_DeviceToCPU((U32 *)(VerticalFilterPointer + VFC_NB_COEFS*sizeof(U32)), &VirtualMapping));
        VerticalLumaFilter = VDP_FILTER_NULL;
        VerticalChromaFilter = VDP_FILTER_NULL;
        EnaVFilterUpdate = 0x1;
        /* Save the new filter in use */
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalLumaFilter = VerticalLumaFilter;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalChromaFilter = VerticalChromaFilter;
    }
    else
#endif /* defined(HAL_VIDEO_LAYER_FILTER_DISABLE) */
    {
        /* Usual case */

#if defined(DVD_SECURED_CHIP)
        /* Secured platforms : get address of the required filters in secured memory */
        stlayer_VDPGetHorizontalCoefficientsFilterAddress(
            (const U32**)&HorizontalFilterPointer, &HorizontalLumaFilter, &HorizontalChromaFilter,
            Increment.LumaHorizontal, Phase.LumaHorizontal);

        stlayer_VDPGetVerticalCoefficientsFilterAddress(
            (const U32**)&VerticalFilterPointer, &VerticalLumaFilter, &VerticalChromaFilter,
            IncrementForCoeffSelectionLumaVertical, PhaseForCoeffSelectionLumaVertical);

#else /* DVD_SECURED_CHIP */

        /* Non secured platforms: Select the appropriate filter */
        HorizontalLumaFilter = stlayer_VDPSelectHorizontalLumaFilter(
                Increment.LumaHorizontal,
                Phase.LumaHorizontal);
        HorizontalChromaFilter = stlayer_VDPSelectHorizontalChromaFilter(
                Increment.ChromaHorizontal,
                Phase.ChromaHorizontal);

        VerticalLumaFilter = stlayer_VDPSelectVerticalLumaFilter(
                IncrementForCoeffSelectionLumaVertical,
                Phase.LumaVerticalTop,
                Phase.LumaVerticalBottom);
        VerticalChromaFilter = stlayer_VDPSelectVerticalChromaFilter(
                IncrementForCoeffSelectionChromaVertical,
                Phase.ChromaVerticalTop,
                Phase.ChromaVerticalBottom);
#endif /* DVD_SECURED_CHIP */

        /* Check if the horizontal filter coeff should be reloaded */
        if ( (HorizontalLumaFilter != LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalLumaFilter) ||
             (HorizontalChromaFilter != LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalChromaFilter) ||
             (FiltersNeedReload) ||
             (LayerCommonData_p->ReloadOfHorizontalFiltersInProgress) )
        {
            /* Horizontal filter needs reloading */
            EnaHFilterUpdate = 0x1;

#if !defined(DVD_SECURED_CHIP)
            /* Fill the buffer with the appropriate coefficients */
            stlayer_VDPWriteHorizontalFilterCoefficientsToMemory(
                    STAVMEM_DeviceToCPU((U32 *)(HorizontalFilterPointer), &VirtualMapping),
                    HorizontalLumaFilter);
            stlayer_VDPWriteHorizontalFilterCoefficientsToMemory(
                    STAVMEM_DeviceToCPU((U32 *)(HorizontalFilterPointer + HFC_NB_COEFS*sizeof(U32)), &VirtualMapping),
                   /* HorizontalLumaFilter 20091015 change*/HorizontalChromaFilter);
#endif /* !defined(DVD_SECURED_CHIP) */

            /* Save the new filter in use */
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalLumaFilter = HorizontalLumaFilter;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalChromaFilter = HorizontalChromaFilter;

            TrHorizontalFilter(("\r\n Reload Horizontal VDP filter:"));
            TrHorizontalFilter(("\r\n   Luma: %s", DbgFilterType[HorizontalLumaFilter] ));
            TrHorizontalFilter(("\r\n   Chroma: %s", DbgFilterType[HorizontalChromaFilter] ));
        }

        /* Check if the vertical filter coeff should be reloaded */
        if ( (VerticalLumaFilter != LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalLumaFilter) ||
             (VerticalChromaFilter != LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalChromaFilter) ||
             (FiltersNeedReload) ||
             (LayerCommonData_p->ReloadOfVerticalFiltersInProgress) )
        {
            /* Vertical filter needs reloading */
            EnaVFilterUpdate = 0x1;

#if !defined(DVD_SECURED_CHIP)
            /* Fill the buffer with the appropriate coefficients */
            stlayer_VDPWriteVerticalFilterCoefficientsToMemory(
                    STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping),
                    VerticalLumaFilter);
            stlayer_VDPWriteVerticalFilterCoefficientsToMemory(
                    STAVMEM_DeviceToCPU((U32 *)(VerticalFilterPointer + VFC_NB_COEFS*sizeof(U32)), &VirtualMapping),
                    VerticalChromaFilter);
#endif /* !defined(DVD_SECURED_CHIP) */

            /* Save the new filter in use */
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalLumaFilter = VerticalLumaFilter;
            LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalChromaFilter = VerticalChromaFilter;

            TrVerticalFilter(("\r\n Reload Vertical VDP filter:"));
            TrVerticalFilter(("\r\n   Luma: %s", DbgFilterType[VerticalLumaFilter] ));
            TrVerticalFilter(("\r\n   Chroma: %s", DbgFilterType[VerticalChromaFilter] ));
        }
    }

    /**************************************************************************************/
    /* Check that the vertical filter selected is compatible with the Increment and Phase used on Top and Bottom fields */
    if (VerticalLumaFilter == VDP_FILTER_B)
    {
        /* FILTER_B corresponds to "no_filter" */
        if ( ( (Increment.LumaVertical != 0x2000) && (Increment.LumaVertical != 0x4000) )   ||
             (Phase.LumaVerticalTop != 0x1000)           ||
             (Phase.LumaVerticalBottom != 0x1000) )
        {
            TrError(("\r\nError on display %d ! VDP_FILTER_B is not adapted to current Luma Increment and Phase! (Inc=0x%x, Phase Top=0x%x Phase Bot=0x%x)",
                DisplayInstanceIndex,
                Increment.LumaVertical,
                Phase.LumaVerticalTop,
                Phase.LumaVerticalBottom));
        }
    }
    if (VerticalChromaFilter == VDP_FILTER_B)
    {
        /* FILTER_B corresponds to "no_filter" */
        if ( ( (Increment.ChromaVertical != 0x2000) && (Increment.ChromaVertical != 0x4000) )   ||
             (Phase.ChromaVerticalTop != 0x1000)           ||
             (Phase.ChromaVerticalBottom != 0x1000) )
        {
            TrError(("\r\nError on display %d ! VDP_FILTER_B is not adapted to current Chroma Increment and Phase! (Inc=0x%x, Phase Top=0x%x Phase Bot=0x%x)",
                DisplayInstanceIndex,
                Increment.ChromaVertical,
                Phase.ChromaVerticalTop,
                Phase.ChromaVerticalBottom));
        }
    }

    /**************************************************************************************/
    /** misc register computation **/
    /* MA_CTL register (top) */
    CoefLoadLine = 3 ;  /* Coef Load Line and Pix Load Line are set to not null values to optimize the bandwidth. */
    PixLoadLine = 4 ;

    TargetHeightTop = (OutputRectangle.Height + ((IsFirstLineTopNotBot ? 1 : 0) * ((!LayerCommonData_p->IsOutputScantypeProgressive) ? 1 : 0)));
    TargetHeightTop = (TargetHeightTop >> (S32)((!LayerCommonData_p->IsOutputScantypeProgressive) ? 1 : 0));
    TargetHeightBottom = (OutputRectangle.Height + (((!IsFirstLineTopNotBot) ? 1 : 0) * ((!LayerCommonData_p->IsOutputScantypeProgressive) ? 1 : 0)));
    TargetHeightBottom = (TargetHeightBottom >> (S32)((!LayerCommonData_p->IsOutputScantypeProgressive) ? 1 : 0));

    /* Retrieving ENA bit to know if display processor must be set active or not */
    EnaVHSRC = LayerCommonData_p->IsViewportEnabled ? 1 : 0;

    TrDebug(("R:%d, IOSTP:%d\r\n",
            LayerCommonData_p->PostDeiFormatInfo.IsFormatRasterNotMacroblock,
            LayerCommonData_p->IsOutputScantypeProgressive));


#ifdef DBG_USE_MANUAL_VHSRC_PARAMS
    if (LayerCommonData_p->ManualParams)
    {
        /* Use the manual params */
        FirstPixelRepetition.LumaTop = LayerCommonData_p->FirstPixelRepetitionLumaTop;
        FirstPixelRepetition.LumaBottom = LayerCommonData_p->FirstPixelRepetitionLumaBottom;
        Phase.LumaVerticalTop = LayerCommonData_p->PhaseRepetitionLumaTop;
        Phase.LumaVerticalBottom = LayerCommonData_p->PhaseRepetitionLumaBottom;
    }
    else
    {
        /* Params chosen automatically by the driver. Simply save them */
        LayerCommonData_p->FirstPixelRepetitionLumaTop = FirstPixelRepetition.LumaTop;
        LayerCommonData_p->FirstPixelRepetitionLumaBottom = FirstPixelRepetition.LumaBottom;
        LayerCommonData_p->PhaseRepetitionLumaTop = Phase.LumaVerticalTop;
        LayerCommonData_p->PhaseRepetitionLumaBottom = Phase.LumaVerticalBottom;
    }
#endif /* DBG_USE_MANUAL_VHSRC_PARAMS */

    if (LayerCommonData_p->IsViewportEnabled)
    {
        /* Check if Vertical Increment exceed the Max Zoom Out factor */
        if ( (Increment.LumaVertical   > LUMA_VERTICAL_MAX_ZOOM_OUT) ||
             (Increment.ChromaVertical > LUMA_VERTICAL_MAX_ZOOM_OUT) )
        {
            TrError(("\r\nError on display %d !Vertical Increment too big!", DisplayInstanceIndex));
        }

        /* Check if Horizontal Increment exceed the Max Zoom Out factor */
        if ( (Increment.LumaHorizontal   > LUMA_HORIZONTAL_MAX_ZOOM_OUT ) ||
             (Increment.ChromaHorizontal > LUMA_HORIZONTAL_MAX_ZOOM_OUT ) )
        {
            TrError(("\r\nError on display %d !Horizontal Increment too big!", DisplayInstanceIndex));
        }
    }

    /********** DEI Configuration *******************************************/

    DeiRegisterMap_p = &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.DeiRegisterMap;

    /* DEI Mode and CVF Type have been handled above in this function */

    vdp_dei_SetOutputViewPortOrigin(DeiRegisterMap_p, &InputRectangle);
    vdp_dei_SetOutputViewPortSize(DeiRegisterMap_p, &InputRectangle);

    /* VF_SIZE: 420MB video input width */
    vdp_dei_SetInputVideoFieldSize(DeiRegisterMap_p, SourcePictureHorizontalSize);

    vdp_dei_SetT3Interconnect (LayerHandle, DeiRegisterMap_p, &LayerCommonData_p->SourceFormatInfo);

    /* Compute Memory Access... */
    vdp_dei_ComputeMemoryAccess (SourcePictureHorizontalSize,
                                 SourcePictureVerticalSize,
                                 &LayerCommonData_p->SourceFormatInfo,
                                 &MemoryAccessParams,
                                 LayerHandle);

#ifdef STLAYER_USE_CRC
    if(ViewPortParams_p->Source_p->Data.VideoStream_p->CRCCheckMode == TRUE)
    {
        SetMemoryAccessForCRC(ViewPortParams_p, &MemoryAccessParams, SourcePictureHorizontalSize, SourcePictureVerticalSize);
    }
#endif /* STLAYER_USE_CRC */

    /*  ... and set the right mode */
    vdp_dei_SetMemoryAccess(DeiRegisterMap_p, &MemoryAccessParams, LayerHandle);

    /****** Configuration VHSRC TOP *********************************************/
    VhsrcTopRegister.RegDISP_VHSRC_CTL    =  (U32) 0x0;

    /* When Viewport is disabled, VHSRC_CTL=0 so the VHSRC is disabled */
    if (LayerCommonData_p->IsViewportEnabled)
    {
        VhsrcTopRegister.RegDISP_VHSRC_CTL = VhsrcTopRegister.RegDISP_VHSRC_CTL
                                              | (U32)((CoefLoadLine & VDP_VHSRC_CTL_CFLDLINE_MASK) << VDP_VHSRC_CTL_CFLDLINE_EMP)
                                              | (U32)((PixLoadLine & VDP_VHSRC_CTL_PXLDLINE_MASK) << VDP_VHSRC_CTL_PXLDLINE_EMP)
                                              | ((U32)EnaVHSRC << VDP_VHSRC_CTL_ENA_VHSRC_EMP)
                                              | ((U32)EnaHFilterUpdate << 4) | ((U32)EnaVFilterUpdate << 3)
                                              | ((U32)EnaCHF << 2) | ((U32)EnaYHF << 1) | 0x0;
    }

    VhsrcTopRegister.RegDISP_VHSRC_CTL   &= VDP_VHSRC_CTL_MASK;

    VhsrcTopRegister.RegDISP_VHSRC_Y_HSRC = (FirstPixelRepetition.LumaProgressive << 29) + (Phase.LumaHorizontal << 16)
                                                + Increment.LumaHorizontal;
    VhsrcTopRegister.RegDISP_VHSRC_C_HSRC = (FirstPixelRepetition.ChromaProgressive << 29) + (Phase.ChromaHorizontal << 16)
                                                + Increment.ChromaHorizontal;

    VhsrcTopRegister.RegDISP_VHSRC_Y_VSRC = (FirstPixelRepetition.LumaTop << 29) + (Phase.LumaVerticalTop << 16)
                                                + Increment.LumaVertical;
    VhsrcTopRegister.RegDISP_VHSRC_C_VSRC = (FirstPixelRepetition.ChromaTop << 29) + (Phase.ChromaVerticalTop << 16)
                                                + Increment.ChromaVertical;

    VhsrcTopRegister.RegDISP_VHSRC_TARGET_SIZE = (TargetHeightTop <<16) + OutputRectangle.Width;

    VhsrcTopRegister.RegDISP_VHSRC_NLZZD_Y = 0;
    VhsrcTopRegister.RegDISP_VHSRC_NLZZD_C = 0;
    VhsrcTopRegister.RegDISP_VHSRC_PDELTA = 0;

    /* Ensure that minimum values for VHSRC_Y_SIZE and VHSRC_C_SIZE are respected */
    VhsrcTopRegister.RegDISP_VHSRC_Y_SIZE = Max(SOURCE_WIDTH1_MINIMUM_VALUE, MemorySize.Width1);
    VhsrcTopRegister.RegDISP_VHSRC_C_SIZE = Max(SOURCE_WIDTH2_MINIMUM_VALUE, MemorySize.Width2);

#if defined(DVD_SECURED_CHIP)
    VhsrcTopRegister.RegDISP_VHSRC_VCOEF_BA = (U32)STAVMEM_VirtualToDevice((void*)VerticalFilterPointer, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
    VhsrcTopRegister.RegDISP_VHSRC_HCOEF_BA = (U32)STAVMEM_VirtualToDevice((void*)HorizontalFilterPointer, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
#else /* defined(DVD_SECURED_CHIP) */
    VhsrcTopRegister.RegDISP_VHSRC_VCOEF_BA = VerticalFilterPointer;
    VhsrcTopRegister.RegDISP_VHSRC_HCOEF_BA = HorizontalFilterPointer;
#endif /* defined(DVD_SECURED_CHIP) */

    /********** Configuration VHSRC BOT *******************************************/

    VhsrcBottomRegister.RegDISP_VHSRC_CTL = VhsrcTopRegister.RegDISP_VHSRC_CTL;
    VhsrcBottomRegister.RegDISP_VHSRC_Y_HSRC = VhsrcTopRegister.RegDISP_VHSRC_Y_HSRC;
    VhsrcBottomRegister.RegDISP_VHSRC_C_HSRC = VhsrcTopRegister.RegDISP_VHSRC_C_HSRC;

    VhsrcBottomRegister.RegDISP_VHSRC_Y_VSRC = (FirstPixelRepetition.LumaBottom << 29) + (Phase.LumaVerticalBottom << 16)
                                                + Increment.LumaVertical;
    VhsrcBottomRegister.RegDISP_VHSRC_C_VSRC = (FirstPixelRepetition.ChromaBottom << 29) + (Phase.ChromaVerticalBottom << 16)
                                                + Increment.ChromaVertical;

    VhsrcBottomRegister.RegDISP_VHSRC_TARGET_SIZE = (TargetHeightBottom << 16) + OutputRectangle.Width;

    VhsrcBottomRegister.RegDISP_VHSRC_NLZZD_Y = VhsrcTopRegister.RegDISP_VHSRC_NLZZD_Y;
    VhsrcBottomRegister.RegDISP_VHSRC_NLZZD_C = VhsrcTopRegister.RegDISP_VHSRC_NLZZD_C;
    VhsrcBottomRegister.RegDISP_VHSRC_PDELTA  = VhsrcTopRegister.RegDISP_VHSRC_PDELTA;

    VhsrcBottomRegister.RegDISP_VHSRC_Y_SIZE      = VhsrcTopRegister.RegDISP_VHSRC_Y_SIZE;
    VhsrcBottomRegister.RegDISP_VHSRC_C_SIZE      = VhsrcTopRegister.RegDISP_VHSRC_C_SIZE;
    VhsrcBottomRegister.RegDISP_VHSRC_VCOEF_BA    = VhsrcTopRegister.RegDISP_VHSRC_VCOEF_BA;
    VhsrcBottomRegister.RegDISP_VHSRC_HCOEF_BA    = VhsrcTopRegister.RegDISP_VHSRC_HCOEF_BA;

    /********** Update Registers *******************************************/

#ifdef HAL_VIDEO_LAYER_REG_DEBUG
    /* if debug set is used ... */
    if (UseLocalDisplayRegisters)
    {
        /* Alternate register set to write to display */
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop       = VhsrcTopDebugRegister;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapBottom    = VhsrcBottomDebugRegister;
    }
    else
#endif
    {
        /* Saving in debug alternate register set */
#ifdef HAL_VIDEO_LAYER_REG_DEBUG
        VhsrcTopDebugRegister    = VhsrcTopRegister;
        VhsrcBottomDebugRegister = VhsrcBottomRegister;
#endif
        /* Normal register set to write to display */
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop       = VhsrcTopRegister;
        LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapBottom    = VhsrcBottomRegister;
    }


    /* If all the fields are the same, we update here and STLAYER will be static.   */
    /* If not, we update here but there will be also an update at each VSync.       */
    if ((VhsrcTopRegister.RegDISP_VHSRC_Y_VSRC == VhsrcBottomRegister.RegDISP_VHSRC_Y_VSRC) &&
        (VhsrcTopRegister.RegDISP_VHSRC_C_VSRC == VhsrcBottomRegister.RegDISP_VHSRC_C_VSRC) &&
        (VhsrcTopRegister.RegDISP_VHSRC_TARGET_SIZE == VhsrcBottomRegister.RegDISP_VHSRC_TARGET_SIZE) )
    {
        LayerCommonData_p->IsPeriodicalUpdateNeeded = FALSE;
    }
    else
    {
        LayerCommonData_p->IsPeriodicalUpdateNeeded = TRUE;
    }

    if (LayerCommonData_p->PresentTopRegisters)
    {
        stlayer_VDPUpdateDisplayRegisters(LayerHandle,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.DeiRegisterMap,
                EnaHFilterUpdate,
                EnaVFilterUpdate);
    }
    else
    {
        stlayer_VDPUpdateDisplayRegisters(LayerHandle,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapBottom,
                &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.DeiRegisterMap,
                EnaHFilterUpdate,
                EnaVFilterUpdate);
    }

    if (EnaHFilterUpdate==0x1)
    {
        TrHorizontalCoeff(("\r\n Last Luma Horizontal Coeff (0x%x-0x%x): %d %d %d %d",
            HorizontalFilterPointer,
            STAVMEM_DeviceToCPU( (U32 *) HorizontalFilterPointer, &VirtualMapping),
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)HorizontalFilterPointer, &VirtualMapping ))[HFC_NB_COEFS*4-4],
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)HorizontalFilterPointer, &VirtualMapping ))[HFC_NB_COEFS*4-3],
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)HorizontalFilterPointer, &VirtualMapping ))[HFC_NB_COEFS*4-2],
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)HorizontalFilterPointer, &VirtualMapping ))[HFC_NB_COEFS*4-1] ));

        /* Horizontal Filter coeffs have been updated: They will be reloaded at next VSync */
        LayerCommonData_p->FilterCoeffsReloaded = FALSE;
        LayerCommonData_p->ReloadOfHorizontalFiltersInProgress = TRUE;

       /* Check HorizontalFilterPointer */
       if (HorizontalFilterPointer !=  (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressAvailForWritting_p)
       {
            TrError(("\r\nError on display %d ! Invalid HorizontalFilterPointer (0x%x)", DisplayInstanceIndex, HorizontalFilterPointer));
       }

#ifdef DBG_FILTER_COEFFS
       LayerCommonData_p->DbgLastU32OfHorizontalLumaBuffer = ( (U32 *) STAVMEM_DeviceToCPU((U32 *)HorizontalFilterPointer, &VirtualMapping ))[HFC_NB_COEFS-1];
#endif
    }

    if (EnaVFilterUpdate == 0x1)
    {
        TrVerticalCoeff(("\r\n Last Luma Vertical Coeff (0x%x-0x%x): %d %d %d %d",
            VerticalFilterPointer,
            STAVMEM_DeviceToCPU( (U32 *) VerticalFilterPointer, &VirtualMapping),
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping ))[VFC_NB_COEFS*4-4],
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping ))[VFC_NB_COEFS*4-3],
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping ))[VFC_NB_COEFS*4-2],
            ( (S8 *) STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping ))[VFC_NB_COEFS*4-1] ));

        /* Vertical Filter coeffs have been updated. They will be reloaded at next VSync */
        LayerCommonData_p->FilterCoeffsReloaded = FALSE;
        LayerCommonData_p->ReloadOfVerticalFiltersInProgress = TRUE;

       /* Check VerticalFilterPointer */
       if (VerticalFilterPointer !=  (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressAvailForWritting_p)
       {
            TrError(("\r\nError on display %d ! Invalid VerticalFilterPointer (0x%x)", DisplayInstanceIndex, VerticalFilterPointer));
       }

#ifdef DBG_FILTER_COEFFS
       LayerCommonData_p->DbgLastU32OfVerticalLumaBuffer = ( (U32 *) STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping ))[VFC_NB_COEFS-1];
#endif
    }

#ifdef DBG_CHECK_VDP_CONSISTENCY
    CheckVdpConsistency(LayerHandle,
                        &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop,
                        &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapBottom,
                        &LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.DeiRegisterMap,
                        &Increment);
#endif

    /* Display periodical status every 100 fields */
    if (LayerCommonData_p->VSyncCounter++ > 100)
    {
#ifdef DBG_FIELD_INVERSIONS
        /* If the display is frozen on a same field, there is no need to check the field inversions */
        if (!SameFieldRepeated)
        {
            /* Display an alarm if there is more than 30% fields with field inversion */
            if (LayerCommonData_p->FieldsWithFieldInversion > 30)
            {
                TrError(("\r\nError on display %d! Suspicious field inversions!!!", DisplayInstanceIndex));
            }

            /* Display an alarm if there is more than 5% fields wrong output position */
            if (LayerCommonData_p->FieldsSetOnOddOutputPosition > 5)
            {
                TrError(("\r\nError on display %d ! Output set on odd position!", DisplayInstanceIndex));
            }

            /* Display an alarm if there is more than 5% fields wrong input position */
            if (LayerCommonData_p->FieldsSetOnOddInputPosition > 5)
            {
                TrError(("\r\nError on display %d ! Input set on odd position!", DisplayInstanceIndex));
            }
        }

        /* Reset counters */
        LayerCommonData_p->FieldsWithFieldInversion = 0;
        LayerCommonData_p->FieldsSetOnOddOutputPosition = 0;
        LayerCommonData_p->FieldsSetOnOddInputPosition = 0;
#endif /* DBG_FIELD_INVERSIONS */

        LayerCommonData_p->VSyncCounter = 0;
    }

#ifdef STLAYER_USE_FMD
    /********** FMD *******************************************/
    if(LayerCommonData_p->FmdReportingEnabled)
    {
        STLAYER_FMDResults_t FMDResults;

        vdp_fmd_CheckFmdSetup(LayerHandle);

        if(vdp_fmd_GetFmdResults(LayerHandle, &FMDResults))
        {
                /* Notify application that a new FMD was reported */
                STEVT_Notify(LayerProperties_p->EventsHandle,
                        LayerProperties_p->RegisteredEventsID[HALLAYER_NEW_FMD_REPORTED_EVT_ID],
                        (const void *) (&FMDResults));
        }
    }
#endif
}/* End of ComputeAndApplyThePolyphaseFilters */


#ifdef DBG_CHECK_VDP_CONSISTENCY
/*******************************************************************************
Name        : CheckVdpConsistency
Description : This functions checks that DEI, VHSRC and Compo settings are consistent.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void CheckVdpConsistency(STLAYER_Handle_t        LayerHandle,
                    HALLAYER_VDPVhsrcRegisterMap_t *    VhsrcTopRegister_p,
                    HALLAYER_VDPVhsrcRegisterMap_t *    VhsrcBottomRegister_p,
                    HALLAYER_VDPDeiRegisterMap_t *      DeiRegister_p,
                    HALLAYER_VDPIncrement_t *           Increment_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;
    U32     PositionY;
    U32     DeiOutputWidth;
    U32     DeiOutputFrameHeight;
    U32     DeiOutputHeight;
    U32     DeiOutputTopHeight;
    U32     DeiOutputBottomHeight;
    U32     VhsrcTargetWidth;
    U32     VhsrcTargetHeight;
    U32     CompoWidth;
    U32     CompoFrameHeight;
    U32     CompoHeight;
    U32     CompoTopHeight;
    U32     CompoBottomHeight;
    U32     VpsRegisterValue;
    U32     VpoRegisterValue;
    U32     XDO, YDO, XDS, YDS;
    U32     ChromaVerticalZoom;
    U32     DeiCtlRegisterValue;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* WARNING:
       Compo and DEI dimensions are FRAME.
       VHSRC dimensions are FIELD.
    */


    /***************************/
    /* Get Compositor settings */
    /***************************/
    VpoRegisterValue = HAL_ReadGam32(GAM_VIDn_VPO);
    VpsRegisterValue = HAL_ReadGam32(GAM_VIDn_VPS);

    XDO = VpoRegisterValue & 0xfff;
    YDO = VpoRegisterValue >> 16;

    XDS = VpsRegisterValue & 0xfff;
    YDS = VpsRegisterValue >> 16;

    if (XDO == XDS)
    {
        CompoWidth  = 0;
    }
    else
    {
        CompoWidth  = XDS - XDO + 1;
    }

    if (YDO == YDS)
    {
        CompoFrameHeight  = 0;
    }
    else
    {
        CompoFrameHeight = YDS - YDO + 1;
    }

    if (!LayerCommonData_p->IsViewportEnabled)
    {
        /* VDP not enabled: Simply check that DEI and VSHRC are disabled and output viewport set to a null size */
        if ( (CompoWidth != 0) || (CompoFrameHeight != 0) )
        {
            TrWarning(("\r\n Output viewport is not disabled!"));
        }

        DeiCtlRegisterValue = HAL_ReadDisp32(VDP_DEI_CTL);
        if (!(DeiCtlRegisterValue & (VDP_DEI_CTL_INACTIVE_MASK << VDP_DEI_CTL_INACTIVE_EMP)))
        {
            TrWarning(("\r\n DEI is not disabled!"));
        }

        if (VhsrcTopRegister_p->RegDISP_VHSRC_CTL & (VDP_VHSRC_CTL_ENA_VHSRC_MASK << VDP_VHSRC_CTL_ENA_VHSRC_EMP))
        {
            TrWarning(("\r\n VHSRC is not disabled!"));
        }

        /* Nothing else to check */
        return;
    }

    if (YDO & 0x01)
    {
        /* First line is Odd (= Bottom) */
        CompoTopHeight = CompoFrameHeight >> 1;
        CompoBottomHeight = CompoFrameHeight - CompoTopHeight;
    }
    else
    {
        /* First line is Even (=Top) */
        CompoBottomHeight = CompoFrameHeight >> 1;
        CompoTopHeight = CompoFrameHeight - CompoBottomHeight;
    }

    if (LayerCommonData_p->IsOutputScantypeProgressive)
    {
        CompoHeight = CompoFrameHeight;
    }
    else
    {
        if (LayerCommonData_p->PresentTopRegisters)
        {
            /* Next VSync will be a Top */
            CompoHeight = CompoTopHeight;
        }
        else
        {
            /* Next VSync will be a Bot */
            CompoHeight = CompoBottomHeight;
        }
    }


    /********************/
    /* Get DEI settings */
    /********************/
    PositionY = (DeiRegister_p->RegDISP_DEI_VIEWPORT_ORIG >> VDP_DEI_VIEWPORT_ORIG_Y_EMP);

    DeiOutputWidth = (DeiRegister_p->RegDISP_DEI_VIEWPORT_SIZE & VDP_DEI_VIEWPORT_SIZE_W_MASK);
    DeiOutputFrameHeight = (DeiRegister_p->RegDISP_DEI_VIEWPORT_SIZE >> VDP_DEI_VIEWPORT_SIZE_HEIGHT_EMP);

    if (LayerCommonData_p->PostDeiFormatInfo.IsInputScantypeProgressive)
    {
        /* VHSRC input is progressive so the whole frame is output by the DEI */
        DeiOutputHeight = DeiOutputFrameHeight;
    }
    else
    {
        /* VHSRC input is Interlaced */

        if (PositionY & 0x01)
        {
            /* First line is Odd (= Bottom) */
            DeiOutputTopHeight = DeiOutputFrameHeight >> 1;
            DeiOutputBottomHeight = DeiOutputFrameHeight - DeiOutputTopHeight;
        }
        else
        {
            /* First line is Even (=Top) */
            DeiOutputBottomHeight = DeiOutputFrameHeight >> 1;
            DeiOutputTopHeight = DeiOutputFrameHeight - DeiOutputBottomHeight;
        }

        if (LayerCommonData_p->PresentTopRegisters)
        {
            /* Next VSync will be a Top */
            DeiOutputHeight = DeiOutputTopHeight;
        }
        else
        {
            /* Next VSync will be a Bot */
            DeiOutputHeight = DeiOutputBottomHeight;
        }
    }


    /**********************/
    /* Get VHSRC settings */
    /**********************/
    if (LayerCommonData_p->PresentTopRegisters)
    {
        /* Next VSync will be a Top */
        VhsrcTargetWidth = (VhsrcTopRegister_p->RegDISP_VHSRC_TARGET_SIZE & 0x7ff);
        VhsrcTargetHeight = (VhsrcTopRegister_p->RegDISP_VHSRC_TARGET_SIZE >> 16);
    }
    else
    {
        /* Next VSync will be a Bot */
        VhsrcTargetWidth = (VhsrcBottomRegister_p->RegDISP_VHSRC_TARGET_SIZE & 0x7ff);
        VhsrcTargetHeight = (VhsrcBottomRegister_p->RegDISP_VHSRC_TARGET_SIZE >> 16);
    }


    /* Check that Compo and VHSRC settings are consistent */
    if (CompoWidth != VhsrcTargetWidth)
    {
        TrWarning(("\r\n Compo and VHSRC width not consistent: 0x%x 0x%x", CompoWidth, VhsrcTargetWidth));
    }

    if (CompoHeight != VhsrcTargetHeight)
    {
        TrWarning(("\r\n Compo and VHSRC height not consistent: 0x%x 0x%x", CompoHeight, VhsrcTargetHeight));
    }

    if ( (VhsrcTargetHeight == 0) ||
         (VhsrcTargetWidth == 0) )
    {
        /* The current Viewport is not valid so no checking are necessary */
        return;
    }

    /* Check that DEI and VHSRC settings are consistent */

    /* Check Luma settings */
    if (Increment_p->LumaVertical != (DeiOutputHeight * 8192 / VhsrcTargetHeight) )
    {
        TrWarning(("\r\n DEI and VHSRC Luma Height not consistent"));
    }

    if (Increment_p->LumaHorizontal != (DeiOutputWidth  * 8192 / VhsrcTargetWidth) )
    {
        TrWarning(("\r\n DEI and VHSRC Luma Width not consistent"));
    }


    /* Check Chroma settings */
    if  (vdp_dei_IsDeiActivated(LayerHandle))
    {
        /* DEI output is in pseudo 4:2:2: No vertical Chroma zoom is needed in VHSRC */
        ChromaVerticalZoom = 1;
    }
    else
    {
        /* DEI output is 4:2:0: A vertical Chroma zoom x2 is needed in VHSRC (in order to generate 4:2:2) */
        ChromaVerticalZoom = 2;
    }

    if (Increment_p->ChromaVertical != (DeiOutputHeight * 8192 / (VhsrcTargetHeight * ChromaVerticalZoom) ) )
    {
        TrWarning(("\r\n DEI and VHSRC Chroma Height not consistent"));
    }

    /* Chroma should be zoomed x2 horizontaly (for 4:2:2 to 4:4:4 conversion) */
    if (Increment_p->ChromaHorizontal != (DeiOutputWidth  * 8192 / (VhsrcTargetWidth * 2) ) )
    {
        TrWarning(("\r\n DEI and VHSRC Chroma Width not consistent"));
    }

}
#endif /* DBG_CHECK_VDP_CONSISTENCY*/

/*******************************************************************************
Name        : SetViewportOutputRectangle
Description : According to the enable/disable viewport flag, the output
              viewport is set or not to the correct values in order to
              see it.
Parameters  : HAL layer common data : LayerCommonData_p
Assumptions : The fields IsViewportEnabled, ViewportOffset and ViewportSize
              are assumed to be correctly set.
Limitations :
Returns     :
*******************************************************************************/
static void SetViewportOutputRectangle (const HALLAYER_LayerProperties_t *    LayerProperties_p)
{
    U32                             OutputHorizontalSize, OutputVerticalSize;
    U32                             OutputHorizontalStart,OutputVerticalStart;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p;

    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    /* init Layer properties.                   */
    LayerCommonData_p = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Test if the viewport is enable or not */
    if (LayerCommonData_p->IsViewportEnabled)
    {
        /* The viewport is enabled. We must apply the right parameters. */
        /* Viewport output width and height */
        if (LayerCommonData_p->ViewPortParams.OutputRectangle.Width <= VDP_VHSRC_MAX_SIZE_MASK)
        {
            OutputHorizontalSize = LayerCommonData_p->ViewPortParams.OutputRectangle.Width;
        }
        else
        {
            OutputHorizontalSize = VDP_VHSRC_MAX_SIZE_MASK;
        }
        if (LayerCommonData_p->ViewPortParams.OutputRectangle.Height <= VDP_VHSRC_MAX_SIZE_MASK)
        {
            OutputVerticalSize   = LayerCommonData_p->ViewPortParams.OutputRectangle.Height;
        }
        else
        {
            OutputVerticalSize = VDP_VHSRC_MAX_SIZE_MASK;
        }

        /* Viewport output positions */
        OutputHorizontalStart = LayerCommonData_p->ViewPortParams.OutputRectangle.PositionX
                                + LayerCommonData_p->LayerPosition.XStart
                                + LayerCommonData_p->LayerPosition.XOffset;
        OutputVerticalStart  = LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY
                                + LayerCommonData_p->LayerPosition.YStart
                                + LayerCommonData_p->LayerPosition.YOffset;

        /* Save the viewport output window dimensions */
        LayerCommonData_p->ViewportOffset = ((OutputVerticalStart & VDP_VHSRC_MAX_XY_MASK) << VDP_VHSRC_Y_EMP) |
                                            ((OutputHorizontalStart & VDP_VHSRC_MAX_XY_MASK) << VDP_VHSRC_X_EMP);


        /* Check if the OutputVerticalSize and OutputHorizontalSize are available */
        if ((OutputHorizontalSize == 0) || (OutputVerticalSize == 0))
        {
            LayerCommonData_p->ViewportSize = 0;
        }
        else
        {
            LayerCommonData_p->ViewportSize =
                    (((OutputVerticalSize   -1) & VDP_VHSRC_MAX_SIZE_MASK) << VDP_VHSRC_HEIGHT_EMP) |
                    (((OutputHorizontalSize -1) & VDP_VHSRC_MAX_SIZE_MASK) << VDP_VHSRC_WIDTH_EMP);
        }
    }
    else
    {
        /* The viewport is disabled. We must apply an invisible area parameters. */
        LayerCommonData_p->ViewportOffset = 0;
        LayerCommonData_p->ViewportSize = 0;
    }

    /* ViewportOffset and ViewportSize have changed: The change will be taken into account at next VSync */
    LayerCommonData_p->UpdateViewport = TRUE;
} /* End of SetViewportOutputRectangle */

/*******************************************************************************
Name        : IsUpdateNeeded
Description : Tell the upper level if an update is needed on next VSync.
              This function is also used by the HAL to know that a VSync has occured.
Parameters  :
Assumptions :
Limitations :
Returns     : BOOL
*******************************************************************************/
static BOOL IsUpdateNeeded(const STLAYER_Handle_t LayerHandle)

{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *) LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *) (LayerProperties_p->PrivateData_p);

    if ( (LayerCommonData_p->ReloadOfHorizontalFiltersInProgress) ||
         (LayerCommonData_p->ReloadOfVerticalFiltersInProgress) )
    {
        /* A VSync has occured so the Filter coeffs have been reloaded */
        LayerCommonData_p->FilterCoeffsReloaded = TRUE;
    }

    return(LayerCommonData_p->IsPeriodicalUpdateNeeded);
} /* End of IsUpdateNeeded() function. */


/*******************************************************************************
Name        : SynchronizedUpdate
Description : called in a task scheduled on Vsync callback (by upper level)
Parameters  :
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
static void SynchronizedUpdate(const STLAYER_Handle_t LayerHandle)
{
#ifdef TRACE_UART
    HALLAYER_LayerProperties_t  * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t  * LayerCommonData_p;
    U8                          DisplayInstanceIndex;

    /* Init Layer/Viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    switch (LayerProperties_p->LayerType)
    {
        case MAIN_DISPLAY :
            DisplayInstanceIndex = 0;
            break;

        case AUX_DISPLAY :
            DisplayInstanceIndex = 1;
            break;

#if defined (HW_7200)
        case SEC_DISPLAY :
            DisplayInstanceIndex = 2;
            break;
#endif /* HW_7200 */

        default :
            break;
    }

    #ifndef ST_OSLINUX
        TrDebug (("DIIdx:%d\r\n",DisplayInstanceIndex));
    #endif
#else
    UNUSED_PARAMETER(LayerHandle);
#endif /* TRACE_UART */

}

/*******************************************************************************
Name        : GetDisplayParams
Description : This function gets params used for Display
Parameters  : ViewportHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated DisplayParams data type
Assumptions :
Limitations :
Returns     : - ST_ERROR_BAD_PARAMETER, if one parameter is wrong.
              - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t GetVideoDisplayParams (const STLAYER_ViewPortHandle_t ViewportHandle,
                                    STLAYER_VideoDisplayParams_t * Params_p)
{

    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;
    U32                             RegAdr,DisplayInstanceIndex;

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *) ViewportHandle;

    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);

    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters                                            */
    if (LayerCommonData_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    if (LayerProperties_p->LayerType == MAIN_DISPLAY)
    {
        DisplayInstanceIndex = 0;
    }
    else
    {
        DisplayInstanceIndex = 1;
    }

    RegAdr=LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop.RegDISP_VHSRC_Y_HSRC;
    /*RegAdr=HAL_ReadDisp32(VDP_VHSRC_Y_HSRC);*/
    Params_p->VHSRCparams.LumaHorizontal.Increment=(RegAdr&0x0000FFFF) ;
    Params_p->VHSRCparams.LumaHorizontal.InitialPhase=(((U32)RegAdr>>16)&0x00001FFF) ;
    Params_p->VHSRCparams.LumaHorizontal.PixelRepeat=(((U32)RegAdr>>29)&0x00000007) ;

    RegAdr=LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop.RegDISP_VHSRC_C_HSRC;
    /*RegAdr=HAL_ReadDisp32(VDP_VHSRC_C_HSRC);*/
    Params_p->VHSRCparams.ChromaHorizontal.Increment=(RegAdr&0x0000FFFF) ;
    Params_p->VHSRCparams.ChromaHorizontal.InitialPhase=(((U32)RegAdr>>16)&0x00001FFF) ;
    Params_p->VHSRCparams.ChromaHorizontal.PixelRepeat=(((U32)RegAdr>>29)&0x00000007) ;

    RegAdr=LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop.RegDISP_VHSRC_Y_VSRC;
    /*RegAdr=HAL_ReadDisp32(VDP_VHSRC_Y_VSRC);*/

    Params_p->VHSRCparams.LumaTop.Increment=(RegAdr&0x0000FFFF) ;
    Params_p->VHSRCparams.LumaTop.InitialPhase=(((U32)RegAdr>>16)&0x00001FFF) ;
    Params_p->VHSRCparams.LumaTop.LineRepeat=(((U32)RegAdr>>29)&0x00000003) ;


    RegAdr=LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapTop.RegDISP_VHSRC_C_VSRC;
    /*RegAdr=HAL_ReadDisp32(VDP_VHSRC_C_VSRC);*/
    Params_p->VHSRCparams.ChromaTop.Increment=(RegAdr&0x0000FFFF) ;
    Params_p->VHSRCparams.ChromaTop.InitialPhase=(((U32)RegAdr>>16)&0x00001FFF) ;
    Params_p->VHSRCparams.ChromaTop.LineRepeat=(((U32)RegAdr>>29)&0x00000003) ;

    RegAdr=LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapBottom.RegDISP_VHSRC_Y_VSRC;
    /*RegAdr=HAL_ReadDisp32(VDP_VHSRC_Y_VSRC);*/

    Params_p->VHSRCparams.LumaBot.Increment=(RegAdr&0x0000FFFF) ;
    Params_p->VHSRCparams.LumaBot.InitialPhase=(((U32)RegAdr>>16)&0x00001FFF) ;
    Params_p->VHSRCparams.LumaBot.LineRepeat=(((U32)RegAdr>>29)&0x00000003) ;


    RegAdr=LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].RegisterConfiguration.VhsrcRegisterMapBottom.RegDISP_VHSRC_C_VSRC;
    /*RegAdr=HAL_ReadDisp32(VDP_VHSRC_C_VSRC);*/
    Params_p->VHSRCparams.ChromaBot.Increment=(RegAdr&0x0000FFFF) ;
    Params_p->VHSRCparams.ChromaBot.InitialPhase=(((U32)RegAdr>>16)&0x00001FFF) ;
    Params_p->VHSRCparams.ChromaBot.LineRepeat=(((U32)RegAdr>>29)&0x00000003) ;

    Params_p->InputRectangle.PositionX = LayerCommonData_p->ViewPortParams.InputRectangle.PositionX / LayerCommonData_p->CurrentPictureHorizontalDecimationFactor;
    Params_p->InputRectangle.PositionY = LayerCommonData_p->ViewPortParams.InputRectangle.PositionY / LayerCommonData_p->CurrentPictureVerticalDecimationFactor;
    Params_p->InputRectangle.Width     = LayerCommonData_p->ViewPortParams.InputRectangle.Width     / LayerCommonData_p->CurrentPictureHorizontalDecimationFactor;
    Params_p->InputRectangle.Height    = LayerCommonData_p->ViewPortParams.InputRectangle.Height    / LayerCommonData_p->CurrentPictureVerticalDecimationFactor;

    Params_p->OutputRectangle=LayerCommonData_p->ViewPortParams.OutputRectangle;

    Params_p->VerticalDecimFactor=LayerCommonData_p->RecommendedVerticalDecimation;
    Params_p->HorizontalDecimFactor=LayerCommonData_p->RecommendedHorizontalDecimation;

    Params_p->SourceScanType=((((LayerCommonData_p->ViewPortParams.Source_p)->Data).VideoStream_p)->ScanType);
    Params_p->DeiMode=LayerCommonData_p->AppliedDeiMode;


    /* Fill LumaHorizontalCoef Filter */
    switch(LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalLumaFilter)
    {
        case VDP_FILTER_H_CHRX2:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoef, HorizontalCoefficientsChromaZoomX2, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoefType = VHSRC_FILTER_CHRX2;
            break;
        case VDP_FILTER_A:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoef, HorizontalCoefficientsA, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoefType = VHSRC_FILTER_A;
            break;
        case VDP_FILTER_B:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoef, HorizontalCoefficientsB, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoefType = VHSRC_FILTER_B;
            break;
        case VDP_FILTER_Y_C:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoef, HorizontalCoefficientsLumaC, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoefType = VHSRC_FILTER_C;
            break;
        case VDP_FILTER_Y_D:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoef, HorizontalCoefficientsLumaD, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoefType = VHSRC_FILTER_D;
            break;
        case VDP_FILTER_Y_E:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoef, HorizontalCoefficientsLumaE, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoefType = VHSRC_FILTER_E;
            break;
        case VDP_FILTER_Y_F:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoef, HorizontalCoefficientsLumaF, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaHorizontalCoefType = VHSRC_FILTER_F;
            break;

        default:
            TrError(("Error on display %d !Unknown type of HorizontalLumaFilter %d \n", DisplayInstanceIndex, LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalLumaFilter));
            break;
    }


    /* Fill Chroma Horizontal Coef Filters*/
    switch(LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalChromaFilter)
    {
        case VDP_FILTER_H_CHRX2:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoef, HorizontalCoefficientsChromaZoomX2, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoefType = VHSRC_FILTER_CHRX2;
            break;
        case VDP_FILTER_A:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoef, HorizontalCoefficientsA, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoefType = VHSRC_FILTER_A;
            break;
        case VDP_FILTER_B:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoef, HorizontalCoefficientsB, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoefType = VHSRC_FILTER_B;
            break;
        case VDP_FILTER_C_C:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoef, HorizontalCoefficientsChromaC, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoefType = VHSRC_FILTER_C;
            break;
        case VDP_FILTER_C_D:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoef, HorizontalCoefficientsChromaD, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoefType = VHSRC_FILTER_D;
            break;
        case VDP_FILTER_C_E:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoef, HorizontalCoefficientsChromaE, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoefType = VHSRC_FILTER_E;
            break;
        case VDP_FILTER_C_F:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoef, HorizontalCoefficientsChromaF, HFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaHorizontalCoefType = VHSRC_FILTER_F;
            break;

        default:
            TrError(("Error on display %d !Unknown type of HorizontalChromaFilter %d \n", DisplayInstanceIndex, LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentHorizontalChromaFilter));
            break;
    }


    /* Fill Luma Vertical Coef Filter */
    switch(LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalLumaFilter)
    {
        case VDP_FILTER_V_CHRX2:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoef, VerticalCoefficientsChromaZoomX2, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoefType = VHSRC_FILTER_CHRX2;
            break;
        case VDP_FILTER_A:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoef, VerticalCoefficientsA, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoefType = VHSRC_FILTER_A;
            break;
        case VDP_FILTER_B:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoef, VerticalCoefficientsB, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoefType = VHSRC_FILTER_B;
            break;
        case VDP_FILTER_Y_C:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoef, VerticalCoefficientsLumaC, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoefType = VHSRC_FILTER_C;
            break;
        case VDP_FILTER_Y_D:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoef, VerticalCoefficientsLumaD, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoefType = VHSRC_FILTER_D;
            break;
        case VDP_FILTER_Y_E:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoef, VerticalCoefficientsLumaE, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoefType = VHSRC_FILTER_E;
            break;
        case VDP_FILTER_Y_F:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoef, VerticalCoefficientsLumaF, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.LumaVerticalCoefType = VHSRC_FILTER_F;
            break;

        default:
            TrError(("Error on display %d !Unknown type of VerticalLumaFilter %d \n", DisplayInstanceIndex, LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalLumaFilter));
            break;
    }


    /* Fill Chroma Vertical Coef Filters */
    switch(LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalChromaFilter)
    {
        case VDP_FILTER_V_CHRX2:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoef, VerticalCoefficientsChromaZoomX2, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoefType = VHSRC_FILTER_CHRX2;
            break;
        case VDP_FILTER_A:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoef, VerticalCoefficientsA, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoefType = VHSRC_FILTER_A;
            break;
        case VDP_FILTER_B:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoef, VerticalCoefficientsB, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoefType = VHSRC_FILTER_B;
            break;
        case VDP_FILTER_C_C:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoef, VerticalCoefficientsChromaC, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoefType = VHSRC_FILTER_C;
            break;
        case VDP_FILTER_C_D:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoef, VerticalCoefficientsChromaD, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoefType = VHSRC_FILTER_D;
            break;
        case VDP_FILTER_C_E:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoef, VerticalCoefficientsChromaE, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoefType = VHSRC_FILTER_E;
            break;
        case VDP_FILTER_C_F:
            STOS_memcpy(Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoef, VerticalCoefficientsChromaF, VFC_NB_COEFS*4);
            Params_p->VHSRCVideoDisplayFilters.ChromaVerticalCoefType = VHSRC_FILTER_F;
            break;

        default:
            TrError(("Error on display %d !Error on display %d !Unknown type of VerticalChromaFilter %d \n", DisplayInstanceIndex, LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].CurrentVerticalChromaFilter));
            break;
    }

    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : GetRequestedDeinterlacingMode
Description : Function used to know the currently requested DEI Mode.
Parameters  :
Assumptions :
Limitations :
Returns     : - ST_ERROR_BAD_PARAMETER, if one parameter is wrong.
              - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t GetRequestedDeinterlacingMode (const STLAYER_ViewPortHandle_t ViewportHandle,
                                                     STLAYER_DeiMode_t * const      RequestedMode_p)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *) ViewportHandle;

    /* Check NULL parameters */
    if ( (ViewportProperties_p == NULL) || (RequestedMode_p == NULL) )
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);

    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    *RequestedMode_p = LayerCommonData_p->RequestedDeiMode;
    return ST_NO_ERROR;
}/* end of GetRequestedDeinterlacingMode() */

/*******************************************************************************
Name        : SetRequestedDeinterlacingMode
Description : Function used to request the use of a DEI Mode.
              NB: The Requested mode will be taken into account at the next VSync
Parameters  :
Assumptions :
Limitations : The requested mode will be applied only if the necessary conditions are met
Returns     : - ST_ERROR_BAD_PARAMETER, if one parameter is wrong.
              - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t SetRequestedDeinterlacingMode (const STLAYER_ViewPortHandle_t ViewportHandle,
                                                     const STLAYER_DeiMode_t        RequestedMode)
{
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *) ViewportHandle;

    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);

    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    if (RequestedMode < STLAYER_DEI_MODE_MAX)
    {
        LayerCommonData_p->RequestedDeiMode = RequestedMode;
        return ST_NO_ERROR;
    }
    else
    {
        /* Invalid mode requested */
        return ST_ERROR_BAD_PARAMETER;
    }

}/* end of SetRequestedDeinterlacingMode() */

/*******************************************************************************
Name        : DisableFMDReporting
Description : This function can be used to disable the FMD reporting.
Parameters  : ViewportHandle IN: Handle of the viewport
Assumptions :
Limitations :
Returns     : - ST_ERROR_BAD_PARAMETER, if one parameter is wrong.
              - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t DisableFMDReporting (const STLAYER_ViewPortHandle_t ViewportHandle)
{
#ifdef STLAYER_USE_FMD
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *) ViewportHandle;

    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);

    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p->FmdReportingEnabled = FALSE;
    vdp_fmd_EnableFmd (ViewportProperties_p->AssociatedLayerHandle, FALSE);

    return ST_NO_ERROR;
#else
    UNUSED_PARAMETER(ViewportHandle);
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
}

/*******************************************************************************
Name        : EnableFMDReporting
Description : This function can be used to enable the FMD reporting.
Parameters  : ViewportHandle IN: Handle of the viewport
Assumptions :
Limitations :
Returns     : - ST_ERROR_BAD_PARAMETER, if one parameter is wrong.
              - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t EnableFMDReporting (const STLAYER_ViewPortHandle_t ViewportHandle)
{
#ifdef STLAYER_USE_FMD
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *) ViewportHandle;

    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);

    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p->FmdReportingEnabled = TRUE;
    return ST_NO_ERROR;
#else
    UNUSED_PARAMETER(ViewportHandle);
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
}

/*******************************************************************************
Name        : GetFMDParams
Description : This function gets params used for FMD.
Parameters  : ViewportHandle IN: Handle of the viewport, Params_p OUT: Pointer
              to an allocated FMDParams data type
Assumptions :
Limitations :
Returns     : - ST_ERROR_BAD_PARAMETER, if one parameter is wrong.
              - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t GetFMDParams (const STLAYER_ViewPortHandle_t ViewportHandle,
                                    STLAYER_FMDParams_t * Params_p)
{
#ifdef STLAYER_USE_FMD
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *) ViewportHandle;

    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);

    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    vdp_fmd_GetFmdThreshold(ViewportProperties_p->AssociatedLayerHandle, Params_p);

    return ST_NO_ERROR;
#else
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(Params_p);
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

}


/*******************************************************************************
Name        : SetFMDParams
Description : This function set params used for FMD.
Parameters  : VPHandle IN: Handle of the viewport, Params_p IN: Pointer
              to an allocated FMDParams data type
Assumptions :
Limitations :
Returns     : - ST_ERROR_BAD_PARAMETER, if one parameter is wrong.
              - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t SetFMDParams (const STLAYER_ViewPortHandle_t ViewportHandle,
                                    STLAYER_FMDParams_t * Params_p)
{
#ifdef STLAYER_USE_FMD
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;

    /* Init Layer/Viewport properties */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *) ViewportHandle;

    /* Check NULL parameters */
    if (ViewportProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);

    /* Check NULL parameters */
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters */
    if (LayerCommonData_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    vdp_fmd_SetFmdThreshold(ViewportProperties_p->AssociatedLayerHandle, Params_p );

    return ST_NO_ERROR;
#else
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(Params_p);
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

}


/* private DEBUG functions -------------------------------------------------- */

#ifdef DBG_FILTER_COEFFS
/*******************************************************************************
Name        : DBG_CheckHorizontalFilterPointer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DBG_CheckHorizontalFilterPointer(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;
    U8                              DisplayInstanceIndex;
    volatile U32                    HorizontalFilterPointer;
    U32                             NewFirstU32OfHorizontalBuffer;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *) LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *) (LayerProperties_p->PrivateData_p);

    if (LayerProperties_p->LayerType == MAIN_DISPLAY)
    {
        DisplayInstanceIndex = 0;
    }
    else
    {
        DisplayInstanceIndex = 1;
    }

#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2(LAYER_PARTITION_AVMEM, &VirtualMapping );
#else
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif

    HorizontalFilterPointer = HAL_ReadDisp32(VDP_VHSRC_HCOEF_BA);

    if ( (HorizontalFilterPointer != (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressUsedByHw_p) &&
        (HorizontalFilterPointer != (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.HorizontalAddressAvailForWritting_p) )
    {
        TrError(("\r\nError on display %d ! Invalid HorizontalFilterPointer (0x%x)", DisplayInstanceIndex, HorizontalFilterPointer));
    }
    else
    {
        /* The pointer is valid: Check that the content didn't change */
        NewFirstU32OfHorizontalBuffer = ( (U32 *) STAVMEM_DeviceToCPU((U32 *)HorizontalFilterPointer, &VirtualMapping ))[HFC_NB_COEFS-1];

        if (NewFirstU32OfHorizontalBuffer != LayerCommonData_p->DbgLastU32OfHorizontalLumaBuffer)
        {
            TrError(("\r\nError on display %d ! Horizontal Buffer has changed! (0x%x 0x%x)",
                  DisplayInstanceIndex,
                  NewFirstU32OfHorizontalBuffer,
                  LayerCommonData_p->DbgLastU32OfHorizontalLumaBuffer));
        }
    }
}

/*******************************************************************************
Name        : DBG_CheckVerticalFilterPointer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DBG_CheckVerticalFilterPointer(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p;
    U8                              DisplayInstanceIndex;
    volatile U32                    VerticalFilterPointer;
    U32                             NewFirstU32OfVerticalBuffer;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    LayerProperties_p   = (HALLAYER_LayerProperties_t *) LayerHandle;
    LayerCommonData_p   = (HALLAYER_VDPLayerCommonData_t *) (LayerProperties_p->PrivateData_p);

    if (LayerProperties_p->LayerType == MAIN_DISPLAY)
    {
        DisplayInstanceIndex = 0;
    }
    else
    {
        DisplayInstanceIndex = 1;
    }

#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2(LAYER_PARTITION_AVMEM, &VirtualMapping );
#else
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif

    VerticalFilterPointer = HAL_ReadDisp32(VDP_VHSRC_VCOEF_BA);

    if ( (VerticalFilterPointer != (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressUsedByHw_p) &&
        (VerticalFilterPointer != (U32) LayerCommonData_p->DisplayInstance[DisplayInstanceIndex].FilterBuffer.VerticalAddressAvailForWritting_p) )
    {
        TrError(("\r\nError on display %d ! Invalid VerticalFilterPointer (0x%x)", DisplayInstanceIndex, VerticalFilterPointer));
    }
    else
    {
        /* The pointer is valid: Check that the content didn't change */
        NewFirstU32OfVerticalBuffer = ( (U32 *) STAVMEM_DeviceToCPU((U32 *)VerticalFilterPointer, &VirtualMapping ))[VFC_NB_COEFS-1];

        if (NewFirstU32OfVerticalBuffer != LayerCommonData_p->DbgLastU32OfVerticalLumaBuffer)
        {
            TrError(("\r\nError on display %d ! Vertical Buffer has changed! (0x%x 0x%x)",
                  DisplayInstanceIndex,
                  NewFirstU32OfVerticalBuffer,
                  LayerCommonData_p->DbgLastU32OfVerticalLumaBuffer));
        }
    }
}
#endif /* DBG_FILTER_COEFFS */


#ifdef VIDEO_HALLAYER_DEBUG
/*******************************************************************************
Name        : HALLAYER_Error
Description : Function called every time an error occurs. A breakpoint here
              should facilitate the debug
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void HALLAYER_Error(void)
{
    while (1);
}

#endif /* VIDEO_HALLAYER_DEBUG */

#if 0
/*******************************************************************************
Name        : DumpFilters
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DumpFilters(void)
{
    /* Horizontal filters */
    DumpFilterCoeffs("HorizontalCoefficientsA", (const S8 *) HorizontalCoefficientsA, HFC_NB_COEFS*4);
    DumpFilterCoeffs("HorizontalCoefficientsChromaZoomX2", (const S8 *) HorizontalCoefficientsChromaZoomX2, HFC_NB_COEFS*4);
    DumpFilterCoeffs("HorizontalCoefficientsB", (const S8 *) HorizontalCoefficientsB, HFC_NB_COEFS*4);

    /* Vertical filters */
    DumpFilterCoeffs("VerticalCoefficientsChromaZoomX2", (const S8 *) VerticalCoefficientsChromaZoomX2, VFC_NB_COEFS*4);
    DumpFilterCoeffs("VerticalCoefficientsA", (const S8 *) VerticalCoefficientsA, VFC_NB_COEFS*4);
    DumpFilterCoeffs("VerticalCoefficientsB", (const S8 *) VerticalCoefficientsB, VFC_NB_COEFS*4);

    DumpFilterCoeffs("VerticalCoefficientsAPatricia", (const S8 *) VerticalCoefficientsAPatricia, VFC_NB_COEFS*4);


}

/*******************************************************************************
Name        : DumpFilterCoeffs
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DumpFilterCoeffs(const char * coeffs_name, const S8 * coeff_p, U32 coeff_length)
{
    U32     i;

    TraceOn(("\r\n\r\nstatic const S8 %s[] = \r\n{", coeffs_name));

    for(i=0; i<coeff_length; i=i+4)
    {
        TraceOn(("\r\n%6d, %7d, %7d, %7d,",
                coeff_p[i],
                coeff_p[i+1],
                coeff_p[i+2],
                coeff_p[i+3] ));
    }

    TraceOn(("\r\n};"));
}
#endif

/* end of hv_vdplay.c */
