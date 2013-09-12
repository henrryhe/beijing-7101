/*******************************************************************************

File name   : layergfx.h

Description : STLAYER API. GFX Part. Prototypes and types

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LAYERGFX_H
#define __LAYERGFX_H

/* Includes ----------------------------------------------------------------- */
#include "stlayer.h"
#include "stos.h"
#include "sttbx.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Macros --------------------------------------------------------------------*/
#ifdef ST_OSLINUX

#define layergfx_Defense(Error) { STTBX_Report((STTBX_REPORT_LEVEL_INFO, 	\
        "STLAYER - GFX : A function returned an error num 0x%x",Error)); 	\
		STTBX_REPORT_NL														\
								}
#define layergfx_Trace(Msg) {STTBX_Report( (STTBX_REPORT_LEVEL_INFO,Msg) ); STTBX_REPORT_NL}

#else

#define layergfx_Defense(Error) STTBX_Report((STTBX_REPORT_LEVEL_INFO, \
        "STLAYER - GFX : A function returned an error num 0x%x",Error))
#define layergfx_Trace(Msg) STTBX_Report( (STTBX_REPORT_LEVEL_INFO,Msg) )

#endif
/* Common functions ----------------------------------------------------------*/

ST_ErrorCode_t LAYERGFX_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability);

ST_ErrorCode_t LAYERGFX_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params);

ST_ErrorCode_t LAYERGFX_Init(const ST_DeviceName_t        DeviceName,
                            const STLAYER_InitParams_t * const InitParams_p);

ST_ErrorCode_t LAYERGFX_Term(const ST_DeviceName_t        DeviceName,
                            const STLAYER_TermParams_t * const TermParams_p);

ST_ErrorCode_t LAYERGFX_Open(const ST_DeviceName_t       DeviceName,
                            const STLAYER_OpenParams_t * const Params,
                            STLAYER_Handle_t *    Handle);

ST_ErrorCode_t LAYERGFX_Close(STLAYER_Handle_t  Handle);

ST_ErrorCode_t LAYERGFX_GetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

ST_ErrorCode_t LAYERGFX_SetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

/* ViewPort functions */
ST_ErrorCode_t LAYERGFX_OpenViewPort(
  STLAYER_Handle_t            LayerHandle,
  STLAYER_ViewPortParams_t*   Params,
  STLAYER_ViewPortHandle_t*   VPHandle);

ST_ErrorCode_t LAYERGFX_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERGFX_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERGFX_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERGFX_AdjustViewPortParams(
  STLAYER_Handle_t LayerHandle,
  STLAYER_ViewPortParams_t*   Params_p
);

ST_ErrorCode_t LAYERGFX_SetViewPortParams(
  const STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_ViewPortParams_t*       Params_p
);

ST_ErrorCode_t LAYERGFX_GetViewPortParams(
  const STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_ViewPortParams_t*       Params_p
);

ST_ErrorCode_t LAYERGFX_SetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t LAYERGFX_GetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t LAYERGFX_SetViewPortIORectangle(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_Rectangle_t*        InputRectangle,
  STGXOBJ_Rectangle_t*        OutputRectangle
);

ST_ErrorCode_t LAYERGFX_AdjustIORectangle(
  STLAYER_Handle_t       Handle,
  STGXOBJ_Rectangle_t*   InputRectangle,
  STGXOBJ_Rectangle_t*   OutputRectangle
);

ST_ErrorCode_t LAYERGFX_GetViewPortIORectangle(
  STLAYER_ViewPortHandle_t  VPHandle,
  STGXOBJ_Rectangle_t*      InputRectangle,
  STGXOBJ_Rectangle_t*      OutputRectangle
);

ST_ErrorCode_t LAYERGFX_SetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32                      XPosition,
  S32                      YPosition
);

ST_ErrorCode_t LAYERGFX_GetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32*                     XPosition,
  S32*                     YPosition
);

ST_ErrorCode_t LAYERGFX_SetViewPortPSI(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_PSI_t*            VPPSI_p
);

ST_ErrorCode_t LAYERGFX_DisableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERGFX_EnableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERGFX_SetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                             BOOL Recordable);

ST_ErrorCode_t LAYERGFX_GetViewPortRecordable(STLAYER_ViewPortHandle_t VPHandle,
                                             BOOL  * Recordable_p);

ST_ErrorCode_t LAYERGFX_SetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t LAYERGFX_GetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t LAYERGFX_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERGFX_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERGFX_GetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t LAYERGFX_SetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t LAYERGFX_SetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
);

ST_ErrorCode_t LAYERGFX_GetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
);
ST_ErrorCode_t LAYERGFX_GetBitmapAllocParams(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  STGXOBJ_BitmapAllocParams_t* Params1_p,
  STGXOBJ_BitmapAllocParams_t* Params2_p
);

ST_ErrorCode_t LAYERGFX_GetBitmapHeaderSize(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  U32 *                        HeaderSize_p
);

ST_ErrorCode_t LAYERGFX_GetPaletteAllocParams(
  STLAYER_Handle_t               LayerHandle,
  STGXOBJ_Palette_t*             Palette_p,
  STGXOBJ_PaletteAllocParams_t*  Params_p
);

ST_ErrorCode_t LAYERGFX_UpdateFromMixer(
        STLAYER_Handle_t        Handle,
        STLAYER_OutputParams_t * OutputParams_p
);

ST_ErrorCode_t LAYERGFX_GetVTGName(
  STLAYER_Handle_t               LayerHandle,
  ST_DeviceName_t * const        VTGName_p
);

ST_ErrorCode_t LAYERGFX_GetVTGParams(
        STLAYER_Handle_t            LayerHandle,
        STLAYER_VTGParams_t * const VTGParams_p
);

ST_ErrorCode_t LAYERGFX_AttachViewPort(
        STLAYER_ViewPortHandle_t VPHandle,
        U32 Ident
);

ST_ErrorCode_t LAYERGFX_SetViewPortFlickerFilterMode(
        STLAYER_ViewPortHandle_t VPHandle,
        STLAYER_FlickerFilterMode_t  FlickerFilterMode
);

ST_ErrorCode_t LAYERGFX_GetViewPortFlickerFilterMode(
        STLAYER_ViewPortHandle_t VPHandle,
        STLAYER_FlickerFilterMode_t * FlickerFilterMode
);

ST_ErrorCode_t LAYERGFX_SetViewportColorFill(
        STLAYER_ViewPortHandle_t VPHandle,
        STGXOBJ_ColorARGB_t*  ColorFill
);

ST_ErrorCode_t LAYERGFX_GetViewportColorFill(
        STLAYER_ViewPortHandle_t VPHandle,
        STGXOBJ_ColorARGB_t*  ColorFill
);

ST_ErrorCode_t LAYERGFX_EnableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t LAYERGFX_DisableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t LAYERGFX_EnableFlickerFilter(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t LAYERGFX_DisableFlickerFilter(STLAYER_ViewPortHandle_t VPHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LAYERGFX_H */

/* End of layergfx.h */
