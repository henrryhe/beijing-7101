/*******************************************************************************

File name   : lay_sub.h

Description :

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2001-01-12         Created                                    Thomas Meyer

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LAYCOMPO_H
#define __LAYCOMPO_H

/* Includes ----------------------------------------------------------------- */
#include "stlayer.h"
#include "stos.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Macros --------------------------------------------------------------------*/
#define LAYCOMPO_NO_VIEWPORT_HANDLE ((STLAYER_ViewPortHandle_t) NULL)

/* Common functions ----------------------------------------------------------*/

ST_ErrorCode_t LAYCOMPO_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability_p);

ST_ErrorCode_t LAYCOMPO_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params_p);

ST_ErrorCode_t LAYCOMPO_Init(const ST_DeviceName_t        DeviceName,
                             const STLAYER_InitParams_t * const InitParams_p);

ST_ErrorCode_t LAYCOMPO_Term(const ST_DeviceName_t        DeviceName,
                             const STLAYER_TermParams_t * const TermParams_p);

ST_ErrorCode_t LAYCOMPO_Open(const ST_DeviceName_t       DeviceName,
                             const STLAYER_OpenParams_t * const Params_p,
                             STLAYER_Handle_t *    Handle_p);

ST_ErrorCode_t LAYCOMPO_Close(STLAYER_Handle_t  Handle);

ST_ErrorCode_t LAYCOMPO_GetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

ST_ErrorCode_t LAYCOMPO_SetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

/* ViewPort functions */
ST_ErrorCode_t LAYCOMPO_OpenViewPort(
  STLAYER_Handle_t            Handle,
  STLAYER_ViewPortParams_t*   Params_p,
  STLAYER_ViewPortHandle_t*   VPHandle_p);

ST_ErrorCode_t LAYCOMPO_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYCOMPO_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYCOMPO_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYCOMPO_SetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource_p
);

ST_ErrorCode_t LAYCOMPO_GetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource_p
);

ST_ErrorCode_t LAYCOMPO_SetViewPortIORectangle(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_Rectangle_t*        InputRectangle_p,
  STGXOBJ_Rectangle_t*        OutputRectangle_p
);

ST_ErrorCode_t LAYCOMPO_AdjustIORectangle(
  STLAYER_Handle_t       Handle,
  STGXOBJ_Rectangle_t*   InputRectangle_p,
  STGXOBJ_Rectangle_t*   OutputRectangle_p
);

ST_ErrorCode_t LAYCOMPO_GetViewPortIORectangle(
  STLAYER_ViewPortHandle_t  VPHandle,
  STGXOBJ_Rectangle_t*      InputRectangle_p,
  STGXOBJ_Rectangle_t*      OutputRectangle_p
);

ST_ErrorCode_t LAYCOMPO_AdjustViewPortParams(STLAYER_Handle_t Handle,
                                           STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYCOMPO_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYCOMPO_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p);


ST_ErrorCode_t LAYCOMPO_SetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32                      PositionX,
  S32                      PositionY
);

ST_ErrorCode_t LAYCOMPO_GetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32*                     PositionX_p,
  S32*                     PositionY_p
);


ST_ErrorCode_t LAYCOMPO_GetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha_p
);

ST_ErrorCode_t LAYCOMPO_SetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha_p
);

ST_ErrorCode_t LAYCOMPO_GetBitmapAllocParams(
  STLAYER_Handle_t             Handle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  STGXOBJ_BitmapAllocParams_t* Params1_p,
  STGXOBJ_BitmapAllocParams_t* Params2_p
);

ST_ErrorCode_t LAYCOMPO_GetBitmapHeaderSize(
  STLAYER_Handle_t             Handle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  U32 *                        HeaderSize_p
);

ST_ErrorCode_t LAYCOMPO_GetPaletteAllocParams(
  STLAYER_Handle_t               Handle,
  STGXOBJ_Palette_t*             Palette_p,
  STGXOBJ_PaletteAllocParams_t*  Params_p
);

ST_ErrorCode_t LAYCOMPO_UpdateFromMixer(
        STLAYER_Handle_t        Handle,
        STLAYER_OutputParams_t * OutputParams_p
);

ST_ErrorCode_t LAYCOMPO_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t LAYCOMPO_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t LAYCOMPO_SetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,BOOL Recordable);
ST_ErrorCode_t LAYCOMPO_GetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,BOOL* Recordable_p);
ST_ErrorCode_t LAYCOMPO_EnableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t LAYCOMPO_DisableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t LAYCOMPO_GetVTGName(STLAYER_Handle_t LayerHandle, ST_DeviceName_t* const VTGName_p);
ST_ErrorCode_t LAYCOMPO_GetVTGParams(STLAYER_Handle_t LayerHandle,STLAYER_VTGParams_t* const VTGParams_p);

ST_ErrorCode_t LAYCOMPO_SetViewPortCompositionRecurrence(STLAYER_ViewPortHandle_t        VPHandle,
                                                   const STLAYER_CompositionRecurrence_t CompositionRecurrence);
ST_ErrorCode_t LAYCOMPO_PerformViewPortComposition(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYCOMPO_SetViewPortFlickerFilterMode(STLAYER_ViewPortHandle_t     VPHandle,
                                                     STLAYER_FlickerFilterMode_t  FlickerFilterMode);

ST_ErrorCode_t LAYCOMPO_GetViewPortFlickerFilterMode(STLAYER_ViewPortHandle_t      VPHandle,
                                                     STLAYER_FlickerFilterMode_t*  FlickerFilterMode_p);

ST_ErrorCode_t LAYCOMPO_InformPictureToBeDecoded(
  STLAYER_ViewPortHandle_t      VPHandle,
  STGXOBJ_PictureInfos_t*       PictureInfos_p
);

ST_ErrorCode_t LAYCOMPO_CommitViewPortParams(STLAYER_ViewPortHandle_t VPHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LAYCOMPO_H */

/* End of laycompo.h */
