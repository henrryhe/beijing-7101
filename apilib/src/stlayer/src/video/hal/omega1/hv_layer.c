/*******************************************************************************

File name   : hv_layer.c

Description : Video HAL (Hardware Abstraction Layer) access to hardware source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
23 Feb 2000        Created                                          PLe
02 Aug 2000        Adaptation for Omega1                            AN
23 Aug 2002        New UpdateScreenParams function call.            GG
14 May 2003        Changed prototype of SetCIFMode() for zoomoutx2
                   Implemented zoomoutx2 in B2R computation algorithm
                   Fix pbs ComputeTheNumberOfLinesRemoved() size 0  HG
17 Nov 2003        Negative offset bug (integer division rounding)  CD
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Work-arounds */
#define WA_GNBvd02889       /* Limitation in video horizontal pan maximum value */
#define WA_GNBvd24691       /* Bug in line drop causing line missing (bad) */

/*#define TRACE_UART*/
#if defined TRACE_UART
#include "../../../tests/src/trace.h"
#endif /* TRACE_UART  */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>

#include "stddefs.h"
#include "stgxobj.h"
#include "stlayer.h"
#include "sttbx.h"

#include "hv_layer.h"
#include "hv_reg.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/*  */
#define HALLAYER_DISABLE               0
#define HALLAYER_ENABLE                1

/* ERROR */
#define HALLAYER_ERROR                 0xFF

/* MAX NUMBER OF VIEWPORT ALLOWED */
#define LAYER_VIEWPORT_MAX_NB                                   1

/* Video Source */
#define        HALLAYER_VIDEO_SOURCE_MAX_REGVALUE               7

/* Viewport priority & Instance*/
#define        HALLAYER_VIEWPORT_MAX_PRIORITY_REGVALUE          3
#define        HALLAYER_VIEWPORT_MAX_INSTANCE_VALUE             3

/* Scan mode */
#define        HALLAYER_SCAN_MODE_MAX_REGVALUE                  1

/* Dummy value that we set to check that a handle is valid, only one chance  */
/* out of 2^32 to get it by accident ! */
#define HALLAYER_VALID_HANDLE 0xde48a925


/* Private Types ------------------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */
static BOOL IsMainLayerInitialized = FALSE;
#ifdef DEBUG
static void HALLAYER_Error(void);
#endif

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
static ST_ErrorCode_t Init(
                         const STLAYER_Handle_t             LayerHandle,
                         const STLAYER_InitParams_t * const LayerInitParams_p);
static ST_ErrorCode_t OpenViewport(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_ViewPortParams_t * const    Params_p,
                         STLAYER_ViewPortHandle_t *          ViewportHandle_p);
static ST_ErrorCode_t CloseViewport(
                         const STLAYER_ViewPortHandle_t      ViewportHandle);
static ST_ErrorCode_t Term(
                         const STLAYER_Handle_t              LayerHandle,
                         const STLAYER_TermParams_t *        TermParams_p);
static void GetLayerParams(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_LayerParams_t *             LayerParams_p);
static void GetCapability(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_Capability_t *              Capability);
static void EnableVideoViewport(
                         const STLAYER_ViewPortHandle_t      ViewportHandle);
static void DisableVideoViewport(
                         const STLAYER_ViewPortHandle_t      ViewportHandle);
static void SetViewportPriority(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         const HALLAYER_ViewportPriority_t   Priority);
static void SetViewportAlpha(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         STLAYER_GlobalAlpha_t*              Alpha);
static void GetViewportAlpha(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         STLAYER_GlobalAlpha_t*              Alpha);
static ST_ErrorCode_t SetViewportSource(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         const STLAYER_ViewPortSource_t *    Source_p);
static void GetViewportSource(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         U32                 *               SourceNumber_p);
static ST_ErrorCode_t SetViewportSpecialMode(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         const STLAYER_OutputMode_t         OuputMode,
                         const STLAYER_OutputWindowSpecialModeParams_t * const Params_p);
static ST_ErrorCode_t GetViewportSpecialMode (
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_OutputMode_t *             const OuputMode_p,
                         STLAYER_OutputWindowSpecialModeParams_t * const Params_p);
static void SetLayerParams(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_LayerParams_t * const       LayerParams_p);
static void SetViewportPosition(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         const U32                           XPos,
                         const U32                           YPos);
static void GetViewportPosition(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         U32 *                               XPos_p,
                         U32 *                               YPos_p);
static void UpdateFromMixer(
                         const STLAYER_Handle_t              LayerHandle,
                         STLAYER_OutputParams_t * const      OutputParams_p);
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

void EnableVerticalScaling(const STLAYER_Handle_t     LayerHandle);
void DisableVerticalScaling(const STLAYER_Handle_t    LayerHandle);
extern void AdjustHorizontalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        STLAYER_ViewPortParams_t * const    Params_p,
                        U32*                                Zx_p);
extern void AdjustVerticalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        STLAYER_ViewPortParams_t * const    Params_p,
                        U32*                                Zy_p);
extern void ApplyHorizontalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        U32                                 Zx,
                        U32                                 Zy);
extern void ApplyVerticalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        BOOL                                Progressive,
                        U32                                 Zx,
                        U32                                 Zy);
extern void SetViewPortPositions(
                        const STLAYER_ViewPortHandle_t      ViewportHandle,
                        STGXOBJ_Rectangle_t * const         OutputRectangle,
                        const BOOL                          Apply);
extern void InitialiseLayer(
                        const STLAYER_Handle_t              LayerHandle,
                        const STLAYER_Layer_t               LayerType);
extern void SetInputMargins(
                        const  STLAYER_ViewPortHandle_t ViewportHandle,
                        S32 * const LeftMargin_p,
                        S32 * const TopMargin_p,
                        const  BOOL Apply);
extern void SetCIFMode(
                        const STLAYER_Handle_t              LayerHandle,
                        const BOOL                          LumaFrameBased,
                        const BOOL                          ChromaFrameBased,
                        const BOOL                          LumaAndChromaThrowOneLineInTwo);

extern void UpdateScreenParams(
                        const STLAYER_Handle_t              LayerHandle,
                        const STLAYER_OutputParams_t *      OutputParams_p);

extern void WriteB2RRegisters(const  STLAYER_Handle_t  LayerHandleLayerHandle);

/* Global Variables --------------------------------------------------------- */
const HALLAYER_FunctionsTable_t  HALLAYER_Omega1Functions =
{
    AdjustViewportParams,            /* Hardware writing */
    CloseViewport,
    DisableFMDReporting,
    DisableVideoViewport,            /* Hardware writing */
    EnableFMDReporting,
    EnableVideoViewport,             /* Hardware writing */
    GetCapability,
    GetFMDParams,
    GetLayerParams,
    GetRequestedDeinterlacingMode,   /* Not used for Omega1 */
    GetViewportAlpha,
    GetViewportParams,
    GetViewportPSI,                  /* Not used for Omega1 */
    GetViewportIORectangle,
    GetViewportPosition,             /* Not used for Omega1 */
    GetViewportSource,               /* Not used for Omega1 */
    GetViewportSpecialMode,
    Init,
    IsUpdateNeeded,                  /* Not used for Omega1 */
    OpenViewport,
    SetFMDParams,
    SetLayerParams,
    SetRequestedDeinterlacingMode,   /* Not used for Omega1 */
    SetViewportAlpha,                /* Hardware writing */
    SetViewportParams,               /* Hardware writing */
    SetViewportPSI,                  /* Not used for Omega1 */
    SetViewportPosition,             /* Not used for Omega1 */
    SetViewportPriority,             /* Not used for Omega1 */
    SetViewportSource,               /* Not used for Omega1 */
    SetViewportSpecialMode,
    SynchronizedUpdate,              /* Not used for Omega1 */
    UpdateFromMixer,
    Term
};                                   /* 15 / 22 functions are useful for Omega1! */


/* Private Function prototypes ---------------------------------------------- */
#if defined(HW_5508) || defined(HW_5518) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578)
void ComputeTheNumberOfLinesRemoved(
                            const HALLAYER_B2RVerticalFiltersParams_t * B2RVerticalFiltersParams_p,
                            const U32 * const OriginalInputSize_p,
                            const U32 ScanIn16thPixel,
                            U32 * const SmallestOutputSize_p);
#endif /* defined(HW_5508) || defined(HW_5518) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578) */


/*******************************************************************************
Name        :
Description : Enables Vertical Format Correction on luma and chroma signals
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void EnableVerticalScaling(const STLAYER_Handle_t   LayerHandle)
{
#ifdef DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No sense for Omega1 cells."));
#endif /* DEBUG */
}


/*******************************************************************************
Name        :
Description : Disables Vertical Format Correction on luma and chroma signals
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void DisableVerticalScaling(const STLAYER_Handle_t   LayerHandle)
{
#ifdef DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No sense for Omega1 cells."));
#endif /* DEBUG */
}



/*******************************************************************************
Name        : AdjustInputWindow
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AdjustInputWindow(
        const STLAYER_Handle_t              LayerHandle,
        STLAYER_ViewPortParams_t * const    Params_p,
        const U32                           HorizontalZoomFactor)
{
S32 XOffset, YOffset;
U32 SourcePictureHorizontalSize, SourcePictureVerticalSize;
#if defined WA_GNBvd02889
BOOL HasConstraintOnPan = FALSE;
#endif /* defined WA_GNBvd02889 */

  /* Retrieve the parameters of the source.                                  */
  SourcePictureHorizontalSize = (((Params_p->Source_p)->Data).VideoStream_p)->
                                    BitmapParams.Width;
  SourcePictureVerticalSize   = (((Params_p->Source_p)->Data).VideoStream_p)->
                                    BitmapParams.Height;

  /* Clip input window.                                                       */
  if (Params_p->InputRectangle.Width > SourcePictureHorizontalSize)
  {
    Params_p->InputRectangle.Width = SourcePictureHorizontalSize;
  }

  if (Params_p->InputRectangle.Height > SourcePictureVerticalSize)
  {
    Params_p->InputRectangle.Height = SourcePictureVerticalSize;
  }

  XOffset = Params_p->InputRectangle.PositionX ;
  YOffset = Params_p->InputRectangle.PositionY ;

  if (XOffset < 0)
  {
    XOffset = 0;
  }
  if ((U32)XOffset > (SourcePictureHorizontalSize << 4))
  {
    XOffset = SourcePictureHorizontalSize << 4;
  }

  if (YOffset < 0)
  {
    YOffset = 0;
  }

  if ((U32)YOffset > (SourcePictureVerticalSize << 4))
  {
    YOffset = SourcePictureVerticalSize << 4;
  }

  /* Call to SetInputMargins. Writes the pan&scan vectors.                 */
  SetInputMargins(LayerHandle, &XOffset, &YOffset, FALSE);

  /*Params_p->InputRectangle.PositionX = XOffset >> 4;
  Params_p->InputRectangle.PositionY = YOffset >> 4;*/

#ifdef WA_GNBvd02889
    /* Due to its architecture, the block-to-row has to output pictures that are minimum 9 pixels wide.
       In case of a 720-pixels wide picture, this means that the horizontal pan value cannot be more than 711. */
    if ((XOffset >> 4) > ((S32)(SourcePictureHorizontalSize - 9)))
    {
        XOffset = (SourcePictureHorizontalSize - 9) << 4;
        HasConstraintOnPan = TRUE;
    }
#endif /* WA_GNBvd02889 */

  /*
   * Insure not to have the edge of the displayed video image falling into the
   * display window
   */
  if (((XOffset >> 4) + Params_p->InputRectangle.Width )
        > SourcePictureHorizontalSize)
  {
    Params_p->InputRectangle.Width =
            SourcePictureHorizontalSize - (XOffset >> 4) ;
  }

  if (((YOffset >> 4) + Params_p->InputRectangle.Height)
        > SourcePictureVerticalSize)
  {
    Params_p->InputRectangle.Height =
            SourcePictureVerticalSize - (YOffset >> 4);
  }

#ifdef WA_GNBvd02889
  if (HasConstraintOnPan)
  {
    Params_p->InputRectangle.PositionX = XOffset;
  }
#endif /* defined defined WA_GNBvd02889 */
} /* end of AdjustInputWindow() function */


/*******************************************************************************
Name        : AdjustOutputWindow
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AdjustOutputWindow(
                const STLAYER_Handle_t              LayerHandle,
                STLAYER_ViewPortParams_t * const    Params_p,
                const U32 * const                   HorizontalZoomFactor_p,
                const U32 * const                   VerticalZoomFactor_p)
{
U32                         WidthToDisplay;
U32                         HeightToDisplay;
HALLAYER_LayerProperties_t* LayerProperties_p;
HALLAYER_PrivateData_t*     LayerPrivateData_p;

  /* Retrieve the general HAL Layer structure from the Layer Handle.         */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
  LayerPrivateData_p =
        (HALLAYER_PrivateData_t*)(LayerProperties_p->PrivateData_p) ;

  /* Apply the zoom factors (which have found according to the hardware      */
  /* constraints on the output).                                             */
  WidthToDisplay =(Params_p->InputRectangle.Width *
        (*HorizontalZoomFactor_p)) / INT_AV_SCALE_CONSTANT;
  if (Params_p->OutputRectangle.Width > WidthToDisplay)
  {
    Params_p->OutputRectangle.Width = WidthToDisplay;
  }

  HeightToDisplay =(Params_p->InputRectangle.Height *
        (*VerticalZoomFactor_p)) / INT_AV_SCALE_CONSTANT;
  if (Params_p->OutputRectangle.Height > HeightToDisplay)
  {
    Params_p->OutputRectangle.Height = HeightToDisplay;
  }

  /* Check the ViexPort output parameters with the Layer dimensions.         */
  if ((Params_p->OutputRectangle.PositionX) <= ((S32)(LayerPrivateData_p->LayerParams.Width)) )
  {
    if (( Params_p->OutputRectangle.PositionX + Params_p->OutputRectangle.Width ) >
            LayerPrivateData_p->LayerParams.Width)
    {
        Params_p->OutputRectangle.Width =
                LayerPrivateData_p->LayerParams.Width - Params_p->OutputRectangle.PositionX;
    }
  }
  else
  {
    Params_p->OutputRectangle.Width = 0;
  }

  if ((Params_p->OutputRectangle.PositionY) <= ((S32)(LayerPrivateData_p->LayerParams.Height)) )
  {
    if ((Params_p->OutputRectangle.PositionY + Params_p->OutputRectangle.Height) >
            LayerPrivateData_p->LayerParams.Height)
    {
        Params_p->OutputRectangle.Height =
                LayerPrivateData_p->LayerParams.Height - Params_p->OutputRectangle.PositionY;
    }
  }
  else
  {
    Params_p->OutputRectangle.Height = 0;
  }

  /* Adjust with the hardware limitations */
  SetViewPortPositions(((const STLAYER_ViewPortHandle_t)LayerPrivateData_p), &(Params_p->OutputRectangle), FALSE);

} /* end of AdjustOutputWindow */


/*******************************************************************************
Name        : CenterNegativeValues
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void CenterNegativeValues(const STLAYER_Handle_t LayerHandle, STLAYER_ViewPortParams_t * const Params_p)
{
  S32 InputPositionX, OutputPositionX ;
  U32 InputWidth, OutputWidth ;
  S32 InputPositionY, OutputPositionY ;
  U32 InputHeight, OutputHeight ;
  HALLAYER_LayerProperties_t* LayerProperties_p;
  HALLAYER_PrivateData_t*     LayerPrivateData_p;

  /* Retrieve the general HAL Layer structure from the Layer Handle.         */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
  LayerPrivateData_p = LayerProperties_p->PrivateData_p;

  /* Use the same unit everywhere. */
  Params_p->InputRectangle.PositionX = Params_p->InputRectangle.PositionX / 16;
  Params_p->InputRectangle.PositionY = Params_p->InputRectangle.PositionY / 16;

  /* InputPositionX < 0*/
  if (Params_p->InputRectangle.PositionX < 0)
  {
    InputPositionX = Params_p->InputRectangle.PositionX;
    InputWidth = Params_p->InputRectangle.Width;
    OutputPositionX = Params_p->OutputRectangle.PositionX;
    OutputWidth = Params_p->OutputRectangle.Width;
    Params_p->InputRectangle.PositionX = 0;
    if ( ((S32)InputWidth) < (- InputPositionX))
    {
        Params_p->InputRectangle.Width = 0;
    }
    else
    {
        Params_p->InputRectangle.Width += InputPositionX;
    }
    Params_p->OutputRectangle.PositionX -= (InputPositionX * LayerPrivateData_p->HorizontalZoomFactor) / INT_AV_SCALE_CONSTANT;
    Params_p->OutputRectangle.Width -= (Params_p->OutputRectangle.PositionX - OutputPositionX);
  }

  /* InputPositionY < 0 */
  if (Params_p->InputRectangle.PositionY < 0)
  {
    InputPositionY = Params_p->InputRectangle.PositionY;
    InputHeight = Params_p->InputRectangle.Height;
    OutputPositionY = Params_p->OutputRectangle.PositionY;
    OutputHeight = Params_p->OutputRectangle.Height;
    Params_p->InputRectangle.PositionY = 0;
    if ( ((S32)InputHeight) < (- InputPositionY) )
    {
        Params_p->InputRectangle.Height = 0;
    }
    else
    {
        Params_p->InputRectangle.Height += InputPositionY;
    }
    Params_p->OutputRectangle.PositionY -= (InputPositionY * LayerPrivateData_p->VerticalZoomFactor) / INT_AV_SCALE_CONSTANT;
    Params_p->OutputRectangle.Height -= (Params_p->OutputRectangle.PositionY - OutputPositionY);
  }

  /* OutputPositionX < 0 */
  if (Params_p->OutputRectangle.PositionX < 0)
  {
    InputPositionX = Params_p->InputRectangle.PositionX;
    InputWidth = Params_p->InputRectangle.Width;
    OutputPositionX = Params_p->OutputRectangle.PositionX;
    OutputWidth = Params_p->OutputRectangle.Width;
    if (LayerPrivateData_p->HorizontalZoomFactor != 0) /* HG: Quick patch to avoid division by 0 ! */
    {
      Params_p->InputRectangle.PositionX = ((- OutputPositionX) * INT_AV_SCALE_CONSTANT) / LayerPrivateData_p->HorizontalZoomFactor;
    }
    else
    {
      Params_p->InputRectangle.PositionX = 0;
    }

    Params_p->InputRectangle.Width -= (Params_p->InputRectangle.PositionX - InputPositionX);
    Params_p->OutputRectangle.PositionX = 0;
    Params_p->OutputRectangle.Width += OutputPositionX ;
  }

  /* OutputPositionY < 0 */
  if (Params_p->OutputRectangle.PositionY < 0)
  {
    InputPositionY = Params_p->InputRectangle.PositionY;
    InputHeight = Params_p->InputRectangle.Height;
    OutputPositionY = Params_p->OutputRectangle.PositionY;
    OutputHeight = Params_p->OutputRectangle.Height;
    if (LayerPrivateData_p->VerticalZoomFactor != 0) /* HG: Quick patch to avoid division by 0 ! */
    {
      Params_p->InputRectangle.PositionY = ((- OutputPositionY) * INT_AV_SCALE_CONSTANT) / LayerPrivateData_p->VerticalZoomFactor;
    }
    else
    {
      Params_p->InputRectangle.PositionY = 0;
    }

    Params_p->InputRectangle.Height -= (Params_p->InputRectangle.PositionY - InputPositionY);
    Params_p->OutputRectangle.PositionY = 0;
    Params_p->OutputRectangle.Height += OutputPositionY ;
  }

  /* Convert in the good unit. */
  Params_p->InputRectangle.PositionX = Params_p->InputRectangle.PositionX * 16;
  Params_p->InputRectangle.PositionY = Params_p->InputRectangle.PositionY * 16;

}


/*******************************************************************************
Name        : Init
Description : Allocate and initialize the HAL_LAYER properties value.
Parameters  : Handle, Init parameters
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t type value :
                - ST_NO_ERROR, if no problem during init.
                - ST_ERROR_BAD_PARAMETER, if bad Init parameters.
                - ST_ERROR_NO_MEMORY, if memory allocation problem.
                - ST_ERROR_ALREADY_INITIALIZED if already initialised.
*******************************************************************************/
static ST_ErrorCode_t Init(const STLAYER_Handle_t LayerHandle, const STLAYER_InitParams_t * const LayerInitParams_p)
{
HALLAYER_LayerProperties_t* LayerProperties_p;
HALLAYER_PrivateData_t*     LayerPrivateData_p;
STLAYER_LayerParams_t*      LayerParams_p;
ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Check null pointer                               */
    if (LayerInitParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if the layer is not already initialised. */
    if (IsMainLayerInitialized)
    {
        ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Retrieve the general HAL Layer structure from the Layer Handle.         */
        LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
        /* Check null pointer                               */
        if (LayerProperties_p == NULL)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Memory allocation for our private structure. In Omega1, we have only    */
        /* 1 ViewPort and only 1 Layer.                                            */
        LayerProperties_p->PrivateData_p = (HALLAYER_PrivateData_t *)STOS_MemoryAllocate(LayerProperties_p->CPUPartition_p,sizeof(HALLAYER_PrivateData_t));
        LayerPrivateData_p = LayerProperties_p->PrivateData_p;

        /* Check if the memory allocation is successful.                           */
        if (LayerPrivateData_p == NULL)
        {
            return (ST_ERROR_NO_MEMORY);
        }

        /* Avoid null pointer for input parameter.          */
        if ( (LayerInitParams_p == NULL) || (LayerInitParams_p->LayerParams_p == NULL) )
        {
            /* De-allocate because failing                  */
            STOS_MemoryDeallocate(LayerProperties_p->CPUPartition_p, (void *) LayerPrivateData_p);
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Get the layer parameters from init. parameters.  */
        LayerParams_p = LayerInitParams_p->LayerParams_p;

        /* Default Affectation in our private structure : the ViewPort is closed.  */
        LayerPrivateData_p->Opened = FALSE;
        LayerPrivateData_p->DisplayEnableFromVideo = FALSE;

        /* Initialise hardware */
        InitialiseLayer(LayerHandle, LayerInitParams_p->LayerType);

        LayerPrivateData_p->OutputParams.XOffset = 0;
        LayerPrivateData_p->OutputParams.YOffset = 0;
        LayerPrivateData_p->OutputParams.XStart = 132;
        LayerPrivateData_p->OutputParams.YStart = 23;
        LayerPrivateData_p->OutputParams.Width = LayerParams_p->Width;
        LayerPrivateData_p->OutputParams.Height = LayerParams_p->Height;
        LayerPrivateData_p->OutputParams.XValue = LayerPrivateData_p->OutputParams.XOffset + LayerPrivateData_p->OutputParams.XStart;
        LayerPrivateData_p->OutputParams.YValue = LayerPrivateData_p->OutputParams.YOffset + LayerPrivateData_p->OutputParams.YStart;
        LayerPrivateData_p->OutputParams.DisplayEnableFromMixer = FALSE;

        LayerPrivateData_p->HorizontalZoomFactor = INT_AV_SCALE_CONSTANT;
        LayerPrivateData_p->VerticalZoomFactor  = INT_AV_SCALE_CONSTANT;

        /* Apply Layer parameters given to the functions.                          */
        LayerPrivateData_p->LayerParams = *LayerParams_p;

        /* Give default values to the private data. */
        LayerPrivateData_p->Scaling.B2RMode4 = FALSE;
        LayerPrivateData_p->Scaling.B2RMode5 = FALSE;
        LayerPrivateData_p->Scaling.B2RMode6 = FALSE;
        LayerPrivateData_p->Scaling.B2RMode89 = FALSE;
        /* Default values as in registers at reset */
        LayerPrivateData_p->Scaling.B2RVFParams.LumaHorizDown = FALSE;
        LayerPrivateData_p->Scaling.B2RVFParams.LumaInterpolate = FALSE;
        LayerPrivateData_p->Scaling.B2RVFParams.LumaZoomOut = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.LumaOffsetOdd = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.LumaOffsetEven = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.LumaIncrement = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.ChromaHorizDown = FALSE;
        LayerPrivateData_p->Scaling.B2RVFParams.ChromaInterpolate = FALSE;
        LayerPrivateData_p->Scaling.B2RVFParams.ChromaZoomOut = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.ChromaOffsetOdd = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.ChromaOffsetEven = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.ChromaIncrement = 0;
        LayerPrivateData_p->Scaling.B2RVFParams.LumaFrameBased = FALSE;
        LayerPrivateData_p->Scaling.B2RVFParams.ChromaFrameBased = FALSE;
        LayerPrivateData_p->Scaling.B2RVFParams.LumaAndChromaThrowOneLineInTwo = FALSE;
        /* Re-apply reset values to B2R register ? */
        /* Re-apply reset values to CIF bits */
        SetCIFMode(LayerHandle, FALSE, FALSE, FALSE);

        /* Init the flag to know that the layer is already initialised. */
        IsMainLayerInitialized = TRUE;
    }

    return (ErrorCode);
}


/*******************************************************************************
Name        : OpenViewport
Description : Opens a new Viewport
Parameters  : HAL Layer manager handle, Viewport params
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t OpenViewport(const STLAYER_Handle_t        LayerHandle,
                                   STLAYER_ViewPortParams_t *    const Params_p,
                                   STLAYER_ViewPortHandle_t *    ViewportHandle_p)
{
HALLAYER_LayerProperties_t *    LayerProperties_p;
HALLAYER_PrivateData_t *        LayerPrivateData_p;
STLAYER_GlobalAlpha_t           Alpha;

  /* Check null pointer                               */
  if ((ViewportHandle_p == NULL) || (Params_p == NULL))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the general HAL Layer structure from the Layer Handle.         */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
  /* Check null pointer                               */
  if (LayerProperties_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the Private Data structure from the LayerProperties structure. */
  LayerPrivateData_p = LayerProperties_p->PrivateData_p;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* If the ViewPort is opened, there is no free handle : return error.      */
  if (LayerPrivateData_p->Opened == TRUE)
  {
    return (STLAYER_ERROR_NO_FREE_HANDLES);
  }

  /* Flag to know the ViewPort is opened.                                    */
  LayerPrivateData_p->Opened = TRUE;

  /* Affectation of the ViewPort Handle.                                     */
  *ViewportHandle_p = (STLAYER_ViewPortHandle_t)LayerPrivateData_p;

  /* Viewport initialisations with the Layer Handle flag.                    */
  LayerPrivateData_p->AssociatedLayerHandle = LayerHandle;

  /* Affectations of the Viewport parameters.                                */
  LayerPrivateData_p->ViewportParams = *Params_p;
  LayerPrivateData_p->HorizontalZoomFactor = INT_AV_SCALE_CONSTANT;
  LayerPrivateData_p->VerticalZoomFactor = INT_AV_SCALE_CONSTANT;

  /* Apply current default parameters.                                       */
  SetViewportParams(*ViewportHandle_p, Params_p);

  Alpha.A0 = 128;
  SetViewportAlpha(*ViewportHandle_p, &Alpha);

  return (ST_NO_ERROR);
}


/*******************************************************************************
Name        : CloseViewport
Description :
Parameters  : HAL Viewport manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t CloseViewport(const STLAYER_ViewPortHandle_t ViewportHandle)
{
HALLAYER_PrivateData_t *        LayerPrivateData_p;

  /* Retrieve the LayerPrivateData structure from the ViewPort Handle.       */
  LayerPrivateData_p = (HALLAYER_PrivateData_t*)ViewportHandle;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Close the ViewPort if opened, returns error otherwise.                  */
  if (LayerPrivateData_p->Opened == TRUE)
  {
    LayerPrivateData_p->Opened = FALSE;
  }
  else
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CloseViewport : No ViewPort opened."));
    return (STLAYER_ERROR_NO_FREE_HANDLES);
  }

  return (ST_NO_ERROR);
}


/*******************************************************************************
Name        :
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetCapability(const STLAYER_Handle_t        LayerHandle,
                                STLAYER_Capability_t *  Capability_p)
{
    HALLAYER_LayerProperties_t *  LayerProperties_p;

    /* Check null pointer                               */
    if (Capability_p == NULL)
    {
      return;
    }

    /* init Layer properties */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    /* Check null pointer                               */
    if (LayerProperties_p == NULL)
    {
      return;
    }

    Capability_p->MultipleViewPorts = FALSE;
    Capability_p->HorizontalResizing = TRUE;
    Capability_p->AlphaBorder = FALSE;
    Capability_p->GlobalAlpha = TRUE;
    Capability_p->ColorKeying = FALSE;
    Capability_p->MultipleViewPortsOnScanLineCapable = FALSE;
    Capability_p->LayerType = LayerProperties_p->LayerType;
}


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
static ST_ErrorCode_t Term(const STLAYER_Handle_t          LayerHandle,
                           const STLAYER_TermParams_t *    TermParams_p)

{
STLAYER_Layer_t             LayerType;
HALLAYER_PrivateData_t*     LayerPrivateData_p;
HALLAYER_LayerProperties_t *  LayerProperties_p;

  /* Check null pointer                               */
  if (TermParams_p == NULL)
  {
     return(ST_ERROR_BAD_PARAMETER);
  }

  /* init Layer properties */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
  /* Check null pointer                               */
  if (LayerProperties_p == NULL)
  {
     return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the general HAL Layer structure from the Layer Handle.         */
  LayerPrivateData_p = ((HALLAYER_LayerProperties_t*)(LayerHandle))->PrivateData_p ;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
     return(ST_ERROR_BAD_PARAMETER);
  }

  LayerType = LayerProperties_p->LayerType;

  /* Check if the layer has already been initialised */
  if ((LayerType == STLAYER_OMEGA1_VIDEO) && (!IsMainLayerInitialized))
  {
    return (ST_ERROR_UNKNOWN_DEVICE);
  }

  if (TermParams_p->ForceTerminate == TRUE)
  {
    /* Force the close of the ViewPort.                                      */
    LayerPrivateData_p->Opened = FALSE; /* identical as a call to the CloseViewport function. */
  }
  else
  {
    if (LayerPrivateData_p->Opened)
    {
        return(ST_ERROR_OPEN_HANDLE);
    }
  }

  /* De-allocate last : data inside cannot be used after !                   */
  STOS_MemoryDeallocate(((HALLAYER_LayerProperties_t*)(LayerHandle))->CPUPartition_p, (void *) LayerPrivateData_p);

  /* Update the falg to autorize or not to init the layer an other time. */
  IsMainLayerInitialized = FALSE;

  return(ST_NO_ERROR);

}


/*******************************************************************************
Name        : EnableVideoViewPort
Description : Enable video Layer
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnableVideoViewport(const STLAYER_ViewPortHandle_t   ViewportHandle)
{
HALLAYER_LayerProperties_t * LayerProperties_p;
HALLAYER_PrivateData_t *  LayerPrivateData_p;

  /* Check null pointer                               */
  if (((HALLAYER_PrivateData_t *)(ViewportHandle)) == NULL)
  {
     return;
  }

  /* Retrieve the Layer Handle from the ViewPort Handle.                     */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_PrivateData_t *)(ViewportHandle))->AssociatedLayerHandle);
  /* Check null pointer                               */
  if (LayerProperties_p == NULL)
  {
     return;
  }

  /* Retrieve the Private Data structure from the LayerProperties structure. */
  LayerPrivateData_p = LayerProperties_p->PrivateData_p;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
     return;
  }

  /* We store that we have been called. */
  LayerPrivateData_p->DisplayEnableFromVideo = TRUE;

  if (LayerPrivateData_p->OutputParams.DisplayEnableFromMixer)
  {
    switch  (LayerProperties_p->LayerType)
    {
        case STLAYER_OMEGA1_VIDEO :
        /* Must protect the access to VID_DCF_16 due to OSD/VID concurrent     */
        /* accesses. */
        if (((LayerPrivateData_p->ViewportParams.OutputRectangle.Width)!=0)
                    &&((LayerPrivateData_p->ViewportParams.OutputRectangle.Height)!=0))
        {
            interrupt_lock();

            /* Register writing.                                                   */
            HAL_SetRegister16Value((U8 *)
                LayerProperties_p->VideoDisplayBaseAddress_p + VID_DCF_16 ,
                VID_DCF_MASK,
                VID_DCF_EVD_MASK,
                VID_DCF_EVD_EMP,
                HAL_ENABLE);

            /* Disable the hardware protection.                                    */
            interrupt_unlock();
        }
        break;

        default :
            #ifdef DEBUG
            HALLAYER_Error();
            #endif
            break;
    }
  }
}


/******************************************************************************
Name        : DisableVideoViewPort
Description : Disable video display
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
******************************************************************************/
static void DisableVideoViewport(const STLAYER_ViewPortHandle_t   ViewportHandle)
{
HALLAYER_LayerProperties_t * LayerProperties_p;
HALLAYER_PrivateData_t *  LayerPrivateData_p;

  /* Check null pointer                               */
  if (((HALLAYER_PrivateData_t *)(ViewportHandle)) == NULL)
  {
     return;
  }

  /* Retrieve the Layer Handle from the ViewPort Handle.                     */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)(((HALLAYER_PrivateData_t *)(ViewportHandle))->AssociatedLayerHandle);
  /* Check null pointer                               */
  if (LayerProperties_p == NULL)
  {
     return;
  }

  /* Retrieve the Private Data structure from the LayerProperties structure. */
  LayerPrivateData_p = LayerProperties_p->PrivateData_p;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
     return;
  }

  /* We store that we have been called. */
  LayerPrivateData_p->DisplayEnableFromVideo = FALSE;

  switch  (LayerProperties_p->LayerType)
  {
    case STLAYER_OMEGA1_VIDEO :
    /* Must protect the access to VID_DCF_16 due to OSD/VID concurrent     */
    /* accesses. */
    interrupt_lock();

    /* Register writing.                                                   */
    HAL_SetRegister16Value((U8 *)
            LayerProperties_p->VideoDisplayBaseAddress_p + VID_DCF_16 ,
            VID_DCF_MASK,
            VID_DCF_EVD_MASK,
            VID_DCF_EVD_EMP,
            HAL_DISABLE);

    /* Disable the hardware protection.                                    */
    interrupt_unlock();
    break;

    default :
    #ifdef DEBUG
        HALLAYER_Error();
    #endif
    break;
  }
}


/*******************************************************************************
Name        : SetViewportPriority
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetViewportPriority(
        const STLAYER_ViewPortHandle_t      ViewportHandle,
        const HALLAYER_ViewportPriority_t   Priority)
{

#ifdef DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No sense for Omega1 cells."));
#endif /* DEBUG */
}


/*******************************************************************************
Name        : SetViewportSource
Description :
Parameters  : LayerHandle,
              ViewportId
              SourceNumber
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetViewportSource(const STLAYER_ViewPortHandle_t   ViewportHandle,
                              const STLAYER_ViewPortSource_t * Source_p)
{
#ifdef DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No sense for Omega1 cells."));
#endif /* DEBUG */

return (ST_ERROR_FEATURE_NOT_SUPPORTED);
}


/*******************************************************************************
Name        : GetViewportSource
Description :
Parameters  : LayerHandle,
              ViewportId
              Source
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetViewportSource(const STLAYER_ViewPortHandle_t   ViewportHandle,
                              U32                 *            SourceNumber_p)
{
#ifdef DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No sense for Omega1 cells."));
#endif /* DEBUG */
}

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
    /* Not supported. */
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
    /* Not supported. */
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

} /* End of GetViewportSpecialMode() function. */

/*******************************************************************************
Name        : SetLayerParams
Description : Sets Layer parameters
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetLayerParams(const STLAYER_Handle_t LayerHandle, STLAYER_LayerParams_t * const LayerParams_p)
{
HALLAYER_PrivateData_t*     LayerPrivateData_p;
STLAYER_UpdateParams_t      UpdateParams;
STLAYER_LayerParams_t       LayerParams;

  /* Check the layer is initialized. */
  /*if(Initialised == FALSE)
  {
  } */

  /* Check the pointer is not null. */
  if(LayerParams_p == NULL)
  {
#ifdef DEBUG
     HALLAYER_Error();
#endif /* DEBUG */
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Null pointer !"));
  }
  else
  {
    /* Retrieve the general HAL Layer structure from the Layer Handle.         */
    LayerPrivateData_p = ((HALLAYER_LayerProperties_t*)(LayerHandle))->PrivateData_p ;
    if (LayerPrivateData_p == NULL)
    {
        return;
    }

    /* Check the layer parameters are included inside the output parameters. */
    if((LayerParams_p->Width > LayerPrivateData_p->OutputParams.Width) ||
       (LayerParams_p->Height > LayerPrivateData_p->OutputParams.Height))
    {
#ifdef DEBUG
       HALLAYER_Error();
#endif /* DEBUG */
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Bad parameters !"));
    }
    else
    {
        /* Store the Layer Parameters in our local structure.                      */
        if ((LayerPrivateData_p->LayerParams.AspectRatio  != LayerParams_p->AspectRatio) ||
            (LayerPrivateData_p->LayerParams.ScanType     != LayerParams_p->ScanType) ||
            (LayerPrivateData_p->LayerParams.Width        != LayerParams_p->Width) ||
            (LayerPrivateData_p->LayerParams.Height       != LayerParams_p->Height))
        {
            /*LayerPrivateData_p->LayerParams = *LayerParams_p;*/
            LayerPrivateData_p->LayerParams.AspectRatio = LayerParams_p->AspectRatio;
            LayerPrivateData_p->LayerParams.ScanType    = LayerParams_p->ScanType;
            LayerPrivateData_p->LayerParams.Width       = LayerParams_p->Width;
            LayerPrivateData_p->LayerParams.Height      = LayerParams_p->Height;

            /* Notify event HALLAYER_UPDATE_PARAMS_EVT_ID */
            UpdateParams.LayerParams_p              = &LayerParams;
            UpdateParams.LayerParams_p->AspectRatio = LayerParams_p->AspectRatio;
            UpdateParams.LayerParams_p->ScanType    = LayerParams_p->ScanType;
            UpdateParams.LayerParams_p->Width       = LayerParams_p->Width;
            UpdateParams.LayerParams_p->Height      = LayerParams_p->Height;
            UpdateParams.UpdateReason               = STLAYER_LAYER_PARAMS_REASON;
            STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle, ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID], (const void *) (&UpdateParams));
        }
    }
  }
}


/*******************************************************************************
Name        : GetLayerParams
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetLayerParams(const STLAYER_Handle_t  LayerHandle,
                           STLAYER_LayerParams_t*  LayerParams_p)
{
HALLAYER_LayerProperties_t* LayerProperties_p;
HALLAYER_PrivateData_t*     LayerPrivateData_p;

  /* Retrieve the general HAL Layer structure from the Layer Handle.         */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
  if (LayerProperties_p == NULL)
  {
    return;
  }

  LayerPrivateData_p = (HALLAYER_PrivateData_t*)(LayerProperties_p->PrivateData_p) ;
  if (LayerPrivateData_p == NULL)
  {
    return;
  }

  /* Restore the Layer Parameters from our local structure.                  */
  *LayerParams_p = LayerPrivateData_p->LayerParams;
}


/*******************************************************************************
Name        : AdjustViewportParams
Description : Adjust the Parameters of the Viewport.
Parameters  : LayerHandle
              Params
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t AdjustViewportParams(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_ViewPortParams_t * const   Params_p)
{
U32 HorizontalZoomFactor;
U32 VerticalZoomFactor;
HALLAYER_LayerProperties_t* LayerProperties_p;
HALLAYER_PrivateData_t*     LayerPrivateData_p;

  /* Check the pointer is not null. */
  if (Params_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the general HAL Layer structure from the Layer Handle.         */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
  if (LayerProperties_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  LayerPrivateData_p = (HALLAYER_PrivateData_t*)(LayerProperties_p->PrivateData_p) ;
  if (LayerPrivateData_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* The HorizontalZoomFactor and VerticalZoomFactor variables only used to be updated
   * when something used in the zoom calculation functions AdjustHorizontalFactor and
   * AdjustVerticalFactor had changed. However, it was possible to have the same parameters
   * when the actual zoom factor was modified (because of adjustments in viewport parameters). */
  AdjustHorizontalFactor( LayerHandle, Params_p , &HorizontalZoomFactor);
  AdjustVerticalFactor( LayerHandle, Params_p , &VerticalZoomFactor);
  LayerPrivateData_p->HorizontalZoomFactor = HorizontalZoomFactor;
  LayerPrivateData_p->VerticalZoomFactor = VerticalZoomFactor;

  /* Re-align the negative values for (xi,yi,xo,yo).                         */
  CenterNegativeValues(LayerHandle, Params_p);

  /* Adjust the Input according to the hardware constraints.                 */
  AdjustInputWindow(LayerHandle, Params_p, HorizontalZoomFactor);

  /* Adjust the Output according to the hardware constraints.                */
  AdjustOutputWindow(LayerHandle, Params_p, &HorizontalZoomFactor, &VerticalZoomFactor);
  WriteB2RRegisters(LayerHandle);

  return(ST_NO_ERROR);
} /* end of AdjustViewportParams() function */


/*******************************************************************************
Name        : SetViewportParams
Description : Apply the ViewPort parameters.
Parameters  : ViewportHandle
              Params
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t SetViewportParams(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p)
{
STLAYER_Handle_t        LayerHandle;
HALLAYER_PrivateData_t* LayerPrivateData_p;
U32                     HorizontalZoomFactor;
U32                     VerticalZoomFactor;
BOOL                    Progressive;
STGXOBJ_ScanType_t      ScanType;
#if defined(HW_5508) || defined(HW_5518) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578)
HALLAYER_ComputeB2RVerticalFiltersInputParams_t ComputeB2RVerticalFiltersInputParams;
HALLAYER_B2RVerticalFiltersParams_t             B2RVerticalFiltersParams;
#endif

  /* Check the pointer is not null. */
  if (Params_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the LayerPrivateData structure from the ViewPort Handle.       */
  LayerPrivateData_p = (HALLAYER_PrivateData_t*)ViewportHandle;
  if (LayerPrivateData_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the Layer Handle from the ViewPort Handle.                     */
  LayerHandle = ((HALLAYER_PrivateData_t *)(ViewportHandle))->AssociatedLayerHandle;
  if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  switch  (((HALLAYER_LayerProperties_t *)LayerHandle)->LayerType)
  {
    case STLAYER_OMEGA1_VIDEO :
      /* Set the applicable scalings according to the hardware constraints.  */
      /* The calculation has been done in the AdjustX... functions.          */
      HorizontalZoomFactor = LayerPrivateData_p->HorizontalZoomFactor;
      VerticalZoomFactor = LayerPrivateData_p->VerticalZoomFactor;
      ScanType    = ((Params_p->Source_p)->Data.VideoStream_p)->ScanType;
      if (ScanType == STGXOBJ_PROGRESSIVE_SCAN)
      {
        Progressive = TRUE;
      }
      else
      {
        Progressive = FALSE;
      }
    if ( (LayerPrivateData_p->ViewportParams.InputRectangle.Width !=
    Params_p->InputRectangle.Width)  ||
    (LayerPrivateData_p->ViewportParams.InputRectangle.Height !=
    Params_p->InputRectangle.Height) ||
    (LayerPrivateData_p->ViewportParams.OutputRectangle.Width !=
    Params_p->OutputRectangle.Width)  ||
    (LayerPrivateData_p->ViewportParams.OutputRectangle.Height !=
    Params_p->OutputRectangle.Height) ||
    ((LayerPrivateData_p->LayerParams.ScanType) != ScanType))
    {
      ApplyHorizontalFactor( LayerHandle,
            HorizontalZoomFactor, VerticalZoomFactor);
      ApplyVerticalFactor( LayerHandle, Progressive,
            HorizontalZoomFactor , VerticalZoomFactor);
    }

    /* Re-align the negative values for (xi,yi,xo,yo).                         */
    CenterNegativeValues(LayerHandle, Params_p);

    /* Set the Input according to the hardware constraints.                */
    AdjustInputWindow(LayerHandle, Params_p, HorizontalZoomFactor);

    /* Set the Output according to the hardware constraints.               */
    AdjustOutputWindow(LayerHandle, Params_p, &HorizontalZoomFactor, &VerticalZoomFactor);

#if defined(HW_5508) || defined(HW_5518) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578)
    /* Adjust viewport height if lines are missing ! */

    if (VerticalZoomFactor == INT_AV_SCALE_CONSTANT)
    {
        /* Treat particular case of full screen with direct values... */
        B2RVerticalFiltersParams.LumaInterpolate = TRUE;
        B2RVerticalFiltersParams.LumaZoomOut = 0;
        B2RVerticalFiltersParams.ChromaInterpolate = TRUE;
        B2RVerticalFiltersParams.ChromaZoomOut = 0;
        if ((((Params_p->Source_p)->Data).VideoStream_p)->ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        {
            /* Progressive */
            B2RVerticalFiltersParams.LumaOffsetOdd = 1536;
            B2RVerticalFiltersParams.LumaOffsetEven = 1024;
            B2RVerticalFiltersParams.LumaIncrement = 1023;
            B2RVerticalFiltersParams.ChromaOffsetOdd = 640;
            B2RVerticalFiltersParams.ChromaOffsetEven = 384;
            B2RVerticalFiltersParams.ChromaIncrement = 511;
            B2RVerticalFiltersParams.LumaFrameBased = TRUE;
            B2RVerticalFiltersParams.ChromaFrameBased = TRUE;
        }
        else
        {
            /* Interlaced */
            B2RVerticalFiltersParams.LumaOffsetOdd = 512;
            B2RVerticalFiltersParams.LumaOffsetEven = 512;
            B2RVerticalFiltersParams.LumaIncrement = 511;
            B2RVerticalFiltersParams.ChromaOffsetOdd = 64;
            B2RVerticalFiltersParams.ChromaOffsetEven = 192;
            B2RVerticalFiltersParams.ChromaIncrement = 255;
            B2RVerticalFiltersParams.LumaFrameBased = FALSE;
            B2RVerticalFiltersParams.ChromaFrameBased = FALSE;
        }
        if ((HorizontalZoomFactor != 0) && (HorizontalZoomFactor < (INT_AV_SCALE_CONSTANT / 2)))
        {
            B2RVerticalFiltersParams.LumaHorizDown = TRUE;
            B2RVerticalFiltersParams.ChromaHorizDown =TRUE;
        }
        else
        {
            B2RVerticalFiltersParams.LumaHorizDown = FALSE;
            B2RVerticalFiltersParams.ChromaHorizDown = FALSE;
        }
        B2RVerticalFiltersParams.LumaAndChromaThrowOneLineInTwo = FALSE;
    }
    else
    {
        /* Standard algorithm for computation of B2R parameters */
        if ((((Params_p->Source_p)->Data).VideoStream_p)->ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        {
            ComputeB2RVerticalFiltersInputParams.ProgressiveSource = TRUE;
        }
        else
        {
            ComputeB2RVerticalFiltersInputParams.ProgressiveSource = FALSE;
        }
        ComputeB2RVerticalFiltersInputParams.IsFreezing = FALSE; /* !!! */
        ComputeB2RVerticalFiltersInputParams.VerticalZoomFactor = VerticalZoomFactor;
        ComputeB2RVerticalFiltersInputParams.HorizontalZoomFactor = HorizontalZoomFactor;
        ComputeB2RVerticalFilters(&ComputeB2RVerticalFiltersInputParams, &B2RVerticalFiltersParams);
    }

    /* Calculate the number of lines lost to keep a perfect luma-chroma alignment. */
    ComputeTheNumberOfLinesRemoved(&B2RVerticalFiltersParams,
            &(Params_p->InputRectangle.Height),
            Params_p->InputRectangle.PositionY,
            &(Params_p->OutputRectangle.Height));
#endif /* defined(HW_5508) || defined(HW_5518) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578) */

    /* Store the Viewport params applied.                                               */
    LayerPrivateData_p->LayerParams.ScanType = ScanType ;
    LayerPrivateData_p->ViewportParams = *Params_p;

    /* Apply the ViewPort Dimensions.                                      */
    SetViewPortPositions(ViewportHandle, &(Params_p->OutputRectangle), TRUE);
      break;

    default :
       #ifdef DEBUG
         HALLAYER_Error();
       #endif
       break;
  }
  WriteB2RRegisters(LayerHandle);
  return(ST_NO_ERROR);
} /* End of SetViewportParams() function */

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
    /* PSI not supported on Omega1.     */
    return (ST_ERROR_FEATURE_NOT_SUPPORTED);

} /* End of SetViewportPSI() function. */

/*******************************************************************************
Name        : GetViewportParams
Description : Retrieve the ViewPort parameters.
Parameters  : ViewportHandle
              Params
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetViewportParams(
                         const STLAYER_ViewPortHandle_t     ViewportHandle,
                         STLAYER_ViewPortParams_t * const   Params_p)
{
STLAYER_Handle_t        LayerHandle;
HALLAYER_PrivateData_t* LayerPrivateData_p;

  /* Check the pointer is not null. */
  if (Params_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the LayerPrivateData structure from the ViewPort Handle.       */
  LayerPrivateData_p = (HALLAYER_PrivateData_t*)ViewportHandle;
  if (LayerPrivateData_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the Layer Handle from the ViewPort Handle.                     */
  LayerHandle = ((HALLAYER_PrivateData_t *)(ViewportHandle))->AssociatedLayerHandle;
  if (((HALLAYER_LayerProperties_t *)LayerHandle) == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }


  switch  (((HALLAYER_LayerProperties_t *)LayerHandle)->LayerType)
  {
    case STLAYER_OMEGA1_VIDEO :
      /* Retrieve the Viewport params applied.                               */
      *Params_p = LayerPrivateData_p->ViewportParams ;
    break;

    default :
       #ifdef DEBUG
       HALLAYER_Error();
       #endif
       break;
  }
  return(ST_NO_ERROR);

}

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

    /* PSI not supported on Omega1.     */
    return (ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of GetViewportPSI() function. */

/*******************************************************************************
Name        : GetViewportIORectangle
Description : Retrieve the ViewPort IO Rectangle.
Parameters  : ViewportHandle
              IO rectangles
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetViewportIORectangle(const STLAYER_ViewPortHandle_t     ViewportHandle,
                                             STGXOBJ_Rectangle_t * InputRectangle_p,
                                             STGXOBJ_Rectangle_t * OutputRectangle_p)
{
STLAYER_Handle_t        LayerHandle;
HALLAYER_PrivateData_t* LayerPrivateData_p;

  /* Check the pointer is not null. */
  if ((InputRectangle_p == NULL) || (OutputRectangle_p == NULL))
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the LayerPrivateData structure from the ViewPort Handle.       */
  LayerPrivateData_p = (HALLAYER_PrivateData_t*)ViewportHandle;
  /* Check the pointer is not null. */
  if (LayerPrivateData_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  /* Retrieve the Layer Handle from the ViewPort Handle.                     */
  LayerHandle = ((HALLAYER_PrivateData_t *)(ViewportHandle))->AssociatedLayerHandle;
  /* Check the pointer is not null. */
  if ((HALLAYER_LayerProperties_t *)LayerHandle == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  switch  (((HALLAYER_LayerProperties_t *)LayerHandle)->LayerType)
  {
    case STLAYER_OMEGA1_VIDEO :
      *InputRectangle_p     = LayerPrivateData_p->ViewportParams.InputRectangle ;
      *OutputRectangle_p    = LayerPrivateData_p->ViewportParams.OutputRectangle ;
    break;

    default :
       #ifdef DEBUG
       HALLAYER_Error();
       #endif
       break;
  }
  return(ST_NO_ERROR);

}


/*******************************************************************************
Name        : GetViewportAlpha
Description : Gets the mixweight
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GetViewportAlpha(const STLAYER_ViewPortHandle_t     ViewportHandle,
                             STLAYER_GlobalAlpha_t*             Alpha)
{
HALLAYER_PrivateData_t* LayerPrivateData_p;

  /* Check null pointer                               */
  if (Alpha == NULL)
  {
     return;
  }

  /* Retrieve the LayerPrivateData structure from the ViewPort Handle.       */
  LayerPrivateData_p = (HALLAYER_PrivateData_t*)ViewportHandle;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
     return;
  }

  Alpha->A0 = LayerPrivateData_p->AlphaA0;
}


/*******************************************************************************
Name        : SetViewportAlpha
Description : Sets the mixweight
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetViewportAlpha(const STLAYER_ViewPortHandle_t     ViewportHandle,
                             STLAYER_GlobalAlpha_t*             Alpha)
{
STLAYER_Handle_t        LayerHandle;
HALLAYER_PrivateData_t* LayerPrivateData_p;
U32                     MixWeight;

  /* Check null pointer                               */
  if (Alpha == NULL)
  {
     return;
  }

  /* Retrieve the LayerPrivateData structure from the ViewPort Handle.       */
  LayerPrivateData_p = (HALLAYER_PrivateData_t*)ViewportHandle;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
     return;
  }

  LayerHandle = ((HALLAYER_PrivateData_t *)(ViewportHandle))->AssociatedLayerHandle;
  /* Check null pointer                               */
  if ((HALLAYER_PrivateData_t *)(ViewportHandle) == NULL)
  {
     return;
  }


  /* Prepare MixWeight value : The range of A0 is 0..128. The range of the register is 255..0 */
  MixWeight = Alpha->A0 * 2;
  if (MixWeight !=0)
  {
      MixWeight -=1;
  }
  MixWeight = 255 - MixWeight;

  /* Mix Weight Still/video */
  HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_MWSV_8 ,
            (MixWeight & VID_MWSV_MASK));

  /* Mix Weight Background/video */
  HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_MWV_8 ,
            (MixWeight & VID_MWV_MASK));

  LayerPrivateData_p->AlphaA0 = Alpha->A0;
}


/*******************************************************************************
Name        : SetViewportPosition
Description : Sets a source position
Parameters  : Display
              ViewportId
              XPos            the horizontal position of this source
              YPos            the vertical position of this source
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetViewportPosition(const STLAYER_ViewPortHandle_t ViewportHandle,
                         const U32                      XPos,
                         const U32                      YPos)
{
#ifdef DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No sense for Omega1 cells."));
#endif /* DEBUG */
}


/*******************************************************************************
Name        : GetViewportPosition
Description : Sets a source position
Parameters  : Display
              ViewportId
              XPos            the horizontal position of this source
              YPos            the vertical position of this source
Assumptions :
Limitations :
*******************************************************************************/
static void GetViewportPosition(
                         const STLAYER_ViewPortHandle_t      ViewportHandle,
                         U32 *                               XPos,
                         U32 *                               YPos)
{
#ifdef DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No sense for Omega1 cells."));
#endif /* DEBUG */
}


/*******************************************************************************
Name        : EnableSRC
Description : Enables Horizontal Scaling from the SRC
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void EnableHorizontalScaling(const STLAYER_Handle_t   LayerHandle)
{

  /* Must protect the access to VID_DCF due to OSD/VID concurrent accesses.  */
  interrupt_lock();

  /* Register writing.                                                       */
  HAL_SetRegister16Value((U8 *)(
       (HALLAYER_LayerProperties_t *)LayerHandle)->
       VideoDisplayBaseAddress_p + VID_DCF_16 ,
       VID_DCF_MASK,
       VID_DCF_DSR_MASK,
       VID_DCF_DSR_EMP,
       HAL_DISABLE);

  /* Disable the hardware protection.                                        */
  interrupt_unlock();
}


/*******************************************************************************
Name        :
Description : Disables Horizontal Scaling from the SRC
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void DisableHorizontalScaling(const STLAYER_Handle_t   LayerHandle)
{
  /* Must protect the access to VID_DCF due to OSD/VID concurrent accesses.  */
  interrupt_lock();

  /* Register writing.                                                       */
  HAL_SetRegister16Value((U8 *)(
       (HALLAYER_LayerProperties_t *)LayerHandle)->
       VideoDisplayBaseAddress_p + VID_DCF_16 ,
       VID_DCF_MASK,
       VID_DCF_DSR_MASK,
       VID_DCF_DSR_EMP,
       HAL_ENABLE);

  /* Disable the hardware protection.                                        */
  interrupt_unlock();
}


/*******************************************************************************
Name        : UpdateFromMixer
Description : Update parameters from mixer called by STMIXER.
Parameters  : ViewportHandle
              Params
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void UpdateFromMixer(
                         const STLAYER_Handle_t             LayerHandle,
                         STLAYER_OutputParams_t * const     OutputParams_p)
{
HALLAYER_LayerProperties_t* LayerProperties_p;
HALLAYER_PrivateData_t*     LayerPrivateData_p;
STLAYER_LayerParams_t       LayerParams;
STLAYER_UpdateParams_t      UpdateParams;

  /* Check null pointer                               */
  if (OutputParams_p == NULL)
  {
     return;
  }

  /* Retrieve the general HAL Layer structure from the Layer Handle.         */
  LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
  /* Check null pointer                               */
  if (LayerProperties_p == NULL)
  {
     return;
  }

  LayerPrivateData_p = (HALLAYER_PrivateData_t*)(LayerProperties_p->PrivateData_p) ;
  /* Check null pointer                               */
  if (LayerPrivateData_p == NULL)
  {
     return;
  }

  /* Retrieve the screen parameters. */
  if ((OutputParams_p->UpdateReason & STLAYER_OFFSET_REASON) != 0)
  {
    LayerPrivateData_p->OutputParams.XOffset = OutputParams_p->XOffset;
    LayerPrivateData_p->OutputParams.YOffset = OutputParams_p->YOffset;

    /* Store the output parameters for the Viewport.                           */
    LayerPrivateData_p->OutputParams.XValue = LayerPrivateData_p->OutputParams.XOffset + LayerPrivateData_p->OutputParams.XStart;
    LayerPrivateData_p->OutputParams.YValue = LayerPrivateData_p->OutputParams.YOffset + LayerPrivateData_p->OutputParams.YStart;

    /* This change need also the re-calculation of all. */
    UpdateParams.LayerParams_p = &LayerParams;
    GetLayerParams(LayerHandle, UpdateParams.LayerParams_p);
    /*UpdateParams.UpdateReason = STLAYER_OFFSET_REASON;*/
    UpdateParams.UpdateReason = STLAYER_LAYER_PARAMS_REASON;
    STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle, ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID], (const void *) (&UpdateParams));
  }
  if ((OutputParams_p->UpdateReason & STLAYER_SCREEN_PARAMS_REASON) != 0)
  {
    LayerPrivateData_p->OutputParams.XStart  = OutputParams_p->XStart;
    LayerPrivateData_p->OutputParams.YStart  = 2 * OutputParams_p->YStart; /* Top + Bot: x2 */
    LayerPrivateData_p->OutputParams.Width   = OutputParams_p->Width;
    LayerPrivateData_p->OutputParams.Height  = OutputParams_p->Height;
    LayerPrivateData_p->OutputParams.FrameRate=OutputParams_p->FrameRate;

    /* Store the output parameters for the Viewport.                           */
    LayerPrivateData_p->OutputParams.XValue = LayerPrivateData_p->OutputParams.XOffset + LayerPrivateData_p->OutputParams.XStart;
    LayerPrivateData_p->OutputParams.YValue = LayerPrivateData_p->OutputParams.YOffset + LayerPrivateData_p->OutputParams.YStart;

    /* Store the new Layer parameters given to the functions.                  */
    LayerPrivateData_p->LayerParams.AspectRatio = OutputParams_p->AspectRatio ;
    LayerPrivateData_p->LayerParams.ScanType    = OutputParams_p->ScanType;
    LayerPrivateData_p->LayerParams.Width       = OutputParams_p->Width;
    LayerPrivateData_p->LayerParams.Height      = OutputParams_p->Height;

    /* This change need also the re-calculation of all and an update the layer parameters dynamically. */
    LayerParams.Width       = OutputParams_p->Width;
    LayerParams.Height      = OutputParams_p->Height;
    LayerParams.ScanType    = OutputParams_p->ScanType;
    LayerParams.AspectRatio = OutputParams_p->AspectRatio;
    /*UpdateParams.UpdateReason   = STLAYER_SCREEN_PARAMS_REASON;*/
    UpdateParams.UpdateReason = STLAYER_LAYER_PARAMS_REASON;
    UpdateParams.LayerParams_p = &LayerParams; /* update the pointer to avoid a NULL pointer. */
    STEVT_Notify(((HALLAYER_LayerProperties_t *) LayerHandle)->EventsHandle, ((HALLAYER_LayerProperties_t *) LayerHandle)->RegisteredEventsID[HALLAYER_UPDATE_PARAMS_EVT_ID], (const void *) (&UpdateParams));

    /* Advise the lower level that the screen parameters changed.   */
    UpdateScreenParams (LayerHandle, OutputParams_p);
  }
  if ((OutputParams_p->UpdateReason & STLAYER_VTG_REASON) != 0)
  {
    /* Done upstaire !!! */
  }
  if ((OutputParams_p->UpdateReason & STLAYER_CHANGE_ID_REASON) != 0)
  {
  }
  if ((OutputParams_p->UpdateReason & STLAYER_DISCONNECT_REASON) != 0)
  {
  }

  /* The reason of the call is an enable or a disable of the display. */
  if ((OutputParams_p->UpdateReason & STLAYER_DISPLAY_REASON) != 0)
  {
    LayerPrivateData_p->OutputParams.DisplayEnableFromMixer = OutputParams_p->DisplayEnable;

    if ((LayerPrivateData_p->OutputParams.DisplayEnableFromMixer) &&
        (LayerPrivateData_p->DisplayEnableFromVideo))
    {
        EnableVideoViewport((const STLAYER_ViewPortHandle_t)LayerPrivateData_p);
    }
    else
    {
        if (LayerPrivateData_p->DisplayEnableFromVideo)
        {
            DisableVideoViewport((const STLAYER_ViewPortHandle_t)LayerPrivateData_p);

            /* Be careful : The flag stays true as it is from a video point of view. */
            LayerPrivateData_p->DisplayEnableFromVideo = TRUE;
        }
        /* else if !LayerPrivateData_p->DisplayEnableFromVideo or Viewport not opened : nothing to do. */
    }
  }

} /* end of UpdateFromMixer() */

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
    /* no update needs to be done on Vsync for this release */
    return(FALSE);
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
    /* no update needs to be done on Vsync for this release */

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
    UNUSED_PARAMETER(ViewportHandle);
    UNUSED_PARAMETER(RequestedMode_p);

    /* Feature not implemented on Omega1 */
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

    /* Feature not implemented on Omega1 */
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

    /* Feature not implemented on OMEGA1 */
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

    /* Feature not implemented on OMEGA1 */
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

    /* Feature not implemented on OMEGA1 */
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

    /* Feature not implemented on OMEGA1 */
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}


/*******************************************************************************
Name        : HALLAYER_Error
Description : Function called every time an error occurs. A breakpoint here
              should facilitate the debug
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef DEBUG
void HALLAYER_Error(void)
{
    while (1);
}
#endif


#if defined(HW_5508) || defined(HW_5518) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578)
/*******************************************************************************
Name        : ComputeB2RVerticalFilters
Description : Calculate values for B2R vertical filters
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void ComputeB2RVerticalFilters(const HALLAYER_ComputeB2RVerticalFiltersInputParams_t * const InputParams_p,
                                     HALLAYER_B2RVerticalFiltersParams_t * const OutputParams_p)
{
    U16 OffsetEven, OffsetOdd;
    U32 VerticalScalingValue;
    U32 AdaptedScalingValue, LineOffset;
    S32 SignedLumaOffsetOdd, SignedChromaOffsetOdd, SignedLumaOffsetEven, SignedChromaOffsetEven;

    if (InputParams_p->ProgressiveSource)
    {
        /* Progressive */
        OffsetEven = 1024;
        OffsetOdd = 1024;
    }
    else
    {
        /* Intertlaced */
        OffsetEven = 512;
        OffsetOdd = 512;
    }

    OutputParams_p->LumaAndChromaThrowOneLineInTwo = FALSE;
    if (InputParams_p->VerticalZoomFactor == 0)
    {
        /* To avoid division by 0 */
        VerticalScalingValue = 1024;
    }
    else if (InputParams_p->VerticalZoomFactor < (INT_AV_SCALE_CONSTANT / 2))
    {
        /* For zoom-out more than 2, throw lines away (1 out of 2), so divide zoom-out by 2 */
        VerticalScalingValue = (1024 * INT_AV_SCALE_CONSTANT) / InputParams_p->VerticalZoomFactor / 2;
        OutputParams_p->LumaAndChromaThrowOneLineInTwo = TRUE;
    }
    else
    {
        VerticalScalingValue = (1024 * INT_AV_SCALE_CONSTANT) / InputParams_p->VerticalZoomFactor;
    }

    LineOffset = 0;

    /* Horizontal scaling */
    if ((InputParams_p->HorizontalZoomFactor != 0) && (InputParams_p->HorizontalZoomFactor < 512))
    {
        OutputParams_p->LumaHorizDown = TRUE;
        OutputParams_p->ChromaHorizDown = TRUE;
    }
    else
    {
        OutputParams_p->LumaHorizDown = FALSE;
        OutputParams_p->ChromaHorizDown = FALSE;
    }

    /* Vertical scaling */
    AdaptedScalingValue = VerticalScalingValue & ~1;

    if (AdaptedScalingValue == ((INT_AV_SCALE_CONSTANT * 4) / 3))
    {
        /* letterboxed output of 16:9 content */
        /* hardware bug workaround: letterboxed 16:9 got slightly wrong size
           we have to use 1368 to have video fit with highlights for letterboxed menus
           (necessary for applications using sub-picture only) */
        AdaptedScalingValue += 3;
        LineOffset = 0;
    }

    /* Note 1:
       the picture can only be shifted upwards with 'offset' (1024 = one field-line = two frame-lines)
       but in special cases (p.e. fieldfreezing) we must be able to shift it down by the fraction of one line
       so we are gaining headspace by starting with a fixed minimum value of one field-line.
       In interlaced mode 'offsetEven' and 'offsetOdd' are representing these values, being adjustable when freezing one field
       standard: offsetEven = 512, offsetOdd = 512
       freezing top-field: offsetEven = 512, offsetOdd = 768
       freezing bottom-field: offsetEven = 256, offsetOdd = 512
       the values are adjusted against each other, so all can be switched without causing a jitter

       Note 2:
       For 'zoom out' more than 'letterboxed' we don't use the zoomOut-bits, but switch to interlaced processing instead.
       Else - before reaching 50% - display artefacts can occur, because of memory bandwith access.*/

    if ((InputParams_p->ProgressiveSource)  &&  (AdaptedScalingValue <= (2 * INT_AV_SCALE_CONSTANT))) /* display cannot handle more in progressive */
    {
        /* Progressive */
        OutputParams_p->LumaInterpolate = TRUE;
        if (((AdaptedScalingValue * 2) <= INT_AV_SCALE_CONSTANT) ||
            ((AdaptedScalingValue <= INT_AV_SCALE_CONSTANT) &&
             (!(OutputParams_p->LumaAndChromaThrowOneLineInTwo))))
        {
            /* Case of zoom Output/Input :   1 -> infinite   */
            OutputParams_p->LumaFrameBased = TRUE;
            OutputParams_p->LumaIncrement = AdaptedScalingValue - 1;
            SignedLumaOffsetEven = LineOffset + 1024;
            SignedLumaOffsetOdd = LineOffset + 1024 + (AdaptedScalingValue / 2);
            OutputParams_p->LumaZoomOut = 0;
            OutputParams_p->ChromaFrameBased = TRUE;
            OutputParams_p->ChromaIncrement = (AdaptedScalingValue / 2) - 1;
            SignedChromaOffsetEven = (LineOffset / 2) + 512 - 128;  /* chroma must be shifted 1/4 chroma-line down */
            SignedChromaOffsetOdd = (LineOffset / 2) + 512 - 128 + ((AdaptedScalingValue + 3) / 4);
        }
        else if (((AdaptedScalingValue) <= INT_AV_SCALE_CONSTANT) ||
            ((AdaptedScalingValue <= (2 * INT_AV_SCALE_CONSTANT)) &&
             (!(OutputParams_p->LumaAndChromaThrowOneLineInTwo))))
        {
            /* Case of zoom Output/Input :   0.5 -> 0.999999   */
            /* we have to use lumaZoomOut = 1 here for downsampling
               instead of having a 3 pol-filter for displaying widescreen letterboxed (mostly progressive) */
            OutputParams_p->LumaFrameBased = FALSE;
            OutputParams_p->LumaIncrement = (AdaptedScalingValue / 2) - 1;
            SignedLumaOffsetEven = (LineOffset / 2) + 512;
            SignedLumaOffsetOdd = (LineOffset / 2) + 512 - 256 + ((AdaptedScalingValue + 3) / 4);
            OutputParams_p->LumaZoomOut = 0;
            OutputParams_p->ChromaFrameBased = TRUE;
            OutputParams_p->ChromaIncrement = (AdaptedScalingValue / 2) - 1;
            SignedChromaOffsetEven = (LineOffset / 2) + 512 - 128;  /* chroma must be shifted 1/4 chroma-line down */
            SignedChromaOffsetOdd = (LineOffset / 2) + 512 - 128 + ((AdaptedScalingValue + 3) / 4);
        }
        else if (((AdaptedScalingValue) <= (2 * INT_AV_SCALE_CONSTANT)) ||
            ((AdaptedScalingValue <= (4 * INT_AV_SCALE_CONSTANT)) &&
             (!(OutputParams_p->LumaAndChromaThrowOneLineInTwo))))
        {
            /* Case of zoom Output/Input :   0.25 -> 0.499999   */
            OutputParams_p->LumaFrameBased = FALSE;
            OutputParams_p->LumaIncrement = (AdaptedScalingValue / 2) - 1;
            SignedLumaOffsetEven = (LineOffset / 2) + 512;
            SignedLumaOffsetOdd = (LineOffset / 2) + 512 - 256 + ((AdaptedScalingValue + 3) / 4); /* - 256 because now processing fieldwise */
            OutputParams_p->LumaZoomOut = 0;
            OutputParams_p->ChromaFrameBased = TRUE;
            OutputParams_p->ChromaIncrement = (AdaptedScalingValue / 2) - 1;
            SignedChromaOffsetEven = (LineOffset / 2) + 512 - 64;
            SignedChromaOffsetOdd = (LineOffset / 2) + 512 - 64 + ((AdaptedScalingValue + 3) / 4);

        }
        else
        {
            OutputParams_p->LumaFrameBased = FALSE;
            OutputParams_p->LumaIncrement = (AdaptedScalingValue / 4) - 1;
            SignedLumaOffsetEven = (LineOffset / 2) + 512 - 384 ;
            SignedLumaOffsetOdd = (LineOffset / 2) + 512 - 512 + ((AdaptedScalingValue + 7) / 8);
            OutputParams_p->LumaZoomOut = 1;
            OutputParams_p->ChromaFrameBased = FALSE;
            OutputParams_p->ChromaIncrement = (AdaptedScalingValue / 4) - 1;
            SignedChromaOffsetEven = (LineOffset / 2) + 512 - 288;
            SignedChromaOffsetOdd = (LineOffset / 2) + 512 - 544 + ((AdaptedScalingValue + 7) / 8);
        }

        OutputParams_p->ChromaInterpolate = TRUE;
        OutputParams_p->ChromaZoomOut = 0;
    }
    else
    {
        /* Interlaced, or progressive more than display can handle */
        OutputParams_p->LumaInterpolate = TRUE;
        OutputParams_p->LumaFrameBased = FALSE;
        OutputParams_p->ChromaInterpolate = TRUE;
        OutputParams_p->ChromaFrameBased = FALSE;
        OutputParams_p->ChromaIncrement = (AdaptedScalingValue / 4) - 1;
        OutputParams_p->ChromaZoomOut = 0;


        if (((AdaptedScalingValue) <= INT_AV_SCALE_CONSTANT) ||
            ((AdaptedScalingValue <= (2 * INT_AV_SCALE_CONSTANT)) &&
             (!(OutputParams_p->LumaAndChromaThrowOneLineInTwo))))
        {
            /* Case of zoom Output/Input :   0.5 -> infinite   */
            SignedLumaOffsetOdd = (LineOffset / 2) + OffsetOdd - 256 + ((AdaptedScalingValue + 3) / 4);
            SignedLumaOffsetEven = (LineOffset / 2) + OffsetEven;
            SignedChromaOffsetOdd = (LineOffset / 4) + OffsetOdd - 576 + ((AdaptedScalingValue + 7) / 8);
            SignedChromaOffsetEven = (LineOffset / 4) + OffsetEven - 320;
            OutputParams_p->LumaIncrement = (AdaptedScalingValue / 2) - 1;
            OutputParams_p->LumaZoomOut = 0;
        }
        else if (((AdaptedScalingValue) <= (2 * INT_AV_SCALE_CONSTANT)) ||
            ((AdaptedScalingValue <= (4 * INT_AV_SCALE_CONSTANT)) &&
             (!(OutputParams_p->LumaAndChromaThrowOneLineInTwo))))
        {
            /* Case of zoom Output/Input :   0.25 -> 0.499999   */
            SignedLumaOffsetOdd = (LineOffset / 2) + OffsetOdd - 256 + ((AdaptedScalingValue + 3) / 4);
            SignedLumaOffsetEven = (LineOffset / 2) + OffsetEven;
            SignedChromaOffsetOdd = (LineOffset / 4) + OffsetOdd - 544 + ((AdaptedScalingValue + 7) / 8);
            SignedChromaOffsetEven = (LineOffset / 4) + OffsetEven - 288;
            OutputParams_p->LumaIncrement = (AdaptedScalingValue / 2) - 1;
            OutputParams_p->LumaZoomOut = 0;
        }
        else
        {
            /* Case of zoom Output/Input :   0 -> 0.249999   */
            SignedLumaOffsetOdd = (LineOffset / 2) + OffsetOdd - 512 + ((AdaptedScalingValue + 7) / 8);
            SignedLumaOffsetEven = (LineOffset / 2) + OffsetEven - 384 ;
            SignedChromaOffsetOdd = (LineOffset / 4) + OffsetOdd - 544 + ((AdaptedScalingValue + 7) / 8);
            SignedChromaOffsetEven = (LineOffset / 4) + OffsetEven - 288;
            OutputParams_p->LumaIncrement = (AdaptedScalingValue / 4) - 1;
            OutputParams_p->LumaZoomOut = 1;
        }
        if (InputParams_p->IsFreezing)
        {
            SignedLumaOffsetOdd += 256;
            SignedChromaOffsetOdd += 128;
        }
    }

    if (SignedLumaOffsetEven < 0)
    {
        OutputParams_p->LumaOffsetEven = 0;
    }
    else
    {
        OutputParams_p->LumaOffsetEven = (U32) SignedLumaOffsetEven;
    }
    if (SignedLumaOffsetOdd < 0)
    {
        OutputParams_p->LumaOffsetOdd = 0;
    }
    else
    {
        OutputParams_p->LumaOffsetOdd = (U32) SignedLumaOffsetOdd;
    }
    if (SignedChromaOffsetEven < 0)
    {
        OutputParams_p->ChromaOffsetEven = 0;
    }
    else
    {
        OutputParams_p->ChromaOffsetEven = (U32) SignedChromaOffsetEven;
    }
    if (SignedChromaOffsetOdd < 0)
    {
        OutputParams_p->ChromaOffsetOdd = 0;
    }
    else
    {
        OutputParams_p->ChromaOffsetOdd = SignedChromaOffsetOdd;
    }

} /* End of ComputeB2RVerticalFilters() function */

/*******************************************************************************
Name        : IsHorizontalDownUsed
Description : Tells whether horizontal downscaling is used or not in B2R vertical filters
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
BOOL IsHorizontalDownUsed(const U32 HorizontalZoomFactor)
{
    /* Warning: must be same test as in ComputeB2RVerticalFilters() function to be consistant ! */

    /* Horizontal scaling */
    if ((HorizontalZoomFactor != 0) && (HorizontalZoomFactor < 512))
    {
        return(TRUE);
    }
    return(FALSE);
} /* End of IsHorizontalDownUsed() function */

/*******************************************************************************
Name        : ComputeTheNumberOfLinesRemoved
Description : To keep a perfect alignment luma-chroma, some lines can be loose
              with some particular zooms.
              This function calculates this number of lost lines.
Parameters  : Filters, Input Size, Output Size.
Assumptions : When entering in the func, (*SmallestOutputSize_p) is the
              original Output Size.
Limitations :
Returns     : (*SmallestOutputSize_p)
*******************************************************************************/
void ComputeTheNumberOfLinesRemoved(const HALLAYER_B2RVerticalFiltersParams_t * B2RVerticalFiltersParams_p,
                                    const U32 * const OriginalInputSize_p,
                                    const U32 ScanIn16thPixel,
                                    U32 * const SmallestOutputSize_p)
{
    BOOL FrameBased[4];
    U8 ZoomOut[4];
    U16 Offset[4], Increment[4];
    U32 InputSize[4], OutputSize[4];
    U32 Tmp32;
    U8 index;
    U32 Offsetvscan;

/* We calculate the 4 output sizes (Luma Odd, Luma Even, Chroma Odd, Chroma Even) with the following formula :  */
/*                                                                                                              */
/*                  inputsize x FBS                                                                             */
/*                 (--------------- - 1 )                                                                       */
/*                      zoomout                                                                                 */
/*                  --------------------  x 512 - offsetvscan - offsetalign                                     */
/*                     prezoomout                                                                               */
/*   outputsize = ----------------------------------------------------------- + 1                               */
/*                            Increment + 1                                                                     */
/*                                                                                                              */
/* Where : increment is the increment value for this filter                                                     */
/*         outputsize is the number of lines in the display region                                              */
/*         prezoomout is the precompression zoom factor (1, 2, 8/3, 4)                                          */
/*         zoomoutratio is the global zoom-out factor                                                           */
/*         offsetvscan is the offset due to the vertical scan applied                                           */
/*                  = 512 * prezoomout * (Scan - (value in macroblocks from register SCN))                      */
/*         offsetalign is the offset value we apply to align odd/even fields and luma/chroma                    */
/*         512 corresponds to the subline resolution of the programmable filter                                 */
/*         inputsize is the number of lines at the input                                                        */
/*         FBS is the way to read of lines before the B2R (frame or field based). Field=1 / Frame=2             */
/*         zoomout is a global factor to zoomout by 2 (bit zoomout4+1)                                          */
/*                                                                                                              */

    /* For a better reading, we create temporary variables. */
    /* 0 : Luma Odd    */
    /* 1 : Luma Even   */
    /* 2 : Chroma Odd  */
    /* 3 : Chroma Even */
    Offset[0] = B2RVerticalFiltersParams_p->LumaOffsetOdd ;
    Offset[1] = B2RVerticalFiltersParams_p->LumaOffsetEven ;
    Offset[2] = B2RVerticalFiltersParams_p->ChromaOffsetOdd;
    Offset[3] = B2RVerticalFiltersParams_p->ChromaOffsetEven;
    Increment[0] = Increment[1] = B2RVerticalFiltersParams_p->LumaIncrement ;
    Increment[2] = Increment[3] = B2RVerticalFiltersParams_p->ChromaIncrement ;
    InputSize[0] = InputSize[1] =  (*OriginalInputSize_p) ;
    InputSize[2] = InputSize[3] =  (*OriginalInputSize_p) / 2 ; /* Due to 4 : 2 : 0, 2 times minus of lines Chroma than Luma. */
    FrameBased[0] = FrameBased[1] = B2RVerticalFiltersParams_p->LumaFrameBased ;
    FrameBased[2] = FrameBased[3] = B2RVerticalFiltersParams_p->ChromaFrameBased ;
    ZoomOut[0] = ZoomOut[1] = B2RVerticalFiltersParams_p->LumaZoomOut ;
    ZoomOut[2] = ZoomOut[3] = B2RVerticalFiltersParams_p->ChromaZoomOut ;
    OutputSize[0] = OutputSize[1] = OutputSize[2] = OutputSize[3] = 0;

    /* Calculation of the 4 OutputSize : Luma Odd, ... , Chroma Even. */
    for (index=0; index < 4; index++)
    {
        if (InputSize[index] == 0)
        {
            /* Nothing to compute if size is 0 */
            continue;
        }
        if (FrameBased[index])
        {
            OutputSize[index] = InputSize[index] << 1;
            Offsetvscan = 512;
        }
        else
        {
            OutputSize[index] = InputSize[index];
            Offsetvscan = 256;
        }
        if (B2RVerticalFiltersParams_p->LumaAndChromaThrowOneLineInTwo)
        {
            /* When throwing away one line out of 2, OutputSize is there divided by 2 */
            OutputSize[index] = OutputSize[index] >> 1;
            Offsetvscan = Offsetvscan << 1;
        }
        OutputSize[index] -= 1;
        OutputSize[index] *= 512;
        if (ZoomOut[index] == 1)
        {
            /* Factor 2 */
            OutputSize[index] = OutputSize[index] >> 1;
            Offsetvscan = Offsetvscan << 1;
        }
        else if (ZoomOut[index] == 2)
        {
            /* Factor 8/3 */
            OutputSize[index] = (3 * OutputSize[index]) >> 3;
            Offsetvscan = (Offsetvscan << 3) / 3;
        }
        else if (ZoomOut[index] == 3)
        {
            /* Factor 4 */
            OutputSize[index] = OutputSize[index] >> 2;
            Offsetvscan = Offsetvscan << 2;
        }
        if (OutputSize[index] > Offset[index])
        {
            OutputSize[index] -= Offset[index];
            if (index <= 1)
            {
                /* Luma */
                Offsetvscan = (Offsetvscan * (ScanIn16thPixel % 256)) / 16;
            }
            else
            {
                /* Chroma divided by 2 for 4:2:0 */
                Offsetvscan = (Offsetvscan * ((ScanIn16thPixel / 2) % 256)) / 16;
            }
            /* Not sure if should be doubled for CIF case */
            if (OutputSize[index] > Offsetvscan)
            {
                OutputSize[index] -= Offsetvscan;
                OutputSize[index] /= (Increment[index] + 1);
                OutputSize[index] += 1;
#if defined WA_GNBvd24691
                /* No work-around proposed yet by validation, temporary fixed by HG: removing 3 more lines (based on some tests) */
                if ((B2RVerticalFiltersParams_p->LumaAndChromaThrowOneLineInTwo) && (OutputSize[index] > 2))
                {
                    /* Need to hide more lines when zoom-out by 2 is used. */
                    OutputSize[index] -= 3;
                }
#endif /* defined WA_GNBvd24691 */
            }
            else
            {
                OutputSize[index] = 0;
            }
        }
        else
        {
                OutputSize[index] = 0;
        }
   }
#ifdef TRACE_UART
/*    TraceBuffer(("-LO=%d-LE=%d-CO=%d-CE=%d\r\n", OutputSize[0], OutputSize[1], OutputSize[2], OutputSize[3] ));*/
#endif /* TRACE_UART */

    /* Comparison of the 4 OutputSize : we are looking for the minus. */
    if (OutputSize[0] < OutputSize[1])
    {
        Tmp32 = OutputSize[0] ;
    }
    else
    {
        Tmp32 = OutputSize[1] ;
    }
    if (OutputSize[2] < Tmp32)
    {
        Tmp32 = OutputSize[2] ;
    }
    if (OutputSize[3] < Tmp32)
    {
        Tmp32 = OutputSize[3] ;
    }

#ifdef TRACE_UART
    TraceBuffer(("Smallest=%d..", Tmp32 ));
#endif /* TRACE_UART */

    /* Limitation */
    if (((S32) ((*SmallestOutputSize_p) - Tmp32)) > 4)
    {
        /* Never need to skip more than 4 lines */
        (*SmallestOutputSize_p) -= 4;
    }
    else
    {
    	if (((S32) ((*SmallestOutputSize_p) - Tmp32)) > 0)
		{
			(*SmallestOutputSize_p) = Tmp32;
		}
    }
} /* End of ComputeTheNumberOfLinesRemoved() function */


/*******************************************************************************
Name        : CompileB2RVerticalFiltersRegisterValues
Description : Calculate registers values of B2R vertical filters
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void CompileB2RVerticalFiltersRegisterValues(const HALLAYER_B2RVerticalFiltersParams_t * const OutputParams_p,
                                             HALLAYER_B2RVerticalFiltersRegisters_t * const Registers_p)
{
    Registers_p->LumaRegister0 = OutputParams_p->LumaIncrement & 0xFF;
    Registers_p->LumaRegister1 = ((OutputParams_p->LumaIncrement & 0x300) >> 8) |
                                 ((OutputParams_p->LumaOffsetEven & 0x003F) << 2);
    Registers_p->LumaRegister2 = ((OutputParams_p->LumaOffsetEven & 0x1FC0) >> 6) |
                                 ((OutputParams_p->LumaOffsetOdd & 0x0001) << 7);
    Registers_p->LumaRegister3 = ((OutputParams_p->LumaOffsetOdd & 0x01FE) >> 1);
    Registers_p->LumaRegister4 = ((OutputParams_p->LumaOffsetOdd & 0x1E00) >> 9) |
                                 ((OutputParams_p->LumaZoomOut & 0x03) << 4);
    if (OutputParams_p->LumaInterpolate)
    {
        Registers_p->LumaRegister4 |= 0x40;
    }
    if (OutputParams_p->LumaHorizDown)
    {
        Registers_p->LumaRegister4 |= 0x80;
    }
    Registers_p->ChromaRegister0 = OutputParams_p->ChromaIncrement & 0xFF;
    Registers_p->ChromaRegister1 = ((OutputParams_p->ChromaIncrement & 0x300) >> 8) |
                                   ((OutputParams_p->ChromaOffsetEven & 0x003F) << 2);
    Registers_p->ChromaRegister2 = ((OutputParams_p->ChromaOffsetEven & 0x1FC0) >> 6) |
                                   ((OutputParams_p->ChromaOffsetOdd & 0x0001) << 7);
    Registers_p->ChromaRegister3 = ((OutputParams_p->ChromaOffsetOdd & 0x01FE) >> 1);
    Registers_p->ChromaRegister4 = ((OutputParams_p->ChromaOffsetOdd & 0x1E00) >> 9) |
                                   ((OutputParams_p->ChromaZoomOut & 0x03) << 4);
    if (OutputParams_p->ChromaInterpolate)
    {
        Registers_p->ChromaRegister4 |= 0x40;
    }
    if (OutputParams_p->ChromaHorizDown)
    {
        Registers_p->ChromaRegister4 |= 0x80;
    }
} /* end of CompileB2RVerticalFiltersRegisterValues() function */
#endif /* defined(HW_5508) || defined(HW_5518) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578) */
/* end of Hv_layer.c */


