/*******************************************************************************

File name   : vid_ordq.c

Description : Video Ordering queue manager source file.

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
07 june 2004       Created from vid_queu.c                           CD
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "ordqueue.h"
#include "vid_ordq.h"

#include "sttbx.h"


/* #define TRACE_QUEUE */


#ifdef TRACE_QUEUE /* define or uncomment in ./makefile to set traces */
  #include "trace.h"
#else
  #define TraceBuffer(x)
  #define TracePicture(x)
#endif


/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDQUEUE_VALID_HANDLE 0x01D51D50


/* Private Macros -------------------------------------------------------- */

#ifdef TRACE_QUEUE
#define  MPEGFrame2Char(Type)  (                                        \
               (  (Type) == STVID_MPEG_FRAME_I)                         \
                ? 'I' : (  ((Type) == STVID_MPEG_FRAME_P)               \
                         ? 'P' : (  ((Type) == STVID_MPEG_FRAME_B)      \
                                  ? 'B'  : 'U') ) )
#define  PictureStruct2String(PicStruct)  (                                                            \
               (  (PicStruct) == STVID_PICTURE_STRUCTURE_FRAME)                                        \
                ? "Frm" : (  ((PicStruct) == STVID_PICTURE_STRUCTURE_TOP_FIELD)                        \
                           ? "Top" : (  ((PicStruct) == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)          \
                                      ? "Bot"  : "---") ) )
#endif

#define Virtual(DevAdd) (void*)((U32)(DevAdd)\
        -(U32) (VIDQUEUE_Data_p->VirtualMapping.PhysicalAddressSeenFromDevice_p)\
        +(U32) (VIDQUEUE_Data_p->VirtualMapping.VirtualBaseAddress_p))

/* Private Functions prototypes --------------------------------------------- */

static void InitQueueElementsPool(const VIDQUEUE_Handle_t OrderingQueueHandle);
static VIDQUEUE_Picture_t * GetFreePictureElement(const VIDQUEUE_Handle_t OrderingQueueHandle);
static void FreesPictureElement(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDQUEUE_Picture_t * const CurrentPicture_p);
static BOOL PictureIsBeyondPictureID(VIDQUEUE_InsertionOrder_t  InsertionOrder,
                                     VIDQUEUE_Picture_t       * CurrentPicture_p,
                                     VIDCOM_PictureID_t       * PictureID_p,
                                     BOOL                     * PictureFound_p);

static BOOL RemovePicturesFromQueue(const VIDQUEUE_Handle_t OrderingQueueHandle,
                                    VIDCOM_PictureID_t * const PictureID_p,
                                    VIDQUEUE_RemoveMode_t RemoveMode);
static void RemoveAllPicturesFromQueue(const VIDQUEUE_Handle_t OrderingQueueHandle);
static BOOL InsertPictureInReorderingQueue(const VIDQUEUE_Handle_t OrderingQueueHandle,
                                           const VIDQUEUE_InsertionOrder_t InsertionOrder,
                                           VIDQUEUE_Picture_t * const Picture_p);
static void ComputePTS(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDQUEUE_Picture_t * Picture_p);
static void PushPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDCOM_PictureID_t * const PictureID_p);
static void PushAllPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle);
static BOOL vidqueue_IsEventToBeNotified(const VIDQUEUE_Handle_t OrderingQueueHandle, const U32 Event_ID);

#ifdef TRACE_QUEUE
static void TracePicture(VIDQUEUE_Picture_t * pic_p);
static void TraceList(const VIDQUEUE_Handle_t OrderingQueueHandle);
#endif
/* Public Functions ------------------------------------------------------------ */

/*******************************************************************************
Name        : VIDQUEUE_Init
Description : Init reordering queue manager module
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_Init(const VIDQUEUE_InitParams_t * const InitParams_p, VIDQUEUE_Handle_t * const OrderingQueueHandle_p)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p;
    STEVT_OpenParams_t  STEVT_OpenParams;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    U32 LoopCounter;

    if ((InitParams_p == NULL) || (OrderingQueueHandle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /****************/
    /* Generic data */
    /****************/

    /* Allocate a VIDQUEUE structure */
    SAFE_MEMORY_ALLOCATE(VIDQUEUE_Data_p, VIDQUEUE_Data_t *, InitParams_p->CPUPartition_p, sizeof(VIDQUEUE_Data_t) );
    if (VIDQUEUE_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Remember partition */
    VIDQUEUE_Data_p->CPUPartition_p  = InitParams_p->CPUPartition_p;

    /* Allocations succeeded: make handle point on structure */
    *OrderingQueueHandle_p = (VIDQUEUE_Handle_t *) VIDQUEUE_Data_p;
    VIDQUEUE_Data_p->ValidityCheck = VIDQUEUE_VALID_HANDLE;

    InitQueueElementsPool(*OrderingQueueHandle_p);
    VIDQUEUE_Data_p->OrderingQueue_p = NULL;
    VIDQUEUE_Data_p->InsertionOrder = VIDQUEUE_INSERTION_ORDER_INCREASING;
    VIDQUEUE_Data_p->IsLastPushedPictureIDValid = FALSE;

#ifdef STVID_DEBUG_GET_STATISTICS
    /* reset all statistics */
    memset((U8 *)&VIDQUEUE_Data_p->Statistics, 0, sizeof(VIDQUEUE_Statistics_t));
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Store parameters */
    VIDQUEUE_Data_p->BufferManagerHandle  = InitParams_p->BufferManagerHandle;

    VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p = semaphore_create_priority(1);

    /* Get AV mem mapping */
    STAVMEM_GetSharedMemoryVirtualMapping(&(VIDQUEUE_Data_p->VirtualMapping));
    /* Ordered event notification configuration ----------------------------- */

    /* Initialise the event notification configuration. */
    for (LoopCounter = 0; LoopCounter < VIDQUEUE_NB_REGISTERED_EVENTS_IDS; LoopCounter++)
    {
        VIDQUEUE_Data_p->EventNotificationConfiguration[LoopCounter].NotificationSkipped                     = 0;
        VIDQUEUE_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.NotificationsToSkip = 0;
        VIDQUEUE_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.Enable              = TRUE;
    }
    /* Set the default value of specific event. */
    VIDQUEUE_Data_p->EventNotificationConfiguration[VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID].ConfigureEventParam.Enable = FALSE;

    /****************/
    /*    Events    */
    /****************/

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &VIDQUEUE_Data_p->EventsHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Ordering Queue manager init: could not open EVT !"));
        VIDQUEUE_Term(*OrderingQueueHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDQUEUE_Data_p->EventsHandle, InitParams_p->VideoName, VIDQUEUE_READY_FOR_DISPLAY_EVT, &(VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_READY_FOR_DISPLAY_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDQUEUE_Data_p->EventsHandle, InitParams_p->VideoName, VIDQUEUE_PICTURE_CANDIDATE_TO_BE_REMOVED_FROM_DISPLAY_QUEUE_EVT, &(VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_PICTURE_CANDIDATE_TO_BE_REMOVED_FROM_DISPLAY_QUEUE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDQUEUE_Data_p->EventsHandle, InitParams_p->VideoName, STVID_NEW_PICTURE_ORDERED_EVT, &(VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID]));
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        TraceBuffer(("Evt Registration ERROR !!!\r\n"));
        VIDQUEUE_Term(*OrderingQueueHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

#ifdef TRACE_QUEUE
    TraceBuffer(("\r\n************************\r\n"));
    TraceBuffer(("*   QUEUE traces OK    *\r\n"));
    TraceBuffer(("************************\r\n"));
#endif

    return (ErrorCode);
} /* end of VIDQUEUE_Init() */

/*******************************************************************************
Name        : VIDQUEUE_Term
Description : Terminate reordering queue manager module
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_Term(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (VIDQUEUE_Data_p->ValidityCheck != VIDQUEUE_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(VIDQUEUE_Data_p->EventsHandle);

    /* reinit element pool */
    InitQueueElementsPool(OrderingQueueHandle);

    /* semaphore delete */
    semaphore_delete(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

    /* De-validate structure */
    VIDQUEUE_Data_p->ValidityCheck = 0; /* not VIDQUEUE_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDQUEUE_Data_p, VIDQUEUE_Data_p->CPUPartition_p, sizeof(VIDQUEUE_Data_t));

    return (ErrorCode);
} /* end of VIDQUEUE_Term() */

/*******************************************************************************
Name        : VIDQUEUE_InsertPicture
Description : Insert picture in the reordering queue
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_InsertPictureInQueue(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDBUFF_PictureBuffer_t * Picture_p, VIDQUEUE_InsertionOrder_t InsertionOrder)
{
    VIDQUEUE_Data_t       * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t    * PictureElem_p;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (Picture_p == NULL)
    {
        /* Nothing to insert: nothing to do */
        return(ST_NO_ERROR);
    }

    /* Lock Access to some parameters of Ordering queue data */
    semaphore_wait(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

    /* Get a free queue element */
    PictureElem_p = GetFreePictureElement(OrderingQueueHandle);
    if (PictureElem_p != NULL)
    {
        /* Attach the picture buffer to the ordering queue element */
        PictureElem_p->PictureBuffer_p = Picture_p;
        PictureElem_p->Next_p = NULL;

#if defined(ST_multicodec)
		/* Mark the picture as not pushed */
        Picture_p->IsPushed = FALSE;
#endif

        /* Orders picture in queue */
        if (! InsertPictureInReorderingQueue(OrderingQueueHandle, InsertionOrder, PictureElem_p))
        {
            /* Picture has not been inserted */
            /* Frees element */
            PictureElem_p->PictureBuffer_p = NULL;
            PictureElem_p->Next_p = NULL;
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        TraceBuffer(("Display pool is not big enough !!\r\n"));

        /* Not enough allocated pool elements */
        ErrorCode = ST_ERROR_NO_MEMORY;
    }

    /* Unlock Access to some parameters of Ordering queue data */
    semaphore_signal(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

    return(ErrorCode);
} /* end of VIDQUEUE_InsertPictureInQueue() */

/*******************************************************************************
Name        : VIDQUEUE_PushPicturesToDisplay
Description : Send all pictures up to a given picture to the display
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_PushPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDCOM_PictureID_t * const PictureID_p)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (PictureID_p != NULL)
    {
        /* Lock Access to some parameters of Ordering queue data */
        semaphore_wait(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

        PushPicturesToDisplay(OrderingQueueHandle, PictureID_p);

        /* Unlock Access to some parameters of Ordering queue data */
        semaphore_signal(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    return (ErrorCode);
} /* end of VIDQUEUE_PushPicturesToDisplay() */

/*******************************************************************************
Name        : VIDQUEUE_PushAllPicturesToDisplay
Description : Send all pending pictures to the display
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_PushAllPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    /* Lock Access to some parameters of Ordering queue data */
    semaphore_wait(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

    PushAllPicturesToDisplay(OrderingQueueHandle);

    /* Unlock Access to some parameters of Ordering queue data */
    semaphore_signal(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

    return (ErrorCode);
} /* end of VIDQUEUE_PushAllPicturesToDisplay() */

/*******************************************************************************
Name        : VIDQUEUE_CancelPicture
Description : Erase a given picture from the reordering queue
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_CancelPicture(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDCOM_PictureID_t * const PictureID_p)
{
    VIDQUEUE_Data_t                               * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_PictureCandidateToBeRemovedParams_t    Param;
    ST_ErrorCode_t                                  ErrorCode = ST_NO_ERROR;
    BOOL                                            Found;

    if (PictureID_p != NULL)
    {
        /* Lock Access to some parameters of Ordering queue data */
        semaphore_wait(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

        Found = RemovePicturesFromQueue(OrderingQueueHandle, PictureID_p, VIDQUEUE_REMOVE_LAST);

        /* Unlock Access to some parameters of Ordering queue data */
        semaphore_signal(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

        if (! Found)
        {
            /* Picture has not been found in ordering queue */
            /* it is probably in the display queue          */
            Param.PictureID = *PictureID_p;

            /* Signal new picture to outside (display should have subscribed !) */
            STEVT_Notify(VIDQUEUE_Data_p->EventsHandle, VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_PICTURE_CANDIDATE_TO_BE_REMOVED_FROM_DISPLAY_QUEUE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &Param);
        }
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    return (ErrorCode);
} /* end of VIDQUEUE_CancelPicture() */

/*******************************************************************************
Name        : VIDQUEUE_CancelUpToPicture
Description : Erase all pictures up to a given picture from the reordering queue
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_CancelUpToPicture(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDCOM_PictureID_t * const PictureID_p)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    if (PictureID_p != NULL)
    {
        /* Lock Access to some parameters of Ordering queue data */
        semaphore_wait(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

        RemovePicturesFromQueue(OrderingQueueHandle, PictureID_p, VIDQUEUE_REMOVE_UPTO);

        /* Unlock Access to some parameters of Ordering queue data */
        semaphore_signal(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    return (ErrorCode);
} /* end of VIDQUEUE_CancelUpToPicture() */

/*******************************************************************************
Name        : VIDQUEUE_CancelAllPictures
Description : Erase all pictures from the reordering queue
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_CancelAllPictures(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;

    /* Lock Access to some parameters of Ordering queue data */
    semaphore_wait(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

    RemoveAllPicturesFromQueue(OrderingQueueHandle);

    /* Unlock Access to some parameters of Ordering queue data */
    semaphore_signal(VIDQUEUE_Data_p->OrderingQueueParamsSemaphore_p);

    VIDQUEUE_Data_p->IsLastPushedPictureIDValid = FALSE;

    return (ST_NO_ERROR);
} /* end of VIDQUEUE_CancelAllPictures() */

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : VIDQUEUE_GetStatistics
Description : Get the statistics of the ordering queue module.
Parameters  : ordering queue display handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER, if get failed.
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_GetStatistics(const VIDQUEUE_Handle_t OrderingQueueHandle,
                                 STVID_Statistics_t * const Statistics_p)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Statistics_p->QueuePictureInsertedInQueue
                    = VIDQUEUE_Data_p->Statistics.PictureInsertedInQueue;
    Statistics_p->QueuePicturePushedToDisplay
                    = VIDQUEUE_Data_p->Statistics.PicturePushedToDisplay;
    Statistics_p->QueuePbPictureTooLateRejected
                    = VIDQUEUE_Data_p->Statistics.PbPictureTooLateRejected;
    Statistics_p->QueuePictureRemovedFromQueue
                    = VIDQUEUE_Data_p->Statistics.PictureRemovedFromQueue;
    return(ST_NO_ERROR);
} /* End of VIDQUEUE_GetStatistics() function */

/*******************************************************************************
Name        : VIDDISP_ResetStatistics
Description : Reset the statistics of the ordering queue module.
Parameters  : Ordering queue display handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER, if get failed.
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_ResetStatistics(const VIDQUEUE_Handle_t OrderingQueueHandle,
                                const STVID_Statistics_t * const Statistics_p)
{
    VIDQUEUE_Data_t   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset parameters that are said to be reset (giving value != 0) */
    if (Statistics_p->QueuePictureInsertedInQueue != 0)
    {
        VIDQUEUE_Data_p->Statistics.PictureInsertedInQueue      = 0;
    }
    if (Statistics_p->QueuePicturePushedToDisplay != 0)
    {
        VIDQUEUE_Data_p->Statistics.PicturePushedToDisplay      = 0;
    }
    if (Statistics_p->QueuePbPictureTooLateRejected != 0)
    {
        VIDQUEUE_Data_p->Statistics.PbPictureTooLateRejected    = 0;
    }
    if (Statistics_p->QueuePictureRemovedFromQueue != 0)
    {
        VIDQUEUE_Data_p->Statistics.PictureRemovedFromQueue     = 0;
    }

    return(ST_NO_ERROR);
} /* End of VIDQUEUE_ResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

/*******************************************************************************
Name        : vidqueue_ConfigureEvent
Description : Configures notification of video decoder events.
Parameters  : Video producer handle
              Event to allow or forbid.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if done successfully
              ST_ERROR_BAD_PARAMETER if the event is not supported.
*******************************************************************************/
ST_ErrorCode_t VIDQUEUE_ConfigureEvent(const VIDQUEUE_Handle_t OrderingQueueHandle, const STEVT_EventID_t Event,
        const STVID_ConfigureEventParams_t * const Params_p)
{
    VIDQUEUE_Data_t * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    switch (Event)
    {
        case STVID_NEW_PICTURE_ORDERED_EVT :
            VIDQUEUE_Data_p->EventNotificationConfiguration[VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID].NotificationSkipped = 0;
            VIDQUEUE_Data_p->EventNotificationConfiguration[VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID].ConfigureEventParam = (*Params_p);
            break;

        default :
            /* Error case. This event can't be configured. */
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    /* Return the configuration's result. */
    return(ErrCode);

} /* End of vidqueue_ConfigureEvent() function */


/* Private Functions ------------------------------------------------------------ */

/*******************************************************************************
Name        : vidqueue_IsEventToBeNotified
Description : Test if the corresponding event is to be notified (enabled,  ...)
Parameters  : Decoder handle,
              event ID (defined in vid_dec.h).
Assumptions : Event_ID is supposed to be valid.
Limitations :
Returns     : Boolean, indicating if the event can be notified.
*******************************************************************************/
static BOOL vidqueue_IsEventToBeNotified(const VIDQUEUE_Handle_t OrderingQueueHandle, const U32 Event_ID)
{
    BOOL RetValue = FALSE;
    VIDQUEUE_Data_t * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;

    STVID_ConfigureEventParams_t * ConfigureEventParams_p;
    U32                          * NotificationSkipped_p;

    /* Get the event configuration. */
    ConfigureEventParams_p = &VIDQUEUE_Data_p->EventNotificationConfiguration[Event_ID].ConfigureEventParam;
    NotificationSkipped_p  = &VIDQUEUE_Data_p->EventNotificationConfiguration[Event_ID].NotificationSkipped;

    /* test if the event is enabled. */
    if (ConfigureEventParams_p->Enable)
    {
        switch (ConfigureEventParams_p->NotificationsToSkip)
        {
            case 0 :
                RetValue = TRUE;
                break;

            case 0xFFFFFFFF :
                if ( (*NotificationSkipped_p) == 0xFFFFFFFF)
                {
                    RetValue = TRUE;
                    *NotificationSkipped_p = 0;
                }
                else
                {
                    (*NotificationSkipped_p)++;
                }
                break;

            default :
                if ( (*NotificationSkipped_p)++ >= ConfigureEventParams_p->NotificationsToSkip)
                {
                    RetValue = TRUE;
                    (*NotificationSkipped_p) = 0;
                }
                break;
        }
    }

    /* Return the result... */
    return(RetValue);

} /* End of vidqueue_IsEventToBeNotified() function. */

#ifdef TRACE_QUEUE
/*******************************************************************************
Name        : TracePicture
Description : Traces picture content
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TracePicture(VIDQUEUE_Picture_t * pic_p)
{
    if (pic_p != NULL)
    {
        TraceBuffer(("%c %d/%d %s %d",
               MPEGFrame2Char(pic_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
               pic_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
               pic_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
               PictureStruct2String(pic_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure),
               pic_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PresentationStartTime));
    }
    else
    {
        TraceBuffer(("NULL"));
    }
}

/*******************************************************************************
Name        : TraceList
Description : Traces all inserted pictures
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TraceList(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t     * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t  * CurrentPicture_p;

    TraceBuffer(("New List:"));
    CurrentPicture_p = VIDQUEUE_Data_p->OrderingQueue_p;
    while (CurrentPicture_p != NULL)
    {
       TraceBuffer((" -> "));TracePicture(CurrentPicture_p);
       CurrentPicture_p = CurrentPicture_p->Next_p;
    }
    TraceBuffer((" -> "));TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));
}
#endif

/*******************************************************************************
Name        : InitQueueElementsPool
Description : Reset all used elements in ordering queue
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitQueueElementsPool(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t     * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t  * QueueElements_p;
    U32 Index;

    QueueElements_p = VIDQUEUE_Data_p->OrderingQueueElements;
    for (Index = 0; Index < ORDQUEUE_NB_ELEM; Index++ , QueueElements_p++)
    {
        QueueElements_p->PictureBuffer_p = NULL;
        QueueElements_p->Next_p          = NULL;
    }
} /* end InitQueueElementsPool() */

/*******************************************************************************
Name        : GetFreePictureElement
Description : return the index in ordering element array of an unused one
Parameters  :
Assumptions :
Limitations :
Returns     : U32 Index of an unused element
*******************************************************************************/
static VIDQUEUE_Picture_t * GetFreePictureElement(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t     * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t  * QueueElements_p;
    U32   Index;

    QueueElements_p = VIDQUEUE_Data_p->OrderingQueueElements;
    for (Index = 0; Index < ORDQUEUE_NB_ELEM; Index++ , QueueElements_p++)
    {
        if (QueueElements_p->PictureBuffer_p == NULL)
        {
            return(QueueElements_p);
        }
    }

    /* loop ended not found */
    TraceBuffer(("Get queue element KO !! \r\n"));
    return(NULL);
} /* end GetFreePictureElement() */

/*******************************************************************************
Name        : FreePictureElement
Description : Frees a picture element
Parameters  :
Assumptions :
Limitations : CAUTION: Element must have been moved out from the list before removing it
Returns     :
*******************************************************************************/
static void FreesPictureElement(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDQUEUE_Picture_t * const CurrentPicture_p)
{
    VIDQUEUE_Data_t     * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;

    /* If this Picture has an associated Picture Buffer, release it also */
    if (CurrentPicture_p->PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        RELEASE_PICTURE_BUFFER(VIDQUEUE_Data_p->BufferManagerHandle, CurrentPicture_p->PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDORDQUEUE_MODULE_BASE);
        /* It is not allowed to set AssociatedDecimatedPicture_p to NULL because the link between Main and Decimated Picture Buffers is managed by VID_BUFF */
    }

    RELEASE_PICTURE_BUFFER(VIDQUEUE_Data_p->BufferManagerHandle, CurrentPicture_p->PictureBuffer_p, VIDCOM_VIDORDQUEUE_MODULE_BASE);
    /* Reset PictureBuffer_p to ensure that vid_ordq doesn't use this pointer now that we have released this picture buffer */
    CurrentPicture_p->Next_p = NULL;
    CurrentPicture_p->PictureBuffer_p   = NULL;
} /* end FreesPictureElement() */

/*******************************************************************************
Name        : PictureIsBeyondPictureID
Description : Check if a picture has gone beyond the given Picture ID
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL PictureIsBeyondPictureID(VIDQUEUE_InsertionOrder_t  InsertionOrder,
                                     VIDQUEUE_Picture_t       * CurrentPicture_p,
                                     VIDCOM_PictureID_t       * PictureID_p,
                                     BOOL                     * PictureFound_p)
{
    BOOL  GoneBeyond = FALSE;
    int   CompareResult;

    CompareResult = vidcom_ComparePictureID(&CurrentPicture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID,
                                            PictureID_p);
    if (CompareResult == 0)
    {
        TraceBuffer(("Found: "));TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));
        *PictureFound_p = TRUE;
    }

    switch (InsertionOrder)
    {
        case VIDQUEUE_INSERTION_ORDER_DECREASING:
            /* Picture into queue is strictly lower than the seek Picture */
            if (CompareResult < 0)
            {
                GoneBeyond = TRUE;                  /* Gone beyond if picture is strictly lower */
            }
            break;

        case VIDQUEUE_INSERTION_ORDER_LAST_PLACE:
            if (CompareResult != 0)
            {
                GoneBeyond = *PictureFound_p;       /* Gone beyond if picture has been previously found */
            }
            break;

        case VIDQUEUE_INSERTION_ORDER_INCREASING:
        default:
            /* Picture into queue is strictly greater than the seek Picture */
            if (CompareResult > 0)
            {
                GoneBeyond = TRUE;          /* Gone beyond if picture is strictly greater */
            }
            break;
    }

  return (GoneBeyond);
}

/*******************************************************************************
Name        : RemovePicturesFromQueue
Description : Clean ordering queue even if pictures should be displayed
Parameters  : Display handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL RemovePicturesFromQueue(const VIDQUEUE_Handle_t OrderingQueueHandle,
                                    VIDCOM_PictureID_t * const PictureID_p,
                                    VIDQUEUE_RemoveMode_t RemoveMode)
{
    VIDQUEUE_Data_t      * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t   * CurrentPicture_p;
    VIDQUEUE_Picture_t   * NextPicture_p;
    VIDQUEUE_Picture_t   * PreviousPicture_p;
    BOOL     PictureFound = FALSE;
    BOOL     RemovePicture;

    if (PictureID_p == NULL)
    {
        TraceBuffer(("un-queue wrong picture \r\n"));
        return (TRUE);
    }

    TraceBuffer(("\n\rREMOVING: %d/%d\r\n", PictureID_p->ID, PictureID_p->IDExtension));

    /* Display Queue is not empty */
    CurrentPicture_p = VIDQUEUE_Data_p->OrderingQueue_p;
    PreviousPicture_p = NULL;

    /* Test picture in display queue */
    while (   (CurrentPicture_p != NULL)
           && (! PictureIsBeyondPictureID(VIDQUEUE_Data_p->InsertionOrder,    /* While picture is not beyond picture ID */
                                          CurrentPicture_p,                   /* CAUTION: PictureFound is used as input */
                                          PictureID_p,                        /*          as well as output parameter   */
                                          &PictureFound)))                    /*          in PictureIsBeyondPictureID() */
    {
        RemovePicture = FALSE;
        NextPicture_p = CurrentPicture_p->Next_p;

        switch (RemoveMode)
        {
            case VIDQUEUE_REMOVE_UPTO:
                RemovePicture = TRUE;
                break;

            case VIDQUEUE_REMOVE_LAST:
            default:
                if (PictureFound)
                {
                    RemovePicture = TRUE;
                }
                else
                {
                    PreviousPicture_p = CurrentPicture_p;
                }
                break;
        }

        if (RemovePicture)
        {
            TraceBuffer(("\r\nRemoving: ")); TracePicture(CurrentPicture_p); TraceBuffer(("\n\r"));

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDQUEUE_Data_p->Statistics.PictureRemovedFromQueue ++;
#endif
            /* Found picture to remove */
            if (PreviousPicture_p != NULL)
            {
                /* Picture was not the first in ordering queue */
                PreviousPicture_p->Next_p = NextPicture_p;
            }
            else
            {
                /* Picture was the first in ordering queue */
                VIDQUEUE_Data_p->OrderingQueue_p = NextPicture_p;
            }

            FreesPictureElement(OrderingQueueHandle, CurrentPicture_p);
        }

        /* Continue search with next in ordering queue */
        CurrentPicture_p = NextPicture_p;
    }

    return (PictureFound);
} /* end RemovePicturesFromQueue() */

/*******************************************************************************
Name        : RemoveAllPicturesFromQueue
Description : Clean ordering queue even if pictures should be displayed
Parameters  : Display handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void RemoveAllPicturesFromQueue(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t      * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t   * CurrentPicture_p;

    TraceBuffer(("\n\rREMOVING ALL\r\n"));

    /* Get first element */
    CurrentPicture_p = VIDQUEUE_Data_p->OrderingQueue_p;

    /* While not last picture */
    while (CurrentPicture_p != NULL)
    {
        VIDQUEUE_Data_p->OrderingQueue_p = CurrentPicture_p->Next_p;

#ifdef STVID_DEBUG_GET_STATISTICS
        VIDQUEUE_Data_p->Statistics.PictureRemovedFromQueue ++;
#endif

        TraceBuffer(("Removing: ")); TracePicture(CurrentPicture_p); TraceBuffer(("\n\r"));

        FreesPictureElement(OrderingQueueHandle, CurrentPicture_p);

        /* Continue search with next in ordering queue */
        CurrentPicture_p = VIDQUEUE_Data_p->OrderingQueue_p;
    }
} /* end RemoveAllPicturesFromQueue */

/*******************************************************************************
Name        : InsertPictureInReorderingQueue
Description : Reorders picture according to ExtendedPresentationOrderPictureID
Parameters  : Queue handle, pointer to picture
Assumptions : By default increasing insertion order is choosen.
Limitations :
Returns     : TRUE if picture inserted, FALSE otherwise
*******************************************************************************/
static BOOL InsertPictureInReorderingQueue(const VIDQUEUE_Handle_t OrderingQueueHandle,
                                           const VIDQUEUE_InsertionOrder_t InsertionOrder,
                                           VIDQUEUE_Picture_t * const Picture_p)
{
    VIDQUEUE_Data_t      * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t   * CurrentPicture_p = NULL;
    VIDQUEUE_Picture_t   * PreviousPicture_p= NULL;
    BOOL                   InsertionPlaceFound = FALSE;
    BOOL                   PicAlreadyFound;   /* Used to test if 2 pictures have the same PictureID */
    int                    LastPushedComparison;

    if (Picture_p == NULL)
    {
        TraceBuffer(("in-queue wrong picture \r\n"));
        /* No picture: nothing to do */
        return (FALSE);
    }

#ifdef TRACE_QUEUE
    TraceBuffer(("\r\nINSERTING: ")); TracePicture(Picture_p);
    switch(InsertionOrder)
    {
        case VIDQUEUE_INSERTION_ORDER_DECREASING:
            TraceBuffer((" , Decreasing order"));
            break;

        case VIDQUEUE_INSERTION_ORDER_LAST_PLACE:
            TraceBuffer((" , Last place"));
            break;

        case VIDQUEUE_INSERTION_ORDER_INCREASING :
            TraceBuffer((" , Increasing order"));
            break;

        default :
            TraceBuffer((" , Unknown order ERROR !!!"));
            break;
    }
    TraceBuffer(("\n\r"));
#endif /* TRACE_QUEUE */


    /* Manages the insertion order */
    if (InsertionOrder != VIDQUEUE_Data_p->InsertionOrder)
    {
        if (VIDQUEUE_Data_p->OrderingQueue_p != NULL)
        {
            /* Trying to invert insertion order whereas queue is not empty */
            /* This is an error case */
            TraceBuffer(("ERROR: Order insertion mismatch\r\n"));
        }

        /* Stores the new insertion order */
        VIDQUEUE_Data_p->InsertionOrder = InsertionOrder;

        /* Insertion order has changed, last pushed picture information is no more relevant */
        VIDQUEUE_Data_p->IsLastPushedPictureIDValid = FALSE;
    }


    /* Manages too late rejected pictures */
    if (VIDQUEUE_Data_p->IsLastPushedPictureIDValid)
    {
        LastPushedComparison = vidcom_ComparePictureID(&VIDQUEUE_Data_p->LastPushedPictureID,
                                                       &Picture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID);

        if (   ((InsertionOrder == VIDQUEUE_INSERTION_ORDER_DECREASING) && (LastPushedComparison < 0))
            || ((InsertionOrder == VIDQUEUE_INSERTION_ORDER_INCREASING) && (LastPushedComparison > 0)))
        {
            /* Picture is lower (INCREASING) or greater (DECREASING) than the last pushed picture */
            /* Reject it ... */
            TraceBuffer(("ERROR: Rejected picture !!!\r\n"));

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDQUEUE_Data_p->Statistics.PbPictureTooLateRejected ++;
#endif
            /* Picture has not been inserted */
            return (FALSE);
        }
    }

    DEBUG_CHECK_PICTURE_BUFFER_VALIDITY(VIDQUEUE_Data_p->BufferManagerHandle, Picture_p->PictureBuffer_p);

    /* Take the Picture Buffer because it is now under queue control */
    /* ErrorCode = */ TAKE_PICTURE_BUFFER(VIDQUEUE_Data_p->BufferManagerHandle, Picture_p->PictureBuffer_p, VIDCOM_VIDORDQUEUE_MODULE_BASE);
    if (Picture_p->PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        /* Take the decimated picture buffer because it is now under queue control */
        /* ErrorCode = */ TAKE_PICTURE_BUFFER(VIDQUEUE_Data_p->BufferManagerHandle, Picture_p->PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDORDQUEUE_MODULE_BASE);
    }

    /* Manages the picture insertion */
    CurrentPicture_p = VIDQUEUE_Data_p->OrderingQueue_p;
    PreviousPicture_p = NULL;

    TraceBuffer(("First= "));TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));
    while (   (CurrentPicture_p != NULL)
           && (! InsertionPlaceFound))
    {
        PicAlreadyFound = FALSE;
        if (InsertionOrder != VIDQUEUE_INSERTION_ORDER_LAST_PLACE)
        {
            /* For particular case of LAST_PLACE, we only seek the last picture in queue, */
            /* so do not bother with beyond concept */

            /* For other cases, check if current picture in ordering queue */
            /* is not beyond the given picture */
            InsertionPlaceFound = PictureIsBeyondPictureID(InsertionOrder,
                                          CurrentPicture_p,
                                          &Picture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID,
                                          &PicAlreadyFound);
        }
        if (PicAlreadyFound==TRUE)
        {
            /* The inserted PictureID is the same as CurrentPicture_p's one*/
            /* Let's compare their presentationStartTime, and decide to insert it before or after CurrentPicture_p */
            /* Check the IsValidPresentationStartTime are OK, and that CurrentPicture_p is after Picture_p */
            if ((Picture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsPresentationStartTimeValid == TRUE) &&
                (CurrentPicture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsPresentationStartTimeValid == TRUE) &&
                (STOS_time_after(CurrentPicture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PresentationStartTime,
                                 Picture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PresentationStartTime) == TRUE))
            {
                InsertionPlaceFound=TRUE;
            }
        }

        if (! InsertionPlaceFound)
        {
            /* Insertion place has not been found yet */
            /* Get next picture in ordering queue     */
            PreviousPicture_p = CurrentPicture_p;
            CurrentPicture_p = PreviousPicture_p->Next_p;

            TraceBuffer(("Next= "));TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));
        }
    }

    if (PreviousPicture_p == NULL)
    {
        /* Place seeking stopped at first occurence */
        if (CurrentPicture_p == NULL)
        {
            TraceBuffer(("Queue Empty: First element\r\n"));
        }
        else
        {
            TraceBuffer(("Before: First element\r\n"));
        }
        /* Insert at first place in ordering queue ! */
        VIDQUEUE_Data_p->OrderingQueue_p = Picture_p;
    }
    else
    {
        /* Found the place where to insert the frame: just after */
        /* previous frame */
        CurrentPicture_p = PreviousPicture_p->Next_p;
        PreviousPicture_p->Next_p = Picture_p;
    }

    /* link inserted picture with its following in ordering queue */
    Picture_p->Next_p = CurrentPicture_p;

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDQUEUE_Data_p->Statistics.PictureInsertedInQueue ++;
#endif

#ifdef TRACE_QUEUE
        TraceList(OrderingQueueHandle);
#endif

    return (TRUE);
} /* End of InsertPictureInReorderingQueue() function */

/*******************************************************************************
Name        : PushPicturesToDisplay
Description : Flush ordering queue pictures to display task
Parameters  : Display handle
Assumptions : PictureID_p not NULL, checked above
Limitations :
Returns     :
*******************************************************************************/
static void PushPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle,
                                  VIDCOM_PictureID_t * const PictureID_p)
{
    VIDQUEUE_Data_t                   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t                * CurrentPicture_p;
    VIDQUEUE_Picture_t                * NextPicture_p;
    VIDQUEUE_ReadyForDisplayParams_t    NewParams;
    BOOL     PictureFound = FALSE;
    STVID_PictureInfos_t                ExportedPictureInfos;

#ifdef TRACE_QUEUE
    TraceBuffer(("\n\rPUSHING: %d/%d", PictureID_p->ID, PictureID_p->IDExtension));
    switch(VIDQUEUE_Data_p->InsertionOrder)
    {
        case VIDQUEUE_INSERTION_ORDER_DECREASING:
            TraceBuffer((", Decreasing order"));
            break;

        case VIDQUEUE_INSERTION_ORDER_LAST_PLACE:
            TraceBuffer((", Last place"));
            break;

        case VIDQUEUE_INSERTION_ORDER_INCREASING :
            TraceBuffer((", Increasing order"));
            break;

        default :
            TraceBuffer((", Unknown order ERROR !!!"));
            break;
    }
    TraceBuffer(("\n\r"));
#endif /* TRACE_QUEUE */

    /* Get first element */
    CurrentPicture_p = VIDQUEUE_Data_p->OrderingQueue_p;

    TraceBuffer(("First: ")); TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));

    /* Test picture in ordering queue */
    while (   (CurrentPicture_p != NULL)                                      /*     Not last element */
           && (! PictureIsBeyondPictureID(VIDQUEUE_Data_p->InsertionOrder,    /* and picture is not beyond picture ID */
                                          CurrentPicture_p,                   /* CAUTION: PictureFound is used as input */
                                          PictureID_p,                        /*          as well as output parameter   */
                                          &PictureFound)))                    /*          in PictureIsBeyondPictureID() */
    {
        TraceBuffer(("Send to display: "));TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));

        ComputePTS( OrderingQueueHandle, CurrentPicture_p);
        NextPicture_p = CurrentPicture_p->Next_p;

        /* Stores last pushed picture ID */
        VIDQUEUE_Data_p->IsLastPushedPictureIDValid = TRUE;
        VIDQUEUE_Data_p->LastPushedPictureID = CurrentPicture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;

        /* Set parameters for event */
        NewParams.Picture_p = CurrentPicture_p->PictureBuffer_p;            /* Picture buffer */

#if defined(ST_multicodec)
		/* Mark the picture as pushed */
        CurrentPicture_p->PictureBuffer_p->IsPushed = TRUE;
#endif

        /* Signal new picture to outside (display should have subscribed !) */
        STEVT_Notify(VIDQUEUE_Data_p->EventsHandle, VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_READY_FOR_DISPLAY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &NewParams);
        if ( vidqueue_IsEventToBeNotified(OrderingQueueHandle, VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID) )
        {
            ExportedPictureInfos = NewParams.Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos;
            /* For the Regular Frame Buffer Pointers*/
            ExportedPictureInfos.BitmapParams.Data1_p = Virtual(ExportedPictureInfos.BitmapParams.Data1_p);
            ExportedPictureInfos.BitmapParams.Data2_p = Virtual(ExportedPictureInfos.BitmapParams.Data2_p);
            /* For the Decimated Buffer Pointers*/
            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p);
            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p);
            /* Notify STVID_NEW_PICTURE_ORDERED_EVT event */
            STEVT_Notify(VIDQUEUE_Data_p->EventsHandle, VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureInfos));
        }

#ifdef STVID_DEBUG_GET_STATISTICS
        VIDQUEUE_Data_p->Statistics.PicturePushedToDisplay ++;
#endif

        /* The Picture is now under the responsability of Display so it can be removed from the element pool */
        FreesPictureElement(OrderingQueueHandle, CurrentPicture_p);

        /* Continue search with next element in ordering queue */
        CurrentPicture_p = NextPicture_p;

        TraceBuffer(("Next: ")); TracePicture(CurrentPicture_p); TraceBuffer(("\n\r"));
    }

    /* Finally set first element with the current picture */
    VIDQUEUE_Data_p->OrderingQueue_p = CurrentPicture_p;

#ifdef TRACE_QUEUE
    TraceList(OrderingQueueHandle);
#endif
} /* end PushPicturesToDisplay() */

/*******************************************************************************
Name        : PushAllPicturesToDisplay
Description : Flush ordering queue pictures to display task
Parameters  : Display handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PushAllPicturesToDisplay(const VIDQUEUE_Handle_t OrderingQueueHandle)
{
    VIDQUEUE_Data_t                   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDQUEUE_Picture_t                * CurrentPicture_p;
    VIDQUEUE_Picture_t                * NextPicture_p;
    VIDQUEUE_ReadyForDisplayParams_t    NewParams;
    STVID_PictureInfos_t                ExportedPictureInfos;

    TraceBuffer(("\n\rPUSHING ALL\r\n"));

    /* Get first element */
    CurrentPicture_p = VIDQUEUE_Data_p->OrderingQueue_p;

    TraceBuffer(("First: ")); TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));

    /* Test picture in ordering queue */
    while (CurrentPicture_p != NULL)                                        /*     Not last element */
    {
        TraceBuffer(("Send to display: "));TracePicture(CurrentPicture_p);TraceBuffer(("\n\r"));

        ComputePTS( OrderingQueueHandle, CurrentPicture_p);
        NextPicture_p = CurrentPicture_p->Next_p;

        /* Set parameters for event */
        NewParams.Picture_p = CurrentPicture_p->PictureBuffer_p;            /* Picture buffer */

#if defined(ST_multicodec)
		CurrentPicture_p->PictureBuffer_p->IsPushed = TRUE;
#endif

        /* Signal new picture to outside (display should have subscribed !) */
        STEVT_Notify(VIDQUEUE_Data_p->EventsHandle, VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_READY_FOR_DISPLAY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &NewParams);
        if ( vidqueue_IsEventToBeNotified(OrderingQueueHandle, VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID) )
        {
            ExportedPictureInfos = NewParams.Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos;
            /* For the Regular Frame Buffer Pointers*/
            ExportedPictureInfos.BitmapParams.Data1_p = Virtual(ExportedPictureInfos.BitmapParams.Data1_p);
            ExportedPictureInfos.BitmapParams.Data2_p = Virtual(ExportedPictureInfos.BitmapParams.Data2_p);
            /* For the Decimated Buffer Pointers*/
            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p);
            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p);
            /* Notify STVID_NEW_PICTURE_ORDERED_EVT event */
            STEVT_Notify(VIDQUEUE_Data_p->EventsHandle, VIDQUEUE_Data_p->RegisteredEventsID[VIDQUEUE_NEW_PICTURE_ORDERED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureInfos));
        }

#ifdef STVID_DEBUG_GET_STATISTICS
        VIDQUEUE_Data_p->Statistics.PicturePushedToDisplay ++;
#endif
        /* Stores last pushed picture ID */
        VIDQUEUE_Data_p->IsLastPushedPictureIDValid = TRUE;
        VIDQUEUE_Data_p->LastPushedPictureID = CurrentPicture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;

        /* The Picture is now under the responsability of Display so it can be removed from the element pool */
        FreesPictureElement(OrderingQueueHandle, CurrentPicture_p);

        /* Continue search with next element in ordering queue */
        CurrentPicture_p = NextPicture_p;

        TraceBuffer(("Next: ")); TracePicture(CurrentPicture_p); TraceBuffer(("\n\r"));
    }

    /* Finally set first element with the current picture */
    VIDQUEUE_Data_p->OrderingQueue_p = CurrentPicture_p;

#ifdef TRACE_QUEUE
    TraceList(OrderingQueueHandle);
#endif
} /* end PushAllPicturesToDisplay() */

/*******************************************************************************
Name        : ComputePTS
Description : Compute the correct PTS, now the pictures have been re-ordered
Parameters  : Display handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ComputePTS(const VIDQUEUE_Handle_t OrderingQueueHandle, VIDQUEUE_Picture_t * Picture_p)
{
    VIDQUEUE_Data_t                   * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) OrderingQueueHandle;
    VIDCOM_PictureGenericData_t * PictGenData_p;
    U32 FrameRateForCalculation;
    U32 TimeDisplayPreviousPicture;
    U32 OldPTS;

    PictGenData_p = &(Picture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData);

    if (PictGenData_p->PictureInfos.VideoParams.PTS.IsValid)
    {
        VIDQUEUE_Data_p->PreviousPictPTS= PictGenData_p->PictureInfos.VideoParams.PTS.PTS;
        VIDQUEUE_Data_p->PreviousPictNbFieldsDisplay = PictGenData_p->NbFieldsDisplay;
        VIDQUEUE_Data_p->FirstPictureEver = FALSE;
        return;
    }


    if (VIDQUEUE_Data_p->FirstPictureEver)
    {
        PictGenData_p->PictureInfos.VideoParams.PTS.PTS = 0;
        PictGenData_p->PictureInfos.VideoParams.PTS.PTS33 = FALSE;
        VIDQUEUE_Data_p->FirstPictureEver = FALSE;
    }
    else
    {
        OldPTS = VIDQUEUE_Data_p->PreviousPictPTS;
        if (PictGenData_p->PictureInfos.VideoParams.FrameRate != 0)
        {
            FrameRateForCalculation = PictGenData_p->PictureInfos.VideoParams.FrameRate;
        }
        else
        {
            /* To avoid division by 0 */
            FrameRateForCalculation = 50000;
        }


        TimeDisplayPreviousPicture = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) *
                                                          VIDQUEUE_Data_p->PreviousPictNbFieldsDisplay /
                                                          FrameRateForCalculation / 2;

        PictGenData_p->PictureInfos.VideoParams.PTS.PTS = OldPTS + TimeDisplayPreviousPicture;
        if (PictGenData_p->PictureInfos.VideoParams.PTS.PTS < OldPTS)
        {
                /* 33th bit to toggle */
                PictGenData_p->PictureInfos.VideoParams.PTS.PTS33 = (!PictGenData_p->PictureInfos.VideoParams.PTS.PTS33);
        }
    }

    PictGenData_p->PictureInfos.VideoParams.PTS.Interpolated = TRUE;
    PictGenData_p->PictureInfos.VideoParams.PTS.IsValid = TRUE;

    VIDQUEUE_Data_p->PreviousPictPTS = PictGenData_p->PictureInfos.VideoParams.PTS.PTS;
    VIDQUEUE_Data_p->PreviousPictNbFieldsDisplay = PictGenData_p->NbFieldsDisplay;
} /* End of compute PTS interpolated */

/* end of vid_ordq.c */
