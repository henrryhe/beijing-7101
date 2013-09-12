/*******************************************************************************

File name   : hv_filt.h

Description :

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
16 Dec 2000        Created                                           GGi
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_FILTER_LAYER_OMEGA2_H
#define __HAL_VIDEO_FILTER_LAYER_OMEGA2_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t InitFilter (const STLAYER_Handle_t LayerHandle);

ST_ErrorCode_t InitFilterEvt (const STLAYER_Handle_t LayerHandle,
        const STLAYER_InitParams_t * const  LayerInitParams_p);

ST_ErrorCode_t GetViewportPsiParameters(const STLAYER_Handle_t LayerHandle,
        STLAYER_PSI_t * const ViewportPSI_p);

ST_ErrorCode_t SetViewportPsiParameters(const STLAYER_Handle_t LayerHandle,
        const STLAYER_PSI_t * const ViewportPSI_p);

ST_ErrorCode_t TermFilter (const STLAYER_Handle_t LayerHandle);

ST_ErrorCode_t UpdateViewportPsiParameters (const STLAYER_Handle_t LayerHandle);

void DCIProcessEndOfAnalysis(const STLAYER_Handle_t LayerHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_FILTER_LAYER_OMEGA2_H */

/* End of hv_filt.h */
