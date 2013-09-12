/*******************************************************************************

File name   : vid_trqueue.c

Description : Video Trancoder queue functions

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
15 June 2006        Created                                         LM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Define to add debug info and traces */
/* See vid_trqueue.h for defines */

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <limits.h>
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"

#include "stvid.h"

#include "vid_com.h"
#include "vid_ctcm.h"

#include "buffers.h"

#include "transcoder.h"
/*#include "vid_tran.h"*/
#include "vid_trqueue.h"
#include "transcodec.h"

#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "sttbx.h"
#endif /* ST_OSLINUX */

#include "stevt.h"

#include "stdevice.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
#ifdef TRACE_TRQUEUE /* define or uncomment in ./makefile to set traces */
  #ifdef TRACE_TRQUEUE_VERBOSE
    #define TraceVerbose(x) TraceBuffer(x)
  #else
    #define TraceVerbose(x)
  #endif
  #include "vid_trc.h"
#else
  #define TraceInfo(x)
  #define TraceBuffer(x)
  #define TraceVerbose(x)
#endif

/* Maximum of elements traced (10s) */
#define MAX_NUMBER_OF_ELEMENTS_TRACED   (30*10)

/* Private Variables (static)------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Global Variables --------------------------------------------------------- */

#ifdef STVID_DEBUG_TRQUEUE
U32 HistoryOfElementsInTranscoderQueueTable[MAX_NUMBER_OF_ELEMENTS_TRACED];
U32 NextElementInHistory;
U32 NbOfElementInQueue;
#endif /* STVID_DEBUG_TRQUEUE */

/* Public Functions --------------------------------------------------------- */

/* Functions Exported in module --------------------------------------------- */


/* Private Functions -------------------------------------------------------- */
/*******************************************************************************
Name        : vidtrqueue_InitTranscoderQueue
Description : Reset all ellements used in transcoder queue
Parameters  :
Assumptions : Ensure no other function can be called at the same time !
Limitations :
Returns     : Nothing
*******************************************************************************/
void vidtrqueue_InitTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p)
{
    U32 Index;

    /* Resetting queue table */
    for (Index = 0; Index < NB_TRQUEUE_ELEM; Index++)
    {
        VIDTRAN_Data_p->QueueElements[Index].Previous_p                 = NULL;
        VIDTRAN_Data_p->QueueElements[Index].Next_p                     = NULL;
        VIDTRAN_Data_p->QueueElements[Index].MainPictureBuffer_p        = NULL;
        VIDTRAN_Data_p->QueueElements[Index].DecimatedPictureBuffer_p   = NULL;
        VIDTRAN_Data_p->QueueElements[Index].State                      = VIDTRQUEUE_STATE_FREE;
    }
    VIDTRAN_Data_p->Queue_p = NULL;

#ifdef STVID_DEBUG_TRQUEUE
    /* Resetting queue history */
    memset(HistoryOfElementsInTranscoderQueueTable, 0x00, MAX_NUMBER_OF_ELEMENTS_TRACED);
    /* First entry is marked 0xFFFFFFFF */
    HistoryOfElementsInTranscoderQueueTable[0] = 0xFFFFFFFF;
    NextElementInHistory = 1;
    NbOfElementInQueue = 1;
#endif /* STVID_DEBUG_TRQUEUE */

    return;
}

#if 0
/*******************************************************************************
Name        : vidtrqueue_FindLastTrPictureElement
Description : return a pointer to the last transcode queue element.
              This function returns NULL if not found.
Parameters  :
Assumptions :
Limitations :
Returns     : A TrPicture unused element pointer
*******************************************************************************/
VIDTRAN_Picture_t * vidtrqueue_FindLastTrPictureElement(const VIDTRAN_Data_t * VIDTRAN_Data_p)
{
    VIDTRAN_Picture_t * CurrentPicture_p = NULL;

    CurrentPicture_p = VIDTRAN_Data_p->Queue_p;

    if (CurrentPicture_p == NULL)
    {
        TraceVerbose(("Queue id empty !\r\n"));
        return (NULL);
    }

    while (CurrentPicture_p->Next_p != NULL)
    {
        /* Continue search with next in display queue */
        CurrentPicture_p = CurrentPicture_p->Next_p;
    }

    return (CurrentPicture_p);
}
#endif

/*******************************************************************************
Name        : vidtrqueue_GetTrPictureElement
Description : return a pointer to a free transcode queue element.
              This function returns NULL if no transcode queue element is available.
Parameters  :
Assumptions :
Limitations :
Returns     : A TrPicture unused element pointer
*******************************************************************************/
VIDTRAN_Picture_t * vidtrqueue_GetTrPictureElement(VIDTRAN_Data_t * VIDTRAN_Data_p)
{
    VIDTRAN_Picture_t * TrPicture_p = NULL;
    U32                 Index = 0;

    /* Do next section without being disturbed */
    STOS_TaskLock();

    while ((VIDTRAN_Data_p->QueueElements[Index].State != VIDTRQUEUE_STATE_FREE) && (Index < NB_TRQUEUE_ELEM))
    {
        Index++;
    }

    /* If one was found */
    if (VIDTRAN_Data_p->QueueElements[Index].State == VIDTRQUEUE_STATE_FREE)
    {
        VIDTRAN_Data_p->QueueElements[Index].State = VIDTRQUEUE_STATE_RESERVED;
        TrPicture_p = &(VIDTRAN_Data_p->QueueElements[Index]);
    }

    /* End of critical section */
    STOS_TaskUnlock();

    return(TrPicture_p);
}

/*******************************************************************************
Name        : vidtrqueue_InsertPictureInTranscoderQueue
Description : Insert picture in transcoder queue
Parameters  : Transcoder handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void vidtrqueue_InsertPictureInTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p, VIDTRAN_Picture_t  * const TrPictures_p)
{
    VIDTRAN_Picture_t * CurrentPicture_p = NULL;
    VIDTRAN_Picture_t * PreviousPicture_p= NULL;

    if ((TrPictures_p == NULL) || (TrPictures_p->MainPictureBuffer_p == NULL))
    {
        TraceInfo(("in-queue wrong picture \r\n"));
        /* No picture: nothing to do */
        return;
    }

    TraceVerbose(("in-queue %i in %i  \r\n", TrPictures_p, Queue_p_p));

    /* Do next section without being disturbed */
    STOS_TaskLock();

    if (VIDTRAN_Data_p->Queue_p == NULL)
    {
        /* Display Queue is empty */
        VIDTRAN_Data_p->Queue_p = TrPictures_p;
    }
    else
    {
        /* Display Queue is not empty */
        CurrentPicture_p  = VIDTRAN_Data_p->Queue_p;
        PreviousPicture_p = CurrentPicture_p;

        while (CurrentPicture_p != NULL)
        {
            PreviousPicture_p = CurrentPicture_p;
            CurrentPicture_p = PreviousPicture_p->Next_p;
        }
        /* Found the place where to insert the frame: at last place */
        /* just after previous frame */
        PreviousPicture_p->Next_p = TrPictures_p;/* Insert frame */
    }

    /* link inserted picture with its following in transcoder queue */
    TrPictures_p->Next_p = CurrentPicture_p;
    TrPictures_p->Previous_p = PreviousPicture_p;

    /* Confirm entry is valid */
    TrPictures_p->State = VIDTRQUEUE_STATE_VALID;

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDTRAN_Data_p->Statistics.EncodePicturesInsertedInQueue++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_TRQUEUE
    /* Updating history */
    HistoryOfElementsInTranscoderQueueTable[NextElementInHistory] = ++NbOfElementInQueue;
    if (NextElementInHistory < MAX_NUMBER_OF_ELEMENTS_TRACED)
    {
        NextElementInHistory++;
    }
    else
    {
        NextElementInHistory = 0;
    }
#endif /* STVID_DEBUG_TRQUEUE */

    /* End of critical section */
    STOS_TaskUnlock();

    return;
} /* End of vidtrqueue_InsertPictureInTranscoderQueue() function */

/*******************************************************************************
Name        : vidtrqueue_TakePicturesBeforeInsertInTranscoderQueue
Description : Take the picture.
Parameters  : Transcoder handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void vidtrqueue_TakePicturesBeforeInsertInTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p, VIDTRAN_Picture_t  * const TrPictures_p)
{
    DEBUG_CHECK_PICTURE_BUFFER_VALIDITY(VIDTRAN_Data_p->BufferManagerHandle, TrPictures_p->MainPictureBuffer_p);

    /* Lock the Picture Buffer because it is now under Transcoder control */
    TAKE_PICTURE_BUFFER(VIDTRAN_Data_p->BufferManagerHandle, TrPictures_p->MainPictureBuffer_p, VIDCOM_XCODE_MODULE_BASE);

    /* When no decimation is requested on the source decoder, the pointer to the decimated picture is NULL */
    if (TrPictures_p->DecimatedPictureBuffer_p != NULL)
    {
        /* Taking the decimated picture */
        TAKE_PICTURE_BUFFER(VIDTRAN_Data_p->BufferManagerHandle, TrPictures_p->DecimatedPictureBuffer_p, VIDCOM_XCODE_MODULE_BASE);
        TraceVerbose(("\r\nINS Decim 0x%x", TrPictures_p->DecimatedPictureBuffer_p));
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDTRAN_Data_p->Statistics.EncodeSourcePictureTaken++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Verbose after Take (just in case) */
    TraceVerbose(("\r\nINS Main  0x%x", TrPictures_p->MainPictureBuffer_p));

    return;
} /* End of vidtrqueue_TakePicturesBeforeInsertInTranscoderQueue */

/*******************************************************************************
Name        : vidtrqueue_RemovePicturesFromTranscoderQueue
Description : Remove picture in transcoder queue
Parameters  : Transcoder handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void vidtrqueue_RemovePicturesFromTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p, VIDTRAN_Picture_t  * const TrPictures_p)
{
    VIDTRAN_Picture_t * CurrentPicture_p = NULL;
    VIDTRAN_Picture_t * PreviousPicture_p= NULL;

    if (TrPictures_p == NULL)
    {
        TraceInfo(("un-queue wrong picture \r\n"));
        /* No picture: nothing to do */
        return;
    }

    /* Do next section without being disturbed */
    STOS_TaskLock();

    /* Transcode Queue is not empty */
    CurrentPicture_p  = VIDTRAN_Data_p->Queue_p;
    PreviousPicture_p = NULL;

    if (CurrentPicture_p != NULL)
    {
        /* Test picture in transcode queue */
        while ((CurrentPicture_p->Next_p != NULL)
        &&  (CurrentPicture_p != TrPictures_p)) /* Picture_p not found */
        {
            /* Continue search with next in display queue */
            PreviousPicture_p = CurrentPicture_p;
            CurrentPicture_p = CurrentPicture_p->Next_p;
        }
        if (CurrentPicture_p == TrPictures_p)
        {
            TraceVerbose(("un-queue %i from primary \r\n",TrPictures_p));
            /* Found picture to remove */
            if (PreviousPicture_p != NULL)
            {
                /* Picture was not the first in display queue */
                PreviousPicture_p->Next_p = CurrentPicture_p->Next_p;
                if (CurrentPicture_p->Next_p != NULL)
                {
                    CurrentPicture_p->Next_p->Previous_p = PreviousPicture_p;
                }
            }
            else
            {
                /* Picture was the first in transcode queue */
                VIDTRAN_Data_p->Queue_p = CurrentPicture_p->Next_p;
                if (CurrentPicture_p->Next_p != NULL)
                {
                    CurrentPicture_p->Next_p->Previous_p = NULL;
                }
            }

            CurrentPicture_p->Next_p            = NULL;
            CurrentPicture_p->Previous_p        = NULL;
            CurrentPicture_p->State             = VIDTRQUEUE_STATE_RELEASE_REQUESTED;

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDTRAN_Data_p->Statistics.EncodePicturesRemovedFromQueue++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_TRQUEUE
            /* Updating history */
            HistoryOfElementsInTranscoderQueueTable[NextElementInHistory] = --NbOfElementInQueue;
            if (NextElementInHistory < MAX_NUMBER_OF_ELEMENTS_TRACED)
            {
                NextElementInHistory++;
            }
            else
            {
                NextElementInHistory = 0;
            }
#endif /* STVID_DEBUG_TRQUEUE */
        }
        else
        {
            /* picture has not been found !! */
            TraceInfo(("un-queue wrong picture \r\n"));
        }
    }
    else
    {
        /* Queue is empty !! */
        TraceInfo(("Queue empty \r\n"));
    }

    /* End of critical section */
    STOS_TaskUnlock();

    return;
} /* End of vidtrqueue_RemovePicturesFromTranscoderQueue() function */

/*******************************************************************************
Name        : vidtrqueue_ReleasePicturesFromTranscoderQueue
Description : Release all pictures in transcoder queue that have been requested
              by a remove.
Parameters  : Transcoder handle, pointer to picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void vidtrqueue_ReleasePicturesFromTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p)
{
    U32 Index;

    for (Index = 0; Index < NB_TRQUEUE_ELEM; Index++)
    {
        if (VIDTRAN_Data_p->QueueElements[Index].State == VIDTRQUEUE_STATE_RELEASE_REQUESTED)
        {
            /* Found a Transcode Queue Element Removed but not released */
            /* Checking if we have a decimated picture buffer. To be released first */
            if (VIDTRAN_Data_p->QueueElements[Index].DecimatedPictureBuffer_p != NULL)
            {
                RELEASE_PICTURE_BUFFER(VIDTRAN_Data_p->BufferManagerHandle, VIDTRAN_Data_p->QueueElements[Index].DecimatedPictureBuffer_p, VIDCOM_XCODE_MODULE_BASE);
                TraceVerbose(("\r\nREM 0x%x", VIDTRAN_Data_p->QueueElements[Index].DecimatedPictureBuffer_p));
            }

            RELEASE_PICTURE_BUFFER(VIDTRAN_Data_p->BufferManagerHandle, VIDTRAN_Data_p->QueueElements[Index].MainPictureBuffer_p, VIDCOM_XCODE_MODULE_BASE);
            TraceVerbose(("\r\nREM 0x%x", VIDTRAN_Data_p->QueueElements[Index].MainPictureBuffer_p));

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDTRAN_Data_p->Statistics.EncodeSourcePictureReleased++;
#endif /* STVID_DEBUG_GET_STATISTICS */

            STOS_TaskLock();

            /* Clearing addresses to free the element */
            VIDTRAN_Data_p->QueueElements[Index].MainPictureBuffer_p = NULL;
            VIDTRAN_Data_p->QueueElements[Index].DecimatedPictureBuffer_p = NULL;

            /* Release done */
            VIDTRAN_Data_p->QueueElements[Index].State = VIDTRQUEUE_STATE_FREE;

            STOS_TaskUnlock();
        }
    }

    return;
}

/*******************************************************************************
Name        : vidtrqueue_InformationOnPicturesInTranscoderQueue
Description : Gives information on queue (nb pictures in transcoder queue, nb of picture in display queue)
Parameters  : Transcoder handle
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
void vidtrqueue_InformationOnPicturesInTranscoderQueue(const VIDTRAN_Data_t * VIDTRAN_Data_p, VIDTRQUEUE_QueueInfo_t * const Queue_Info_p)
{
    U32 Index;

    Queue_Info_p->NbOfPicturesFree = 0;
    Queue_Info_p->NbOfPicturesValidInQueue = 0;
    Queue_Info_p->NbOfPicturesStillInDisplayQueue = 0;
    Queue_Info_p->NbOfPicturesReleaseRequested = 0;
    Queue_Info_p->NbOfPicturesToBeTranscoded = 0;

    /* Do we have to protect this area ? */
    /* STOS_TaskLock(); */

    for (Index = 0; Index < NB_TRQUEUE_ELEM; Index++)
    {
        if (VIDTRAN_Data_p->QueueElements[Index].State != VIDTRQUEUE_STATE_FREE)
        {
            if (VIDTRAN_Data_p->QueueElements[Index].State == VIDTRQUEUE_STATE_VALID)
            {
                /* Found a Transcode Queue Element available */
                Queue_Info_p->NbOfPicturesValidInQueue ++;

                /* Checking if still in Display Queue */
                if (VIDTRAN_Data_p->QueueElements[Index].MainPictureBuffer_p != NULL)
                {
                    if (VIDTRAN_Data_p->QueueElements[Index].MainPictureBuffer_p->Buffers.IsInDisplayQueue == TRUE) /* Is in display queue but may be on display */
                    {
                         Queue_Info_p->NbOfPicturesStillInDisplayQueue++;
                    }
                }
                else
                {
                    TraceVerbose(("\r\nBad Picture (null framebuffer) 0x%x !", VIDTRAN_Data_p->QueueElements[Index]));
                }
            }

            /* Checking if a remove is requested */
            if (VIDTRAN_Data_p->QueueElements[Index].State == VIDTRQUEUE_STATE_RELEASE_REQUESTED)
            {
                 Queue_Info_p->NbOfPicturesReleaseRequested++;
            }
        }
        else
        {
            /* This element is free */
            Queue_Info_p->NbOfPicturesFree++;
        }
    }

    /* End of critical section */
    /* STOS_TaskUnlock(); */
    Queue_Info_p->NbOfPicturesToBeTranscoded = Queue_Info_p->NbOfPicturesValidInQueue - Queue_Info_p->NbOfPicturesStillInDisplayQueue;

    return;
}

#ifdef STVID_DEBUG_TRQUEUE
/*******************************************************************************
Name        : vidtrqueue_HistoryOfPicturesInTranscoderQueue
Description : Display history of Nb of pictures in transcoder queue
Parameters  : Transcoder handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void vidtrqueue_HistoryOfPicturesInTranscoderQueue(const VIDTRAN_Data_t * VIDTRAN_Data_p)
{
    U32 Index, CurrentNextElementInHistory;

    UNUSED_PARAMETER(VIDTRAN_Data_p);

    TraceBuffer(("History Of Transcoding queue\n"));
    CurrentNextElementInHistory = NextElementInHistory;

    if (HistoryOfElementsInTranscoderQueueTable[0] != 0xFFFFFFFF)
    {
        for (Index = CurrentNextElementInHistory; Index < MAX_NUMBER_OF_ELEMENTS_TRACED; Index++)
        {
            TraceBuffer(("Index:%02d  Queue Size:%04d\n",Index, HistoryOfElementsInTranscoderQueueTable[Index]));
        }
    }

    /* First entries of the table */
    for (Index = 0; Index < CurrentNextElementInHistory; Index++)
    {
        TraceBuffer(("Index:%02d  Queue Size:%04d\n",Index, HistoryOfElementsInTranscoderQueueTable[Index]));
    }

    return;
}

#endif /* STVID_DEBUG_TRQUEUE */

/* End of vid_trqueue.c */



