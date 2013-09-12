/*******************************************************************************

File name   : vid_queu.c

Description : Video display source file. Display queue manager.

   viddisp_InitQueueElementsPool
   viddisp_GetPictureElement
   viddisp_RemovePictureFromDisplayQueue
   viddisp_InsertPictureInDisplayQueue
   VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue
   viddisp_PurgeQueues

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
 02 Sep 2002        Created                                   Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "stddefs.h"
#include "vid_disp.h"
#include "queue.h"

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART */

/* Select the Traces you want */
#define TrElement                TraceOn
#define TrPurge                  TraceOn

#define TrWarning                TraceOn
/* TrError is now defined in vid_trace.h */

/* "vid_trace.h" should be included AFTER the definition of the traces wanted */
#include "vid_trace.h"

/* ---------------------------------------------- */

#ifdef TRACE_QUEUE /* define or uncomment in ./makefile to set traces */
  #ifdef TRACE_QUEUE_VERBOSE
    #define TraceVerbose(x) TraceBuffer(x)
  #else
    #define TraceVerbose(x)
  #endif
  #include "vid_trc.h"
#else
  #define TraceInfo(x)
  #define TraceVerbose(x)
#endif

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : viddisp_InitQueueElementsPool
Description : Reset all ellements used in display queue
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_InitQueueElementsPool(VIDDISP_Data_t * VIDDISP_Data_p)
{
    U32 Index;
    for (Index = 0; Index < NB_QUEUE_ELEM; Index++)
    {
        VIDDISP_Data_p->DisplayQueueElements[Index].PrimaryPictureBuffer_p = NULL;
        VIDDISP_Data_p->DisplayQueueElements[Index].SecondaryPictureBuffer_p = NULL;
        VIDDISP_Data_p->DisplayQueueElements[Index].PictureInformation_p = NULL;
        VIDDISP_Data_p->DisplayQueueElements[Index].Next_p          = NULL;
    }
}
/*******************************************************************************
Name        : viddisp_GetPictureElement
Description : return a pointer to a free display queue element.
              This function returns NULL if no display queue element is available.
Parameters  :
Assumptions :
Limitations :
Returns     : U32 Index of an unused element
*******************************************************************************/
VIDDISP_Picture_t * viddisp_GetPictureElement(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    U32                 Index;

    for (Index = 0; Index < NB_QUEUE_ELEM; Index++)
    {
        if ( (VIDDISP_Data_p->DisplayQueueElements[Index].PrimaryPictureBuffer_p == NULL) &&
             (VIDDISP_Data_p->DisplayQueueElements[Index].SecondaryPictureBuffer_p == NULL) )
        {
            /* Found a Display Queue Element available */
            return( &(VIDDISP_Data_p->DisplayQueueElements[Index]) );
        }
    }

    /* No Display Queue Element available! */
    TrError(("\r\nError! Display Queue full!"));

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDISP_Data_p->StatisticsPbDisplayQueueOverflow++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    return(NULL);
}

/*******************************************************************************
Name        : viddisp_ReleasePictureBuffer
Description : Release a picture buffer from display queue
Parameters  : Display handle, pointer to picture buffer,
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_ReleasePictureBuffer(const VIDDISP_Handle_t        DisplayHandle,
                                  VIDBUFF_PictureBuffer_t *     PictureBufferToRelease_p)
{
    VIDDISP_Data_t *            VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (PictureBufferToRelease_p == NULL)
    {
        TrError(("Invalid Picture Buffer to release\r\n"));
        return;
    }

    VIDBUFF_SetDisplayStatus(VIDDISP_Data_p->BufferManagerHandle,
                             VIDBUFF_NOT_ON_DISPLAY,
                             PictureBufferToRelease_p);

    PictureBufferToRelease_p->Buffers.IsInDisplayQueue = FALSE;

    /* Inform VIDBUFF that this picture has been removed from Display Queue*/
    VIDBUFF_PictureRemovedFromDisplayQueue(VIDDISP_Data_p->BufferManagerHandle, PictureBufferToRelease_p);

    RELEASE_PICTURE_BUFFER( VIDDISP_Data_p->BufferManagerHandle, PictureBufferToRelease_p, VIDCOM_VIDQUEUE_MODULE_BASE);
    TrElement(("\r\nRelease 0x%x", PictureBufferToRelease_p));
}

/*******************************************************************************
Name        : viddisp_RemovePictureFromDisplayQueue
Description : Release picture from display queue
Parameters  : Display handle, pointer to picture,
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_RemovePictureFromDisplayQueue(const VIDDISP_Handle_t DisplayHandle,
                                 VIDDISP_Picture_t * const Picture_p)
{
    VIDDISP_Picture_t *         CurrentPicture_p;
    VIDDISP_Picture_t *         PreviousPicture_p;
    VIDDISP_Data_t *            VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (Picture_p == NULL)
    {
        TraceInfo(("un-queue wrong picture \r\n"));
        return;
    }

    if ((Picture_p == VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p) ||
        (Picture_p == VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p) ||
        (Picture_p == VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p) ||
        (Picture_p == VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p))
    {
        TraceInfo(("ERROR : try to remove a used picture \r\n"));
        return;
    }

    /*TrElement(("\r\nREM 0x%x", Picture_p->PictureBuffer_p));*/

    /* Display Queue is not empty */
    CurrentPicture_p  = VIDDISP_Data_p->DisplayQueue_p;
    PreviousPicture_p = NULL;
    if (CurrentPicture_p != NULL)
    {
        /* Test picture in display queue */
        while ((CurrentPicture_p->Next_p != NULL)
        &&  (CurrentPicture_p != Picture_p)) /* Picture_p not found */
        {
            /* Continue search with next in display queue */
            PreviousPicture_p = CurrentPicture_p;
            CurrentPicture_p = CurrentPicture_p->Next_p;
        }
        if (CurrentPicture_p == Picture_p)
        {
            TraceVerbose(("un-queue %i \r\n",Picture_p));

            /* Found picture to remove */
            if (PreviousPicture_p != NULL)
            {
                /* Picture was not the first in display queue */
                PreviousPicture_p->Next_p = CurrentPicture_p->Next_p;
            }
            else
            {
                /* Picture was the first in display queue */
                VIDDISP_Data_p->DisplayQueue_p = CurrentPicture_p->Next_p;
            }

            if (Picture_p == VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p)
            {
                /* The Picture pointed by "PictDisplayedWhenDisconnect_p" has been removed so we should erase this reference */
                VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p = NULL;
            }
            if (Picture_p == VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p)
            {
                /* The Picture pointed by "PictDisplayedWhenDisconnect_p" has been removed so we should erase this reference */
                VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p = NULL;
            }

             Picture_p->Next_p = NULL;

            /* Check if there is a decimated picture to release */
            if (Picture_p->SecondaryPictureBuffer_p != NULL)
            {
                /* Release the decimated picture */
                viddisp_ReleasePictureBuffer(DisplayHandle, Picture_p->SecondaryPictureBuffer_p);
                Picture_p->SecondaryPictureBuffer_p = NULL;
            }

            /* Check if there is a main picture to release */
            if (Picture_p->PrimaryPictureBuffer_p != NULL)
            {
                /* Release the main picture */
                viddisp_ReleasePictureBuffer(DisplayHandle, Picture_p->PrimaryPictureBuffer_p);
                Picture_p->PrimaryPictureBuffer_p = NULL;
            }

            Picture_p->PictureInformation_p = NULL;
            return;
        }
    }

    /* picture has not been found !! */
    TraceInfo(("un-queue wrong picture \r\n"));
    TrError(("\r\nError! Pict 0x%x not found!", Picture_p));

} /* End of viddisp_RemovePictureFromDisplayQueue() function */

/*******************************************************************************
Name        : viddisp_InsertPictureInDisplayQueue
Description : Insert picture in display queue taking into account the
                ExtendedPresentationOrderPictureID
Parameters  : Display handle, pointer to picture
Assumptions : By default increasing insertion order is choosen.
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddisp_InsertPictureInDisplayQueue(const VIDDISP_Handle_t DisplayHandle,
                                VIDDISP_Picture_t * const Picture_p,
                                VIDDISP_Picture_t ** Queue_p_p)
{
    VIDDISP_Data_t *    VIDDISP_Data_p;
    VIDDISP_Picture_t * CurrentPicture_p = NULL;
    VIDDISP_Picture_t * PreviousPicture_p= NULL;

    if (Picture_p == NULL)
    {
        TraceInfo(("in-queue wrong picture \r\n"));
        /* No picture: nothing to do */
        return;
    }

    VIDDISP_Data_p = (VIDDISP_Data_t *)DisplayHandle;

    TrElement(("\r\nINS 0x%x in 0x%x", Picture_p->PrimaryPictureBuffer_p, Queue_p_p));

    /* Lock the Picture Buffer because it is now under Display control */
    if (Picture_p->PrimaryPictureBuffer_p != NULL)
    {
        TAKE_PICTURE_BUFFER(VIDDISP_Data_p->BufferManagerHandle, Picture_p->PrimaryPictureBuffer_p, VIDCOM_VIDQUEUE_MODULE_BASE);
        Picture_p->PrimaryPictureBuffer_p->Buffers.IsInDisplayQueue = TRUE;
    }

    /* Lock also the associated decimated Picture (if it exists) */
    if (Picture_p->SecondaryPictureBuffer_p != NULL)
    {
        TAKE_PICTURE_BUFFER(VIDDISP_Data_p->BufferManagerHandle, Picture_p->SecondaryPictureBuffer_p, VIDCOM_VIDQUEUE_MODULE_BASE);
        Picture_p->SecondaryPictureBuffer_p->Buffers.IsInDisplayQueue = TRUE;
    }

    TraceVerbose(("in-queue %i in %i  \r\n",Picture_p,Queue_p_p));
    if (*Queue_p_p == NULL)
    {
        /* Display Queue is empty */
        * Queue_p_p = Picture_p;
    }
    else
    {
        /* Display Queue is not empty */
        CurrentPicture_p = * Queue_p_p;
        PreviousPicture_p = CurrentPicture_p;

        while (CurrentPicture_p != NULL)
        {
            PreviousPicture_p = CurrentPicture_p;
            CurrentPicture_p = PreviousPicture_p->Next_p;
        }
        /* Found the place where to insert the frame: at last place */
        /* just after previous frame */
        PreviousPicture_p->Next_p = Picture_p;/* Insert frame */
    }

    /* link inserted picture with its following in display queue */
    Picture_p->Next_p = CurrentPicture_p;

} /* End of viddisp_InsertPictureInDisplayQueue() function */

/*******************************************************************************
Name        : VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue
Description : Clean display queue even if pictures should be displayed
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(
                                        const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Picture_t * CurrentPicture_p;
    VIDDISP_Picture_t * NextPicture_p;
    VIDDISP_Data_t *    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);

    CurrentPicture_p    = VIDDISP_Data_p->DisplayQueue_p;
    while (CurrentPicture_p != NULL)
    {
        NextPicture_p = CurrentPicture_p->Next_p;
        if ((CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p) &&
            (CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p) &&
            (CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p) &&
            (CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p))
        {
            /* do not remove if only one element */
            if ((CurrentPicture_p->Next_p != NULL) ||
                (CurrentPicture_p != VIDDISP_Data_p->DisplayQueue_p))
            {
                viddisp_RemovePictureFromDisplayQueue(DisplayHandle, CurrentPicture_p);
            }
            else
            {
                /* in smooth backward mode, reset extended temporal reference */
                /* will allow VIDBUFF_GetUnusedPictureFrameBuffer function to */
                /* considerate this bufer as the oldest reference if P or I */
                CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID = 0;
                CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = 0;
            }
        }
        else
        {
            /* in smooth backward mode, reset extended temporal reference will*/
            /* allow VIDBUFF_GetUnusedPictureFrameBuffer function to */
            /* considerate this bufer as the oldest reference if P or I */
            /* Save ExtendedPresentationOrderPictureID.ID value to be reused when switching from FW to BW. */
            CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = 0;
        }
        CurrentPicture_p = NextPicture_p;
    } /* end while */

    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

    return(ST_NO_ERROR);
} /* end VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue */
/*******************************************************************************
Name        : viddisp_PurgeQueues
Description : Try to remove unused elements
Assumptions : This function must be called once and ONLY once
              per layer per Vsync. (Ex from synchronize-display) because it
              sequences the 'after display-life' of the buffers
Param       :
*******************************************************************************/
void viddisp_PurgeQueues(U32 Layer, VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *        VIDDISP_Data_p;
    VIDDISP_Picture_t *     CurrentPicture_p;
    VIDDISP_Picture_t *     NextPicture_p;
    BOOL                    Continue;

    /* shortcut */
    VIDDISP_Data_p  = (VIDDISP_Data_t *) DisplayHandle;

    if (VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p == NULL)
    {
        return; /* don't purge if nothing is displayed yet */
    }

    /* mark picture in queue */
    /*-------------------------------*/
    CurrentPicture_p    = VIDDISP_Data_p->DisplayQueue_p;
    Continue = TRUE; /* first entrance in the do-while */
    while((Continue)&&(CurrentPicture_p != NULL))
    {
        if ((CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[Layer].CurrentlyDisplayedPicture_p) &&
            (CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p))
        {
            /* Mark the picture */
            if (CurrentPicture_p->Counters[Layer].CounterInDisplay >= 1)
            {
                CurrentPicture_p->Counters[Layer].CounterInDisplay --;
            }
        }
        else
        {
            Continue = FALSE; /* must exit now */
        }
        CurrentPicture_p = CurrentPicture_p->Next_p;
    }/* end while */

    /* And Now, scan the marked pictures */
    /*-----------------------------------*/
    CurrentPicture_p    = VIDDISP_Data_p->DisplayQueue_p;
    Continue = TRUE; /* first entrance in the do-while */
    while((Continue)&&(CurrentPicture_p != NULL))
    {
        NextPicture_p = CurrentPicture_p->Next_p; /* store next before remove */
        if ((CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p) &&
            (CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p) &&
            (CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p) &&
            (CurrentPicture_p != VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p))
        {
            if ((CurrentPicture_p->Counters[0].CounterInDisplay == 0) &&
                (CurrentPicture_p->Counters[1].CounterInDisplay == 0))
            {   /* do not remove if only one element */
                if ((CurrentPicture_p->Next_p != NULL) ||
                    (CurrentPicture_p != VIDDISP_Data_p->DisplayQueue_p))
                {
                    viddisp_RemovePictureFromDisplayQueue(DisplayHandle, CurrentPicture_p);
                }
                else
                {
                    TrPurge(("\r\n 0x%x not purged: only one elem", CurrentPicture_p));
                }
            }
            else
            {
                TrPurge(("\r\n 0x%x not purged: count %d %d", CurrentPicture_p, CurrentPicture_p->Counters[0].CounterInDisplay, CurrentPicture_p->Counters[1].CounterInDisplay));
            }
        }
        else
        {
            TrPurge(("\r\n 0x%x not purged: still on display", CurrentPicture_p));
            Continue = FALSE; /* must exit now */
        }
        CurrentPicture_p = NextPicture_p;
    }/* end while */

    /* And Now, reset counters of currently displayed picture when switching BW/FW or FW/BW */
    /*-----------------------------------*/
    if ((VIDDISP_Data_p->DisplayForwardNotBackward && !VIDDISP_Data_p->LastDisplayDirectionIsForward) ||
        (!VIDDISP_Data_p->DisplayForwardNotBackward && VIDDISP_Data_p->LastDisplayDirectionIsForward))
    {
        CurrentPicture_p = VIDDISP_Data_p->DisplayQueue_p;
        if (CurrentPicture_p != NULL)
        {
            CurrentPicture_p->Counters[Layer].TopFieldCounter = 0;
            CurrentPicture_p->Counters[Layer].BottomFieldCounter = 0;
            CurrentPicture_p->Counters[Layer].RepeatFieldCounter = 0;
        }
    }
} /* End of viddisp_PurgeQueues() function */

/*******************************************************************************
Name        : viddisp_ResetCountersInQueue
Description : This function parses the Display Queues and reset the Display Counters associated to
              a given Layer. It should be called when the Viewport on this Layer is closed.
              This will ensure that no Picture remain locked for this Layer
              (the pictures will be removed by viddisp_PurgeQueues() when they will not be used anymore by the other Layer)
Assumptions :
Param       :
*******************************************************************************/
void viddisp_ResetCountersInQueue(U32 Layer, VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t *        VIDDISP_Data_p;
    VIDDISP_Picture_t *     CurrentPicture_p;

    /* shortcut */
    VIDDISP_Data_p  = (VIDDISP_Data_t *) DisplayHandle;

    TrPurge(("\r\n viddisp_ResetCountersInQueue Layer %d", Layer));

    CurrentPicture_p    = VIDDISP_Data_p->DisplayQueue_p;

    while (CurrentPicture_p != NULL)
    {
        CurrentPicture_p->Counters[Layer].CounterInDisplay = 0;
        CurrentPicture_p = CurrentPicture_p->Next_p;
    }
}

/******************************************************************************/
