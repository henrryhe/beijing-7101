/*******************************************************************************
File name   : speed.h

Description : Video speed header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
10 Jan 2006       Created                                           RH

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_SPEED_H
#define __VIDEO_SPEED_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

#include "stvid.h"

#include "buffers.h"

#ifdef ST_ordqueue
#include "ordqueue.h"
#endif /* ST_ordqueue */

#include "producer.h"

#ifdef ST_display
#include "display.h"
#endif /* ST_display */

#include "stavmem.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define VIDSPEED_MODULE_BASE   0x800

enum
{
    /* This event passes a (VIDDISP_DisplayParams_t *) as parameter */
    VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT = STVID_DRIVER_BASE + VIDSPEED_MODULE_BASE
};

/* Exported Types ----------------------------------------------------------- */
typedef void * VIDSPEED_Handle_t;

typedef struct VIDSPEED_InitParams_s
{
    ST_Partition_t *            CPUPartition_p;
    ST_DeviceName_t             EventsHandlerName;
    VIDPROD_Handle_t            ProducerHandle;
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
} VIDSPEED_InitParams_t;


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t VIDSPEED_DisableSpeedControl(const VIDSPEED_Handle_t SpeedHandle);
ST_ErrorCode_t VIDSPEED_EnableSpeedControl(const VIDSPEED_Handle_t SpeedHandle);
ST_ErrorCode_t VIDSPEED_GetSpeed(const VIDSPEED_Handle_t SpeedHandle, S32 * const Speed_p);
ST_ErrorCode_t VIDSPEED_Init(const VIDSPEED_InitParams_t * const InitParams_p, VIDSPEED_Handle_t * const SpeedHandle_p);
ST_ErrorCode_t VIDSPEED_SetSpeed(const VIDSPEED_Handle_t SpeedHandle, const S32 Speed);
ST_ErrorCode_t VIDSPEED_Stop(const VIDSPEED_Handle_t SpeedHandle, const STVID_Stop_t StopMode, BOOL * const SpeedWillStopDecode_p, const STVID_Freeze_t * const Freeze_p);
ST_ErrorCode_t VIDSPEED_Term(const VIDSPEED_Handle_t SpeedHandle);
ST_ErrorCode_t VIDSPEED_DisplayManagedBySpeed(const VIDSPEED_Handle_t SpeedHandle, const BOOL DisplayManagedBySpeed);
ST_ErrorCode_t VIDSPEED_VsyncActionSpeed(const VIDSPEED_Handle_t SpeedHandle, BOOL IsReferenceDisplay);
ST_ErrorCode_t VIDSPEED_PostVsyncAction(const VIDSPEED_Handle_t SpeedHandle, BOOL IsReferenceDisplay);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VIDSPEED_GetStatistics(const VIDSPEED_Handle_t SpeedHandle, STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t VIDSPEED_ResetStatistics(const VIDSPEED_Handle_t SpeedHandle, const STVID_Statistics_t * const Statistics_p);
#endif /*STVID_DEBUG_GET_STATISTICS*/
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_SPEED_H */

/* End of speed.h */

