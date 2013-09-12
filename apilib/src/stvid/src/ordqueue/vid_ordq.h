/*******************************************************************************

File name   : vid_ordq.h

Description : Video ordering queue header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
07 Jun 2004        Created                                           CD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_ORDERINGQUEUE_COMMON_H
#define __VIDEO_ORDERINGQUEUE_COMMON_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stevt.h"
#include "stvid.h"
#include "buffers.h"
#include "ordqueue.h"

#include "vid_ctcm.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Macros ---------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

enum
{
    VIDQUEUE_READY_FOR_DISPLAY_EVT_ID = 0,      /* Always set first element as 0 as it will be used for a table */
    VIDQUEUE_PICTURE_CANDIDATE_TO_BE_REMOVED_FROM_DISPLAY_QUEUE_EVT_ID,
    VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID,
    VIDQUEUE_NB_REGISTERED_EVENTS_IDS           /* always keep as last one to count event ID's */
};

/* Number of allocated pool elements at init */
#define ORDQUEUE_NB_ELEM                     (VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES + 2 + MAX_EXTRA_FRAME_BUFFER_NBR) * 2

/* Exported Types ----------------------------------------------------------- */

#ifdef STVID_DEBUG_GET_STATISTICS
typedef struct VIDQUEUE_Statistics_s
{
    U32     PbPictureTooLateRejected;
    U32     PictureInsertedInQueue;
    U32     PicturePushedToDisplay;
    U32     PictureRemovedFromQueue;
} VIDQUEUE_Statistics_t;
#endif

typedef enum VIDQUEUE_RemoveMode_e
{
    VIDQUEUE_REMOVE_UPTO,               /* Remove all elements up to the selected one */
    VIDQUEUE_REMOVE_LAST                /* Remove only the selected element */
} VIDQUEUE_RemoveMode_t;

typedef struct VIDQUEUE_Picture_s
{
    struct  VIDQUEUE_Picture_s  * Next_p;
    VIDBUFF_PictureBuffer_t     * PictureBuffer_p;
} VIDQUEUE_Picture_t;

typedef struct VIDQUEUE_Data_s
{
    ST_Partition_t *    CPUPartition_p;
    STEVT_Handle_t      EventsHandle;
    STEVT_EventID_t     RegisteredEventsID[VIDQUEUE_NB_REGISTERED_EVENTS_IDS];
    VIDBUFF_Handle_t    BufferManagerHandle;
    U32                 ValidityCheck;

    semaphore_t        * OrderingQueueParamsSemaphore_p;

    VIDQUEUE_Picture_t          OrderingQueueElements[ORDQUEUE_NB_ELEM]; /* pool */
    VIDQUEUE_Picture_t        * OrderingQueue_p;
    VIDQUEUE_InsertionOrder_t   InsertionOrder;

    VIDCOM_PictureID_t          LastPushedPictureID;
    BOOL                        IsLastPushedPictureIDValid;

    U32                          PreviousPictPTS;
    U32                          PreviousPictNbFieldsDisplay;
    BOOL                        FirstPictureEver;

    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping; /* Needed to translate data1 and data2 in NEW_PICTURE_ORDERED_EVT */
    /* Ordered event notification configuration ----------------------------- */
    struct
    {
        STVID_ConfigureEventParams_t ConfigureEventParam;    /* Contains informations : Event enable/disable notification
                                                            and Number of event notification to skip before notifying the event. */
        U32                          NotificationSkipped;    /* Number of notification skipped since the last one. */
    } EventNotificationConfiguration [VIDQUEUE_NB_REGISTERED_EVENTS_IDS];

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDQUEUE_Statistics_t       Statistics;
#endif
} VIDQUEUE_Data_t;

/* Exported Functions ------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_ORDERINGQUEUE_COMMON_H */

/* End of vid_ordq.h */
