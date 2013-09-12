/*****************************************************************************
 *
 *  Module      : stlayer_core_exports.c
 *  Date        : 13-11-2005
 *  Author      : AYARITAR
 *  Description : Implementation for exporting STAPI functions
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/version.h> /* Kernel version */

#include "stlayer.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

EXPORT_SYMBOL (STLAYER_Init);
EXPORT_SYMBOL (STLAYER_Term);
EXPORT_SYMBOL (STLAYER_Open);
EXPORT_SYMBOL (STLAYER_Close);
EXPORT_SYMBOL (STLAYER_GetCapability);
EXPORT_SYMBOL (STLAYER_GetRevision);
EXPORT_SYMBOL (STLAYER_GetInitAllocParams);
EXPORT_SYMBOL (STLAYER_GetLayerParams);
EXPORT_SYMBOL (STLAYER_SetLayerParams);
EXPORT_SYMBOL (STLAYER_OpenViewPort);
EXPORT_SYMBOL (STLAYER_CloseViewPort);
EXPORT_SYMBOL (STLAYER_EnableViewPort);
EXPORT_SYMBOL (STLAYER_DisableViewPort);
EXPORT_SYMBOL (STLAYER_AdjustViewPortParams);
EXPORT_SYMBOL (STLAYER_SetViewPortParams);
EXPORT_SYMBOL (STLAYER_GetViewPortParams);
EXPORT_SYMBOL (STLAYER_SetViewPortSource);
EXPORT_SYMBOL (STLAYER_GetViewPortSource);
EXPORT_SYMBOL (STLAYER_SetViewPortIORectangle);
EXPORT_SYMBOL (STLAYER_AdjustIORectangle);
EXPORT_SYMBOL (STLAYER_GetViewPortIORectangle);
EXPORT_SYMBOL (STLAYER_SetViewPortPosition);
EXPORT_SYMBOL (STLAYER_GetViewPortPosition);
EXPORT_SYMBOL (STLAYER_SetViewPortPSI);
EXPORT_SYMBOL (STLAYER_GetViewPortPSI);
EXPORT_SYMBOL (STLAYER_SetViewPortSpecialMode);
EXPORT_SYMBOL (STLAYER_GetViewPortSpecialMode);
EXPORT_SYMBOL (STLAYER_DisableColorKey);
EXPORT_SYMBOL (STLAYER_EnableColorKey);
EXPORT_SYMBOL (STLAYER_SetViewPortColorKey);
EXPORT_SYMBOL (STLAYER_GetViewPortColorKey);
EXPORT_SYMBOL (STLAYER_DisableBorderAlpha);
EXPORT_SYMBOL (STLAYER_EnableBorderAlpha);
EXPORT_SYMBOL (STLAYER_GetViewPortAlpha);
EXPORT_SYMBOL (STLAYER_SetViewPortAlpha);
EXPORT_SYMBOL (STLAYER_SetViewPortGain);
EXPORT_SYMBOL (STLAYER_GetViewPortGain);
EXPORT_SYMBOL (STLAYER_SetViewPortRecordable);
EXPORT_SYMBOL (STLAYER_GetViewPortRecordable);
EXPORT_SYMBOL (STLAYER_GetBitmapAllocParams);
EXPORT_SYMBOL (STLAYER_GetBitmapHeaderSize);
EXPORT_SYMBOL (STLAYER_GetPaletteAllocParams);
EXPORT_SYMBOL (STLAYER_GetVTGName);
EXPORT_SYMBOL (STLAYER_GetVTGParams);
EXPORT_SYMBOL (STLAYER_UpdateFromMixer);
EXPORT_SYMBOL (STLAYER_DisableViewPortFilter);
EXPORT_SYMBOL (STLAYER_EnableViewPortFilter);
EXPORT_SYMBOL (STLAYER_AttachAlphaViewPort);
EXPORT_SYMBOL (STLAYER_SetViewPortFlickerFilterMode);
EXPORT_SYMBOL (STLAYER_GetViewPortFlickerFilterMode);
EXPORT_SYMBOL (STLAYER_DisableViewportColorFill);
EXPORT_SYMBOL (STLAYER_EnableViewportColorFill);
EXPORT_SYMBOL (STLAYER_SetViewportColorFill);
EXPORT_SYMBOL (STLAYER_GetViewportColorFill);
EXPORT_SYMBOL (STLAYER_InformPictureToBeDecoded);
EXPORT_SYMBOL (STLAYER_CommitViewPortParams);
#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
EXPORT_SYMBOL (STLAYER_GetVideoDisplayParams);
#endif
#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
EXPORT_SYMBOL (STLAYER_GetRequestedDeinterlacingMode);
EXPORT_SYMBOL (STLAYER_SetRequestedDeinterlacingMode);
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */

#ifdef ST_XVP_ENABLE_FLEXVP
EXPORT_SYMBOL (STLAYER_XVPInit);
EXPORT_SYMBOL (STLAYER_XVPTerm);
EXPORT_SYMBOL (STLAYER_XVPSetParam);
EXPORT_SYMBOL (STLAYER_XVPSetExtraParam);
EXPORT_SYMBOL (STLAYER_XVPCompute);
#endif


