/*****************************************************************************
 *
 *  Module      : stvid_core_exports.c
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description : Implementation for exporting STAPI functions
 *  Copyright   : STMicroelectronics (c)2006
 *****************************************************************************/


/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/version.h> /* Kernel version */

#include "stvid.h"
#ifdef STUB_INJECT
#include "inject.h"
#endif /* STUB_INJECT */

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

/* Exported STVID functions */
EXPORT_SYMBOL (STVID_Init);
EXPORT_SYMBOL (STVID_Open);
EXPORT_SYMBOL (STVID_Close);
EXPORT_SYMBOL (STVID_Term);
EXPORT_SYMBOL (STVID_GetRevision);
EXPORT_SYMBOL (STVID_GetCapability);
#ifdef STVID_DEBUG_GET_STATISTICS
EXPORT_SYMBOL (STVID_GetStatistics);
EXPORT_SYMBOL (STVID_ResetStatistics);
#endif
#ifdef STVID_DEBUG_GET_STATUS
EXPORT_SYMBOL (STVID_GetStatus);
#endif
EXPORT_SYMBOL (STVID_Clear);
EXPORT_SYMBOL (STVID_CloseViewPort);
EXPORT_SYMBOL (STVID_ConfigureEvent);
EXPORT_SYMBOL (STVID_DataInjectionCompleted);
EXPORT_SYMBOL (STVID_DeleteDataInputInterface);
EXPORT_SYMBOL (STVID_DisableBorderAlpha);
EXPORT_SYMBOL (STVID_DisableColorKey);
EXPORT_SYMBOL (STVID_DisableDeblocking);
EXPORT_SYMBOL (STVID_DisableDisplay);
EXPORT_SYMBOL (STVID_DisableFrameRateConversion);
EXPORT_SYMBOL (STVID_DisableOutputWindow);
EXPORT_SYMBOL (STVID_DisableSynchronisation);
EXPORT_SYMBOL (STVID_DuplicatePicture);
EXPORT_SYMBOL (STVID_EnableBorderAlpha);
EXPORT_SYMBOL (STVID_EnableColorKey);
EXPORT_SYMBOL (STVID_EnableDeblocking);
EXPORT_SYMBOL (STVID_EnableDisplay);
EXPORT_SYMBOL (STVID_EnableFrameRateConversion);
EXPORT_SYMBOL (STVID_EnableOutputWindow);
EXPORT_SYMBOL (STVID_EnableSynchronisation);
EXPORT_SYMBOL (STVID_Freeze);
EXPORT_SYMBOL (STVID_GetAlignIOWindows);
EXPORT_SYMBOL (STVID_GetBitBufferFreeSize);
EXPORT_SYMBOL (STVID_GetBitBufferParams);
EXPORT_SYMBOL (STVID_GetDataInputBufferParams);
EXPORT_SYMBOL (STVID_GetDecodedPictures);
EXPORT_SYMBOL (STVID_GetDisplayAspectRatioConversion);
EXPORT_SYMBOL (STVID_GetErrorRecoveryMode);
EXPORT_SYMBOL (STVID_GetHDPIPParams);
EXPORT_SYMBOL (STVID_GetInputWindowMode);
EXPORT_SYMBOL (STVID_GetIOWindows);
EXPORT_SYMBOL (STVID_GetMemoryProfile);
EXPORT_SYMBOL (STVID_GetOutputWindowMode);
EXPORT_SYMBOL (STVID_GetPictureAllocParams);
EXPORT_SYMBOL (STVID_GetPictureParams);
EXPORT_SYMBOL (STVID_GetPictureInfos);
EXPORT_SYMBOL (STVID_GetDisplayPictureInfo);
EXPORT_SYMBOL (STVID_GetSpeed);
EXPORT_SYMBOL (STVID_GetViewPortAlpha);
EXPORT_SYMBOL (STVID_GetViewPortColorKey);
EXPORT_SYMBOL (STVID_GetViewPortPSI);
EXPORT_SYMBOL (STVID_GetViewPortSpecialMode);
EXPORT_SYMBOL (STVID_HidePicture);
EXPORT_SYMBOL (STVID_InjectDiscontinuity);
EXPORT_SYMBOL (STVID_OpenViewPort);
EXPORT_SYMBOL (STVID_Pause);
EXPORT_SYMBOL (STVID_PauseSynchro);
EXPORT_SYMBOL (STVID_SetViewPortQualityOptimizations);
EXPORT_SYMBOL (STVID_GetViewPortQualityOptimizations);

#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
EXPORT_SYMBOL (STVID_GetRequestedDeinterlacingMode);
EXPORT_SYMBOL (STVID_SetRequestedDeinterlacingMode);
#endif
EXPORT_SYMBOL (STVID_Resume);
EXPORT_SYMBOL (STVID_SetDataInputInterface);
EXPORT_SYMBOL (STVID_SetDecodedPictures);
EXPORT_SYMBOL (STVID_SetDisplayAspectRatioConversion);
EXPORT_SYMBOL (STVID_SetErrorRecoveryMode);
EXPORT_SYMBOL (STVID_SetHDPIPParams);
EXPORT_SYMBOL (STVID_SetInputWindowMode);
EXPORT_SYMBOL (STVID_SetIOWindows);
EXPORT_SYMBOL (STVID_SetMemoryProfile);
EXPORT_SYMBOL (STVID_SetOutputWindowMode);
EXPORT_SYMBOL (STVID_SetSpeed);
EXPORT_SYMBOL (STVID_Setup);
EXPORT_SYMBOL (STVID_SetViewPortAlpha);
EXPORT_SYMBOL (STVID_SetViewPortColorKey);
EXPORT_SYMBOL (STVID_SetViewPortPSI);
EXPORT_SYMBOL (STVID_SetViewPortSpecialMode);
EXPORT_SYMBOL (STVID_ShowPicture);
EXPORT_SYMBOL (STVID_SkipSynchro);
EXPORT_SYMBOL (STVID_Start);
EXPORT_SYMBOL (STVID_StartUpdatingDisplay);
EXPORT_SYMBOL (STVID_Step);
EXPORT_SYMBOL (STVID_Stop);
EXPORT_SYMBOL (STVID_ForceDecimationFactor);
EXPORT_SYMBOL (STVID_GetDecimationFactor);
#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
EXPORT_SYMBOL (STVID_GetVideoDisplayParams);
#endif

#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
EXPORT_SYMBOL (STVID_GetSynchronizationDelay);
EXPORT_SYMBOL (STVID_SetSynchronizationDelay);
#endif
EXPORT_SYMBOL (STVID_GetPictureBuffer);
EXPORT_SYMBOL (STVID_ReleasePictureBuffer);
EXPORT_SYMBOL (STVID_DisplayPictureBuffer);
EXPORT_SYMBOL (STVID_TakePictureBuffer);
#ifdef STUB_INJECT
EXPORT_SYMBOL (StubInject_SetStreamSize);
EXPORT_SYMBOL (StubInject_GetStreamSize);
EXPORT_SYMBOL (StubInject_GetBBInfo);
EXPORT_SYMBOL (StubInject_SetBBInfo);
#endif
#ifdef ST_XVP_ENABLE_FLEXVP
EXPORT_SYMBOL (STVID_ActivateXVPProcess);
EXPORT_SYMBOL (STVID_DeactivateXVPProcess);
EXPORT_SYMBOL (STVID_ShowXVPProcess);
EXPORT_SYMBOL (STVID_HideXVPProcess);
#endif /* ST_XVP_ENABLE_FLEXVP */
#ifdef STVID_USE_CRC
EXPORT_SYMBOL (STVID_CRCStartQueueing);
EXPORT_SYMBOL (STVID_CRCStopQueueing);
EXPORT_SYMBOL (STVID_CRCGetQueue);
EXPORT_SYMBOL (STVID_CRCStartCheck);
EXPORT_SYMBOL (STVID_CRCStopCheck);
EXPORT_SYMBOL (STVID_CRCGetCurrentResults);
#endif
/* end of stvid_core_exports.c */
