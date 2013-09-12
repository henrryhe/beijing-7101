/*******************************************************************************

File name   : vid_mb2r.c

Description : Video Macroblock to Raster and Raster buffer Manager
              use source file.

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
24 dec 2004        Created                                          PLe
*******************************************************************************/



/* Includes ----------------------------------------------------------------- */

#include <string.h>

#include "stddefs.h"

#include "dtvdefs.h"
#include "stgvobj.h"
#include "stavmem.h"
#include "dv_rbm.h"

#include "vid_mpeg.h"

#include "vid_dec.h"

#include "vid_mb2r.h"



/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t FillRasterPictureStructure(VIDDEC_Data_t * VIDDEC_Data_p,
    STGVOBJ_Picture_t * FieldPicture_p,
    STGVOBJ_PictureStructure_t ePictureStructure,
    const STGVOBJ_DecimationFactor_t eDecimation);

static ST_ErrorCode_t GetMainRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p, DVRBM_PictureGetUseFuncParams_t * pGetUseFuncParams);
static ST_ErrorCode_t GetDecimatedRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p, DVRBM_PictureGetUseFuncParams_t * pGetUseFuncParams);

/* Functions ---------------------------------------------------------------- */



/*******************************************************************************
Name        : viddec_GetRasterPictures
Description : Gets Pictures From the raster buffer manager, with their associated identifiers.
Parameters  : IN: video decode handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t viddec_GetRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* init local variables */
    DVRBM_PictureGetUseFuncParams_t RasterPictureGetParams;

    /* unuse the locked pictures in case the pipeline or the decoder have been reseted after */
    /* errors on the previous pictures */
    viddec_UnuseRasterPictures(VIDDEC_Data_p);

    /* mutex_lock(VIDDEC_Data_p->RasterBufManagerParams.RasterBuffersMutex_p); */

    /*******************************************/
    /* configuration of RasterPictureGetParams */
    /*******************************************/

    /* Prepare the parameters to get the convenient STGVOBJ_PictureId_t */
    RasterPictureGetParams.ulWidth = VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Width;
    RasterPictureGetParams.ulHeight = VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Height;

    if (VIDDEC_Data_p->ForTask.PictureStreamInfo.IsBitStreamMPEG1)
    {
        /* MPEG 1 : first field is always top, never First field top be repeated */
        RasterPictureGetParams.bRepeatFieldNeeded = FALSE;
        RasterPictureGetParams.bRepeatFieldIsTop = FALSE;
    }
    else
    {
        RasterPictureGetParams.bRepeatFieldNeeded =
            VIDDEC_Data_p->ForTask.StreamInfoForDecode.PictureCodingExtension.repeat_first_field;
        if (RasterPictureGetParams.bRepeatFieldNeeded) /* can be TRUE only in case of frame pictures */
        {
            RasterPictureGetParams.bRepeatFieldIsTop =
                VIDDEC_Data_p->ForTask.StreamInfoForDecode.PictureCodingExtension.top_field_first;
        }
        else
        {
             /* No field to be repeated : bRepeatFieldIsTop has no meaning. To be set to FALSE */
             RasterPictureGetParams.bRepeatFieldIsTop = FALSE;
        }
    }

    switch (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PictureStructure)
    {
        case STVID_PICTURE_STRUCTURE_FRAME :
            RasterPictureGetParams.ePictureStructure = STGVOBJ_kPictureStructureFrame;
            if (VIDDEC_Data_p->ForTask.PictureStreamInfo.IsBitStreamMPEG1)
            {
              /* MPEG 1 :pictures are always frame; top is always first field */
              VIDDEC_Data_p->RasterBufManagerParams.FirstFieldIsTop = TRUE;
            }
            else
            {
              if (VIDDEC_Data_p->ForTask.StreamInfoForDecode.PictureCodingExtension.progressive_frame)
              {
                /* if progressive top field first has no meaning : set it to TRUE in order to respect the order */
                /* of the storage in memory */
                VIDDEC_Data_p->RasterBufManagerParams.FirstFieldIsTop = TRUE;
              }
              else
              {
                /* MPEG 2 : store the TopFirstField flag for next New Decoded Picture evt send attached infos management */
                VIDDEC_Data_p->RasterBufManagerParams.FirstFieldIsTop = VIDDEC_Data_p->ForTask.StreamInfoForDecode.PictureCodingExtension.top_field_first;
              }
            }
            break;
        case STVID_PICTURE_STRUCTURE_TOP_FIELD :
            RasterPictureGetParams.ePictureStructure = STGVOBJ_kPictureStructureTopField;
            /* FirstFieldIsTop has to be set to TRUE even in case of a top field with the top_field_first flag of
            the picture coding extension to  0 */
            VIDDEC_Data_p->RasterBufManagerParams.FirstFieldIsTop = TRUE;
            break;
        case STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
            RasterPictureGetParams.ePictureStructure = STGVOBJ_kPictureStructureBottomField;
            VIDDEC_Data_p->RasterBufManagerParams.FirstFieldIsTop = FALSE;
            break;
        default :
            return (ST_ERROR_BAD_PARAMETER);
            break;
    }

    /************************/
    /* Getting the buffers  */
    /************************/

    /* Get Main buffers for main pictures acquisition */
    if (VIDDEC_Data_p->RasterBufManagerParams.MainRasterReconstructionIsEnabled)
    {
        RasterPictureGetParams.eDecimation = STGVOBJ_kDecimationFactorNone;
        ErrorCode = GetMainRasterPictures(VIDDEC_Data_p, &RasterPictureGetParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }
        VIDDEC_Data_p->RasterBufManagerParams.MainRasterPicturesLocked  = TRUE;
    }

    /* Get reduced buffers for decimated pictures acquisition */
    if (VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation != STGVOBJ_kDecimationFactorNone)
    {
        RasterPictureGetParams.eDecimation = VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation;
        ErrorCode = GetDecimatedRasterPictures(VIDDEC_Data_p, &RasterPictureGetParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* in case getting decimated buffers has not been successfl, the eventual locked */
            /* main buffers have to be unused */
            viddec_UnuseRasterPictures(VIDDEC_Data_p);
            return (ErrorCode);
        }
        VIDDEC_Data_p->RasterBufManagerParams.DecimatedRasterPicturesLocked  = TRUE;
    }

    /* mutex_release(VIDDEC_Data_p->RasterBufManagerParams.RasterBuffersMutex_p); */

    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : GetMainRasterPictures
Description : Sub function getting main buffers
Parameters  : IN: video decode handle
              OUT : pointers  on the pictures to be given by dv_rbm and their identifiers, that are stored
              in the video decode context data structure.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetMainRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p, DVRBM_PictureGetUseFuncParams_t * pGetUseFuncParams)
{
    STVAPI_Status_t ErrorCode = ST_NO_ERROR;

    if (!(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolIdIsAvailable))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = DVRBM_PictureGetUse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
        pGetUseFuncParams,
        &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainTopFieldPictureId),
        &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainBottomFieldPictureId),
        &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainRepeatFieldPictureId));

    /* fill the top field picture structure provided by DVRBM */
    if (ErrorCode == ST_NO_ERROR)
    {
        if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainTopFieldPictureId != STGVOBJ_kForbiddenPictureId)
        {
            ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainTopFieldPictureId,
                &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p));
            if (ErrorCode == ST_NO_ERROR)
            {
                 ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                     VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p,
                     STGVOBJ_kPictureStructureTopField,
                     STGVOBJ_kDecimationFactorNone);
            }
        }
    }

    /* fill the bottom field picture structure provided by DVRBM */
    if (ErrorCode == ST_NO_ERROR)
    {
        if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainBottomFieldPictureId != STGVOBJ_kForbiddenPictureId)
        {
            ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainBottomFieldPictureId,
                &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p));
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                    VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p,
                    STGVOBJ_kPictureStructureBottomField,
                    STGVOBJ_kDecimationFactorNone);
            }
        }
    }

    /* fill the repeat field picture structure provided by DVRBM */
    if (ErrorCode == ST_NO_ERROR)
    {
        if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainRepeatFieldPictureId != STGVOBJ_kForbiddenPictureId)
        {
            ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainRepeatFieldPictureId,
                &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainRepeatFieldPicture_p));
            if (ErrorCode == ST_NO_ERROR)
            {
                if (pGetUseFuncParams->bRepeatFieldIsTop)
                {
                    ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                        VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainRepeatFieldPicture_p,
                        STGVOBJ_kPictureStructureTopField,
                        STGVOBJ_kDecimationFactorNone);

                }
                else
                {
                    ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                        VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainRepeatFieldPicture_p,
                        STGVOBJ_kPictureStructureBottomField,
                        STGVOBJ_kDecimationFactorNone);
                }
            }
        }
    }

    return (ErrorCode);
}

/*******************************************************************************
Name        : GetDecimatedRasterPictures
Description : Sub function getting main buffers
Parameters  : IN: video decode handle
              OUT : pointers  on the pictures to be given by dv_rbm and their identifiers, that are stored
              in the video decode context data structure.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t GetDecimatedRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p, DVRBM_PictureGetUseFuncParams_t * pGetUseFuncParams)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolIdIsAvailable))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = DVRBM_PictureGetUse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
        pGetUseFuncParams,
        &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedTopFieldPictureId),
        &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedBottomFieldPictureId),
        &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedRepeatFieldPictureId));

    /* fill the top field picture structure provided by DVRBM */
    if (ErrorCode == ST_NO_ERROR)
    {
        if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedTopFieldPictureId != STGVOBJ_kForbiddenPictureId)
        {
            ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedTopFieldPictureId,
                &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedTopFieldPicture_p));
            if (ErrorCode == ST_NO_ERROR)
            {
                 ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                     VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedTopFieldPicture_p,
                     STGVOBJ_kPictureStructureTopField,
                     VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation);
            }
        }
    }

    /* fill the bottom field picture structure provided by DVRBM */
    if (ErrorCode == ST_NO_ERROR)
    {
        if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedBottomFieldPictureId != STGVOBJ_kForbiddenPictureId)
        {
            ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedBottomFieldPictureId,
                &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedBottomFieldPicture_p));
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                    VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedBottomFieldPicture_p,
                    STGVOBJ_kPictureStructureBottomField,
                    VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation);
            }
        }
    }

    /* fill the repeat field picture structure provided by DVRBM */
    if (ErrorCode == ST_NO_ERROR)
    {
        if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedRepeatFieldPictureId != STGVOBJ_kForbiddenPictureId)
        {
            ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedRepeatFieldPictureId,
                &(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedRepeatFieldPicture_p));
            if (ErrorCode == ST_NO_ERROR)
            {
                if (pGetUseFuncParams->bRepeatFieldIsTop)
                {
                    ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                        VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedRepeatFieldPicture_p,
                        STGVOBJ_kPictureStructureTopField,
                        VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation);

                }
                else
                {
                    ErrorCode = FillRasterPictureStructure(VIDDEC_Data_p,
                        VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedRepeatFieldPicture_p,
                        STGVOBJ_kPictureStructureBottomField,
                        VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation);
                }
            }
        }
    }

    return (ErrorCode);
}


/*******************************************************************************
Name        : viddec_MacroBlockToRasterInterruptHandler
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
int viddec_MacroBlockToRasterInterruptHandler(void * Param)
{
    const VIDDEC_Handle_t VIDDEC_Handle = (VIDDEC_Handle_t) Param;
    U32 MB2RInterruptStatus;
#ifdef END_OF_DECODE_ON_MB2R_IT
    U32 InterruptStatus;
#endif

#ifdef INTERRUPT_INSTALL_WORKAROUND
    int   pollStatus;
    /* Check that the device reall has a pending interrupt */
    pollStatus = 0;
    interrupt_poll(interrupt_handle(((VIDDEC_Data_t *)VIDDEC_Handle)->MB2RInterrupt.Number), &pollStatus);
    if (pollStatus == 0)
    {
      return (OS21_FAILURE);
    }
#endif /* INTERRUPT_INSTALL_WORKAROUND */

    MB2RInterruptStatus = HALDEC_GetMacroBlockToRasterStatus(((VIDDEC_Data_t *)VIDDEC_Handle)->HALDecodeHandle);

#ifdef END_OF_DECODE_ON_MB2R_IT
    /* end of raster reconstruction, whatever it is : main or secondary, means end of decode. */
    if ((MB2RInterruptStatus & (HALDEC_END_OF_MAIN_RASTER_RECONSTRUCTION | HALDEC_END_OF_SECONDARY_RASTER_RECONSTRUCTION )) != 0)
    {
       InterruptStatus = HALDEC_PIPELINE_IDLE_MASK | HALDEC_DECODER_IDLE_MASK;
       PushInterruptCommand(VIDDEC_Handle, InterruptStatus);
    }
#endif


    return (OS21_SUCCESS);
} /* End of MacroBlockToRasterInterruptHandler() function */

/*******************************************************************************
Name        : viddec_UnuseRasterPictures
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_UnuseRasterPictures(VIDDEC_Data_t * VIDDEC_Data_p)
{

  /* mutex_lock(VIDDEC_Data_p->RasterBufManagerParams.RasterBuffersMutex_p); */

  /* 1. main pictures */
  if (VIDDEC_Data_p->RasterBufManagerParams.MainRasterPicturesLocked)
  {
      if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainTopFieldPictureId != STGVOBJ_kForbiddenPictureId)
      {
          DVRBM_PictureUnuse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainTopFieldPictureId);
      }
      if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainBottomFieldPictureId != STGVOBJ_kForbiddenPictureId)
      {
          DVRBM_PictureUnuse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainBottomFieldPictureId);
      }
      if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainRepeatFieldPictureId != STGVOBJ_kForbiddenPictureId)
      {
          DVRBM_PictureUnuse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainRepeatFieldPictureId);
      }

      /* once, the pictures are unused, the ids are unuseful : reset them is preferable. */
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainTopFieldPictureId = STGVOBJ_kForbiddenPictureId;
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainBottomFieldPictureId = STGVOBJ_kForbiddenPictureId;
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainRepeatFieldPictureId = STGVOBJ_kForbiddenPictureId;
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p = NULL;
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p = NULL;
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainRepeatFieldPicture_p = NULL;

      /* unuse has  been done */
      VIDDEC_Data_p->RasterBufManagerParams.MainRasterPicturesLocked = FALSE;
  }

  /* 2. Decimated pictures */
  if (VIDDEC_Data_p->RasterBufManagerParams.DecimatedRasterPicturesLocked)
  {
      if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedTopFieldPictureId != STGVOBJ_kForbiddenPictureId)
      {
          DVRBM_PictureUnuse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedTopFieldPictureId);
      }
      if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedBottomFieldPictureId != STGVOBJ_kForbiddenPictureId)
      {
          DVRBM_PictureUnuse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedBottomFieldPictureId);
      }
      if (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedRepeatFieldPictureId != STGVOBJ_kForbiddenPictureId)
      {
          DVRBM_PictureUnuse(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedRepeatFieldPictureId);
      }

      /* once, the pictures are unused, the ids are unuseful : reset them is preferable. */
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedTopFieldPictureId = STGVOBJ_kForbiddenPictureId;
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedBottomFieldPictureId = STGVOBJ_kForbiddenPictureId;
      VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.DecimatedRepeatFieldPictureId = STGVOBJ_kForbiddenPictureId;

      /* unuse has  been done */
      VIDDEC_Data_p->RasterBufManagerParams.DecimatedRasterPicturesLocked = FALSE;
  }

  /* mutex_release(VIDDEC_Data_p->RasterBufManagerParams.RasterBuffersMutex_p); */
}


/*******************************************************************************
Name        : viddec_ComputeFielsPicturesPTS
Description : this function copies or interpolates the PTS provided by STVID for the field
              pictures created because of the DTV software rules.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t viddec_ComputeFielsPicturesPTS(VIDDEC_Data_t * VIDDEC_Data_p, STVID_NewDecodedPictureEvtParams_t * pExportedPictureInfos, STVID_PTS_t * pPTS)
{
    U32 FrameRateForCalculation = 0;
    U32 OneFieldDuration = 0;
    STVID_PTS_t TempPTS;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    STGVOBJ_Picture_t * pMainFirstFieldPicture;
    STGVOBJ_Picture_t * pMainSecondFieldPicture;
    STGVOBJ_Picture_t * pMainRepeatFieldPicture;
    STGVOBJ_Picture_t * pDecimatedFirstFieldPicture;
    STGVOBJ_Picture_t * pDecimatedSecondFieldPicture;
    STGVOBJ_Picture_t * pDecimatedRepeatFieldPicture;

    /* each MPEG picture can be equivalent to 1, 2, or 3 field pictures in DTV100 sofwatre world */
    FrameRateForCalculation = VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate;

    if (FrameRateForCalculation != 0)
    {
        OneFieldDuration = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation / 2;
    }

    /* First main Picture */
    if (pExportedPictureInfos->MainFirstFieldPictureId != STGVOBJ_kForbiddenPictureId)
    {
        ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, pExportedPictureInfos->MainFirstFieldPictureId, &pMainFirstFieldPicture);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }
        pMainFirstFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.ulPTS =
            pPTS->PTS;
        pMainFirstFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bPTS33 =
            pPTS->PTS33;
        pMainFirstFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bInterpolated =
            pPTS->Interpolated;
    }

    /* Second main Picture */
    if (pExportedPictureInfos->MainSecondFieldPictureId != STGVOBJ_kForbiddenPictureId)
    {
        ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, pExportedPictureInfos->MainSecondFieldPictureId, &pMainSecondFieldPicture);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }

        TempPTS = *pPTS;
        vidcom_AddU32ToPTS (TempPTS, OneFieldDuration);

        pMainSecondFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.ulPTS =
            TempPTS.PTS;
        pMainSecondFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bPTS33 =
            TempPTS.PTS33;
        pMainSecondFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bInterpolated = TRUE;
    }

    /* Repeat main picture */
    if (pExportedPictureInfos->MainRepeatFieldPictureId != STGVOBJ_kForbiddenPictureId)
    {
        ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, pExportedPictureInfos->MainRepeatFieldPictureId, &pMainRepeatFieldPicture);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }

        TempPTS = *pPTS;
        vidcom_AddU32ToPTS (TempPTS, 2 * OneFieldDuration);

        pMainRepeatFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.ulPTS =
            TempPTS.PTS;
        pMainRepeatFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bPTS33 =
            TempPTS.PTS33;
        pMainRepeatFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bInterpolated = TRUE;
    }

    /* First decimated  Picture */
    if (pExportedPictureInfos->DecimatedFirstFieldPictureId != STGVOBJ_kForbiddenPictureId)
    {
        ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, pExportedPictureInfos->DecimatedFirstFieldPictureId, &pDecimatedFirstFieldPicture);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }
        pDecimatedFirstFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.ulPTS =
            pPTS->PTS;
        pDecimatedFirstFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bPTS33 =
            pPTS->PTS33;
        pDecimatedFirstFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bInterpolated =
            pPTS->Interpolated;
    }

    /* Second decimated Picture */
    if (pExportedPictureInfos->DecimatedSecondFieldPictureId != STGVOBJ_kForbiddenPictureId)
    {
        ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, pExportedPictureInfos->DecimatedSecondFieldPictureId, &pDecimatedSecondFieldPicture);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }

        TempPTS = *pPTS;
        vidcom_AddU32ToPTS (TempPTS, OneFieldDuration);

        pDecimatedSecondFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.ulPTS =
            TempPTS.PTS;
        pDecimatedSecondFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bPTS33 =
            TempPTS.PTS33;
        pDecimatedSecondFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bInterpolated = TRUE;
    }

    /* Repeat picture */
    if (pExportedPictureInfos->DecimatedRepeatFieldPictureId != STGVOBJ_kForbiddenPictureId)
    {
        ErrorCode = DVRBM_PictureFromIdGet(VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId, pExportedPictureInfos->DecimatedRepeatFieldPictureId, &pDecimatedRepeatFieldPicture);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }

        TempPTS = *pPTS;
        vidcom_AddU32ToPTS (TempPTS, 2 * OneFieldDuration);

        pDecimatedRepeatFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.ulPTS =
            TempPTS.PTS;
        pDecimatedRepeatFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bPTS33 =
            TempPTS.PTS33;
        pDecimatedRepeatFieldPicture->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.stExtractedPTS.bInterpolated = TRUE;
    }


    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : viddec_SetRasterReconstructionConfiguration
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t viddec_SetRasterReconstructionConfiguration(VIDDEC_Data_t * VIDDEC_Data_p, VIDDEC_DecodePictureParams_t * const DecodeParams_p, const STVID_ScanType_t ScanType)
{
    /* primary raster reconstruction */
    if ((VIDDEC_Data_p->RasterBufManagerParams.MainRasterReconstructionIsEnabled) &&
        (VIDDEC_Data_p->RasterBufManagerParams.MainRasterPicturesLocked))
    {
        /* if picture is a frame or a top, then only the top field picture parameters are enough to be written in the registers. */
        if (DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
        {
            /* the register wants half the value of the pitch */
            HALDEC_SetPrimaryRasterReconstructionLumaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p->Data1.pBaseAddress,
                (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p->Data1.ulPitch / 2));
            HALDEC_SetPrimaryRasterReconstructionChromaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p->Data2.pBaseAddress);
        }
        else
        {
            /* retrieve pitch / 2 to the start address of the bottom frame buffers */
            HALDEC_SetPrimaryRasterReconstructionLumaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                (void *)((U32)(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.pBaseAddress)
                - (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.ulPitch / 2)),
                (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.ulPitch / 2));
            HALDEC_SetPrimaryRasterReconstructionChromaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                (void *)((U32)VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data2.pBaseAddress
                - (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.ulPitch / 2)));
        }
        HALDEC_EnablePrimaryRasterReconstruction(VIDDEC_Data_p->HALDecodeHandle);
    }
    else
    {
        HALDEC_DisablePrimaryRasterReconstruction(VIDDEC_Data_p->HALDecodeHandle);
    }

    /* secondary reconstruction */
    if ((VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation != STGVOBJ_kDecimationFactorNone)
      && (VIDDEC_Data_p->RasterBufManagerParams.DecimatedRasterPicturesLocked))
    {
        HALDEC_EnableSecondaryRasterReconstruction(VIDDEC_Data_p->HALDecodeHandle,
            VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation & ~(STGVOBJ_kDecimationFactorV2 | STGVOBJ_kDecimationFactorV4),
            VIDDEC_Data_p->RasterBufManagerParams.CurrentRasterDecimation & ~(STGVOBJ_kDecimationFactorH2 | STGVOBJ_kDecimationFactorH4),
        ScanType);

         /* if picture is a frame or a top, then only the top field picture parameters are enough to be written in the registers. */
        if (DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
        {
            /* the register wants half the value of the pitch */
            HALDEC_SetSecondaryRasterReconstructionLumaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedTopFieldPicture_p->Data1.pBaseAddress,
                (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedTopFieldPicture_p->Data1.ulPitch / 2));
            HALDEC_SetSecondaryRasterReconstructionChromaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedTopFieldPicture_p->Data2.pBaseAddress);
        }
        else
        {
            /* retrieve pitch / 2 to the start address of the bottom frame buffers */
            HALDEC_SetSecondaryRasterReconstructionLumaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                (void *)((U32)(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedBottomFieldPicture_p->Data1.pBaseAddress)
                - (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedBottomFieldPicture_p->Data1.ulPitch / 2)),
                (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedBottomFieldPicture_p->Data1.ulPitch / 2));
            HALDEC_SetSecondaryRasterReconstructionChromaFrameBuffer(VIDDEC_Data_p->HALDecodeHandle,
                (void *)((U32)VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedBottomFieldPicture_p->Data2.pBaseAddress
                - (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.DecimatedBottomFieldPicture_p->Data1.ulPitch / 2)));
        }
    }
    else
    {
        HALDEC_DisableSecondaryRasterReconstruction(VIDDEC_Data_p->HALDecodeHandle);
    }

    return (ST_NO_ERROR);
}


/*******************************************************************************
Name        : FillRasterPictureStructure
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t FillRasterPictureStructure(VIDDEC_Data_t * VIDDEC_Data_p,
    STGVOBJ_Picture_t * FieldPicture_p,
    STGVOBJ_PictureStructure_t ePictureStructure,
    const STGVOBJ_DecimationFactor_t eDecimation)
{

    if (FieldPicture_p == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    FieldPicture_p->stPictureInfos.ePictureStructure = ePictureStructure;
    FieldPicture_p->stPictureInfos.eVideoInputType = STGVOBJ_kCompressedInputType;

    /* fill the other fields of the picture. */
    switch(VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame)
    {
        case STVID_MPEG_FRAME_I :
            FieldPicture_p->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.ePictureType = STGVOBJ_kMpegPictureType_I;
        break;
        case STVID_MPEG_FRAME_P :
            FieldPicture_p->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.ePictureType = STGVOBJ_kMpegPictureType_P;
        break;
        case STVID_MPEG_FRAME_B :
            FieldPicture_p->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.ePictureType = STGVOBJ_kMpegPictureType_B;
        break;
    }

    /* Aspect ratio */
    FieldPicture_p->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.eAspectRatio =
      (STGVOBJ_AspectRatio_t)VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.AspectRatio;

    FieldPicture_p->stPictureInfos.uSpecificPicturesInfos.stCompressedInputPictureInfos.ulExtendedTemporalReference =
      VIDDEC_Data_p->ForTask.PictureStreamInfo.ExtendedTemporalReference;

    FieldPicture_p->stPictureInfos.stVideoInputInfo.eVideoDataFormat = STGVOBJ_kDataFormatYCbCr888_420;

    /* According to STGXOBJ_Scan type, set the matching STGVOBJ input STGVOBJ_InputFieldSequence_t one */
    if (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.ScanType == STGXOBJ_INTERLACED_SCAN)
    {
      FieldPicture_p->stPictureInfos.stVideoInputInfo.eFieldSequence =  STGVOBJ_kInputSequenceInterlaced;
    }
    else if (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN)
    {
      FieldPicture_p->stPictureInfos.stVideoInputInfo.eFieldSequence =  STGVOBJ_kInputSequenceProgressive;
    }

    /* The definition of the frame rate in STGVOBJ is the same than the MPEG norm. the value can be copied as it is */
    FieldPicture_p->stPictureInfos.stVideoInputInfo.ulFrameRate = VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.FrameRate;

    FieldPicture_p->stPictureInfos.stVideoInputInfo.stActiveVideoRes.ulWidth = VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Width;
    FieldPicture_p->stPictureInfos.stVideoInputInfo.stActiveVideoRes.ulHeight = VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Height;

    FieldPicture_p->eDecimationFactor = eDecimation;
    FieldPicture_p->eDiscontinuity = STGVOBJ_kNoDiscontinuity;
    FieldPicture_p->ulPictureNumberReference = 0;
    FieldPicture_p->ulPictureGroupReference = 0;

    return (ST_NO_ERROR);
}



/* End of vid_mb2r.c */

