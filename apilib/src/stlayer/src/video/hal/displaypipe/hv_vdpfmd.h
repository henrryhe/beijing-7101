/*******************************************************************************

File name   : hv_vdpfmd.h

Description :

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                 ----
02 Nov 2006       Created                                          OL
*******************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __HAL_VDP_FMD_H
#define __HAL_VDP_FMD_H

#ifdef STLAYER_USE_FMD
/* Includes ------------------------------------------------------------ */

/* Private Macros ----------------------------------------------------- */

/* Exported Types ------------------------------------------------------ */

/* Exported Constants -------------------------------------------------- */

/* Exported Variables -------------------------------------------------- */

/* Exported Macros ----------------------------------------------------- */

/* Exported Functions -------------------------------------------------- */

ST_ErrorCode_t vdp_fmd_Init (STLAYER_Handle_t   LayerHandle);
ST_ErrorCode_t vdp_fmd_Term (const STLAYER_Handle_t LayerHandle);
ST_ErrorCode_t vdp_fmd_SetFmdThreshold (const STLAYER_Handle_t LayerHandle, const STLAYER_FMDParams_t * Params_p);
ST_ErrorCode_t vdp_fmd_GetFmdThreshold (const STLAYER_Handle_t LayerHandle, STLAYER_FMDParams_t * Params_p);
void vdp_fmd_CheckFmdSetup(const STLAYER_Handle_t LayerHandle);
ST_ErrorCode_t vdp_fmd_EnableFmd (const STLAYER_Handle_t LayerHandle, BOOL FmdEnabled);
BOOL vdp_fmd_GetFmdResults(const STLAYER_Handle_t LayerHandle, STLAYER_FMDResults_t *FMDResults_p);

#endif /* #ifdef STLAYER_USE_FMD  */

#endif /* #ifndef __HAL_VDP_FMD_H */
/* End of hv_vdpfmd.h */
