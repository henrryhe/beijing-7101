/*******************************************************************************

File name   : lay_sub.h

Description :

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2001-01-12         Created                                    Thomas Meyer

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LAYER_SUB_H
#define __LAYER_SUB_H

/* Includes ----------------------------------------------------------------- */
#include "stlayer.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Macros --------------------------------------------------------------------*/


/* Common functions ----------------------------------------------------------*/

ST_ErrorCode_t LAYERSUB_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability_p);

ST_ErrorCode_t LAYERSUB_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params_p);

ST_ErrorCode_t LAYERSUB_Init(const ST_DeviceName_t        DeviceName,
                             const STLAYER_InitParams_t * const InitParams_p);

ST_ErrorCode_t LAYERSUB_Term(const ST_DeviceName_t        DeviceName,
                             const STLAYER_TermParams_t * const TermParams_p);

ST_ErrorCode_t LAYERSUB_Open(const ST_DeviceName_t       DeviceName,
                             const STLAYER_OpenParams_t * const Params_p,
                             STLAYER_Handle_t *    Handle_p);

ST_ErrorCode_t LAYERSUB_Close(STLAYER_Handle_t  Handle);

ST_ErrorCode_t LAYERSUB_GetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

ST_ErrorCode_t LAYERSUB_SetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

/* ViewPort functions */
ST_ErrorCode_t LAYERSUB_OpenViewPort(
  STLAYER_Handle_t            Handle,
  STLAYER_ViewPortParams_t*   Params_p,
  STLAYER_ViewPortHandle_t*   VPHandle_p);

ST_ErrorCode_t LAYERSUB_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSUB_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSUB_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSUB_SetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource_p
);

ST_ErrorCode_t LAYERSUB_GetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource_p
);

ST_ErrorCode_t LAYERSUB_SetViewPortIORectangle(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_Rectangle_t*        InputRectangle_p,
  STGXOBJ_Rectangle_t*        OutputRectangle_p
);

ST_ErrorCode_t LAYERSUB_AdjustIORectangle(
  STLAYER_Handle_t       Handle,
  STGXOBJ_Rectangle_t*   InputRectangle_p,
  STGXOBJ_Rectangle_t*   OutputRectangle_p
);

ST_ErrorCode_t LAYERSUB_GetViewPortIORectangle(
  STLAYER_ViewPortHandle_t  VPHandle,
  STGXOBJ_Rectangle_t*      InputRectangle_p,
  STGXOBJ_Rectangle_t*      OutputRectangle_p
);

ST_ErrorCode_t LAYERSUB_AdjustViewPortParams(STLAYER_Handle_t Handle,
                                           STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYERSUB_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYERSUB_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p);


ST_ErrorCode_t LAYERSUB_SetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32                      PositionX,
  S32                      PositionY
);

ST_ErrorCode_t LAYERSUB_GetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32*                     PositionX_p,
  S32*                     PositionY_p
);


ST_ErrorCode_t LAYERSUB_GetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha_p
);

ST_ErrorCode_t LAYERSUB_SetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha_p
);

ST_ErrorCode_t LAYERSUB_GetBitmapAllocParams(
  STLAYER_Handle_t             Handle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  STGXOBJ_BitmapAllocParams_t* Params1_p,
  STGXOBJ_BitmapAllocParams_t* Params2_p
);

ST_ErrorCode_t LAYERSUB_GetBitmapHeaderSize(
  STLAYER_Handle_t             Handle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  U32 *                        HeaderSize_p
);

ST_ErrorCode_t LAYERSUB_GetPaletteAllocParams(
  STLAYER_Handle_t               Handle,
  STGXOBJ_Palette_t*             Palette_p,
  STGXOBJ_PaletteAllocParams_t*  Params_p
);

ST_ErrorCode_t LAYERSUB_UpdateFromMixer(
        STLAYER_Handle_t        Handle,
        STLAYER_OutputParams_t * OutputParams_p
);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LAYER_SUB_H */

/* End of lay_sub.h */
