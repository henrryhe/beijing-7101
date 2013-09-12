/*******************************************************************************

File name   : ordqueue.h

Description : Video reordering queue header file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
07 Jun 2004        Created from producer module                      CD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_QUEUE_H
#define __VIDEO_QUEUE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"
#include "buffers.h"

#include "vid_com.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define VIDQUEUE_MODULE_BASE   0xD00

enum
{
    /* This event passes a (VIDQUEUE_ReadyForDisplayParams_t *) as parameter */
    VIDQUEUE_READY_FOR_DISPLAY_EVT = STVID_DRIVER_BASE + VIDQUEUE_MODULE_BASE,
    /* This event passes a (VIDQUEUE_PictureCandidateToBeRemovedParams_t *) as parameter */
    VIDQUEUE_PICTURE_CANDIDATE_TO_BE_REMOVED_FROM_DISPLAY_QUEUE_EVT
};

typedef enum VIDQUEUE_InsertionOrder_e
{
    VIDQUEUE_INSERTION_ORDER_INCREASING, /* Insert picture at right place in ExtendedPresentationOrderPictureID increasing order */
    VIDQUEUE_INSERTION_ORDER_DECREASING, /* Insert picture at right place in ExtendedPresentationOrderPictureID decreasing order */
    VIDQUEUE_INSERTION_ORDER_LAST_PLACE  /* Insert picture at last place in queue */
} VIDQUEUE_InsertionOrder_t;

/* Exported Types ----------------------------------------------------------- */

typedef void * VIDQUEUE_Handle_t;

typedef struct VIDQUEUE_ReadyForDisplayParams_s
{
    VIDBUFF_PictureBuffer_t   * Picture_p;      /* Picture to queue on display */
} VIDQUEUE_ReadyForDisplayParams_t;

typedef struct VIDQUEUE_PictureCandidateToBeRemovedParams_s
{
    VIDCOM_PictureID_t          PictureID;      /* Picture to remove on display */
} VIDQUEUE_PictureCandidateToBeRemovedParams_t;

typedef struct
{
    ST_Partition_t *            CPUPartition_p;
    ST_DeviceName_t             EventsHandlerName;

    VIDBUFF_Handle_t            BufferManagerHandle;
    ST_DeviceName_t             VideoName;              /* Name of the video instance */
} VIDQUEUE_InitParams_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t VIDQUEUE_Init(const VIDQUEUE_InitParams_t * const InitParams_p, VIDQUEUE_Handle_t * const OrderingQueueHandle_p);
ST_ErrorCode_t VIDQUEUE_Term(const VIDQUEUE_Handle_t OrderingQueueHandle);

ST_ErrorCode_t VIDQUEUE_InsertPictureInQueue(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDBUFF_PictureBuffer_t * Picture_p, VIDQUEUE_InsertionOrder_t InsertionOrder);
ST_ErrorCode_t VIDQUEUE_PushPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDCOM_PictureID_t * const PictureID_p);
ST_ErrorCode_t VIDQUEUE_PushAllPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle);

ST_ErrorCode_t VIDQUEUE_CancelPicture(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDCOM_PictureID_t * const PictureID_p);
ST_ErrorCode_t VIDQUEUE_CancelUpToPicture(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDCOM_PictureID_t * const PictureID_p);
ST_ErrorCode_t VIDQUEUE_CancelAllPictures(const VIDQUEUE_Handle_t OrderingQueueHandle);

ST_ErrorCode_t VIDQUEUE_ConfigureEvent(const VIDQUEUE_Handle_t OrderingQueueHandle, const STEVT_EventID_t Event, const STVID_ConfigureEventParams_t * const Params_p);

#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VIDQUEUE_GetStatistics(const VIDQUEUE_Handle_t OrderingQueueHandle, STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t VIDQUEUE_ResetStatistics(const VIDQUEUE_Handle_t OrderingQueueHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_QUEUE_H */

/* End of ordqueue.h */
