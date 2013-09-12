/*******************************************************************************

File name   : vid_buf.c

Description :

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 23 Jun 2000        Created                                          AN
 07 Aug 2002        STVID_GetPictureBuffer() modification. For
                    digital input, only "B" picture are wanted.
 05 Oct 2004        Changed for new vid_com.h structures             CL
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */


#include "stddefs.h"
#include "stvid.h"
#include "api.h"

/* Public functions --------------------------------------------------------- */
/*
   STVID_GetPictureBuffer
   STVID_DisplayPictureBuffer
   STVID_ReleasePictureBuffer
   STVID_TakePictureBuffer
*/

/* Private Constants -------------------------------------------------------- */
/* For Colour Primaries */
#define UNSPECIFIED_COLOUR_PRIMARIES 2
/* For Transfer Characteristics */
#define UNSPECIFIED_TRANSFER_CHARACTERISTICS 2
/* For Matrix Coefficients */
#define UNSPECIFIED_MATRIX_COEFFICIENTS 2

/*
--------------------------------------------------------------------------------
Get a Picture buffer
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_GetPictureBuffer(const STVID_Handle_t  Handle,
               const STVID_GetPictureBufferParams_t * const Params_p,
               STVID_PictureBufferDataParams_t * const PictureBufferParams_p,
               STVID_PictureBufferHandle_t * const PictureBufferHandle_p)
{
    ST_ErrorCode_t ErrorCode;
    VideoDevice_t*                          Device_p;
    VIDBUFF_GetUnusedPictureBufferParams_t  GetUnusedPictureBufferParams;
    VIDBUFF_PictureBuffer_t*                PictureBuffer_p;
    VIDCOM_InternalProfile_t                MemoryProfile;
    U8      panandscan;
#if defined(ST_multicodec)
    U32     counter;
#endif /* ST_multicodec */

    if ((Params_p == NULL) || (PictureBufferParams_p == NULL)
                           || (PictureBufferHandle_p == NULL))
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

    if ( (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT)   ||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)   ||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT))
    {
        /* GGi (07/08/2002).                                                    */
        /* Set the default value of MPEGFrame : "STVID_MPEG_FRAME_B".           */
        /* When using only 2 buffers, if the frame type is not "B", Di input    */
        /* could be ahead of buffer liberation by display -> troubleshootings.  */
        GetUnusedPictureBufferParams.MPEGFrame                  = STVID_MPEG_FRAME_B;
    }
    else
    {
        GetUnusedPictureBufferParams.MPEGFrame                  = STVID_MPEG_FRAME_I;
    }
    GetUnusedPictureBufferParams.PictureStructure
        = Params_p->PictureStructure;
    GetUnusedPictureBufferParams.TopFieldFirst
        = Params_p->TopFieldFirst;
    GetUnusedPictureBufferParams.ExpectingSecondField
        = Params_p->ExpectingSecondField;
    GetUnusedPictureBufferParams.DecodingOrderFrameID
        = Params_p->ExtendedTemporalReference;
    GetUnusedPictureBufferParams.PictureWidth
        = Params_p->PictureWidth;
    GetUnusedPictureBufferParams.PictureHeight
        = Params_p->PictureHeight;

     VIDBUFF_GetMemoryProfile(Device_p->DeviceData_p->BuffersHandle, &MemoryProfile);
    /* Call to VIDBUFF_GetUnusedPictureBuffer function.
     * All parameters already tested, no need to check function return value! */
    if((((Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT)   ||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT)   ||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT)||
         (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT))
            &&(MemoryProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE))
       || (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)
       || (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)
       || (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)
       || (Device_p->DeviceData_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
    {
        ErrorCode = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(Device_p->DeviceData_p->BuffersHandle, &GetUnusedPictureBufferParams, &PictureBuffer_p, VIDCOM_APPLICATION_MODULE_BASE);
    }
    else
    {
        ErrorCode = VIDBUFF_GetAndTakeUnusedPictureBuffer(Device_p->DeviceData_p->BuffersHandle, &GetUnusedPictureBufferParams, &PictureBuffer_p, VIDCOM_APPLICATION_MODULE_BASE);
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        PictureBuffer_p->AssociatedDecimatedPicture_p = NULL;

        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID = 0;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension = Params_p->ExtendedTemporalReference;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.NumberOfPanAndScan = 0;
        for (panandscan = 0; panandscan < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; panandscan++)
        {
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset = 0;
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset = 0;
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].DisplayHorizontalSize = 0;
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].DisplayVerticalSize = 0;
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].HasDisplaySizeRecommendation = FALSE;
        }
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField = FALSE;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.RepeatProgressiveCounter = 0;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields = Params_p->ExpectingSecondField;

        PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p = &(PictureBuffer_p->PictureDecodingContext);
        PictureBuffer_p->ParsedPictureInformation.SizeOfPictureSpecificDataInByte = 0; /* see next what to put inside specfic to MPEG parser/decoder */
        PictureBuffer_p->ParsedPictureInformation.PictureSpecificData_p = PictureBuffer_p->PictureSpecificData;
        PictureBuffer_p->PictureDecodingContext.GlobalDecodingContext_p = &(PictureBuffer_p->GlobalDecodingContext);

        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure = Params_p->PictureStructure;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = Params_p->TopFieldFirst;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width = Params_p->PictureWidth;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height = Params_p->PictureHeight;
        /* CL: don't know how to set the remaining of PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos */
        /* This is not really a problem because STVIN will set the whole PictureInfos structure at STVID_DisplayPictureBuffer() call */

        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = Params_p->ExtendedTemporalReference;

        /*--------------------------------------------------------------------------*/
        /*    not used in old viddec (taken from old decode viddec_p structure)     */
        /*--------------------------------------------------------------------------*/
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsReference = FALSE;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsDisplayBoundPictureIDValid = TRUE;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DisplayBoundPictureID.ID = 0;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DisplayBoundPictureID.IDExtension = Params_p->ExtendedTemporalReference;

#if defined(ST_multicodec)
        for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; counter++)
        {
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.FullReferenceFrameList[counter] = 0;
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsValidIndexInReferenceFrame[counter] = FALSE;
        }
        for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; counter++)
        {
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ReferenceListP0[counter] = 0;
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ReferenceListB0[counter] = 0;
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ReferenceListB1[counter] = 0;
        }
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.UsedSizeInReferenceListP0 = 0;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.UsedSizeInReferenceListB0 = 0;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.UsedSizeInReferenceListB1 = 0;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.BitBufferPictureStartAddress = NULL;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress = NULL;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.FlushPresentationQueue = FALSE;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.FreezePresentationOnThisFrame = FALSE;
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.ResumePresentationOnThisFrame = FALSE;
#endif /* ST_multicodec */

    /*    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodeStartTime;*/
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsDecodeStartTimeValid = FALSE;
    /*    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PresentationStartTime;*/
        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsPresentationStartTimeValid = FALSE;

        /*PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo = ???;*/
        /*--------------------------------------------------------------------------*/
        /* End of not used in old viddec (taken from old decode viddec_p structure) */
        /*--------------------------------------------------------------------------*/

        PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;

        PictureBuffer_p->GlobalDecodingContext.SizeOfGlobalDecodingContextSpecificDataInByte = 0;
        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextSpecificData_p = PictureBuffer_p->GlobalDecodingContextSpecificData;

        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.ColourPrimaries = UNSPECIFIED_COLOUR_PRIMARIES;
        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.TransferCharacteristics = UNSPECIFIED_TRANSFER_CHARACTERISTICS;
        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MatrixCoefficients = UNSPECIFIED_MATRIX_COEFFICIENTS;

        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.LeftOffset = 0;
        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.RightOffset = 0;
        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.TopOffset = 0;
        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset = 0;

        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.NumberOfReferenceFrames = 0;
        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MaxDecFrameBuffering = 0;

        PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;

        /* Set Picture Buffer handle */
        *PictureBufferHandle_p  = (STVID_PictureBufferHandle_t) PictureBuffer_p;

        /* Set Picture Buffer parameters according to the available reconstruction mode.    */
        PictureBufferParams_p->Data1_p  = PictureBuffer_p->FrameBuffer_p->Allocation.Address_p;
        PictureBufferParams_p->Size1    = PictureBuffer_p->FrameBuffer_p->Allocation.TotalSize -
                                          PictureBuffer_p->FrameBuffer_p->Allocation.Size2;
        PictureBufferParams_p->Data2_p  = PictureBuffer_p->FrameBuffer_p->Allocation.Address2_p;
        PictureBufferParams_p->Size2    = PictureBuffer_p->FrameBuffer_p->Allocation.Size2;
    }

    return(ErrorCode);
} /* End of STVID_GetPictureBuffer() function */


/*
--------------------------------------------------------------------------------
Display a Picture buffer
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_DisplayPictureBuffer(const STVID_Handle_t                  Handle,
                                          const STVID_PictureBufferHandle_t     PictureBufferHandle,
                                          const STVID_PictureInfos_t*           const PictureInfos_p)
{
    VIDBUFF_PictureBuffer_t *               PictureBuffer_p;
    VideoDevice_t                         * Device_p;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;

    if ((PictureInfos_p == NULL) || (PictureBufferHandle == (STVID_PictureBufferHandle_t)NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check PictureBufferHande to be valid */

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)Handle)->Device_p;

    PictureBuffer_p = (VIDBUFF_PictureBuffer_t *) PictureBufferHandle;

    /* Update picture buffer */
    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos  = *PictureInfos_p;
    PictureBuffer_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_DECODED;
    PictureBuffer_p->Decode.DecodingError         = VIDCOM_ERROR_LEVEL_NONE;

    /* Update field in PictureInfos */
    vidcom_IncrementTimeCode(&(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TimeCode),
                             PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.FrameRate,
                             FALSE);                                               /* TBDone by STVIN ? */

#ifdef ST_ordqueue
    /* Call to VIDQUEUE_InsertPictureInQueue function.
     * All parameters already tested, no need to check function return value! */
    ErrorCode = VIDQUEUE_InsertPictureInQueue(Device_p->DeviceData_p->OrderingQueueHandle,
                                              PictureBuffer_p,
                                              VIDQUEUE_INSERTION_ORDER_LAST_PLACE);
    /* We can release the picture and the decimated picture if exists right now as it is under diplay's control */
    RELEASE_PICTURE_BUFFER(Device_p->DeviceData_p->BuffersHandle, PictureBuffer_p, VIDCOM_APPLICATION_MODULE_BASE);
    if (PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
    {
        RELEASE_PICTURE_BUFFER(Device_p->DeviceData_p->BuffersHandle, PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_APPLICATION_MODULE_BASE);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Push directly picture to display */
        ErrorCode = VIDQUEUE_PushPicturesToDisplay(Device_p->DeviceData_p->OrderingQueueHandle,
                         &(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));
    }
#else /* ST_ordqueue */
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif /* not ST_ordqueue */

    return(ErrorCode);
} /* End of STVID_DisplayPictureBuffer() function */


/*
--------------------------------------------------------------------------------
Release a Picture buffer
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_ReleasePictureBuffer(const STVID_Handle_t Handle,
                const STVID_PictureBufferHandle_t     PictureBufferHandle)
{
    ST_ErrorCode_t ErrorCode;
    VideoDevice_t*                          Device_p;
    VIDBUFF_PictureBuffer_t *               PictureBuffer_p;

    if (PictureBufferHandle == (STVID_PictureBufferHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = ((VideoUnit_t *)Handle)->Device_p;
    PictureBuffer_p = (VIDBUFF_PictureBuffer_t *) PictureBufferHandle;

    DEBUG_CHECK_PICTURE_BUFFER_VALIDITY(Device_p->DeviceData_p->BuffersHandle, PictureBuffer_p);
    ErrorCode = RELEASE_PICTURE_BUFFER(Device_p->DeviceData_p->BuffersHandle, PictureBuffer_p, VIDCOM_APPLICATION_MODULE_BASE);

    return(ErrorCode);
} /* end of STVID_ReleasePictureBuffer() function */

/*
--------------------------------------------------------------------------------
Take a Picture buffer
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STVID_TakePictureBuffer(const STVID_Handle_t Handle,
                        const STVID_PictureBufferHandle_t PictureBufferHandle)
{
    VideoDevice_t           * Device_p;
    ST_ErrorCode_t          ErrorCode;
    VIDBUFF_PictureBuffer_t * PictureBuffer_p;

    if (PictureBufferHandle == (STVID_PictureBufferHandle_t)NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }
    Device_p        = ((VideoUnit_t *)Handle)->Device_p;
    PictureBuffer_p = (VIDBUFF_PictureBuffer_t *) PictureBufferHandle;

    DEBUG_CHECK_PICTURE_BUFFER_VALIDITY(Device_p->DeviceData_p->BuffersHandle, PictureBuffer_p);
    ErrorCode = TAKE_PICTURE_BUFFER(Device_p->DeviceData_p->BuffersHandle, PictureBuffer_p, VIDCOM_APPLICATION_MODULE_BASE);

    return(ErrorCode);
} /* end of STVID_TakePictureBuffer() function */

ST_ErrorCode_t STVID_PrintPictureBuffersStatus(const STVID_Handle_t Handle)
{
    VideoDevice_t           * Device_p;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(Handle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = ((VideoUnit_t *)Handle)->Device_p;

    VIDBUFF_PrintPictureBuffersStatus(Device_p->DeviceData_p->BuffersHandle);

    return(ErrorCode);
} /* end of STVID_PrintPictureBuffersStatus() function */

/* end of vid_pict.c */
