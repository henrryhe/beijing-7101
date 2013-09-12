/*******************************************************************************

File name   : hv_vdpfmd.c

Description :

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                 ----
02 Nov 2006       Created                                          OL
*******************************************************************************/

/* Includes ----------------------------------------------------------- */
#include "stddefs.h"
#include "stdevice.h"

#include "hv_vdplay.h"
#include "hv_vdpdei.h"
#include "hv_vdpfmd.h"

#include "string.h"

#ifdef STLAYER_USE_FMD

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/* #define TRACE_UART */

/* Select the Traces you want */
#define TrMain                 TraceOn
#define TrFmdStatus            TraceOn
#define TrFmdDetail            TraceOn
#define TrFmdResult            TraceOn
#define TrFmdBlock             TraceOff

#define TrWarning              TraceOn
/* TrError is now defined in layer_trace.h */

/* layer_trace.h" should be included AFTER the definition of the traces wanted */
#include "layer_trace.h"


/* Private Macros ----------------------------------------------------- */


/* Private Types ------------------------------------------------------ */


/* Private Constants -------------------------------------------------- */

#define LUMA_PIXEL_MAX_VALUE        255

/* Block size = 80 * 40 */
#define HORIZONTAL_BLOCK_SIZE       80
#define VERTICAL_BLOCK_SIZE            40

/* Default FMD parameters */
#define DEFAULT_CFDTHRESHOLD 10
#define DEFAULT_BLOCKMOVEDETECTIONTHRESHOLD 10
#define DEFAULT_PIXELMOVEDETECTIONTHRESHOLD 35
#define DEFAULT_REPEATFIELDDETECTIONTHRESHOLD 70
#define DEFAULT_SCENECHANGEDETECTIONTHRESHOLD 15

/* Private Function prototypes ---------------------------------------- */

static ST_ErrorCode_t   vdp_fmd_SetFmdBlockNumber (const STLAYER_Handle_t LayerHandle);
static BOOL vdp_fmd_IsFmdEnabled (const STLAYER_Handle_t LayerHandle);

#ifdef TRACE_UART
static ST_ErrorCode_t vdp_fmd_DisplayFmdStatus (const STLAYER_Handle_t LayerHandle);
#endif

/* Functions ---------------------------------------------------------- */

/******************************************************************************/
/*************************      API FUNCTIONS    **********************************/
/******************************************************************************/

/*******************************************************************************
Name        : vdp_fmd_Init
Description : Initialisation of FMD module
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t vdp_fmd_Init (STLAYER_Handle_t   LayerHandle)
{
   STLAYER_FMDParams_t DefaultFMDThresholds;
   ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
   HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    /* Default values for FMD */
    DefaultFMDThresholds.CFDThreshold = DEFAULT_CFDTHRESHOLD;
    DefaultFMDThresholds.BlockMoveDetectionThreshold = DEFAULT_BLOCKMOVEDETECTIONTHRESHOLD;
    DefaultFMDThresholds.PixelMoveDetectionThreshold = DEFAULT_PIXELMOVEDETECTIONTHRESHOLD;
    DefaultFMDThresholds.RepeatFieldDetectionThreshold = DEFAULT_REPEATFIELDDETECTIONTHRESHOLD;
    DefaultFMDThresholds.SceneChangeDetectionThreshold = DEFAULT_SCENECHANGEDETECTIONTHRESHOLD;

    /* Set FMD Thresholds and start it */
    vdp_fmd_SetFmdThreshold(LayerHandle, &DefaultFMDThresholds);

    vdp_fmd_EnableFmd (LayerHandle, FALSE);

    LayerCommonData_p->FmdReportingEnabled = FALSE;
    LayerCommonData_p->FmdFieldRepeatCount=0;
    LayerCommonData_p->FMDSourcePictureDimensions.Width = 0;
    LayerCommonData_p->FMDSourcePictureDimensions.Height= 0;

    return (ErrorCode);
} /* End of vdp_fmd_Init() function. */

/*******************************************************************************
Name        : vdp_fmd_Term
Description : Termination of FMD module: Release memory.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_fmd_Term (const STLAYER_Handle_t LayerHandle)
{
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    LayerCommonData_p->FmdReportingEnabled = FALSE;
    vdp_fmd_EnableFmd (LayerHandle, FALSE);

    TrFmdStatus(("\r\nvdp_fmd_Term"));

    return (ErrorCode);
}

/*******************************************************************************
Name        : vdp_fmd_SetFmd
Description : Function used to enable/disable FMD
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_fmd_EnableFmd (const STLAYER_Handle_t LayerHandle, BOOL FmdEnabled)
{
    U32     DeiCtlRegisterValue;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    TrFmdDetail(("\r\nvdp_fmd_SetFmd"));

    /* Get the current DEI CTL register value */
    DeiCtlRegisterValue = HAL_ReadDisp32(VDP_DEI_CTL);

    if (FmdEnabled)
    {
        DeiCtlRegisterValue = DeiCtlRegisterValue | (VDP_DEI_CTL_FMD_ON << VDP_DEI_CTL_FMD_ON_EMP) ;
    }
    else
    {
        DeiCtlRegisterValue = DeiCtlRegisterValue & ~ (VDP_DEI_CTL_FMD_ON << VDP_DEI_CTL_FMD_ON_EMP);
    }

    /* Write the value to the register */
    HAL_WriteDisp32(VDP_DEI_CTL, DeiCtlRegisterValue);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name        : vdp_fmd_SetFmdThreshold
Description : Function used to set FMD thresholds used to detect block moves
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_fmd_SetFmdThreshold (const STLAYER_Handle_t LayerHandle, const STLAYER_FMDParams_t * Params_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    STLAYER_FMDParams_t  tmp;

    TrFmdDetail(("\r\nvdp_dei_SetFmdThreshold"));

    HAL_WriteDisp32(VDP_FMD_THRESHOLD_SCD,  Params_p->SceneChangeDetectionThreshold & VDP_FMD_SCENE_MASK);
    HAL_WriteDisp32(VDP_FMD_THRESHOLD_RFD, Params_p->RepeatFieldDetectionThreshold & VDP_FMD_REPEAT_MASK);
    HAL_WriteDisp32(VDP_FMD_THRESHOLD_MOVE, (Params_p->BlockMoveDetectionThreshold & VDP_FMD_NUM_MOVE_PIX_MASK) << VDP_FMD_NUM_MOVE_PIX_EMP | (Params_p->PixelMoveDetectionThreshold & VDP_FMD_MOVE_MASK));
    HAL_WriteDisp32(VDP_FMD_THRESHOLD_CFD, Params_p->CFDThreshold & VDP_FMD_NOISE_MASK);

    vdp_fmd_GetFmdThreshold(LayerHandle, &tmp);
    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : vdp_fmd_GetFmdThreshold
Description : Function used to get FMD thresholds used to detect block moves
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_fmd_GetFmdThreshold (const STLAYER_Handle_t LayerHandle, STLAYER_FMDParams_t * Params_p)
{
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    TrFmdDetail(("\r\nvdp_dei_GetFmdThreshold"));

    Params_p->SceneChangeDetectionThreshold  = HAL_ReadDisp32(VDP_FMD_THRESHOLD_SCD) & VDP_FMD_SCENE_MASK;
    Params_p->RepeatFieldDetectionThreshold  = HAL_ReadDisp32(VDP_FMD_THRESHOLD_RFD) & VDP_FMD_REPEAT_MASK;
    Params_p->BlockMoveDetectionThreshold = (HAL_ReadDisp32(VDP_FMD_THRESHOLD_MOVE) >> VDP_FMD_NUM_MOVE_PIX_EMP) & VDP_FMD_NUM_MOVE_PIX_MASK;
    Params_p->PixelMoveDetectionThreshold = HAL_ReadDisp32(VDP_FMD_THRESHOLD_MOVE) & VDP_FMD_MOVE_MASK;
    Params_p->CFDThreshold = HAL_ReadDisp32(VDP_FMD_THRESHOLD_CFD) & VDP_FMD_NOISE_MASK;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name        : vdp_fmd_SetFmdBlockNumber
Description : Function used to set the number of Horizontal and Vertical Blocks used by FMD
                    Block size = 80 * 40
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vdp_fmd_SetFmdBlockNumber (const STLAYER_Handle_t LayerHandle)
{
    U32         SourcePictureHorizontalSize, SourcePictureVerticalSize;
    U32         h_nb, v_nb;
    U32         BlockNumberRegisterValue;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    HALLAYER_VDPLayerCommonData_t * LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    TrFmdBlock(("\r\nvdp_fmd_SetBlockNumber"));

    /* Get Source Picture Dimensions */
    SourcePictureHorizontalSize = LayerCommonData_p->SourcePictureDimensions.Width;
    SourcePictureVerticalSize = LayerCommonData_p->SourcePictureDimensions.Height;
    TrFmdBlock(("\r\nSource = %d * %d", SourcePictureHorizontalSize, SourcePictureVerticalSize));

    /* Block size = 80 * 40 */
    h_nb = SourcePictureHorizontalSize / HORIZONTAL_BLOCK_SIZE;
    v_nb = SourcePictureVerticalSize / HORIZONTAL_BLOCK_SIZE;
    TrFmdBlock(("\r\nh_nb=%d, v_nb=%d", h_nb, v_nb));

    BlockNumberRegisterValue = (v_nb & VDP_FMD_V_BLK_NB_MASK) << VDP_FMD_V_BLK_NB_EMP |
                                               (h_nb & VDP_FMD_H_BLK_NB_MASK);

    HAL_WriteDisp32(VDP_FMD_BLOCK_NUMBER,  BlockNumberRegisterValue);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name        : vdp_fmd_GetFmdResults
Description : Get FMD results about Consecutive Fields and Fields of same parity

                   !!! NB: We collect the result corresponding to the previous VSync !!!
Parameters  : LayerHandle IN: Handle of the layer, FMDResults_p OUT: Pointer
              to an allocated FMDResults data type
Assumptions :
Limitations :
Returns     : TRUE if a new FMD result is available, FALSE otherwise
*******************************************************************************/
BOOL vdp_fmd_GetFmdResults(const STLAYER_Handle_t LayerHandle, STLAYER_FMDResults_t *FMDResults_p)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
    U32         FmdStatusRegisterValue;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;
    BOOL NewFMDResultsAvailable = FALSE;

    /* Check if FMD was enabled */
    if (vdp_fmd_IsFmdEnabled(LayerHandle) == FALSE)
    {
        return FALSE;
    }

    /* We should check that the Current Field for next VSync is different from the Current Field used by the Hw.
         They can be identical if a Frame is repeated or if a Freeze has been done.
         In that case we should not overwrite the last Motion information*/
    if ( (LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.PictureIndex != LayerCommonData_p->FieldsCurrentlyUsedByHw.CurrentField.PictureIndex) ||
          (LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.FieldType != LayerCommonData_p->FieldsCurrentlyUsedByHw.CurrentField.FieldType) )
    {
        /* The above result correspond to the motion between the Fields set during the previous VSync.
             A new VSync has occured so the Motion between Current and Next now corresponds to the
             motion between Previous and Current. */

        FMDResults_p->PictureIndex = LayerCommonData_p->FieldsCurrentlyUsedByHw.PreviousField.PictureIndex;
        FMDResults_p->FieldRepeatCount = LayerCommonData_p->FmdFieldRepeatCount;

        FMDResults_p->FieldSum = HAL_ReadDisp32(VDP_FMD_FIELD_SUM);
        FMDResults_p->CFDSum = HAL_ReadDisp32(VDP_FMD_CFD_SUM);
        FmdStatusRegisterValue = HAL_ReadDisp32(VDP_FMD_STATUS);
        FMDResults_p->SceneCount =FmdStatusRegisterValue & VDP_FMD_SCENECOUNT_MASK;
        FMDResults_p->MoveStatus = (FmdStatusRegisterValue & VDP_FMD_MOVE_STATUS) != 0;
        FMDResults_p->RepeatStatus = (FmdStatusRegisterValue & VDP_FMD_REPEAT_STATUS) != 0;

        LayerCommonData_p->FmdFieldRepeatCount=0;
        NewFMDResultsAvailable = TRUE;
    }
    else
    {
        /* The new Field is identical to the last "Current Field" so the Motion information is unchanged */
        LayerCommonData_p->FmdFieldRepeatCount++;
        NewFMDResultsAvailable = FALSE;
    }

#ifdef TRACE_UART
         vdp_fmd_DisplayFmdStatus (LayerHandle);
#endif

    return NewFMDResultsAvailable;
}

/*******************************************************************************
Name        : vdp_fmd_CheckFmdSetup
Description : This function check if FMD should be enabled:
                    FMD absolutely needs to operate on 3 Fields (and 3 Fields with the right Parity).
                    Otherwise we should disable it.
              It also set the correct block numbers according to source dimensions
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void vdp_fmd_CheckFmdSetup(const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);

    if ( (LayerCommonData_p->FieldsToUseAtNextVSync.PreviousField.FieldAvailable == FALSE)  ||
          (LayerCommonData_p->FieldsToUseAtNextVSync.NextField.FieldAvailable == FALSE)  )
    {
        /* Previous or Next Field is not available: FMD not possible */
        TrWarning(("\r\nFMD OFF"));
        vdp_fmd_EnableFmd (LayerHandle, FALSE);
    }
    else
    {
        /* Previous and Next Fields are available */

        /* Disable FMD when DEI is not activated */
        if ( (LayerCommonData_p->AppliedDeiMode == STLAYER_DEI_MODE_OFF) ||
              (LayerCommonData_p->AppliedDeiMode == STLAYER_DEI_MODE_BYPASS)  )
        {
            TrWarning(("\r\nDEI is not ON => FMD OFF"));
            vdp_fmd_EnableFmd (LayerHandle, FALSE);
        }
        else
        {
            /* If source picture size has changed we need to re-compute block number for FMD */
            if((LayerCommonData_p->SourcePictureDimensions.Width != LayerCommonData_p->FMDSourcePictureDimensions.Width) ||
                (LayerCommonData_p->SourcePictureDimensions.Height != LayerCommonData_p->FMDSourcePictureDimensions.Height) )
            {
                /* Set the number of Blocks on which FMD will operate (depends on resolution of source) */
                vdp_fmd_SetFmdBlockNumber(LayerHandle);
                LayerCommonData_p->FMDSourcePictureDimensions = LayerCommonData_p->SourcePictureDimensions;
            }

            /* Normal case: FMD ON */
            vdp_fmd_EnableFmd (LayerHandle, TRUE);
        }
    }
}

/******************************************************************************/
/*************************   LOCAL FUNCTIONS   **********************************/
/******************************************************************************/


/*******************************************************************************
Name        : vdp_fmd_IsFmdEnabled
Description : Function indicating if FMD is enabled or not
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL vdp_fmd_IsFmdEnabled (const STLAYER_Handle_t LayerHandle)
{
    U32     DeiCtlRegisterValue;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Get the current DEI CTL register value */
    DeiCtlRegisterValue = HAL_ReadDisp32(VDP_DEI_CTL);

    if (DeiCtlRegisterValue & (VDP_DEI_CTL_FMD_ON << VDP_DEI_CTL_FMD_ON_EMP) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*******************************************************************************
Name        : vdp_fmd_DumpFmdStatus
Description : Display FMD status about Consecutive Fields and Fields of same parity

                   !!! NB: We collect the result corresponding to the previous VSync !!!
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t vdp_fmd_DisplayFmdStatus (const STLAYER_Handle_t LayerHandle)
{
    HALLAYER_VDPLayerCommonData_t *    LayerCommonData_p = COMMON_DATA_PTR(LayerHandle);
    U32         CFDSumRegisterValue;
    U32         FieldSumRegisterValue;
    U32         FmdStatusRegisterValue;
    HALLAYER_LayerProperties_t    * LayerProperties_p = (HALLAYER_LayerProperties_t *)LayerHandle;

    /* Check if FMD was enabled */
    if (vdp_fmd_IsFmdEnabled(LayerHandle) == FALSE)
    {
        TrFmdDetail(("\r\nFMD was disabled => No FMD result"));
        goto done;
    }

    CFDSumRegisterValue = HAL_ReadDisp32(VDP_FMD_CFD_SUM);
    FieldSumRegisterValue = HAL_ReadDisp32(VDP_FMD_FIELD_SUM);
    FmdStatusRegisterValue = HAL_ReadDisp32(VDP_FMD_STATUS);

   TrFmdResult(("\r\nCFD_SUM = %d", CFDSumRegisterValue));
   TrFmdResult(("\r\nFIELD_SUM = %d", FieldSumRegisterValue));
   TrFmdDetail(("\r\nFMD_STATUS = 0x%08x", FmdStatusRegisterValue));

    /* "VDP_FMD_MOVE_STATUS" gives the motion between Previous and Next Fields of the last VSync */
    TrFmdStatus(("\r\n%d%s<->%d%s: ",
            LayerCommonData_p->FieldsCurrentlyUsedByHw.PreviousField.PictureIndex,
            FIELD_TYPE(LayerCommonData_p->FieldsCurrentlyUsedByHw.PreviousField),
            LayerCommonData_p->FieldsCurrentlyUsedByHw.NextField.PictureIndex,
            FIELD_TYPE(LayerCommonData_p->FieldsCurrentlyUsedByHw.NextField) ));

    if (FmdStatusRegisterValue & VDP_FMD_MOVE_STATUS)
    {
        TrFmdStatus(("Motion"));
    }
    else
    {
        TrFmdStatus(("No Motion"));
    }

    /* "CFD_SUM" gives the motion between Current and Next Fields of the last VSync  */
    TrFmdStatus(("\r\n%d%s<->%d%s: ",
            LayerCommonData_p->FieldsCurrentlyUsedByHw.CurrentField.PictureIndex,
            FIELD_TYPE(LayerCommonData_p->FieldsCurrentlyUsedByHw.CurrentField),
            LayerCommonData_p->FieldsCurrentlyUsedByHw.NextField.PictureIndex,
            FIELD_TYPE(LayerCommonData_p->FieldsCurrentlyUsedByHw.NextField) ));


    /* We should check that the Current Field for next VSync is different from the Current Field used by the Hw.
         They can be identical if a Frame is repeated or if a Freeze has been done.
         In that case we should not overwrite the last Motion information*/
    if ( (LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.PictureIndex != LayerCommonData_p->FieldsCurrentlyUsedByHw.CurrentField.PictureIndex) ||
          (LayerCommonData_p->FieldsToUseAtNextVSync.CurrentField.FieldType != LayerCommonData_p->FieldsCurrentlyUsedByHw.CurrentField.FieldType) )
    {
        /* The above result correspond to the motion between the Fields set during the previous VSync.
             A new VSync has occured so the Motion between Current and Next now corresponds to the
             motion between Previous and Current. */
    }
    else
    {
        /* The new Field is identical to the last "Current Field" so the Motion information is unchanged */
    }


 done:
    return ST_NO_ERROR;
}


/******************************************************************************/

#endif

/* End of hv_vdpfmd.c */

