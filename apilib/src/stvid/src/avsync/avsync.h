/*******************************************************************************

File name   : avsync.h

Description : Video AVSYNC module header file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
 7 Sep 2000        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_AVSYNC_H
#define __VIDEO_AVSYNC_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#ifdef ST_producer
#include "producer.h"
#endif /* ST_producer */

#ifdef ST_display
#include "display.h"
#endif /* ST_display */


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define VIDAVSYNC_MODULE_BASE   0x300

enum
{
    /* This evt passes an U32 as parameter. It's a delay that will be respected
       before displaying the next picture */
    VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT = STVID_DRIVER_BASE + VIDAVSYNC_MODULE_BASE
};

/* Exported Types ----------------------------------------------------------- */

typedef void * VIDAVSYNC_Handle_t;

typedef enum VIDAVSYNC_SynchroType_e
{
    VIDAVSYNC_CHECK_SYNCHRO_ON_PICTURE_CURRENTLY_DISPLAYED,
    VIDAVSYNC_CHECK_SYNCHRO_ON_PREPARED_PICTURE_FOR_NEXT_VSYNC
} VIDAVSYNC_SynchroType_t;

typedef struct
{
    ST_Partition_t *        CPUPartition_p;
    ST_DeviceName_t         EventsHandlerName;
    STVID_DeviceType_t      DeviceType;
    VIDAVSYNC_SynchroType_t SynchroType;
    ST_DeviceName_t         ClockRecoveryName;
#ifdef ST_display
    VIDDISP_Handle_t        DisplayHandle;
#endif /* ST_display */
#ifdef ST_producer
    BOOL                    IsDecodeActive;
    VIDPROD_Handle_t        DecodeHandle;
#endif /* ST_producer */
    U32                     AVSYNCDriftThreshold;
    ST_DeviceName_t         VideoName;          /* Name of the video instance */
} VIDAVSYNC_InitParams_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t VIDAVSYNC_ConfigureEvent(const VIDAVSYNC_Handle_t AVSyncHandle, const STEVT_EventID_t Event, const STVID_ConfigureEventParams_t * const Params_p);
ST_ErrorCode_t VIDAVSYNC_DisableSynchronization(const VIDAVSYNC_Handle_t AVSyncHandle);
ST_ErrorCode_t VIDAVSYNC_EnableSynchronization(const VIDAVSYNC_Handle_t AVSyncHandle);
ST_ErrorCode_t VIDAVSYNC_Init(const VIDAVSYNC_InitParams_t * const InitParams_p, VIDAVSYNC_Handle_t * const AVSyncHandle_p);
BOOL           VIDAVSYNC_IsSynchronizationEnabled(const VIDAVSYNC_Handle_t AVSyncHandle);
ST_ErrorCode_t VIDAVSYNC_ResetSynchronization(const VIDAVSYNC_Handle_t AVSyncHandle);
ST_ErrorCode_t VIDAVSYNC_ResumeSynchronizationActions(const VIDAVSYNC_Handle_t AVSyncHandle);
ST_ErrorCode_t VIDAVSYNC_SetSynchronizationDelay(const VIDAVSYNC_Handle_t AVSyncHandle, const S32 SynchronizationDelay);
#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
ST_ErrorCode_t VIDAVSYNC_GetSynchronizationDelay(const VIDAVSYNC_Handle_t AVSyncHandle, S32 * const SynchronizationDelay_p);
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */
ST_ErrorCode_t VIDAVSYNC_SuspendSynchronizationActions(const VIDAVSYNC_Handle_t AVSyncHandle);
ST_ErrorCode_t VIDAVSYNC_Term(const VIDAVSYNC_Handle_t AVSyncHandle);
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VIDAVSYNC_GetStatistics(const VIDAVSYNC_Handle_t AVSyncHandle, STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t VIDAVSYNC_ResetStatistics(const VIDAVSYNC_Handle_t AVSyncHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
ST_ErrorCode_t VIDAVSYNC_GetStatus(const VIDAVSYNC_Handle_t AVSyncHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */




/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_AVSYNC_H */

/* End of avsync.h */
