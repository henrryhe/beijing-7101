/*******************************************************************************

File name   : hv_gdp.c

Description : HAL graphics video display family source file

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                     Name
----               ------------                                     ----
10 September 2007  Created                                          MB

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"

#include "vid_disp.h"
#include "halv_dis.h"
#include "display.h"
#include "stevt.h"
#include "sttbx.h"



/* Private Types ------------------------------------------------------------ */
typedef struct LayerProperties_s {
    void*   BufferLumaAddress_p;
} LayerProperties_t;


/* Private Constants -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions in HALDISP_FunctionsTable_t */
static void DisablePresentationChromaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                                                 const STLAYER_Layer_t  LayerType);
static void DisablePresentationLumaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                                               const STLAYER_Layer_t  LayerType);
static void EnablePresentationChromaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                                                const STLAYER_Layer_t  LayerType);
static void EnablePresentationLumaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                                              const STLAYER_Layer_t  LayerType);
static ST_ErrorCode_t Init(const HALDISP_Handle_t DisplayHandle);
static void SelectPresentationChromaBottomField(const HALDISP_Handle_t DisplayHandle,
                                                const STLAYER_Layer_t  LayerType,
                                                const U32 LayerIdent,
                                                const VIDBUFF_PictureBuffer_t* const PictureBuffer_p);
static void SelectPresentationChromaTopField(const HALDISP_Handle_t DisplayHandle,
                                             const STLAYER_Layer_t  LayerType,
                                             const U32 LayerIdent,
                                             const VIDBUFF_PictureBuffer_t* const PictureBuffer_p);
static void SelectPresentationLumaBottomField(const HALDISP_Handle_t DisplayHandle,
                                              const STLAYER_Layer_t  LayerType,
                                              const U32 LayerIdent,
                                              const VIDBUFF_PictureBuffer_t* const PictureBuffer_p);
static void SelectPresentationLumaTopField(const HALDISP_Handle_t DisplayHandle,
                                           const STLAYER_Layer_t  LayerType,
                                           const U32 LayerIdent,
                                           const VIDBUFF_PictureBuffer_t* const PictureBuffer_p);
static void SetPresentationChromaFrameBuffer(const HALDISP_Handle_t DisplayHandle, const STLAYER_Layer_t LayerType, void * const BufferAddress_p);
static void SetPresentationFrameDimensions(const HALDISP_Handle_t DisplayHandle,
                                           const STLAYER_Layer_t   LayerType,
                                           const U32 HorizontalSize,
                                           const U32 VerticalSize);
static void SetPresentationLumaFrameBuffer(const HALDISP_Handle_t DisplayHandle, const STLAYER_Layer_t LayerType, void * const BufferAddress_p);
static void SetPresentationStoredPictureProperties(const HALDISP_Handle_t              DisplayHandle,
                                                   const STLAYER_Layer_t               LayerType,
                                                   const VIDDISP_ShowParams_t * const  ShowParams_p);
static void PresentFieldsForNextVSync(const HALDISP_Handle_t HALDisplayHandle,
                                      const STLAYER_Layer_t  LayerType,
                                      const U32           LayerIdent,
                                      const HALDISP_Field_t * FieldsForNextVSync);
static void Term(const HALDISP_Handle_t DisplayHandle);


/* Global Variables --------------------------------------------------------- */
const HALDISP_FunctionsTable_t  HALDISP_GraphicsDisplayFunctions =
{
    DisablePresentationChromaFieldToggle,
    DisablePresentationLumaFieldToggle,
    EnablePresentationChromaFieldToggle,
    EnablePresentationLumaFieldToggle,
    Init,
    SelectPresentationChromaBottomField,
    SelectPresentationChromaTopField,
    SelectPresentationLumaBottomField,
    SelectPresentationLumaTopField,
    SetPresentationChromaFrameBuffer,
    SetPresentationFrameDimensions,
    SetPresentationLumaFrameBuffer,
    SetPresentationStoredPictureProperties,
    PresentFieldsForNextVSync,
    Term
};

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : DisablePresentationChromaFieldToggle
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DisablePresentationChromaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                                                 const STLAYER_Layer_t  LayerType)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
} /* End of DisablePresentationChromaFieldToggle() function */


/*******************************************************************************
Name        : DisablePresentationLumaFieldToggle
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DisablePresentationLumaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                                               const STLAYER_Layer_t  LayerType)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
} /* End of DisablePresentationLumaFieldToggle() function */


/*******************************************************************************
Name        : EnablePresentationChromaFieldToggle
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnablePresentationChromaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                                                const STLAYER_Layer_t  LayerType)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
} /* End of EnablePresentationChromaFieldToggle() function */


/*******************************************************************************
Name        : EnablePresentationLumaFieldToggle
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnablePresentationLumaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                                              const STLAYER_Layer_t  LayerType)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
} /* End of EnablePresentationLumaFieldToggle() function */


/*******************************************************************************
Name        : Init
Description : HAL Display manager handle
Parameters  :
Assumptions :  HalDisplayHandle  valid and not NULL
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDISP_Handle_t HalDisplayHandle)
{

    ST_ErrorCode_t          Err = ST_NO_ERROR;

    /* Allocate LayerProperties_t structure */
    SAFE_MEMORY_ALLOCATE(((HALDISP_Properties_t *) HalDisplayHandle)->PrivateData_p,
                         LayerProperties_t *,
                         ((HALDISP_Properties_t *) HalDisplayHandle)->CPUPartition_p,
                         sizeof(LayerProperties_t));
    if (((HALDISP_Properties_t *) HalDisplayHandle)->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Register  VIDDISP_UPDATE_GFX_LAYER_SRC_EVT in order to notify changes of field to the layer */
    Err = STEVT_RegisterDeviceEvent(((HALDISP_Properties_t *) HalDisplayHandle)->EventsHandle,
                                    ((HALDISP_Properties_t *) HalDisplayHandle)->VideoName,
                                    VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                    &(((HALDISP_Properties_t *) HalDisplayHandle)->UpdateGfxLayerSrcEvtID));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_RegisterDeviceEvent failed. Error = 0x%x !", Err));
        Term(HalDisplayHandle);
    }

    return(Err);
}  /* End of Init() */

/*******************************************************************************
Name        : SelectPresentationChromaBottomField
Description :
Parameters  : HAL display handle
Assumptions : Must be called after a SelectPresentationLumaBottomField
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SelectPresentationChromaBottomField(const HALDISP_Handle_t HALDisplayHandle,
                                                const STLAYER_Layer_t  LayerType,
                                                const U32 LayerIdent,
                                                const VIDBUFF_PictureBuffer_t * PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    /* Do nothing */
} /* End of SelectPresentationChromaBottomField() function */


/*******************************************************************************
Name        : SelectPresentationChromaTopField
Description :
Parameters  : HAL display handle
Assumptions : Must be called after a SelectPresentationLumaTopField
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SelectPresentationChromaTopField(const HALDISP_Handle_t HALDisplayHandle,
                                             const STLAYER_Layer_t  LayerType,
                                             const U32 LayerIdent,
                                             const VIDBUFF_PictureBuffer_t * PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    /* Do nothing */
} /* End of SelectPresentationChromaTopField() function */

/*******************************************************************************
Name        : SelectPresentationLumaBottomField
Description : Presents the bottom field
Parameters  : HAL display manager handle
Assumptions : Errors returned potentially by STLAYER are ignored
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationLumaBottomField(const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t* PictureBuffer_p)
{
    HALDISP_Properties_t*        DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    VIDDISP_LayerUpdateParams_t  UpdateParams;

    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Update DataPointer structure */
    UpdateParams.UpdateReason = VIDDISP_LAYER_UPDATE_BOTTOM_FIELD_ADDRESS;
    UpdateParams.Data2_p      = (void*)((LayerProperties_t *)(DisplayProperties_p->PrivateData_p))->BufferLumaAddress_p;
    UpdateParams.LayerType    = LayerType;

    /* Notify VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_Notify(DisplayProperties_p->EventsHandle, DisplayProperties_p->UpdateGfxLayerSrcEvtID,
                 STEVT_EVENT_DATA_TYPE_CAST &UpdateParams);
} /* End of SelectPresentationLumaBottomField() function */

/*******************************************************************************
Name        : SelectPresentationLumaTopField
Description : Presents the top field
Parameters  : HAL display manager handle
Assumptions : Errors returned potentially by STLAYER are ignored
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationLumaTopField(const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t* PictureBuffer_p)
{
    HALDISP_Properties_t*        DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    VIDDISP_LayerUpdateParams_t  UpdateParams;

    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Update DataPointer structure */
    UpdateParams.UpdateReason = VIDDISP_LAYER_UPDATE_TOP_FIELD_ADDRESS;
    UpdateParams.Data1_p      = (void*)((LayerProperties_t *)(DisplayProperties_p->PrivateData_p))->BufferLumaAddress_p;
    UpdateParams.LayerType    = LayerType;

    /* Notify VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_Notify(DisplayProperties_p->EventsHandle, DisplayProperties_p->UpdateGfxLayerSrcEvtID,
                 STEVT_EVENT_DATA_TYPE_CAST &UpdateParams);
} /* End of SelectPresentationLumaTopField() function */

/*******************************************************************************
Name        : SetPresentationChromaFrameBuffer
Description : Set display Top frame buffer
Parameters  : Digital Input registers address, buffer address
Assumptions : Display Handle valid and not NULL
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationChromaFrameBuffer(
                         const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    UNUSED_PARAMETER(HalDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(BufferAddress_p);
    /* Do Nothing */
} /* End of SetPresentationChromaFrameBuffer*/

/*******************************************************************************
Name        : SetPresentationFrameDimensions
Description : Do Nothing
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationFrameDimensions(
                                const HALDISP_Handle_t  HalDisplayHandle,
                                const STLAYER_Layer_t   LayerType,
                                const U32               HorizontalSize,
                                const U32               VerticalSize)
{
    UNUSED_PARAMETER(HalDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(HorizontalSize);
    UNUSED_PARAMETER(VerticalSize);
    /* Do Nothing */
}  /* End of SetPresentationFrameDimensions */

/*******************************************************************************
Name        : SetPresentationLumaFrameBuffer
Description : Set display Top frame buffer
Parameters  : Digital Input registers address, buffer address
Assumptions : Display Handle valid and not NULL
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationLumaFrameBuffer(
                         const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    HALDISP_Properties_t* DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    UNUSED_PARAMETER(LayerType);

    ((LayerProperties_t *)(DisplayProperties_p->PrivateData_p))->BufferLumaAddress_p = (void*) BufferAddress_p;

} /* End of SetPresentationLumaFrameBuffer */

/*******************************************************************************
Name        : SetPresentationStoredPictureProperties
Description : Shows how the presented picture has been stored.
Parameters  : HAL display manager handle
              Parameters to display (scantype, decimation, ...).
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetPresentationStoredPictureProperties(
                         const HALDISP_Handle_t              DisplayHandle,
                         const STLAYER_Layer_t               LayerType,
                         const VIDDISP_ShowParams_t * const  ShowParams_p)
{
    UNUSED_PARAMETER(DisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(ShowParams_p);
    /* Do Nothing */
} /* End of SetPresentationStoredPictureProperties */

/*******************************************************************************
Name        : PresentFieldsForNextVSync
Description : Do nothing!
Parameters  : FieldsForNextVSync : Array of 3 Fields containing the Fields Previous, Current, Next.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PresentFieldsForNextVSync(
                           const HALDISP_Handle_t HALDisplayHandle,
                           const STLAYER_Layer_t  LayerType,
                           const U32           LayerIdent,
                           const HALDISP_Field_t * FieldsForNextVSync)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(FieldsForNextVSync);
    /* Do Nothing */
} /* End of PresentFieldsForNextVSync */

/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  :
Assumptions : HalDisplayHandle  valid and not NULL
Limitations :
Returns     :
*******************************************************************************/
static void Term(const HALDISP_Handle_t HalDisplayHandle)
{

    /* Unregister VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_UnregisterDeviceEvent(
            ((HALDISP_Properties_t *) HalDisplayHandle)->EventsHandle, ((HALDISP_Properties_t *) HalDisplayHandle)->VideoName, VIDDISP_UPDATE_GFX_LAYER_SRC_EVT);

    /* Deallocate LayerProperties_t structure */
    SAFE_MEMORY_DEALLOCATE(((HALDISP_Properties_t *) HalDisplayHandle)->PrivateData_p,
                           ((HALDISP_Properties_t *) HalDisplayHandle)->CPUPartition_p,
                           sizeof(LayerProperties_t));

}  /* End of Term() */

/* End of hv_gdp.c */
