/*******************************************************************************

File name   : layerosd.h

Description : STLAYER API. GFX  OSD Part. Prototypes and types

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                               Name
----               ------------                               ----
2000-11-27         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LAYEROSD_H
#define __LAYEROSD_H

/* Includes ----------------------------------------------------------------- */
#include "stlayer.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Macros --------------------------------------------------------------------*/

#include "sttbx.h"
#define layerosd_Defense(Error) STTBX_Report((STTBX_REPORT_LEVEL_INFO, \
        "STLAYER - OSD : A function returned an error num 0x%x",Error))
#define layerosd_Trace(Msg) STTBX_Report( (STTBX_REPORT_LEVEL_INFO,Msg) )

/* Common functions ----------------------------------------------------------*/

ST_ErrorCode_t LAYEROSD_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability);

ST_ErrorCode_t LAYEROSD_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params);

ST_ErrorCode_t LAYEROSD_Init(const ST_DeviceName_t        DeviceName,
                            const STLAYER_InitParams_t * const InitParams_p);

ST_ErrorCode_t LAYEROSD_Term(const ST_DeviceName_t        DeviceName,
                            const STLAYER_TermParams_t * const TermParams_p);

ST_ErrorCode_t LAYEROSD_Open(const ST_DeviceName_t       DeviceName,
                            const STLAYER_OpenParams_t * const Params,
                            STLAYER_Handle_t *    Handle);

ST_ErrorCode_t LAYEROSD_Close(STLAYER_Handle_t  Handle);

ST_ErrorCode_t LAYEROSD_GetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

ST_ErrorCode_t LAYEROSD_SetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

/* ViewPort functions */
ST_ErrorCode_t LAYEROSD_OpenViewPort(
  STLAYER_Handle_t            LayerHandle,
  STLAYER_ViewPortParams_t*   Params,
  STLAYER_ViewPortHandle_t*   VPHandle);

ST_ErrorCode_t LAYEROSD_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYEROSD_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYEROSD_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYEROSD_SetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t LAYEROSD_GetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t LAYEROSD_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYEROSD_AdjustViewPortParams(STLAYER_Handle_t Handle,
                                           STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYEROSD_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                           STLAYER_ViewPortParams_t * Params_p);


ST_ErrorCode_t LAYEROSD_SetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                             BOOL Recordable);

ST_ErrorCode_t LAYEROSD_GetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                             BOOL  * Recordable_p);


ST_ErrorCode_t LAYEROSD_SetViewPortIORectangle(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_Rectangle_t*        InputRectangle,
  STGXOBJ_Rectangle_t*        OutputRectangle
);

ST_ErrorCode_t LAYEROSD_AdjustIORectangle(
  STLAYER_Handle_t       Handle,
  STGXOBJ_Rectangle_t*   InputRectangle,
  STGXOBJ_Rectangle_t*   OutputRectangle
);

ST_ErrorCode_t LAYEROSD_GetViewPortIORectangle(
  STLAYER_ViewPortHandle_t  VPHandle,
  STGXOBJ_Rectangle_t*      InputRectangle,
  STGXOBJ_Rectangle_t*      OutputRectangle
);

ST_ErrorCode_t LAYEROSD_SetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32                      XPosition,
  S32                      YPosition
);

ST_ErrorCode_t LAYEROSD_GetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32*                     XPosition,
  S32*                     YPosition
);

ST_ErrorCode_t LAYEROSD_SetViewPortPSI(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_PSI_t*            VPPSI_p
);

ST_ErrorCode_t LAYEROSD_DisableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYEROSD_EnableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYEROSD_SetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t LAYEROSD_GetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t LAYEROSD_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYEROSD_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYEROSD_GetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t LAYEROSD_SetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t LAYEROSD_SetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
);

ST_ErrorCode_t LAYEROSD_GetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
);
ST_ErrorCode_t LAYEROSD_GetBitmapAllocParams(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  STGXOBJ_BitmapAllocParams_t* Params1_p,
  STGXOBJ_BitmapAllocParams_t* Params2_p
);

ST_ErrorCode_t LAYEROSD_GetBitmapHeaderSize(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  U32 *                        HeaderSize_p
);

ST_ErrorCode_t LAYEROSD_GetPaletteAllocParams(
  STLAYER_Handle_t               LayerHandle,
  STGXOBJ_Palette_t*             Palette_p,
  STGXOBJ_PaletteAllocParams_t*  Params_p
);

ST_ErrorCode_t LAYEROSD_UpdateFromMixer(
        STLAYER_Handle_t        Handle,
        STLAYER_OutputParams_t * OutputParams_p
);

ST_ErrorCode_t LAYEROSD_EnableViewPortFilter(void);
ST_ErrorCode_t LAYEROSD_DisableViewPortFilter(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LAYEROSD_H */

/* End of layerosd.h */
