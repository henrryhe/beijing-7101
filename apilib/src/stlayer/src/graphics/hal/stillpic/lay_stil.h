/*******************************************************************************

File name   : lay_stil.h

Description : 

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-12-20         Created                                    Michel Bruant

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LAYERSTI_H
#define __LAYERSTI_H

/* Includes ----------------------------------------------------------------- */
#include "stlayer.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Macros --------------------------------------------------------------------*/

#include "sttbx.h"
#define layerstill_Defense(Error) STTBX_Report((STTBX_REPORT_LEVEL_INFO, \
        "STLAYER - STILL : A function returned an error num 0x%x",Error))
#define layerstill_Trace(Msg) STTBX_Report( (STTBX_REPORT_LEVEL_INFO,Msg) )

/* Common functions ----------------------------------------------------------*/
    
ST_ErrorCode_t LAYERSTILL_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability);

ST_ErrorCode_t LAYERSTILL_GetInitAllocParams(STLAYER_Layer_t LayerType, 
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params); 

ST_ErrorCode_t LAYERSTILL_Init(const ST_DeviceName_t        DeviceName,
                            const STLAYER_InitParams_t * const InitParams_p);

ST_ErrorCode_t LAYERSTILL_Term(const ST_DeviceName_t        DeviceName,
                            const STLAYER_TermParams_t * const TermParams_p);

ST_ErrorCode_t LAYERSTILL_Open(const ST_DeviceName_t       DeviceName,
                            const STLAYER_OpenParams_t * const Params,
                            STLAYER_Handle_t *    Handle);

ST_ErrorCode_t LAYERSTILL_Close(STLAYER_Handle_t  Handle);

ST_ErrorCode_t LAYERSTILL_GetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

ST_ErrorCode_t LAYERSTILL_SetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

/* ViewPort functions */
ST_ErrorCode_t LAYERSTILL_OpenViewPort(
  STLAYER_Handle_t            LayerHandle,
  STLAYER_ViewPortParams_t*   Params,   
  STLAYER_ViewPortHandle_t*   VPHandle);

ST_ErrorCode_t LAYERSTILL_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSTILL_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSTILL_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSTILL_SetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t LAYERSTILL_GetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t LAYERSTILL_SetViewPortIORectangle(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_Rectangle_t*        InputRectangle,
  STGXOBJ_Rectangle_t*        OutputRectangle
);

ST_ErrorCode_t LAYERSTILL_AdjustIORectangle(
  STLAYER_Handle_t       Handle,
  STGXOBJ_Rectangle_t*   InputRectangle,  
  STGXOBJ_Rectangle_t*   OutputRectangle
);

ST_ErrorCode_t LAYERSTILL_GetViewPortIORectangle(
  STLAYER_ViewPortHandle_t  VPHandle,
  STGXOBJ_Rectangle_t*      InputRectangle,
  STGXOBJ_Rectangle_t*      OutputRectangle
);

ST_ErrorCode_t LAYERSTILL_AdjustViewPortParams(STLAYER_Handle_t LayerHandle,
                                           STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYERSTILL_SetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                    STLAYER_ViewPortParams_t * Params_p);

ST_ErrorCode_t LAYERSTILL_GetViewPortParams(STLAYER_ViewPortHandle_t  VPHandle,
                                   STLAYER_ViewPortParams_t * Params_p);


ST_ErrorCode_t LAYERSTILL_SetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32                      XPosition,
  S32                      YPosition
);

ST_ErrorCode_t LAYERSTILL_GetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32*                     XPosition,
  S32*                     YPosition
);

ST_ErrorCode_t LAYERSTILL_SetViewPortPSI(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_PSI_t*            VPPSI_p
);

ST_ErrorCode_t LAYERSTILL_DisableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSTILL_EnableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSTILL_SetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t LAYERSTILL_GetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t LAYERSTILL_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSTILL_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t LAYERSTILL_GetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t LAYERSTILL_SetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t LAYERSTILL_SetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
); 

ST_ErrorCode_t LAYERSTILL_GetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
);
ST_ErrorCode_t LAYERSTILL_GetBitmapAllocParams(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  STGXOBJ_BitmapAllocParams_t* Params1_p,
  STGXOBJ_BitmapAllocParams_t* Params2_p
);

ST_ErrorCode_t LAYERSTILL_GetBitmapHeaderSize(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  U32 *                        HeaderSize_p
);

ST_ErrorCode_t LAYERSTILL_GetPaletteAllocParams(
  STLAYER_Handle_t               LayerHandle,
  STGXOBJ_Palette_t*             Palette_p,
  STGXOBJ_PaletteAllocParams_t*  Params_p
);

ST_ErrorCode_t LAYERSTILL_UpdateFromMixer(
        STLAYER_Handle_t        Handle,
        STLAYER_OutputParams_t * OutputParams_p
);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LAYERSTIL_H */

/* End of lay_stil.h */
