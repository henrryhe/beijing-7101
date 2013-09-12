/*******************************************************************************

File name   : vid_pict.c

Description : Video picture manipulation functions commands source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
 23 Jun 2000        Created                                          AN
 22 Sep 2000        Implemented DuplicatePicture(), GetPictureParams()
                    and GetPictureAllocParams().
                    Add handle to GetPictureAllocParams()            HG
 07 Aug 2002        STVID_GetPictureBuffer() modification. For
                    digital input, only "B" picture are wanted.      GG
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"

#include "api.h"

#include "vid_ctcm.h"

#include "stavmem.h"

/* Public functions --------------------------------------------------------- */

/*
   STVID_DuplicatePicture
   STVID_GetPictureAllocParams
   STVID_GetPictureParams
*/


/*
--------------------------------------------------------------------------------
Copy a picture to a user defined area.

Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if parameter problem
              STVID_ERROR_MEMORY_ACCESS if memory copy fails
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_DuplicatePicture(const STVID_PictureParams_t * const Source_p,
        STVID_PictureParams_t * const Dest_p)
{
    ST_ErrorCode_t ErrorCode;
    void * TmpData;

    if ((Source_p == NULL) || (Dest_p == NULL))
    {
        /* Can't do anything if pointer is NULL */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Copy picture */
    ErrorCode = STAVMEM_CopyBlock1D(Source_p->Data, Dest_p->Data, Source_p->Size);

    if ( ErrorCode != ST_NO_ERROR)
    {
        return(STVID_ERROR_MEMORY_ACCESS);
    }

    /* Copy structure parameters */
    TmpData = Dest_p->Data;
    *Dest_p = *Source_p;
    Dest_p->Data = TmpData;

    return(ST_NO_ERROR);
}


/*
--------------------------------------------------------------------------------
Retrieves the parameters required to allocate the space to store the specified picture.
Fills only fields Size and Alignment of the STAVMEM_AllocBlockParams_t structure
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_GetPictureAllocParams(const STVID_Handle_t VideoHandle,
        const STVID_PictureParams_t * const Params_p,
        STAVMEM_AllocBlockParams_t * const AllocParams_p)
{
    VideoDevice_t * Device_p;

    if ((Params_p == NULL) || (AllocParams_p == NULL))
    {
        /* Can't return data if pointer is NULL */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    VIDBUFF_GetPictureAllocParams(Device_p->DeviceData_p->BuffersHandle, Params_p->Width, Params_p->Height, &(AllocParams_p->Size), &(AllocParams_p->Alignment));

    return(ST_NO_ERROR);
} /* End of STVID_GetPictureAllocParams() function */


/*
--------------------------------------------------------------------------------
Gets informations on a given picture.
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_GetPictureInfos(const STVID_Handle_t VideoHandle, const STVID_Picture_t PictureType, STVID_PictureInfos_t * const PictureInfos_p)
{
    VideoDevice_t * Device_p;
    VideoStatus_t   Status;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (PictureInfos_p == NULL)
    {
        /* Can't return data if pointer is NULL */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid: point to device */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    /* Check the video decoder status. */
    Status = GET_VIDEO_STATUS(Device_p);
    if (Status == VIDEO_STATUS_RUNNING)
    {
        ErrorCode = STVID_ERROR_DECODER_RUNNING;
    }
    else
    {
        switch (PictureType)
        {
#ifdef ST_producer
            case STVID_PICTURE_LAST_DECODED :
                if (Device_p->DeviceData_p->ProducerHandleIsValid)
                {
                    ErrorCode = VIDPROD_GetLastDecodedPictureInfos(Device_p->DeviceData_p->ProducerHandle, PictureInfos_p);
                }
                else
                {
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                }
                break;
#endif /* #ifdef ST_producer */

#ifdef ST_display
            case STVID_PICTURE_DISPLAYED :
                ErrorCode = VIDDISP_GetDisplayedPictureInfos(Device_p->DeviceData_p->DisplayHandle, PictureInfos_p);
                break;
#endif /* ST_display */

            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Convert addresses according to STAVMEM mapping */
        PictureInfos_p->BitmapParams.Data1_p = DeviceToVirtual(PictureInfos_p->BitmapParams.Data1_p, Device_p->DeviceData_p->VirtualMapping);
        PictureInfos_p->BitmapParams.Data2_p = DeviceToVirtual(PictureInfos_p->BitmapParams.Data2_p, Device_p->DeviceData_p->VirtualMapping);
    }
    return(ErrorCode);
}

/*
---------------------------------------------------------------------------------------------
Gets informations on a given picture. (obsolete : shouldn't be used any more by applications)
---------------------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_GetPictureParams(const STVID_Handle_t VideoHandle,
        const STVID_Picture_t PictureType, STVID_PictureParams_t * const Params_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STVID_PictureInfos_t    PictureInfos;

    if (Params_p == NULL)
    {
        /* Can't return data if pointer is NULL */
        return(ST_ERROR_BAD_PARAMETER);
    }
    ErrorCode = STVID_GetPictureInfos(VideoHandle, PictureType, &PictureInfos);
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Convert to old format of picture infos, TemporalReference info is unreachable so put 0 */
        vidcom_PictureInfosToPictureParams(&PictureInfos, Params_p, 0);
    }
    return(ErrorCode);
} /* End of STVID_GetPictureParams() function */


/*
---------------------------------------------------------------------------------------------
Gets PAN&SCAN informations on a given picture.
---------------------------------------------------------------------------------------------
*/

ST_ErrorCode_t STVID_GetDisplayPictureInfo(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t PictureBufferHandle, STVID_DisplayPictureInfos_t * const DisplayPictureInfos_p)
{

    VIDBUFF_PictureBuffer_t*                PictureBuffer_p;
    VideoDevice_t * Device_p;

    if ((DisplayPictureInfos_p == NULL) || (PictureBufferHandle == (STVID_PictureBufferHandle_t)NULL))
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

	PictureBuffer_p = (VIDBUFF_PictureBuffer_t *) PictureBufferHandle;

    /** Be aware that the size of the structure VIDCOM_PanAndScanIn16thPixel_t and VIDCOM_FrameCropInPixel_t and the internal
       structure  PanAndScanIn16thPixel and FrameCropInPixel defined in stvid.h should be the same  **/

    STOS_memcpy(&(DisplayPictureInfos_p->PanAndScanIn16thPixel),PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel,
                    STVID_MAX_NUMBER_OF_PAN_AND_SCAN * sizeof(VIDCOM_PanAndScanIn16thPixel_t));

    STOS_memcpy(&(DisplayPictureInfos_p->FrameCropInPixel),&(PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel), sizeof(VIDCOM_FrameCropInPixel_t));

    DisplayPictureInfos_p->NumberOfPanAndScan = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.NumberOfPanAndScan;

    return(ST_NO_ERROR);
}


