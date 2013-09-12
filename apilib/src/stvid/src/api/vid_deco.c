/*******************************************************************************

File name   : vid_deco.c

Description : Video decoder functions commands source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 23 Jun 2000        Created                                          AN
 20 Sep 2000        Implement Dis/EnableSynchronisation              HG
 22 Sep 2000        Implement Get/SetSpeed()                         PLe
 27 Sep 2000        Add function to debug video sync delay           HG
 23 Oct 2000        Implement Start/Stop/Pause/Resume                HG
 06 Feb 2001        Digital Input                                    JA
 14 Feb 2001        Memory profile management Get/SetMemoryProfile   GG
 14 Feb 2001        Pause no more handled by VIDDEC, only by VIDDISP HG
 02 Mar 2001        Parameters' check for STVID_SetMemoryProfile()   GG
 14 Mar 2001        Memory profile's check in STVID_Start function
                    VIDBUFF_AllocateMemoryForProfile return value.   GG
 23 Apr 2001        DecodeOnce: proper actions with VIDDEC event     HG
  6 Feb 2002        Set memory profile now updates DecodedPictures.
                    Now notifyes IMPOSSIBLE_WITH_PROFILE_EVT when
                    problem of buffer number in Freeze() and SetDeco-
                    -dedPictures(). DisplayParams from trickmode now
                    taken into account.                              HG
 17 Feb 2003        New HDPIP functions access.
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/*#define DEBUG_VIDEO_SYNC_DELAY*/

/* Includes ----------------------------------------------------------------- */

#include "api.h"
#include "vid_ctcm.h"

#ifdef ST_avsync
#include "avsync.h"
#endif

#ifdef ST_speed
# include "speed.h"
#endif /* ST_speed */
#ifdef ST_trickmod
# include "trickmod.h"
#endif

#ifdef ST_ordqueue
#include "vid_ordq.h"
#endif /* ST_ordqueue */

#ifdef ST_diginput
# ifndef ST_MasterDigInput
#  include "diginput.h"
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */

#include "vid_err.h"

/* Define to add debug info and traces */
/*#define TRACE_API*/

#ifdef TRACE_API
#define TRACE_UART
#endif /* TRACE_API */

#ifdef TRACE_UART
    #include "trace.h"
#else
    #define TraceBuffer(x)
#endif /* TRACE_UART */

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Minimum number of frame buffers required in order to decode I, IP, or all types of frames */
/*------------------------------------------------------------------------------------------------------------------------*/
/* Decoded Pictures  | I (no freeze)   I (freeze) | IP (no freeze)   IP (freeze) | IPB (no freeze)   IPB (freeze) */
/*------------------------------------------------------------------------------------------------------------------------*/
/* OVW supported     |      1             2       |      2               3       |        3                4      */
/* OVW not supported |      2             2       |      2               3       |        4                4      */
/*------------------------------------------------------------------------------------------------------------------------*/
/* Raster storage mode       |             1              |               2              |                  2             */
/*------------------------------------------------------------------------------------------------------------------------*/
#define STVID_MIN_BUFFER_DECODE_I_OVW            1 /* To store all I's with overwrite */
#define STVID_MIN_BUFFER_DECODE_I_noOVW          2 /* To store all I's without overwrite */

#define STVID_MIN_BUFFER_DECODE_IP               2 /* To store all refs alternatively */

#define STVID_MIN_BUFFER_DECODE_ALL_OVW          3 /* To store 2 refs + B's with overwrite */
#define STVID_MIN_BUFFER_DECODE_ALL_noOVW        4 /* To store 2 refs + B's without overwrite */
#define STVID_MIN_ADDITIONAL_BUFFER_FREEZE       1 /* one more buffer for the freezed picture */

#define STVID_MIN_BUFFER_DECODE_SMOOTH_BACKWARD  6 /* To store refs and B's for smooth backward algorithm */
#define STVID_MIN_ADDITIONAL_BUFFER_MULTI_DECODE 1 /* B overwrite not allowed in multi-decode */
#define STVID_MIN_BUFFER_DIGINPUT                2 /* To store 2 buffers with overwrite */

/* if raster reconstruction (DTV only) macroblocks buffers are only used to store the */
/* references pictures. no MB buffer is needed if only decode of I pictures is required */
/* 2 MB buffers are needed in all the other cases */
#define STVID_MIN_BUFFER_DECODE_I_RASTER         0
#define STVID_MIN_BUFFER_DECODE_IP_RASTER        2
#define STVID_MIN_BUFFER_DECODE_ALL_RASTER       2

/* if deblocking reconstruction, at least 4 main and 4 decimated framebuffers are needed */
#define STVID_MIN_BUFFER_DECODE_AND_DEBLOCK      4

/* min and max deblocking strength allowed */
#define STVID_MPEG2_DEBLOCKING_STRENGTH_MIN   0
#define STVID_MPEG2_DEBLOCKING_STRENGTH_MAX   50


#define MIN_VSYNCS_NUMBER_FOR_FRAME_BUFFER_HOLD_TIME    1

#define MIN_S32 0x80000000

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
#ifdef ST_producer
static void GetMinimalNumberOfBuffers(const STVID_DeviceType_t  DeviceType,
                                      const U8                  FrameBufferHoldTime,
                                      U8* const                 MinBufferDecodeI_p,
                                      U8* const                 MinBufferDecodeIP_p,
                                      U8* const                 MinBufferDecodeAll_p);
static ST_ErrorCode_t stvid_ReadyToDecodeNewPictureForceDecimationFactor(VideoDevice_t * const Device_p, VIDPROD_NewPictureDecodeOrSkip_t *   NewPictureDecodeOrSkipEventData_p);
#endif /* ST_producer */
static ST_ErrorCode_t CheckDisplayParamsAndUpdate(VideoDevice_t * const Device_p);
static ST_ErrorCode_t DeAllocateAfterStop(const VideoDevice_t * Device_p, BOOL RecoverUnusedMemory);
static ST_ErrorCode_t ActionFreeze(VideoDevice_t * const Device_p, const STVID_Freeze_t * const Freeze_p);
static ST_ErrorCode_t ActionPause(VideoDevice_t * const Device_p, const STVID_Freeze_t * const Freeze_p);
static U8 GetNbMainFrameBuffers(const STVID_MemoryProfile_t * const MemProf);
static U8 GetNbDecimatedFrameBuffers(const STVID_MemoryProfile_t * const MemProf);
static void SetNbMainFrameBuffers(STVID_MemoryProfile_t * const MemProf, const U8 NbMainFr);

/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : STVID_Clear
Description : Fills a Picture buffer with a pattern and display it/
              Free currently displayed buffer
Parameters  : Video handle, parameters of the new picture buffer
Assumptions :
Limitations :
Returns     : ERROR_FEATURE_NOT_SUPPORTED, ST_ERROR_BAD_PARAMETER,
              ST_ERROR_INVALID_HANDLE, STVID_ERROR_MEMORY_ACCESS, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_Clear(const STVID_Handle_t Handle, const STVID_ClearParams_t * const ClearParams_p)
{
    VIDBUFF_PictureBuffer_t*                PictureBuffer_p;
    VIDBUFF_GetUnusedPictureBufferParams_t  GetUnusedPictureBufferParams;
    ST_ErrorCode_t                          ErrorCode   = ST_NO_ERROR;
    VIDCOM_InternalProfile_t                MemoryProfile;
    VideoStatus_t                           Status;
    int                                     MinBufferDecodeI_noOVW;
    VideoDevice_t*                          Device_p;
#ifdef ST_display
    BOOL                                    IsAnyPictureDisplayed = FALSE;
    VIDBUFF_PictureBuffer_t                 DisplayedPictureBuffer;
    VideoViewPort_t*                        ViewPort_p;
    U32                                     k;
    U8                                      FrameBufferHoldTime;
#endif /* ST_display */

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    if(ClearParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Device_p = ((VideoUnit_t *)Handle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

    /* If the decoder is not stopped, or if the display has not finished displaying all the pictures in the queue,
       raise an error */
    Status = GET_VIDEO_STATUS(Device_p);
#ifdef ST_display
    if ((Status != VIDEO_STATUS_STOPPED) || (Device_p->DeviceData_p->ExpectingEventStopped))
#else /* ST_display */
    if (Status != VIDEO_STATUS_STOPPED)
#endif /* not ST_display */
    {
        stvid_LeaveCriticalSection(Device_p);
        return(STVID_ERROR_DECODER_RUNNING);
    }

    switch(ClearParams_p->ClearMode)
    {
        case STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER:
#ifdef ST_display
            /* Ensure nobody else accesses layers/viewports. */
            semaphore_wait(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);

            /* Disable all the opened instances of Video ViewPort. */
            k = 0;
            while (k<Device_p->DeviceData_p->MaxViewportNumber)
            {
                ViewPort_p = &Device_p->DeviceData_p->ViewportProperties_p[k];
                if( (ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE) &&
                    (ViewPort_p->Params.DisplayEnable) )
                {
                    ErrorCode = stvid_ActionDisableOutputWindow(ViewPort_p);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        break;
                    }
                    else
                    {
                        /* Set again to TRUE to remember to re-enable it later */
                        ViewPort_p->Params.DisplayEnable = TRUE;
                    }
                }
                k++;
            }
            /* If one of the viewports can't be disabled, raise an error but
             * before try to re-enable the disabled ones */
            if (ErrorCode != ST_NO_ERROR)
            {
                while (k>0)
                {
                    k--;
                    ViewPort_p = &Device_p->DeviceData_p->ViewportProperties_p[k];
                    if( (ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE) &&
                        (ViewPort_p->Params.DisplayEnable) )
                    {
                        /* Set to FALSE because it should be FALSE, it is only TRUE because we forced it */
                        ViewPort_p->Params.DisplayEnable = FALSE;
                        /* Don't verify error code of ActionEnable as we have to conserve the last error code of
                         * disabling action  */
                        stvid_ActionEnableOutputWindow(ViewPort_p);
                    }
                }
            }
            else /* All the opened viewports were disabled */
            {
                /* Reset the display to unqueue the displayed picture */
                ErrorCode = VIDDISP_Reset(Device_p->DeviceData_p->DisplayHandle);
                /* De-allocate unused frame buffers now that decoder is stopped */
                ErrorCode = VIDBUFF_DeAllocateUnusedMemoryOfProfile(Device_p->DeviceData_p->BuffersHandle);
                /* Enable all the opened instances of Video ViewPort. */
                while (k>0)
                {
                    k--;
                    ViewPort_p = &Device_p->DeviceData_p->ViewportProperties_p[k];
                    if( (ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE) &&
                        (ViewPort_p->Params.DisplayEnable) )
                    {
                        /* Set to FALSE because it should be FALSE, it is only TRUE because we forced it */
                        ViewPort_p->Params.DisplayEnable = FALSE;
                        ErrorCode = stvid_ActionEnableOutputWindow(ViewPort_p);
                    }
                }
            }
            /* Ensure nobody else accesses layers/viewports. */
            semaphore_signal(Device_p->DeviceData_p->LayersViewportsLockingSemaphore_p);
#endif /* ST_display */
            break;
        case STVID_CLEAR_DISPLAY_PATTERN_FILL:
            if ((ClearParams_p->PatternAddress1_p == NULL) || (ClearParams_p->PatternAddress2_p == NULL))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
            }
        case STVID_CLEAR_DISPLAY_BLACK_FILL:
            /* Get informations about the currently displayed buffer in order to have the same properties for new buffer */
                MinBufferDecodeI_noOVW = STVID_MIN_BUFFER_DECODE_I_noOVW;
#ifdef ST_display
            VIDDISP_GetFrameBufferHoldTime(Device_p->DeviceData_p->DisplayHandle, &FrameBufferHoldTime);
            if(FrameBufferHoldTime > MIN_VSYNCS_NUMBER_FOR_FRAME_BUFFER_HOLD_TIME)
            {
                MinBufferDecodeI_noOVW++;
            }
#endif /* ST_display */
#ifdef ST_display
            ErrorCode = VIDDISP_GetDisplayedPictureExtendedInfos(Device_p->DeviceData_p->DisplayHandle, &DisplayedPictureBuffer);
            if(ErrorCode == ST_NO_ERROR)
            {
                IsAnyPictureDisplayed = TRUE;
            }
            else
            {
                IsAnyPictureDisplayed = FALSE;
            }
            if (IsAnyPictureDisplayed)
            {
                /* Allocate one frame buffer to fill in */
                /* Request for MinBufferDecodeI_noOVW+1, in case 2 frameBuffers are locked by the display when we stop */
                VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &MemoryProfile);
                ErrorCode = VIDBUFF_AllocateMemoryForProfile(Device_p->DeviceData_p->BuffersHandle, TRUE, MinBufferDecodeI_noOVW+1);

                /* Get a new picture buffer */
                GetUnusedPictureBufferParams.MPEGFrame                   = DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame;
                GetUnusedPictureBufferParams.PictureStructure            = DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure;
                GetUnusedPictureBufferParams.TopFieldFirst               = DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst;
                GetUnusedPictureBufferParams.PictureWidth                = DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width;
                GetUnusedPictureBufferParams.PictureHeight               = DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height;
                GetUnusedPictureBufferParams.DecodingOrderFrameID        = DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID;
                GetUnusedPictureBufferParams.ExpectingSecondField        = DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields;
                GetUnusedPictureBufferParams.PictureStructure            = STVID_PICTURE_STRUCTURE_FRAME;
                GetUnusedPictureBufferParams.TopFieldFirst               = TRUE;
                GetUnusedPictureBufferParams.ExpectingSecondField       = FALSE;
                GetUnusedPictureBufferParams.MPEGFrame                   = STVID_MPEG_FRAME_I;
            }
            else
#endif /* ST_display */
            {
                /* Allocate one frame buffer to fill in */
                VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &MemoryProfile);
                ErrorCode = VIDBUFF_AllocateMemoryForProfile(Device_p->DeviceData_p->BuffersHandle, TRUE, MinBufferDecodeI_noOVW - 1);

                /* Get a new picture buffer */
                GetUnusedPictureBufferParams.MPEGFrame                   = STVID_MPEG_FRAME_I;
                GetUnusedPictureBufferParams.PictureStructure            = STVID_PICTURE_STRUCTURE_FRAME;
                GetUnusedPictureBufferParams.TopFieldFirst               = TRUE;
                GetUnusedPictureBufferParams.PictureWidth                = MemoryProfile.ApiProfile.MaxWidth;
                GetUnusedPictureBufferParams.PictureHeight               = MemoryProfile.ApiProfile.MaxHeight;
                GetUnusedPictureBufferParams.DecodingOrderFrameID        = 0;
                GetUnusedPictureBufferParams.ExpectingSecondField       = FALSE;
            }

            if(ErrorCode == ST_NO_ERROR)
            {
                /* Call to VIDBUFF_GetUnusedPictureBuffer function.*/
                if((((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT)   ||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)   ||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT) ||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT)||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT)||
                    (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) ||
	                (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT))
                    &&(MemoryProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE))
                   	||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)
                	||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)
                    || (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)
                	||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
                {
                    ErrorCode = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(Device_p->DeviceData_p->BuffersHandle, &GetUnusedPictureBufferParams, &PictureBuffer_p, VIDCOM_VIDDECO_MODULE_BASE);
                }
                else
                {
                    ErrorCode = VIDBUFF_GetAndTakeUnusedPictureBuffer(Device_p->DeviceData_p->BuffersHandle, &GetUnusedPictureBufferParams, &PictureBuffer_p, VIDCOM_VIDDECO_MODULE_BASE);
                }
            }

            if(ErrorCode == ST_NO_ERROR)
            {
#ifdef ST_display
                if (IsAnyPictureDisplayed)
                {
                    /* Fill the new PictureBuffer structure with the same parameters as those of the displayed one*/
                    memcpy(&(PictureBuffer_p->ParsedPictureInformation.PictureGenericData),
                            &(DisplayedPictureBuffer.ParsedPictureInformation.PictureGenericData),
                            sizeof(VIDCOM_PictureGenericData_t));
                }
                else /* No picture on display */
#endif /* ST_display */
                {
                    BitmapDefaultFillParams_t   BitmapDefaultFillParams;
                    U8 panandscan;

                    /* Prepare structure to get default params for the source bitmap */
                    BitmapDefaultFillParams.DeviceType      = Device_p->DeviceData_p->DeviceType;
                    BitmapDefaultFillParams.BrdCstProfile   = Device_p->DeviceData_p->StartParams.BrdCstProfile;
                    BitmapDefaultFillParams.VTGFrameRate    = Device_p->DeviceData_p->VTGFrameRate;

                    /* Bitmap params */
                    vidcom_FillBitmapWithDefaults(&BitmapDefaultFillParams, &(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams));
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width  = MemoryProfile.ApiProfile.MaxWidth;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height = MemoryProfile.ApiProfile.MaxHeight;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Pitch  = MemoryProfile.ApiProfile.MaxWidth;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Offset = 0;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Data1_p  = PictureBuffer_p->FrameBuffer_p->Allocation.Address_p;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Size1    = PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize - PictureBuffer_p->FrameBuffer_p->Allocation.Size2;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Data2_p  = PictureBuffer_p->FrameBuffer_p->Allocation.Address2_p;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Size2    = PictureBuffer_p->FrameBuffer_p->Allocation.Size2;
                    /* Video Params */
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.FrameRate         = Device_p->DeviceData_p->VTGFrameRate;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.ScanType          = STGXOBJ_INTERLACED_SCAN;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame         = STVID_MPEG_FRAME_I;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure  = STVID_PICTURE_STRUCTURE_FRAME;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst     = TRUE;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.CompressionLevel  = STVID_COMPRESSION_LEVEL_NONE;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors = STVID_DECIMATION_FACTOR_NONE;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.PictureBufferHandle = (STVID_PictureBufferHandle_t) NULL;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.DecimatedPictureBufferHandle = (STVID_PictureBufferHandle_t) NULL;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField = FALSE;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.RepeatProgressiveCounter = 0;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID = 0;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = 0;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsDisplayBoundPictureIDValid = TRUE;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DisplayBoundPictureID.ID = 0;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DisplayBoundPictureID.IDExtension = 0;

                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsReference = TRUE; /* we set this dummy picture as I frame */

                    PictureBuffer_p->AssociatedDecimatedPicture_p                   = NULL;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields = FALSE;
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = 0;

                    for (panandscan = 0; panandscan < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; panandscan++)
                    {
                        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset = 0;
                        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset = 0;
                        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].DisplayHorizontalSize = MemoryProfile.ApiProfile.MaxWidth * 16;
                        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].DisplayVerticalSize = MemoryProfile.ApiProfile.MaxHeight * 16;
                        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].HasDisplaySizeRecommendation = FALSE;
                        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset = 0;
                        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset = 0;
                    }
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.NumberOfPanAndScan = 0;
                    /* to bypass any future display error recovery */
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;
                    /* to bypass any future display error recovery */
                    PictureBuffer_p->Decode.DecodingError         = VIDCOM_ERROR_LEVEL_NONE;
                    PictureBuffer_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_DECODED;
                    PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p = NULL;

                }

#ifdef ST_producer
                /* Set as a reference here */
                ErrorCode = VIDPROD_NewReferencePicture(Device_p->DeviceData_p->ProducerHandle, PictureBuffer_p );
#endif /* ST_producer */
                if(ErrorCode == ST_NO_ERROR)
                {
                    /* Clear the framebuffer */
                    ErrorCode = VIDBUFF_ClearFrameBuffer(Device_p->DeviceData_p->BuffersHandle,
                                                            &(PictureBuffer_p->FrameBuffer_p->Allocation),
                                                            ClearParams_p);
#ifdef ST_display
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        /* We are internally waiting for display stop */
                        Device_p->DeviceData_p->ExpectingEventStopped = TRUE;
                        /* Don't inform the world when display stops */
                        Device_p->DeviceData_p->NotifyStopped = FALSE;
                        ErrorCode = VIDDISP_Start(Device_p->DeviceData_p->DisplayHandle, TRUE);
                        if (ErrorCode == ST_NO_ERROR)
                        {
#ifdef ST_ordqueue
                            /* Call to VIDQUEUE_InsertPictureInQueue function.
                            * All parameters already tested, no need to check function return value! */
                            ErrorCode = VIDQUEUE_InsertPictureInQueue(Device_p->DeviceData_p->OrderingQueueHandle, PictureBuffer_p, VIDQUEUE_INSERTION_ORDER_LAST_PLACE);
                            if (ErrorCode == ST_NO_ERROR)
                            {
                                /* Push directly picture to display */
                                ErrorCode = VIDQUEUE_PushPicturesToDisplay(Device_p->DeviceData_p->OrderingQueueHandle,
                                                &(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));
                            }
#endif /* ST_ordqueue */
                            /* We need to inform the display that a discontinuity occured in MPEG frame
                            continuity (only one picture in queue), so that it is not locked for ever. */
                            VIDDISP_SetDiscontinuityInMPEGDisplay(Device_p->DeviceData_p->DisplayHandle);
                            if (ErrorCode == ST_NO_ERROR)
                            {
                                ErrorCode = VIDDISP_Stop(Device_p->DeviceData_p->DisplayHandle,
                                                        NULL,
                                                        VIDDISP_STOP_LAST_PICTURE_IN_QUEUE);
                            }
                        }
                        /* If display start/stop request failed, don't expect to be notified of the display stop */
                        if(ErrorCode != ST_NO_ERROR)
                        {
                            Device_p->DeviceData_p->ExpectingEventStopped = FALSE;
                        }
                    }
#endif /* ST_display */
                }
            }
            break;
        default:
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    } /* switch ClearMode */
    stvid_LeaveCriticalSection(Device_p);
    return(ErrorCode);
} /* End of STVID_Clear() function */

/*******************************************************************************
Name        : STVID_DataInjectionCompleted
Description : Indicates end of data injection
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_DataInjectionCompleted(const STVID_Handle_t VideoHandle, const STVID_DataInjectionCompletedParams_t * const Params_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoDevice_t * Device_p;

    /* Exit now if parameters are bad */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

#ifdef ST_diginput
# ifdef ST_MasterDigInput
    if (Device_p->DeviceData_p->DigitalInputIsValid)
# else
    if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
# endif /* ST_MasterDigInput */
    {
        /* Nothing to do for digital input */
        return(ST_NO_ERROR);
    }
#endif /*ST_diginput */

    stvid_EnterCriticalSection(Device_p);

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        /* Inform decoder of end of injection */
        /*ErrorCode =*/ VIDPROD_DataInjectionCompleted(Device_p->DeviceData_p->ProducerHandle, Params_p);
    }
#endif /* ST_producer */

#ifdef ST_smoothbackward
    if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    {
        /* If trick mode is present, inform trick mode */
        /*ErrorCode =*/ VIDTRICK_DataInjectionCompleted(Device_p->DeviceData_p->TrickModeHandle, Params_p);
    }
#endif /* ST_smoothbackward */

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);

} /* End of STVID_DataInjectionCompleted() function */

/*******************************************************************************
Name        : STVID_DeleteDataInputInterface
Description : fct used to autorize the video to controle injection if done
                by software. cascaded to decode module
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_DeleteDataInputInterface(const STVID_Handle_t VideoHandle)
{
#if !defined(ST_51xx_Device) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200)
    return(ST_ERROR_FEATURE_NOT_SUPPORTED); /* inj controlled by hardware */
#else
    ST_ErrorCode_t ErrorCode;
    VideoDevice_t * Device_p;
    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    ErrorCode = VIDPROD_DeleteDataInputInterface(Device_p->DeviceData_p->ProducerHandle);
    return(ErrorCode);
#endif
}

/*******************************************************************************
Name        : STVID_DisableFrameRateConversion
Description : Disable the frame rate conversion
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_DisableFrameRateConversion(const STVID_Handle_t VideoHandle)
{
#ifdef ST_display
    VideoDevice_t * Device_p;
#endif
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_display
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    if (Device_p->DeviceData_p->DisplayHandleIsValid)
    {
        VIDDISP_DisableFrameRateConversion(Device_p->DeviceData_p->DisplayHandle);
    }
#else
     ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

    return(ErrorCode);
} /* End of STVID_DisableFrameRateConversion() function */


/*******************************************************************************
Name        : STVID_DisableSynchronisation
Description : Disables the video synchronization with the local system clock.
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE
*******************************************************************************/
ST_ErrorCode_t STVID_DisableSynchronisation(const STVID_Handle_t VideoHandle)
{
#ifdef ST_avsync
    VideoDevice_t * Device_p;
#endif /* ST_avsync */
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_avsync
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    /* Error not allowed */
    /*ErrorCode =*/ VIDAVSYNC_DisableSynchronization(Device_p->DeviceData_p->AVSyncHandle);
#else /* ST_avsync */
    /* No avsync: synchronization is always disabled ! */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_avsync */

    return(ErrorCode);
} /* End of STVID_DisableSynchronisation() function */

/*******************************************************************************
Name        : STVID_EnableFrameRateConversion
Description : Enable the frame rate conversion
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_EnableFrameRateConversion(const STVID_Handle_t VideoHandle)
{
#ifdef ST_display
    VideoDevice_t * Device_p;
#endif
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_display
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    if (Device_p->DeviceData_p->DisplayHandleIsValid)
    {
        VIDDISP_EnableFrameRateConversion(Device_p->DeviceData_p->DisplayHandle);
    }
#else
     ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

    return(ErrorCode);
} /* End of STVID_EnableFrameRateConversion() function */


/*******************************************************************************
Name        : STVID_EnableSynchronisation
Description : Enables video synchonisation with local system clock
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_EnableSynchronisation(const STVID_Handle_t VideoHandle)
{
#ifdef ST_avsync
    VideoDevice_t * Device_p;
#endif /* ST_avsync*/
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_avsync
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    /* Enable automatic AVSync */
    /* Error not allowed */
    /*ErrorCode =*/ VIDAVSYNC_EnableSynchronization(Device_p->DeviceData_p->AVSyncHandle);
#else /* ST_avsync */
    /* Only avsync module can set synchonisation. */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* not ST_avsync*/

    return(ErrorCode);
} /* End of STVID_EnableSynchronisation() function */


/*******************************************************************************
Name        : STVID_DisableDisplay
Description :
Parameters  : IN  : Video Handle.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED.
*******************************************************************************/
ST_ErrorCode_t STVID_DisableDisplay(const STVID_Handle_t VideoHandle)
{
#ifdef ST_display
    VideoDevice_t * Device_p;
#endif
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_display
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    if (Device_p->DeviceData_p->DisplayHandleIsValid)
    {
	VIDDISP_DisableDisplay(Device_p->DeviceData_p->DisplayHandle, Device_p->DeviceName);
    }
#else
     ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_EnableDisplay
Description :
Parameters  : IN  : Video Handle.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED.
*******************************************************************************/
ST_ErrorCode_t STVID_EnableDisplay(const STVID_Handle_t VideoHandle)
{
#ifdef ST_display
    VideoDevice_t * Device_p;
#endif
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_display
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    if (Device_p->DeviceData_p->DisplayHandleIsValid)
    {
	VIDDISP_EnableDisplay(Device_p->DeviceData_p->DisplayHandle, Device_p->DeviceName);
    }
#else
     ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

    return(ErrorCode);
}

/*******************************************************************************
Name        : STVID_GetDataInputBufferParams
Description : if req (injection via video): appli must get such info and get
              it to the pti.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetDataInputBufferParams(const STVID_Handle_t
     VideoHandle, void ** const BaseAddress_p, U32 * const Size_p)
{
#if !defined(ST_51xx_Device) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200)
    return(ST_ERROR_FEATURE_NOT_SUPPORTED); /* inj controlled by hardware */
#else
    ST_ErrorCode_t ErrorCode;
    VideoDevice_t * Device_p;
    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

    ErrorCode = VIDPROD_GetDataInputBufferParams(Device_p->DeviceData_p->ProducerHandle, BaseAddress_p, Size_p);

    return(ErrorCode);
#endif
}

/*******************************************************************************
Name        : STVID_Freeze
Description : Freezes the decoded video
Parameters  : Video handle, Freeze mode
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED, STVID_ERROR_DECODER_FREEZING, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_Freeze(const STVID_Handle_t VideoHandle,
                            const STVID_Freeze_t * const Freeze_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoStatus_t    Status;

    /* Exit now if parameters are bad */
    if (Freeze_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_FREEZING)
    {
        ErrorCode = STVID_ERROR_DECODER_FREEZING;
    }
    else if (Status == VIDEO_STATUS_STOPPED)
    {
        ErrorCode = STVID_ERROR_DECODER_STOPPED;
    }
    else
    {
        switch (Freeze_p->Mode)
        {
            case STVID_FREEZE_MODE_NONE :
            case STVID_FREEZE_MODE_FORCE :
            case STVID_FREEZE_MODE_NO_FLICKER :
                break;

            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Freeze(): Invalid Freeze Mode !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
        }

        switch (Freeze_p->Field)
        {
            case STVID_FREEZE_FIELD_TOP :
            case STVID_FREEZE_FIELD_BOTTOM :
            case STVID_FREEZE_FIELD_CURRENT :
            case STVID_FREEZE_FIELD_NEXT :
                break;

            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Freeze(): Invalid Freeze Field !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Do actions for freeze in all modules */
        ErrorCode = ActionFreeze(Device_p, Freeze_p);
    }

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_Freeze function */


/*******************************************************************************
Name        : ActionFreeze
Description : Freezes the decoded video internally, with no handle
Parameters  : Video device, Freeze mode
Assumptions : Function call is protected with stvid_EnterCriticalSection(Device_p)
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, STVID_ERROR_DECODER_FREEZING, ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t ActionFreeze(VideoDevice_t * const Device_p, const STVID_Freeze_t * const Freeze_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDCOM_InternalProfile_t  CommonMemoryProfile;
    U8 MinBufferFreezeDecodeI, MinBufferFreezeDecodeIP, MinBufferFreezeDecodeAll;

#ifdef ST_producer
    STVID_DecodedPictures_t DecodedPictures;
#endif /* ST_producer */
#if defined ST_trickmod || defined ST_speed
    S32 Speed = 100;
#endif /*  ST_trickmod || ST_speed*/

#ifdef ST_speed
    /* !!! HG: TO BE REMOVED !!! because freeze should be supported at any speed ! */
    if (Device_p->DeviceData_p->SpeedHandleIsValid)
    {
        ErrorCode = VIDSPEED_GetSpeed(Device_p->DeviceData_p->SpeedHandle, &Speed);
        /* if speed > 300, The display queue falls in a loked state.  */
        /* If speed < 100, VIDTRICK_Repeatfiels should be modified so that it should */
        /* increase counters of the second picture of the display queue */
        /* That's why it may be better to forbidden STVID_Freeze if speed != 100 */
        if ((ErrorCode == ST_NO_ERROR) && (Speed != 100))
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
#endif /*ST_speed*/
#ifdef ST_trickmod
    /* !!! HG: TO BE REMOVED !!! because freeze should be supported at any speed ! */
    if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    {
        ErrorCode = VIDTRICK_GetSpeed(Device_p->DeviceData_p->TrickModeHandle, &Speed);
        /* if speed > 300, The display queue falls in a loked state.  */
        /* If speed < 100, VIDTRICK_Repeatfiels should be modified so that it should */
        /* increase counters of the second picture of the display queue */
        /* That's why it may be better to forbidden STVID_Freeze if speed != 100 */
        if ((ErrorCode == ST_NO_ERROR) && (Speed != 100))
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
#endif /*ST_trickmod*/

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Get memory profile to check if there's enough buffers to do the freeze. */
        ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &CommonMemoryProfile);
        if (ErrorCode != ST_NO_ERROR)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_GetMemoryProfile failed"));
        }
        else
        {
#ifdef ST_producer
            /* Decode: must have one more buffer than minimum */
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                ErrorCode = VIDPROD_GetDecodedPictures(Device_p->DeviceData_p->ProducerHandle, &DecodedPictures);

                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetDecodedPictures() : failed !"));
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    if (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5528_MPEG)
                    {
                        MinBufferFreezeDecodeI   = STVID_MIN_BUFFER_DECODE_I_noOVW;
                        MinBufferFreezeDecodeIP  = STVID_MIN_BUFFER_DECODE_IP + STVID_MIN_ADDITIONAL_BUFFER_FREEZE;
                        MinBufferFreezeDecodeAll = STVID_MIN_BUFFER_DECODE_ALL_noOVW;
                    }
                    else if (  (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG)
                             ||(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG))
                    {
                        MinBufferFreezeDecodeI   = STVID_MIN_BUFFER_DECODE_I_noOVW   ;
                        MinBufferFreezeDecodeIP  = STVID_MIN_BUFFER_DECODE_IP + STVID_MIN_ADDITIONAL_BUFFER_FREEZE;
                        MinBufferFreezeDecodeAll = STVID_MIN_BUFFER_DECODE_ALL_noOVW ;
                    }
                    else
                    {
                        MinBufferFreezeDecodeI   = STVID_MIN_BUFFER_DECODE_I_OVW    + STVID_MIN_ADDITIONAL_BUFFER_FREEZE;
                        MinBufferFreezeDecodeIP  = STVID_MIN_BUFFER_DECODE_IP       + STVID_MIN_ADDITIONAL_BUFFER_FREEZE;
                        MinBufferFreezeDecodeAll = STVID_MIN_BUFFER_DECODE_ALL_OVW  + STVID_MIN_ADDITIONAL_BUFFER_FREEZE;
                    }
                    /* Having memory profile and decoded pictures: can check if freeze is possible */
                    if (!((CommonMemoryProfile.NbMainFrameStore >= MinBufferFreezeDecodeAll) ||
                         ((CommonMemoryProfile.NbMainFrameStore >= MinBufferFreezeDecodeIP) && (DecodedPictures != STVID_DECODED_PICTURES_ALL)) ||
                         ((CommonMemoryProfile.NbMainFrameStore >= MinBufferFreezeDecodeI) && (DecodedPictures == STVID_DECODED_PICTURES_I))))
                    {
                        ErrorCode = STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE;
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Parameter incompatible with the current Memory Profile."));
                        /* Propose an updated number of frame buffers */
                        if (DecodedPictures == STVID_DECODED_PICTURES_I)
                        {
                            SetNbMainFrameBuffers(&(CommonMemoryProfile.ApiProfile), MinBufferFreezeDecodeI);
                            CommonMemoryProfile.NbMainFrameStore = MinBufferFreezeDecodeI;
                        }
                        else if (DecodedPictures == STVID_DECODED_PICTURES_IP)
                        {
                            SetNbMainFrameBuffers(&(CommonMemoryProfile.ApiProfile), MinBufferFreezeDecodeIP);
                            CommonMemoryProfile.NbMainFrameStore = MinBufferFreezeDecodeIP;
                        }
                        else
                        {
                            SetNbMainFrameBuffers(&(CommonMemoryProfile.ApiProfile), MinBufferFreezeDecodeAll);
                            CommonMemoryProfile.NbMainFrameStore = MinBufferFreezeDecodeAll;
                        }
                        /* Notify the event. */
                        STEVT_Notify(Device_p->DeviceData_p->EventsHandle,
                                Device_p->DeviceData_p->EventID[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID],
                                STEVT_EVENT_DATA_TYPE_CAST &CommonMemoryProfile.ApiProfile);
                    }
                }
            }
#endif /* ST_producer */
#ifdef ST_diginput
#ifndef ST_MasterDigInput
            if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
            {
                /* To be defined: how much buffers are necessary for digital input ? */
                if (CommonMemoryProfile.NbMainFrameStore < (STVID_MIN_BUFFER_DIGINPUT + STVID_MIN_ADDITIONAL_BUFFER_FREEZE))
                {
                    ErrorCode = STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE;
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Parameter incompatible with the current Memory Profile."));
                    /* Propose an updated number of frame buffers */
                    SetNbMainFrameBuffers(&(CommonMemoryProfile.ApiProfile), (STVID_MIN_BUFFER_DIGINPUT + STVID_MIN_ADDITIONAL_BUFFER_FREEZE));
                    CommonMemoryProfile.NbMainFrameStore = (STVID_MIN_BUFFER_DIGINPUT + STVID_MIN_ADDITIONAL_BUFFER_FREEZE);
                    /* Notify the event. */
                    STEVT_Notify(Device_p->DeviceData_p->EventsHandle,
                            Device_p->DeviceData_p->EventID[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID],
                            STEVT_EVENT_DATA_TYPE_CAST &CommonMemoryProfile.ApiProfile);
                }
            } /* end digital input */
#endif /* ST_MasterDigInput */
#endif /* ST_diginput */
        } /* end get profile successful */
    } /* no error */

#ifdef ST_diginput
#ifndef ST_MasterDigInput
    if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->DigitalInputHandleIsValid))
    {
        ErrorCode = VIDINP_Freeze(Device_p->DeviceData_p->DigitalInputHandle);
    }
#endif /* ST_MasterDigInput */
#endif /* ST_diginput */

#ifdef ST_producer
    if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->ProducerHandleIsValid))
    {
        ErrorCode = VIDPROD_Freeze(Device_p->DeviceData_p->ProducerHandle);
    }
#endif /* ST_producer */
    if (ErrorCode == ST_NO_ERROR)
    {
#ifdef ST_display
        if (Device_p->DeviceData_p->DisplayHandleIsValid)
        {
            ErrorCode = VIDDISP_Freeze(Device_p->DeviceData_p->DisplayHandle, Freeze_p);
        }
#else /* ST_display */
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_display */
        if (ErrorCode != ST_NO_ERROR)
        {
#ifdef ST_producer
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                /*ErrorCode =*/ VIDPROD_Resume(Device_p->DeviceData_p->ProducerHandle);
            }
#endif /* ST_producer */
#ifdef ST_diginput
# ifndef ST_MasterDigInput
            if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
            {
                /*ErrorCode =*/ VIDINP_Resume(Device_p->DeviceData_p->DigitalInputHandle);
            }
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */
        }
        else
        {
            SET_VIDEO_STATUS(Device_p, VIDEO_STATUS_FREEZING);
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Freeze() done."));

#ifdef STVID_DEBUG_GET_STATUS
			Device_p->DeviceData_p->LastFreezeParams = (*Freeze_p);
			Device_p->DeviceData_p->VideoAlreadyFreezed = TRUE;
#endif /* STVID_DEBUG_GET_STATUS */

        }
    }

    return(ErrorCode);
} /* End of ActionFreeze() function */


/*******************************************************************************
Name        : STVID_GetBitBufferFreeSize
Description : Get the bit buffer free size.
Parameters  : Video handle, free size.
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_GetBitBufferFreeSize(const STVID_Handle_t VideoHandle, U32 * const FreeSize_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (FreeSize_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
#ifdef ST_trickmod
        S32   Speed;

        if (Device_p->DeviceData_p->TrickModeHandleIsValid)
        {
            /* Get current play speed to determine driver mode (AUTOMATIC or MANUAL) */
            /* then Bit Buffer management policy (Circular or Linear)                */
            VIDTRICK_GetSpeed(Device_p->DeviceData_p->TrickModeHandle, &Speed);
            if (Speed < 0)
            {
                ErrorCode = VIDTRICK_GetBitBufferFreeSize(Device_p->DeviceData_p->TrickModeHandle, FreeSize_p);
            }
            else
            {
                ErrorCode = VIDPROD_GetBitBufferFreeSize(Device_p->DeviceData_p->ProducerHandle, FreeSize_p);
            }
        }
        else
#endif /* ST_trickmod */
        {
            ErrorCode = VIDPROD_GetBitBufferFreeSize(Device_p->DeviceData_p->ProducerHandle, FreeSize_p);
        }

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetBitBufferFreeSize() : failed !"));
        }
    }
    else
    {
        /* Only decode module is concerned by a bit buffer */
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
#else /* ST_producer */
    /* Only decode module is concerned by a bit buffer */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* not ST_producer */

    return(ErrorCode);
} /* End of STVID_GetBitBufferFreeSize() function */


/*******************************************************************************
Name        : STVID_GetDecodedPictures
Description : Get the picture coding type decoded.
Parameters  : Video handle, pointer to fill.
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_GetDecodedPictures(const STVID_Handle_t VideoHandle, STVID_DecodedPictures_t * const DecodedPictures_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (DecodedPictures_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        ErrorCode = VIDPROD_GetDecodedPictures(Device_p->DeviceData_p->ProducerHandle, DecodedPictures_p);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetDecodedPictures() : failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        /* Only decode module can restrict pictures decoded */
        *DecodedPictures_p = STVID_DECODED_PICTURES_ALL;
    }
#else /* ST_producer */
    /* Only decode module can restrict pictures decoded */
    *DecodedPictures_p = STVID_DECODED_PICTURES_ALL;
#endif /* not ST_producer */

    return(ErrorCode);
} /* End of STVID_GetDecodedPictures() function */

/*******************************************************************************
Name        : STVID_GetHDPIPParams
Description : Get Currently applyed HD-PIP parameters.
Parameters  : Video handle, HD-PIP parameters to fill.
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t STVID_GetHDPIPParams(const STVID_Handle_t Handle,
        STVID_HDPIPParams_t * const HDPIPParams_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (HDPIPParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)Handle)->Device_p;

    /* Test if device type is correct.  */
    if ( (Device_p->DeviceData_p->DeviceType  != STVID_DEVICE_TYPE_7020_MPEG) ||
         ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7020_MPEG) &&
         ((Device_p->DecoderNumber != 0) && (Device_p->DecoderNumber != 1))) )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "HDPIP only accessible for OMEGA2 chips, decoder 1 or 2"));
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#ifdef ST_producer
    ErrorCode = VIDPROD_GetHDPIPParams(Device_p->DeviceData_p->ProducerHandle,
            HDPIPParams_p);
#else /* ST_producer */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* ST_producer */

    return(ErrorCode);

} /* End of STVID_GetHDPIPParams() function. */

/*******************************************************************************
Name        : STVID_GetMemoryProfile
Description : Get the memory profile set by the function STVID_SetMemoryProfile().
Parameters  : Video handle, memory profile to fill with the current one.
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_GetMemoryProfile(const STVID_Handle_t VideoHandle, STVID_MemoryProfile_t * const MemoryProfile_p)
{
    VideoDevice_t * Device_p;
    VIDCOM_InternalProfile_t InternalProfile;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (MemoryProfile_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &InternalProfile);

    if (ErrorCode == ST_NO_ERROR)
    {
        *MemoryProfile_p =  InternalProfile.ApiProfile;
    }

    return(ErrorCode);

} /* End of STVID_GetMemoryProfile() function */



/*******************************************************************************
Name        : STVID_GetSpeed
Description : Retrieves the current play speed.
Parameters  : Video handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_GetSpeed(const STVID_Handle_t VideoHandle, S32 * const Speed_p)
{
    VideoDevice_t * Device_p;
#if defined ST_trickmod || defined ST_speed
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#endif /* ST_trickmod || ST_speed */

    /* Exit now if parameters are bad */
    if (Speed_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    if (IsTrickModeHandleOrSpeedHandleValid(Device_p))
    {
#ifdef ST_speed
    	if (Device_p->DeviceData_p->SpeedHandleIsValid)
    	{
        	ErrorCode = VIDSPEED_GetSpeed(Device_p->DeviceData_p->SpeedHandle, Speed_p);
        	if (ErrorCode != ST_NO_ERROR)
        	{
            	STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDSPEED_GetSpeed() : failed !"));
            	ErrorCode = ST_ERROR_BAD_PARAMETER;
        	}
    	}
#endif /* ST_speed */
#ifdef ST_trickmod
    	if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    	{
        	ErrorCode = VIDTRICK_GetSpeed(Device_p->DeviceData_p->TrickModeHandle, Speed_p);
        	if (ErrorCode != ST_NO_ERROR)
        	{
            	STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDTRICK_GetSpeed() : failed !"));
            	ErrorCode = ST_ERROR_BAD_PARAMETER;
        	}
    	}
#endif /* ST_trickmod */
    }
    else
    {
        *Speed_p = 100;
    }

    return(ST_NO_ERROR);
} /* End of STVID_GetSpeed() function */


#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
/*******************************************************************************
Name        : STVID_GetSynchronizationDelay
Description : Get the video synchronization delay (on the extended STC) set by
              the function STVID_SetSynchronizationDelay().
Parameters  : Video handle, Synchronization Delay pointer to fill.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR             if success,
              ST_ERROR_BAD_PARAMETER  if the AVSync component fails or NULL pointer,
              ST_ERROR_BAD_PARAMETER  if SyncDelay_p is a NULL pointer,
              ST_ERROR_INVALID_HANDLE if the Video handle is invalid.
*******************************************************************************/
ST_ErrorCode_t STVID_GetSynchronizationDelay(const STVID_Handle_t VideoHandle, S32 * const SyncDelay_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (SyncDelay_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

#ifdef ST_avsync
    ErrorCode = VIDAVSYNC_GetSynchronizationDelay(Device_p->DeviceData_p->AVSyncHandle, SyncDelay_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDAVSYNC_GetSynchronizationDelay failed"));
    }
#else /* ST_avsync */
    /* No avsync: nothing to do */
    ErrorCode = ST_NO_ERROR;
#endif /* no ST_avsync */

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_GetSynchronizationDelay() function */
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */


/*******************************************************************************
Name        : STVID_InjectDiscontinuity
Description : Injects discontinuity
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE or ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_InjectDiscontinuity(const STVID_Handle_t VideoHandle)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        /* Inform decoder of end of injection */
        /*ErrorCode =*/  VIDPROD_FlushInjection(Device_p->DeviceData_p->ProducerHandle);
    }
#endif /* ST_producer */

    return(ST_NO_ERROR);
} /* End of STVID_InjectDiscontinuity() function */


/*******************************************************************************
Name        : STVID_Pause
Description : Pauses the decoded video
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED, STVID_ERROR_RUNNING_IN_RT_MODE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_Pause(const STVID_Handle_t VideoHandle,
        const STVID_Freeze_t * const Freeze_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoStatus_t    Status;

    /* Exit now if parameters are bad */
    if (Freeze_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

#ifdef ST_diginput
# ifdef ST_MasterDigInput
    if (Device_p->DeviceData_p->DigitalInputIsValid)
# else /* ST_MasterDigInput */
    if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
# endif /* not ST_MasterDigInput*/
    {
        /* Digital input is real time: only Freeze() allowed */
        return(STVID_ERROR_DECODER_RUNNING_IN_RT_MODE);
    }
#endif /* ST_diginput */

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_PAUSING)
    {
        ErrorCode = STVID_ERROR_DECODER_PAUSING;
    }
    else if (Status == VIDEO_STATUS_STOPPED)
    {
        ErrorCode = STVID_ERROR_DECODER_STOPPED;
    }
    else
    {
        switch (Freeze_p->Mode)
        {
            case STVID_FREEZE_MODE_NONE :
            case STVID_FREEZE_MODE_FORCE :
            case STVID_FREEZE_MODE_NO_FLICKER :
                break;

            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Pause(): Invalid Freeze Mode !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
        }

        switch (Freeze_p->Field)
        {
            case STVID_FREEZE_FIELD_TOP :
            case STVID_FREEZE_FIELD_BOTTOM :
            case STVID_FREEZE_FIELD_CURRENT :
                break;

            case STVID_FREEZE_FIELD_NEXT :
                /* STVID_FREEZE_FIELD_NEXT should not be used by STVID_Pause (see STVID Api document) */
            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Pause(): Invalid Freeze Field !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = ActionPause(Device_p, Freeze_p);
    }

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_Pause() function */


/*******************************************************************************
Name        : ActionPause
Description : Pauses the decoded video internally, with no handle
Parameters  : Video device, freeze params
Assumptions : Function calls are protected with stvid_EnterCriticalSection(Device_p)
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, STVID_ERROR_DECODER_RUNNING_IN_RT_MODE, ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t ActionPause(VideoDevice_t * const Device_p, const STVID_Freeze_t * const Freeze_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Device_p->DeviceData_p->StartParams.RealTime)
    {
        /* Decoder is decoding in real time data input mode, thus it is not possible to pause the input stream. */
        ErrorCode = STVID_ERROR_DECODER_RUNNING_IN_RT_MODE;
    }

#ifdef ST_speed
    	/* Disables Trick Mode Speed Control */
        if ((Device_p->DeviceData_p->SpeedHandleIsValid) && (ErrorCode == ST_NO_ERROR))
    	{
        	ErrorCode = VIDSPEED_DisableSpeedControl(Device_p->DeviceData_p->SpeedHandle);
        	if (ErrorCode != ST_NO_ERROR)
        	{
            	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                	"STVID_Pause() : Speed disable speed control failed !"));
            	ErrorCode = ST_ERROR_BAD_PARAMETER;
        	}
    	}
#endif /*ST_speed*/
#ifdef ST_trickmod
    	/* Disables Trick Mode Speed Control */
        if ((Device_p->DeviceData_p->TrickModeHandleIsValid) && (ErrorCode == ST_NO_ERROR))
    	{
        	ErrorCode = VIDTRICK_DisableSpeedControl(Device_p->DeviceData_p->TrickModeHandle);
        	if (ErrorCode != ST_NO_ERROR)
        	{
            	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                	"STVID_Pause() : TrickMode disable speed control failed !"));
            	ErrorCode = ST_ERROR_BAD_PARAMETER;
        	}
    	}
#endif /* ST_trickmod */

#ifdef ST_display
    /* Pauses video display */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDDISP_Pause(Device_p->DeviceData_p->DisplayHandle, Freeze_p);
    }
#else /* ST_display */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_display */

    if (ErrorCode == ST_NO_ERROR)
    {
        SET_VIDEO_STATUS(Device_p, VIDEO_STATUS_PAUSING);
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Pause() done."));
    }

    return(ErrorCode);
} /* End of ActionPause() function */


/*******************************************************************************
Name        : GetNbMainFrameBuffers
Description : Return the number of allocated Main frame buffers
Parameters  : MemoryProfile
Assumptions :
Limitations :
Returns     : U8
*******************************************************************************/
static U8 GetNbMainFrameBuffers(const STVID_MemoryProfile_t * const MemProf)
{
    switch (MemProf->NbFrameStore)
    {
        case STVID_OPTIMISED_NB_FRAME_STORE_ID:
            return (MemProf->FrameStoreIDParams.OptimisedNumber.Main);
            break;

        case STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID:
        case STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID:
            /* set 4 just to return something consistent, we can't know how FB will be actually allocated */
            return(4);
            break;

        default:
            return (MemProf->NbFrameStore);
    }
}


/*******************************************************************************
Name        : GetNbDecimatedFrameBuffers
Description : Return the number of allocated Decimated frame buffers
Parameters  : MemoryProfile
Assumptions :
Limitations :
Returns     : U8
*******************************************************************************/
static U8 GetNbDecimatedFrameBuffers(const STVID_MemoryProfile_t * const MemProf)
{
    switch (MemProf->NbFrameStore)
    {
        case STVID_OPTIMISED_NB_FRAME_STORE_ID:
            return (MemProf->FrameStoreIDParams.OptimisedNumber.Decimated);
            break;

        case STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID:
        case STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID:
            /* set 4 just to return something consistent, we can't know how FB will be actually allocated */
            return(4);
            break;

        default:
            return (MemProf->NbFrameStore);
    }
}


/*******************************************************************************
Name        : SetNbMainFrameBuffers
Description : Set the number of allocated Main frame buffers
Parameters  : MemoryProfile, U8 Number
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
static void SetNbMainFrameBuffers(STVID_MemoryProfile_t * const MemProf, const U8 NbMainFr)
{
    switch (MemProf->NbFrameStore)
    {
        case STVID_OPTIMISED_NB_FRAME_STORE_ID:
            MemProf->FrameStoreIDParams.OptimisedNumber.Main = NbMainFr;
            break;

        default:
            MemProf->NbFrameStore = NbMainFr;
            break;
    }
}


/*******************************************************************************
Name        : STVID_PauseSynchro
Description :
Parameters  : Video handle, Time
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_PauseSynchro(const STVID_Handle_t VideoHandle,
                                  const STTS_t Time)
{
#ifdef ST_display
    VideoDevice_t * Device_p;
    U32 TimeInPTSUnit, OneVSyncDuration, Tmp32;
    VideoStatus_t Status;
#endif /* ST_display */
#if defined ST_trickmod || defined ST_speed
    S32 Speed = 100;
#endif /*  ST_trickmod || ST_speed*/
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_display
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Get current play speed... */
#ifdef ST_speed
    if ((Device_p->DeviceData_p->SpeedHandleIsValid) && (ErrorCode == ST_NO_ERROR))
    {
        ErrorCode = VIDSPEED_GetSpeed(Device_p->DeviceData_p->SpeedHandle, &Speed);
        if ((ErrorCode == ST_NO_ERROR) && (Speed != 100))
        {
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
#endif /*ST_speed*/
#ifdef ST_trickmod
    if ((Device_p->DeviceData_p->TrickModeHandleIsValid) && (ErrorCode == ST_NO_ERROR))
    {
        ErrorCode = VIDTRICK_GetSpeed(Device_p->DeviceData_p->TrickModeHandle, &Speed);
        if ((ErrorCode == ST_NO_ERROR) && (Speed != 100))
        {
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
#endif /* ST_trickmod*/

    /* Only do something if synchro is off */
#ifdef ST_avsync
    if (VIDAVSYNC_IsSynchronizationEnabled(Device_p->DeviceData_p->AVSyncHandle))
    {
        /* Synchro on or speed not 100: no synchro action can be done ! */
        ErrorCode = STVID_ERROR_NOT_AVAILABLE;
    }
#endif /* ST_avsync*/

    if (ErrorCode == ST_NO_ERROR)
    {
        stvid_EnterCriticalSection(Device_p);
        Status = GET_VIDEO_STATUS(Device_p);
        if (Status == VIDEO_STATUS_RUNNING)
        {
            TimeInPTSUnit = ((U32) Time);
            if (Device_p->DeviceData_p->VTGFrameRate != 0)
            {
                OneVSyncDuration = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000) / Device_p->DeviceData_p->VTGFrameRate);
            }
            else
            {
                OneVSyncDuration = STVID_DEFAULT_VSYNC_DURATION_MICROSECOND_WHEN_NO_VTG * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / 1000000;
            }
            /* Security: OneVSyncDuration should not be 0 because of division... */
            if (OneVSyncDuration != 0)
            {
                /*ErrorCode =*/ VIDDISP_RepeatFields(Device_p->DeviceData_p->DisplayHandle, (TimeInPTSUnit / OneVSyncDuration), &Tmp32);
            }
            else
            {
                /* Could not perform skip because of error: return stupid error code to inform user */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        /* else stopped or pausing or freezing: PauseSynchro has no effect but is not an error: display is already paused ! */
        stvid_LeaveCriticalSection(Device_p);
    } /* no error */
#else /* ST_display */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_display */

#ifdef TRACE_UART
/*    TraceBuffer(("-REPEATSYNCHRO(%d=%d/%d:%d)", (U32) Time, Tmp32, NumberToDo, ErrorCode));*/
#endif /* TRACE_UART */

    return(ErrorCode);
} /* end of STVID_PauseSynchro() function */


/*******************************************************************************
Name        : STVID_Resume
Description : Resume the decoded video
Parameters  : Video handle
Assumptions :
Limitations :
Returns     :ST_ERROR_INVALID_HANDLE,STVID_ERROR_DECODER_NOT_PAUSING,
*******************************************************************************/
ST_ErrorCode_t STVID_Resume(const STVID_Handle_t VideoHandle)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoStatus_t    Status;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if ((Status != VIDEO_STATUS_PAUSING) && (Status != VIDEO_STATUS_FREEZING))
    {
        ErrorCode = STVID_ERROR_DECODER_NOT_PAUSING;
    }
    /* VIDPROD_Resume already returns STVID_ERROR_DECODER_NOT_PAUSING thus, return directly ErrorCode */
    else if (Status == VIDEO_STATUS_FREEZING)
    {
#ifdef ST_producer
        if (Device_p->DeviceData_p->ProducerHandleIsValid)
        {
            ErrorCode = VIDPROD_Resume(Device_p->DeviceData_p->ProducerHandle);
        }
#endif /* ST_producer */
#ifdef ST_diginput
# ifndef ST_MasterDigInput
        if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
        {
            ErrorCode = VIDINP_Resume(Device_p->DeviceData_p->DigitalInputHandle);
        }
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */
    }

    if (ErrorCode == ST_NO_ERROR)
    {
#ifdef ST_display
        ErrorCode = VIDDISP_Resume(Device_p->DeviceData_p->DisplayHandle);
#else /* ST_display */
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_display */
        /* VIDDISP_Resume already returns STVID_ERROR_DECODER_NOT_PAUSING thus, return directly ErrorCode */

        if ((ErrorCode != ST_NO_ERROR) && (Status == VIDEO_STATUS_FREEZING))
        {
#ifdef ST_producer
            if (Device_p->DeviceData_p->ProducerHandleIsValid)
            {
                VIDPROD_Freeze(Device_p->DeviceData_p->ProducerHandle);
            }
#endif /* ST_producer */
#ifdef ST_diginput
# ifndef ST_MasterDigInput
            if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
            {
                VIDINP_Freeze(Device_p->DeviceData_p->DigitalInputHandle);
            }
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */
        }
    }

#ifdef ST_speed
    	/* Enables Trick Mode Speed Control */
    	if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->SpeedHandleIsValid))
    	{
        	/* Enable speed control if not real time */
        	ErrorCode = VIDSPEED_EnableSpeedControl(Device_p->DeviceData_p->SpeedHandle);
        	if (ErrorCode != ST_NO_ERROR)
        	{
            	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        	"STVID_Resume() : Speed enable speed control failed !"));
            	ErrorCode = ST_ERROR_BAD_PARAMETER;
        	}
    	}
#endif /*ST_speed*/
#ifdef ST_trickmod
    	/* Enables Trick Mode Speed Control */
    	if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->TrickModeHandleIsValid))
    	{
        	/* Enable speed control if not real time */
        	ErrorCode = VIDTRICK_EnableSpeedControl(Device_p->DeviceData_p->TrickModeHandle);
        	if (ErrorCode != ST_NO_ERROR)
        	{
            	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        	"STVID_Resume() : TrickMode enable speed control failed !"));
            	ErrorCode = ST_ERROR_BAD_PARAMETER;
        	}
    	}
#endif /* ST_trickmod*/


    if (ErrorCode == ST_NO_ERROR)
    {
        SET_VIDEO_STATUS(Device_p, VIDEO_STATUS_RUNNING);
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Resume() done."));
    }

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_Resume() function */

/*******************************************************************************
Name        : STVID_SetDataInputInterface
Description : fct used to autorize the video to controle injection if done
                by software. cascaded to decode module
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetDataInputInterface(const STVID_Handle_t VideoHandle,
     ST_ErrorCode_t (*GetWriteAddress)  (void * const ExtHandle, void ** const Address_p),
     void (*InformReadAddress)(void * const ExtHandle, void * const Address),
     void * const ExtHandle )
{
#if !defined(ST_51xx_Device) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_7109) && !defined(ST_7200)
    return(ST_ERROR_FEATURE_NOT_SUPPORTED); /* inj controlled by hardware */
#else
    ST_ErrorCode_t ErrorCode;
    VideoDevice_t * Device_p;
    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        ErrorCode = VIDPROD_SetDataInputInterface(
                            Device_p->DeviceData_p->ProducerHandle,
                            GetWriteAddress,InformReadAddress,ExtHandle );
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    return(ErrorCode);

#endif
}


/*******************************************************************************
Name        : STVID_SetDecodedPictures
Description : Set the picture coding type to decode.
Parameters  : Video handle, pointer to fill.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_SetDecodedPictures(const STVID_Handle_t VideoHandle, const STVID_DecodedPictures_t DecodedPictures)
{
    VideoDevice_t * Device_p;
#ifdef ST_producer
    VIDCOM_InternalProfile_t  CommonMemoryProfile;
    VIDBUFF_StrategyForTheBuffersFreeing_t StrategyForTheBuffersFreeing;
    U8 MinBufferDecodeI, MinBufferDecodeIP, MinBufferDecodeAll, FrameBufferHoldTime = 1;
#endif /* ST_producer */

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) VideoHandle)->Device_p;

#ifdef ST_diginput
# ifdef ST_MasterDigInput
    if ((Device_p->DeviceData_p->DigitalInputIsValid) && (DecodedPictures != STVID_DECODED_PICTURES_ALL))
# else
    if ((Device_p->DeviceData_p->DigitalInputHandleIsValid) && (DecodedPictures != STVID_DECODED_PICTURES_ALL))
# endif /* ST_MasterDigInput */
    {
        /* Digital input : only ALL pictures allowed */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif /* ST_diginput */
#ifdef ST_producer
    stvid_EnterCriticalSection(Device_p);

    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &CommonMemoryProfile);
        if (ErrorCode != ST_NO_ERROR)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_GetMemoryProfile failed"));
        }
        else
        {
#ifdef ST_display
            VIDDISP_GetFrameBufferHoldTime(Device_p->DeviceData_p->DisplayHandle, &FrameBufferHoldTime);
#endif /* ST_display */
            GetMinimalNumberOfBuffers(Device_p->DeviceData_p->DeviceType,
                                      FrameBufferHoldTime,
                                      &MinBufferDecodeI,
                                      &MinBufferDecodeIP,
                                      &MinBufferDecodeAll);

            if (((CommonMemoryProfile.NbMainFrameStore < MinBufferDecodeAll) && (DecodedPictures == STVID_DECODED_PICTURES_ALL)) ||
                ((CommonMemoryProfile.NbMainFrameStore < MinBufferDecodeIP) && (DecodedPictures == STVID_DECODED_PICTURES_IP)) ||
                ((CommonMemoryProfile.NbMainFrameStore < MinBufferDecodeI) && (DecodedPictures == STVID_DECODED_PICTURES_I)))
            {
                ErrorCode = STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Parameter incompatible with the current Memory Profile."));
                /* Propose an updated number of frame buffers */
                if (DecodedPictures == STVID_DECODED_PICTURES_ALL)
                {
                    SetNbMainFrameBuffers(&(CommonMemoryProfile.ApiProfile), MinBufferDecodeAll);
                    CommonMemoryProfile.NbMainFrameStore = MinBufferDecodeAll;
                }
                else if (DecodedPictures == STVID_DECODED_PICTURES_IP)
                {
                    SetNbMainFrameBuffers(&(CommonMemoryProfile.ApiProfile), MinBufferDecodeIP);
                    CommonMemoryProfile.NbMainFrameStore = MinBufferDecodeIP;
                }
                else
                {
                    SetNbMainFrameBuffers(&(CommonMemoryProfile.ApiProfile), MinBufferDecodeI);
                    CommonMemoryProfile.NbMainFrameStore = MinBufferDecodeI;
                }
                /* Notify the event. */
                STEVT_Notify(Device_p->DeviceData_p->EventsHandle,
                        Device_p->DeviceData_p->EventID[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID],
                        STEVT_EVENT_DATA_TYPE_CAST &CommonMemoryProfile.ApiProfile);
            }
            else
            {
            if (IsTrickModeHandleOrSpeedHandleValid(Device_p))
            {
#ifdef ST_speed
                if (Device_p->DeviceData_p->SpeedHandleIsValid)
                {
                    if  (DecodedPictures != STVID_DECODED_PICTURES_ALL)
                    {
                        VIDSPEED_DisplayManagedBySpeed(Device_p->DeviceData_p->SpeedHandle, TRUE);
                        /*VIDQUEUE_CancelAllPictures(Device_p->DeviceData_p->OrderingQueueHandle);*/
                        /* Frees all not displayed pictures */
                        /*VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(Device_p->DeviceData_p->DisplayHandle);*/
                    }
                }
#endif /*ST_speed*/
#ifdef ST_trickmod
                if (Device_p->DeviceData_p->TrickModeHandleIsValid)
                {
                    ErrorCode = VIDTRICK_SetDecodedPictures(Device_p->DeviceData_p->TrickModeHandle, DecodedPictures);
                }
#endif /* ST_trickmod */
            }
                ErrorCode = VIDPROD_SetDecodedPictures(Device_p->DeviceData_p->ProducerHandle, DecodedPictures);
                if (ErrorCode != ST_NO_ERROR)
                {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_SetDecodedPictures failed"));
                }
                else
                {
                    /* Ensure nobody else accesses display params */
                    semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

#ifdef ST_display
                    /* Set successful: update display params eventually */
                    if (DecodedPictures == STVID_DECODED_PICTURES_ALL)
                    {
                        Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
                    }
                    else
                    {
                        Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
                    }
#endif /* ST_display */
                    /* Check if there some change and send to VIDDISP. */
                    CheckDisplayParamsAndUpdate(Device_p);

                    /* Define the buffers freeing strategy. */
                    AffectTheBuffersFreeingStrategy(
                            Device_p->DeviceData_p->DeviceType,
                            CommonMemoryProfile.NbMainFrameStore,
                            DecodedPictures,
                            &StrategyForTheBuffersFreeing);

                    /* Inform buffer which needs to know it to choose the buffers freeing strategy. */
                    ErrorCode = VIDBUFF_SetStrategyForTheBuffersFreeing(Device_p->DeviceData_p->BuffersHandle, &StrategyForTheBuffersFreeing);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_SetStrategyForTheBuffersFreeing failed"));
                    }

                    /* Release access to display params */
                    semaphore_signal(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

#ifdef ST_ordqueue
                    if (DecodedPictures != STVID_DECODED_PICTURES_ALL)
                    {
                        VIDQUEUE_PushAllPicturesToDisplay(Device_p->DeviceData_p->OrderingQueueHandle);
                    }
#endif /* ST_ordqueue */
                    if (DecodedPictures == STVID_DECODED_PICTURES_FIRST_I)
                    {
#ifdef ST_ordqueue
                        VIDQUEUE_CancelAllPictures(Device_p->DeviceData_p->OrderingQueueHandle);
#endif /* ST_ordqueue */
#ifdef ST_display
                        VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(Device_p->DeviceData_p->DisplayHandle);
#endif /* ST_display */
                    }

                } /* end set DecodedPictures successful */
#ifdef ST_speed
    if (Device_p->DeviceData_p->SpeedHandleIsValid)
    {
        if  (DecodedPictures == STVID_DECODED_PICTURES_ALL)
        {   /* ask to start the decode on next I picture */
            VIDPROD_SkipData(Device_p->DeviceData_p->ProducerHandle, VIDPROD_SKIP_NO_PICTURE);
            VIDPROD_SkipData(Device_p->DeviceData_p->ProducerHandle,  VIDPROD_SKIP_UNTIL_NEXT_SEQUENCE);
        }
    }
#endif /*ST_speed*/

            } /* end memory profile OK */
        } /* end get memory profile successful */
    } /* end MPEG decoder */
    else
    {
        if (DecodedPictures != STVID_DECODED_PICTURES_ALL)
        {
            /* Only decode module can change decoded pictures */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
    }

    stvid_LeaveCriticalSection(Device_p);
#else /* ST_producer */
    if (DecodedPictures != STVID_DECODED_PICTURES_ALL)
    {
        /* Only decode module can change decoded pictures */
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
#endif /* not ST_producer */
    return(ErrorCode);
} /* End of STVID_SetDecodedPictures() function */

#ifdef STVID_STVAPI_ARCHITECTURE
/*******************************************************************************
Name        : STVID_SetExternalRasterBufferManagerPoolId
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetExternalRasterBufferManagerPoolId(const STVID_Handle_t VideoHandle, const DVRBM_BufferPoolId_t BufferPoolId)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        VIDPROD_SetExternalRasterBufferManagerPoolId(Device_p->DeviceData_p->ProducerHandle, BufferPoolId);
    }
#endif /* ST_producer */

    stvid_LeaveCriticalSection(Device_p);

    return(ST_NO_ERROR);
} /* End of STVID_SetExternalRasterBufferManagerPoolId() function */


/*******************************************************************************
Name        : STVID_SetRasterReconstructionMode
Description : this function provides the enables/disables the main raster reconstruction and
              sets the provided decimation, if any.
Parameters  :
Assumptions : the call of STVID_SetRasterReconstructionMode(VideoHandle, FALSE, STGVOBJ_kDecimationFactorNone); has no sense
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetRasterReconstructionMode(const STVID_Handle_t VideoHandle, const BOOL EnableMainRasterRecontruction, const STGVOBJ_DecimationFactor_t Decimation)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        ErrorCode = VIDPROD_SetRasterReconstructionMode(Device_p->DeviceData_p->ProducerHandle, EnableMainRasterRecontruction, Decimation);
    }
#endif /* ST_producer */

    stvid_LeaveCriticalSection(Device_p);

    return (ErrorCode);
}
#endif /* end of def STVID_STVAPI_ARCHITECTURE */


/*******************************************************************************
Name        : STVID_SetHDPIPParams
Description : Set HD-PIP parameters to decoder.
Parameters  : Video handle, HD-PIP parameters to set.
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
              ST_ERROR_FEATURE_NOT_SUPPORTED.
*******************************************************************************/
ST_ErrorCode_t STVID_SetHDPIPParams(const STVID_Handle_t Handle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    VideoDevice_t *             Device_p;
#ifdef ST_producer
    VIDCOM_InternalProfile_t    CommonMemoryProfile;
    STVID_HDPIPParams_t         PreviousHDPIPParams;
#endif /* ST_producer */
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Exit now if parameters are bad */
    if (HDPIPParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)Handle)->Device_p;

    /* Test if device type is correct.  */
    if ( (Device_p->DeviceData_p->DeviceType  != STVID_DEVICE_TYPE_7020_MPEG) ||
         ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7020_MPEG) &&
         ((Device_p->DecoderNumber != 0) && (Device_p->DecoderNumber != 1))) )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "HDPIP only accessible for OMEGA2 chips, decoder 1 or 2"));
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

#ifdef ST_producer
    /* Get Current memory profile. HDPIP can't be enabled if decimation is used.    */
    ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &CommonMemoryProfile);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_GetMemoryProfile failed"));
        return(ST_ERROR_BAD_PARAMETER);
    }
    else
    {
        if ( (CommonMemoryProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE)
             && (HDPIPParams_p->Enable) )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "HDPIP not compatible with decimation"));
            return(ST_ERROR_BAD_PARAMETER);
        }
    }

    stvid_EnterCriticalSection(Device_p);

    /* Get current HDPIP parameters (in case of error).         */
    ErrorCode = VIDPROD_GetHDPIPParams(Device_p->DeviceData_p->ProducerHandle,
            &PreviousHDPIPParams);

    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = VIDPROD_SetHDPIPParams(Device_p->DeviceData_p->ProducerHandle,
                HDPIPParams_p);

        if (ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = VIDBUFF_SetHDPIPParams(Device_p->DeviceData_p->BuffersHandle,
                    HDPIPParams_p);
        }

        /* Retrieve good state even if error is reported.   */
        if (ErrorCode != ST_NO_ERROR)
        {
            VIDPROD_SetHDPIPParams(Device_p->DeviceData_p->ProducerHandle,
                    &PreviousHDPIPParams);
        }
    }

    stvid_LeaveCriticalSection(Device_p);
#else /* ST_producer */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* ST_producer */

    return(ErrorCode);

} /* End of STVID_SetHDPIPParams() function. */



/*******************************************************************************
Name        : STVID_SetMemoryProfile
Description : Set the memory profile for the decode/display process.
Parameters  : Video handle, memory profile to take into account.
Assumptions : The video decoder linked to this buffer handle should be stopped.
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER  if parameters are invalid.
              ST_ERROR_INVALID_HANDLE if the handle is invalid, STVID_ERROR_RUNNING, STVID_ERROR_PAUSING, ST_NO_ERROR
              ST_ERROR_FEATURE_NOT_SUPPORTED if at least one parameter is not supported by the chip.
*******************************************************************************/
ST_ErrorCode_t STVID_SetMemoryProfile(const STVID_Handle_t VideoHandle,
                                      const STVID_MemoryProfile_t * const MemoryProfile_p)
{
    VideoDevice_t * Device_p;
    VideoStatus_t Status;
    VIDCOM_InternalProfile_t CommonMemoryProfile;
#ifdef ST_producer
    STVID_DecodedPictures_t DecodedPictures;
    VIDBUFF_StrategyForTheBuffersFreeing_t StrategyForTheBuffersFreeing;
    STVID_HDPIPParams_t     HDPIPParams;
    U8 FrameBufferHoldTime = 0;
#endif /* ST_producer */
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (MemoryProfile_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Check for Memory profile parameters validity... */
    if ((MemoryProfile_p->MaxWidth == 0) || (MemoryProfile_p->MaxHeight == 0))
    {
        /* The wanted frame buffer size isn't available. */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check for Optimised Frame Buffers Numbers validity... */
    switch (MemoryProfile_p->NbFrameStore)
    {
        case STVID_OPTIMISED_NB_FRAME_STORE_ID:
            if (MemoryProfile_p->FrameStoreIDParams.OptimisedNumber.Main == 0)
            {
                /* The wanted Main frame buffer number is invalid */
                return(ST_ERROR_BAD_PARAMETER);
            }
            if (MemoryProfile_p->DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
            {
                if (MemoryProfile_p->FrameStoreIDParams.OptimisedNumber.Decimated == 0)
                {
                    /* The wanted Decimated frame buffer number is invalid */
                    return(ST_ERROR_BAD_PARAMETER);
                }
            }
            break;

        case STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID:
            if ( (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_H264) &&
                 (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_H264) &&
                 (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7200_H264) )
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            else
            {
                if (MemoryProfile_p->FrameStoreIDParams.VariableInFixedSize.Main == 0)
                {
                    /* The wanted Main frame buffer number is invalid */
                    return(ST_ERROR_BAD_PARAMETER);
                }
                if (MemoryProfile_p->DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                {
                    if ((MemoryProfile_p->DecimationFactor & STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2) == STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2)
                    {
                        /* The adaptative decimation is not compatible (and useless) with the dynamic frame buffer allocation */
                        return(ST_ERROR_BAD_PARAMETER);
                    }
                    if (MemoryProfile_p->FrameStoreIDParams.VariableInFixedSize.Decimated == 0)
                    {
                        /* The wanted Decimated frame buffer number is invalid */
                        return(ST_ERROR_BAD_PARAMETER);
                    }
                }
            }
            break;

        case STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID:
            if ( (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_H264) &&
                 (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_H264) &&
                 (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7200_H264) )
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            else
            {
                if (MemoryProfile_p->FrameStoreIDParams.VariableInFullPartition.Main == (STAVMEM_PartitionHandle_t) NULL)
                {
                    /* The wanted Main frame buffer partition handle is invalid */
                    return(ST_ERROR_BAD_PARAMETER);
                }
                ErrorCode = VIDBUFF_SetAvmemPartitionForFrameBuffers(Device_p->DeviceData_p->BuffersHandle, MemoryProfile_p->FrameStoreIDParams.VariableInFullPartition.Main);
                if (ErrorCode != ST_NO_ERROR)
                {
                    return(ErrorCode);
                }
                if (MemoryProfile_p->DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                {
                    if ((MemoryProfile_p->DecimationFactor & STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2) == STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2)
                    {
                        /* The adaptative decimation is not compatible (and useless) with the dynamic frame buffer allocation */
                        return(ST_ERROR_BAD_PARAMETER);
                    }
                    if (MemoryProfile_p->FrameStoreIDParams.VariableInFullPartition.Decimated == (STAVMEM_PartitionHandle_t) NULL)
                    {
                        /* The wanted Decimated frame buffer partition handle  is invalid */
                        return(ST_ERROR_BAD_PARAMETER);
                    }
                    /* Decimated partition setting is deferred after decimation factor setting */
                }
            }
            break;

        case 0:
            /* The wanted frame buffer size isn't available. */
            return(ST_ERROR_BAD_PARAMETER);
            break;

       default:
            break;
    }

    if (MemoryProfile_p->NbFrameStore == STVID_OPTIMISED_NB_FRAME_STORE_ID)
    {
        if (MemoryProfile_p->FrameStoreIDParams.OptimisedNumber.Main == 0)
        {
            /* The wanted Main frame buffer number is invalid */
            return(ST_ERROR_BAD_PARAMETER);
        }
        if (MemoryProfile_p->DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
        {
            if (MemoryProfile_p->FrameStoreIDParams.OptimisedNumber.Decimated == 0)
            {
                /* The wanted Decimated frame buffer number is invalid */
                return(ST_ERROR_BAD_PARAMETER);
            }
        }
    }

    switch (MemoryProfile_p->CompressionLevel)
    {
        case STVID_COMPRESSION_LEVEL_NONE:
            /* No restriction. This is its default value. */
            break;
        case STVID_COMPRESSION_LEVEL_1:
        case STVID_COMPRESSION_LEVEL_2:
            /* In fact, all other compression levels !!! */

            /* Some chips don't support these compression levels. */
            if ( (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7015_MPEG) &&
                 (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7020_MPEG) )
            {
                /* This feature is not supported by this chip. */
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            break;
        default :
            /* Value not available (out of range). */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    switch (MemoryProfile_p->DecimationFactor&~STVID_MPEG_DEBLOCK_RECONSTRUCTION)
    {
        case STVID_DECIMATION_FACTOR_NONE:
            /* No restriction. This is its default value. */
            break;

        case STVID_DECIMATION_FACTOR_H2:
        case STVID_DECIMATION_FACTOR_V2:
        case STVID_DECIMATION_FACTOR_H4:
        case STVID_DECIMATION_FACTOR_V4:
        case STVID_DECIMATION_FACTOR_2:
        case STVID_DECIMATION_FACTOR_4:
        case STVID_DECIMATION_FACTOR_H4 | STVID_DECIMATION_FACTOR_V2:
        case STVID_DECIMATION_FACTOR_V4 | STVID_DECIMATION_FACTOR_H2:
        case STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2 :
        case STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2 | STVID_DECIMATION_FACTOR_V2:
        case STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2 | STVID_DECIMATION_FACTOR_V4:
            /* In fact, all other decimation factor !!! */
            /* Some chips don't support these decimation factors. */
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_7015_MPEG :
                case STVID_DEVICE_TYPE_7020_MPEG :
#ifdef ST_producer
                    /* Decimation is required. Check if HD-PIP is already activated.    */
                    ErrorCode = VIDPROD_GetHDPIPParams(Device_p->DeviceData_p->ProducerHandle,
                            &HDPIPParams);

                    if ( (ErrorCode != ST_NO_ERROR) || ((ErrorCode == ST_NO_ERROR) && (HDPIPParams.Enable)) )
                    {
                        /* Can't activate decimation if HD-PIP is already activated.    */
                        return(ST_ERROR_BAD_PARAMETER);
                    }
#endif /* ST_producer */
                    break;

                case STVID_DEVICE_TYPE_7200_H264 :
#ifdef ST_XVP_ENABLE_FGT
                    if (VIDFGT_IsActivated(Device_p->DeviceData_p->FGTHandle) == TRUE)
                    {
                        /* because of FGT use decimated buffers, FGT is forbidden when
                           decimation is enabled : force FGT termination*/
                        VIDFGT_Deactivate(Device_p->DeviceData_p->FGTHandle);
                        if (Device_p->DeviceData_p->ViewportProperties_p)
                        {
                            STLAYER_XVPTerm(Device_p->DeviceData_p->ViewportProperties_p->Identity.LayerViewPortHandle);
                        }
                    }
#endif /* ST_XVP_ENABLE_FGT */

                case STVID_DEVICE_TYPE_7100_H264 :
                case STVID_DEVICE_TYPE_7109_H264 :
                    if ((MemoryProfile_p->DecimationFactor & STVID_DECIMATION_FACTOR_V4) == STVID_DECIMATION_FACTOR_V4)
                    {
                        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                    }
                    break;

                case STVID_DEVICE_TYPE_5528_MPEG :
                case STVID_DEVICE_TYPE_7710_MPEG :
                case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7100_MPEG :
                case STVID_DEVICE_TYPE_7109_MPEG :
                case STVID_DEVICE_TYPE_7109_VC1 :
                case STVID_DEVICE_TYPE_7109D_MPEG :
                case STVID_DEVICE_TYPE_7200_MPEG :
                case STVID_DEVICE_TYPE_7200_VC1 :
                case STVID_DEVICE_TYPE_7200_MPEG4P2 :
                case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
#ifdef STVID_STVAPI_ARCHITECTURE
                case STVID_DEVICE_TYPE_STD2000_MPEG  :
#endif /* STVID_STVAPI_ARCHITECTURE */
                    /* Decimation is supported. Nothing special to do.                  */
                    break;

                case STVID_DEVICE_TYPE_7100_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_MPEG4P2 :
                case STVID_DEVICE_TYPE_7109_AVS :
                default :
                    /* This feature is not supported by all other chips.                */
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                    break;
            }
            break;

		case STVID_POSTPROCESS_RECONSTRUCTION:
            /* Nothing special to do. */
            break;

        default :
            /* Value not available (out of range). */
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }
    if ((MemoryProfile_p->DecimationFactor & STVID_MPEG_DEBLOCK_RECONSTRUCTION)
        &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG)
        &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (Status != VIDEO_STATUS_STOPPED)
    {
        ErrorCode = (Status == VIDEO_STATUS_RUNNING ?
                     STVID_ERROR_DECODER_RUNNING : STVID_ERROR_DECODER_PAUSING);
    }
    else
    {
#ifdef ST_producer
        /* According of number of buffers, eventually reduce decoded pictures if MPEG decoder */
#ifdef ST_SecondInstanceSharingTheSameDecoder
        if ((Device_p->DeviceData_p->ProducerHandleIsValid)
                &&(!Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder))
#else
        if (Device_p->DeviceData_p->ProducerHandleIsValid)
#endif /* ST_SecondInstanceSharingTheSameDecoder */
        {
            ErrorCode = VIDPROD_GetDecodedPictures(Device_p->DeviceData_p->ProducerHandle, &DecodedPictures);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Cannot know DecodedPictures ! */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetDecodedPictures() : failed !"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
#ifdef ST_display
                VIDDISP_GetFrameBufferHoldTime(Device_p->DeviceData_p->DisplayHandle, &FrameBufferHoldTime);
#endif /* ST_display */

                /* If not enough buffer to decode all kind of pictures, set the suitable Decoded pictures */
                ErrorCode = AffectDecodedPictures(Device_p,
                                                  MemoryProfile_p,
                                                  FrameBufferHoldTime,
                                                  &DecodedPictures);
                if(ErrorCode != ST_NO_ERROR)
                {
                    /* Cannot affect the decoded pictures ! */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetDecodedPictures() : failed !"));
                }
                else
                {
                    /* Ensure nobody else accesses display params */
                    semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

#ifdef ST_display
                    /* Update the display according to the new decoded pictures */
                    switch(DecodedPictures)
                    {
                        case STVID_DECODED_PICTURES_I:
                        case STVID_DECODED_PICTURES_IP:
                        case STVID_DECODED_PICTURES_FIRST_I :

                        /* Set successful: update display params eventually , ensure that in 2 buffers the start is AS_AVAILABLE */
                        Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;

                        case STVID_DECODED_PICTURES_ALL:
                        /* Enough buffers and more is decode all: set display to CARE */
                        Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
                    }
#endif /* ST_display */

                    /* Check if there is some change and send to VIDDISP. */
                    CheckDisplayParamsAndUpdate(Device_p);
                    /* Release access to display params */
                    semaphore_signal(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);
                }
            }
        } /* end decoder handle valid */
#endif /* ST_producer */
#ifdef ST_diginput
# ifdef ST_MasterDigInput
        if (Device_p->DeviceData_p->DigitalInputIsValid)
# else
        if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
# endif /* ST_MasterDigInput */
        {
            /* Can't allow running with less than the min number    */
            /* of buffer for Digital input.                         */
            if (GetNbMainFrameBuffers(MemoryProfile_p) < STVID_MIN_BUFFER_DIGINPUT)
            {
                stvid_LeaveCriticalSection(Device_p);
                return(ST_ERROR_BAD_PARAMETER);
            }
        }
#endif /* ST_diginput */

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Profile OK and set in buffer manager: store  */
            CommonMemoryProfile.ApiProfile= (*MemoryProfile_p);

            /* Setting the number of Main Frame Buffers */
            CommonMemoryProfile.NbMainFrameStore = GetNbMainFrameBuffers(MemoryProfile_p);

            /* Secondary buffers are created only if decimation is required */
#ifndef STVID_STVAPI_ARCHITECTURE
            CommonMemoryProfile.NbSecondaryFrameStore  =
                (((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
                (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
                (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2) ||
                (MemoryProfile_p->DecimationFactor != STVID_DECIMATION_FACTOR_NONE))
                ? GetNbDecimatedFrameBuffers(MemoryProfile_p) : 0);
#else /* STVID_STVAPI_ARCHITECTURE */
            CommonMemoryProfile.NbSecondaryFrameStore  = 1;
#endif /* not STVID_STVAPI_ARCHITECTURE */

            /* Set ApplicableDecimationFactor to the default value */
            CommonMemoryProfile.ApplicableDecimationFactor = CommonMemoryProfile.ApiProfile.DecimationFactor;

            /* Set profile in buffer manager */
            ErrorCode = VIDBUFF_SetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &CommonMemoryProfile);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* VIDBUFF refused profile, so refuse to set profile */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_SetMemoryProfile failed"));
            }
            else
            {
                /* change decimated buffers partition for STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID mode now as we set decimation factors as requested */
                if ((MemoryProfile_p->NbFrameStore == STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID) && ((MemoryProfile_p->DecimationFactor != STVID_DECIMATION_FACTOR_NONE)))
                {
                    ErrorCode = VIDBUFF_SetAvmemPartitionForDecimatedFrameBuffers(Device_p->DeviceData_p->BuffersHandle, MemoryProfile_p->FrameStoreIDParams.VariableInFullPartition.Decimated);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return(ErrorCode);
                    }
                }
#ifdef ST_producer
                if (Device_p->DeviceData_p->ProducerHandleIsValid)
                {
                    AffectTheBuffersFreeingStrategy(
                            Device_p->DeviceData_p->DeviceType,
                            CommonMemoryProfile.NbMainFrameStore,
                            DecodedPictures,
                            &StrategyForTheBuffersFreeing);

                    /* Inform buffer which needs to know it to choose the buffers freeing strategy. */
                    ErrorCode = VIDBUFF_SetStrategyForTheBuffersFreeing(Device_p->DeviceData_p->BuffersHandle,
                            &StrategyForTheBuffersFreeing);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_SetStrategyForTheBuffersFreeing failed"));
                    }
                }
#endif /* ST_producer */
#ifdef ST_diginput
# ifdef ST_MasterDigInput
                if (Device_p->DeviceData_p->DigitalInputIsValid)
# else
                if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
# endif
                {
                    StrategyForTheBuffersFreeing.IsTheBuffersFreeingOptimized = FALSE;

                    /* Inform buffer which needs to know it to choose the buffers freeing strategy. */
                    ErrorCode = VIDBUFF_SetStrategyForTheBuffersFreeing(Device_p->DeviceData_p->BuffersHandle,
                            &StrategyForTheBuffersFreeing);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_SetStrategyForTheBuffersFreeing failed"));
                    }
                }
#endif /* ST_diginput */
            }
        } /* Profile OK to be set in VIDBUFF */
    } /* end video stopped */

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);

} /* End of STVID_SetMemoryProfile() function */

/*******************************************************************************
Name        : STVID_ForceDecimationFactor
Description : Changes the decimation factor while driver is not stopped
Parameters  : Video handle, forcing params
Assumptions : 
Limitations :
Returns     : ST_NO_ERROR               if success,
              ST_ERROR_BAD_PARAMETER    if the params are invalid
              ST_ERROR_INVALID_HANDLE   if the Video handle is invalid. 
              STVID_ERROR_NOT_AVAILABLE if previous force hasn't yet been
                                        satisfied
*******************************************************************************/
ST_ErrorCode_t STVID_ForceDecimationFactor(const STVID_Handle_t VideoHandle, 
                                           const STVID_ForceDecimationFactorParams_t * const Params_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDCOM_InternalProfile_t CommonMemoryProfile;

    /* Exit now if parameters are bad */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_producer
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Get current memory profile  */
    stvid_EnterCriticalSection(Device_p);

    /* Don't allow forcing if previous forcing is still on-going */
    if(Device_p->DeviceData_p->ForceDecimationFactor.State == FORCE_PENDING)
    {
        stvid_LeaveCriticalSection(Device_p);
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &CommonMemoryProfile);

    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_GetMemoryProfile failed"));
    }
    else
    {
        /* Check validity of the force mode */
        if((Params_p->ForceMode == STVID_FORCE_MODE_ENABLED)&&
           ((CommonMemoryProfile.ApiProfile.DecimationFactor == STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2)||
            ((CommonMemoryProfile.ApiProfile.DecimationFactor & STVID_DECIMATION_FACTOR_V4)&&
             (Params_p->DecimationFactor & STVID_DECIMATION_FACTOR_V2))||
            ((CommonMemoryProfile.ApiProfile.DecimationFactor & STVID_DECIMATION_FACTOR_H4)&&
             (Params_p->DecimationFactor & STVID_DECIMATION_FACTOR_H2))||
            (Params_p->DecimationFactor == STVID_DECIMATION_FACTOR_NONE)||
            (Params_p->DecimationFactor == STVID_MPEG_DEBLOCK_RECONSTRUCTION)||
            (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)||
            (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)||
            (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2)))
        {
            stvid_LeaveCriticalSection(Device_p);
            /* Exist if parameters are bad */
            return(ST_ERROR_BAD_PARAMETER);
        }
        /* Remember last decimation factor before forcing a new factor */
        if(Device_p->DeviceData_p->ForceDecimationFactor.State == FORCE_ONGOING)
        {
            Device_p->DeviceData_p->ForceDecimationFactor.LastDecimationFactor = Device_p->DeviceData_p->ForceDecimationFactor.DecimationFactor;
        }
        else
        {
            Device_p->DeviceData_p->ForceDecimationFactor.LastDecimationFactor = CommonMemoryProfile.ApiProfile.DecimationFactor;
        }
        /* Set the new decimation factor to be forced */
        switch(Params_p->ForceMode)
        {
            case STVID_FORCE_MODE_ENABLED:
                Device_p->DeviceData_p->ForceDecimationFactor.DecimationFactor = Params_p->DecimationFactor;
                break;
            default:
            case STVID_FORCE_MODE_DISABLED:
                Device_p->DeviceData_p->ForceDecimationFactor.DecimationFactor = CommonMemoryProfile.ApiProfile.DecimationFactor;
                break;
        }
        /* Remember parameters to force decimation when possible, to avoid
         * display artifacts */
        Device_p->DeviceData_p->ForceDecimationFactor.State = FORCE_PENDING;
    }
    stvid_LeaveCriticalSection(Device_p);
#endif /* ST_producer */

    return(ErrorCode);
} /* End of STVID_ForceDecimationFactor() function */

/*******************************************************************************
Name        : STVID_GetDecimationFactor
Description : Returns the currently appliable decimation factor
Parameters  : Video handle, DecimationFactor variable
Assumptions : 
Limitations :
Returns     : ST_NO_ERROR             if success,
              ST_ERROR_BAD_PARAMETER  if the return variable is NULL
*******************************************************************************/
ST_ErrorCode_t STVID_GetDecimationFactor(const STVID_Handle_t VideoHandle, 
                                         STVID_DecimationFactor_t * const DecimationFactor_p)
{
    VideoDevice_t * Device_p;
    VIDCOM_InternalProfile_t CommonMemoryProfile;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (DecimationFactor_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Fill in with default return value */
    (*DecimationFactor_p) = STVID_DECIMATION_FACTOR_NONE;
#ifdef ST_producer
    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Get current memory profile and return the applicable decimation factor */
    stvid_EnterCriticalSection(Device_p);
    ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &CommonMemoryProfile);
    if (ErrorCode != ST_NO_ERROR)
    {
        stvid_LeaveCriticalSection(Device_p);
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_GetMemoryProfile failed"));
        return(ST_ERROR_BAD_PARAMETER);
    }
    (*DecimationFactor_p) = CommonMemoryProfile.ApplicableDecimationFactor;
    stvid_LeaveCriticalSection(Device_p);
#endif /* ST_producer */

    return(ErrorCode);
} /* End of STVID_GetDecimationFactor() function */

/*******************************************************************************
Name        : STVID_EnableDeblocking
Description : Enable the deblocking
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR             if success,
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE
              ST_ERROR_INVALID_HANDLE if the Video handle is invalid.
*******************************************************************************/
ST_ErrorCode_t STVID_EnableDeblocking(const STVID_Handle_t VideoHandle)
{
#ifdef ST_deblock
    VideoDevice_t *         Device_p;
    STVID_MemoryProfile_t   MemoryProfile;
    VideoStatus_t           Status;
    BOOL                    DeblockingEnabled = TRUE;
    ST_ErrorCode_t          ErrorCode= ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Deblocking is only possible on 7100 and 7109 */
    if((Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG)
            &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    STVID_GetMemoryProfile( VideoHandle, &MemoryProfile);

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (Status != VIDEO_STATUS_STOPPED)
    {
        ErrorCode = (Status == VIDEO_STATUS_RUNNING ?
                     STVID_ERROR_DECODER_RUNNING : STVID_ERROR_DECODER_PAUSING);
    }
    else
    {
    /* verify that memory profile is correctly set:
    * decimation factor STVID_MPEG_DEBLOCK_RECONSTRUCTION +
    * at least 4 main buffers + 4 secondary buffers  */
        if ((MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main < STVID_MIN_BUFFER_DECODE_AND_DEBLOCK)
            ||(MemoryProfile.FrameStoreIDParams.OptimisedNumber.Decimated < STVID_MIN_BUFFER_DECODE_AND_DEBLOCK)
            ||(!(MemoryProfile.DecimationFactor & STVID_MPEG_DEBLOCK_RECONSTRUCTION)))
        {
            ErrorCode = STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE;
        }
        else
        {
            VIDPROD_SetDeblockingMode(Device_p->DeviceData_p->ProducerHandle, DeblockingEnabled);
            ErrorCode = ST_NO_ERROR;
        }
    }
    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);

#else /* ST_deblock */
    UNUSED_PARAMETER(VideoHandle);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

#endif /* ST_deblock */
} /* End of STVID_EnableDeblocking() function */

#ifdef VIDEO_DEBLOCK_DEBUG
/*******************************************************************************
Name        : STVID_SetDeblockingStrength
Description : Change the deblocking Strength
Parameters  : Video handle, DeblockingStrength from 0(deblocking disabled) to 50
Assumptions : Deblocking enabled
Limitations :
Returns     : ST_NO_ERROR             if success,
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE
              ST_ERROR_INVALID_HANDLE if the Video handle is invalid.
*******************************************************************************/
ST_ErrorCode_t STVID_SetDeblockingStrength(const STVID_Handle_t VideoHandle, const U8 DeblockingStrength)
{
#ifdef ST_deblock
    VideoDevice_t *         Device_p;
    STVID_MemoryProfile_t   MemoryProfile;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Deblocking is only possible on 7100 and 7109 */
    if((Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG)
            &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    STVID_GetMemoryProfile( VideoHandle, &MemoryProfile);

    /* verify that memory profile is correctly set:
    * decimation factor STVID_MPEG_DEBLOCK_RECONSTRUCTION +
    * at least 4 main buffers + 4 secondary buffers  */
    if ((MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main < STVID_MIN_BUFFER_DECODE_AND_DEBLOCK)
        ||(MemoryProfile.FrameStoreIDParams.OptimisedNumber.Decimated < STVID_MIN_BUFFER_DECODE_AND_DEBLOCK)
        ||(MemoryProfile.DecimationFactor != STVID_MPEG_DEBLOCK_RECONSTRUCTION))
    {
        ErrorCode = STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE;
    }
    else
    {
        if ((DeblockingStrength > STVID_MPEG2_DEBLOCKING_STRENGTH_MAX)
                ||(DeblockingStrength < STVID_MPEG2_DEBLOCKING_STRENGTH_MIN))
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        stvid_EnterCriticalSection(Device_p);

        VIDPROD_SetDeblockingStrength(Device_p->DeviceData_p->ProducerHandle, (int)DeblockingStrength);
        ErrorCode = ST_NO_ERROR;

        stvid_LeaveCriticalSection(Device_p);
    }

    return(ErrorCode);

#else /* ST_deblock */
    UNUSED_PARAMETER(VideoHandle);
    UNUSED_PARAMETER(DeblockingStrength);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

#endif /* ST_deblock */
} /* End of STVID_SetDeblockingStrength() function */
#endif /*VIDEO_DEBLOCK_DEBUG */

/*******************************************************************************
Name        : STVID_DisableDeblocking
Description : Disable the deblocking
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR             if success,
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE
              ST_ERROR_INVALID_HANDLE if the Video handle is invalid.
*******************************************************************************/
ST_ErrorCode_t STVID_DisableDeblocking(const STVID_Handle_t VideoHandle)
{
#ifdef ST_deblock
    VideoDevice_t *         Device_p;
    STVID_MemoryProfile_t   MemoryProfile;
    VideoStatus_t           Status;
    BOOL                    DeblockingEnabled=FALSE;
    ST_ErrorCode_t          ErrorCode= ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Deblocking is only possible on 7100 and 7109 */
    if((Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG)
            &&(Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    STVID_GetMemoryProfile( VideoHandle, &MemoryProfile);

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (Status != VIDEO_STATUS_STOPPED)
    {
        ErrorCode = (Status == VIDEO_STATUS_RUNNING ?
                     STVID_ERROR_DECODER_RUNNING : STVID_ERROR_DECODER_PAUSING);
    }
    else
    {
    /* verify that memory profile is correctly set:
     * decimation factor is different from STVID_MPEG_DEBLOCK_RECONSTRUCTION
     * otherwise , secondary buffers would be displayed*/
        if (MemoryProfile.DecimationFactor == STVID_MPEG_DEBLOCK_RECONSTRUCTION)
        {
            ErrorCode = STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE;
        }
        else
        {
            VIDPROD_SetDeblockingMode(Device_p->DeviceData_p->ProducerHandle, DeblockingEnabled);
            ErrorCode = ST_NO_ERROR;
        }
    }
    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);

#else /* ST_deblock */
    UNUSED_PARAMETER(VideoHandle);
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

#endif /* ST_deblock */
} /* End of STVID_DisableDeblocking() function */


/*******************************************************************************
Name        : STVID_SetSpeed
Description : Defines Trick mode speed.
Parameters  : Video handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SetSpeed(const STVID_Handle_t VideoHandle, const S32 Speed)
{
    VideoDevice_t * Device_p;
    VideoStatus_t Status;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
/* #ifdef ST_speed */
/*     BOOL SkipAllP; */
/* #endif */ /*ST_speed*/

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (IsTrickModeHandleOrSpeedHandleValid(Device_p))
    {
#ifdef ST_speed
	    if (Device_p->DeviceData_p->SpeedHandleIsValid)
	    {
        	/* !!! HG: TO BE REMOVED !!! : always call VIDTRICK_SetSpeed() !!! */
        	if ((Speed != 100) && (Status == VIDEO_STATUS_FREEZING))
        	{
            	/* A speed != 100 cannot be required if the decoder is frozen */
            	ErrorCode = STVID_ERROR_DECODER_FREEZING;
        	}
        	else
        	{

                /* SkipAllP = VIDSPEED_IsSkippingAllP(Device_p->DeviceData_p->SpeedHandle); */
                ErrorCode = VIDSPEED_SetSpeed(Device_p->DeviceData_p->SpeedHandle, Speed);
/*                 if ( SkipAllP) */
/*                 { */
/*                     VIDPROD_SkipData(Device_p->DeviceData_p->ProducerHandle, VIDPROD_SKIP_UNTIL_NEXT_SEQUENCE); */
/*                 } */
                /*VIDQUEUE_CancelAllPictures(Device_p->DeviceData_p->OrderingQueueHandle);*/

                /* Frees all not displayed pictures */
                /*VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(Device_p->DeviceData_p->DisplayHandle);*/
			}
        }
#endif /*ST_speed*/
#ifdef ST_trickmod
    	if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    	{
        	/* !!! HG: TO BE REMOVED !!! : always call VIDTRICK_SetSpeed() !!! */
        	if ((Speed != 100) && (Status == VIDEO_STATUS_FREEZING))
        	{
            	/* A speed != 100 cannot be required if the decoder is frozen */
            	ErrorCode = STVID_ERROR_DECODER_FREEZING;
        	}
        	else
        	{
            	ErrorCode = VIDTRICK_SetSpeed(Device_p->DeviceData_p->TrickModeHandle, Speed);
        	}
    	}
#endif /*ST_trickmod */
    }
    else
    {
        if (Speed != 100)
        {
            /* Only trickmod module can set speed */
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
    }
#ifdef ST_avsync
    /* Suspend/Resume synchronisation actions depending on speed */
    if (ErrorCode == ST_NO_ERROR)
    {
        if (Speed == 100)
        {
            /* ErrorCode =*/ VIDAVSYNC_ResumeSynchronizationActions(Device_p->DeviceData_p->AVSyncHandle);
        }
        else
        {
            /* ErrorCode =*/ VIDAVSYNC_SuspendSynchronizationActions(Device_p->DeviceData_p->AVSyncHandle);
        }
    }
#endif /* ST_avsync */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Error recovery new speed advice. */
        ErrorCode = stvid_SetSpeedErrorRecovery(Device_p, Speed);
    }

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_SetSpeed() function */

/*******************************************************************************
Name        : STVID_Setup
Description : Customise some objects which are setup automatically by the video driver at initialisation.
Parameters  : Video handle, Parameters for the setup of the objects
Assumptions : Video instance should be in STOPPED state.
Limitations :
Returns     : ST_NO_ERROR                       if success,
              ST_ERROR_BAD_PARAMETER            if one of the supplied parameter is invalid,
              ST_ERROR_INVALID_HANDLE           if the Video handle is invalid,
              ST_ERROR_ALREADY_INITIALIZED      if it is no more allowed to call this function,
              ST_ERROR_FEATURE_NOT_SUPPORTED    if one of the supplied parameter is not supported.
              ST_ERROR_DECODER_RUNNING          if the video instance is not in the STOPPED state.
              ST_ERROR_NO_MEMORY                if no memory to allocate object
*******************************************************************************/
ST_ErrorCode_t STVID_Setup(STVID_Handle_t Handle, const STVID_SetupParams_t * const SetupParams_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoStatus_t  Status;

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SetupParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)Handle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (Status != VIDEO_STATUS_STOPPED)
    {
        ErrorCode = STVID_ERROR_DECODER_RUNNING;
    }
    else
    {
        /* Video driver is in STOPPED state, we can proceed */
        switch (SetupParams_p->SetupObject)
        {
/*********/ case STVID_SETUP_FRAME_BUFFERS_PARTITION:   /********************************************************************************/
                ErrorCode = VIDBUFF_SetAvmemPartitionForFrameBuffers(Device_p->DeviceData_p->BuffersHandle,
                                                                     SetupParams_p->SetupSettings.AVMEMPartition);
                break;

/*********/ case STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION:   /**********************************************************************/
                ErrorCode = VIDBUFF_SetAvmemPartitionForDecimatedFrameBuffers(Device_p->DeviceData_p->BuffersHandle,
                                                                              SetupParams_p->SetupSettings.AVMEMPartition);
                break;

#ifdef ST_producer
/*********/ case STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION:   /******************************************************************/
                if (!(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_H264) &&
                    !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264) &&
                    !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_H264))
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                else if (Device_p->DeviceData_p->ProducerHandleIsValid)
                {
                    ErrorCode = VIDPROD_Setup(Device_p->DeviceData_p->ProducerHandle, SetupParams_p);
                }
                break;

/*********/ case STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION:   /********************************************************************/
                if ((Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_H264) &&
                    (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7100_DIGITAL_INPUT) &&
                    (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_H264) &&
                    (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7109_VC1) &&
                    (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7200_H264) &&
                    (Device_p->DeviceData_p->DeviceType != STVID_DEVICE_TYPE_7200_VC1))
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                else
                {
                    ErrorCode = VIDBUFF_SetAvmemPartitionForAdditionnalData(Device_p->DeviceData_p->BuffersHandle,
                                                                            SetupParams_p->SetupSettings.AVMEMPartition);
                }
                break;
#endif /* ST_producer */

/*********/ case STVID_SETUP_FDMA_NODES_PARTITION:   /***********************************************************************************/
                if (!(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_H264)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_VC1)
                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                else if (Device_p->DeviceData_p->ProducerHandleIsValid)
                {
                    ErrorCode = VIDPROD_Setup(Device_p->DeviceData_p->ProducerHandle, SetupParams_p);
                }
                break;

/*********/ case STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION:   /***************************************************************************************/
/*********/ case STVID_SETUP_BIT_BUFFER_PARTITION:   /***********************************************************************************/
/*********/ case STVID_SETUP_DATA_INPUT_BUFFER_PARTITION:   /***********************************************************************/
                if (!(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG)          */
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) */
                   && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1)
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)       */
                )
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                else if (Device_p->DeviceData_p->ProducerHandleIsValid)
                {
                    ErrorCode = VIDPROD_Setup(Device_p->DeviceData_p->ProducerHandle, SetupParams_p);
                }
                break;


#if defined(DVD_SECURED_CHIP)
/*********/ case STVID_SETUP_ES_COPY_BUFFER_PARTITION:   /*******************************************************************************/
                if (!(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG)*/
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT)*/
                   && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1)
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)*/
                 )
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                else
                {
                    /* That partition must be set in non-secure memory zone in application memory map. */
                    ErrorCode = VIDBUFF_SetAvmemPartitionForESCopyBuffer(Device_p->DeviceData_p->BuffersHandle,
                                                                         SetupParams_p->SetupSettings.AVMEMPartition);
                }
                break;

/*********/ case STVID_SETUP_ES_COPY_BUFFER:   /*****************************************************************************************/
                if (!(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG)*/
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT)*/
                   && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1)
/*                 && !(Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)*/
                 )
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                else if (Device_p->DeviceData_p->ProducerHandleIsValid)
                {
                    ErrorCode = VIDPROD_Setup(Device_p->DeviceData_p->ProducerHandle, SetupParams_p);
                }
                break;
#endif /* DVD_SECURED_CHIP */

/*********/ default:   /*****************************************************************************************************************/
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Setup(): Object not supported !"));
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
    }

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
}/* End of STVID_Setup() function */

#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
/*******************************************************************************
Name        : STVID_SetSynchronizationDelay
Description : Set the video synchronization delay on the Ext STC.
Parameters  : Video handle, Synchronization Delay
Assumptions :
Limitations :
Returns     : ST_NO_ERROR             if success,
              ST_ERROR_BAD_PARAMETER  if the AVSync component fails,
              ST_ERROR_INVALID_HANDLE if the Video handle is invalid.
*******************************************************************************/
ST_ErrorCode_t STVID_SetSynchronizationDelay(const STVID_Handle_t VideoHandle, const S32 SyncDelay)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

#ifdef ST_avsync
    ErrorCode = VIDAVSYNC_SetSynchronizationDelay(Device_p->DeviceData_p->AVSyncHandle, SyncDelay);
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDAVSYNC_SetSynchronizationDelay failed"));
    }
#else /* ST_avsync */
    /* No avsync: nothing to do */
    ErrorCode = ST_NO_ERROR;
#endif /* no ST_avsync */

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_SetSynchronizationDelay() function */
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */


/*******************************************************************************
Name        : STVID_SkipSynchro
Description :
Parameters  : Video handle, Time
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_SkipSynchro(const STVID_Handle_t VideoHandle,
                                 const STTS_t Time)
{
    VideoDevice_t * Device_p;
    U32 TimeInPTSUnit, OneVSyncDuration, NumberToDo, Tmp32;
    VideoStatus_t Status;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#if defined ST_trickmod || defined ST_speed
    S32 Speed = 100;
#endif /*  ST_trickmod || ST_speed*/

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Get current play speed... */
#ifdef ST_speed
    if ((Device_p->DeviceData_p->SpeedHandleIsValid) && (ErrorCode == ST_NO_ERROR))
    {
        ErrorCode = VIDSPEED_GetSpeed(Device_p->DeviceData_p->SpeedHandle, &Speed);
        if ((ErrorCode == ST_NO_ERROR) && (Speed != 100))
        {
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
#endif /*ST_speed*/
#ifdef ST_trickmod
    if ((Device_p->DeviceData_p->TrickModeHandleIsValid) && (ErrorCode == ST_NO_ERROR))
    {
        ErrorCode = VIDTRICK_GetSpeed(Device_p->DeviceData_p->TrickModeHandle, &Speed);
        if ((ErrorCode == ST_NO_ERROR) && (Speed != 100))
        {
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    }
#endif /* ST_trickmod*/

    /* Only do something if synchro is off */
#ifdef ST_avsync
    if (VIDAVSYNC_IsSynchronizationEnabled(Device_p->DeviceData_p->AVSyncHandle))
    {
        /* Synchro on or speed not 100: no synchro action can be done ! */
        ErrorCode = STVID_ERROR_NOT_AVAILABLE;
    }
#endif /* ST_avsync*/

    if (ErrorCode == ST_NO_ERROR)
    {
        stvid_EnterCriticalSection(Device_p);
        Status = GET_VIDEO_STATUS(Device_p);
        if (Status == VIDEO_STATUS_RUNNING)
        {
            TimeInPTSUnit = ((U32) Time);
            if (Device_p->DeviceData_p->VTGFrameRate != 0)
            {
                OneVSyncDuration = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000) / Device_p->DeviceData_p->VTGFrameRate);
            }
            else
            {
                OneVSyncDuration = STVID_DEFAULT_VSYNC_DURATION_MICROSECOND_WHEN_NO_VTG * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / 1000000;
            }
            /* Security: OneVSyncDuration should not be 0 because of division... */
            if (OneVSyncDuration != 0)
            {
                NumberToDo = (TimeInPTSUnit / OneVSyncDuration);
#ifdef ST_display
                /*ErrorCode =*/ VIDDISP_SkipFields(Device_p->DeviceData_p->DisplayHandle, NumberToDo, &Tmp32, TRUE, VIDDISP_SKIP_STARTING_FROM_CURRENTLY_DISPLAYED_PICTURE);
#else /* ST_display */
                Tmp32 = 0;
#endif /* no ST_display */
#ifdef ST_producer
                /* If skip display failed and decode available: do a little skip of decode.
                But don't do that if VTGFrameRate is 0: if no display connected, why perturb decode ?? */
                if ((Device_p->DeviceData_p->ProducerHandleIsValid) && ((NumberToDo - Tmp32) >= 2) && (Device_p->DeviceData_p->VTGFrameRate != 0))
                {
                    /*ErrorCode =*/ VIDPROD_SkipData(Device_p->DeviceData_p->ProducerHandle, VIDPROD_SKIP_NEXT_B_PICTURE);
                    Tmp32 += 2;
                }
#endif /* ST_producer */
                if (Tmp32 < NumberToDo)
                {
                    /* Could not perform all the skip: return stupid error code to inform user */
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            else
            {
                /* Could not perform skip because of error: return stupid error code to inform user */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {
            /* Status not running: return stupid error code */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        } /* not running */
        stvid_LeaveCriticalSection(Device_p);
    } /* no error */

#ifdef TRACE_UART
    TraceBuffer(("-SKIPSYNCHRO(%d=%d/%d:%d)", (U32) Time, Tmp32, NumberToDo, ErrorCode));
#endif /* TRACE_UART */

    return(ErrorCode);
} /* End of STVID_SkipSynchro() function */

/*******************************************************************************
Name        : STVID_Start
Description : Start the video decode
Parameters  : Video handle
              video decoder start parameters
Assumptions : The memory profile has been correctly set up by application
              (use of STVID_SetMemoryProfile).
Limitations :
Returns     : ST_NO_ERROR               if decode successfully started.
              ST_ERROR_INVALID_HANDLE   if the handle is not valid.
              ST_ERROR_BAD_PARAMETER    if at least one input parameter or
                    the memory profile is not valid.
              ST_ERROR_NO_MEMORY        if STAVMEM allocation failed
*******************************************************************************/
ST_ErrorCode_t STVID_Start(const STVID_Handle_t VideoHandle,
                           const STVID_StartParams_t * const Params_p)
{
    VideoStatus_t    Status;
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Only for MPEG Decoder */
#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        /* Disregard invalid stream type */
        switch (Params_p->StreamType)
        {
            case STVID_STREAM_TYPE_ES :
            case STVID_STREAM_TYPE_PES :
            case STVID_STREAM_TYPE_MPEG1_PACKET :
            case STVID_STREAM_TYPE_UNCOMPRESSED :
                break;

            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "STVID_Start() : Invalid stream type !"));
                return(ST_ERROR_BAD_PARAMETER);
        }

        /* Disregard invalid stream ID, but only is useful (Stream ID has meaning only if PES or MPEG1 packets */
        if (((Params_p->StreamType == STVID_STREAM_TYPE_PES) || (Params_p->StreamType == STVID_STREAM_TYPE_MPEG1_PACKET)) &&
            (Params_p->StreamID > 15) &&
            (Params_p->StreamID != STVID_IGNORE_ID))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Start() : Invalid stream ID !"));
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Disregard invalid broadcast profile */
        switch (Params_p->BrdCstProfile)
        {
            case STVID_BROADCAST_DVB :
            case STVID_BROADCAST_DIRECTV :
            case STVID_BROADCAST_ATSC :
            case STVID_BROADCAST_DVD :
            case STVID_BROADCAST_ARIB :
                break;

            default :
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "STVID_Start() : Invalid broadcast profile !"));
                return(ST_ERROR_BAD_PARAMETER);
        }
    }
#endif /* ST_producer */

    stvid_EnterCriticalSection(Device_p);
    /* Make sure that Video is not already running */
    Status = GET_VIDEO_STATUS(Device_p);
    if (Status != VIDEO_STATUS_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STVID_Start() : Decoder not stoppped !"));
        if (Status == VIDEO_STATUS_PAUSING)
        {
            ErrorCode = STVID_ERROR_DECODER_PAUSING;
        }
        else if (Status == VIDEO_STATUS_FREEZING)
        {
            ErrorCode = STVID_ERROR_DECODER_FREEZING;
        }
        else
        {
            ErrorCode = STVID_ERROR_DECODER_RUNNING;
        }
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = stvid_ActionStart(Device_p, Params_p);
    }


    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_Start() function */

/*******************************************************************************
Name        : stvid_ActionStart
Description : Actions to start the decoder
Parameters  : Video device, start params
Assumptions : Function calls are protected with stvid_EnterCriticalSection(Device_p)
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvid_ActionStart(VideoDevice_t * const Device_p, const STVID_StartParams_t * const Params_p)
{
#ifdef ST_producer
    VIDPROD_StartParams_t   ProducerStartParams;
#endif /* ST_producer */
#ifdef ST_diginput
# ifndef ST_MasterDigInput
    VIDINP_StartParams_t    DigInputStartParams;
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDCOM_InternalProfile_t CommonMemoryProfile;
#ifdef ST_display
    VIDBUFF_AvailableReconstructionMode_t  Reconstruction;
    U32                     Index;
    VideoViewPort_t *       ViewPort_p;
#endif /* ST_display */

    /* Get the memory profile of THIS decode/display process. */
    ErrorCode = VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &CommonMemoryProfile);
    if (ErrorCode!=ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                "STVID_Start() : Impossible to get memory profile !"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if the memory profile is good to be allocated !!! */
    if (    (CommonMemoryProfile.ApiProfile.MaxWidth     == 0)
         || (CommonMemoryProfile.ApiProfile.MaxHeight    == 0)
         || (CommonMemoryProfile.NbMainFrameStore == 0) )
    {
        /* The memory profile is not valid to be allocated. Return bad parameter. */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Test if stop was missing */
        if (Device_p->DeviceData_p->ExpectingEventStopped)
        {
            /* Particular case when stop was requested (NOT now), but didn't occur before Start() occurs again */
            STVID_PictureParams_t PictureParams;
#ifdef ST_display
            STVID_PictureInfos_t PictureInfos;
#endif /* ST_display */

            /* Force stop now: de-allocated buffers (and recovers those erroneously locked) */
            DeAllocateAfterStop(Device_p, TRUE);

#ifdef ST_display
            /* Retrieve displayed picture information */
            VIDDISP_GetDisplayedPictureInfos(Device_p->DeviceData_p->DisplayHandle, &PictureInfos);
            /* Convert to old format of picture infos, TemporalReference info is unreachable so put 0 */
            vidcom_PictureInfosToPictureParams(&PictureInfos, &PictureParams, 0);
            /* Notify STVID exported event */
            PictureParams.Data = DeviceToVirtual(PictureParams.Data, Device_p->DeviceData_p->VirtualMapping);
#else /* ST_display */
            /* PictureParams. = ; Don't leave PictureParams empty !!! */
#endif /* no ST_display */
            if(Device_p->DeviceData_p->NotifyStopped)
            {
                STEVT_Notify(Device_p->DeviceData_p->EventsHandle, Device_p->DeviceData_p->EventID[STVID_STOPPED_ID], STEVT_EVENT_DATA_TYPE_CAST &PictureParams);
                Device_p->DeviceData_p->NotifyStopped = FALSE;
            }
            Device_p->DeviceData_p->ExpectingEventStopped = FALSE;
        }

        /* Now that all error cases have been treated, do start ! */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Start()..."));

        /* Allocate frame buffers for the current profile */
        if ((CommonMemoryProfile.ApiProfile.NbFrameStore != STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID) && (CommonMemoryProfile.ApiProfile.NbFrameStore != STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID))
        {
            ErrorCode = VIDBUFF_AllocateMemoryForProfile(Device_p->DeviceData_p->BuffersHandle, FALSE, 0);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "STVID_Start() : Memory profile allocation failed !"));
            }
        }

#ifdef ST_ordqueue
        if (Device_p->DeviceData_p->OrderingQueueHandleIsValid)
        {
            VIDQUEUE_Data_t  * VIDQUEUE_Data_p = (VIDQUEUE_Data_t *) (Device_p->DeviceData_p->OrderingQueueHandle);
            VIDQUEUE_Data_p->FirstPictureEver = TRUE;
        }
#endif /*ST_ordqueue*/

    }
#ifdef ST_SecondInstanceSharingTheSameDecoder
if (!(Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder))
{
#endif /* ST_SecondInstanceSharingTheSameDecoder */
    if (ErrorCode == ST_NO_ERROR)
    {
#ifdef ST_producer
        /* Only for Decode Process */
        if (Device_p->DeviceData_p->ProducerHandleIsValid)
        {
            /* Highest value of Picture ID : set to minimal possible value */
            Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID.ID = MIN_S32;
            Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID.IDExtension = 0;
            Device_p->DeviceData_p->ForceDecimationFactor.FirstForcedPictureID = Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID;

            /* Inform error recovery that start is done */
            stvid_StartVideoErrorRecovery(Device_p, Params_p->RealTime);

            ProducerStartParams.RealTime = Params_p->RealTime;
            ProducerStartParams.StreamType = Params_p->StreamType;
            ProducerStartParams.BroadcastProfile = Params_p->BrdCstProfile;
            ProducerStartParams.StreamID = Params_p->StreamID;
            ProducerStartParams.UpdateDisplay = Params_p->UpdateDisplay;
            ProducerStartParams.DecodeOnce = Params_p->DecodeOnce;
            ProducerStartParams.UpdateDisplayQueueMode = Device_p->DeviceData_p->UpdateDisplayQueueMode;

            ErrorCode = VIDPROD_Start(Device_p->DeviceData_p->ProducerHandle, &ProducerStartParams);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Decoder failed to start: try to stop then re-start */
                VIDPROD_Stop(Device_p->DeviceData_p->ProducerHandle, STVID_STOP_NOW, FALSE);
                ErrorCode = VIDPROD_Start(Device_p->DeviceData_p->ProducerHandle, &ProducerStartParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    VIDBUFF_DeAllocateUnusedMemoryOfProfile(Device_p->DeviceData_p->BuffersHandle); /* Undo allocation done previously */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "STVID_Start() : Decoder start failed !"));
                }
            }
        }
#endif /* ST_producer */
#ifdef ST_diginput
# ifndef ST_MasterDigInput
        /* Only for Digital Input */
        if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
        {
            ErrorCode = VIDINP_Start(Device_p->DeviceData_p->DigitalInputHandle, &DigInputStartParams);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Digital Input failed to start: try to stop then re-start */
                VIDINP_Stop(Device_p->DeviceData_p->DigitalInputHandle, STVID_STOP_NOW);
                ErrorCode = VIDINP_Start(Device_p->DeviceData_p->DigitalInputHandle, &DigInputStartParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    VIDBUFF_DeAllocateUnusedMemoryOfProfile(Device_p->DeviceData_p->BuffersHandle); /* Undo allocation done previously */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "STVID_Start() : Digital Input start failed !"));
                }
            }
        }
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */
    }

#ifdef ST_display
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Added for 7020 chip / dual reconstruction chip : */
        if(CommonMemoryProfile.NbSecondaryFrameStore == 0)
        {
            ViewPort_p = Device_p->DeviceData_p->ViewportProperties_p;
            for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
            {
                if(ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE)
                {
                    ErrorCode = VIDDISP_GetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                        ViewPort_p->Identity.Layer_p->LayerType,
                        &Reconstruction);
                    if((Reconstruction != VIDBUFF_NONE_RECONSTRUCTION)&&(ErrorCode == ST_NO_ERROR))
                    {
                        VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                            ViewPort_p->Identity.Layer_p->LayerType,
                            VIDBUFF_MAIN_RECONSTRUCTION);
                    }
                }
            ViewPort_p++;
            }/* end for */
        }/*end nb secondary buff == 0 */
        else
        {
#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
        /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
        * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum)
        * Force reconstruction chan even if aux display is already set in decimated ReconstructionMode */
            /* As decimation is allowed, secondary reconstruction will be enabled so we can start to use secondary buffer right now */
            ViewPort_p = Device_p->DeviceData_p->ViewportProperties_p;
            for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
            {
                if ((ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE) &&
                    (ViewPort_p->Identity.Layer_p->LayerType == STLAYER_HDDISPO2_VIDEO2))
                {
                    VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                        ViewPort_p->Identity.Layer_p->LayerType,
                        VIDBUFF_SECONDARY_RECONSTRUCTION);
                }
            ViewPort_p++;
            }/* end for */
#else /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
            if ((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)||
                (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
                (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
                (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
            {
                /* As decimation is allowed, secondary reconstruction will be enabled so we can start to use secondary buffer right now */
                ViewPort_p = Device_p->DeviceData_p->ViewportProperties_p;
                for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
                {
                    if (ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE)
                    {
                        VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                            ViewPort_p->Identity.Layer_p->LayerType,
                            VIDBUFF_SECONDARY_RECONSTRUCTION);
                    }
                ViewPort_p++;
                }/* end for */
            }
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
#ifdef ST_deblock
            if ((CommonMemoryProfile.ApplicableDecimationFactor == STVID_MPEG_DEBLOCK_RECONSTRUCTION) ||
				(CommonMemoryProfile.ApplicableDecimationFactor == STVID_POSTPROCESS_RECONSTRUCTION))
            {
                /* As decimation is allowed, secondary reconstruction will be enabled so we can start to use secondary buffer right now */
                ViewPort_p = Device_p->DeviceData_p->ViewportProperties_p;
                for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
                {
                    if (ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE)
                    {
                        VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                            ViewPort_p->Identity.Layer_p->LayerType,
                            VIDBUFF_SECONDARY_RECONSTRUCTION);
                    }
                ViewPort_p++;
                }/* end for */
            }
            else
            {
                /* As decimation is allowed, secondary reconstruction will be enabled so we can start to use secondary buffer right now */
                ViewPort_p = Device_p->DeviceData_p->ViewportProperties_p;
                for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
                {
                    if (ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE)
                    {
                        VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                            ViewPort_p->Identity.Layer_p->LayerType,
                            VIDBUFF_MAIN_RECONSTRUCTION);
                    }
                ViewPort_p++;
                }/* end for */
            }
#else /* !ST_deblock */
			if (CommonMemoryProfile.ApplicableDecimationFactor == STVID_POSTPROCESS_RECONSTRUCTION)
			{
				/* As decimation is allowed, secondary reconstruction will be enabled so we can start to use secondary buffer right now */
                ViewPort_p = Device_p->DeviceData_p->ViewportProperties_p;
                for (Index = 0; Index < Device_p->DeviceData_p->MaxViewportNumber; Index++)
                {
                    if (ViewPort_p->Identity.ViewPortOpenedAndValidityCheck == VIDAPI_VALID_VIEWPORT_HANDLE)
                    {
                        VIDDISP_SetReconstructionMode(Device_p->DeviceData_p->DisplayHandle,
                            ViewPort_p->Identity.Layer_p->LayerType,
                            VIDBUFF_SECONDARY_RECONSTRUCTION);
                    }
                ViewPort_p++;
                }/* end for */
			}
#endif /* ST_deblock */

        } /*end nb secondary buff != 0 */

        if (Params_p->UpdateDisplay) /* Take sure the display is started if display update is asked ...*/
        {
            ErrorCode = VIDDISP_Start (Device_p->DeviceData_p->DisplayHandle, FALSE);
        }
    }
#endif /* ST_display */

#ifdef ST_avsync
    /* Reset synchronisation */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Reset synchronisation */
        ErrorCode = VIDAVSYNC_ResetSynchronization(Device_p->DeviceData_p->AVSyncHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STVID_Start() : Reset of synchronisation failed !"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
#endif /* ST_avsync */

    /* Enables Trick Mode Speed Control */
#if defined ST_speed
    if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->SpeedHandleIsValid))
    {
        /* Enable speed control if not real time */
        ErrorCode = VIDSPEED_EnableSpeedControl(Device_p->DeviceData_p->SpeedHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STVID_Start() : Speed enable speed control failed!"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
#endif  /*ST_speed*/
#ifdef ST_trickmod
    if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->TrickModeHandleIsValid))
    {
        /* Enable speed control if not real time */
        ErrorCode = VIDTRICK_EnableSpeedControl(Device_p->DeviceData_p->TrickModeHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STVID_Start() : TrickMode enable speed control failed!"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
#ifdef ST_smoothbackward
        VIDTRICK_Start(Device_p->DeviceData_p->TrickModeHandle);
#endif /* ST_smoothbackward */
    }
#endif /* ST_trickmod*/

#ifdef ST_SecondInstanceSharingTheSameDecoder
}
else
{
    ErrorCode = ST_NO_ERROR ;
}
#endif /* ST_SecondInstanceSharingTheSameDecoder */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Save parameters for eventual re-start */
        Device_p->DeviceData_p->StartParams = *Params_p;

        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Start() done."));
        SET_VIDEO_STATUS(Device_p, VIDEO_STATUS_RUNNING);
#ifdef STVID_DEBUG_GET_STATUS
		Device_p->DeviceData_p->LastStartParams = (*Params_p);
		Device_p->DeviceData_p->VideoAlreadyStarted = TRUE;
#endif /* STVID_DEBUG_GET_STATUS */
#ifdef ST_SecondInstanceSharingTheSameDecoder
        if (!(Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder))
#endif /* ST_SecondInstanceSharingTheSameDecoder */
        {
            /* Ensure nobody else accesses display params */
            semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

            /* At start re-check display params and send to VIDDISP. */
            CheckDisplayParamsAndUpdate(Device_p);

            /* Release access to display params */
            semaphore_signal(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);
        }

    }
    return(ErrorCode);
} /* End of stvid_ActionStart */

/*******************************************************************************
Name        : STVID_StartUpdatingDisplay
Description : Start updating the display of the video
Parameters  : Video handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR               if decode successfully started.
              ST_ERROR_INVALID_HANDLE   if the handle is not valid.
              STVID_ERROR_DECODER_STOPPED if the decoder is stopped
*******************************************************************************/
ST_ErrorCode_t STVID_StartUpdatingDisplay(const STVID_Handle_t VideoHandle)
{
    VideoDevice_t * Device_p;
    VideoStatus_t   Status;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: Get an entry point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Get the video decoder status. */
    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_STOPPED)
    {
        ErrorCode = STVID_ERROR_DECODER_STOPPED;
    }
    else
    {
        /* Ok, the StartUpdateDisplay action can be performed... */

        /* TO BE DONE porperly !!! */

#ifdef ST_producer
        if (Device_p->DeviceData_p->ProducerHandleIsValid)
        {
            ErrorCode = VIDPROD_StartUpdatingDisplay(Device_p->DeviceData_p->ProducerHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_StartUpdatingDisplay() failed !"));
            }
        }
#endif /* ST_producer */
#ifdef ST_diginput
# ifndef ST_MasterDigInput
        /* Only for Digital Input */
        if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
        {
/*            ErrorCode = VIDINP_StartUpdatingDisplay(Device_p->DeviceData_p->ProducerHandle);*/
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDINP_StartUpdatingDisplay() failed !"));
            }
        }
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */
        /* !!! TO BE REMOVED */
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    } /* end do start update display */

    /* Return the function's result */
    return(ErrorCode);

} /* End of STVID_StartUpdatingDisplay() function */


/*******************************************************************************
Name        : STVID_Step
Description : Displays next field when paused or freezed
Parameters  : Video handle
Assumptions : The video decoder is in Paused or Freezed state.
Limitations : Not available with device D1 input.
Returns     : ST_NO_ERROR                    - if the step is done successfully.
              ST_ERROR_INVALID_HANDLE        - if the video handle is invalid.
              ST_ERROR_FEATURE_NOT_SUPPORTED - if the function is not supported
                                               by the device.
              STVID_ERROR_DECODER_NOT_PAUSING - If the decoder is not pausing nor freezing
*******************************************************************************/
ST_ErrorCode_t STVID_Step(const STVID_Handle_t VideoHandle)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    VideoDevice_t * Device_p;
    VideoStatus_t   Status;

    /* Exit now if parameters are bad */
    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: Get an entry point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Get the video decoder status. */
    Status = GET_VIDEO_STATUS(Device_p);
    if ((Status != VIDEO_STATUS_PAUSING) && (Status != VIDEO_STATUS_FREEZING))
    {
        ErrorCode = STVID_ERROR_DECODER_NOT_PAUSING;
    }
    else
    {
#ifdef ST_display
        /* Ok, the Step action can be performed... */
        ErrorCode = VIDDISP_Step(Device_p->DeviceData_p->DisplayHandle);
#else /* ST_display */
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* no ST_display */
    }

    /* Return the function's result */
    return(ErrorCode);

} /* End of STVID_Step() function. */

/*******************************************************************************
Name        : STVID_Stop
Description : Stop the video decode
Parameters  : Video handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STVID_Stop(const STVID_Handle_t VideoHandle,
        const STVID_Stop_t StopMode, const STVID_Freeze_t * const Freeze_p)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (Freeze_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Disregard invalid stop mode */
    switch(StopMode)
    {
        case STVID_STOP_WHEN_END_OF_DATA :
        case STVID_STOP_WHEN_NEXT_I :
        case STVID_STOP_WHEN_NEXT_REFERENCE :
            switch (Device_p->DeviceData_p->DeviceType)
            {
                case STVID_DEVICE_TYPE_7015_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7020_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7710_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7100_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7109_DIGITAL_INPUT :
                case STVID_DEVICE_TYPE_7200_DIGITAL_INPUT :
                    /* Don't allow other stop modes than "now" with digital inputs */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Stop(): Stop mode not allowed !"));
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);

                default :
                    /* OK for all MPEG types*/
                    break;
            }
            break;

        case STVID_STOP_NOW :
            /* OK */
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Stop(): Unknown stop mode !"));
            return(ST_ERROR_BAD_PARAMETER);
    }

    /* Disregard invalid field */
    switch (Freeze_p->Field)
    {
        case STVID_FREEZE_FIELD_TOP :
        case STVID_FREEZE_FIELD_BOTTOM :
        case STVID_FREEZE_FIELD_CURRENT :
        case STVID_FREEZE_FIELD_NEXT :
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_Stop(): Unknown freeze field !"));
            return(ST_ERROR_BAD_PARAMETER);
    }

    stvid_EnterCriticalSection(Device_p);

    /* Make sure video is running */
    if (GET_VIDEO_STATUS(Device_p) == VIDEO_STATUS_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STVID_Stop() : Video already stopped !"));
        ErrorCode = STVID_ERROR_DECODER_STOPPED;
    }

    /* Allow only STVID_STOP_NOW or STVID_STOP_WHEN_END_OF_DATA if the driver is freezed or paused */
    if ( (ErrorCode == ST_NO_ERROR) &&
        ((GET_VIDEO_STATUS(Device_p) == VIDEO_STATUS_PAUSING) || (GET_VIDEO_STATUS(Device_p) == VIDEO_STATUS_FREEZING)) &&
        ((StopMode != STVID_STOP_NOW) && (StopMode != STVID_STOP_WHEN_END_OF_DATA)))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STVID_Stop() : Only Stop Now is allowed when pausing or freezing !"));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Now that all error cases have been treated, do stop ! */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Stop()..."));

        /* Perform stop actions. */
        ErrorCode = stvid_ActionStop(Device_p, StopMode, Freeze_p);
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID_Stop() done."));
    }

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_Stop() function */


/* Private Functions -------------------------------------------------------- */


/*******************************************************************************
Name        : CheckDisplayParamsAndUpdate
Description : Check if display params have change and send to VIDDISP
Parameters  : pointer to device
Assumptions : Device_p is not NULL
              Function calls must be protected with DisplayParamsAccessSemaphore_p
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t CheckDisplayParamsAndUpdate(VideoDevice_t * const Device_p)
{
#ifdef ST_display
    VIDDISP_DisplayParams_t DisplayParams;
#endif /* ST_display */
#if defined ST_trickmod || defined ST_speed
    S32 Speed = 100;
#endif /* ST_trickmod || ST_speed */

    if (GET_VIDEO_STATUS(Device_p) == VIDEO_STATUS_STOPPED)
    {
        /* No need to update display if stopped ! */
        return(ST_NO_ERROR);
    }

#ifdef ST_display
    /* By default: take params of API */
    DisplayParams = Device_p->DeviceData_p->DisplayParamsForAPI;

#if defined ST_speed
    if (Device_p->DeviceData_p->SpeedHandleIsValid)
    {
        /* Get current play speed... */
        if (VIDSPEED_GetSpeed(Device_p->DeviceData_p->SpeedHandle, &Speed) != ST_NO_ERROR)
        {
            Speed = 100;
        }
    }
#endif /*ST_speed*/
#ifdef ST_trickmod
    if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    {
        /* Get current play speed... */
        if (VIDTRICK_GetSpeed(Device_p->DeviceData_p->TrickModeHandle, &Speed) != ST_NO_ERROR)
        {
            Speed = 100;
        }
    }
#endif /* ST_trickmod */
#if defined ST_trickmod || defined ST_speed
    if (IsTrickModeHandleOrSpeedHandleValid(Device_p))
    {
        /* Take infos coming from trickmode if speed not 100 */
        if (Speed != 100)
        {
            DisplayParams.DisplayForwardNotBackward = Device_p->DeviceData_p->DisplayParamsForTrickMode.DisplayForwardNotBackward;
            DisplayParams.DisplayWithCareOfFieldsPolarity = Device_p->DeviceData_p->DisplayParamsForTrickMode.DisplayWithCareOfFieldsPolarity;
        }
        /* For DisplayStart, taking point of view of trick mode even if speed is 100 may be useful for speed change cases (just after set to 100 ?) */
        if ((Device_p->DeviceData_p->DisplayParamsForAPI.DisplayStart == VIDDISP_DISPLAY_START_CARE_MPEG_FRAME) &&
            (Device_p->DeviceData_p->DisplayParamsForTrickMode.DisplayStart == VIDDISP_DISPLAY_START_CARE_MPEG_FRAME))
        {
            /* Only allow mode CARE if all conditions are met for that: API and trickmodes should want it. */
            DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
        }
        else
        {
            DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
        }
    }
#endif /* ST_trickmod || ST_speed */

    /* Set display params */
    VIDDISP_SetDisplayParams(Device_p->DeviceData_p->DisplayHandle, &DisplayParams);
#endif /* ST_display */

    return(ST_NO_ERROR);

} /* end of CheckDisplayParamsAndUpdate() function */


/*******************************************************************************
Name        : DeAllocateAfterStop
Description : De-allocate unused frame buffers after stop
Parameters  : pointer to device
              Boolean indicating if a recovery of the frame buffers locked should be done.
Assumptions : Device_p is not NULL
              Function calls must be protected with stvid_EnterCriticalSection(Device_p)
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t DeAllocateAfterStop(const VideoDevice_t * Device_p, BOOL RecoverUnusedMemory)
{
    ST_ErrorCode_t ErrorCode;

#ifdef ST_ordqueue
    /* Cancel all pictures waiting in ordering queue */
    VIDQUEUE_CancelAllPictures(Device_p->DeviceData_p->OrderingQueueHandle);
#endif /* ST_ordqueue */

#ifdef ST_display
    /* Stop display to one picture, others are cleaned from display queue */
    VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(Device_p->DeviceData_p->DisplayHandle);
#endif /* ST_display */

    if (RecoverUnusedMemory)
    {
        /* Recover frame buffers still locked after stop */
        VIDBUFF_RecoverFrameBuffers(Device_p->DeviceData_p->BuffersHandle);
    }

    /* De-allocate unused frame buffers now that decoder is stopped */
    ErrorCode = VIDBUFF_DeAllocateUnusedMemoryOfProfile(Device_p->DeviceData_p->BuffersHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STVID_Stop() : Memory profile de-allocation failed !"));
    }

    return(ErrorCode);
} /* End of DeAllocateAfterStop() function */


/*******************************************************************************
Name        : stvid_ActionStop
Description : Actions to stop the decoder
Parameters  : Video device, stop mode, freeze
Assumptions : Function calls are protected with stvid_EnterCriticalSection(Device_p)
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, STVID_ERROR_DECODER_RUNNING_IN_RT_MODE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvid_ActionStop(VideoDevice_t * const Device_p, const STVID_Stop_t StopMode, const STVID_Freeze_t * const Freeze_p)
{
#ifdef ST_display
    VIDDISP_DisplayState_t DisplayState;
#endif /* ST_display */
#ifdef ST_producer
#if defined ST_trickmod || defined ST_speed
    BOOL TrickModeOrSpeedWillStopDecode = FALSE;
#endif /* ST_trickmod || ST_speed */
#endif /* ST_producer */
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
#if defined ST_speed
        if (Device_p->DeviceData_p->SpeedHandleIsValid)
        {
            /* if smooth and speed < 0, Trickmode is the main controller of decode-display. */
            /* Then Trickmode has to be stopped */
            ErrorCode = VIDSPEED_Stop(Device_p->DeviceData_p->SpeedHandle, StopMode, &TrickModeOrSpeedWillStopDecode, Freeze_p);
            if (ErrorCode != ST_NO_ERROR)
            {
                return(ErrorCode);
            }
        } /* end smooth backward trickmod active */
#endif /*ST_speed*/
#ifdef ST_trickmod
        if (Device_p->DeviceData_p->TrickModeHandleIsValid)
        {
            /* if smooth and speed < 0, Trickmode is the main controller of decode-display. */
            /* Then Trickmode has to be stopped */
            ErrorCode = VIDTRICK_Stop(Device_p->DeviceData_p->TrickModeHandle, StopMode, &TrickModeOrSpeedWillStopDecode, Freeze_p);
            if (ErrorCode != ST_NO_ERROR)
            {
                return(ErrorCode);
            }
        } /* end smooth backward trickmod active */
#endif /* ST_trickmod*/

#ifdef ST_display
        VIDDISP_GetState(Device_p->DeviceData_p->DisplayHandle, &DisplayState);
        if (StopMode == STVID_STOP_NOW)
        {
            /* Stop on the required frozen field now */
            /* No need to wait VIDPROD_Stop event : it will not be sent if StopMode == STVID_STOP_NOW */
            /* because stop has to be done right now and decode will not send VIDPROD_STOPPED_EVENT */
            VIDDISP_Stop(Device_p->DeviceData_p->DisplayHandle, NULL, VIDDISP_STOP_NOW);

            /* VIDDISP_Freeze instead of VIDDISP_Pause because, if next field of the picture */
            /* has to be displayed, it will be skipped. */
            VIDDISP_Freeze(Device_p->DeviceData_p->DisplayHandle, Freeze_p);
        }
        else
        {
            /* Remember Freeze params, if display is not paused. Will be given to display when Decoder will be stopped */
            if (DisplayState != VIDDISP_DISPLAY_STATE_PAUSED)
            {
                /* Ensure nobody else accesses display params */
                semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

                Device_p->DeviceData_p->RequiredStopFreezeParams = *Freeze_p;

                /* Release access to display params */
                semaphore_signal(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);
            }
            else /* Display is paused */
            {
                /* Call VIDDISP_Stop to release all frames that are blocked in the display queue */
                /* and make decoder can't decode and stop on the specified picture */
                VIDDISP_Stop(Device_p->DeviceData_p->DisplayHandle, NULL, VIDDISP_STOP_NOW);
            }
        }
#endif /* ST_display */

#if defined ST_trickmod || defined ST_speed
        /* If display is paused, must STOP decoder when no more buffers are available  */
        if (!(TrickModeOrSpeedWillStopDecode))
        {
#endif /* ST_trickmod || ST_speed */
#ifdef ST_SecondInstanceSharingTheSameDecoder
#ifdef ST_display
            if (Device_p->DeviceData_p->SpecialCaseOfSecondInstanceSharingTheSameDecoder)
            {
                if (DisplayState == VIDDISP_DISPLAY_STATE_PAUSED)
                {
                    ErrorCode = VIDPROD_StopSecond(Device_p->DeviceData_p->ProducerHandle, StopMode, TRUE);
                }
                else
                {
#endif /* ST_display */
                    ErrorCode = VIDPROD_StopSecond(Device_p->DeviceData_p->ProducerHandle, StopMode, FALSE);
#ifdef ST_display
                }
#endif /* ST_display */
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                "STVID_Stop() : Decoder stop failed !"));
                }
            }
            else
#endif /* ST_SecondInstanceSharingTheSameDecoder */
            {
#ifdef ST_display
            /* If display is paused, must STOP decoder when no more buffers are available  */

            if (DisplayState == VIDDISP_DISPLAY_STATE_PAUSED)
            {
                ErrorCode = VIDPROD_Stop(Device_p->DeviceData_p->ProducerHandle, StopMode, TRUE);
            }
            else
            {
#endif /* ST_display */
                ErrorCode = VIDPROD_Stop(Device_p->DeviceData_p->ProducerHandle, StopMode, FALSE);
#ifdef ST_display
            }
#endif /* ST_display */
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "STVID_Stop() : Decoder stop failed !"));
            }
            }
#if defined ST_trickmod || defined ST_speed
        }
#endif /* ST_trickmod || ST_speed */
    } /* end decode handle valid */
    else
    {
#endif /* ST_producer */

#ifdef ST_diginput
#ifndef ST_MasterDigInput
        if (Device_p->DeviceData_p->DigitalInputHandleIsValid)
        {
            ErrorCode = VIDINP_Stop(Device_p->DeviceData_p->DigitalInputHandle, StopMode);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "STVID_Stop() : Digital Input stop failed !"));
            }
        }
        else
        {
#endif /* ST_MasterDigInput */
#endif /* ST_diginput */

#ifdef ST_display
            /* Stop display */
            if (Device_p->DeviceData_p->DisplayHandleIsValid)
            {
                VIDDISP_Stop(Device_p->DeviceData_p->DisplayHandle, NULL, VIDDISP_STOP_NOW);
            }
#endif /* ST_display */

#ifdef ST_diginput
# ifndef ST_MasterDigInput
        }
# endif /* ST_MasterDigInput */
#endif /* ST_diginput */

#ifdef ST_producer
    }
#endif /* ST_producer */

    /* If stop later, event STVID_STOPPED_EVT will call VIDBUFF_DeAllocateUnusedMemoryOfProfile() */
    if ((ErrorCode == ST_NO_ERROR) && (StopMode == STVID_STOP_NOW))
    {
        /* Deallocate unused frame buffers after stop (and recovers those erroneously locked) */
        DeAllocateAfterStop(Device_p, TRUE);
    }
    else
    {
        /* We are internally waiting for display stop */
        Device_p->DeviceData_p->ExpectingEventStopped = TRUE;
        /* We have to notify the world when display stops */
        Device_p->DeviceData_p->NotifyStopped = TRUE;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        Device_p->DeviceData_p->StopMode = StopMode;
        SET_VIDEO_STATUS(Device_p, VIDEO_STATUS_STOPPED);
    }

    return(ErrorCode);
} /* End of stvid_ActionStop() function */


/*******************************************************************************
Name        : stvid_BuffersImpossibleWithProfile
Description : Function subscribed by the API for VIDBUFF event impossible with profile
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_BuffersImpossibleWithProfile(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);
    /* Directly notify event to application, just give sub-structure as data... */
    STEVT_Notify(((VideoDevice_t *) SubscriberData_p)->DeviceData_p->EventsHandle,
            ((VideoDevice_t *) SubscriberData_p)->DeviceData_p->EventID[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID],
            STEVT_EVENT_DATA_TYPE_CAST ((STVID_MemoryProfile_t *) EventData_p));
} /* end of stvid_BuffersImpossibleWithProfile() function */


#ifdef ST_display
/*******************************************************************************
Name        : stvid_DisplayOutOfPictures
Description : Function subscribed by the API from VIDDISP. Event OUT_OF_PICTURES.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_DisplayOutOfPictures(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
#if defined ST_trickmod || defined ST_speed
    ST_ErrorCode_t ErrorCode;
#endif /* ST_trickmod || ST_speed*/
    VideoStatus_t Status;
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    stvid_EnterCriticalSection(Device_p);

    /* Verify that state is still stop: if it changed since asked for stop, cancel the event. */
    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_STOPPED)
    {
        /* Clean buffers and notify that driver has stopped*/
#if defined ST_speed
        /* Disables Trick Mode Speed Control */
        if (Device_p->DeviceData_p->SpeedHandleIsValid)
        {
            ErrorCode = VIDSPEED_DisableSpeedControl(Device_p->DeviceData_p->SpeedHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                "STVID_Stop() : Speed disable speed control failed!"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
#endif /*ST_speed */
#ifdef ST_trickmod
        /* Disables Trick Mode Speed Control */
        if (Device_p->DeviceData_p->TrickModeHandleIsValid)
        {
            ErrorCode = VIDTRICK_DisableSpeedControl(Device_p->DeviceData_p->TrickModeHandle);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                "STVID_Stop() : TrickMode disable speed control failed!"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
#endif /* ST_trickmod*/

        /* Now that decoder has stopped: de-allocated buffers */
        DeAllocateAfterStop(Device_p, FALSE);

        /* Don't expect anymore the event as it was notified */
        Device_p->DeviceData_p->ExpectingEventStopped = FALSE;
        /* Export the event if needed */
        if(Device_p->DeviceData_p->NotifyStopped)
        {
        /* Translate address */
            if (EventData_p != NULL)
            {
        ((STVID_PictureParams_t *) EventData_p)->Data = DeviceToVirtual(((STVID_PictureParams_t *) EventData_p)->Data, Device_p->DeviceData_p->VirtualMapping);
            }
            STEVT_Notify(Device_p->DeviceData_p->EventsHandle, Device_p->DeviceData_p->EventID[STVID_STOPPED_ID], STEVT_EVENT_DATA_TYPE_CAST ((STVID_PictureParams_t *) EventData_p));
            Device_p->DeviceData_p->NotifyStopped = FALSE;
        }
    }

    stvid_LeaveCriticalSection(Device_p);

} /* End of stvid_DisplayOutOfPictures() function */
#endif /* ST_display */


#ifdef ST_producer
/*******************************************************************************
Name        : stvid_DecoderStopped
Description : Function to subscribe the VIDPROD event STOPPED
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_DecoderStopped(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
#ifdef ST_display
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
    STVID_PictureParams_t * LastPictureParams_p;
    VideoStatus_t Status;
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    stvid_EnterCriticalSection(Device_p);

    /* Verify that state is still stop: if it changed since asked for stop, cancel the event. */
    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_STOPPED)
    {
        /*  */
        LastPictureParams_p = (STVID_PictureParams_t *) EventData_p;

        if (Device_p->DeviceData_p->StopMode == STVID_STOP_WHEN_END_OF_DATA)
        {
            /* stop display on the last picture */
            VIDDISP_Stop(Device_p->DeviceData_p->DisplayHandle, NULL, VIDDISP_STOP_LAST_PICTURE_IN_QUEUE);
        }
        else
        {
            /* stop display on the required picture, given by STVID_Stop parameters */
            if (LastPictureParams_p == NULL)
            {
                /* To avoid accidental NULL picture pointer, call stop on last picture in display queue */
                VIDDISP_Stop(Device_p->DeviceData_p->DisplayHandle, NULL, VIDDISP_STOP_LAST_PICTURE_IN_QUEUE);
            }
            else
            {
            VIDDISP_Stop(Device_p->DeviceData_p->DisplayHandle, &(LastPictureParams_p->PTS), VIDDISP_STOP_ON_PTS);
        }
        }

        /* Ensure nobody else accesses display params */
        semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

        /* VIDDISP_Freeze instead of VIDDISP_Pause because, if next field of the picture */
        /* has to be displayed, it will be skipped */
        VIDDISP_Freeze(Device_p->DeviceData_p->DisplayHandle, &(Device_p->DeviceData_p->RequiredStopFreezeParams));

        /* Release access to display params */
        semaphore_signal(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);
    }

    stvid_LeaveCriticalSection(Device_p);
#endif /* ST_display */

} /* End of stvid_DecoderStopped() function  */
#endif /* ST_producer */



#ifdef ST_producer
/*******************************************************************************
Name        : stvid_DecodeOnceReady
Description : Function to subscribe the VIDPROD event DecodeOnce ready
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_DecodeOnceReady(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
    VideoStatus_t Status;
    STVID_Freeze_t Freeze;
    clock_t t1;
    U32 nbTicks;
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);
	UNUSED_PARAMETER(EventData_p);
    /* With DecodeOnce: should pause or freeze after the display of first picture */

    /* let enough time for vid_disp to display the decoded picture on either the top or bottm vsync*/
    nbTicks = ST_GetClocksPerSecond();
    t1 = time_now()+(60*nbTicks)/1000;
    while (time_now() < t1)       /* wait >30ms */
    {}

    stvid_EnterCriticalSection(Device_p);

#ifdef ST_display
    /* We need to inform the display that a discontinuity occured in MPEG frame
    continuity (only one picture in queue), so that it is not locked for ever. */
    VIDDISP_SetDiscontinuityInMPEGDisplay(Device_p->DeviceData_p->DisplayHandle);
#endif /* ST_display */

    /* Verify that state is still running: if it is not, cancel the event. */
    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_RUNNING)
    {
        /* First picture passed to display: now pause or freeze */
        Freeze.Mode     = STVID_FREEZE_MODE_FORCE; /* !!! ? this */
        Freeze.Field    = STVID_FREEZE_FIELD_CURRENT; /* !!! ? this */

        if (Device_p->DeviceData_p->StartParams.RealTime)
        {
            ActionFreeze(Device_p, &Freeze);
        }
        else
        {
            ActionPause(Device_p, &Freeze);
        }
    }

    stvid_LeaveCriticalSection(Device_p);
} /* End of stvid_DecodeOnceReady() function */
#endif /* ST_producer */


#ifdef ST_producer
/*******************************************************************************
Name        : stvid_NewPictureSkippedWithoutRequest
Description : Function to subscribe the VIDPROD event that a picture was skipped without request
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_NewPictureSkippedWithoutRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
    VideoStatus_t Status;
#if defined ST_trickmod || defined ST_speed
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#endif /* ST_trickmod || ST_speed*/
#if defined ST_display || defined ST_trickmod || defined ST_speed
    S32 Speed = 100;
#endif /* ST_display || ST_trickmod || ST_speed*/
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);
	UNUSED_PARAMETER(EventData_p);

    /* Verify that state is still running: if it is not, cancel the event. */
    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_RUNNING)
    {
#if defined ST_speed
        /* Get current play speed... */
        if (Device_p->DeviceData_p->SpeedHandleIsValid)
        {
            ErrorCode = VIDSPEED_GetSpeed(Device_p->DeviceData_p->SpeedHandle, &Speed);
            if (ErrorCode != ST_NO_ERROR)
            {
            	Speed = 100;
            }
        }
#endif /*ST_speed*/
#ifdef ST_trickmod
        /* Get current play speed... */
        if (Device_p->DeviceData_p->TrickModeHandleIsValid)
        {
            ErrorCode = VIDTRICK_GetSpeed(Device_p->DeviceData_p->TrickModeHandle, &Speed);
            if (ErrorCode != ST_NO_ERROR)
            {
            	Speed = 100;
            }
        }
#endif /* ST_trickmod */
#ifdef ST_display
        if (Speed == 100)
        {
            /* When speed is 100 (not handled by trickmodes), we care about this event */
            /* Indeed, we need to inform the display that a discontinuity occured in MPEG frame
            display order, so that it is not locked for ever. */
            VIDDISP_SetDiscontinuityInMPEGDisplay(Device_p->DeviceData_p->DisplayHandle);
        }
#endif /* ST_display */
    } /* running */

} /* End of stvid_NewPictureSkippedWithoutRequest() function */
#endif /* ST_producer */

#ifdef ST_producer
/*******************************************************************************
Name        : stvid_ReadyToDecodeNewPictureForceDecimationFactor
Description : Function called when a new decode is ready
Parameters  : 
Assumptions :
Limitations :
Returns     : 
*******************************************************************************/
static ST_ErrorCode_t stvid_ReadyToDecodeNewPictureForceDecimationFactor(VideoDevice_t * const Device_p, VIDPROD_NewPictureDecodeOrSkip_t *   NewPictureDecodeOrSkipEventData_p)
{
    STVID_PictureInfos_t *   PictureInfos_p = NewPictureDecodeOrSkipEventData_p->PictureInfos_p;
    ST_ErrorCode_t           ErrorCode = ST_NO_ERROR;
    STVID_DecimationFactor_t MemoryProfileDecimationFactor;

    /* ErrorCode = */ VIDBUFF_GetMemoryProfileDecimationFactor(Device_p->DeviceData_p->BuffersHandle, &MemoryProfileDecimationFactor);
    /* -------- Check if a Forcing of the decimation factor is pending. ------ */
    switch(Device_p->DeviceData_p->ForceDecimationFactor.State)
    {
        case FORCE_PENDING:
            /* Force decimation factor only if all the following pictures to be
               decoded has a bigger ID. This will avoid having mixed decimation
               factors within the same video sequence. */
            if(((NewPictureDecodeOrSkipEventData_p->IsExtendedPresentationOrderIDValid)&&
                (vidcom_ComparePictureID(&NewPictureDecodeOrSkipEventData_p->ExtendedPresentationOrderPictureID, &Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID) >= 0))||
               ((!NewPictureDecodeOrSkipEventData_p->IsExtendedPresentationOrderIDValid)&&
                ((PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)||((PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)))))
            {
                /* ErrorCode = */ VIDBUFF_ForceDecimationFactor(Device_p->DeviceData_p->BuffersHandle, Device_p->DeviceData_p->ForceDecimationFactor.DecimationFactor);
                Device_p->DeviceData_p->ForceDecimationFactor.FirstForcedPictureID = NewPictureDecodeOrSkipEventData_p->ExtendedPresentationOrderPictureID;
                Device_p->DeviceData_p->ForceDecimationFactor.State = FORCE_ONGOING;
            }
            break;
        case FORCE_ONGOING:
            /* First picture for forcing is found */
            /* Pictures with a smaller ID than first picture must be forced
               to the new decimation factor */
            if((NewPictureDecodeOrSkipEventData_p->IsExtendedPresentationOrderIDValid)&&
               (vidcom_ComparePictureID(&NewPictureDecodeOrSkipEventData_p->ExtendedPresentationOrderPictureID, &Device_p->DeviceData_p->ForceDecimationFactor.FirstForcedPictureID) < 0))
            {
                /* ErrorCode = */ VIDBUFF_ForceDecimationFactor(Device_p->DeviceData_p->BuffersHandle, Device_p->DeviceData_p->ForceDecimationFactor.LastDecimationFactor);
            }
            /* Pictures with a bigger ID than first picture must be forced
               to the new decimation factor */
            else 
            {
                /* ErrorCode = */ VIDBUFF_ForceDecimationFactor(Device_p->DeviceData_p->BuffersHandle, Device_p->DeviceData_p->ForceDecimationFactor.DecimationFactor);
            }
            break;
        default:
            break;
    }

    /* ------------------ Special case of adaptative decimation -------------- */ 
    if(ErrorCode == ST_NO_ERROR)
    {
        if ((MemoryProfileDecimationFactor & STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2) == STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2)
        {
            /* Force decimation factor according to picture's width */
            if (PictureInfos_p->BitmapParams.Width <= 1280)
            {
                /* ErrorCode = */ VIDBUFF_ForceDecimationFactor(Device_p->DeviceData_p->BuffersHandle, STVID_DECIMATION_FACTOR_H2);
            }
            else
            {
                /* ErrorCode = */ VIDBUFF_ForceDecimationFactor(Device_p->DeviceData_p->BuffersHandle, STVID_DECIMATION_FACTOR_H4);
            }
        }
    }

    /* ---------- Update the highest value of Picture ID ever seen ----------- */
    if((NewPictureDecodeOrSkipEventData_p->IsExtendedPresentationOrderIDValid)&&(vidcom_ComparePictureID(&NewPictureDecodeOrSkipEventData_p->ExtendedPresentationOrderPictureID, &Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID) > 0))
    {
        Device_p->DeviceData_p->HighestExtendedPresentationOrderPictureID = NewPictureDecodeOrSkipEventData_p->ExtendedPresentationOrderPictureID;
    }

    return(ErrorCode);
} /* end of stvid_ReadyToDecodeNewPictureForceDecimationFactor() function */
#endif /* ST_producer */

#ifdef ST_producer
/*******************************************************************************
Name        : stvid_ReadyToDecodeNewPicture
Description : Function to subscribe producer event 
              VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_ReadyToDecodeNewPicture(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
    VIDPROD_NewPictureDecodeOrSkip_t *   NewPictureDecodeOrSkipEventData_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_NewPictureDecodeOrSkip_t *, EventData_p);

	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    /* ErrorCode = */ stvid_ReadyToDecodeNewPictureErrorRecovery(Device_p);
    /* ErrorCode = */ stvid_ReadyToDecodeNewPictureForceDecimationFactor(Device_p, NewPictureDecodeOrSkipEventData_p);
} /* end of stvid_ReadyToDecodeNewPicture() function */
#endif /* ST_producer */

#if defined ST_trickmod || defined ST_speed
/*******************************************************************************
Name        : stvid_TrickmodeVsyncActionSpeed
Description : Function to subscribe display event VIDDISP_PRE_DISPLAY_VSYNC_EVT
              (notified each VSYNC before the display task begins working on
              display elements)
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_TrickmodeVsyncActionSpeed(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
#if defined ST_trickmod || defined ST_speed
    BOOL            IsReferenceDisplay = *((BOOL *) EventData_p);
#else
    UNUSED_PARAMETER(EventData_p);
#endif
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Inform Trickmode that a Vsync occured. */
#if defined ST_speed
    if (Device_p->DeviceData_p->SpeedHandleIsValid)
    {
        VIDSPEED_VsyncActionSpeed(Device_p->DeviceData_p->SpeedHandle, IsReferenceDisplay);
    }
#endif /*ST_speed*/
#ifdef ST_trickmod
    if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    {
        VIDTRICK_VsyncActionSpeed(Device_p->DeviceData_p->TrickModeHandle, IsReferenceDisplay);
	}
#endif /* ST_trickmod */
} /* end of stvid_TrickmodeVsyncActionSpeed() function */

/*******************************************************************************
Name        : stvid_TrickmodePostVsyncAction
Description : Function to subscribe display event VIDDISP_PRE_DISPLAY_VSYNC_EVT
              (notified each VSYNC after the display task has finished working
              on display elements)
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_TrickmodePostVsyncAction(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
#if defined ST_trickmod || defined ST_speed
    BOOL            IsReferenceDisplay = *((BOOL *) EventData_p);
#else
    UNUSED_PARAMETER(EventData_p);
#endif

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Inform Trickmode that a Vsync occured. */
#if defined ST_speed
    if (Device_p->DeviceData_p->SpeedHandleIsValid)
    {
        VIDSPEED_PostVsyncAction(Device_p->DeviceData_p->SpeedHandle, IsReferenceDisplay);
    }
#endif /*ST_speed*/
#ifdef ST_trickmod
    if (Device_p->DeviceData_p->TrickModeHandleIsValid)
    {
        VIDTRICK_PostVsyncAction(Device_p->DeviceData_p->TrickModeHandle, IsReferenceDisplay);
	}
#endif /* ST_trickmod */
} /* end of stvid_TrickmodePostVsyncAction() function */

/*******************************************************************************
Name        : stvid_TrickmodeDisplayParamsChange
Description : Function to subscribe the VIDTRICK event that notifies display params change
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
void stvid_TrickmodeDisplayParamsChange(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    /* Ensure nobody else accesses display params */
    semaphore_wait(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

    /* Store trickmode display params */
    Device_p->DeviceData_p->DisplayParamsForTrickMode = *((VIDDISP_DisplayParams_t *) EventData_p);

    /* Check if there some change and send to VIDDISP. */
    CheckDisplayParamsAndUpdate(Device_p);

    /* Release access to display params */
    semaphore_signal(Device_p->DeviceData_p->DisplayParamsAccessSemaphore_p);

} /* end of stvid_TrickmodeDisplayParamsChange() function */
#endif /* ST_trickmod || ST_speed */

#ifdef ST_producer
/*******************************************************************************
Name        : AffectTheBuffersFreeingStrategy
Description : Return the strategy to choose for the freeing of the Frame Buffers.
Parameters  : Device type, Nb FB, Decoded Pict
Assumptions :
Limitations :
Returns     : TRUE if the freeing of the fb has to be optimized, FALSE otherwise
*******************************************************************************/
ST_ErrorCode_t AffectTheBuffersFreeingStrategy(
        const STVID_DeviceType_t DeviceType,
        const U8 NbFrameStore,
        const STVID_DecodedPictures_t DecodedPictures,
        VIDBUFF_StrategyForTheBuffersFreeing_t * const StrategyForTheBuffersFreeing_p)
{
    BOOL IsTheBuffersFreeingOptimized;

    /* init the tmp variable. */
    IsTheBuffersFreeingOptimized = FALSE;

    switch (DeviceType)
    {
        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
        case STVID_DEVICE_TYPE_5528_MPEG :
        case STVID_DEVICE_TYPE_7100_MPEG4P2 :
        case STVID_DEVICE_TYPE_7100_H264 :
        case STVID_DEVICE_TYPE_7109_MPEG4P2 :
        case STVID_DEVICE_TYPE_7109_AVS :
        case STVID_DEVICE_TYPE_7109_H264 :
        case STVID_DEVICE_TYPE_7109_VC1 :
        case STVID_DEVICE_TYPE_7109D_MPEG :
        case STVID_DEVICE_TYPE_7200_MPEG4P2 :
        case STVID_DEVICE_TYPE_7200_H264 :
        case STVID_DEVICE_TYPE_7200_VC1 :
        case STVID_DEVICE_TYPE_7200_MPEG :

            /* Feature Overwrite not supported by the 5528, 5100 and 7100 H264. */
            IsTheBuffersFreeingOptimized = FALSE;
            break;

        default :
            switch (DecodedPictures)
            {
                case STVID_DECODED_PICTURES_ALL :
                    if (NbFrameStore == STVID_MIN_BUFFER_DECODE_ALL_OVW)
                    {
                        /* Feature Overwrite necessary in this case */
                        IsTheBuffersFreeingOptimized = TRUE;
                    }
                    break;

                case STVID_DECODED_PICTURES_IP :
                    if (NbFrameStore == STVID_MIN_BUFFER_DECODE_IP)
                    {
                        /* Feature Overwrite unuseful */
                        IsTheBuffersFreeingOptimized = FALSE;
                    }
                    break;

                case STVID_DECODED_PICTURES_I :
                    if (NbFrameStore == STVID_MIN_BUFFER_DECODE_I_OVW)
                    {
                        /* Feature Overwrite necessary in this case */
                        IsTheBuffersFreeingOptimized = TRUE;
                    }
                    break;

                default :
                    break;
            }
            break;
    }

    /* Buffers freeing affectation. */
    StrategyForTheBuffersFreeing_p->IsTheBuffersFreeingOptimized = IsTheBuffersFreeingOptimized;

    return(ST_NO_ERROR);
} /* end of AffectTheBuffersFreeingStrategy */

/*******************************************************************************
Name        : GetMinimalNumberOfBuffers
Description : Return the minimal number of buffers, according to the framebuffer
              hold time and the devicetype
Parameters  : device type, framebuffer hold time
Assumptions :
Limitations :
Returns     : minimal number of buffers to decode I, IP or all the pictures
*******************************************************************************/
static void GetMinimalNumberOfBuffers(const STVID_DeviceType_t  DeviceType,
                                      const U8                  FrameBufferHoldTime,
                                      U8* const                 MinBufferDecodeI_p,
                                      U8* const                 MinBufferDecodeIP_p,
                                      U8* const                 MinBufferDecodeAll_p)
{
    switch(DeviceType)
    {
        case STVID_DEVICE_TYPE_5528_MPEG :
        case STVID_DEVICE_TYPE_5100_MPEG :
        case STVID_DEVICE_TYPE_5301_MPEG :
            /* No OVW feature */
            (*MinBufferDecodeI_p)    = STVID_MIN_BUFFER_DECODE_I_noOVW;
            (*MinBufferDecodeIP_p)   = STVID_MIN_BUFFER_DECODE_IP;
            (*MinBufferDecodeAll_p)  = STVID_MIN_BUFFER_DECODE_ALL_noOVW;
            break;
#ifdef STVID_STVAPI_ARCHITECTURE
        case STVID_DEVICE_TYPE_STD2000_MPEG :
            (*MinBufferDecodeI_p)    = STVID_MIN_BUFFER_DECODE_I_RASTER;
            (*MinBufferDecodeIP_p)   = STVID_MIN_BUFFER_DECODE_IP_RASTER;
            (*MinBufferDecodeAll_p)  = STVID_MIN_BUFFER_DECODE_ALL_RASTER;
            break;
#endif /* STVID_STVAPI_ARCHITECTURE */
        default:
            /* the other chips have the OVW feature */
            (*MinBufferDecodeI_p)    = STVID_MIN_BUFFER_DECODE_I_OVW;
            (*MinBufferDecodeIP_p)   = STVID_MIN_BUFFER_DECODE_IP;
            (*MinBufferDecodeAll_p)  = STVID_MIN_BUFFER_DECODE_ALL_OVW;
            break;
    }
    /* Add an additional frame buffer if the framebuffer hold time is more than 2 VSYNCs */
    if(FrameBufferHoldTime > MIN_VSYNCS_NUMBER_FOR_FRAME_BUFFER_HOLD_TIME)
    {
        (*MinBufferDecodeI_p)++;
        (*MinBufferDecodeIP_p)++;
        (*MinBufferDecodeAll_p)++;
    }
} /* end of function GetMinimalNumberOfBuffers() */

/*******************************************************************************
Name        : AffectDecodedPictures
Description : Tries to adjust the decoded pictures
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no errors happened, ST_ERROR_BAD_PARAMETER otherwise
*******************************************************************************/
ST_ErrorCode_t AffectDecodedPictures(VideoDevice_t * const Device_p,
                                               const STVID_MemoryProfile_t * const MemoryProfile_p,
                                               const U8 FrameBufferHoldTime,
                                               STVID_DecodedPictures_t* const  DecodedPictures_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8 MinBufferDecodeI, MinBufferDecodeIP, MinBufferDecodeAll;
    U8 NbMainFrameBuffers;

    /* Get the number of the set main frame buffers */
    NbMainFrameBuffers = GetNbMainFrameBuffers(MemoryProfile_p);

    /* Get the minimal number of buffers for decoding only I, IP or IPB pictures */
    GetMinimalNumberOfBuffers(Device_p->DeviceData_p->DeviceType,
                              FrameBufferHoldTime,
                              &MinBufferDecodeI,
                              &MinBufferDecodeIP,
                              &MinBufferDecodeAll);

    /* If not enough buffer to decode all kind of pictures, set the suitable Decoded pictures */
    if (NbMainFrameBuffers < MinBufferDecodeAll)
    {
        /* Check if current DecodedPictures is still possible with new profile. If not, reduce DecodedPictures */
        if (NbMainFrameBuffers < MinBufferDecodeI)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else if (NbMainFrameBuffers < MinBufferDecodeIP)
        {
            /* only enough buffers to decode I pictures ==> Set Decoded Pictures to I */
            if ((*DecodedPictures_p) != STVID_DECODED_PICTURES_I)
            {
                (*DecodedPictures_p) = STVID_DECODED_PICTURES_I;
                ErrorCode = VIDPROD_SetDecodedPictures(Device_p->DeviceData_p->ProducerHandle, (*DecodedPictures_p));
            }
        }
        else
        {
            /* only enough buffers to decode I and P pictures ==> Set Decoded Pictures to I */
            if ((*DecodedPictures_p) == STVID_DECODED_PICTURES_ALL)
            {
                (*DecodedPictures_p) = STVID_DECODED_PICTURES_IP;
                ErrorCode = VIDPROD_SetDecodedPictures(Device_p->DeviceData_p->ProducerHandle, (*DecodedPictures_p));
            }
        }
        if (ErrorCode != ST_NO_ERROR)
        {
            /* DecodedPictures reduction failed, so refuse to set profile */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_SetDecodedPictures failed"));
        }
    } /* if (MemoryProfile_p->NbFrameStore < MinBufferDecodeAll) */

    return(ErrorCode);
} /* end of function AffectDecodedPictures() */
#endif /* ST_producer */

#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        : STVID_SetupReservedPartitionForH264PreprocWA_GNB42332
Description : Reallocates GNBvd42332 WA data into passed partition
Parameters  : Handle, Partition where the data will be reallocated.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no errors happened,
              ST_ERROR_FEATURE_NOT_SUPPORTED if not H264 device,
              STVID_ERROR_DECODER_RUNNING if decoder not stopped
              ST_ERROR_BAD_PARAMETER otherwise
*******************************************************************************/
ST_ErrorCode_t STVID_SetupReservedPartitionForH264PreprocWA_GNB42332(const STVID_Handle_t Handle, const STAVMEM_PartitionHandle_t AVMEMPartition)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VideoStatus_t  Status;

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (AVMEMPartition == (STAVMEM_PartitionHandle_t) NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)Handle)->Device_p;

    stvid_EnterCriticalSection(Device_p);

    Status = GET_VIDEO_STATUS(Device_p);
    if (Status != VIDEO_STATUS_STOPPED)
    {
        ErrorCode = STVID_ERROR_DECODER_RUNNING;
    }
    else
    {
        if (!Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
        {
            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
        else
        {
            ErrorCode = VIDPROD_SetupForH264PreprocWA_GNB42332(((VIDPROD_Properties_t *)(Device_p->DeviceData_p->ProducerHandle))->InternalProducerHandle, AVMEMPartition);
        }
    }

    stvid_LeaveCriticalSection(Device_p);

    return(ErrorCode);
} /* End of STVID_SetupReservedPartitionForH264PreprocWA_GNB42332() function */
#endif /* DVD_SECURED_CHIP */

/*******************************************************************************
Name        : STVID_GetBitBufferParams
Description : Get the bit buffer info.
Parameters  : Video handle, BaseAddress, Size.
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STVID_GetBitBufferParams(const STVID_Handle_t Handle, void ** const BaseAddress_p, U32 * const InitSize_p)
{
    VideoDevice_t * Device_p;

    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ( (BaseAddress_p == NULL) || (InitSize_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *) Handle)->Device_p;

    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {

        ErrorCode = VIDPROD_GetBitBufferParams(Device_p->DeviceData_p->ProducerHandle, BaseAddress_p,InitSize_p);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_GetBitBufferParams() : failed !"));
        }

    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }


    return(ErrorCode);

} /* End of STVID_GetBitBufferParams() function */

/* End of vid_deco.c */
