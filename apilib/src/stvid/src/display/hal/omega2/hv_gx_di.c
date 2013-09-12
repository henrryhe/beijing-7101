/*******************************************************************************

File name   : hv_gx_di.c

Description : HAL video display omega 2 family source file for gfx layer use

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
27 July 2001        Created                                           TM

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include "stddefs.h"
#include "halv_dis.h"
#include "display.h"
#include "stevt.h"
#include "sttbx.h"



/* Private Types ------------------------------------------------------------ */
typedef struct LayerProperties_s {
    void*   BufferAddressTop_p;
    void*   BufferAddressBottom_p;
} LayerProperties_t;


/* Private Constants -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Private Functions (static) ---------------------------------------------- */
static void SelectPresentationTopField(
                         const HALDISP_Handle_t    DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t* PictureBuffer_p);
static void SelectPresentationBottomField(
                         const HALDISP_Handle_t    DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t* PictureBuffer_p);
static ST_ErrorCode_t Init(const HALDISP_Handle_t HalDisplayHandle);
static void SetPresentationFrameBufferTop(
    const HALDISP_Handle_t  DisplayHandle,
    const STLAYER_Layer_t   LayerType,
    void * const            BufferAddress_p);
static void SetPresentationFrameBufferBottom(
    const HALDISP_Handle_t  DisplayHandle,
    const STLAYER_Layer_t   LayerType,
    void * const            BufferAddress_p);
static void Term(const HALDISP_Handle_t DisplayHandle);
static void DoNothingPrototype1(
                         const HALDISP_Handle_t HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static void DoNothingPrototype3(const HALDISP_Handle_t  HalDisplayHandle,
                                const STLAYER_Layer_t   LayerType,
                                const U32               HorizontalSize,
                                const U32               VerticalSize);
static void DoNothingPrototype4(
                         const HALDISP_Handle_t HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t* PictureBuffer_p);
static void SetPresentationStoredPictureProperties(
            const HALDISP_Handle_t              DisplayHandle,
            const STLAYER_Layer_t               LayerType,
            const VIDDISP_ShowParams_t * const  ShowParams_p);
static void DoNothingPrototype5(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);

/* Global Variables --------------------------------------------------------- */
const HALDISP_FunctionsTable_t  HALDISP_FunctionsGenericDigitalInput =
{
    DoNothingPrototype1,
    DoNothingPrototype1,
    DoNothingPrototype1,
    DoNothingPrototype1,
    Init,
    DoNothingPrototype4,
    DoNothingPrototype4,
    SelectPresentationBottomField,
    SelectPresentationTopField,
    SetPresentationFrameBufferBottom,
    DoNothingPrototype3,
    SetPresentationFrameBufferTop,
    SetPresentationStoredPictureProperties,
    DoNothingPrototype5,
    Term
};

/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : DoNothing
Description : Do Nothing !
Parameters  : HAL display manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void DoNothingPrototype1(const HALDISP_Handle_t    HalDisplayHandle,
                                const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(HalDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    /* Do Nothing */
}

/*******************************************************************************
Name        : DoNothing
Description : Do Nothing !
Parameters  : HAL display manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void DoNothingPrototype3(const HALDISP_Handle_t  HalDisplayHandle,
                                const STLAYER_Layer_t   LayerType,
                                const U32               HorizontalSize,
                                const U32               VerticalSize)
{
    UNUSED_PARAMETER(HalDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(HorizontalSize);
    UNUSED_PARAMETER(VerticalSize);
    /* Do Nothing */
}

/*******************************************************************************
Name        : DoNothing
Description : Do Nothing !
Parameters  : HAL display manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void DoNothingPrototype4(const HALDISP_Handle_t HalDisplayHandle,
                                const STLAYER_Layer_t     LayerType,
                                const U32 LayerIdent,
                                const VIDBUFF_PictureBuffer_t* PictureBuffer_p)
{
    UNUSED_PARAMETER(HalDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    /* Do Nothing */
}


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

    HALDISP_Properties_t*   DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    LayerProperties_t*      LayerData_p;
    ST_ErrorCode_t          Err = ST_NO_ERROR;

    /* Allocate LayerProperties_t structure */
    DisplayProperties_p->PrivateData_p = (LayerProperties_t*) STOS_MemoryAllocate(DisplayProperties_p->CPUPartition_p, sizeof(LayerProperties_t));
    if (DisplayProperties_p->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    LayerData_p = (LayerProperties_t *) DisplayProperties_p->PrivateData_p;

    /* Register  VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    Err = STEVT_RegisterDeviceEvent(DisplayProperties_p->EventsHandle, DisplayProperties_p->VideoName,
                                    VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                    &(DisplayProperties_p->UpdateGfxLayerSrcEvtID));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_RegisterDeviceEvent failed. Error = 0x%x !", Err));
    }

    return(Err);
}


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
    HALDISP_Properties_t*   DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;

    /* Unregister VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_UnregisterDeviceEvent(DisplayProperties_p->EventsHandle, DisplayProperties_p->VideoName, VIDDISP_UPDATE_GFX_LAYER_SRC_EVT);

    /* Deallocate LayerProperties_t structure */
    STOS_MemoryDeallocate(DisplayProperties_p->CPUPartition_p, DisplayProperties_p->PrivateData_p);

}

/*******************************************************************************
Name        : SetPresentationFrameBufferTop
Description : Set display Top frame buffer
Parameters  : Digital Input registers address, buffer address
Assumptions : Display Handle valid and not NULL
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationFrameBufferTop(
                         const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    HALDISP_Properties_t* DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    UNUSED_PARAMETER(LayerType);

    ((LayerProperties_t *)(DisplayProperties_p->PrivateData_p))->BufferAddressTop_p = (void*) BufferAddress_p;
}
/*******************************************************************************
Name        : SetPresentationFrameBufferBottom
Description : Set display Top frame buffer
Parameters  : Digital Input registers address, buffer address
Assumptions : Display Handle valid and not NULL
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationFrameBufferBottom(
                         const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    HALDISP_Properties_t* DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    UNUSED_PARAMETER(LayerType);

    ((LayerProperties_t *)(DisplayProperties_p->PrivateData_p))->BufferAddressBottom_p = (void*) BufferAddress_p;
}

/*******************************************************************************
Name        : SelectPresentationTopField
Description : Presents the top field
Parameters  : HAL display manager handle
Assumptions : Errors returned potentially by STLAYER are ignored
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationTopField(const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t* PictureBuffer_p)
{
    HALDISP_Properties_t*        DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    VIDDISP_LayerUpdateParams_t  UpdateParams;

    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Update DataPointer structure */
    UpdateParams.UpdateReason = VIDDISP_LAYER_UPDATE_TOP_FIELD_ADDRESS;
    UpdateParams.Data1_p      = (void*)((LayerProperties_t *)(DisplayProperties_p->PrivateData_p))->BufferAddressTop_p;

    /* Notify VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_Notify(DisplayProperties_p->EventsHandle, DisplayProperties_p->UpdateGfxLayerSrcEvtID,
                 STEVT_EVENT_DATA_TYPE_CAST &UpdateParams);
} /* End of SelectPresentationTopField() function */


/*******************************************************************************
Name        : SelectPresentationBottomField
Description : Presents the bottom field
Parameters  : HAL display manager handle
Assumptions : Errors returned potentially by STLAYER are ignored
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationBottomField(const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t* PictureBuffer_p)
{
    HALDISP_Properties_t*        DisplayProperties_p = (HALDISP_Properties_t *) HalDisplayHandle;
    VIDDISP_LayerUpdateParams_t  UpdateParams;

    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Update DataPointer structure */
    UpdateParams.UpdateReason = VIDDISP_LAYER_UPDATE_BOTTOM_FIELD_ADDRESS;
    UpdateParams.Data2_p      = (void*)((LayerProperties_t *)(DisplayProperties_p->PrivateData_p))->BufferAddressBottom_p;

    /* Notify VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_Notify(DisplayProperties_p->EventsHandle, DisplayProperties_p->UpdateGfxLayerSrcEvtID,
                 STEVT_EVENT_DATA_TYPE_CAST &UpdateParams);
} /* End of SelectPresentationBottomField() function */


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
}

/*******************************************************************************
Name        : DoNothingPrototype5
Description : Do nothing!
Parameters  : FieldsForNextVSync : Array of 3 Fields containing the Fields Previous, Current, Next.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DoNothingPrototype5(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(FieldsForNextVSync);

    /* Not used on Omega2 */
}

/* End of hv_gx_di.c */
