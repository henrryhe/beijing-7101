/*******************************************************************************

File name   : vid_cmd.c

Description : Video display source file. Display API functions.

    VIDDISP_Freeze
    VIDDISP_Pause
    VIDDISP_Resume
    VIDDISP_Start
    VIDDISP_Step
    VIDDISP_Stop
    VIDDISP_SkipFields
    VIDDISP_RepeatFields
    VIDDISP_DisplayBitmap
    VIDDISP_ShowPicture
    VIDDISP_ShowPictureLayer
    VIDDISP_Skip

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 02 Sep 2002        Created                                   Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "stddefs.h"
#include "stvid.h"
#include "display.h"
#include "vid_disp.h"
#include "queue.h"
#include "fields.h"

#ifdef TRACE_CMD /* define or uncomment in ./makefile to set traces */
  #include "vid_trc.h"
#else
  #define TraceBuffer(x)
#endif

/* Private Constants -------------------------------------------------------- */

#define MAX_WAIT_STOP_TIME        (2 * STVID_MAX_VSYNC_DURATION)

/* Private Macros ----------------------------------------------------------- */

#define Device(VirAdd) (void*) ( \
         (U32)(VirAdd)\
        -(U32)(VIDDISP_Data_p->VirtualMapping.VirtualBaseAddress_p)\
        +(U32)(VIDDISP_Data_p->VirtualMapping.PhysicalAddressSeenFromDevice_p))

#define DEVICE_TYPE_7100 ( \
    ((VIDDISP_Data_p->DeviceType >= STVID_DEVICE_TYPE_7100_MPEG) && \
    (VIDDISP_Data_p->DeviceType <= STVID_DEVICE_TYPE_7100_DIGITAL_INPUT)) ? TRUE: FALSE )

#define DEVICE_TYPE_7109 ( \
    ((VIDDISP_Data_p->DeviceType >= STVID_DEVICE_TYPE_7109_MPEG) && \
    (VIDDISP_Data_p->DeviceType <= STVID_DEVICE_TYPE_7109_DIGITAL_INPUT)) ? TRUE: FALSE )

#define DEVICE_TYPE_7200 ( \
    ((VIDDISP_Data_p->DeviceType >= STVID_DEVICE_TYPE_7200_MPEG) && \
    (VIDDISP_Data_p->DeviceType <= STVID_DEVICE_TYPE_7200_DIGITAL_INPUT)) ? TRUE: FALSE )

/* Static functions --------------------------------------------------------- */

static void ResetValuesOfChangeEVTs(const VIDDISP_Handle_t DisplayHandle);

/* Public functions --------------------------------------------------------- */

/*******************************************************************************
Name        : VIDDISP_Freeze
Description : Freeze display
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Freeze(const VIDDISP_Handle_t DisplayHandle,
                              const STVID_Freeze_t * const Freeze_p)
{
    VIDDISP_Data_t *  VIDDISP_Data_p;

    if (Freeze_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structure */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* Freeze state can now be changed even if the display was already frozen */
    VIDDISP_Data_p->FreezeParams.Freeze = *Freeze_p;

    if (VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_FREEZED)
    {
        return (STVID_ERROR_DECODER_FREEZING);
    }

    /* flag the cmd */
    VIDDISP_Data_p->FreezeParams.PauseRequestIsPending  = FALSE;
    VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = TRUE;

    return(ST_NO_ERROR);

} /* End of VIDDISP_Freeze() function */

/*******************************************************************************
Name        : VIDDISP_Pause
Description : Pause display
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Pause(const VIDDISP_Handle_t DisplayHandle,
                             const STVID_Freeze_t * const Freeze_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p;

    if (Freeze_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structure */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_PAUSED)
    {
        return (STVID_ERROR_DECODER_PAUSING);
    }

    /* Store Freeze informations */
    VIDDISP_Data_p->FreezeParams.Freeze = *Freeze_p;

    /* flag the cmd */
    VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = FALSE;
    VIDDISP_Data_p->FreezeParams.PauseRequestIsPending  = TRUE;

    return(ST_NO_ERROR);
} /* End of VIDDISP_Pause() function */

/*******************************************************************************
Name        : VIDDISP_Resume
Description : Resume display
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              STVID_ERROR_DECODER_NOT_PAUSING if not in pause or freeze
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Resume(const VIDDISP_Handle_t DisplayHandle)
{
    /* Find structure */
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (VIDDISP_Data_p->FreezeParams.PauseRequestIsPending
      || VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending)
    {
        /* There was non satisfied pause or freeze request */
        VIDDISP_Data_p->FreezeParams.PauseRequestIsPending  = FALSE;
        VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = FALSE;

        TraceBuffer(("Display resumed !! \r\n"));
        return(ST_NO_ERROR);
    }
    else if ((VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_PAUSED)
           ||(VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_FREEZED))
    {
        /* Update the Display process' state. */
        VIDDISP_Data_p->DisplayState = VIDDISP_DISPLAY_STATE_DISPLAYING;

        TraceBuffer(("Display resumed !! \r\n"));
        return(ST_NO_ERROR);
    }

    /* case display was displaying state */
    return(STVID_ERROR_DECODER_NOT_PAUSING);

} /* End of VIDDISP_Resume() function */

/*******************************************************************************
Name        : VIDDISP_Start
Description : Start the display process.
              This function takes sure the display process is started after a
              stop of the video driver.
Parameters  : Display handle, boolean to say don't reset changed EVT values
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER either.
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Start(const VIDDISP_Handle_t DisplayHandle, const BOOL KeepLastValuesOfChangeEVT)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDCOM_InternalProfile_t                CommonMemoryProfile;
    VIDBUFF_AvailableReconstructionMode_t   Reconstruction;

    /* Any Request pending */
    VIDDISP_Data_p->StopParams.StopRequestIsPending     = FALSE;
    VIDDISP_Data_p->StopParams.StopMode                 = VIDDISP_STOP_NOW;
    VIDDISP_Data_p->NotifyStopWhenFinish                = FALSE;
    VIDDISP_Data_p->FreezeParams.PauseRequestIsPending  = FALSE;
    VIDDISP_Data_p->FreezeParams.FreezeRequestIsPending = FALSE;
    VIDDISP_Data_p->FreezeParams.SteppingOrderReceived  = FALSE;

    /* Force a 'change evt' on next picture,  */
    /* needed if memory profile was changed during a stop/start sequence */
    if(!KeepLastValuesOfChangeEVT)
    {
        ResetValuesOfChangeEVTs(DisplayHandle);
    }

    /* Initialize the display reconstruction mode in case of digital    */
    /* input.                                                           */
    if ( (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT))
    {
        /* Get current memory profile.                                       */
        if ( VIDBUFF_GetMemoryProfile(VIDDISP_Data_p->BufferManagerHandle,
                &CommonMemoryProfile) != ST_NO_ERROR )
        {
            /* Error in internal parameters. */
            return(ST_ERROR_BAD_PARAMETER);
        }

        Reconstruction =
                (CommonMemoryProfile.ApplicableDecimationFactor
                == STVID_DECIMATION_FACTOR_NONE ?
                VIDBUFF_MAIN_RECONSTRUCTION :
                VIDBUFF_SECONDARY_RECONSTRUCTION);

        if (VIDDISP_Data_p->LayerDisplay[0].IsOpened)
        {
            VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode = Reconstruction;
        }
        if (VIDDISP_Data_p->LayerDisplay[1].IsOpened)
        {
            VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode = Reconstruction;
        }
    }

    /* Set counters of picture on display to zero to let it remove as soon as */
    /* the display re-start even if fields remain                             */
    if (VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p != NULL)
    {
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[0].TopFieldCounter    = 0;
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[0].BottomFieldCounter = 0;
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[0].RepeatFieldCounter = 0;
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[0].RepeatProgressiveCounter= 0;
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[1].TopFieldCounter    = 0;
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[1].BottomFieldCounter = 0;
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[1].RepeatFieldCounter = 0;
        VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p->Counters[1].RepeatProgressiveCounter= 0;
    }

    VIDDISP_Data_p->DisplayState = VIDDISP_DISPLAY_STATE_DISPLAYING;

    TraceBuffer(("Display started !! \r\n"));

    /* Ok, return result. */
    return(ST_NO_ERROR);

} /* End of VIDDISP_Start() function. */

/*******************************************************************************
Name        : VIDDISP_Step
Description : Perform the step action.
Parameters  : Display handle
Assumptions : The display process' state is "PAUSED" or "FREEZED".
Limitations :
Returns     : ST_NO_ERROR if success.
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Step(const VIDDISP_Handle_t DisplayHandle)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* Lock access */
    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);

    /* Remember that a step action is wanted. It will be taken into account */
    /* during the next VSync occurrence. */
    VIDDISP_Data_p->FreezeParams.SteppingOrderReceived = TRUE;

    /* Un-lock access */
    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

    /* Ok, return result. */
    return(ST_NO_ERROR);

} /* End of VIDDISP_Step() function */

/*******************************************************************************
Name        : VIDDISP_Stop
Description : Stop the display on the picture and on the field required
            by STVID_Stop NEXT_REFERENCE, NEXT_I, NOW or END_OF_DATA
Parameters  : Display Handle
              LastPicturePTS_p : pointer an a PTS which should be the one
              of the last picture to display is display is forward
              StopNow : Stop Request is a STVID_STOP_NOW or not
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if LastPicturePTS_p == NULL
              and StopNow is FALSE
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Stop(const VIDDISP_Handle_t DisplayHandle,
                            const STVID_PTS_t * const LastPicturePTS_p,
                            const VIDDISP_Stop_t StopMode)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    STOS_Clock_t     MaxWaitStopTime;

    if ((StopMode == VIDDISP_STOP_ON_PTS) && (LastPicturePTS_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* No need to lock access to display queue */

    /* Stop Request Is Pending */
    if (StopMode == VIDDISP_STOP_ON_PTS)
    {
        /* if stop not now, PTS or Extended Temporal Reference must be valid */
        VIDDISP_Data_p->StopParams.RequestStopPicturePTS = *LastPicturePTS_p;
    }

    /* Take every tokens of "ActuallyStoppedSemaphore_p" to consume previously signaled stop */
    while (semaphore_wait_timeout(VIDDISP_Data_p->StopParams.ActuallyStoppedSemaphore_p, TIMEOUT_IMMEDIATE) == 0)
    {
        /* Nothing to do: just to decrement counter until 0, to consume previously signaled stop */;
    }

    VIDDISP_Data_p->StopParams.StopRequestIsPending = TRUE;
    VIDDISP_Data_p->StopParams.StopMode = StopMode;

    /* wait for actual stop completion */
    MaxWaitStopTime = time_plus(time_now(), MAX_WAIT_STOP_TIME);
    if (semaphore_wait_timeout(VIDDISP_Data_p->StopParams.ActuallyStoppedSemaphore_p, &MaxWaitStopTime) != 0)
    {
        TraceBuffer(("VIDDISP_Stop() returns on SemaphoreWait timeout !\r\n"));
    }

    /* Ok, return result. */
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDDISP_SkipFields
Description : Skip n fields in display
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              returns number of fields skipped in NumberDone_p if not NULL
*******************************************************************************/
ST_ErrorCode_t VIDDISP_SkipFields(const VIDDISP_Handle_t DisplayHandle,
                                  const U32 NumberOfFields,
                                  U32 * const NumberDone_p,
                                  const BOOL SkipAllFieldsOfSamePictureAllowed,
                                  const VIDDISP_SkipMode_t SkipMode)
{
    U32                         FieldsSkipped = 0;
    VIDDISP_Picture_t   *       CurrentPicture_p;
    VIDDISP_Picture_t   *       NextPictureInQueue_p;
    VIDDISP_FieldDisplay_t      NextFieldOnDisplay;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    VIDDISP_Data_t *            VIDDISP_Data_p;

    TraceBuffer(("Request to skip %d fields.\r\n", NumberOfFields));

    if (NumberDone_p != NULL)
    {
        *NumberDone_p = 0;
    }
    else
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    VIDDISP_Data_p = (VIDDISP_Data_t *)DisplayHandle;
    if (VIDDISP_Data_p->DisplayState == VIDDISP_DISPLAY_STATE_FREEZED)
    {
        return (ST_NO_ERROR);
    }

    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);
    /* If display queue is empty, it means that there is still no video. We can consider that the skip was done fine */
    /* (Note that it also lets trick mode not accumulate late */
    if (VIDDISP_Data_p->DisplayQueue_p == NULL)
    {
        *NumberDone_p = NumberOfFields;
    }
    else
    {
        switch(SkipMode)
        {
            case VIDDISP_SKIP_STARTING_FROM_PREPARED_PICTURE_FOR_NEXT_VSYNC:
                CurrentPicture_p = VIDDISP_Data_p->LayerDisplay
                    [VIDDISP_Data_p->DisplayRef].PicturePreparedForNextVSync_p;
                break;
            case VIDDISP_SKIP_STARTING_FROM_CURRENTLY_DISPLAYED_PICTURE:
                CurrentPicture_p = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p;
                break;
            default:
                CurrentPicture_p = NULL;
        }
        /* the picture may be on display  */
        if (CurrentPicture_p == NULL)
        {
            *NumberDone_p = NumberOfFields;
             STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
             return(ST_NO_ERROR);
        }

        if(SkipMode == VIDDISP_SKIP_STARTING_FROM_CURRENTLY_DISPLAYED_PICTURE)
        {
            /* Found pict on display with counters not 0, decrement its counters */
            /* But it is mandatory to keep at least counters of first picture at */
            /* least to one */
            while ((CurrentPicture_p != NULL) /* picture on screen: */
                   &&(RemainingFieldsToDisplay(*CurrentPicture_p) > 1)
                   && (FieldsSkipped != NumberOfFields))
            {
                if (!(VIDDISP_Data_p->DisplayForwardNotBackward))
                {
                    ErrorCode = viddisp_DecrementFieldCounterBackward(
                        CurrentPicture_p,
                        &NextFieldOnDisplay,
                        VIDDISP_Data_p->DisplayRef);
                }
                else
                {
                    ErrorCode = viddisp_DecrementFieldCounterForward(
                        CurrentPicture_p,
                        &NextFieldOnDisplay,
                        VIDDISP_Data_p->DisplayRef);
                }
                FieldsSkipped++;
                /* Set first picture from which to begin skipping/repeating for frame rate conversion*/
                VIDDISP_Data_p->InitialTemporalReferenceForSkipping =
                    VIDDISP_Data_p->InitialTemporalReferenceForRepeating =
                    VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;

                TraceBuffer(("\r\nSkip requested : Initial skipping/repeating is FRConvTempRef=%d PTS=%d\r\n",
                             VIDDISP_Data_p->InitialTemporalReferenceForSkipping,
                             CurrentPicture_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.PTS));
            }
            /* there are still fields to skip ! Consider next pictures, */
            /* which are not on display */
            CurrentPicture_p = CurrentPicture_p->Next_p;
        }
        else /* VIDDISP_SKIP_STARTING_FROM_PREPARED_PICTURE_FOR_NEXT_VSYNC skip mode */
        {
            /* Remove the already prepared. We will put it again on the display if it is not totally skipped */
            /* This is necessary for calls to viddisp_RemovePictureFromDisplayQueue() to work. Otherwise, it exits with no effect. */
            VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].PicturePreparedForNextVSync_p = NULL;
        }

        while ((FieldsSkipped != NumberOfFields) && (CurrentPicture_p != NULL))
        {
            /* References pict have never to have their counters set to 0 */
            /* and removed */
            /* Allow to decrement counter of pict not yet on display until 0. */
            if ((RemainingFieldsToDisplay(*CurrentPicture_p)) == 0)
            {
                /* This picture has null counter so it should be removed from display queue */

                /* remember Next picture before removing CurrentPicture_p */
                NextPictureInQueue_p = CurrentPicture_p->Next_p;

                viddisp_RemovePictureFromDisplayQueue(DisplayHandle,
                                                 CurrentPicture_p);
                CurrentPicture_p = NextPictureInQueue_p;
            }
            else if ( ( (RemainingFieldsToDisplay(*CurrentPicture_p)) == 1) && (!SkipAllFieldsOfSamePictureAllowed) )
            {
                /* Time to check decrement of next pict in display queue... */
                CurrentPicture_p = CurrentPicture_p->Next_p;
                TraceBuffer(("=== Suspicious skips! ===\r\n"));
            }
            else
            {
                if (!VIDDISP_Data_p->DisplayForwardNotBackward)
                {
                    ErrorCode = viddisp_DecrementFieldCounterBackward(
                                                CurrentPicture_p,
                                                &NextFieldOnDisplay,
                                                VIDDISP_Data_p->DisplayRef);
                }
                else
                {
                    ErrorCode = viddisp_DecrementFieldCounterForward(
                                                CurrentPicture_p,
                                                &NextFieldOnDisplay,
                                                VIDDISP_Data_p->DisplayRef);
                }
                if (NextFieldOnDisplay == VIDDISP_FIELD_DISPLAY_NONE)
                {
                    /* remember Next picture in display queue before removing */
                    /* it, in order to run correctly the while display queue  */
                    /* until its end. */
                    NextPictureInQueue_p = CurrentPicture_p->Next_p;

                    viddisp_RemovePictureFromDisplayQueue(DisplayHandle,
                                                     CurrentPicture_p);
                    CurrentPicture_p = NextPictureInQueue_p;
                }

                if (ErrorCode == ST_NO_ERROR)
                {
                    FieldsSkipped++;
                    /* Set first picture from which to begin skipping/repeating for frame rate conversion*/

                VIDDISP_Data_p->InitialTemporalReferenceForSkipping =
                    VIDDISP_Data_p->InitialTemporalReferenceForRepeating =
                    VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;

                    /* CurrentPicture_p->PictureBuffer_p can be NULL now that we have removed this picture from display queue */
                    TraceBuffer(("\r\nSkip requested : Initial skipping/repeating is FRConvTempRef=%d\r\n",
                        VIDDISP_Data_p->InitialTemporalReferenceForSkipping));
                }
            }
        } /* end while */

        /* Return number done if required */
        if (NumberDone_p != NULL)
        {
            *NumberDone_p = FieldsSkipped;
        }
        /* Now update the picture to be displayed on the next VSYNC, if we skipped a
         * lot of fields. */
        if(SkipMode == VIDDISP_SKIP_STARTING_FROM_PREPARED_PICTURE_FOR_NEXT_VSYNC)
        {
            /* Warning: "CurrentPicture_p" may be NULL if we have skiped more Pictures than available */
            VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].PicturePreparedForNextVSync_p = CurrentPicture_p;
            if (CurrentPicture_p != NULL)
            {
                VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].PicturePreparedForNextVSync_p->Counters[VIDDISP_Data_p->DisplayRef].NextFieldOnDisplay = NextFieldOnDisplay;
            }
        }
    }

    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

    return(ST_NO_ERROR);

} /* End of VIDDISP_SkipFields() function */


/*******************************************************************************
Name        : VIDDISP_RepeatFields
Description : Repeat n fields in display
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              returns number of fields repeated in NumberDone_p if not NULL
*******************************************************************************/
ST_ErrorCode_t VIDDISP_RepeatFields(const VIDDISP_Handle_t DisplayHandle,
                                const U32 NumberOfFields,
                                U32 * const NumberDone_p)
{
    /* Find structure */
    VIDDISP_Data_t *        VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDDISP_Picture_t *     PictureDisplayedAtNextVSync_p;
    VIDDISP_FieldDisplay_t  RepeatedField;

    TraceBuffer(("Request to repeat %d fields.\r\n", NumberOfFields));

    /* case : repeat while state is not displaying */
    if (VIDDISP_Data_p->DisplayState != VIDDISP_DISPLAY_STATE_DISPLAYING)
    {
        if (NumberDone_p != NULL)
        {
            *NumberDone_p = NumberOfFields;
        }
        if (*NumberDone_p != 0)
        {
            /* Advise the outside world, fields has been repeated.  */
            STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                         VIDDISP_Data_p->RegisteredEventsID
                                    [VIDDISP_FIELDS_REPEATED_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST (NumberDone_p));
        }
        return (ST_NO_ERROR);
    }

    /* Lock access */
    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);

    /* If display queue is empty, it means that there is still no video . */
    /* It is better to let trick mode not accumulate late*/
    if (VIDDISP_Data_p->DisplayQueue_p == NULL)
    {
        /* Return number done if required */
        if (NumberDone_p != NULL)
        {
            *NumberDone_p = NumberOfFields;
        }
    }
    else
    {
        /* Consider output of reference display */
        PictureDisplayedAtNextVSync_p = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].PicturePreparedForNextVSync_p;

        if (PictureDisplayedAtNextVSync_p == NULL)
        {
            /* No picture prepared for next VSync so the current picture is going to be repeated. */
            PictureDisplayedAtNextVSync_p = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p;

            if (PictureDisplayedAtNextVSync_p == NULL)
            {
                /* Error! No picture is on display! */
                *NumberDone_p = NumberOfFields;
                STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
                return (ST_NO_ERROR);
            }
            else
            {
                RepeatedField = PictureDisplayedAtNextVSync_p->Counters[VIDDISP_Data_p->DisplayRef].CurrentFieldOnDisplay;
            }
        }
        else
        {
            RepeatedField = PictureDisplayedAtNextVSync_p->Counters[VIDDISP_Data_p->DisplayRef].NextFieldOnDisplay;
        }

        /* At this stage we are sure that PictureDisplayedAtNextVSync_p is not NULL and RepeatedField is set */

        switch (RepeatedField)
        {
            case VIDDISP_FIELD_DISPLAY_TOP :
                PictureDisplayedAtNextVSync_p->Counters[VIDDISP_Data_p->DisplayRef].TopFieldCounter += NumberOfFields;
                /* Set first picture from which to begin skipping/repeating for frame rate conversion*/
                VIDDISP_Data_p->InitialTemporalReferenceForSkipping =
                    VIDDISP_Data_p->InitialTemporalReferenceForRepeating =
                    VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;
                TraceBuffer(("\r\nRepeat requested : Initial skipping/repeating is FRConvTempRef=%d PTS=%d\r\n",
                        VIDDISP_Data_p->InitialTemporalReferenceForRepeating,
                        PictureDisplayedAtNextVSync_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.PTS));
                break;

            case VIDDISP_FIELD_DISPLAY_BOTTOM :
                PictureDisplayedAtNextVSync_p->Counters[VIDDISP_Data_p->DisplayRef].BottomFieldCounter += NumberOfFields;
                /* Set first picture from which to begin skipping/repeating for frame rate conversion*/
                VIDDISP_Data_p->InitialTemporalReferenceForSkipping =
                    VIDDISP_Data_p->InitialTemporalReferenceForRepeating =
                    VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;
                TraceBuffer(("\r\nRepeat requested : Initial skipping/repeating is FRConvTempRef=%d PTS=%d\r\n",
                        VIDDISP_Data_p->InitialTemporalReferenceForRepeating,
                        PictureDisplayedAtNextVSync_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.PTS));
                break;

            case VIDDISP_FIELD_DISPLAY_REPEAT :
                PictureDisplayedAtNextVSync_p->Counters[VIDDISP_Data_p->DisplayRef].RepeatFieldCounter += NumberOfFields;
                /* Set first picture from which to begin skipping/repeating for frame rate conversion*/
                VIDDISP_Data_p->InitialTemporalReferenceForSkipping =
                    VIDDISP_Data_p->InitialTemporalReferenceForRepeating =
                    VIDDISP_Data_p->FrameRatePatchCounterTemporalReference;
                TraceBuffer(("\r\nRepeat requested : Initial skipping/repeating is FRConvTempRef=%d PTS=%d\r\n",
                        VIDDISP_Data_p->InitialTemporalReferenceForRepeating,
                        PictureDisplayedAtNextVSync_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.PTS));
                break;

            case VIDDISP_FIELD_DISPLAY_NONE :
            default :
                TraceBuffer(("Inconsistence in sequencement !!\r\n"));
                break;
        }

        /* Return number done if required */
        if (NumberDone_p != NULL)
        {
            *NumberDone_p = NumberOfFields;
        }
    }

    /* Un-lock access */
    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

    if (*NumberDone_p != 0)
    {
        /* Advise the outside world, fields has been repeated.  */
        STEVT_Notify(VIDDISP_Data_p->EventsHandle,
                     VIDDISP_Data_p->RegisteredEventsID
                                        [VIDDISP_FIELDS_REPEATED_EVT_ID],
                     STEVT_EVENT_DATA_TYPE_CAST (NumberDone_p));
    }

    return(ST_NO_ERROR);
} /* End of VIDDISP_RepeatFields() function */


/*******************************************************************************
Name        : VIDDISP_DisplayBitmap
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDDISP_DisplayBitmap(const VIDDISP_Handle_t DisplayHandle,
                                    BOOL MainNotAux,
                                    STGXOBJ_Bitmap_t * BitmapParams_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDDISP_ShowParams_t ShowParams;
    U32     LayerIdent;

    /* Check Input parameter. */
    if (BitmapParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (MainNotAux)
    {
        LayerIdent = 0; /* main layer */
    }
    else
    {
        LayerIdent = 1; /* aux layer (dual display) */
    }

    /* Call the HAL to update the hardware selected display */
    /* Move display pointer */
    HALDISP_SetPresentationLumaFrameBuffer(
        ((VIDDISP_Data_t *) DisplayHandle)->HALDisplayHandle[GetHALIndex(DisplayHandle, LayerIdent)],
        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
        Device(BitmapParams_p->Data1_p));
    HALDISP_SetPresentationChromaFrameBuffer(
        ((VIDDISP_Data_t *) DisplayHandle)->HALDisplayHandle[GetHALIndex(DisplayHandle, LayerIdent)],
        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
        Device(BitmapParams_p->Data2_p));

    /* Common HAL writing. */
    /* Set picture properties summarized in ShowParams data.  */
    ShowParams.HDecimation  = STVID_DECIMATION_FACTOR_NONE;
    ShowParams.VDecimation  = STVID_DECIMATION_FACTOR_NONE;
    ShowParams.Compression  = STVID_COMPRESSION_LEVEL_NONE;
    ShowParams.ScanType     = STGXOBJ_PROGRESSIVE_SCAN;
    ShowParams.Height       = BitmapParams_p->Height;
    ShowParams.Width        = BitmapParams_p->Width;
    ShowParams.BitmapType   = BitmapParams_p->BitmapType;
    HALDISP_SetPresentationStoredPictureProperties(
        ((VIDDISP_Data_t *) DisplayHandle)->HALDisplayHandle[GetHALIndex(DisplayHandle, LayerIdent)],
        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
        &ShowParams);
    /* Set display parameters depending on picture */
    HALDISP_SetPresentationFrameDimensions(
        ((VIDDISP_Data_t *) DisplayHandle)->HALDisplayHandle[GetHALIndex(DisplayHandle, LayerIdent)],
        ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[LayerIdent].LayerType,
        BitmapParams_p->Width,
        BitmapParams_p->Height);

    return(ST_NO_ERROR);
} /* End of VIDDISP_DisplayBitmap() function */


/*******************************************************************************
Name        : VIDDISP_ShowPicture
Description : Show specified picture on display instead of current display
Parameters  : Display handle, picture parameters
Assumptions : Picture_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDISP_ShowPicture(const VIDDISP_Handle_t DisplayHandle,
            VIDBUFF_PictureBuffer_t  * const Picture_p,
            const U32 Layer,
            VIDDISP_ShowPicture_t ShowPict)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    Error = VIDDISP_ShowPictureLayer(DisplayHandle,Picture_p,Layer, ShowPict);

    return(Error);
} /* end VIDDISP_ShowPicture */

/******************************************************************************/
ST_ErrorCode_t VIDDISP_ShowPictureLayer(const VIDDISP_Handle_t DisplayHandle,
                            VIDBUFF_PictureBuffer_t * const Picture_p,
                            U32 LayerIdent,
                            VIDDISP_ShowPicture_t ShowPict)
{
    VIDDISP_Data_t *            VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDDISP_ShowParams_t        ShowParams;
    STVID_PictureInfos_t *      CurrentPictureInfos_p;
    void *                      BitmapLumaData_p;
    HALDISP_Handle_t            HALDISP_Handle;

    /* Check Input parameter. */
    if (Picture_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get HAL Handle */
    HALDISP_Handle = VIDDISP_Data_p->HALDisplayHandle[GetHALIndex(DisplayHandle, LayerIdent)];


    if (VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType != STLAYER_GAMMA_GDP)
    {
        /* Special case for 7109 MAIN or 7200 */
        if ( DEVICE_TYPE_7109 ||
             DEVICE_TYPE_7100 || DEVICE_TYPE_7200 )
        {
            if ( (ShowPict == VIDDISP_SHOW_VIDEO) ||
                 (ShowPict == VIDDISP_RESTORE_ORIGINAL_PICTURE) )
            {
                /* Send a Null pointer to the Hall Layer to indicate that we don't want to show a picture */
                HALDISP_SetPresentationLumaFrameBuffer(
                        HALDISP_Handle,
                        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                        NULL);
                HALDISP_SetPresentationChromaFrameBuffer(
                        HALDISP_Handle,
                        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                        NULL);
                /* Leave the function now */
                return(ST_NO_ERROR);
            }
        }
    }

    /* Shortcut */
    CurrentPictureInfos_p = &(Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos);

    /* Common parameters. */
    ShowParams.Width     = CurrentPictureInfos_p->BitmapParams.Width;
    ShowParams.Height    = CurrentPictureInfos_p->BitmapParams.Height;
    ShowParams.ScanType  = CurrentPictureInfos_p->VideoParams.ScanType;
    ShowParams.BitmapType= CurrentPictureInfos_p->BitmapParams.BitmapType;

    /* Is there a Layer with a type opened? */
    if (VIDDISP_Data_p->LayerDisplay[LayerIdent].IsOpened)
    {
        /*  is opened. Update the ShowParams for Main according to */
        /* the reconstruction mode. */
        if (Picture_p->FrameBuffer_p->AvailableReconstructionMode
                            == VIDBUFF_SECONDARY_RECONSTRUCTION)
        {
            /* Report the picture's decimation (filter by kind of decimation */
            /* vertical or horizontal.                                */
            ShowParams.HDecimation
                = CurrentPictureInfos_p->VideoParams.DecimationFactors
                    &~(STVID_DECIMATION_FACTOR_V2|STVID_DECIMATION_FACTOR_V4);

            ShowParams.VDecimation
                = CurrentPictureInfos_p->VideoParams.DecimationFactors
                    &~(STVID_DECIMATION_FACTOR_H2|STVID_DECIMATION_FACTOR_H4);

            /* No compression to manage.        */
            ShowParams.Compression = STVID_COMPRESSION_LEVEL_NONE;
        } /* end secondary reconstruction */
        else /* Main reconstruction  */
        {
            /* Init other ShowParams without any decimation (not supported). */
            /* No decimation.               */
            ShowParams.HDecimation = STVID_DECIMATION_FACTOR_NONE;
            ShowParams.VDecimation = STVID_DECIMATION_FACTOR_NONE;

            /* Just repport the picture compression level.  */
            ShowParams.Compression = Picture_p->FrameBuffer_p->CompressionLevel;
        } /* End primary reconstruction */
        ShowParams.Data_p      = Picture_p->FrameBuffer_p->Allocation.Address_p;
        ShowParams.ChromaData_p= Picture_p->FrameBuffer_p->Allocation.Address2_p;
        ShowParams.BitmapType  = CurrentPictureInfos_p->BitmapParams.BitmapType;
        BitmapLumaData_p       = CurrentPictureInfos_p->BitmapParams.Data1_p;

        /* Call the HAL to update the hardware for Main. */
        /* Move display pointer */
        if ( (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)
           &&(LayerIdent != VIDDISP_Data_p->DisplayRef) )
        {
            /* Nothing to do....    */
        }
        else
        {
            if((ShowParams.BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM) ||
               (ShowParams.BitmapType == STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE))
            {   /* Only used when Raster format is used: update from STVIN driver */
                HALDISP_SetPresentationLumaFrameBuffer(
                        HALDISP_Handle,
                        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                        BitmapLumaData_p);
            }
            else
            {
                HALDISP_SetPresentationLumaFrameBuffer(
                        HALDISP_Handle,
                        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                        ShowParams.Data_p);
                HALDISP_SetPresentationChromaFrameBuffer(
                        HALDISP_Handle,
                        VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                        ShowParams.ChromaData_p);
           }
            /* Set picture properties summarized in ShowParams data.  */
            HALDISP_SetPresentationStoredPictureProperties(
                    HALDISP_Handle,
                    VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                    &ShowParams);
        }
    } /* End Opened */

    /* Common HAL writing. */
    /* Set display parameters depending on picture */
    switch (ShowParams.BitmapType)
    {
        case STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM:
        case STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE:
            HALDISP_SetPresentationFrameDimensions(
                HALDISP_Handle,
                VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                CurrentPictureInfos_p->BitmapParams.Pitch,
                ShowParams.Height);
            break;

        case STGXOBJ_BITMAP_TYPE_MB:
        case STGXOBJ_BITMAP_TYPE_MB_HDPIP:
        case STGXOBJ_BITMAP_TYPE_MB_TOP_BOTTOM:
        default: /* should not come here */
            HALDISP_SetPresentationFrameDimensions(
                HALDISP_Handle,
                VIDDISP_Data_p->LayerDisplay[LayerIdent].LayerType,
                ShowParams.Width,
                ShowParams.Height);
            break;
    }
    return(ST_NO_ERROR);
} /* End of VIDDISP_ShowPictureLayer() function */

/*******************************************************************************
Name        : VIDDISP_Skip
Description : Skip...
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDISP_Skip(const VIDDISP_Handle_t DisplayHandle)
{
    UNUSED_PARAMETER(DisplayHandle);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
} /* End of VIDDISP_Skip() function */


/*******************************************************************************
Name        : ResetValuesOfChangeEVTs
Description : Reset values that trigger the "change EVT's"
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ResetValuesOfChangeEVTs(const VIDDISP_Handle_t DisplayHandle)
{
    /* Put impossible values "change EVT's" are notified next time */
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.AspectRatio
                                    = (STVID_DisplayAspectRatio_t) (-1);
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.ColorSpaceConversion
                                    = (STGXOBJ_ColorSpaceConversionMode_t) (-1);
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.PictureFrameRate = 0;
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.ScanType    = (STGXOBJ_ScanType_t) (-1);
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.Height      = 0;
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.Width       = 0;
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.PanAndScanIn16thPixel.FrameCentreHorizontalOffset = SHRT_MAX;
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.PanAndScanIn16thPixel.FrameCentreHorizontalOffset = SHRT_MAX;
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.Reconstruction = VIDBUFF_NONE_RECONSTRUCTION;
#ifdef ST_crc
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.CRCCheckMode = FALSE;
    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract.CRCScanType = (STGXOBJ_ScanType_t) (-1);
#endif  /* ST_crc */

    ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[1].DisplayCaract = ((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[0].DisplayCaract;

} /* End of ResetValuesOfChangeEVTs() function */

/* end of vid_cmd.c */
