/*******************************************************************************

File name   : vid_sets.c

Description : Video display source file. Display API functions (Set/Get).

    VIDDISP_GetReconstructionMode
    VIDDISP_SetReconstructionMode
    VIDDISP_SetDisplayParams
    VIDDISP_SetDiscontinuityInMPEGDisplay
    VIDDISP_GetDisplayedPictureInfos
    VIDDISP_ConfigureEvent
    VIDDISP_LayerConnection
    VIDDISP_NewVTG
    VIDDISP_GetDisplayedPictureParams
    VIDDISP_GetStatistics
    VIDDISP_ResetStatistics
    VIDDISP_SetSlaveDisplayUpdate
    VIDDISP_UpdateLayerParams

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
 02 Sep 2002        Created                                   Michel Bruant
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#include <assert.h>
#endif

#include "stddefs.h"
#include "stvtg.h"
#include "stvid.h"
#include "display.h"
#include "vid_disp.h"
#include "vid_ctcm.h"
#include "queue.h"

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART */

/* Select the Traces you want */
#define TrViewport               TraceOff
#define TrReconstruction         TraceOn

#define TrWarning                TraceOn
/* TrError is now defined in vid_trace.h */

/* "vid_trace.h" should be included AFTER the definition of the traces wanted */
#include "vid_trace.h"

/* ---------------------------------------------- */

#ifdef TRACE_SETTING /* define or uncomment in ./makefile to set traces */
  #include "vid_trc.h"
#else
  #define TraceInfo(x)
  #define TraceError(x)
#endif

/* Private Constants -------------------------------------------------------- */

#if !defined(STVID_MINIMIZE_MEMORY_USAGE_FOR_DEI)
/* Some extra frame buffers are necessary because there is a need for buffers to be locked by the
   Deinterlacer while they are used as reference. */
    #define     DISPLAYPIPE_EXTRA_FRAME_BUFFERS_NBR     2
#else
    #define     DISPLAYPIPE_EXTRA_FRAME_BUFFERS_NBR     0
#endif /* STVID_MINIMIZE_MEMORY_USAGE_FOR_DEI */

/* Local Functions Constants -------------------------------------------------------- */

static ST_ErrorCode_t viddisp_ReserveFrameBuffersForReference(const VIDDISP_Handle_t DisplayHandle, const U32 LayerIdent);
static ST_ErrorCode_t viddisp_FreeFrameBuffersReservedForReference(const VIDDISP_Handle_t DisplayHandle, const U32 LayerIdent);


/*******************************************************************************
Name        : VIDDISP_GetStatus
Description : returns the display status
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDDISP_GetState(const VIDDISP_Handle_t DisplayHandle,
                       VIDDISP_DisplayState_t* DisplayState)
{
    VIDDISP_Data_t * VIDDISP_Data_p;

    /* Find structure */
    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    *DisplayState = VIDDISP_Data_p->DisplayState;
} /* End of VIDDISP_GetState() function */

/*******************************************************************************
Name        : VIDDISP_GetReconstructionMode
Description : Get the reconstruction Mode of a layer
Parameters  : Display handle, pointer to reconstruction mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDISP_GetReconstructionMode(
                      const VIDDISP_Handle_t DisplayHandle,
                      const STLAYER_Layer_t LayerType,
             VIDBUFF_AvailableReconstructionMode_t * const ReconstructionMode_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (ReconstructionMode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* In the case if display on auxiliary and secondary (specific 7200) */
    if (( VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
          VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
          (VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
          VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ) )
    {
        if ( VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
            VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 )
        {
            if (LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
            {
                TraceInfo(("Get Recons aux\r\n"));
                *ReconstructionMode_p = VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode;
            }
            else if (LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
            {
                TraceInfo(("Get Recons secondary\r\n"));
                *ReconstructionMode_p = VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode;
            }
            else
                return(ST_ERROR_BAD_PARAMETER);
        } else
        {
            if (LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
            {
                TraceInfo(("Get Recons aux\r\n"));
                *ReconstructionMode_p = VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode;
            }
            else if (LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
            {
                TraceInfo(("Get Recons secondary\r\n"));
                *ReconstructionMode_p = VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode;
            }
            else
                return(ST_ERROR_BAD_PARAMETER);
        }
    } else
    {
        /* aux display / dual display */
        if ( (LayerType == STLAYER_OMEGA2_VIDEO2)
          || (LayerType == STLAYER_7020_VIDEO2)
          || (LayerType == STLAYER_SDDISPO2_VIDEO2)
          || (LayerType == STLAYER_HDDISPO2_VIDEO2)
          || (LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
          || (LayerType == STLAYER_DISPLAYPIPE_VIDEO4))
        {
            TraceInfo(("Get Recons aux\r\n"));
            *ReconstructionMode_p =
                VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode;
        }
        /* main display / single display */
        else if ((LayerType == STLAYER_OMEGA2_VIDEO1)
              || (LayerType == STLAYER_OMEGA1_VIDEO)
              || (LayerType == STLAYER_7020_VIDEO1)
              || (LayerType == STLAYER_COMPOSITOR)
              || (LayerType == STLAYER_SDDISPO2_VIDEO1)
              || (LayerType == STLAYER_HDDISPO2_VIDEO1)
              || (LayerType == STLAYER_DISPLAYPIPE_VIDEO1)
              || (LayerType == STLAYER_DISPLAYPIPE_VIDEO2)
              || (LayerType == STLAYER_GAMMA_GDP)
              )
        {
            TraceInfo(("Get Recons main\r\n"));
            *ReconstructionMode_p
                = VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode;
        }
        else
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
    }
    return(ST_NO_ERROR);
} /* End of VIDDISP_GetReconstructionMode() function */

/*******************************************************************************
Name        : VIDDISP_SetReconstructionMode
Description : Set the way the display will reconstruct the frame buffers.
Parameters  : Display handle, Layer type, Reconstruction mode.
Assumptions : ReconstructionMode is supported by the chip.
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if any bad parameter
              ST_ERROR_ALREADY_INITIALIZED if decimation already selected.
*******************************************************************************/
ST_ErrorCode_t VIDDISP_SetReconstructionMode(
        const VIDDISP_Handle_t DisplayHandle,
        const STLAYER_Layer_t LayerType,
        const VIDBUFF_AvailableReconstructionMode_t ReconstructionMode)
{
    VIDDISP_Data_t            * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    U32                         Layer = 0;
    VIDDISP_Picture_t         * CurrentPict_p;
    VIDCOM_InternalProfile_t    MemoryProfile;
    VIDBUFF_PictureBuffer_t *   CurrentPictureBuff_p = NULL;

    TrReconstruction(("\r\n Enter VIDDISP_SetReconstructionMode"));

    /* Check the input parameter */
    if ((ReconstructionMode > VIDBUFF_SECONDARY_RECONSTRUCTION)
            || ((LayerType != STLAYER_OMEGA1_VIDEO)
            && (LayerType != STLAYER_OMEGA2_VIDEO1)
            && (LayerType != STLAYER_OMEGA2_VIDEO2)
            && (LayerType != STLAYER_7020_VIDEO1)
            && (LayerType != STLAYER_7020_VIDEO2)
            && (LayerType != STLAYER_COMPOSITOR)
            && (LayerType != STLAYER_SDDISPO2_VIDEO1)
            && (LayerType != STLAYER_SDDISPO2_VIDEO2)
            && (LayerType != STLAYER_HDDISPO2_VIDEO1)
            && (LayerType != STLAYER_HDDISPO2_VIDEO2)
            && (LayerType != STLAYER_DISPLAYPIPE_VIDEO1)
            && (LayerType != STLAYER_DISPLAYPIPE_VIDEO2)
            && (LayerType != STLAYER_DISPLAYPIPE_VIDEO3)
            && (LayerType != STLAYER_DISPLAYPIPE_VIDEO4)
            && (LayerType != STLAYER_GAMMA_GDP)
            ))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* In the case of display on auxiliary and secondary (specific 7200) */
    if (( VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
          VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
          (VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
          VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ) )
    {
        if (( VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
          VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ))
        {
            if (LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                Layer = 0;
            else if (LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                Layer = 1;
        }
        else
        {
            if (LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
                Layer = 1;
            else if (LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
                Layer = 0;
        }
    }
    else
    {
        /* main display / single display */
        if ((LayerType == STLAYER_OMEGA1_VIDEO)
         || (LayerType == STLAYER_OMEGA2_VIDEO1)
         || (LayerType == STLAYER_7020_VIDEO1)
         || (LayerType == STLAYER_COMPOSITOR)
         || (LayerType == STLAYER_SDDISPO2_VIDEO1)
         || (LayerType == STLAYER_HDDISPO2_VIDEO1)
         || (LayerType == STLAYER_DISPLAYPIPE_VIDEO1)
         || (LayerType == STLAYER_DISPLAYPIPE_VIDEO2)
         || (LayerType == STLAYER_GAMMA_GDP)
         )
        {
            Layer = 0;
            TraceInfo(("Set Recons main %i",ReconstructionMode));
        }
        else if ((LayerType == STLAYER_OMEGA2_VIDEO2)
              || (LayerType == STLAYER_7020_VIDEO2)
              || (LayerType == STLAYER_SDDISPO2_VIDEO2)
              || (LayerType == STLAYER_HDDISPO2_VIDEO2)
              || (LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
              || (LayerType == STLAYER_DISPLAYPIPE_VIDEO4))
        {
            Layer = 1;
            TraceInfo(("Set Recons aux %i",ReconstructionMode));
        }
    }

    TrReconstruction(("\r\n Layer %d, request to set reconstruction to %d", Layer, ReconstructionMode));

    VIDBUFF_GetMemoryProfile(VIDDISP_Data_p->BufferManagerHandle, &MemoryProfile);

#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
        /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
         * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum)
         * Don't exit in case of aux display already set in decimated ReconstructionMode */
    if ((MemoryProfile.ApplicableDecimationFactor == STVID_DECIMATION_FACTOR_NONE) &&
        (ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION) &&
        (ReconstructionMode == VIDDISP_Data_p->LayerDisplay[Layer].ReconstructionMode)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG4P2)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG4P2)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_AVS)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7200_MPEG4P2))
#else /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
    if ((ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION) &&
        (ReconstructionMode == VIDDISP_Data_p->LayerDisplay[Layer].ReconstructionMode)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG4P2)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG4P2)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_AVS)&&
        (VIDDISP_Data_p->DeviceType != STVID_DEVICE_TYPE_7200_MPEG4P2))
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
    {
        /* For this Layer we have already had a request to go in SECONDARY.  */
        /* If it is asked an other time it means that the decimation was not */
        /* enough. */
        TrReconstruction(("...was already decimated recons\r\n"));
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* Reconstruction changes: */
    /*-------------------------*/
#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
        /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
         * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum)
         * Force reconstruction chan even if aux display is already set in decimated ReconstructionMode */
    if (((LayerType == STLAYER_HDDISPO2_VIDEO2) &&
         (MemoryProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE) &&
         (ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)) ||
        (VIDDISP_Data_p->LayerDisplay[Layer].ReconstructionMode != ReconstructionMode) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
#else /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
    if ((VIDDISP_Data_p->LayerDisplay[Layer].ReconstructionMode != ReconstructionMode) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
        (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
    {

        STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);
        /* Set the Reconstruction Mode parameter for the layer.*/
        VIDDISP_Data_p->LayerDisplay[Layer].ReconstructionMode = ReconstructionMode;
        TrReconstruction(("\r\n Layer %d, Reconstruction set to %d", Layer, ReconstructionMode));

#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
        if (((LayerType == STLAYER_HDDISPO2_VIDEO2) &&
            (MemoryProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE))||
            (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)||
            (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)||
            (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)||
            (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
#else /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
        if ((VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)||
            (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)||
            (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)||
            (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
        {
            VIDDISP_Data_p->LayerDisplay[Layer].DisplayCaract.Reconstruction = ReconstructionMode;
            CurrentPict_p = VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p;

            if (CurrentPict_p != NULL)
            {
                /* Select Main or Decimated picture buffer */
                CurrentPictureBuff_p = GetAppropriatePictureBufferForLayer(DisplayHandle, CurrentPict_p, Layer);
            }
            if ((CurrentPict_p != NULL) && (CurrentPictureBuff_p != NULL))
            {
                VIDDISP_ShowPictureLayer(DisplayHandle, CurrentPictureBuff_p, Layer, VIDDISP_SHOW_VIDEO);
                VIDDISP_Data_p->LayerDisplay[Layer].DisplayCaract.DecimationFactor =
                        CurrentPict_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;
            }
            else
            {
                VIDDISP_Data_p->LayerDisplay[Layer].DisplayCaract.DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
            }

            STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

            return(ST_NO_ERROR);
        }

        /* Try to exchange the current picture */
        CurrentPict_p = VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p;
        if (CurrentPict_p == NULL)
        {
            CurrentPict_p = VIDDISP_Data_p->LayerDisplay[Layer].CurrentlyDisplayedPicture_p;
        }

        if (CurrentPict_p != NULL)
        {
            /* Select Main or Decimated picture buffer */
            CurrentPictureBuff_p = GetAppropriatePictureBufferForLayer(DisplayHandle, CurrentPict_p, Layer);
        }

        if ( (CurrentPict_p != NULL) && (CurrentPictureBuff_p != NULL) )
        {
            /* OK, we can update the display */
            /* set the new picture (good decimated) */
            VIDDISP_Data_p->LayerDisplay[Layer].PicturePreparedForNextVSync_p
                            = CurrentPict_p;
            VIDBUFF_SetDisplayStatus(VIDDISP_Data_p->BufferManagerHandle,
                              VIDBUFF_ON_DISPLAY,
                              CurrentPictureBuff_p );
            VIDDISP_ShowPictureLayer(DisplayHandle,
                              CurrentPictureBuff_p,Layer, VIDDISP_SHOW_VIDEO);
            /* ... and reset the previous picture */
            CurrentPict_p->Counters[Layer].NextFieldOnDisplay
                            = VIDDISP_FIELD_DISPLAY_NONE;
            /* No need to Re-lock the picture buffer since the Picture remains protected while it is in the Display Queue */
            TraceInfo(("-> switch queue"));
        }
        else
        {
            /* We can't update the display with a good picture */
            TraceInfo(("-> Can't switch !!"));
        }

        /* update params now to avoid a change caracteristics detection */
        VIDDISP_Data_p->LayerDisplay[Layer].DisplayCaract.Reconstruction = ReconstructionMode;

        if (ReconstructionMode != VIDBUFF_SECONDARY_RECONSTRUCTION)
        {
            /* Main reconstruction */
            /* Here ReconstructionMode should be VIDBUFF_MAIN_RECONSTRUCTION, but go on without
            checking so that if there is a problem corrupting the value of the variable, display goes on on main queue at least. */
            VIDDISP_Data_p->LayerDisplay[Layer].DisplayCaract.DecimationFactor
                        = STVID_DECIMATION_FACTOR_NONE;
        }
        else
        {
            /* Secondary reconstruction */
            VIDDISP_Data_p->LayerDisplay[Layer].DisplayCaract.DecimationFactor
                        = MemoryProfile.ApplicableDecimationFactor;
        }

         STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
    }
    else
    {
        TrReconstruction(("\r\n Error Layer %d, Reconstruction not set to %d !", Layer, ReconstructionMode));
    }

    TraceInfo(("\r\n"));
    return(ST_NO_ERROR);
} /* End of VIDDISP_SetReconstructionMode() function */

/*******************************************************************************
Name        : VIDDISP_SetDisplayParams
Description : Set paramaters of display
Parameters  : Display handle, params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDISP_SetDisplayParams(const VIDDISP_Handle_t DisplayHandle,
                                 const VIDDISP_DisplayParams_t * const Params_p)
{
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    TraceInfo(("Set Display Params\r\n"));

    ((VIDDISP_Data_t *) DisplayHandle)->DisplayForwardNotBackward
                                    = Params_p->DisplayForwardNotBackward;
    /* memorize Polarity which has been required by the application */
    ((VIDDISP_Data_t *) DisplayHandle)->DisplayWithCareOfFieldsPolarity
                                    = Params_p->DisplayWithCareOfFieldsPolarity;
    return(ST_NO_ERROR);
} /* End of VIDDISP_SetDisplayParams() function */

/*******************************************************************************
Name        : VIDDISP_SetDiscontinuityInMPEGDisplay
Description : Informs the display that a discontinuity is occuring in the
                MPEG series given to it.
Parameters  : Display handle,
Assumptions : Should be called when a reference picture is decoded but not
                passed to display, to inform the display that it should
                unlock display if it was locked because of DisplayStart
                == VIDDISP_DISPLAY_START_CARE_MPEG_FRAME.
              Indeed, when such a case arrives, there's no need to wait now for
              a coming B picture, display should go on to the next picture after
              the discontinuity. If not, this may run to definitive lock of
              display queue !
              Example of critical definitive lock:
           * one picture is on display, last field on display (current display)
             No other picture is in display queue, so display queue is locked
           * one (in 3 buffers) or 2 (in 4 buffers) can then be reserved but not
             passed to display for various reasons (trickmod control,
             error recovery)
           * one picture can then be inserted in display queue.
               * But without calling this function, unless in DisplayStart "as
               available", the display queue would not be unlocked !
               Then the buffer manager cannot find a free buffer and the video
               driver is crashed !
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDISP_SetDiscontinuityInMPEGDisplay(
                                        const VIDDISP_Handle_t DisplayHandle)
{
    /* Remember MPEG display discontinuity */
    ((VIDDISP_Data_t *) DisplayHandle)->HasDiscontinuityInMPEGDisplay = TRUE;

    return(ST_NO_ERROR);
} /* End of VIDDISP_SetDiscontinuityInMPEGDisplay() function */


/*******************************************************************************
Name        : VIDDISP_GetDisplayedPictureInfos
Description : Gives params of picture currently on display
Parameters  : Display handle, pointer to params to return
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              STVID_ERROR_NOT_AVAILABLE if no picture on display
              ST_ERROR_BAD_PARAMETER if NULL pointer and info available
*******************************************************************************/
ST_ErrorCode_t VIDDISP_GetDisplayedPictureInfos(
                                const VIDDISP_Handle_t DisplayHandle,
                                STVID_PictureInfos_t * const PictureInfos_p)
{
    VIDDISP_Picture_t * CurrentPicture_p;
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (PictureInfos_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Consider output of display Queue */
    CurrentPicture_p = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].
            PicturePreparedForNextVSync_p;

    if (CurrentPicture_p == NULL)
    {
        CurrentPicture_p = VIDDISP_Data_p->LayerDisplay
            [VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p;
    }

    if ((CurrentPicture_p == NULL) || (CurrentPicture_p->PictureInformation_p == NULL))
    {
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    /* OK, fill the infos*/
    *PictureInfos_p= CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos;

    return(ST_NO_ERROR);
} /* End of VIDDISP_GetDisplayedPictureInfos() function */

/*******************************************************************************
Name        : VIDDISP_GetDisplayedPictureExtendedInfos
Description : Gives exetended params of picture currently on display
Parameters  : Display handle, pointer to params to return
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              STVID_ERROR_NOT_AVAILABLE if no picture on display
              ST_ERROR_BAD_PARAMETER if NULL pointer and info available
*******************************************************************************/
ST_ErrorCode_t VIDDISP_GetDisplayedPictureExtendedInfos(
                                const VIDDISP_Handle_t DisplayHandle,
                                VIDBUFF_PictureBuffer_t * const PictureExtendedInfos_p)
{
    VIDDISP_Picture_t * CurrentPicture_p;
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (PictureExtendedInfos_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Consider output of display Queue */
    CurrentPicture_p = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].
            PicturePreparedForNextVSync_p;

    if (CurrentPicture_p == NULL)
    {
        CurrentPicture_p = VIDDISP_Data_p->LayerDisplay
            [VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p;
    }
    if (CurrentPicture_p == NULL)
    {
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    /* OK, fill the infos*/
    STOS_memcpy(PictureExtendedInfos_p, CurrentPicture_p->PictureInformation_p, sizeof(VIDBUFF_PictureBuffer_t) );

    return(ST_NO_ERROR);

} /* End of VIDDISP_GetDisplayedPictureExtendedInfos() function */

/*******************************************************************************
Name        : VIDDISP_ConfigureEvent
Description : Configures notification of display's events.
Parameters  : Video Handle
              Event to allow or not
              Param_p parameters of notification.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              STVID_ERROR_NOT_AVAILABLE if no picture on display
              ST_ERROR_BAD_PARAMETER if NULL pointer and info available
*******************************************************************************/
ST_ErrorCode_t VIDDISP_ConfigureEvent(const VIDDISP_Handle_t DisplayHandle,
                        const STEVT_EventID_t Event,
                        const STVID_ConfigureEventParams_t * const Params_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    U32 EventID = 0;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    switch (Event)
    {
        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
            EventID = VIDDISP_NEW_PICTURE_TO_BE_DISPLAYED_EVT_ID;
            break;
        case STVID_DISPLAY_NEW_FRAME_EVT:
            EventID = VIDDISP_DISPLAY_NEW_FRAME_EVT_ID;
            break;
        case STVID_NEW_FIELD_TO_BE_DISPLAYED_EVT:
            EventID = VIDDISP_NEW_FIELD_TO_BE_DISPLAYED_EVT_ID;
            break;

        default :
            /* Error case. This event can't be configured. */
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    if (ErrCode == ST_NO_ERROR)
    {
        VIDDISP_Data_p->EventNotificationConfiguration[EventID].
                                        NotificationSkipped = 0;
        VIDDISP_Data_p->EventNotificationConfiguration[EventID].
                                        ConfigureEventParam = (*Params_p);
    }

    /* Return the configuration's result. */
    return(ErrCode);

} /* End of VIDDISP_ConfigureEvent() function. */

/*******************************************************************************
Name        : VIDDISP_LayerConnection
Description : Informs the display which Layer type is connected.
Parameters  : Display handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDISP_LayerConnection(const VIDDISP_Handle_t DisplayHandle,
                                        const VIDDISP_LayerConnectionParams_t* LayerConnectionParams_p,
                                        const BOOL IsOpened)
{
    VIDDISP_Data_t *                        VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    VIDCOM_InternalProfile_t                CommonMemoryProfile;
    VIDBUFF_AvailableReconstructionMode_t   Reconstruction;
    U32                                     LayerIdent;

    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);

    /* Get current memory profile.                                          */
    VIDBUFF_GetMemoryProfile(VIDDISP_Data_p->BufferManagerHandle,
            &CommonMemoryProfile);

    /* Initialize default display reconstruction mode.                      */
    Reconstruction = VIDBUFF_MAIN_RECONSTRUCTION;

    if ( (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) ||
         (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT))
    {
        /* Set the reconstruction mode, assuming the default one is */
        /* VIDBUFF_MAIN_RECONSTRUCTION !!!                          */
        if (CommonMemoryProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE)
        {
            Reconstruction = VIDBUFF_SECONDARY_RECONSTRUCTION;
        }
    }

    /* Aux display + Sec display / dual display */
    if (( VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 &&
          VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 ) ||
          (VIDDISP_Data_p->LayerDisplay[0].LayerType == STLAYER_DISPLAYPIPE_VIDEO4 &&
          VIDDISP_Data_p->LayerDisplay[1].LayerType == STLAYER_DISPLAYPIPE_VIDEO3 ) )
    {
        /* In this case the secondary is arbitrary selected as the main display */
        if (LayerConnectionParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4)
        {
            LayerIdent = 0;
            VIDDISP_Data_p->LayerDisplay[0].IsOpened            = IsOpened;
            VIDDISP_Data_p->LayerDisplay[0].LayerType           = LayerConnectionParams_p->LayerType;
            if (IsOpened)
            {
                VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode = Reconstruction;

                /* Disable the Layer toggle in manual mode. */
                /* No need to use the GetHALIndex macro to find the HALDisplayHandle index here */
                /* as the result will always be zero. */
                HALDISP_DisablePresentationChromaFieldToggle(VIDDISP_Data_p->
                        HALDisplayHandle[GetHALIndex(DisplayHandle, 0)], LayerConnectionParams_p->LayerType);
                HALDISP_DisablePresentationLumaFieldToggle(VIDDISP_Data_p->
                        HALDisplayHandle[GetHALIndex(DisplayHandle, 0)], LayerConnectionParams_p->LayerType);
                VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p
                    = VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p;
            }
            else
            {
                VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode = VIDBUFF_NONE_RECONSTRUCTION;
                /* Remember the last displayed picture */
                if (VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p)
                {
                    VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p
                    = VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p;
                }
                if (VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p)
                {
                    VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p
                    = VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p;
                }
                VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p  = NULL;
                VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p= NULL;

                /* Ensure that no picture has a lock counter on this Layer */
                viddisp_ResetCountersInQueue(LayerIdent, DisplayHandle);
            }
        } else
        {
            LayerIdent = 1;
            VIDDISP_Data_p->LayerDisplay[1].IsOpened            = IsOpened;
            VIDDISP_Data_p->LayerDisplay[1].LayerType           = LayerConnectionParams_p->LayerType;
            if (IsOpened)
            {
                VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode = Reconstruction;

                /* Disable the Layer toggle in manual mode. */
                HALDISP_DisablePresentationChromaFieldToggle(VIDDISP_Data_p->
                        HALDisplayHandle[GetHALIndex(DisplayHandle, 1)], LayerConnectionParams_p->LayerType);
                HALDISP_DisablePresentationLumaFieldToggle(VIDDISP_Data_p->
                        HALDisplayHandle[GetHALIndex(DisplayHandle, 1)], LayerConnectionParams_p->LayerType);
                VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p
                    = VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p;
            }
            else
            {
                VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode = VIDBUFF_NONE_RECONSTRUCTION;
                /* Remember the last displayed picture */
                if (VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p)
                {
                    VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p
                    = VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p;
                }
                if (VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p)
                {
                    VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p
                    = VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p;
                }
                VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p= NULL;
                VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p  = NULL;

                /* Ensure that no picture has a lock counter on this Layer */
                viddisp_ResetCountersInQueue(LayerIdent, DisplayHandle);
            }
        }
    }
    /* Aux display / dual display */
    else if ( (LayerConnectionParams_p->LayerType == STLAYER_OMEGA2_VIDEO2)
      || (LayerConnectionParams_p->LayerType == STLAYER_7020_VIDEO2)
      || (LayerConnectionParams_p->LayerType == STLAYER_SDDISPO2_VIDEO2)
      || (LayerConnectionParams_p->LayerType == STLAYER_HDDISPO2_VIDEO2)
      || (LayerConnectionParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO3)
      || (LayerConnectionParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO4))
    {
        LayerIdent = 1;
        VIDDISP_Data_p->LayerDisplay[1].IsOpened            = IsOpened;
        VIDDISP_Data_p->LayerDisplay[1].LayerType           = LayerConnectionParams_p->LayerType;
        VIDDISP_Data_p->LayerDisplay[1].LayerHandle         = LayerConnectionParams_p->LayerHandle;

        if (IsOpened)
        {
            VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode = Reconstruction;

            /* Disable the Layer toggle in manual mode. */
            HALDISP_DisablePresentationChromaFieldToggle(VIDDISP_Data_p->
                    HALDisplayHandle[GetHALIndex(DisplayHandle, 1)], LayerConnectionParams_p->LayerType);
            HALDISP_DisablePresentationLumaFieldToggle(VIDDISP_Data_p->
                    HALDisplayHandle[GetHALIndex(DisplayHandle, 1)], LayerConnectionParams_p->LayerType);
            VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p
                = VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p;
        }
        else
        {
            VIDDISP_Data_p->LayerDisplay[1].ReconstructionMode = VIDBUFF_NONE_RECONSTRUCTION;
            /* Remember the last displayed picture */
            if (VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p)
            {
                VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p
                = VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p;
            }
            if (VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p)
            {
                VIDDISP_Data_p->LayerDisplay[1].PictDisplayedWhenDisconnect_p
                = VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p;
            }
            VIDDISP_Data_p->LayerDisplay[1].PicturePreparedForNextVSync_p= NULL;
            VIDDISP_Data_p->LayerDisplay[1].CurrentlyDisplayedPicture_p  = NULL;

            /* Ensure that no picture has a lock counter on this Layer */
            viddisp_ResetCountersInQueue(LayerIdent, DisplayHandle);
        }
    }
    /* main display / single display */
    else if ((LayerConnectionParams_p->LayerType == STLAYER_OMEGA2_VIDEO1)
          || (LayerConnectionParams_p->LayerType == STLAYER_OMEGA1_VIDEO)
          || (LayerConnectionParams_p->LayerType == STLAYER_7020_VIDEO1)
          || (LayerConnectionParams_p->LayerType == STLAYER_COMPOSITOR)
          || (LayerConnectionParams_p->LayerType == STLAYER_SDDISPO2_VIDEO1)
          || (LayerConnectionParams_p->LayerType == STLAYER_HDDISPO2_VIDEO1)
          || (LayerConnectionParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO1)
          || (LayerConnectionParams_p->LayerType == STLAYER_DISPLAYPIPE_VIDEO2)
          || ((LayerConnectionParams_p->LayerType == STLAYER_GAMMA_GDP ) && (VIDDISP_Data_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)) /* For temporary digital input on STi5528 */
          || (LayerConnectionParams_p->LayerType == STLAYER_GAMMA_GDP)
          )
    {
        LayerIdent = 0;
        VIDDISP_Data_p->LayerDisplay[0].IsOpened            = IsOpened;
        VIDDISP_Data_p->LayerDisplay[0].LayerType           = LayerConnectionParams_p->LayerType;
        VIDDISP_Data_p->LayerDisplay[0].LayerHandle         = LayerConnectionParams_p->LayerHandle;

        if (IsOpened)
        {
            VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode = Reconstruction;

            /* Disable the Layer toggle in manual mode. */
            /* No need to use the GetHALIndex macro to find the HALDisplayHandle index here */
            /* as the result will always be zero. */
            HALDISP_DisablePresentationChromaFieldToggle(VIDDISP_Data_p->
                    HALDisplayHandle[GetHALIndex(DisplayHandle, 0)], LayerConnectionParams_p->LayerType);
            HALDISP_DisablePresentationLumaFieldToggle(VIDDISP_Data_p->
                    HALDisplayHandle[GetHALIndex(DisplayHandle, 0)], LayerConnectionParams_p->LayerType);
            VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p
                = VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p;
        }
        else
        {
            VIDDISP_Data_p->LayerDisplay[0].ReconstructionMode = VIDBUFF_NONE_RECONSTRUCTION;
            /* Remember the last displayed picture */
            if (VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p)
            {
                VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p
                = VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p;
            }
            if (VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p)
            {
                VIDDISP_Data_p->LayerDisplay[0].PictDisplayedWhenDisconnect_p
                = VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p;
            }
            VIDDISP_Data_p->LayerDisplay[0].CurrentlyDisplayedPicture_p  = NULL;
            VIDDISP_Data_p->LayerDisplay[0].PicturePreparedForNextVSync_p= NULL;

            /* Ensure that no picture has a lock counter on this Layer */
            viddisp_ResetCountersInQueue(LayerIdent, DisplayHandle);
        }
    }
    else
    {
        STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set master/slave display */
    if (  (VIDDISP_Data_p->LayerDisplay[1].IsOpened) &&
        (!(VIDDISP_Data_p->LayerDisplay[0].IsOpened)))
    {
        VIDDISP_Data_p->DisplayRef    = 1;
    }
    else
    {
        VIDDISP_Data_p->DisplayRef    = 0;
    }

    /* Set framebuffer display latency and framebuffer hold time*/
    VIDDISP_Data_p->LayerDisplay[0].DisplayLatency                = LayerConnectionParams_p->DisplayLatency;
    VIDDISP_Data_p->LayerDisplay[1].DisplayLatency                = LayerConnectionParams_p->DisplayLatency;
    VIDDISP_Data_p->LayerDisplay[0].FrameBufferHoldTime           = LayerConnectionParams_p->FrameBufferHoldTime;
    VIDDISP_Data_p->LayerDisplay[1].FrameBufferHoldTime           = LayerConnectionParams_p->FrameBufferHoldTime;

    if (VIDDISP_Data_p->LayerDisplay[LayerIdent].IsOpened)
    {
        TrViewport(("\r\n Open Layer Connection %d", LayerIdent));
        /* Check if it is necessary to reserve some Frame Buffers used as reference */
        viddisp_ReserveFrameBuffersForReference(DisplayHandle, LayerIdent);
    }
    else
    {
        TrViewport(("\r\n Close Layer Connection %d", LayerIdent));
        /* Free the Frame Buffers which may have been reserved for reference */
        viddisp_FreeFrameBuffersReservedForReference(DisplayHandle, LayerIdent);
    }

    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
    return(ST_NO_ERROR);
} /* End of VIDDISP_LayerConnection() function */

/*******************************************************************************
Name        : VIDDISP_NewVTG
Description : There is a new VSync generator: change subscribtion to VSYNC event
              VTGName can be NULL: this just un-subscribes.
Parameters  : Display handle, new VTG name
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDISP_NewVTG(const VIDDISP_Handle_t DisplayHandle,
                              BOOL MainNotAux,
                              const STLAYER_VTGParams_t * const VTGParams_p)
{
    U32                              LayerIdent;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;
    ST_ErrorCode_t                  ErrorCode;
    STEVT_Handle_t                  UsedEvtHandle;
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);
    if (MainNotAux)
    {
        LayerIdent                            = 0; /* main layer */
        UsedEvtHandle                         = VIDDISP_Data_p->EventsHandle;
        STEVT_SubscribeParams.NotifyCallback  = viddisp_VSyncDisplayMain;

    }
    else
    {
        LayerIdent                            = 1; /* aux layer(dual display) */
        UsedEvtHandle                         = VIDDISP_Data_p->AuxEventsHandle;
        STEVT_SubscribeParams.NotifyCallback  = viddisp_VSyncDisplayAux;
    }
    if ((VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGName)[0] != '\0')
    {
        /* There was a VTG previously subscribed, un-subscribe it */
        STEVT_UnsubscribeDeviceEvent(UsedEvtHandle,
                               VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGName,
                               STVTG_VSYNC_EVT);
    }

    /* Subscribe to new VTG name if required, nothing to do otherwise. */
    if ((VTGParams_p->VTGName)[0] != '\0')
    {
        /* Remember a new VTG is connected. */
        VIDDISP_Data_p->IsVTGChangedSinceLastVSync  = TRUE;
        /* Subscribe to VSYNC EVT */
        STEVT_SubscribeParams.SubscriberData_p = (void *) (DisplayHandle);
        ErrorCode = STEVT_SubscribeDeviceEvent(UsedEvtHandle,
                                                VTGParams_p->VTGName,
                                                STVTG_VSYNC_EVT,
                                                &STEVT_SubscribeParams);
        if (ErrorCode == ST_NO_ERROR)
        {
            VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGVSyncFrequency
                                                = VTGParams_p->VTGFrameRate;
            VIDDISP_Data_p->LayerDisplay[LayerIdent].IsOpened = TRUE;
            /* Set master/slave display */
            if ( (VIDDISP_Data_p->LayerDisplay[1].IsOpened) &&
               (!(VIDDISP_Data_p->LayerDisplay[0].IsOpened)))
            {
                VIDDISP_Data_p->DisplayRef    = 1;
            }
            else
            {
                VIDDISP_Data_p->DisplayRef    = 0;
            }
        }
        else /* Error: subscribe failed */
        {
            TraceError(("Display cant subscribe to STVTG_VSYNC_EVT !\r\n"));
            /* Try to re-subscribe to old VSync, to avoid being blocked ? !!! */
            if ((VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGName)[0] != '\0')
            {
                ErrorCode = STEVT_SubscribeDeviceEvent(
                               UsedEvtHandle,
                               VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGName,
                               STVTG_VSYNC_EVT,
                               &STEVT_SubscribeParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    (VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGName)[0]= '\0';
                    /* No more VTG connected: no frame rate */
                    VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGVSyncFrequency = 0;
                }
            }
            VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentlyDisplayedPicture_p
                = VIDDISP_Data_p->LayerDisplay[LayerIdent].
                            PictDisplayedWhenDisconnect_p;
            STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
            return(ST_ERROR_BAD_PARAMETER);
        }
    } /* end subscribe to new VTG needed. */
    else
    {
        /* No more VTG connected: no frame rate */
        VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGVSyncFrequency   = 0;

        if(VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentlyDisplayedPicture_p)
        {
            VIDDISP_Data_p->LayerDisplay[LayerIdent].
                        PictDisplayedWhenDisconnect_p
            = VIDDISP_Data_p->LayerDisplay[LayerIdent].
                        CurrentlyDisplayedPicture_p;
        }
        if(VIDDISP_Data_p->LayerDisplay[LayerIdent].
                        PicturePreparedForNextVSync_p)
        {
            VIDDISP_Data_p->LayerDisplay[LayerIdent].
                        PictDisplayedWhenDisconnect_p
            = VIDDISP_Data_p->LayerDisplay[LayerIdent].
                        PicturePreparedForNextVSync_p;
        }
        VIDDISP_Data_p->LayerDisplay[LayerIdent].CurrentlyDisplayedPicture_p = NULL;
        VIDDISP_Data_p->LayerDisplay[LayerIdent].PicturePreparedForNextVSync_p = NULL;
        VIDDISP_Data_p->LayerDisplay[LayerIdent].IsOpened       = FALSE;
        /* Set master/slave display */
        if ( (VIDDISP_Data_p->LayerDisplay[0].IsOpened) &&
           (!(VIDDISP_Data_p->LayerDisplay[1].IsOpened)))
        {
            VIDDISP_Data_p->DisplayRef    = 0;
        }
        else if ((VIDDISP_Data_p->LayerDisplay[1].IsOpened) &&
               (!(VIDDISP_Data_p->LayerDisplay[0].IsOpened)))
        {
            VIDDISP_Data_p->DisplayRef    = 1;
        }
        /* ... other case : if both display are open : keep same master, */
        /* if both display are not open : it doesn't matter */
    }

    strcpy(VIDDISP_Data_p->LayerDisplay[LayerIdent].VTGName,
            VTGParams_p->VTGName);

    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);
    return(ST_NO_ERROR);
} /* End of VIDDISP_NewVTG() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDISP_GetDisplayedPictureParams
Description : Gives params of picture currntly on display
Parameters  : Display handle, pointer to params to return
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              STVID_ERROR_NOT_AVAILABLE if no picture on display
              ST_ERROR_BAD_PARAMETER if NULL pointer and info available
*******************************************************************************/
ST_ErrorCode_t VIDDISP_GetDisplayedPictureParams(
                                const VIDDISP_Handle_t DisplayHandle,
                                STVID_PictureParams_t * const PictureParams_p)
{
    VIDDISP_Picture_t * CurrentPicture_p;
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (PictureParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Consider output of display Queue */
    CurrentPicture_p = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].PicturePreparedForNextVSync_p;

    if (CurrentPicture_p == NULL)
    {
        CurrentPicture_p = VIDDISP_Data_p->LayerDisplay[VIDDISP_Data_p->DisplayRef].CurrentlyDisplayedPicture_p;
    }
    if (CurrentPicture_p == NULL)
    {
        /* If first picture in queue not displayed, nothing to update */
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    /* OK, fill the infos*/
    vidcom_PictureInfosToPictureParams(&(CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
             PictureParams_p, CurrentPicture_p->PictureInformation_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);

    return(ST_NO_ERROR);
} /* End of VIDDISP_GetDisplayedPictureParams() function */
#endif /*ST_smoothbackward */

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : VIDDISP_GetStatistics
Description : Get the statistics of the display module.
Parameters  : Video display handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER, if get failed.
*******************************************************************************/
ST_ErrorCode_t VIDDISP_GetStatistics(const VIDDISP_Handle_t DisplayHandle,
                                 STVID_Statistics_t * const Statistics_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Statistics_p->DisplayPictureInsertedInQueue
                    = VIDDISP_Data_p->StatisticsPictureInsertedInQueue;
    Statistics_p->DisplayPictureDisplayedByMain
                    = VIDDISP_Data_p->StatisticsPictureDisplayedByMain;
    Statistics_p->DisplayPictureDisplayedByAux
                    = VIDDISP_Data_p->StatisticsPictureDisplayedByAux;
    Statistics_p->DisplayPictureDisplayedBySec
                    = VIDDISP_Data_p->StatisticsPictureDisplayedBySec;
    Statistics_p->DisplayPictureDisplayedDecimatedByMain
                    = VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByMain;
    Statistics_p->DisplayPictureDisplayedDecimatedByAux
                    = VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByAux;
    Statistics_p->DisplayPictureDisplayedDecimatedBySec
                    = VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedBySec;
    Statistics_p->DisplayPbPictureTooLateRejectedByMain
                    = VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByMain;
    Statistics_p->DisplayPbPictureTooLateRejectedByAux
                    = VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByAux;
    Statistics_p->DisplayPbPictureTooLateRejectedBySec
                    = VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedBySec;
    Statistics_p->DisplayPbPicturePreparedAtLastMinuteRejected
                    = VIDDISP_Data_p->StatisticsPbPicturePreparedAtLastMinuteRejected;
    Statistics_p->DisplayPbQueueLockedByLackOfPicture
                    = VIDDISP_Data_p->StatisticsPbQueueLockedByLackOfPicture;
    Statistics_p->DisplayPbQueueOverflow
                    = VIDDISP_Data_p->StatisticsPbDisplayQueueOverflow;
#ifdef ST_speed
    Statistics_p->SpeedDisplayIFramesNb
                    = VIDDISP_Data_p->StatisticsSpeedDisplayIFramesNb;
    Statistics_p->SpeedDisplayPFramesNb
                    = VIDDISP_Data_p->StatisticsSpeedDisplayPFramesNb;
    Statistics_p->SpeedDisplayBFramesNb
                    = VIDDISP_Data_p->StatisticsSpeedDisplayBFramesNb;
#else
    Statistics_p->SpeedDisplayIFramesNb = 0;
    Statistics_p->SpeedDisplayPFramesNb = 0;
    Statistics_p->SpeedDisplayBFramesNb = 0;
#endif /*ST_speed*/
    return(ST_NO_ERROR);
} /* End of VIDDISP_GetStatistics() function */

/*******************************************************************************
Name        : VIDDISP_ResetStatistics
Description : Reset the statistics of the display module.
Parameters  : Video display handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER, if get failed.
*******************************************************************************/
ST_ErrorCode_t VIDDISP_ResetStatistics(const VIDDISP_Handle_t DisplayHandle,
                                const STVID_Statistics_t * const Statistics_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset parameters that are said to be reset (giving value != 0) */
    if (Statistics_p->DisplayPictureInsertedInQueue != 0)
    {
        VIDDISP_Data_p->StatisticsPictureInsertedInQueue            = 0;
    }
    if (Statistics_p->DisplayPictureDisplayedByMain != 0)
    {
        VIDDISP_Data_p->StatisticsPictureDisplayedByMain            = 0;
    }
    if (Statistics_p->DisplayPictureDisplayedByAux != 0)
    {
        VIDDISP_Data_p->StatisticsPictureDisplayedByAux             = 0;
    }
    if (Statistics_p->DisplayPictureDisplayedBySec != 0)
    {
        VIDDISP_Data_p->StatisticsPictureDisplayedBySec             = 0;
    }
    if (Statistics_p->DisplayPictureDisplayedDecimatedByMain != 0)
    {
        VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByMain   = 0;
    }
    if (Statistics_p->DisplayPictureDisplayedDecimatedByAux != 0)
    {
        VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedByAux    = 0;
    }
    if (Statistics_p->DisplayPictureDisplayedDecimatedBySec != 0)
    {
        VIDDISP_Data_p->StatisticsPictureDisplayedDecimatedBySec    = 0;
    }
    if (Statistics_p->DisplayPbPictureTooLateRejectedByMain != 0)
    {
        VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByMain    = 0;
    }
    if (Statistics_p->DisplayPbPictureTooLateRejectedByAux != 0)
    {
        VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedByAux     = 0;
    }
    if (Statistics_p->DisplayPbPictureTooLateRejectedBySec != 0)
    {
        VIDDISP_Data_p->StatisticsPbPictureTooLateRejectedBySec     = 0;
    }
    if (Statistics_p->DisplayPbPicturePreparedAtLastMinuteRejected != 0)
    {
        VIDDISP_Data_p->StatisticsPbPicturePreparedAtLastMinuteRejected = 0;
    }
    if (Statistics_p->DisplayPbQueueLockedByLackOfPicture != 0)
    {
        VIDDISP_Data_p->StatisticsPbQueueLockedByLackOfPicture      = 0;
    }
    if (Statistics_p->DisplayPbQueueOverflow != 0)
    {
        VIDDISP_Data_p->StatisticsPbDisplayQueueOverflow            = 0;
    }
#ifdef ST_speed
    if (Statistics_p->SpeedDisplayPFramesNb != 0)
    {
        VIDDISP_Data_p->StatisticsSpeedDisplayIFramesNb = 0;
    }
    if (Statistics_p->SpeedDisplayPFramesNb != 0)
    {
        VIDDISP_Data_p->StatisticsSpeedDisplayPFramesNb = 0;
    }
    if (Statistics_p->SpeedDisplayBFramesNb != 0)
    {
        VIDDISP_Data_p->StatisticsSpeedDisplayBFramesNb = 0;
    }
#endif /*ST_speed*/
    return(ST_NO_ERROR);
} /* End of VIDDISP_ResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

/*******************************************************************************
Name        : VIDDISP_NewMixerConnection
Description : Informs the display that the layer has been connected to a new
              mixer, and that display latencies and frame buffers
              hold time might be changed
Parameters  : Video display handle, display latency, framebuffer hold time
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDISP_NewMixerConnection(const VIDDISP_Handle_t DisplayHandle,
                                          const U32 DisplayLatency,
                                          const U32 FrameBufferHoldTime)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    STOS_MutexLock(VIDDISP_Data_p->ContextLockingMutex_p);

    VIDDISP_Data_p->LayerDisplay[0].DisplayLatency      = DisplayLatency;
    VIDDISP_Data_p->LayerDisplay[1].DisplayLatency      = DisplayLatency;
    VIDDISP_Data_p->LayerDisplay[0].FrameBufferHoldTime = FrameBufferHoldTime;
    VIDDISP_Data_p->LayerDisplay[1].FrameBufferHoldTime = FrameBufferHoldTime;

    STOS_MutexRelease(VIDDISP_Data_p->ContextLockingMutex_p);

    return(ST_NO_ERROR);
} /* End of VIDDISP_NewMixerConnection() function */

/*******************************************************************************
Name        : VIDDISP_GetFrameBufferHoldTime
Description : returns the number of VSYNCs that the framebuffer needs to be
              hold before unlocking it, after beeing displayed
Parameters  : Video display handle
Assumptions :
Limitations :
Returns     : framebuffer hold time
*******************************************************************************/
ST_ErrorCode_t VIDDISP_GetFrameBufferHoldTime(const VIDDISP_Handle_t DisplayHandle, U8* const FrameBufferHoldTime_p)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    (*FrameBufferHoldTime_p) = VIDDISP_Data_p->LayerDisplay[0].FrameBufferHoldTime;

    return(ST_NO_ERROR);
} /* End of VIDDISP_GetFrameBufferHoldTime() function */

/*******************************************************************************
Name        : VIDDISP_SetSlaveDisplayUpdate
Description : Sets or resets IsSlaveSequencerSuspended field of VIDDISP_Data_t to
              bypass (or not) the slave display sequencer.
Parameters  : - Display handle
              - Set IsViewportInitialized to 1 to run slave sequencer.
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void VIDDISP_SetSlaveDisplayUpdate(const VIDDISP_Handle_t DisplayHandle, const BOOL IsViewportInitialized)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    assert(VIDDISP_Data_p != NULL);

    VIDDISP_Data_p->IsSlaveSequencerAllowedToRun = IsViewportInitialized;
} /* End of viddisp_SetSlaveDisplayUpdate() function */


/*******************************************************************************
Name        : VIDDISP_UpdateLayerParams
Description : Set VIDDISP_Data_p->LayerParams
Parameters  : Display handle, LayerParams
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDDISP_UpdateLayerParams(const VIDDISP_Handle_t DisplayHandle,
                               const STLAYER_LayerParams_t * LayerParams_p,
                               const U32 LayerId)
{
    VIDDISP_Data_t * VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;
    assert(VIDDISP_Data_p != NULL);

    VIDDISP_Data_p->LayerDisplay[LayerId].LayerParams = *LayerParams_p;
} /* End of VIDDISP_UpdateLayerParams() function */

/*******************************************************************************
Name        : viddisp_ReserveFrameBuffersForReference
Description : This function reserve a certain number of extra SD Frame Buffers. It corresponds to the
                    maximum number of Frame Buffers that can be locked by the Display (when used
                    as reference)
Parameters  :
Assumptions :
Limitations : It should be called when a Viewport is opened
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t viddisp_ReserveFrameBuffersForReference(const VIDDISP_Handle_t DisplayHandle, const U32 LayerIdent)
{
    U32                         NbrOfExtraPrimaryFrameBuffersRequired, NbrOfExtraSecondaryFrameBuffersRequired;
    VIDDISP_Data_t *    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    if (VIDDISP_Data_p->LayerDisplay[LayerIdent].IsOpened)
    {
        /* Allocate extra frame buffers only for layer requiring them i.e doing DEI */
        if( viddisp_IsDeinterlacingPossible(DisplayHandle, LayerIdent) == TRUE)
        {
            NbrOfExtraPrimaryFrameBuffersRequired = DISPLAYPIPE_EXTRA_FRAME_BUFFERS_NBR;
            NbrOfExtraSecondaryFrameBuffersRequired = DISPLAYPIPE_EXTRA_FRAME_BUFFERS_NBR;
        }
        else
        {
            NbrOfExtraPrimaryFrameBuffersRequired = 0;
            NbrOfExtraSecondaryFrameBuffersRequired = 0;
        }

#ifdef USE_EXTRA_FRAME_BUFFERS
        if (NbrOfExtraPrimaryFrameBuffersRequired != 0)
        {
            /* Reserve the specified number of SD frame buffers */
            VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers(VIDDISP_Data_p->BufferManagerHandle, NbrOfExtraPrimaryFrameBuffersRequired);
        }
        if (NbrOfExtraSecondaryFrameBuffersRequired != 0)
        {
            /* Reserve the specified number of SD frame buffers */
            VIDBUFF_SetNbrOfExtraSecondaryFrameBuffers(VIDDISP_Data_p->BufferManagerHandle, NbrOfExtraSecondaryFrameBuffersRequired);
        }

        /* Save the Number of Extra primary Frame Buffers allocated for this Layer */
        VIDDISP_Data_p->LayerDisplay[LayerIdent].NbrOfExtraPrimaryFrameBuffers = NbrOfExtraPrimaryFrameBuffersRequired;
        VIDDISP_Data_p->LayerDisplay[LayerIdent].NbrOfExtraSecondaryFrameBuffers = NbrOfExtraSecondaryFrameBuffersRequired;
#endif /* USE_EXTRA_FRAME_BUFFERS */
    }

    return (ST_NO_ERROR);
}


/*******************************************************************************
Name        : viddisp_FreeFrameBuffersReservedForReference
Description : This function frees the extra Frame Buffers which may have been allocated.
              It should be called when the Viewport is closed.
Parameters  :
Assumptions :
Limitations : It should be called when a Viewport is closed
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t viddisp_FreeFrameBuffersReservedForReference(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent)
{
    VIDDISP_Data_t *    VIDDISP_Data_p = (VIDDISP_Data_t *) DisplayHandle;

    /* The Viewport on this Layer is closed so we should:
        - Unlock all the Fields which were used as referenced for this layer
        - Free the Extra Frame Buffers */
    viddisp_UnlockAllFields(DisplayHandle, LayerIdent);

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* de-allocate extra frame buffers only for layer requiring them i.e doing DEI */
    if( viddisp_IsDeinterlacingPossible(DisplayHandle, LayerIdent) == TRUE)
    {
        if (VIDDISP_Data_p->LayerDisplay[LayerIdent].NbrOfExtraPrimaryFrameBuffers != 0)
        {
            VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers(VIDDISP_Data_p->BufferManagerHandle, 0);
            VIDBUFF_DeAllocateUnusedExtraBuffers(VIDDISP_Data_p->BufferManagerHandle);
        }
        if (VIDDISP_Data_p->LayerDisplay[LayerIdent].NbrOfExtraSecondaryFrameBuffers != 0)
        {
            VIDBUFF_SetNbrOfExtraSecondaryFrameBuffers(VIDDISP_Data_p->BufferManagerHandle, 0);
            VIDBUFF_DeAllocateUnusedExtraSecondaryBuffers(VIDDISP_Data_p->BufferManagerHandle);
        }
    }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    VIDDISP_Data_p->LayerDisplay[LayerIdent].NbrOfExtraPrimaryFrameBuffers = 0;
    VIDDISP_Data_p->LayerDisplay[LayerIdent].NbrOfExtraSecondaryFrameBuffers = 0;

    return (ST_NO_ERROR);
}



/* end of vid_sets.c */
