/*******************************************************************************

File name   : layerapi.h

Description : STLAYER API:  wrapper

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                               Name
----               ------------                               ----
2000-07-22         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LAYERAPI_H
#define __LAYERAPI_H

/* Includes ----------------------------------------------------------------- */
#include "stlayer.h"
#include  "stos.h"
/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Types -------------------------------------------------------------------- */

typedef struct
{
    STLAYER_Layer_t     LayerType;
    ST_DeviceName_t     LayerName;
    STLAYER_Handle_t    VideoHandle;
}stlayer_LayerElement_t;

typedef struct
{
    stlayer_LayerElement_t *    LayerDevice;
    STLAYER_Handle_t            LowLevelHandle;
    U32                         Used;
}stlayer_OpenElement;

/* Compil flags for the api ------------------------------------------------- */

/* Condition to call the graphic procedures */
/*------------------------------------------*/
/* cases st7015, st7020, GX1: gamma gfx layers are linked */
#if defined HW_7015  || defined HW_7020 || defined HW_GX1 || defined HW_5528 || defined HW_7710 || defined HW_7100 || defined HW_7109  \
|| defined HW_7200

#define GFX_GAMMA

#else
/* cases 55XX + layer archi: omega1 gfx layers are linked */
#if defined HW_5508 || defined HW_5510 || defined HW_5512 \
 || defined HW_5514 || defined HW_5516 || defined HW_5518 \
 || defined HW_5517 || defined HW_5578
#if !defined OLD_ARCHI
#define GFX_55XX

/* cases st55XX : still is linked */
#if defined HW_5510 || defined HW_5512 || defined HW_5514 \
 || defined HW_5516 || defined HW_5517 || defined HW_5578
#define GFX_STILL
#endif

#else
/* cases 55XX + old archi archi: omega1 gfx layers are not linked */
#endif

#else
#if defined HW_5100 || defined HW_5105 || defined HW_5301 || defined HW_5188 || defined HW_5525 || defined HW_5107 || defined HW_5162
#define LAYER_COMPO
#else
#error None chip defined. Use DVD_BACKEND
#endif
#endif
#endif

/* Condition to call the video procedures */
/*----------------------------------------*/
/* cases all chips except the GX1 & sti5100 & sti5105 & sti5188 & sti5107 & sti5162 */
#if !defined HW_GX1 && !defined HW_5100 && !defined HW_5105 && !defined HW_5301 && !defined HW_5188 && !defined HW_5525 && !defined HW_5107\
 && !defined HW_5162
#define VIDEO
#endif

/* Constants ---------------------------------------------------------------- */

#define TOTAL_MAX_LAYER     8       /* to be tuned */
#define MAX_OPEN            3       /* to be tuned */

/* Variables ---------------------------------------------------------------- */

#ifdef VARIABLE_API_RESERVATION
#define VARIABLE_API           /* real reservation   */
#else
#define VARIABLE_API extern    /* extern reservation */
#endif

VARIABLE_API stlayer_LayerElement_t  stlayer_LayerArray[TOTAL_MAX_LAYER];
VARIABLE_API stlayer_OpenElement  stlayer_OpenArray[TOTAL_MAX_LAYER * MAX_OPEN];

/* Exported Functions ------------------------------------------------------- */

U32 stlayer_CleanLayerViewports(stlayer_LayerElement_t * Layer);
void stlayer_SetVPFilter(STLAYER_ViewPortHandle_t VPHandle,
                         STLAYER_ViewPortHandle_t FltHandle);
STLAYER_ViewPortHandle_t stlayer_GetVPFilter(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t stlayer_AttachFilterViewPort(STLAYER_ViewPortHandle_t VPHandle,
                                      STLAYER_ViewPortHandle_t FilteredHandle);

ST_ErrorCode_t stlayer_EnableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle,
                                                STLAYER_Handle_t FilterHandle);
ST_ErrorCode_t stlayer_DisableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle);


/* C++ support */
#ifdef __cplusplus
}
#endif


#endif /* #ifndef __LAYERAPI_H */

