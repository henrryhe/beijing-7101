/*******************************************************************************

File name   : hv_layer.c

Description : Layer Video HAL (Hardware Abstraction Layer) for Omega2 chips access to hardware source file

Notes       : Because of harware limitations, the multi-opened viewports won't be managed for the main
              display windows. The result is that many viewport's parameters will be managed at layer
              levels.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
23 Feb 2000        Created                                          PLe
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Additional trace for debug. */
/* #define TRACE_HAL_VIDEO_LAYER */
#ifdef TRACE_HAL_VIDEO_LAYER
  extern void trace (const char *format,...);
# define TraceBuffer(x)  trace x
#endif

/*#define VIDEO_HALLAYER_DEBUG*/    /* Internal flag to check error cases.    */

/* Chip selection.                                                            */
#if defined ST_7015

# define GENERIC_MAIN_DISPLAY   STLAYER_OMEGA2_VIDEO1
# define GENERIC_AUX_DISPLAY    STLAYER_OMEGA2_VIDEO2

#elif defined ST_7020

# define GENERIC_MAIN_DISPLAY   STLAYER_7020_VIDEO1
# define GENERIC_AUX_DISPLAY    STLAYER_7020_VIDEO2

/* test the DVD_CFLAGS that could allow the use of cut 1.0  */
# ifdef DVD_VIDEO_LAYER_STI7020_CUT1
#  define IS_CHIP_STi7020_CUT_1
#  define WA_GNBvd13019  /* "HFC filtering gives wrong colors"                     */
# else  /* not DVD_VIDEO_LAYER_STI7020_CUT1 */
#  define IS_CHIP_STi7020_CUT_2
# endif /* not DVD_VIDEO_LAYER_STI7020_CUT1 */
#endif  /* ST_7020 */


/* Temporary: work-arounds 7020 */
#define WA_GNBvd13974 /* "First video line is always filled with green."        */
#ifdef  WA_GNBvd13974
# define NB_LINE_TO_DISSIMULATE 2
#endif /* WA_GNBvd13974 */

/* Temporary: work-arounds 7015 */
#define WA_GNBvd06059 /* available for 7015 chips first versions                */
#define WA_GNBvd06322 /* do not read VIDn_CTL !                                 */
#define WA_GNBvd08340 /* Bypass VFC if vertical format conversion is not needed */
#define WA_GNBvd06532 /* Wrong displayed field depending on parity of video     */
                      /* viewport position                                      */
#define WA_GNBvd12756 /* Auxilliary Display shows white or no chroma top field  */
#ifdef  WA_GNBvd12756
# define ADDITIONAL_LMU_LINE 34
#endif /* WA_GNBvd12756 */

#define WA_GNBvd09294 /* Standard Video Output Calibration (display positioning */
                      /* in output window) */
#ifdef  WA_GNBvd09294
# define BOTTOM_BLANKING_SIZE 3  /* lines */
#endif /* WA_GNBvd09294 */

#define WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD /* Work around to manage progressive HD streams         */
                                                   /* like interlaced streams (due to Hardware limits.).   */
#ifdef  WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
#define FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED 1080
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "stlayer.h"
#include "stgxobj.h"
#include "sttbx.h"

#include "halv_lay.h"
#include "hv_layer.h"
#include "filters.h"
#include "layer2.h"

/* Private Constants -------------------------------------------------------- */

/* Private Types ------------------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */
static BOOL IsMainLayerInitialized = FALSE;
static BOOL IsAuxLayerInitialized = FALSE;

/* Functions ---------------------------------------------------------------- */
static void SetViewportOutputRectangle (const HALLAYER_LayerProperties_t *    LayerProperties_p);

static U32 GetDecimationFactorFromDecimation (STLAYER_DecimationFactor_t Decimation);
static STLAYER_DecimationFactor_t GetDecimationFromDecimationFactor (U32 DecimationFactor);

static void UpdateHFCZones(HALLAYER_LayerCommonData_t * LayerCommonData_p,
                        U16 * const Zones,
                        S16 * const Deltas,
                        U16 * InitialStep_p);
static void ComputeAndApplyHfcVfc(HALLAYER_LayerCommonData_t *    LayerCommonData_p,
                        HALLAYER_ViewportProperties_t * ViewportProperties_p,
                        HALLAYER_LayerProperties_t * LayerProperties_p,
                        U32 PanVector,U32 ScanVector,
                        U32 SourceHorizontalSize, U32 SourceVerticalSize,
                        U32 InputHorizontalSize, U32  InputVerticalSize,
                        U32 OutputHorizontalSize, U32  OutputVerticalSize,
                        STGXOBJ_ScanType_t ScanType, STGXOBJ_ScanType_t SourceScanType);
static ST_ErrorCode_t AdjustViewportParams(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_ViewPortParams_t * const   Params_p);
static ST_ErrorCode_t SetColorSpaceConversionMode(
        const HALLAYER_LayerProperties_t * const LayerProperties_p,
        const STGXOBJ_ColorSpaceConversionMode_t ColorSpaceConversionMode);
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


static void EnableHorizontalFiltering(const STLAYER_Handle_t   LayerHandle);
static void DisableHorizontalFiltering(const STLAYER_Handle_t  LayerHandle);

static void EnableVerticalFiltering(const STLAYER_Handle_t     LayerHandle);
static void DisableVerticalFiltering(const STLAYER_Handle_t  LayerHandle,
                        BOOL DisableChroma, BOOL DisableLuma);
static void SetVFCCoeffs(
                         const STLAYER_Handle_t              LayerHandle,
                         const S16      *                    Coeffs);
static void SetVFCZones(
                         const STLAYER_Handle_t              LayerHandle,
                         U16 * const                         Zones,
                         S16 * const                         Deltas);
static void SetHFCCoeffs(
                         STLAYER_Handle_t LayerHandle,
                         const S16 *                          LumaCoeffs_p,
                         const S16 *                          ChromaCoeffs_p);
static void SetHFCZones(
                         STLAYER_Handle_t                    LayerHandle,
                         U16 * const                         Zones,
                         S16 * const                         Deltas);
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

/* private ERROR management functions --------------------------------------- */
#ifdef VIDEO_HALLAYER_DEBUG
static void HALLAYER_Error(void);
#else
static void HALLAYER_Error(void){}
#endif

/* private MAIN DISPLAY management functions -------------------------------- */
static void EnableLMU  (HALLAYER_LayerProperties_t * LayerProperties_p);
static void DisableLMU (HALLAYER_LayerProperties_t * LayerProperties_p);
/* private AUXILIARY DISPLAY management functions --------------------------- */

/* private MAIN/AUXILIARY DISPLAY management functions ---------------------- */
static void InitializeVideoProcessor (const STLAYER_Handle_t LayerHandle);

static void AdjustInputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p);
static void AdjustOutputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p);
static void ApplyViewPortParams(const STLAYER_ViewPortHandle_t ViewportHandle, STLAYER_ViewPortParams_t * const Params_p);
static void CenterNegativeValues(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p);

static ST_ErrorCode_t CheckForAvailableScaling (
        const STLAYER_Handle_t          LayerHandle,
        const STLAYER_ViewPortParams_t  *ViewPortParams_p,
        STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
        STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation,
        STLAYER_DecimationFactor_t      *RecommendedHorizontalDecimation_p,
        STLAYER_DecimationFactor_t      *RecommendedVerticalDecimation_p,
        U32                             *RecommendedHorizontalSize_p,
        U32                             *RecommendedVerticalSize_p);


/* Global Variables --------------------------------------------------------- */
const HALLAYER_FunctionsTable_t  HALLAYER_Omega2Functions =
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


#ifndef WA_GNBvd13019
/*******************************************************************************
Name        : EnableHorizontalFiltering
Description : Enables Horizontal Format Correction on luma and chroma signals
Parameters  : HAL layer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableHorizontalFiltering(const STLAYER_Handle_t   LayerHandle)
{

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_HFC_THRU, 0);

} /* End of EnableHorizontalFiltering() function */
#endif

/*******************************************************************************
Name        : DisableHorizontalFiltering
Description : Disables Horizontal Format Correction on luma and chroma signals
Parameters  : HAL layer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableHorizontalFiltering(const STLAYER_Handle_t   LayerHandle)
{

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_HFC_THRU, 1);

} /* End of DisableHorizontalFiltering() function */

/*******************************************************************************
Name        : EnableVerticalFiltering
Description : Enables Vertical Format Correction on luma and chroma signals
Parameters  : HAL layer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void EnableVerticalFiltering(const STLAYER_Handle_t   LayerHandle)
{

    HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_VFC_THRU, 0);

} /* End of EnableVerticalFiltering() function */

/*******************************************************************************
Name        : DisableVerticalFiltering
Description : Disables Vertical Format Correction on luma and/or chroma signals
Parameters  : HAL layer manager handle
              DisableChroma
              DisableLuma
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DisableVerticalFiltering(const STLAYER_Handle_t  LayerHandle,
                        BOOL DisableChroma, BOOL DisableLuma)
{
    U32 RegisterValue = 0;

    RegisterValue = ((DisableLuma   ? (1 << DIX_VFC_THRU_Y_EMP) : 0 ) |
                     (DisableChroma ? (1 << DIX_VFC_THRU_C_EMP) : 0 ));

    /* Disable only luma as chroma filtering has to be done.    */
    HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_VFC_THRU,
            (RegisterValue & DIX_VFC_THRU_MASK));

} /* End of DisableVerticalFiltering() function */


/*******************************************************************************
Name        : AdjustInputWindow
Description : Adjust the input window parameters according to hardware
              constaints and check for basic parameters coherence.
Parameters  :   IN   - HAL layer manager handle.
              IN/OUT - Init viewport params.
Assumptions : The decimation factor must be applied before calling this function.
              Otherwise they won't taken into account.
              The function CenterNegativeValues must be called before this one.
Limitations :
Returns     :
*******************************************************************************/
static void AdjustInputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    S32                             XOffset;
    S32                             YOffset;
    U32                             SourcePictureHorizontalSize;
    U32                             SourcePictureVerticalSize;
    STGXOBJ_ColorType_t             ColorType;
    STGXOBJ_ScanType_t              SourceScanType;

    /* Avoid null pointer for input parameter.          */
    if (Params_p == NULL)
    {
        return;
    }

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    ViewportProperties_p= (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
    /* Check null pointer                               */
    if (ViewportProperties_p == NULL)
    {
        return;
    }

    /* Retrieve the parameters of the source.   */
    SourcePictureHorizontalSize = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
    SourcePictureVerticalSize   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;

#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
    if (SourcePictureVerticalSize >= FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED)
    {
        SourceScanType          = STGXOBJ_INTERLACED_SCAN;
    }
    else
    {
        SourceScanType          = (((Params_p->Source_p)->Data).VideoStream_p)->ScanType;
    }
#else
    SourceScanType              = (((Params_p->Source_p)->Data).VideoStream_p)->ScanType;
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */

    ColorType                   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.ColorType;

    /* Tests to avoid any division by zero !!!  */
    if ( (Params_p->InputRectangle.Height != 0) && (Params_p->InputRectangle.Width != 0) )
    {
        /* XOffset and YOffset tests...                             */
        /* PS: the input rectangle position unit is 1/16 pixel !!!  */
        XOffset = Params_p->InputRectangle.PositionX >> 4;
        YOffset = Params_p->InputRectangle.PositionY >> 4;

        if ((U32)XOffset > SourcePictureHorizontalSize)
        {
            XOffset = SourcePictureHorizontalSize;
        }

        if ((U32)YOffset > SourcePictureVerticalSize)
        {
            YOffset = SourcePictureVerticalSize;
        }

        /* Insure to have the edge of the displayed video image falling into        */
        /* the display window.                                                      */
        if ((XOffset + Params_p->InputRectangle.Width) > SourcePictureHorizontalSize)
        {
            /* Adjust the width of output window, to keep a constant ratio. */
            Params_p->OutputRectangle.Width  = ( (SourcePictureHorizontalSize - XOffset) * Params_p->OutputRectangle.Width)
                    / Params_p->InputRectangle.Width;
            Params_p->InputRectangle.Width   = SourcePictureHorizontalSize - XOffset ;
        }

        if ((YOffset + Params_p->InputRectangle.Height) > SourcePictureVerticalSize)
        {
            /* Adjust the height of output window, to keep a constant ratio. */
            Params_p->OutputRectangle.Height  = ( (SourcePictureVerticalSize - YOffset) * Params_p->OutputRectangle.Height)
                    / Params_p->InputRectangle.Height;
            Params_p->InputRectangle.Height   = SourcePictureVerticalSize - YOffset ;
        }

        switch ( (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.ColorType)
        {

            case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422   :
            case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 :
                if (SourceScanType == STGXOBJ_INTERLACED_SCAN)
                {
                    /* The height of the displayed picture is managed thanks to DIS_VBC_VINP register.                      */
                    /* For 4:2:2 interlace, DIS_VBC_VINP must be a multiple of 2 and the first line is DIS_VBC_VINP/2 + 1.  */
                    Params_p->InputRectangle.PositionY &= ((U32)(0xFFFFFFE) << 4);
                    /* For 4:2:2 interlace, DIS_VFC_VINL must be a multiple of 2 lines.                                     */
                    Params_p->InputRectangle.Height    &= 0xFFFFFFFE;
                }

                /* For 4:2:2 progressive, DIS_VBC_VINP must be a multiple of 1 and the first line is DIS_VBC_VINP + 1       */
                /* For 4:2:2 progressive, DIS_VFC_VINL must be a multiple of 1 line (no restriction).                       */
                /* --> no restriction.                                                                                      */

                break;

            case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420 :
            case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420 :
            default :
                if (SourceScanType == STGXOBJ_INTERLACED_SCAN)
                {
                    /* The height of the displayed picture is managed thanks to DIS_VBC_VINP register.                      */
                    /* For 4:2:0 interlace, DIS_VBC_VINP must be a multiple of 4 and the first line is DIS_VBC_VINP/4 + 1.  */
                    Params_p->InputRectangle.PositionY &= ((U32)(0xFFFFFFC) << 4);
                    /* For 4:2:0 interlace, DIS_VFC_VINL must be a multiple of 4 lines.                                     */
                    Params_p->InputRectangle.Height    &= 0xFFFFFFFC;
                }
                else
                {
                    /* The height of the displayed picture is managed thanks to DIS_VBC_VINP register.                       */
                    /*  For 4:2:0 progressive, DIS_VBC_VINP must be a multiple of 2 and the first line is DIS_VBC_VINP/2 + 1.*/
                    Params_p->InputRectangle.PositionY &= ((U32)(0xFFFFFFE) << 4);
                    /* For 4:2:0 progressive, DIS_VFC_VINL must be a multiple of 2 line.                                     */
                    Params_p->InputRectangle.Height    &= 0xFFFFFFFE;
                }
                break;
        }
    }

} /* End of AdjustInputWindow() function. */


/*******************************************************************************
Name        : AdjustOutputWindow
Description : Adjust the output window parameters according to hardware
              constaints and check for basic parameters coherence.
Parameters  :   IN   - HAL layer manager handle.
              IN/OUT - Init viewport params.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AdjustOutputWindow(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p)
{
    U32                             NonVisibleWidth;
    U32                             NonVisibleHeight;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    /* Avoid null pointer for input parameter.          */
    if (Params_p == NULL)
    {
        return;
    }

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
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
                if (Params_p->OutputRectangle.Width != 0) /* HG: Quick patch to avoid division by 0 ! */
                {
                    Params_p->InputRectangle.Width     -= ((NonVisibleWidth * Params_p->InputRectangle.Width)
                            / (Params_p->OutputRectangle.Width));
                }
                Params_p->OutputRectangle.Width    -= NonVisibleWidth;
                /* Just be sure the input rectangle width is even.                  */
                Params_p->InputRectangle.Width     &= 0xFFFFFFFE;
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
                if (Params_p->OutputRectangle.Height != 0) /* HG: Quick patch to avoid division by 0 ! */
                {
                    Params_p->InputRectangle.Height    -= ((NonVisibleHeight * Params_p->InputRectangle.Height)
                            / Params_p->OutputRectangle.Height);
                }
                Params_p->OutputRectangle.Height   -= NonVisibleHeight;
            }
        }
        else
        {
            Params_p->OutputRectangle.Height = 0;
        }

        if (LayerProperties_p->LayerType == GENERIC_AUX_DISPLAY)
        {
            /* Special case for the auxiliary display processor (max width : 720 pixels).   */
            if (Params_p->OutputRectangle.Width > HALLAYER_MAX_AUXILIARY_DISPLAY_WIDTH)
            {
                Params_p->OutputRectangle.Width = HALLAYER_MAX_AUXILIARY_DISPLAY_WIDTH;

                Params_p->OutputRectangle.Height = ((Params_p->OutputRectangle.Width * Params_p->InputRectangle.Height)
                        / Params_p->InputRectangle.Width );
            }
        }

        /* At the end, adjust the output format (always even !!!) */
        Params_p->OutputRectangle.Width     &= 0xFFFFFFFE;
        Params_p->OutputRectangle.Height    &= 0xFFFFFFFE;
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
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;

    STLAYER_UpdateParams_t          UpdateParams;

    U32                             PanVector,            ScanVector;
    U32                             InputHorizontalSize,  InputVerticalSize;
    U32                             OutputHorizontalSize, OutputVerticalSize;
    U32                             OutputHorizontalStart,OutputVerticalStart;
    U32                             SourceHorizontalSize, SourceVerticalSize;
    STGXOBJ_ScanType_t              ScanType,             SourceScanType;
    STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation;
    STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation;
    STLAYER_DecimationFactor_t      RecommendedHorizontalDecimation;
    STLAYER_DecimationFactor_t      RecommendedVerticalDecimation;
    U32                             RecommendedOutputHorizontalSize;
    U32                             RecommendedOutputVerticalSize;
    U32                             CurrentPictureHorizontalDecimationFactor;
    U32                             CurrentPictureVerticalDecimationFactor;

    ST_ErrorCode_t                  ErrorCode;

    /* Avoid null pointer for input parameter.          */
    if (Params_p == NULL)
    {
        return;
    }

    /* init Layer and viewport properties.  */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;
    /* Check null pointer                               */
    if (ViewportProperties_p == NULL)
    {
        return;
    }

    LayerProperties_p    = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p    = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Set local variables CurrentPictureHorizontalDecimation and CurrentPictureVerticalDecimation. */
    CurrentPictureHorizontalDecimation = (((Params_p->Source_p)->Data).VideoStream_p)->HorizontalDecimationFactor ;
    CurrentPictureVerticalDecimation = (((Params_p->Source_p)->Data).VideoStream_p)->VerticalDecimationFactor ;

#ifdef TRACE_HAL_VIDEO_LAYER
    TraceBuffer(("\r\n\r\n ApplyViewPortParams on %s: (%d,%d,%d,%d) to (%d,%d,%d,%d)",
            ((LayerProperties_p->LayerType == GENERIC_MAIN_DISPLAY)?"MAIN":"AUX"),
            LayerCommonData_p->ViewPortParams.InputRectangle.PositionX,
            LayerCommonData_p->ViewPortParams.InputRectangle.PositionY,
            LayerCommonData_p->ViewPortParams.InputRectangle.Width,
            LayerCommonData_p->ViewPortParams.InputRectangle.Height,
            LayerCommonData_p->ViewPortParams.OutputRectangle.PositionX,
            LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY,
            LayerCommonData_p->ViewPortParams.OutputRectangle.Width,
            LayerCommonData_p->ViewPortParams.OutputRectangle.Height ));
#endif

    /* Check for viewport parameters. If correct, then apply this viewport. */
    /* Otherwise, it's an error case. Disable the current viewport.         */
    if ( (LayerCommonData_p->ViewPortParams.InputRectangle.Width  == 0) ||
         (LayerCommonData_p->ViewPortParams.InputRectangle.Height == 0) ||
         (LayerCommonData_p->ViewPortParams.OutputRectangle.Width == 0) ||
         (LayerCommonData_p->ViewPortParams.OutputRectangle.Height== 0))
    {
        /* Special case to display nothing !!!! */

        /* We must apply an invisible area parameters. */
        /* VIn_VPO : Viewport Origin.   */
        HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_VPO, 0);
        /* VIn_VPS : Viewport Stop.     */
        HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_VPS, 0);
    }
    else
    {
        ScanType        = LayerCommonData_p->LayerParams.ScanType;

#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
        if ((((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height >= FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED)
        {
            SourceScanType  = STGXOBJ_INTERLACED_SCAN;
        }
        else
        {
            SourceScanType  = (((Params_p->Source_p)->Data).VideoStream_p)->ScanType;
        }
#else
        SourceScanType  = (((Params_p->Source_p)->Data).VideoStream_p)->ScanType;
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */

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
                STLAYER_DECIMATION_FACTOR_NONE,     STLAYER_DECIMATION_FACTOR_NONE,
                &RecommendedHorizontalDecimation,   &RecommendedVerticalDecimation,
                &RecommendedOutputHorizontalSize,   &RecommendedOutputVerticalSize);

        if (ErrorCode == ST_NO_ERROR)
        {
            /* ST_NO_ERROR means that the scaling is possible without decimation. */
            /* Force video display to present non-decimated picture.              */
            /* This change need also the re-calculation of all.                   */
            /* Working without decimation is not possible. Try with the current decimation if existing. */
            if ((CurrentPictureHorizontalDecimation != STLAYER_DECIMATION_FACTOR_NONE) ||
                (CurrentPictureVerticalDecimation != STLAYER_DECIMATION_FACTOR_NONE))
            {
                UpdateParams.UpdateReason       = STLAYER_DECIMATION_NEED_REASON;
                UpdateParams.IsDecimationNeeded = FALSE;
                UpdateParams.RecommendedHorizontalDecimation = RecommendedHorizontalDecimation;
                UpdateParams.RecommendedVerticalDecimation = RecommendedVerticalDecimation;
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
        } /* end of 1st check is NO_ERROR */
        else if (ErrorCode == ST_ERROR_FEATURE_NOT_SUPPORTED)
        {
            /* Working without decimation is not possible. Try with the current decimation if existing. */
            if ((CurrentPictureHorizontalDecimation != STLAYER_DECIMATION_FACTOR_NONE) ||
                (CurrentPictureVerticalDecimation != STLAYER_DECIMATION_FACTOR_NONE))
            {
                /* A current decimation exists : try it. */
                ErrorCode = CheckForAvailableScaling (ViewportProperties_p->AssociatedLayerHandle, Params_p,
                        CurrentPictureHorizontalDecimation,   CurrentPictureVerticalDecimation,
                        &RecommendedHorizontalDecimation,     &RecommendedVerticalDecimation,
                        &RecommendedOutputHorizontalSize,     &RecommendedOutputVerticalSize);

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Adjust the Output according to the new hardware constraints. */
                    AdjustOutputWindow(ViewportProperties_p->AssociatedLayerHandle, Params_p);

                    /* Store the Viewport params applied.                               */
                    LayerCommonData_p->ViewPortParams = *Params_p;
                } /* end of 1st check is NO_ERROR */
            }
            /* else : there is no current decimation. */

            if (ErrorCode == ST_ERROR_FEATURE_NOT_SUPPORTED)
            {
                /* Use the Recommended parameters and send an event to ask the decimation. */
#ifdef TRACE_HAL_VIDEO_LAYER
                TraceBuffer(("\r\n !!! Impossible with HDec %d and VDec %d",
                        (((Params_p->Source_p)->Data).VideoStream_p)->HorizontalDecimationFactor,
                        (((Params_p->Source_p)->Data).VideoStream_p)->VerticalDecimationFactor ));
                TraceBuffer(("\r\n --->Recommended HDec %d and VDec %d ",
                        RecommendedHorizontalDecimation,
                        RecommendedVerticalDecimation ));
                TraceBuffer(("\r\n --->Recommended OutHSize %d and OutVSize %d ",
                        RecommendedOutputHorizontalSize,
                        RecommendedOutputVerticalSize ));
#endif

                /* Force video display to present decimated picture if possible.    */
                /* This change need also the re-calculation of all.                 */
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

                /* Store the Viewport params applied.                               */
                LayerCommonData_p->ViewPortParams = *Params_p;
            }
        } /* end of 1st check is FEATURE_NOT_SUPPORTED */

        /* Here is the apply part : */
        if (ErrorCode != ST_ERROR_BAD_PARAMETER)
        {
            /* Ok for the scaling process...    */

            PanVector   = LayerCommonData_p->ViewPortParams.InputRectangle.PositionX / 16;
            ScanVector  = LayerCommonData_p->ViewPortParams.InputRectangle.PositionY / 16;

            InputHorizontalSize = LayerCommonData_p->ViewPortParams.InputRectangle.Width;
            InputVerticalSize   = LayerCommonData_p->ViewPortParams.InputRectangle.Height;

            OutputHorizontalSize = LayerCommonData_p->ViewPortParams.OutputRectangle.Width;
            OutputVerticalSize   = LayerCommonData_p->ViewPortParams.OutputRectangle.Height;

            SourceHorizontalSize = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
            SourceVerticalSize   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;

            OutputHorizontalStart = LayerCommonData_p->ViewPortParams.OutputRectangle.PositionX
                                    + LayerCommonData_p->LayerPosition.XStart
                                    + LayerCommonData_p->LayerPosition.XOffset;
            OutputVerticalStart  = LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY
                                    + LayerCommonData_p->LayerPosition.YStart
                                    + LayerCommonData_p->LayerPosition.YOffset;

#ifdef WA_GNBvd06532
            if (OutputVerticalStart & 0x00000001)
            {
                /* Odd value for vertical viewport position. */

                /* Apply the filter to the Viewport vertical Offset. */
                LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY += 1;
                OutputVerticalStart += 1;
            }
#endif

            /* Apply decimation factors.     */
            CurrentPictureHorizontalDecimationFactor = GetDecimationFactorFromDecimation(CurrentPictureHorizontalDecimation);
            CurrentPictureVerticalDecimationFactor = GetDecimationFactorFromDecimation(CurrentPictureVerticalDecimation);
            PanVector  /= CurrentPictureHorizontalDecimationFactor;
            ScanVector /= CurrentPictureVerticalDecimationFactor;

            InputHorizontalSize /= CurrentPictureHorizontalDecimationFactor;
            InputVerticalSize   /= CurrentPictureVerticalDecimationFactor;

            SourceHorizontalSize /= CurrentPictureHorizontalDecimationFactor;
            SourceVerticalSize   /= CurrentPictureVerticalDecimationFactor;

            /* Check for some parameters...  */
            if (InputHorizontalSize > (SourceHorizontalSize & 0xfffffff0))
            {
                InputHorizontalSize &= 0xfffffff0;
            }
            /* Then store "real" input and output width.    */
            LayerCommonData_p->RealInputWidth = InputHorizontalSize;
            LayerCommonData_p->RealOutputWidth = OutputHorizontalSize;

            ComputeAndApplyHfcVfc(LayerCommonData_p,ViewportProperties_p,
                    LayerProperties_p, PanVector,ScanVector,
                    SourceHorizontalSize, SourceVerticalSize,
                    InputHorizontalSize,  InputVerticalSize ,
                    OutputHorizontalSize,   OutputVerticalSize,
                    ScanType,  SourceScanType);

            /*- Pan and Scan Vectors -------------------------------------------------*/
            /* the value to write in VINP depends in the ScanType. if progressive : multiple of 2. if interlaced multiple of 4 */
            if (ScanType == STGXOBJ_PROGRESSIVE_SCAN)
            {
                /* Scan Vector  */
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_VBC_VINP,
                        ((U32)ScanVector ) & 0xFFFE & DIS_VBC_VINP_MASK);
            }
            else
            {
                /* Scan Vector  */
#ifdef WA_GNBvd13974
            if ( ((LayerCommonData_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)||
                 (LayerCommonData_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)) &&
                 (SourceScanType == STGXOBJ_INTERLACED_SCAN) && (SourceHorizontalSize <= 720) )
            {
                /* Source is SDin, so apply the work-around : move down 2 lines the input   */
                /* window so that they are dissimulated.                                    */
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_VBC_VINP,
                        ((U32)(ScanVector + NB_LINE_TO_DISSIMULATE) ) & 0xFFFC & DIS_VBC_VINP_MASK);
            }
            else
            {
                /* Normal case. Apply common scanvector parameter.  */
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_VBC_VINP,
                        ((U32)ScanVector ) & 0xFFFC & DIS_VBC_VINP_MASK);
            }
#else
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_VBC_VINP,
                        ((U32)ScanVector ) & 0xFFFC & DIS_VBC_VINP_MASK);

#endif /* WA_GNBvd13974 */
            }

            /* Save the viewport output window dimensions.                       */
            /* VIn_VPO : Viewport Origin.   */
            LayerCommonData_p->ViewportOffset = ((OutputVerticalStart << 16) | OutputHorizontalStart);

            /* VIn_VPS : Viewport Stop.     */
            /* Check if the OutputVerticalSize and OutputHorizontalSize are available.  */
            if ( ( OutputHorizontalSize == 0) || ( OutputVerticalSize == 0))
            {
                /* VIn_VPS = VPO */
                LayerCommonData_p->ViewportStop = LayerCommonData_p->ViewportOffset;
            }
            else
            {
                LayerCommonData_p->ViewportStop = ((OutputVerticalStart + OutputVerticalSize -1 ) << 16) |
                                                   (OutputHorizontalStart + OutputHorizontalSize - 1);
            }

            /* Then, apply these dimensions. */
            SetViewportOutputRectangle (LayerProperties_p);

            SetColorSpaceConversionMode(LayerProperties_p,
                    ((Params_p->Source_p)->Data.VideoStream_p)->BitmapParams.ColorSpaceConversion);

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
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    S32 InputPositionX, OutputPositionX ;
    U32 InputWidth,     OutputWidth ;
    S32 InputPositionY, OutputPositionY ;
    U32 InputHeight,    OutputHeight ;

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

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
        Params_p->InputRectangle.PositionX   <<= 4;
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
        Params_p->InputRectangle.PositionY  <<= 4;
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
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    STLAYER_LayerParams_t         * LayerParams_p;
    U8                              ViewportId;
    U8                              cpt;
    STLAYER_TermParams_t            TermParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    STAVMEM_AllocBlockParams_t      AvmemAllocParams;
    STAVMEM_FreeBlockParams_t       AvmemFreeParams;
    STAVMEM_BlockHandle_t           LmuChromaBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    STAVMEM_BlockHandle_t           LmuLumaBufferHandle   = STAVMEM_INVALID_BLOCK_HANDLE;
    void                          * LmuBufferAddress;
    void                          * LmuBufferVirtualAddress;

#ifdef TRACE_HAL_VIDEO_LAYER
    TraceBuffer(("\r\n Init of Video layer"));
#endif

    /* init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Avoid null pointer for input parameter.          */
    if ( (LayerInitParams_p == NULL) || (LayerInitParams_p->LayerParams_p == NULL) || (LayerProperties_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* memory allocation for common layer datas structure */
    LayerProperties_p->PrivateData_p = (void *)STOS_MemoryAllocate(LayerProperties_p->CPUPartition_p, sizeof(HALLAYER_LayerCommonData_t));
    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check if the memory allocation is successful.    */
    if (LayerCommonData_p == NULL)
    {
        return (ST_ERROR_NO_MEMORY);
    }

    /* Get the layer parameters from init. parameters.  */
    LayerParams_p = LayerInitParams_p->LayerParams_p;

    switch (LayerProperties_p->LayerType)
    {
        case GENERIC_MAIN_DISPLAY :
            /* Check if the layer is not already initialised. */
            if (IsMainLayerInitialized)
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

                    /* Get AV Mem mapping               */
                    STAVMEM_GetSharedMemoryVirtualMapping(&LayerCommonData_p->VirtualMapping);

                    /* Correction of DDTS GNBvd13029                                            */
                    /* Allocate shared memory for LMU usage. TWO buffers have to be allocated   */
                    /* Maximum size allowed will be allocated with L=720 and H=576.             */
                    /*                                                                          */
                    /*                              case 4:2:0               case 4:2:2         */
                    /*    Luma size needed  :         2.(L.H)                  2.(L.H)          */
                    /*    Chroma size needed:       2.1/2(L.H)                 2.(L.H)          */
                    /*                            -------------             -------------       */
                    /*       Total size:              3.(L.H)                  4.(L.H)          */
                    /****************************************************************************/
                    /* Conclusion: As at this step we know don't the type of input, we must     */
                    /* allocate the greatest possible buffers.                                  */
                    /****************************************************************************/

                    AvmemAllocParams.Alignment       = 256;
                    AvmemAllocParams.PartitionHandle = LayerInitParams_p->AVMEM_Partition;
#ifdef WA_GNBvd12756
                    AvmemAllocParams.Size            = 4 * HALLAYER_LMU_MAX_WIDTH * (HALLAYER_LMU_MAX_HEIGHT/2 + ADDITIONAL_LMU_LINE);
#else
                    AvmemAllocParams.Size            = 2 * (HALLAYER_LMU_MAX_WIDTH * HALLAYER_LMU_MAX_HEIGHT);
#endif
                    AvmemAllocParams.AllocMode       = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
                    AvmemAllocParams.NumberOfForbiddenRanges    = 0;
                    AvmemAllocParams.ForbiddenRangeArray_p      = NULL;
                    AvmemAllocParams.NumberOfForbiddenBorders   = 0;
                    AvmemAllocParams.ForbiddenBorderArray_p     = NULL;

                    /* ********************* Allocate LMU Chroma buffer ********************* */
                    ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &LmuChromaBufferHandle);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        /* Error handling */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate lmu buffer !"));
                        /* De-allocate common layer Properties */
                        STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerProperties_p->PrivateData_p);
                        ErrorCode = ST_ERROR_NO_MEMORY;
                    }
                    else
                    {
                        ErrorCode = STAVMEM_GetBlockAddress(LmuChromaBufferHandle, &LmuBufferVirtualAddress);
                        LmuBufferAddress = STAVMEM_VirtualToDevice(LmuBufferVirtualAddress,
                            &LayerCommonData_p->VirtualMapping);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* Error handling */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of lmu buffer !"));
                            /* Try to free block before leaving */
                            AvmemFreeParams.PartitionHandle = LayerInitParams_p->AVMEM_Partition;
                            STAVMEM_FreeBlock(&AvmemFreeParams, &LmuChromaBufferHandle);
                            /* De-allocate common layer Properties */
                            STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerProperties_p->PrivateData_p);
                            ErrorCode = ST_ERROR_NO_MEMORY;
                        }
                        else
                        {
                            LayerCommonData_p->LmuChromaBuffer_p    = LmuBufferAddress;
                            LayerCommonData_p->LmuChromaBlockHandle = LmuChromaBufferHandle;
                        }
                    }

                    /* ********************* Allocate LMU Luma buffer ********************* */
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &LmuLumaBufferHandle);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* Error handling */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Cannot allocate lmu buffer !"));
                            /* Try to free block before leaving */
                            AvmemFreeParams.PartitionHandle = LayerInitParams_p->AVMEM_Partition;
                            STAVMEM_FreeBlock(&AvmemFreeParams, &LmuChromaBufferHandle);
                            /* De-allocate common layer Properties */
                            STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerProperties_p->PrivateData_p);
                            ErrorCode = ST_ERROR_NO_MEMORY;
                        }
                        else
                        {
                            ErrorCode = STAVMEM_GetBlockAddress(LmuLumaBufferHandle, &LmuBufferVirtualAddress);
                            LmuBufferAddress = STAVMEM_VirtualToDevice(LmuBufferVirtualAddress,
                                &LayerCommonData_p->VirtualMapping);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* Error handling */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of lmu buffer !"));
                                /* Try to free block before leaving */
                                AvmemFreeParams.PartitionHandle = LayerInitParams_p->AVMEM_Partition;
                                STAVMEM_FreeBlock(&AvmemFreeParams, &LmuChromaBufferHandle);
                                STAVMEM_FreeBlock(&AvmemFreeParams, &LmuLumaBufferHandle);
                                /* De-allocate common layer Properties */
                                STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerProperties_p->PrivateData_p);
                                ErrorCode = ST_ERROR_NO_MEMORY;
                            }
                            else
                            {
                                /* Allocation is Ok. Returns param of lmu buffers.  */
                                LayerCommonData_p->AvmemPartitionHandle = LayerInitParams_p->AVMEM_Partition;
                                LayerCommonData_p->LmuLumaBlockHandle   = LmuLumaBufferHandle;
                                LayerCommonData_p->LmuLumaBuffer_p      = LmuBufferAddress;

                                /* Update flag to avoid initialising the same layer */
                                IsMainLayerInitialized = TRUE;
                            }
                        }
                    }
                }
            } /* end of not IsMainLayerInitialized  */
            break;
        case GENERIC_AUX_DISPLAY   :
            /* Check if the layer is not already initialised. */
            if (IsAuxLayerInitialized)
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
                    IsAuxLayerInitialized = TRUE;

                    LayerCommonData_p->LmuChromaBlockHandle = (U32) (NULL);
                    LayerCommonData_p->LmuLumaBlockHandle   = (U32) (NULL);
                    LayerCommonData_p->AvmemPartitionHandle = (U32) (NULL);
                    LayerCommonData_p->VirtualMapping.PhysicalAddressSeenFromCPU_p     = NULL;
                    LayerCommonData_p->VirtualMapping.PhysicalAddressSeenFromDevice_p  = NULL;
                    LayerCommonData_p->VirtualMapping.PhysicalAddressSeenFromDevice2_p = NULL;
                    LayerCommonData_p->VirtualMapping.VirtualBaseAddress_p             = NULL;
                    LayerCommonData_p->VirtualMapping.VirtualSize                      = 0;
                    LayerCommonData_p->VirtualMapping.VirtualWindowOffset              = 0;
                    LayerCommonData_p->VirtualMapping.VirtualWindowSize                = 0;
                    LayerCommonData_p->LmuLumaBuffer_p      = NULL;
                    LayerCommonData_p->LmuChromaBuffer_p    = NULL;
                }
            }
            break;
        default :
            HALLAYER_Error();
            break;
    }
    /* Check for previous setting result.    */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Store the layer dimensions.      */
        LayerCommonData_p->LayerParams = *LayerParams_p;

        LayerCommonData_p->VTGFrameRate          = 0;
        LayerCommonData_p->EvtHandle             = (U32) (NULL);

        LayerCommonData_p->LayerPosition.XStart  = 0;
        LayerCommonData_p->LayerPosition.YStart  = 0;
        LayerCommonData_p->LayerPosition.XOffset = 0;
        LayerCommonData_p->LayerPosition.YOffset = 0;

        /* Initialization of all allowed viewports. */
        ViewportProperties_p = (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
        for (ViewportId = 0; ViewportId < ((HALLAYER_LayerCommonData_t *)(LayerCommonData_p))->MaxViewportNumber; ViewportId++)
        {
            ViewportProperties_p[ViewportId].AssociatedLayerHandle  = (U32) (NULL);
            ViewportProperties_p[ViewportId].Opened                 = FALSE;
            ViewportProperties_p[ViewportId].ViewportId             = ViewportId;
            ViewportProperties_p[ViewportId].SourceNumber           = (U32)(-1);
        }

        LayerCommonData_p->ViewPortParams.Source_p                  = NULL;
        LayerCommonData_p->ViewPortParams.InputRectangle.PositionX  = 0;
        LayerCommonData_p->ViewPortParams.InputRectangle.PositionY  = 0;
        LayerCommonData_p->ViewPortParams.InputRectangle.Width      = 0;
        LayerCommonData_p->ViewPortParams.InputRectangle.Height     = 0;

        LayerCommonData_p->ViewPortParams.OutputRectangle.PositionX = 0;
        LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY = 0;
        LayerCommonData_p->ViewPortParams.OutputRectangle.Width     = LayerParams_p->Width;
        LayerCommonData_p->ViewPortParams.OutputRectangle.Height    = LayerParams_p->Height;

        LayerCommonData_p->SourcePictureDimensions.Width            = 0;
        LayerCommonData_p->SourcePictureDimensions.Height           = 0;

        LayerCommonData_p->RealInputWidth                           = 0;
        LayerCommonData_p->RealOutputWidth                          = 0;

        LayerCommonData_p->ColorType                                = (STGXOBJ_ColorType_t) (-1);

        LayerCommonData_p->IsViewportEnable                         = FALSE;
        LayerCommonData_p->ViewportOffset                           = 0;
        LayerCommonData_p->ViewportStop                             = 0;

        LayerCommonData_p->HorizontalFormatFilter                   = HALLAYER_NONE_FILTER;
        LayerCommonData_p->VerticalFormatFilter                     = HALLAYER_NONE_FILTER;

        LayerCommonData_p->RegToUpdate                              = 0;
        LayerCommonData_p->IsUpdateNeeded                           = FALSE;
        LayerCommonData_p->CurrentGlobalAlpha                       = VIn_ALP_DEFAULT;

        LayerCommonData_p->OutputMode                               = STLAYER_NORMAL_MODE;

        LayerCommonData_p->OutputModeParams.SpectacleModeParams.NumberOfZone           = 0;
        LayerCommonData_p->OutputModeParams.SpectacleModeParams.EffectControl          = 0;
        LayerCommonData_p->OutputModeParams.SpectacleModeParams.SizeOfZone_p           =
                LayerCommonData_p->SizeOfZone;

        for (cpt=0; cpt < HFC_ZONE_NBR; cpt++)
        {
            LayerCommonData_p->SizeOfZone[cpt] = 0;
        }
        LayerCommonData_p->MultiAccessSem_p = = STOS_SemaphoreCreateFifo(NULL,1);


    }
    /* Check for previous setting result.    */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = InitFilterEvt (LayerHandle, LayerInitParams_p);
        ErrorCode = InitFilter (LayerHandle);

        if (ErrorCode != ST_NO_ERROR)
        {
            TermParams.ForceTerminate = TRUE;
            Term(LayerHandle, &TermParams);
        }
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

    /* Check if any input parameters is unavailable.                    */
    if ((LayerProperties_p != NULL) && (Capability_p != NULL))
    {
        Capability_p->MultipleViewPorts  = FALSE;
        Capability_p->HorizontalResizing = TRUE;
        Capability_p->AlphaBorder = FALSE;
        Capability_p->GlobalAlpha = TRUE;
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
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    STGXOBJ_Rectangle_t             InputRectangle, OutputRectangle;
    U32                             SourcePictureHorizontalSize, SourcePictureVerticalSize, PanVector, ScanVector;
    U32                             InputHorizontalSize, InputVerticalSize;
    U32                             OutputPositionX, OutputPositionY, OutputHorizontalSize, OutputVerticalSize;
    STLAYER_DecimationFactor_t      HorizontalDecimationFactor, VerticalDecimationFactor;
    STGXOBJ_ColorType_t             ColorType;
    U8                              ViewportId = 0;

    /* Check if any input parameters is unavailable.                    */
    if ((ViewportHandle_p == NULL) || (Params_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* init Layer and viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters.                                           */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters.                                           */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ViewportProperties_p= (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
    /* Check NULL parameters.                                           */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve the first free viewport.     */
    while ((ViewportProperties_p[ViewportId].Opened == TRUE) & (ViewportId < LayerCommonData_p->MaxViewportNumber))
    {
        ViewportId ++;
    }
    /* Test a new viewport can be opened.   */
    if (ViewportId >= LayerCommonData_p->MaxViewportNumber)
    {
        /* No mmore free handle found.      */
        return (STLAYER_ERROR_NO_FREE_HANDLES);
    }

    /*- Input Window Verifications -------------------------------------------*/
    /*------------------------------------------------------------------------*/
    InputRectangle  = Params_p->InputRectangle;
    OutputRectangle = Params_p->OutputRectangle;
    PanVector       = InputRectangle.PositionX >> 4;
    ScanVector      = InputRectangle.PositionY >> 4;

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

    /* Test if outpout Window Position out of Layer Dimensions, or if risk of division by 0 */
    if ((OutputPositionX > LayerCommonData_p->LayerParams.Width) || (OutputPositionY > LayerCommonData_p->LayerParams.Height) ||
         (InputHorizontalSize == 0) || (InputVerticalSize == 0))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /*- Scaling Capabilities Verifications -------------------------------------*/
    HorizontalDecimationFactor  = (((Params_p->Source_p)->Data).VideoStream_p)->HorizontalDecimationFactor;
    VerticalDecimationFactor    = (((Params_p->Source_p)->Data).VideoStream_p)->VerticalDecimationFactor;

    /* --------- Viewport initialisations --------- */
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
/*    LayerCommonData_p->HorizontalDecimationFactor     = HorizontalDecimationFactor;*/
/*    LayerCommonData_p->VerticalDecimationFactor       = VerticalDecimationFactor;*/
    LayerCommonData_p->ColorType                      = ColorType;

    /* Initialize the main registers to work !      */
    InitializeVideoProcessor (LayerHandle);
    /* Sets the source of the viewport.             */
    SetViewportSource(*ViewportHandle_p, Params_p->Source_p);

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
    STLAYER_Handle_t                LayerHandle;
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;


    /* init viewport properties.    */
    ViewportProperties = (HALLAYER_ViewportProperties_t *)(ViewportHandle);
    LayerHandle = ((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle;
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   =
            (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters.                                           */
    if (ViewportProperties == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    InitFilter(LayerHandle);
    SetColorSpaceConversionMode(LayerProperties_p,
                LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->BitmapParams.ColorSpaceConversion);
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
                - ST_ERROR_OPEN_HANDLE, if one viewport is still opened.
                - ST_ERROR_BAD_PARAMETER if null pointer.
                - ST_ERROR_UNKNOWN_DEVICE if layer not initialized.
                - ST_NO_ERROR, if no error occurs.
*******************************************************************************/
static ST_ErrorCode_t Term(const STLAYER_Handle_t       LayerHandle,
                    const STLAYER_TermParams_t*         TermParams_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    STAVMEM_FreeBlockParams_t       AvmemFreeParams;
    STLAYER_Layer_t                 LayerType;
    U8                              ViewPortId;

    /* Check if any input parameters is unavailable.                    */
    if (TermParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* init Layer and viewport properties.  */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters.                                           */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters.                                           */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)(LayerCommonData_p->ViewportProperties_p);
    /* Check NULL parameters.                                           */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerType = LayerProperties_p->LayerType;

    /* Check if layer is weel initialized. */
    if ( ((LayerType == GENERIC_MAIN_DISPLAY) && (!IsMainLayerInitialized)) ||
         ((LayerType == GENERIC_AUX_DISPLAY ) && (!IsAuxLayerInitialized)) )
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
         if (TermParams_p->ForceTerminate == TRUE)
         {
             CloseViewport((STLAYER_ViewPortHandle_t)(&ViewportProperties_p[ViewPortId]));
         }
         /* else verify that all the viewports are closed */
         else
         {
              if (ViewportProperties_p[ViewPortId].Opened == TRUE)
              {
                  return (ST_ERROR_OPEN_HANDLE);
              }
         }
    }

    STOS_SemaphoreDelete(NULL,NULL,LayerCommonData_p->MultiAccessSem_p);

    /* De-allocate LMU buffer in shared memory  */
    if (LayerCommonData_p->AvmemPartitionHandle != (U32) (NULL))
    {
        AvmemFreeParams.PartitionHandle = LayerCommonData_p->AvmemPartitionHandle;
        STAVMEM_FreeBlock(&AvmemFreeParams, &(LayerCommonData_p->LmuLumaBlockHandle));
        STAVMEM_FreeBlock(&AvmemFreeParams, &(LayerCommonData_p->LmuChromaBlockHandle));
    }

    /* De-allocate Viewport Properties. */
    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p,LayerCommonData_p->ViewportProperties_p);

    /* De-allocate last: data inside cannot be used after ! */
    STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, LayerCommonData_p);

    /* Update flag to allow a new init for the layer type. */
    switch (LayerType)
    {
        case GENERIC_MAIN_DISPLAY :
            IsMainLayerInitialized = FALSE;
            break;

        case GENERIC_AUX_DISPLAY   :
            IsAuxLayerInitialized = FALSE;
            break;

        default :
            break;
    }


    /* No error to return.              */
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
   HALLAYER_LayerCommonData_t * LayerCommonData_p;
   U8 ViewportId;

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
   LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    switch  (LayerProperties_p->LayerType)
    {
        /* Main and Auxiliary display processor.  */
        case GENERIC_MAIN_DISPLAY   :

            /* Get the viewport ID. */
            ViewportId = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->ViewportId;
            HAL_SetRegister32Value((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_XCFG1 + ViewportId * DIS_XCFG_STEP,
                                DIS_XCFG_MASK,
                                DIS_XCFG_ENA_DISA_LAYER_MASK,
                                DIS_XCFG_ENA_DISA_LAYER_EMP,
                                HALLAYER_ENABLE);

            /* Test now if the LMU was activated before the enable of the viewport. */
            if (HAL_Read32(((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_LMU_CTRL)) != 0)
            {
                /* Yes it was. Disable the viewport during a VSync's time thanks to     */
                /* Global Alpha in order to dissimulate a possible pending picture in   */
                /* the display pipe.                                                    */
                HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_ALP,
                        VIn_ALP_FULL_TRANSPARANCY);
                /* Next VSync, the current global alpha setting will be restored.       */
                LayerCommonData_p->RegToUpdate      |= GLOBAL_ALPHA;
                LayerCommonData_p->IsUpdateNeeded   = TRUE;
            }

        /* Auxiliary display processor.  */
        case GENERIC_AUX_DISPLAY   :

            /* Save this state.     */
            LayerCommonData_p->IsViewportEnable = TRUE;
            /* Set the viewable viewport.   */
            SetViewportOutputRectangle (LayerProperties_p);

            break;

        default :
            HALLAYER_Error();
            break;
    }
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
    HALLAYER_LayerProperties_t * LayerProperties_p;
    HALLAYER_LayerCommonData_t * LayerCommonData_p;
    U8 ViewportId;

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

   LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    switch  (LayerProperties_p->LayerType)
    {
        /* Main and Auxiliary display processor.  */
        case GENERIC_MAIN_DISPLAY   :

            /* At first, check if there's a delayed enable viewport.    */
            /* PS: Only possible on the main display.                   */
            if ((LayerCommonData_p->RegToUpdate&GLOBAL_ALPHA) == GLOBAL_ALPHA)
            {
                /* Ok, cancel it.                                       */
                LayerCommonData_p->RegToUpdate &= (~GLOBAL_ALPHA);
            }

            /* Get the viewport ID. */
            ViewportId = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->ViewportId;
            HAL_SetRegister32Value((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_XCFG1 + ViewportId * DIS_XCFG_STEP,
                                DIS_XCFG_MASK,
                                DIS_XCFG_ENA_DISA_LAYER_MASK,
                                DIS_XCFG_ENA_DISA_LAYER_EMP,
                                HALLAYER_DISABLE);

        /* Auxiliary display processor.  */
        case GENERIC_AUX_DISPLAY   :

            /* Save this state.     */
            LayerCommonData_p->IsViewportEnable = FALSE;
            /* Set the viewable viewport.   */
            SetViewportOutputRectangle (LayerProperties_p);

            break;

       default :
           HALLAYER_Error();
           break;
    }
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
   HALLAYER_LayerProperties_t * LayerProperties_p;
   U8 PriorityToReg;
   U8 ViewportId;

    /* Check null pointer                               */
    if ((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL)
    {
        return;
    }

   /* Test the viewport priority. */
   switch (Priority)
   {
   case HALLAYER_VIEWPORT_PRIORITY_0 :
        PriorityToReg = DIS_XCFG_PRIORITY_0;
        break;

    case HALLAYER_VIEWPORT_PRIORITY_1 :
        PriorityToReg = DIS_XCFG_PRIORITY_1;
        break;

    case HALLAYER_VIEWPORT_PRIORITY_2 :
        PriorityToReg = DIS_XCFG_PRIORITY_2;
        break;

    case HALLAYER_VIEWPORT_PRIORITY_3 :
        PriorityToReg = DIS_XCFG_PRIORITY_3;
        break;

    default :
        /* Should never happen !!! */
        PriorityToReg = DIS_XCFG_PRIORITY_1;
        HALLAYER_Error();
        break;
    }

    /* init Layer properties.       */
   LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

   switch  (LayerProperties_p->LayerType)
   {
        /* Main display processor.  */
        case GENERIC_MAIN_DISPLAY   :
            /* Get the viewport ID. */
            ViewportId = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->ViewportId;
            HAL_SetRegister32Value((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_XCFG1 + ViewportId * DIS_XCFG_STEP,
                                DIS_XCFG_MASK,
                                DIS_XCFG_PRIORITY_MASK,
                                DIS_XCFG_PRIORITY_EMP,
                                PriorityToReg ) ;
            break;

       /* Only one viewport in auxiliary display processor ==> no priority */
       default :
           HALLAYER_Error();
           break;
    }
} /* End of SetViewportPriority() function */

/*******************************************************************************
Name        : SetViewportSource
Description : Sets the characteristics of the source for the specified viewport
              (Source dimensions, source (Video 1,..., SD1, ...)).
Parameters  : HAL viewport manager handle.
              Data of the source.
Assumptions : The source number (VideoSourceNumber)is compatible with register's
              values.
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no error occurs.
                - ST_ERROR_FEATURE_NOT_SUPPORTED if wrong color space conversion
*******************************************************************************/
static ST_ErrorCode_t SetViewportSource(const STLAYER_ViewPortHandle_t  ViewportHandle,
                       const STLAYER_ViewPortSource_t *                 Source_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p;
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    U8 VideoSourceNumber, ViewportId;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

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

    VideoSourceNumber = Source_p->Data.VideoStream_p->SourceNumber;
    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
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

    /* Retrieve the source characteristics (source picture dimensions). */
    LayerCommonData_p->SourcePictureDimensions.Width  = (((Source_p)->Data).VideoStream_p)->BitmapParams.Width;
    LayerCommonData_p->SourcePictureDimensions.Height = (((Source_p)->Data).VideoStream_p)->BitmapParams.Height;

    /* Get the viewport ID.         */
    ViewportId = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->ViewportId;
    /* And remember it.             */
    ViewportProperties_p[ViewportId].SourceNumber = VideoSourceNumber;

    switch  (LayerProperties_p->LayerType)
    {
        /* Main display processor.  */
        case GENERIC_MAIN_DISPLAY   :
            /* Set the source.      */
            HAL_SetRegister32Value((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_XCFG1 + ViewportId * DIS_XCFG_STEP,
                                DIS_XCFG_MASK,
                                DIS_XCFG_SRC_MASK,
                                DIS_XCFG_SRC_EMP,
                                VideoSourceNumber);

            break;

        /* Auxiliary display processor. */
        case GENERIC_AUX_DISPLAY   :
            /* Set the source. */
            HAL_SetRegister32Value((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIP_XCFG,
                                DIP_XCFG_MASK,
                                DIP_XCFG_SRC_MASK,
                                DIP_XCFG_SRC_EMP,
                                VideoSourceNumber);
            break;

        default :
            HALLAYER_Error();
            break;
    }

    ErrorCode = SetColorSpaceConversionMode(LayerProperties_p,
            Source_p->Data.VideoStream_p->BitmapParams.ColorSpaceConversion);

    /* No error to return.          */
    return (ErrorCode);

} /* End of SetViewportSource() function */

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
    HALLAYER_LayerProperties_t * LayerProperties_p;
    U8 ViewportId;

    /* Check null pointer                               */
    if ((SourceNumber_p == NULL) || ((HALLAYER_ViewportProperties_t *)ViewportHandle == NULL))
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

    switch  (LayerProperties_p->LayerType)
    {
        /* Main display processor.  */
        case GENERIC_MAIN_DISPLAY :
            /* Get the viewport ID. */
            ViewportId = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->ViewportId;
            *SourceNumber_p = HAL_GetRegister32Value((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_XCFG1 + ViewportId * DIS_XCFG_STEP,
                                DIS_XCFG_SRC_MASK,
                                DIS_XCFG_SRC_EMP);
            break;

        /* Auxiliary display processor. */
        case GENERIC_AUX_DISPLAY :
            *SourceNumber_p = HAL_GetRegister32Value((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIP_XCFG,
                                DIP_XCFG_SRC_MASK,
                                DIP_XCFG_SRC_EMP);
            break;

        default :
            HALLAYER_Error();
            break;
    }
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
                         const STLAYER_ViewPortHandle_t             ViewportHandle,
                         const STLAYER_OutputMode_t                       OuputMode,
                         const STLAYER_OutputWindowSpecialModeParams_t *  const Params_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;
    U16                             cpt;
    U16                             Total = 0;
    U16                             Zones[HFC_ZONE_NBR]    = { 0, 0, 0, 0, 0 };
    S16                             Deltas[HFC_ZONE_NBR-1] =  { 0, 0, 0, 0 };
    U16                             InitialStep;

    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;
    /* Check null pointer                               */
    if (ViewportProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check input parameters.  */
    switch (OuputMode)
    {
        case STLAYER_NORMAL_MODE :
            break;
        case STLAYER_SPECTACLE_MODE :
            if ( (Params_p->SpectacleModeParams.NumberOfZone > HFC_ZONE_NBR) || /* Nb of zone out of HW constraints.    */
                 (Params_p->SpectacleModeParams.NumberOfZone <= 1)           || /* Nb of zone out of HW constraints.    */
                 (Params_p->SpectacleModeParams.NumberOfZone & 0xfffe
                  == Params_p->SpectacleModeParams.NumberOfZone)             || /* Number of zonr not even              */
                 (Params_p->SpectacleModeParams.EffectControl > 100)         || /* Effect control is greater than 100%  */
                 (Params_p->SpectacleModeParams.SizeOfZone_p == NULL)           /* Pointer is NULL                      */
               )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            for (cpt=0; cpt < Params_p->SpectacleModeParams.NumberOfZone; cpt ++)
            {
                Total += Params_p->SpectacleModeParams.SizeOfZone_p[cpt];
            }
            if (Total != 100)
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;

        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Save new parameters.....                     */
        LayerCommonData_p->OutputMode       = OuputMode;
        LayerCommonData_p->OutputModeParams.SpectacleModeParams.EffectControl
                = Params_p->SpectacleModeParams.EffectControl;
        LayerCommonData_p->OutputModeParams.SpectacleModeParams.NumberOfZone
                = Params_p->SpectacleModeParams.NumberOfZone;

        for (cpt=0; cpt < Params_p->SpectacleModeParams.NumberOfZone; cpt ++)
        {
            LayerCommonData_p->SizeOfZone[cpt] =
                    Params_p->SpectacleModeParams.SizeOfZone_p[cpt];
        }
        /* Now, should update register and scaling...   */
        UpdateHFCZones(LayerCommonData_p, Zones, Deltas, &InitialStep);
        /* Update new PSTEP value.                      */
        HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
            + DIX_HFC_PSTEP, InitialStep & DIX_HFC_PSTEP_MASK);

        SetHFCZones(ViewportProperties_p->AssociatedLayerHandle, Zones, Deltas);
    }

    return(ErrorCode);

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
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;
    U16                             cpt;

    /* Init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Check input parameters.  */
    if ( (Params_p == NULL) || (OuputMode_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    *OuputMode_p = LayerCommonData_p->OutputMode;
    switch (LayerCommonData_p->OutputMode)
    {
        case STLAYER_SPECTACLE_MODE :
            for (cpt=0; cpt < LayerCommonData_p->OutputModeParams.SpectacleModeParams.NumberOfZone; cpt++)
            {
                Params_p->SpectacleModeParams.SizeOfZone_p[cpt] = LayerCommonData_p->SizeOfZone[cpt];
            }
            break;
        case STLAYER_NORMAL_MODE :
        default :
            break;
    }
    *Params_p = LayerCommonData_p->OutputModeParams;

    return(ErrorCode);

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
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    STLAYER_UpdateParams_t          UpdateParams;
    STLAYER_LayerParams_t           LayerParams;

    /* init Layer properties.  */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p != NULL)
    {
        LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
        /* Check null pointer                               */
        if (LayerCommonData_p != NULL)
        {
            /* Check the pointer is not null. */
            if (LayerParams_p == NULL)
            {
                HALLAYER_Error();
            }
            else
            {
                /* Check if at least, one parameter changed.                */
                if ((LayerCommonData_p->LayerParams.AspectRatio  != LayerParams_p->AspectRatio) ||
                    (LayerCommonData_p->LayerParams.ScanType     != LayerParams_p->ScanType)    ||
                    (LayerCommonData_p->LayerParams.Width        != LayerParams_p->Width)       ||
                    (LayerCommonData_p->LayerParams.Height       != LayerParams_p->Height))
                {

                    /* Store the Layer Parameters in our local structure.   */
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
                    STLAYER_LayerParams_t *             LayerParams_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;

    /* Check null pointer                               */
    if (LayerParams_p == NULL)
    {
        return;
    }

    /* init Layer properties.  */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
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
    U8 ViewportId;
    HALLAYER_LayerProperties_t * LayerProperties_p;
    HALLAYER_LayerCommonData_t * LayerCommonData_p;

    /* Init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle);
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
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


    switch  (LayerProperties_p->LayerType)
    {
        /* Main display processor.  */
        case GENERIC_MAIN_DISPLAY :
            /* Get the viewport ID. */
            ViewportId = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->ViewportId;
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_XDOFF1 + ViewportId * DIS_XDOFF_STEP,
                        (LayerCommonData_p->LayerPosition.XStart +  ViewportXPosition) >> 4);
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_YDOFF1 + ViewportId * DIS_YDOFF_STEP,
                        (LayerCommonData_p->LayerPosition.YStart +  ViewportYPosition) >> 4);

            LayerCommonData_p->ViewPortParams.OutputRectangle.PositionX = ViewportXPosition;
            LayerCommonData_p->ViewPortParams.OutputRectangle.PositionY = ViewportYPosition;

            break;

        /* viewport not positionnable in aux display */
        default :
            HALLAYER_Error();
            break;
    }
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
    U8 ViewportId;
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

    switch  (LayerProperties_p->LayerType)
    {
        /* Main display processor.  */
        case GENERIC_MAIN_DISPLAY   :
            /* Get the viewport ID. */
            ViewportId = ((HALLAYER_ViewportProperties_t *)ViewportHandle)->ViewportId;

            *XPos_p = (U32)HAL_Read32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_XDOFF1 + ViewportId * DIS_XDOFF_STEP);
            *XPos_p = *XPos_p << 4 ;

            *YPos_p = (U32)HAL_Read32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_YDOFF1 + ViewportId * DIS_YDOFF_STEP);
            *YPos_p = *YPos_p << 4;

            break;

        /* viewport not positionnable in aux display */
        default :
            HALLAYER_Error();
            break;
    }
} /* End of GetViewportPosition() function */

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
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;

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

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch  (LayerProperties_p->LayerType)
    {
        case GENERIC_MAIN_DISPLAY   :
        case GENERIC_AUX_DISPLAY   :
            /* Get the viewport ID.     */
            *Params_p = LayerCommonData_p->ViewPortParams;
            break;

        /* viewport not positionnable in aux display */
        default :
            HALLAYER_Error();
            break;
    }

    /* No error to return.          */
    return(ST_NO_ERROR);

} /* End of GetViewportParams() function */

/*******************************************************************************
Name        : GetViewportPSI
Description : Set PSI filters behavior (enable/disable, gain, ...).
Parameters  : HAL layer manager handle
              HAL viewport manager handle.
              PSI parameters.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetViewportPSI(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_PSI_t * const        ViewportPSI_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Check NULL parameters                                            */
    if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get the current layer PSI parameters. */
    ErrorCode = GetViewportPsiParameters(LayerHandle, ViewportPSI_p);

    return(ErrorCode);

} /* End of GetViewportPSI() function */

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
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;

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

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch  (LayerProperties_p->LayerType)
    {
        case GENERIC_MAIN_DISPLAY   :
        case GENERIC_AUX_DISPLAY   :
            *InputRectangle_p  = LayerCommonData_p->ViewPortParams.InputRectangle;
            *OutputRectangle_p = LayerCommonData_p->ViewPortParams.OutputRectangle;
            break;

        /* viewport not positionnable in aux display */
        default :
            HALLAYER_Error();
            break;
    }

    /* No error to return.          */
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

    /* Check if any input parameters is unavailable.                                                    */
    if ( (Params_p->InputRectangle.Width == 0) || (Params_p->InputRectangle.Height  == 0) ||
         (Params_p->OutputRectangle.Width== 0) || (Params_p->OutputRectangle.Height == 0))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Re-align the negative values for (xi,yi,xo,yo).                         */
    CenterNegativeValues(LayerHandle, Params_p);

    /* Adjust the Input according to the hardware constraints.                 */
    AdjustInputWindow(LayerHandle, Params_p);

    /* Adjust the Output according to the hardware constraints.                */
    AdjustOutputWindow(LayerHandle, Params_p);

    /* No error to return.          */
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
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    /* Init Layer/Viewport properties.                                  */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)ViewportHandle;

    /* Check if any input parameters is unavailable.                    */
    if ((ViewportProperties_p == NULL) || (Params_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerProperties_p    = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    LayerCommonData_p    = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    /* Check NULL parameters                                            */
    if ((LayerProperties_p == NULL) || (LayerCommonData_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Re-align the negative values for (xi,yi,xo,yo).                  */
    CenterNegativeValues(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle, Params_p);

    /* Set the Input according to the hardware constraints.             */
    AdjustInputWindow(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle, Params_p);

    /* Set the Output according to the hardware constraints.            */
    AdjustOutputWindow(((HALLAYER_ViewportProperties_t *)(ViewportHandle))->AssociatedLayerHandle, Params_p);

    /* Store the Viewport params applied.                               */
    LayerCommonData_p->ViewPortParams = *Params_p;
    /* And its source characteristics.                                  */
    LayerCommonData_p->SourcePictureDimensions.Width    = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Width;
    LayerCommonData_p->SourcePictureDimensions.Height   = (((Params_p->Source_p)->Data).VideoStream_p)->BitmapParams.Height;

    /* Apply the ViewPort Dimensions.                                   */
    ApplyViewPortParams(ViewportHandle, Params_p);

    /* Then update filter if needed.                                    */
    UpdateViewportPsiParameters (ViewportProperties_p->AssociatedLayerHandle);

    /* No error to return.                                              */
    return (ST_NO_ERROR);

} /* End of SetViewportParams() function */

/*******************************************************************************
Name        : SetViewportPSI
Description : Set PSI filters behavior (enable/disable, gain, ...).
Parameters  : HAL layer manager handle
              HAL viewport manager handle.
              PSI parameters.
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_NO_ERROR
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t SetViewportPSI(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_PSI_t * const        ViewportPSI_p)
{
    HALLAYER_LayerProperties_t *    LayerProperties_p;
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;
    U32                             ActivityMask;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Check NULL parameters                                            */
    if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check null pointer                               */
    if (LayerCommonData_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ActivityMask = LayerCommonData_p->PSI_Parameters.ActivityMask;
    ErrorCode = SetViewportPsiParameters(LayerHandle, ViewportPSI_p);
    if (ActivityMask != LayerCommonData_p->PSI_Parameters.ActivityMask)
    {
        SetColorSpaceConversionMode(LayerProperties_p,
                     LayerCommonData_p->ViewPortParams.Source_p->Data.VideoStream_p->BitmapParams.ColorSpaceConversion);
    }

    /* Return the current error status.     */
    return(ErrorCode);

} /* End of SetViewportPSI() function */

/*******************************************************************************
Name        : SetHFCCoeffs
Description : Fills Horizontal Format Correction coefficients.
Parameters  : LumaCoeffs_p    Array of 256 coefficients, on 9 bits, 2's comp.
              ChromaCoeffs_p  Array of 128 coefficients, on 9 bits, 2's comp.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetHFCCoeffs(const STLAYER_Handle_t LayerHandle,
                 const S16 *  LumaCoeffs_p,
                 const S16 *  ChromaCoeffs_p)
{

    U32 i, j;

    /* Check NULL parameters                                            */
    if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
    {
        return;
    }

    switch  (((HALLAYER_LayerProperties_t *)LayerHandle)->LayerType)
    {
        case GENERIC_MAIN_DISPLAY   :
        case GENERIC_AUX_DISPLAY   :
            for (i=0; i<HFC_NB_PHASES; i++) /* phase */
            {
                for (j=0; j<HFC_NB_LUMA_TAPS; j++)  /* tap */
                {
                    HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_HFC_LCOEFFS_32 + ((i*8)+j)*4,
                            LumaCoeffs_p[(j*32)+i]);
                }
            }
            for (i=0; i<HFC_NB_PHASES; i++)
            {
                for (j=0; j<HFC_NB_CHROMA_TAPS; j++)    /* tap */
                {
                    HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_HFC_CCOEFFS_32+ ((i*4)+j)*4,
                                ChromaCoeffs_p[(j*32)+i]);
                }
            }
            break;

        default :
            HALLAYER_Error();
            break;
    }
} /* End of SetHFCCoeffs() function */

/*******************************************************************************
Name        : SetVFCCoeffs
Description : Fills Vertical sample rate converter
Parameters  : HAL layer manager handle, Coeffs : Array of 128 coefficients, on 9 bits, 2's comp.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetVFCCoeffs(const STLAYER_Handle_t   LayerHandle,
                  const S16 *               Coeffs)
{
    U32 i,j;

    /* Check NULL parameters                                            */
    if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
    {
        return;
    }

    switch  (((HALLAYER_LayerProperties_t *)LayerHandle)->LayerType)
    {
        case GENERIC_MAIN_DISPLAY   :
        case GENERIC_AUX_DISPLAY   :
            for (i=0; i<VFC_NB_PHASES; i++) /* phase */
            {
                for (j=0; j<VFC_NB_TAPS; j++)   /* tap */
                {
                    HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_VFC_COEFFS_32 + ((i*4)+j)*4,
                            Coeffs[(j*32)+i]);
                }
            }
            break;

        default :
            HALLAYER_Error();
            break;
    }

} /* End of SetVFCCoeffs() function */


/*******************************************************************************
Name        : SetVFCZones
Description : Sets Vertical Format Correction non-linear zones
Parameters  : Zones           End of each of the 5 vertical zones (in frame lines)
              Deltas          Change of Line Step for zones 1, 2 and 4, 5, coded as
                              a 9 bits 2's complement value, with 2 fractionnal
                              bits (unit is a quarter of an Line Step unit)
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetVFCZones(const STLAYER_Handle_t  LayerHandle,
                 U16 * const             Zones,
                 S16 * const             Deltas)

{
    U32 LoopCounter;

    /* Check NULL parameters                                            */
    if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
    {
        return;
    }

    switch  (((HALLAYER_LayerProperties_t *)LayerHandle)->LayerType)
    {
        case GENERIC_MAIN_DISPLAY   :
        case GENERIC_AUX_DISPLAY   :
            for (LoopCounter = 0; LoopCounter < VFC_ZONE_NBR; LoopCounter ++)
            {
                HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_VFC_VZONE1 + LoopCounter * 4,
                                Zones[LoopCounter] & DIX_VFC_VZONE_MASK);
            }
            for (LoopCounter = 0; LoopCounter < (VFC_ZONE_NBR - 1); LoopCounter ++)
            {
                HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_VFC_LDELTA1 + LoopCounter * 4,
                                Deltas[LoopCounter] & DIX_VFC_LDELTA_MASK);
            }
            break;

        default :
            HALLAYER_Error();
            break;
    }
}

/*******************************************************************************
Name        : SetHFCZones
Description : Sets Horizontal Format Correction non-linear zones
Parameters  : Zones           End of each of the 5 horizontal zones (in pixels)
              Deltas          Change of Pixel Step for zones 1, 2 and 4, 5, coded as
                              a 9 bits 2's complement value (unit is a Pixel Step unit)
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetHFCZones(const STLAYER_Handle_t  LayerHandle,
                 U16 * const             Zones,
                 S16 * const             Deltas)
{
U32 LoopCounter;

    /* Check NULL parameters                                            */
    if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
    {
        return;
    }

   switch  (((HALLAYER_LayerProperties_t *)LayerHandle)->LayerType)
   {
        case GENERIC_MAIN_DISPLAY   :
        case GENERIC_AUX_DISPLAY   :
           for (LoopCounter = 0; LoopCounter < HFC_ZONE_NBR; LoopCounter ++)
           {
               HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_HFC_HZONE1 + LoopCounter * 4,
                              Zones[LoopCounter] & DIX_HFC_HZONE_MASK);
           }
           for (LoopCounter = 0; LoopCounter < (HFC_ZONE_NBR - 1); LoopCounter ++)
           {
               HAL_Write32(VideoBaseAddress(LayerHandle) + DIX_HFC_PDELTA1 + LoopCounter * 4,
                              Deltas[LoopCounter] & DIX_HFC_PDELTA_MASK);
           }
           break;

       default :
           HALLAYER_Error();
           break;
    }
}

/*******************************************************************************
Name        : UpdateHFCZones
Description :
Parameters  : Zones           End of each of the 5 horizontal zones (in pixels)
              Deltas          Change of Pixel Step for zones 1, 2 and 4, 5, coded as
                              a 9 bits 2's complement value (unit is a Pixel Step unit)
Assumptions : Zones and Deltas array are initialized to zero.
Limitations :
Returns     : N.A.
*******************************************************************************/
static void UpdateHFCZones(HALLAYER_LayerCommonData_t * LayerCommonData_p,
                 U16 * const    Zones,
                 S16 * const    Deltas,
                 U16 *          InitialStep_p)
{
    U32     InputWidth, OutputWidth;
    U16     *APIZones;
    U16     CorrectedAPIZones[HFC_ZONE_NBR] = {0,0,0,0,0};
    U16     ZoneSizes[HFC_ZONE_NBR], ZonePos;
    U32     Zone1, Zone2, Zone3;
    U32     MaxDelta1;
    U32     InitialStep, Delta1, Delta2;
    U32     PowerEffect;
    U32     cpt;

    /* Retreive "real" input and outpout width. */
    InputWidth = LayerCommonData_p->RealInputWidth;
    OutputWidth= LayerCommonData_p->RealOutputWidth;

    switch (LayerCommonData_p->OutputMode)
    {
        case STLAYER_SPECTACLE_MODE :
            APIZones = LayerCommonData_p->OutputModeParams.SpectacleModeParams.SizeOfZone_p;

            /* Transform a little bit zones.. */
            if (LayerCommonData_p->OutputModeParams.SpectacleModeParams.NumberOfZone == 3)
            {
                CorrectedAPIZones[0]                = APIZones[0];
                CorrectedAPIZones[HFC_ZONE_NBR/2]   = APIZones[1];
                CorrectedAPIZones[HFC_ZONE_NBR-1]   = APIZones[2];
            }
            else
            {
                for (cpt=0; cpt < HFC_ZONE_NBR; cpt++)
                {
                    CorrectedAPIZones[cpt] = APIZones[cpt];
                }
            }

            ZonePos = 0;
            for (cpt=0; cpt < HFC_ZONE_NBR; cpt++)
            {
                ZoneSizes[cpt]  = (CorrectedAPIZones[cpt] * OutputWidth)/100;
                Zones[cpt]      = ZoneSizes[cpt] + ZonePos;
                ZonePos        += ZoneSizes[cpt];
            }


            /* Be sure output width is in last zone[x] register.    */
            Zones[HFC_ZONE_NBR-1] = OutputWidth;

            /* Temporary variables to optimize code.    */
            Zone1 = (U32)((U16)ZoneSizes[0]);
            Zone2 = (U32)((U16)ZoneSizes[1]);
            Zone3 = (U32)((U16)ZoneSizes[2]);
            /* Calculate the max. Delta1 value so that initial step is zero. */
            MaxDelta1 =
                (8*8192*InputWidth) / (Zone1*(Zone1-1)+((Zone2*(Zone2-1))/2)+2*Zone2*Zone1+Zone1*Zone3+(Zone2*Zone3)/2);
            if (MaxDelta1 > DIX_HFC_PDELTA_MAX_VALUE)
            {
                MaxDelta1 = DIX_HFC_PDELTA_MAX_VALUE;
            }

            PowerEffect = LayerCommonData_p->OutputModeParams.SpectacleModeParams.EffectControl;
            Delta1 = (MaxDelta1*PowerEffect == 0 ? 1 : (MaxDelta1 * PowerEffect) / 100);
            Delta2 = Delta1/2;

            InitialStep = (8192*InputWidth - (Delta1*Zone1*(Zone1-1))/8 - (Delta2*Zone2*(Zone2-1))/8
                    - (2*Zone1*Zone2*Delta1)/8 - (Zone3*(Delta1*Zone1+Delta2*Zone2))/8) / (2*Zone1+2*Zone2+Zone3);
            /* Now test if InitialStep is out of spec. */
            if (InitialStep > DIX_HFC_PSTEP_MAX_VALUE )
            {
                Delta1 = DIX_HFC_PSTEP_MAX_VALUE;
            }

            *InitialStep_p = InitialStep;
            /* Update Zones and Deltas..    */
            Deltas[0] = Delta1;
            Deltas[1] = Delta2;
            Deltas[2] = -Delta2;
            Deltas[3] = -Delta1;

            break;

        case STLAYER_NORMAL_MODE :
        default :
            /* Only last zone is used */
            Zones[HFC_ZONE_NBR-1] = LayerCommonData_p->ViewPortParams.
                            OutputRectangle.Width;
            *InitialStep_p = (U32)((HALLAYER_SCALING_FACTOR
                    * InputWidth) / OutputWidth);
            break;
    }

} /* End of UpdateHFCZones() function. */

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
    HALLAYER_LayerCommonData_t *LayerCommonData_p;
    STLAYER_LayerParams_t       LayerParams;
    STLAYER_UpdateParams_t      UpdateParams;

    /* Check NULL parameters                                            */
    if (OutputParams_p == NULL)
    {
        return;
    }

    /* Init Layer properties.                   */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p) ;
    /* Check NULL parameters                                            */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Retrieve the screen parameters.          */
    if ((OutputParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)
    {
        /* Save new layer offset parameters.    */
        LayerCommonData_p->LayerPosition.XOffset = OutputParams_p->XOffset;
        LayerCommonData_p->LayerPosition.YOffset = OutputParams_p->YOffset;
        /* This change need also the re-calculation of all. */
        GetLayerParams(LayerHandle, &LayerParams);
        UpdateParams.LayerParams_p = &LayerParams;
        UpdateParams.UpdateReason = STLAYER_LAYER_PARAMS_REASON;
        STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle, ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID], (const void *) (&UpdateParams));
    }

    if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
    {
        /* Save new layer start position.           */
        LayerCommonData_p->LayerPosition.XStart = OutputParams_p->XStart;
        LayerCommonData_p->LayerPosition.YStart = OutputParams_p->YStart;

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
    }

    if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
    {
        /* Retrieve the VTGFrameRate of output. */
        LayerCommonData_p->VTGFrameRate = OutputParams_p->FrameRate;
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
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;


    /* Check NULL parameters                                            */
    if (Alpha_p == NULL)
    {
        return;
    }

    /* Init Layer/Viewport properties.                                  */
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

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters                                            */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Set the global alpha value for this video gamma. */
    HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_ALP,
            Alpha_p->A0 & VIn_ALP_MASK);

    LayerCommonData_p->CurrentGlobalAlpha = (U32) (Alpha_p->A0);

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
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;

    /* Check NULL parameters                                            */
    if (Alpha_p == NULL)
    {
        return;
    }

    /* Init Layer/Viewport properties.                                                                  */
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

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters                                            */
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Get the global alpha value for this video gamma. */
    Alpha_p->A0 = (U8) (LayerCommonData_p->CurrentGlobalAlpha);

} /* End of GetViewportAlpha() function. */

/* private MAIN DISPLAY management functions -------------------------------- */
/*******************************************************************************
Name        : EnableLMU
Description : Enables the LMU use, in order to deinterlace the standard
              definition stream.
Parameters  : HAL layer manager handle : LayerHandle.
Assumptions : To use only with MAIN DISPLAY
Limitations :
Returns     :
*******************************************************************************/
static void EnableLMU (HALLAYER_LayerProperties_t * LayerProperties_p)
{

    /* DIS_DCI_BYPASS Bypass control for the PSI section.   */
    HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_LMU_CTRL,
            ((1 & DIS_LMU_CTRL_EN_MASK) << DIS_LMU_CTRL_EN_C_EMP) |  /* enable luma LMU                     */
            ((2 & DIS_LMU_CTRL_CK_MASK) << DIS_LMU_CTRL_CK_C_EMP) |  /* Clock luma control : divide by 3    */
            ((1 & DIS_LMU_CTRL_MC_MASK) << DIS_LMU_CTRL_MC_C_EMP) |  /* chroma motion comparison enable     */
            ((1 & DIS_LMU_CTRL_EN_MASK) << DIS_LMU_CTRL_EN_Y_EMP) |  /* enable chroma LMU                   */
            ((3 & DIS_LMU_CTRL_CK_MASK) << DIS_LMU_CTRL_CK_Y_EMP)    /* Clock luma control : divide by 4    */
            );

} /* End of EnableLMU() function. */

/*******************************************************************************
Name        : DisableLMU
Description : Disables the LMU use
Parameters  : HAL layer manager handle : LayerHandle.
Assumptions : To use only with MAIN DISPLAY
Limitations :
Returns     :
*******************************************************************************/
static void DisableLMU (HALLAYER_LayerProperties_t * LayerProperties_p)
{

    /* DIS_DCI_BYPASS Bypass control for the PSI section.   */
    HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_LMU_CTRL, 0);

} /* End of DisableLMU() function. */


/* private AUXILIARY DISPLAY management functions --------------------------- */

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
    HALLAYER_LayerCommonData_t *    LayerCommonData_p;

    /* init Layer properties.               */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    /* Check NULL parameters                                            */
    if (LayerCommonData_p == NULL)
    {
        return;
    }


    switch (LayerProperties_p->LayerType)
    {
        /* Main display processor.      */
        case GENERIC_MAIN_DISPLAY   :

            /* LMU default settings.    */
            /* Set the LMU luma start pointer.  */
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_LMP,
                                        ((U32)(LayerCommonData_p->LmuLumaBuffer_p) & (DIS_LMP_MASK << DIS_LMP_EMP)));
            /* Set the LMU chroma start pointer.*/
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_LCMP,
                                        ((U32)(LayerCommonData_p->LmuChromaBuffer_p) & (DIS_LCMP_MASK << DIS_LCMP_EMP)));
            break;

        /* Auxiliary display processor. */
        case GENERIC_AUX_DISPLAY   :
            break;
        default:
            HALLAYER_Error();
            break;
    }

    /* Initialization of the video gamma compositor registers */
    /* VIn_CTL : control */
#ifdef WA_GNBvd06322
    HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_CTL, HALLAYER_DISABLE);
#else
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_CTL,
                                  VIn_CTL_MASK,
                                  HALLAYER_DISABLE);
#endif

    /* VIn_ALP : Global Alpha Value              */
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_ALP,
                                VIn_ALP_MASK,
                                VIn_ALP_DEFAULT);

    /* VIn_KEY1 : lower limit of the color range */
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_KEY1,
                                  VIn_KEY1_MASK,
                                  HALLAYER_DISABLE);

    /* VIn_KEY2 : upper limit of the color range */
    HAL_SetRegister32DefaultValue((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_KEY2,
                                  VIn_KEY2_MASK,
                                  HALLAYER_DISABLE);

} /* End of InitializeVideoProcessor() function */

/*******************************************************************************
Name        : CheckForAvailableScaling
Description : Check if the wanted scaling is available.
Parameters  : IN :  layer handle : LayerHandle
                    viewport parameters : ViewPortParams_p
                    current decimation        (horizontal and vertical)
              OUT : recommended decimations   (horizontal and vertical)
                    recommended output sizes  (horizontal and vertical)
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
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    BOOL                            IsEndOfScalingTestReached = FALSE;
    BOOL                            IsSquareFrequencyDetected = FALSE;
    BOOL                            IsSDFrequencyDetected     = FALSE;

    /* Input parameters.                */
    U32                 SourceHorizontalSize;
    U32                 OriginalSourceHorizontalSize;
    U32                 SourceVerticalSize;
    U32                 SourceScanTypeFactor; /* Scan mode factor : 0 for interlaced, 1 for progressive.  */
    U32                 InputHorizontalSize;
    U32                 InputVerticalSize;

    /* Intermediate input parameters.   */
    U32                 VLRin;              /* Number of input active lines read from memory for luma   */
                                            /* Vin/(Vdec*(2-PDL))                                       */
    U32                 OriginalVLRin = 0;      /* Original value of VLRin, before decimation manipulation. */
    U32                 HRin;               /* Number of input picture pixels read from memory          */
                                            /* Hin/Hdec                                                 */
    U32                 HorizontalRequiredZoomFactor;
    U32                 VerticalRequiredZoomFactor;     /* Vout / VLRin */

    /* Output parameters.               */
    U32                 OutputVTGFrameRate; /* Calculated from VTGFrameRate.                            */
    U32                 ScanTypeFactor;     /* Scan mode factor : 0 for interlaced, 1 for progressive.  */
    U32                 OutputHorizontalSize;
    U32                 OutputVerticalSize;
    U32                 Vout;               /* Number of output active lines per vertical period (field */
                                            /* active lines if display is interlaced, frame if display  */
                                            /* is progressive)                                          */
    U32                 TotalOutputHorizontalSize;
    U32                 TotalOutputVerticalSize;

    /* Bandwidth claculation parameters.*/
    U32                 OutputPixelClock;
    U32                 OutputRequiredPixelRate;

    U32                 CurrentPictureHorizontalDecimationFactor;
    U32                 CurrentPictureVerticalDecimationFactor;
    U32                 RecommendedHDecimation;
    U32                 RecommendedVDecimation;

    U32                 MaxVerticalZoomFactor;
    U32                 MaxHorizontalZoomFactor;

    BOOL                IsRecommendedOutputSizeFound = FALSE;
    U32                 RecommendedOutputHorizontalSize;
    U32                 RecommendedOutputVerticalSize;
    U32                 RecommendedOutputVerticalSize2;

    U32 HardwareMaxOutPixelRate;
    U32 AuxDisplayBandwidthLimitation;

    /* Init Layer/Viewport properties.  */
    LayerProperties_p   = (HALLAYER_LayerProperties_t *)LayerHandle;
    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);

    CurrentPictureHorizontalDecimationFactor = GetDecimationFactorFromDecimation(CurrentPictureHorizontalDecimation);
    CurrentPictureVerticalDecimationFactor = GetDecimationFactorFromDecimation(CurrentPictureVerticalDecimation);
    RecommendedHDecimation = CurrentPictureHorizontalDecimationFactor;
    RecommendedVDecimation = CurrentPictureVerticalDecimationFactor;

#ifdef TRACE_HAL_VIDEO_LAYER
    TraceBuffer(("\r\nCheckForAvailableScaling %s:DecH:%d DecV:%d",
        ((LayerProperties_p->LayerType == GENERIC_MAIN_DISPLAY)?"MAIN":"AUX"),
        CurrentPictureHorizontalDecimationFactor,
        CurrentPictureVerticalDecimationFactor));
#endif

    /* Calculate the output scan mode factor.       */
    ScanTypeFactor   = (LayerCommonData_p->LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN ? 0 : 1);

#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
    if ((((LayerCommonData_p->ViewPortParams.Source_p)->Data).VideoStream_p)->BitmapParams.Height
            >= FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED)
    {
        SourceScanTypeFactor = 0;
    }
    else
    {
        SourceScanTypeFactor=
            ((((LayerCommonData_p->ViewPortParams.Source_p)->Data).VideoStream_p)->ScanType == STGXOBJ_INTERLACED_SCAN ? 0 : 1);
    }
#else
    SourceScanTypeFactor=
            ((((LayerCommonData_p->ViewPortParams.Source_p)->Data).VideoStream_p)->ScanType == STGXOBJ_INTERLACED_SCAN ? 0 : 1);
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */

    SourceVerticalSize   = (((LayerCommonData_p->ViewPortParams.Source_p)->Data).VideoStream_p)->BitmapParams.Height;
    SourceHorizontalSize = (((LayerCommonData_p->ViewPortParams.Source_p)->Data).VideoStream_p)->BitmapParams.Width;
    OriginalSourceHorizontalSize = SourceHorizontalSize;

    /* Compute the total output vertical size (screen parameters).  */
#ifdef WA_GNBvd09294
    TotalOutputVerticalSize   = LayerCommonData_p->LayerParams.Height + LayerCommonData_p->LayerPosition.YStart
                                + BOTTOM_BLANKING_SIZE;
#else
    TotalOutputVerticalSize   = LayerCommonData_p->LayerParams.Height + LayerCommonData_p->LayerPosition.YStart;
#endif /* WA_GNBvd09294 */
    TotalOutputHorizontalSize = LayerCommonData_p->LayerParams.Width  + LayerCommonData_p->LayerPosition.XStart;

    OutputHorizontalSize = ViewPortParams_p->OutputRectangle.Width;
    OutputVerticalSize   = ViewPortParams_p->OutputRectangle.Height;

    /* Default : recommended output window is the original one. */
    RecommendedOutputHorizontalSize = OutputHorizontalSize;
    RecommendedOutputVerticalSize   = OutputVerticalSize;

    InputHorizontalSize = ViewPortParams_p->InputRectangle.Width;
    InputVerticalSize   = ViewPortParams_p->InputRectangle.Height;

    /* VTG Frame Rate calculation.                              */
    /* So that 60000HZ becomes 60, and 59940 becomes 60 too.    */
    OutputVTGFrameRate = LayerCommonData_p->VTGFrameRate / 1000;
    if ( (OutputVTGFrameRate * 1000) < LayerCommonData_p->VTGFrameRate)
    {
        OutputVTGFrameRate += 1;
    }

    /* Test if a Square output frequency is activated.          */
    if (    (LayerCommonData_p->LayerParams.Width  == 640)
        &&  (LayerCommonData_p->LayerParams.Height == 480)
        &&(LayerCommonData_p->LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN) )
    {
        IsSquareFrequencyDetected = TRUE;
    }
    /* Test if a normal SD output is activated.                 */
    else if (  (LayerCommonData_p->LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN)
       && (LayerCommonData_p->LayerParams.Width  == 720) )
    {
        IsSDFrequencyDetected = TRUE;
    }
    else
    {
        /* Else it's the HD ouput that is detected. No flag to check it.    */
    }

    switch (LayerProperties_p->LayerType)
    {
#ifdef ST_7015
        case GENERIC_MAIN_DISPLAY :
            HardwareMaxOutPixelRate = STi7015_DISPLAY_CLOCK_SD;
            break;
        case GENERIC_AUX_DISPLAY :
            if (IsSquareFrequencyDetected)
            {
                HardwareMaxOutPixelRate = STi7015_AUX_DISPLAY_CLOCK_SQUARE;
            }
            else
            {
                HardwareMaxOutPixelRate = STi7015_AUX_DISPLAY_CLOCK_SD;
            }
            break;
#else /* not ST_7015 */
        case GENERIC_MAIN_DISPLAY :
            if (IsSquareFrequencyDetected)
            {
                HardwareMaxOutPixelRate = STi7020_DISPLAY_CLOCK_SQUARE;
            }
            else if (IsSDFrequencyDetected)
            {
                HardwareMaxOutPixelRate = STi7020_DISPLAY_CLOCK_SD;
            }
            else
            {
                HardwareMaxOutPixelRate = STi7020_DISPLAY_CLOCK_HD;
            }
            break;

        case GENERIC_AUX_DISPLAY :
            if (IsSquareFrequencyDetected)
            {
                HardwareMaxOutPixelRate = STi7020_AUX_DISPLAY_CLOCK_SQUARE;
            }
            else if (IsSDFrequencyDetected)
            {
                HardwareMaxOutPixelRate = STi7020_AUX_DISPLAY_CLOCK_SD;
            }
            else
            {
                HardwareMaxOutPixelRate = STi7020_AUX_DISPLAY_CLOCK_HD;
            }
            break;
#endif /* not ST_7015 */
        default :
            /* Should never happen !!!!             */
            HardwareMaxOutPixelRate = DEFAULT_DISPLAY_CLOCK;
            break;
    }
    /* Apply the security coefficient.                          */
    HardwareMaxOutPixelRate = (HardwareMaxOutPixelRate / 100) * SECURITY_COEFF;

    /* Avoid division by zero.  */
    if ( (RecommendedHDecimation    == 0) || (RecommendedVDecimation    == 0) ||
         (SourceVerticalSize        == 0) || (SourceHorizontalSize      == 0) ||
         (OutputHorizontalSize      == 0) || (OutputVerticalSize        == 0) ||
         (TotalOutputHorizontalSize == 0) || (TotalOutputVerticalSize   == 0) ||
         (InputVerticalSize         == 0) || (InputHorizontalSize       == 0) ||
         (OutputVTGFrameRate        == 0) )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Test specific case for auxiliary diaplay. Output width have to   */
    /* be less than 720 pixels.                                         */
    if ( (LayerProperties_p->LayerType == GENERIC_AUX_DISPLAY) &&
         (OutputHorizontalSize > HALLAYER_MAX_AUXILIARY_DISPLAY_WIDTH) )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Original Intermediate input parameters calculation.  */
        OriginalVLRin = InputVerticalSize / (RecommendedVDecimation  * (2 - SourceScanTypeFactor));

        while (!IsEndOfScalingTestReached)
        {
            /* Intermediate input parameters calculation.   */
            VLRin = InputVerticalSize / (RecommendedVDecimation  * (2 - SourceScanTypeFactor));
            HRin = InputHorizontalSize / RecommendedHDecimation;

            /* Ouput parameters calculation.                */
            Vout  = OutputVerticalSize / (2 - ScanTypeFactor);

            /* Limitation calculation :                     */

            /* HFC / VFC limitations.                       */
            /* NB: those are number are * 10000 multiple.   */
            VerticalRequiredZoomFactor   = (Vout * 10000) / VLRin ;
            HorizontalRequiredZoomFactor = (OutputHorizontalSize * 10000)/ HRin;

            MaxVerticalZoomFactor   = ((1 + ScanTypeFactor) * 10000)/ (6 * (1 + SourceScanTypeFactor));
            MaxHorizontalZoomFactor = HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR;

            /* Calculate the ouput pixel clock.             */
            OutputPixelClock = (TotalOutputHorizontalSize * TotalOutputVerticalSize * OutputVTGFrameRate)
                    / (2 - ScanTypeFactor);

#ifdef TRACE_HAL_VIDEO_LAYER
       /*     TraceBuffer(("\r\n   PixClk : %d (Max %d), VFC %d (Max %d), HFC %d (Max %d) HDec %d VDec %d",
                            OutputRequiredPixelRate, HardwareMaxOutPixelRate,
                            VerticalRequiredZoomFactor, MaxVerticalZoomFactor,
                            HorizontalRequiredZoomFactor, MaxHorizontalZoomFactor,
                            RecommendedHDecimation, RecommendedVDecimation ));*/
#endif
            switch (LayerProperties_p->LayerType)
            {
                /* Main display processor.      */
                case GENERIC_MAIN_DISPLAY   :

                    /* Calculate the output required pixel rate.    */
                    OutputRequiredPixelRate = (OutputPixelClock / TotalOutputHorizontalSize)
                            * ( (SourceHorizontalSize * (VLRin > Vout ? VLRin : Vout)) / (Vout * RecommendedHDecimation) );

                    if (OutputRequiredPixelRate < HardwareMaxOutPixelRate)
                    {
                        /* Ok, global bandwidth respected. Now check for vertical zoom factor (VFC).  */
                        if (VerticalRequiredZoomFactor > MaxVerticalZoomFactor)
                        {
                            /* Ok. Now check for horizontal zoom factor (HFC).*/
                            if (HorizontalRequiredZoomFactor >= MaxHorizontalZoomFactor)
                            {
                                /* OK, the scaling is possible.             */
                                ErrorCode = ST_NO_ERROR;
                                /* Ok, as the current setting is correct.       */
                                IsEndOfScalingTestReached = TRUE;
                            }
                            else
                            {
                                if (!IsRecommendedOutputSizeFound)
                                {
                                    /* Third case : HFC KO                  */
                                    RecommendedOutputHorizontalSize = (HRin * HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR) / 10000;
                                    /* Ok, recommended output window found. */
                                    IsRecommendedOutputSizeFound = TRUE;
                                }

                                /* HFC zoom factor impossible.                  */
                                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                                /* Try to increase the horizontal decimation.   */
                                if (RecommendedHDecimation < 4)
                                {
                                    /* Ok, increase the horizontal decimation.  */
                                    RecommendedHDecimation <<= 1;
                                }
                                else
                                {
                                    /* Max horizontal decimation is reached.    */
                                    /* Impossible to perform the wanted scaling.*/
                                    IsEndOfScalingTestReached = TRUE;
                                }
                            } /* if (HorizontalRequiredZoomFactor >= MaxHorizontalZoomFactor) */
                        }
                        else /* if (VerticalRequiredZoomFactor > MaxVerticalZoomFactor) */
                        {
                            if (!IsRecommendedOutputSizeFound)
                            {
                                /* Second case : VFC KO                 */
                                RecommendedOutputVerticalSize = (MaxVerticalZoomFactor * InputVerticalSize * (2 - ScanTypeFactor))
                                    / (CurrentPictureVerticalDecimationFactor * (2 - SourceScanTypeFactor) * 10000) + 3;

                                /* Now check for horizontal zoom factor (HFC).*/
                                if (HorizontalRequiredZoomFactor < MaxHorizontalZoomFactor)
                                {
                                    /* Third case : HFC KO                  */
                                    RecommendedOutputHorizontalSize = (HRin * HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR) / 10000;
                                }
                                /* Ok, recommended output window found. */
                                IsRecommendedOutputSizeFound = TRUE;
                            }

                            /* VFC zoom factor impossible.                  */
                            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                            /* Try to increase the vertical decimation, then horizontal one. */
                            if (RecommendedVDecimation < 4)
                            {
                                /* Ok, increase the vertical decimation.    */
                                RecommendedVDecimation <<= 1;
                            }
                            else
                            {
                                /* Max vertical decimation is reached.      */
                                /* Try to increase the horizontal one.      */
                                if (RecommendedHDecimation < 4)
                                {
                                    /* Ok, increase the horizontal decimation.  */
                                    RecommendedHDecimation <<= 1;
                                }
                                else
                                {
                                    /* Impossible to perform the wanted scaling.*/
                                    IsEndOfScalingTestReached = TRUE;
                                }
                            }
                        } /* if (VerticalRequiredZoomFactor > MaxVerticalZoomFactor) */
                    }
                    else /* if (OutputRequiredPixelRate < HardwareMaxOutPixelRate) */
                    {
                        if (!IsRecommendedOutputSizeFound)
                        {
                            /* First case : Required pixel rate KO      */
                            RecommendedOutputVerticalSize = (((TotalOutputVerticalSize * OutputVTGFrameRate)/100 + 1)
                                    * (VLRin * SourceHorizontalSize))
                                    / (CurrentPictureHorizontalDecimationFactor * (HardwareMaxOutPixelRate / 100));

                            /* Now check for vertical zoom factor (VFC).*/
                            if (VerticalRequiredZoomFactor < MaxVerticalZoomFactor)
                            {
                                /* Second case : VFC KO                 */
                                /* PS: + 3 is done for specific little vertical values. */
                                RecommendedOutputVerticalSize2 = (MaxVerticalZoomFactor * InputVerticalSize * (2 - ScanTypeFactor))
                                    / (CurrentPictureVerticalDecimationFactor * (2 - SourceScanTypeFactor) * 10000) + 3;
                                if (RecommendedOutputVerticalSize2 > RecommendedOutputVerticalSize )
                                {
                                    RecommendedOutputVerticalSize = RecommendedOutputVerticalSize2;
                                }
                            }
                            /* Now check for horizontal zoom factor (HFC).*/
                            if (HorizontalRequiredZoomFactor < MaxHorizontalZoomFactor)
                            {
                                /* Third case : HFC KO                  */
                                RecommendedOutputHorizontalSize = (HRin * HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR) / 10000;
                            }
                            /* Ok, recommended output window found. */
                            IsRecommendedOutputSizeFound = TRUE;
                        }

                        /* Output pixel rate impossible with current decimation.        */
                        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                        /* Try to increase the horizontal decimation, then vertical one.*/
                        if (RecommendedHDecimation < 4)
                        {
                            /* Ok, increase the horizontal decimation.  */
                            RecommendedHDecimation <<= 1;
                        }
                        else
                        {
                            /* Max horizontal decimation is reached.    */
                            /* Try to increase the vertical one.      */
                            if (RecommendedVDecimation < 4)
                            {
                                /* Ok, increase the vertical decimation.  */
                                RecommendedVDecimation <<= 1;
                            }
                            else
                            {
                                /* Impossible to perform the wanted scaling.*/
                                IsEndOfScalingTestReached = TRUE;
                            }
                        }
                    } /* if (OutputRequiredPixelRate < HardwareMaxOutPixelRate) */

                    break;

                /* Auxiliary display processor. */
                case GENERIC_AUX_DISPLAY   :

                    /* Calculate the output required pixel rate.    */
                    OutputRequiredPixelRate = (OutputPixelClock / TotalOutputHorizontalSize)
                            * ( (SourceHorizontalSize * VLRin) / (Vout * RecommendedHDecimation) );

                    if (OutputRequiredPixelRate < HardwareMaxOutPixelRate)
                    {
                        /* Ok, global bandwidth respected. Now check for vertical zoom factor (VFC).  */
                        if (VerticalRequiredZoomFactor > MaxVerticalZoomFactor)
                        {
                            /* Ok. Now check for horizontal zoom factor (HFC).*/
                            if (HorizontalRequiredZoomFactor >= MaxHorizontalZoomFactor)
                            {
                                /* OK, the scaling is possible.             */
                                ErrorCode = ST_NO_ERROR;
                                /* Ok, as the current setting is correct.       */
                                IsEndOfScalingTestReached = TRUE;
                            }
                            else
                            {
                                if (!IsRecommendedOutputSizeFound)
                                {
                                    /* Third case : HFC KO                  */
                                    RecommendedOutputHorizontalSize = (HRin * HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR) / 10000;
                                    /* Ok, recommended output window found. */
                                    IsRecommendedOutputSizeFound = TRUE;
                                }

                                /* HFC zoom factor impossible.                  */
                                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                                /* Try to increase the horizontal decimation.   */
                                if (RecommendedHDecimation < 4)
                                {
                                    /* Ok, increase the horizontal decimation.  */
                                    RecommendedHDecimation <<= 1;
                                }
                                else
                                {
                                    /* Max horizontal decimation is reached.    */
                                    /* Impossible to perform the wanted scaling.*/
                                    IsEndOfScalingTestReached = TRUE;
                                }
                            } /* if (HorizontalRequiredZoomFactor >= MaxHorizontalZoomFactor) */
                        }
                        else /* if (VerticalRequiredZoomFactor > MaxVerticalZoomFactor) */
                        {
                            if (!IsRecommendedOutputSizeFound)
                            {
                                /* Second case : VFC KO                 */
                                RecommendedOutputVerticalSize = (MaxVerticalZoomFactor * InputVerticalSize * (2 - ScanTypeFactor))
                                    / (CurrentPictureVerticalDecimationFactor * (2 - SourceScanTypeFactor) * 10000) + 3;

                                /* Now check for horizontal zoom factor (HFC).*/
                                if (HorizontalRequiredZoomFactor < MaxHorizontalZoomFactor)
                                {
                                    /* Third case : HFC KO                  */
                                    RecommendedOutputHorizontalSize = (HRin * HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR) / 10000;
                                }
                                /* Ok, recommended output window found. */
                                IsRecommendedOutputSizeFound = TRUE;
                            }

                            /* VFC zoom factor impossible.                  */
                            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                            /* Try to increase the vertical decimation, then horizontal one. */
                            if (RecommendedVDecimation < 4)
                            {
                                /* Ok, increase the vertical decimation.    */
                                RecommendedVDecimation <<= 1;
                            }
                            else
                            {
                                /* Max vertical decimation is reached.      */
                                /* Try to increase the horizontal one.      */
                                if (RecommendedHDecimation < 4)
                                {
                                    /* Ok, increase the horizontal decimation.  */
                                    RecommendedHDecimation <<= 1;
                                }
                                else
                                {
                                    /* Impossible to perform the wanted scaling.*/
                                    IsEndOfScalingTestReached = TRUE;
                                }
                            }
                        } /* if (VerticalRequiredZoomFactor > MaxVerticalZoomFactor) */
                    }
                    else /* if (OutputRequiredPixelRate < HardwareMaxOutPixelRate) */
                    {
                        if (!IsRecommendedOutputSizeFound)
                        {
                            /* First case : Required pixel rate KO      */
                            RecommendedOutputVerticalSize = (((TotalOutputVerticalSize * OutputVTGFrameRate)/100 + 1)
                                    * (VLRin * SourceHorizontalSize))
                                    / (CurrentPictureHorizontalDecimationFactor * (HardwareMaxOutPixelRate / 100));

                            /* Now check for vertical zoom factor (VFC).*/
                            if (VerticalRequiredZoomFactor < MaxVerticalZoomFactor)
                            {
                                /* Second case : VFC KO                 */
                                /* PS: + 3 is done for specific little vertical values. */
                                RecommendedOutputVerticalSize2 = (MaxVerticalZoomFactor * InputVerticalSize * (2 - ScanTypeFactor))
                                    / (CurrentPictureVerticalDecimationFactor * (2 - SourceScanTypeFactor) * 10000) + 3;
                                if (RecommendedOutputVerticalSize2 > RecommendedOutputVerticalSize )
                                {
                                    RecommendedOutputVerticalSize = RecommendedOutputVerticalSize2;
                                }
                            }
                            /* Now check for horizontal zoom factor (HFC).*/
                            if (HorizontalRequiredZoomFactor < MaxHorizontalZoomFactor)
                            {
                                /* Third case : HFC KO                  */
                                RecommendedOutputHorizontalSize = (HRin * HFC_MAIN_DISPLAY_MAX_ZOOM_FACTOR) / 10000;
                            }
                            /* Ok, recommended output window found. */
                            IsRecommendedOutputSizeFound = TRUE;
                        }

                        /* Output pixel rate impossible with current decimation.        */
                        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                        /* Try to increase the horizontal decimation, then vertical one.*/
                        if (RecommendedHDecimation < 4)
                        {
                            /* Ok, increase the horizontal decimation.  */
                            RecommendedHDecimation <<= 1;
                        }
                        else
                        {
                            /* Max horizontal decimation is reached.    */
                            /* Try to increase the vertical one.      */
                            if (RecommendedVDecimation < 4)
                            {
                                /* Ok, increase the vertical decimation.  */
                                RecommendedVDecimation <<= 1;
                            }
                            else
                            {
                                /* Impossible to perform the wanted scaling.*/
                                IsEndOfScalingTestReached = TRUE;
                            }
                        }
                    } /* if (OutputRequiredPixelRate < HardwareMaxOutPixelRate) */

                    break;

                default :
                    HALLAYER_Error();
                    /* Bad video layer type.    */
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            } /* switch layer type */
        } /* while (!IsEndOfScalingTestReached) */
    } /* no error */

    if (ErrorCode != ST_ERROR_BAD_PARAMETER)
    {
        /* Check if scaling is possible with current decimation.    */
        if ((RecommendedHDecimation != CurrentPictureHorizontalDecimationFactor) ||
            (RecommendedVDecimation != CurrentPictureVerticalDecimationFactor) )
        {
            /* It's a recommended one. Return an error. */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }

        /* And update in/out recommended decimation parameters. */
        *RecommendedHorizontalDecimation_p = GetDecimationFromDecimationFactor(RecommendedHDecimation);
        *RecommendedVerticalDecimation_p = GetDecimationFromDecimationFactor(RecommendedVDecimation);

        if (LayerProperties_p->LayerType == GENERIC_AUX_DISPLAY)
            {
            /* Maximum allowed bandwidth for both chroma and luma.      */
            /* To set in registers DIP_IF_YRCC and DIP_IF_CRCC.         */
            AuxDisplayBandwidthLimitation = (((16 * TotalOutputVerticalSize * OutputVTGFrameRate)/1000 + 1)
                                                * (OriginalVLRin * OriginalSourceHorizontalSize))
                                                / (RecommendedOutputVerticalSize * (HardwareMaxOutPixelRate / 1000));

            /* Test if register not too big (it should not happen !!).  */
            if (AuxDisplayBandwidthLimitation >= 16)
            {
                AuxDisplayBandwidthLimitation = 15;
            }

#ifdef TRACE_HAL_VIDEO_LAYER
       /*     TraceBuffer(("\r\n   Limitation Bandwidth Reg : %d", AuxDisplayBandwidthLimitation));*/
#endif

            /* Set the available bandwith for the auxiliary-display pipeline.   */
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_IF_YRCC,
                    AuxDisplayBandwidthLimitation & DIP_IF_YRCC_MASK);
            HAL_Write32(VideoBaseAddress(LayerHandle) + DIP_IF_CRCC,
                    AuxDisplayBandwidthLimitation & DIP_IF_CRCC_MASK);
            }

    }
    /* Set the recommended size for the output sizes.   */
    *RecommendedHorizontalSize_p = RecommendedOutputHorizontalSize;
    *RecommendedVerticalSize_p   = RecommendedOutputVerticalSize;

#ifdef TRACE_HAL_VIDEO_LAYER
    TraceBuffer(("--> Result %d\r\n",ErrorCode  ));
#endif /* TRACE_HAL_VIDEO_LAYER */
    return(ErrorCode);

} /* End of CheckForAvailableScaling function */

/* private DEBUG functions -------------------------------------------------- */

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

/*******************************************************************************
Name        : ComputeAndApplyHfcVfc
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ComputeAndApplyHfcVfc(HALLAYER_LayerCommonData_t *    LayerCommonData_p,
                HALLAYER_ViewportProperties_t * ViewportProperties_p,
                HALLAYER_LayerProperties_t * LayerProperties_p,
                U32 PanVector,U32 ScanVector,
                U32 SourceHorizontalSize, U32 SourceVerticalSize,
                U32 InputHorizontalSize, U32  InputVerticalSize,
                U32 OutputHorizontalSize, U32  OutputVerticalSize,
                STGXOBJ_ScanType_t ScanType, STGXOBJ_ScanType_t SourceScanType)
{
    U16 Zones[VFC_ZONE_NBR]    = { 0, 0, 0, 0, 0 };
    S16 Deltas[VFC_ZONE_NBR-1] =  { 0, 0, 0, 0 };
    U16 InitialStep;
    U32 SourceHorizontalSizeRounded;

    /* Vertical Format Control */
    /*-------------------------*/

    /* Set Vertical Zones */
    /* Only last zone is used */
    Zones[VFC_ZONE_NBR-1] = LayerCommonData_p->ViewPortParams.
                            OutputRectangle.Height;
    SetVFCZones(ViewportProperties_p->AssociatedLayerHandle, Zones, Deltas);

    /*- Set VFC Scaling -----------------------------------------------------*/
    /* By definition, there are 8192 steps between progressive input lines */
    /* and 16384 steps between  Interlaced input lines. This unsigned      */
    /* registers defines the number of steps between the lines at the output */
    /* of the VFC. In other words, DIS_VFC_LSTEP controls the  vertical zoom.*/
    /* The vertical  zoom is calculated as: vzoom = 8192/L_STEP. This means */
    /* that LSTEP  values less than 8192 soecify a  vertical compression  */
    /* while LSTEP values greater than 8192 specify a vertical expansion.    */

    /* Avoid division by zero... */
    if (OutputVerticalSize != 0)
    {
        HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_VFC_LSTEP,
            (U32)(HALLAYER_SCALING_FACTOR * InputVerticalSize / OutputVerticalSize)
                    & DIX_VFC_LSTEP_MASK);
    }

    if (ScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_VFC_LSTEP_INIT,
                (((ScanVector  & 0x1 )* HALLAYER_SCALING_FACTOR) >> 4)
                & DIX_VFC_LSTEP_INIT_MASK);
    }
    else
    {
        HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_VFC_LSTEP_INIT,
                (((ScanVector & 0x3 ) * HALLAYER_SCALING_FACTOR) >> 4)
                & DIX_VFC_LSTEP_INIT_MASK);
    }

    /* HINP */
    /* the value to write in HINP must be even ==> & 0xFFFE */
    HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
            + DIX_VFC_HINP,
            ((U32)PanVector) & 0xFFFE & DIX_VFC_HINP_MASK);

    /*- number of VFC Input Lines --------------------------------------------*/
    /* for 4:2:0 inputs the register value must be a multiple of 2 for */
    /* progressive inputs and a multiple of 4  for interlace inputs. */
    /* For 4:2:2 inputs the register value must be a multiple of 2 for */
    /* interlace inputs. There are no limitations for 4:2:2 progressive */
    /* inputs */

    /*- Stored Horizontal Size -----------------------------------------------*/

    /* Get the number of pixel of the source picture, rounded to the upper 16 value */
    if ( (SourceHorizontalSize & 0xFFFFFFF0) != SourceHorizontalSize)
    {
        SourceHorizontalSizeRounded = ( (SourceHorizontalSize + ~0xFFFFFFF0) & 0xFFFFFFF0 & DIX_VFC_HSINL_MASK);
    }
    else
    {
        SourceHorizontalSizeRounded = SourceHorizontalSize;
    }

    /* set stored image horizontal size. The value can differ  */
    /* from the original input format because of the decimation */
    switch  (LayerProperties_p->LayerType)
    {
        case GENERIC_MAIN_DISPLAY  :
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_VFC_HSINL,
                (SourceHorizontalSizeRounded & DIX_VFC_HSINL_MASK));

            /* The main window size is used with the source size, in order to allow to  */
            /* use the "greatest" capability of the hardware.                           */
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_MXFW,
                    (SourceHorizontalSizeRounded >> 4) & DIS_MXFW_MASK);

            /* configure the BTL: number of pairs of 8 bits blocks per line  */
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_BTL_BPL,
                    (SourceHorizontalSizeRounded >> 4) & DIS_BTL_BPL_MASK);

            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_VFC_VINL,
                (InputVerticalSize & DIX_VFC_VINL_MASK) );


#ifdef WA_GNBvd06059
            if ((LayerCommonData_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422) || (LayerCommonData_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)) /* && image height < 1024 ? */
            {
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_MXFH,
                        DIS_MXFH_MASK);
            }
            else
            {
#endif
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p + DIS_MXFH,
                        ((SourceVerticalSize >> 4) + 1) & DIS_MXFH_MASK);
#ifdef WA_GNBvd06059
            }
#endif
            /*- Set Scanning mode ----------------------------------------------------*/
            /* Test if LMU module has to be used.       */
            /* enable conditions are :                  */
            /*  - Input stream INTERLACED.                                              */
            /*  - SourceVerticalSise up to 480 for NTSC and 576 to PAL standards.       */
            /*  - Vout > 1.8 Vin if interlaced ouput.                                   */
            /*  - Vout > 0.9 Vin if progressive output.                                 */
            if ( (SourceHorizontalSizeRounded <= HALLAYER_MAX_LMU_WIDTH)  &&
                 (SourceScanType              == STGXOBJ_INTERLACED_SCAN) &&
                 (((ScanType == STGXOBJ_INTERLACED_SCAN)
                    &&  ((OutputVerticalSize * 100) >= (2*InputVerticalSize * HALLAYER_LMU_USE_THRESHOLD_INTERLACED)) ) ||
                  ((ScanType == STGXOBJ_PROGRESSIVE_SCAN)
                    && ((OutputVerticalSize * 100) >= (InputVerticalSize * HALLAYER_LMU_USE_THRESHOLD_PROGRESSIVE)) ) )
                 )
            {

                /* Enable LMU control.  */
                EnableLMU (LayerProperties_p);

                /* VIID : input image type. the LSB is for luma and MSB for Chroma. */
                /* As LMU module is used, the VFC input type is progressive.        */
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                        + DIX_VFC_IID,
                        3 & DIX_VFC_IID_MASK);

#ifdef TRACE_HAL_VIDEO_LAYER
                /*TraceBuffer(("\r\n LMU enabled..."));*/
#endif
            }
            else
            {
                /* VIID : input image type. the LSB is for luma and MSB for Chroma. */
                HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                        + DIX_VFC_IID,
                    ((U32)(SourceScanType == STGXOBJ_PROGRESSIVE_SCAN) |
                    (U32)((SourceScanType == STGXOBJ_PROGRESSIVE_SCAN) << 1))
                    & DIX_VFC_IID_MASK);

                /* Disable LMU control.  */
                DisableLMU (LayerProperties_p);

#ifdef TRACE_HAL_VIDEO_LAYER
       /*         TraceBuffer(("\r\n LMU disabled..."));*/
#endif
            }

            break;

        case GENERIC_AUX_DISPLAY  :
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_VFC_HSINL,
                ( SourceHorizontalSizeRounded & DIX_VFC_HSINL_MASK));

            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_VFC_VINL,
            SourceVerticalSize  & DIX_VFC_VINL_MASK);

            /*- Set Scanning mode ----------------------------------------------------*/
            /* VIID : input image type. the LSB is for luma and MSB for Chroma. */
            HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                    + DIX_VFC_IID,
                ((U32)(SourceScanType == STGXOBJ_PROGRESSIVE_SCAN) |
                (U32)((SourceScanType == STGXOBJ_PROGRESSIVE_SCAN) << 1))
                & DIX_VFC_IID_MASK);

            break;
        default:
            /* Should never get here */
            break;
    }

    /* Then enable it.... */
    if((100 * OutputVerticalSize) < (50 * InputVerticalSize))
    {
        if(LayerCommonData_p->VerticalFormatFilter != HALLAYER_C_FILTER)
        {
            LayerCommonData_p->VerticalFormatFilter = HALLAYER_C_FILTER;
            /* multi-task protection */
            STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);
            LayerCommonData_p->RegToUpdate         |= VERTICAL_C_FILTER;
            LayerCommonData_p->RegToUpdate         &= ~VERTICAL_A_FILTER;
            LayerCommonData_p->RegToUpdate         &= ~VERTICAL_B_FILTER;
            LayerCommonData_p->IsUpdateNeeded       = TRUE;
            /* multi-task protection */
            STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);
        }
    }
    else if ((100 * OutputVerticalSize) < (90 * InputVerticalSize))
    {
        if(LayerCommonData_p->VerticalFormatFilter != HALLAYER_B_FILTER)
        {
            LayerCommonData_p->VerticalFormatFilter = HALLAYER_B_FILTER;
            /* multi-task protection */
            STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);
            LayerCommonData_p->RegToUpdate         |= VERTICAL_B_FILTER;
            LayerCommonData_p->RegToUpdate         &= ~VERTICAL_A_FILTER;
            LayerCommonData_p->RegToUpdate         &= ~VERTICAL_C_FILTER;
            LayerCommonData_p->IsUpdateNeeded       = TRUE;
            /* multi-task protection */
            STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);
        }
    }
    else
    {
        if (LayerCommonData_p->VerticalFormatFilter != HALLAYER_A_FILTER)
        {
            LayerCommonData_p->VerticalFormatFilter = HALLAYER_A_FILTER;
            /* multi-task protection */
            STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);
            LayerCommonData_p->RegToUpdate         |= VERTICAL_A_FILTER;
            LayerCommonData_p->RegToUpdate         &= ~VERTICAL_B_FILTER;
            LayerCommonData_p->RegToUpdate         &= ~VERTICAL_C_FILTER;
            LayerCommonData_p->IsUpdateNeeded       = TRUE;
            /* multi-task protection */
            STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);
        }
    }

#ifdef WA_GNBvd08340
        /* Check if a vertical conversion is needed.*/
        if (InputVerticalSize == OutputVerticalSize)
        {
            /* No, disable the vertical filtering component according   */
            /* to input type (4:2:0 or 4:2:2).                          */
            if ( (LayerCommonData_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)
               ||(LayerCommonData_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422))
            {
                DisableVerticalFiltering (ViewportProperties_p->AssociatedLayerHandle, TRUE, TRUE);
            }
            else
            {
                DisableVerticalFiltering (ViewportProperties_p->AssociatedLayerHandle, FALSE, TRUE);
            }
        }
        else
        {
            EnableVerticalFiltering (ViewportProperties_p->AssociatedLayerHandle);
        }
#else
    EnableVerticalFiltering (ViewportProperties_p->AssociatedLayerHandle);
#endif

    /* Horizontal Format Control */
    /*-------------------------*/

    /* Set Horizontal Zones */
    /* Update zones and delta according to requirements.    */
    UpdateHFCZones(LayerCommonData_p, Zones, Deltas, &InitialStep);

    SetHFCZones(ViewportProperties_p->AssociatedLayerHandle, Zones, Deltas);

    /*- destination Width ----------------------------------------------------*/
    /* set luma destination width (be sure it's a multiple of 2)*/
    HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
            + DIX_HFC_YHINL,
            (InputHorizontalSize) & DIX_HFC_YHINL_MASK & 0xFFFFFFFE);

    /* set chroma desttion width, we're always processing 4:2:2 or 4:2:0 (?) */
    /* The value to be written in CHINL must be even */
    /* Should it be divided by 2 ???? */
    HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
            + DIX_HFC_CHINL,
            (InputHorizontalSize) & DIX_HFC_CHINL_MASK & 0xFFFFFFFE);

    /*- Set HFC Scaling -----------------------------------------------------*/
    /* set PSTEP number of steps between two output pixels. The number of    */
    /* steps between 2 input pixels is 8192. For linear SRC, the step value  */
    /* should be: 8192*(InputHorizontalSize/InputHorizontalSize). non-linear */
    /* mode, proper initial value should becomputed by the application.      */
    if (OutputHorizontalSize != 0)
    {
        HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
            + DIX_HFC_PSTEP, InitialStep & DIX_HFC_PSTEP_MASK);
    }

    /* PSTEP_INIT */
    if (ScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
        HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_HFC_PSTEP_INIT,
                ((PanVector * 2 * HALLAYER_SCALING_FACTOR) >> 4)
                & DIX_HFC_PSTEP_INIT_MASK);
    }
    else
    {
        HAL_Write32((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p
                + DIX_HFC_PSTEP_INIT,
                ((PanVector * HALLAYER_SCALING_FACTOR) >> 4)
                & DIX_HFC_PSTEP_INIT_MASK);
    }

    /* Then enable it.... */
    if((100 * OutputHorizontalSize) < (50 * InputHorizontalSize))
    {
        if(LayerCommonData_p->HorizontalFormatFilter != HALLAYER_C_FILTER)
        {
            LayerCommonData_p->HorizontalFormatFilter = HALLAYER_C_FILTER;
            /* multi-task protection */
            STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);
            LayerCommonData_p->RegToUpdate           |= HORIZONTAL_C_FILTER;
            LayerCommonData_p->RegToUpdate           &= ~HORIZONTAL_A_FILTER;
            LayerCommonData_p->RegToUpdate           &= ~HORIZONTAL_B_FILTER;
            LayerCommonData_p->IsUpdateNeeded         = TRUE;
            /* multi-task protection */
            STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);
        }
    }
    else if ((100 * OutputHorizontalSize) < (90 * InputHorizontalSize))
    {
        if(LayerCommonData_p->HorizontalFormatFilter != HALLAYER_B_FILTER)
        {
            LayerCommonData_p->HorizontalFormatFilter = HALLAYER_B_FILTER;
            /* multi-task protection */
            STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);
            LayerCommonData_p->RegToUpdate           |= HORIZONTAL_B_FILTER;
            LayerCommonData_p->RegToUpdate           &= ~HORIZONTAL_A_FILTER;
            LayerCommonData_p->RegToUpdate           &= ~HORIZONTAL_C_FILTER;
            LayerCommonData_p->IsUpdateNeeded         = TRUE;
            /* multi-task protection */
            STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);
        }
    }
    else
    {
        if(LayerCommonData_p->HorizontalFormatFilter != HALLAYER_A_FILTER)
        {
            LayerCommonData_p->HorizontalFormatFilter = HALLAYER_A_FILTER;
            /* multi-task protection */
            STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);
            LayerCommonData_p->RegToUpdate           |= HORIZONTAL_A_FILTER;
            LayerCommonData_p->RegToUpdate           &= ~HORIZONTAL_B_FILTER;
            LayerCommonData_p->RegToUpdate           &= ~HORIZONTAL_C_FILTER;
            LayerCommonData_p->IsUpdateNeeded         = TRUE;
            /* multi-task protection */
            STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);
        }
    }
#ifdef WA_GNBvd13019
    DisableHorizontalFiltering(ViewportProperties_p->AssociatedLayerHandle);
#else
    /* Check if a horizontal conversion is needed.*/
    if (InputHorizontalSize == OutputHorizontalSize)
    {
        /* No, disable the horizontal scaling.   */
        DisableHorizontalFiltering(ViewportProperties_p->AssociatedLayerHandle);
    }
    else
    {
        EnableHorizontalFiltering(ViewportProperties_p->AssociatedLayerHandle);
    }
#endif

} /* End of ComputeAndApplyHfcVfc */

/*******************************************************************************
Name        : SetColorSpaceConversionMode
Description : Set color space conversion settings.
Parameters  : HAL layer properties : LayerProperties_p
              Color space conversion mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR or ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t SetColorSpaceConversionMode(
        const HALLAYER_LayerProperties_t * const LayerProperties_p,
        const STGXOBJ_ColorSpaceConversionMode_t ColorSpaceConversionMode)
{
   HALLAYER_LayerCommonData_t *            LayerCommonData_p;

    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);


    switch (ColorSpaceConversionMode)
    {
        case    STGXOBJ_ITU_R_BT470_2_M:
        case    STGXOBJ_ITU_R_BT470_2_BG:
        case    STGXOBJ_SMPTE_170M:
        case    STGXOBJ_FCC:
        case    STGXOBJ_ITU_R_BT601 :

            /* Test of current chip. Video Compositor matrix are not the same */
            /* Between STi7015 and STi7020.                                   */
#ifdef ST_7015
            /* ********************** STi7015 chip ********************** */
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR1,
                    VIn_MPR1_MASK & 0x0a950040);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR2,
                    VIn_MPR2_MASK & 0x0acc0000);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR3,
                    VIn_MPR3_MASK & 0x09300737);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR4,
                    VIn_MPR4_MASK & 0x00000c81);
#else /* not ST_7015 */
            /* ********************** STi7020 chip ********************** */
            if (LayerCommonData_p->PSI_Parameters.ActivityMask != 0 )
            {
                HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR1,
                    VIn_MPR1_MASK & 0x0a880020);;
            }
            else
            {
                HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR1,
                    VIn_MPR1_MASK & 0x0a800000);;
            }
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR2,
                    VIn_MPR2_MASK & 0x0aaf0000);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR3,
                    VIn_MPR3_MASK & 0x094e0754);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR4,
                    VIn_MPR4_MASK & 0x00000add);
#endif /* not ST_7015 */
            break;

        case    STGXOBJ_SMPTE_240M:
        case    STGXOBJ_ITU_R_BT709 :

            /* Test of current chip. Video Compositor matrix are not the same   */
            /* Between STi7015 and STi7020.                                     */
#ifdef ST_7015
            /* ********************** STi7015 chip ********************** */
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR1,
                    VIn_MPR1_MASK & 0x0a950040);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR2,
                    VIn_MPR2_MASK & 0x0ae60000);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR3,
                    VIn_MPR3_MASK & 0x09780526);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR4,
                    VIn_MPR4_MASK & 0x00000c87);
#else /* not STi7015 */
            /* ********************** STi7020 chip ********************** */
            if (LayerCommonData_p->PSI_Parameters.ActivityMask != 0)
            {
                HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR1,
                    VIn_MPR1_MASK & 0x0a800000);
            }
            else
            {
                   HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR1,
                    VIn_MPR1_MASK & 0x0a880020);
            }
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR2,
                    VIn_MPR2_MASK & 0x0ac50000);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR3,
                    VIn_MPR3_MASK & 0x07160544);
            HAL_Write32(
                    (U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_MPR4,
                    VIn_MPR4_MASK & 0x00000ae8);
#endif /* not STi7015 */
            break;
        case    STGXOBJ_CONVERSION_MODE_UNKNOWN:
            /* Do not change the default matrix (set to CCIR 709).  */
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);

        default :
            return (ST_ERROR_BAD_PARAMETER);
    }
    /* OK, return a good status.    */
    return(ST_NO_ERROR);

} /* End of SetColorSpaceConversionMode function */

/*******************************************************************************
Name        : SetViewportOutputRectangle
Description : According to the enable/disable viewport flag, the output
              viewport is set or not to the correct values in order to
              see it.
Parameters  : HAL layer common data : LayerCommonData_p
Assumptions : The fields IsViewportEnable, ViewportOffset and ViewportStop
              are assumed to be correctly set.
Limitations :
Returns     :
*******************************************************************************/
static void SetViewportOutputRectangle (const HALLAYER_LayerProperties_t *    LayerProperties_p)
{

    HALLAYER_LayerCommonData_t *    LayerCommonData_p;

    /* Check NULL parameters                                            */
    if (LayerProperties_p == NULL)
    {
        return;
    }

    /* init Layer properties.                   */
    LayerCommonData_p = (HALLAYER_LayerCommonData_t *)(LayerProperties_p->PrivateData_p);
    if (LayerCommonData_p == NULL)
    {
        return;
    }

    /* Test if the viewport is enable or not.   */
    if (LayerCommonData_p->IsViewportEnable)
    {
        /* The viewport is enable. We must apply the right parameters.          */

        /* VIn_VPO : Viewport Origin.   */
        HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_VPO,
                LayerCommonData_p->ViewportOffset);
        /* VIn_VPS : Viewport Stop.     */
        HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_VPS,
                LayerCommonData_p->ViewportStop);
    }
    else
    {
        /* The viewport is disable. We must apply an invisible area parameters. */
        /* VIn_VPO : Viewport Origin.   */
        HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_VPO, 0);
        /* VIn_VPS : Viewport Stop.     */
        HAL_Write32((U8 *)LayerProperties_p->GammaBaseAddress_p + VIn_VPS, 0);
    }

} /* End of SetViewportOutputRectangle */

/*******************************************************************************
Name        : GetDecimationFactorFromDecimation
Description : Convert a given decimation into a decimation factor that can be
              applied to picture sizes.
Parameters  : Decimation enumerate Decimation.
              Converted decimation factor : DecimationFactor.
Assumptions :
Limitations :
Returns     : decimation factor.
*******************************************************************************/
static U32 GetDecimationFactorFromDecimation (STLAYER_DecimationFactor_t Decimation)
{
    switch (Decimation)
    {
        case STLAYER_DECIMATION_FACTOR_NONE :
            return( 1 );
            break;

        case STLAYER_DECIMATION_FACTOR_2 :
            return( 2 );
            break;

        case STLAYER_DECIMATION_FACTOR_4 :
            return( 4 );
            break;

        default :
            return( 1 );
            break;
    }
} /* End of GetDecimationFactorFromDecimation */


/*******************************************************************************
Name        : GetDecimationFromDecimationFactor
Description : Convert a given decimation factor into a decimation that can be
              applied to picture sizes.
Parameters  : Decimation enumerate Decimation.
              Converted decimation : Decimation.
Assumptions :
Limitations :
Returns     : decimation.
*******************************************************************************/
static STLAYER_DecimationFactor_t GetDecimationFromDecimationFactor (U32 DecimationFactor)
{
    switch (DecimationFactor)
    {
        case 1 :
            return( STLAYER_DECIMATION_FACTOR_NONE );
            break;

        case 2 :
            return( STLAYER_DECIMATION_FACTOR_2 );
            break;

        case 4 :
            return( STLAYER_DECIMATION_FACTOR_4 );
            break;

        default :
            return( STLAYER_DECIMATION_FACTOR_NONE );
            break;
    }
} /* End of GetDecimationFromDecimationFactor */


/*******************************************************************************
Name        : IsUpdateNeeded
Description : Tell the upper level if an update is needed on next VSync
Parameters  :
Assumptions :
Limitations :
Returns     : BOOL
*******************************************************************************/
static BOOL IsUpdateNeeded(const STLAYER_Handle_t LayerHandle)

{
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)
                          (((HALLAYER_LayerProperties_t *)LayerHandle)->PrivateData_p);
    return(LayerCommonData_p->IsUpdateNeeded);
}

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
    HALLAYER_LayerCommonData_t    * LayerCommonData_p;
    HALLAYER_ViewportProperties_t * ViewportProperties_p;

    LayerCommonData_p   = (HALLAYER_LayerCommonData_t *)
                          (((HALLAYER_LayerProperties_t *)LayerHandle)->PrivateData_p);

    ViewportProperties_p= (HALLAYER_ViewportProperties_t *)
                          (LayerCommonData_p->ViewportProperties_p);

    /* multi-task protection */
    STOS_SemaphoreWait(LayerCommonData_p->MultiAccessSem_p);

    /* test the updates to be done */
    if((LayerCommonData_p->RegToUpdate & HORIZONTAL_A_FILTER)
                                      == HORIZONTAL_A_FILTER)
    {
            SetHFCCoeffs(ViewportProperties_p->AssociatedLayerHandle,
                        &HorLumaCoeffA[0],&HorChromaCoeffA[0]);
#ifdef TRACE_HAL_VIDEO_LAYER
            TraceBuffer(("Horizontal Coeff 'A' loaded\r\n"));
#endif
    }
    if((LayerCommonData_p->RegToUpdate & HORIZONTAL_B_FILTER)
                                      == HORIZONTAL_B_FILTER)
    {
            SetHFCCoeffs(ViewportProperties_p->AssociatedLayerHandle,
                        &HorLumaCoeffB[0],&HorChromaCoeffB[0]);
#ifdef TRACE_HAL_VIDEO_LAYER
            TraceBuffer(("Horizontal Coeff 'B' loaded\r\n"));
#endif
    }
    if((LayerCommonData_p->RegToUpdate & HORIZONTAL_C_FILTER)
                                      == HORIZONTAL_C_FILTER)
    {
            SetHFCCoeffs(ViewportProperties_p->AssociatedLayerHandle,
                        &HorLumaCoeffC[0],&HorChromaCoeffC[0]);
#ifdef TRACE_HAL_VIDEO_LAYER
            TraceBuffer(("Horizontal Coeff 'C' loaded\r\n"));
#endif
    }
    if((LayerCommonData_p->RegToUpdate & VERTICAL_A_FILTER)
                                      == VERTICAL_A_FILTER)
    {
            SetVFCCoeffs(ViewportProperties_p->AssociatedLayerHandle,
                        &VerLumaCoeffA[0]);
#ifdef TRACE_HAL_VIDEO_LAYER
            TraceBuffer(("Vertical Coeff 'A' loaded\r\n"));
#endif
    }
    if((LayerCommonData_p->RegToUpdate & VERTICAL_B_FILTER)
                                      == VERTICAL_B_FILTER)
    {
            SetVFCCoeffs(ViewportProperties_p->AssociatedLayerHandle,
                        &VerLumaCoeffB[0]);
#ifdef TRACE_HAL_VIDEO_LAYER
            TraceBuffer(("Vertical Coeff 'B' loaded\r\n"));
#endif
    }
    if((LayerCommonData_p->RegToUpdate & VERTICAL_C_FILTER)
                                      == VERTICAL_C_FILTER)
    {
            SetVFCCoeffs(ViewportProperties_p->AssociatedLayerHandle,
                        &VerLumaCoeffC[0]);
#ifdef TRACE_HAL_VIDEO_LAYER
            TraceBuffer(("Vertical Coeff 'C' loaded\r\n"));
#endif
    }

    if((LayerCommonData_p->RegToUpdate & DCI_FILTER)
                                      == DCI_FILTER)
    {
        DCIProcessEndOfAnalysis(LayerHandle);
    }

    if((LayerCommonData_p->RegToUpdate & GLOBAL_ALPHA)
                                      == GLOBAL_ALPHA)
    {
        SetViewportAlpha((STLAYER_ViewPortHandle_t) (ViewportProperties_p),
                (STLAYER_GlobalAlpha_t *)(&LayerCommonData_p->CurrentGlobalAlpha));
    }

    /* reset the flag */
    LayerCommonData_p->IsUpdateNeeded = FALSE;
    LayerCommonData_p->RegToUpdate    = 0;

    /* multi-task protection */
    STOS_SemaphoreSignal(LayerCommonData_p->MultiAccessSem_p);
} /* End of SynchronizedUpdate() function. */

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
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(RequestedMode_p);

    /* Feature not implemented on Omega2 */
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

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
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(RequestedMode);

    /* Feature not implemented on Omega2 */
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

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
    UNUSED_PARAMETER(ViewportHandle);

    /* Feature not implemented on Omega2 */
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
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
    UNUSED_PARAMETER(ViewportHandle);

    /* Feature not implemented on Omega2 */
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
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
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(Params_p);

    /* Feature not implemented on Omega2 */
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
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
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(Params_p);

    /* Feature not implemented on Omega2 */
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}



