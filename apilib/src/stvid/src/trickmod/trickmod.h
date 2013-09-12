/*******************************************************************************
File name   : trickmod.h

Description : Video trickmod header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
14 Sept 2000       Created                                           PLe
29 Jan 2002        Added event for display params                    HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_TRICKMOD_H
#define __VIDEO_TRICKMOD_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "buffers.h"

#ifdef ST_ordqueue
#include "ordqueue.h"
#endif /* ST_ordqueue */

#include "decode.h"

#ifdef ST_display
#include "display.h"
#endif /* ST_display */

#include "stavmem.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define VIDTRICK_MODULE_BASE   0x800

enum
{
    /* This event passes a (VIDDISP_DisplayParams_t *) as parameter */
    VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT = STVID_DRIVER_BASE + VIDTRICK_MODULE_BASE
};

/* Exported Types ----------------------------------------------------------- */
typedef void * VIDTRICK_Handle_t;

typedef struct VIDTRICK_InitParams_s
{
    ST_Partition_t *            CPUPartition_p;
    ST_DeviceName_t             EventsHandlerName;
    VIDPROD_Handle_t            DecodeHandle;
    VIDDISP_Handle_t            DisplayHandle;
#ifdef ST_smoothbackward
    VIDBUFF_Handle_t            BufferManagerHandle;
#endif
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
#ifdef ST_ordqueue
    VIDQUEUE_Handle_t           OrderingQueueHandle;
#endif /* ST_ordqueue */
    ST_DeviceName_t             VideoName;      /* Name of the video instance */
    U8                          DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    U8                          DisplayDeviceAdditionnalFieldLatency; /* Additional number of fields to insert in the display queue */
} VIDTRICK_InitParams_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
#ifdef ST_smoothbackward
ST_ErrorCode_t VIDTRICK_DataInjectionCompleted(const VIDTRICK_Handle_t TrickModeHandle, const STVID_DataInjectionCompletedParams_t * const DataInjectionCompletedParams_p);
#endif
ST_ErrorCode_t VIDTRICK_DisableSpeedControl(const VIDTRICK_Handle_t TrickModeHandle);
ST_ErrorCode_t VIDTRICK_EnableSpeedControl(const VIDTRICK_Handle_t TrickModeHandle);
ST_ErrorCode_t VIDTRICK_GetSpeed(const VIDTRICK_Handle_t TrickModeHandle, S32 * const Speed_p);
ST_ErrorCode_t VIDTRICK_Init(const VIDTRICK_InitParams_t * const InitParams_p, VIDTRICK_Handle_t * const TrickModeHandle_p);
ST_ErrorCode_t VIDTRICK_SetSpeed(const VIDTRICK_Handle_t TrickModeHandle, const S32 Speed);
ST_ErrorCode_t VIDTRICK_GetBitBufferFreeSize(const VIDTRICK_Handle_t TrickModeHandle, U32 * const FreeSize_p);
ST_ErrorCode_t VIDTRICK_Start(const VIDTRICK_Handle_t TrickModeHandle);
ST_ErrorCode_t VIDTRICK_Stop(const VIDTRICK_Handle_t TrickModeHandle, const STVID_Stop_t StopMode, BOOL * const TrickModeWillStopDecode_p, const STVID_Freeze_t * const Freeze_p);
ST_ErrorCode_t VIDTRICK_Term(const VIDTRICK_Handle_t TrickModeHandle);
ST_ErrorCode_t VIDTRICK_VsyncActionSpeed(const VIDTRICK_Handle_t TrickModeHandle, BOOL IsReferenceDisplay);
ST_ErrorCode_t VIDTRICK_PostVsyncAction(const VIDTRICK_Handle_t TrickModeHandle, BOOL IsReferenceDisplay);
ST_ErrorCode_t VIDTRICK_SetDecodedPictures(const VIDTRICK_Handle_t TrickModeHandle, const STVID_DecodedPictures_t DecodedPictures);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDTRICK_DECODE_H */

/* End of trickmod.h */







