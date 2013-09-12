/*******************************************************************************

File name   : blitdisp.c

Description : Hal display for blitter-display functions.

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
 May 2003            Created                                      Michel Bruant

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include "stddefs.h"
#include "stevt.h"
#include "stvtg.h"
#include "halv_dis.h"
#include "blitdisp.h"
#include "stlayer.h"
#include "vid_ctcm.h"
#include "vid_disp.h"

/* Private Types ------------------------------------------------------------ */
typedef struct BlitterDisplayData_s
{
    VIDDISP_BlitterDisplayUpdateParams_t    BlitterDisplay;
    STVTG_VSYNC_t                           VSync;
    BOOL                                    PictureIsNew;
#ifdef GNBvd33476_WA
    BOOL                                    FirstTopField;
#endif
} BlitterDisplayData_t;

/* Private Macros ----------------------------------------------------------- */

#define HAL_CONTEXT_P ((BlitterDisplayData_t*) \
        (((HALDISP_Properties_t*) HalDisplayHandle)->PrivateData_p))

/* Private Functions (static) ---------------------------------------------- */

static void VSyncHALDisplay(STEVT_CallReason_t Reason,
                        const ST_DeviceName_t RegistrantName,
                        STEVT_EventConstant_t Event, const void *EventData_p,
                        const void *SubscriberData_p);

/* Statics but called from display thanks to the 'hal functions table' */
static ST_ErrorCode_t Init(const HALDISP_Handle_t HalDisplayHandle);
static void Term(const HALDISP_Handle_t DisplayHandle);

static void SelectPresentationTopField(
                        const HALDISP_Handle_t  DisplayHandle,
                        const STLAYER_Layer_t   LayerType,
                        const U32 LayerIdent,
                        const VIDBUFF_PictureBuffer_t* PictureBuffer_p);
static void SelectPresentationBottomField(
                        const HALDISP_Handle_t  DisplayHandle,
                        const STLAYER_Layer_t   LayerType,
                        const U32 LayerIdent,
                        const VIDBUFF_PictureBuffer_t* PictureBuffer_p);
static void SetPresentationLumaFrameBuffer(
                        const HALDISP_Handle_t  DisplayHandle,
                        const STLAYER_Layer_t   LayerType,
                        void * const            BufferAddress_p);
static void SetPresentationChromaFrameBuffer(
                        const HALDISP_Handle_t  DisplayHandle,
                        const STLAYER_Layer_t   LayerType,
                        void * const            BufferAddress_p);
static void DoNothingPrototype1(
                        const HALDISP_Handle_t  HalDisplayHandle,
                        const STLAYER_Layer_t   LayerType);
static void DoNothingPrototype2(const HALDISP_Handle_t  HalDisplayHandle,
                        const STLAYER_Layer_t   LayerType,
                        const U32               HorizontalSize,
                        const U32               VerticalSize);
static void DoNothingPrototype3(
                        const HALDISP_Handle_t  DisplayHandle,
                        const STLAYER_Layer_t   LayerType,
                        const VIDDISP_ShowParams_t * const  ShowParams_p);
static void DoNothingPrototype4(
                        const HALDISP_Handle_t  HalDisplayHandle,
                        const STLAYER_Layer_t   LayerType,
                        const U32 LayerIdent,
                        const VIDBUFF_PictureBuffer_t* PictureBuffer_p);
static void DoNothingPrototype5(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);


/* Global Variables --------------------------------------------------------- */

const HALDISP_FunctionsTable_t  HALDISP_BlitterDisplayFunctions =
{
    /* Fct called by upper level --------------------> Fct linked in this hal */
    /* ---------------------------------------------------------------------- */

    /* DisablePresentationChromaFieldToggle -----------> */ DoNothingPrototype1,
    /* DisablePresentationLumaFieldToggle -------------> */ DoNothingPrototype1,
    /* EnablePresentationChromaFieldToggle ------------> */ DoNothingPrototype1,
    /* EnablePresentationLumaFieldToggle --------------> */ DoNothingPrototype1,
    /* Init -------------------------------------------> */ Init,
    /* SelectPresentationChromaBottomField ------------> */ DoNothingPrototype4,
    /* SelectPresentationChromaTopField ---------------> */ DoNothingPrototype4,
    /* SelectPresentationLumaBottomField ----> */ SelectPresentationBottomField,
    /* SelectPresentationLumaTopField ----------> */ SelectPresentationTopField,
    /* SetPresentationChromaFrameBuffer --> */ SetPresentationChromaFrameBuffer,
    /* SetPresentationFrameDimensions -----------------> */ DoNothingPrototype2,
    /* SetPresentationLumaFrameBuffer ------> */ SetPresentationLumaFrameBuffer,
    /* SetPresentationStoredPictureProperties ---------> */ DoNothingPrototype3,
    /* PresentFieldsForNextVSync ---------> */ DoNothingPrototype5,
    /* Term -------------------------------------------> */ Term
};

/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : VSyncHALDisplay
Description : Function to subscribe the VSYNC event
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void VSyncHALDisplay(STEVT_CallReason_t Reason,
                const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p,
                const void *SubscriberData_p)
{
    BlitterDisplayData_t *          Data_p = (BlitterDisplayData_t*)
        ((HALDISP_Properties_t *) SubscriberData_p)->PrivateData_p;
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Store the VSync type */
    Data_p->VSync= (*(VIDDISP_VsyncLayer_t*)EventData_p).VSync;
} /* End of VSyncHALDisplay() function */

/*******************************************************************************
Name        : Init
Description : HAL Display manager handle
Parameters  :
Assumptions : HalDisplayHandle valid and not NULL
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDISP_Handle_t HalDisplayHandle)
{
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;
    HALDISP_Properties_t *          HalDispCommonData_p;
    ST_ErrorCode_t                  Err = ST_NO_ERROR;
#ifdef GNBvd33476_WA
    BlitterDisplayData_t *          Data_p;
#endif

    HalDispCommonData_p = (HALDISP_Properties_t *) HalDisplayHandle;

    /* Allocate BlitterDisplayData_t structure */
    SAFE_MEMORY_ALLOCATE(HalDispCommonData_p->PrivateData_p, BlitterDisplayData_t *, HalDispCommonData_p->CPUPartition_p, sizeof(BlitterDisplayData_t));
    if (HalDispCommonData_p->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
#ifdef GNBvd33476_WA
    Data_p = (BlitterDisplayData_t*) HalDispCommonData_p->PrivateData_p;
    Data_p->FirstTopField = FALSE;
#endif

    /* Register  VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    Err = STEVT_RegisterDeviceEvent(HalDispCommonData_p->EventsHandle,
                                HalDispCommonData_p->VideoName,
                                VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                &(HalDispCommonData_p->UpdateGfxLayerSrcEvtID));
    /* Subscribe to VSYNC EVT */
    STEVT_SubscribeParams.NotifyCallback        = VSyncHALDisplay;
    STEVT_SubscribeParams.SubscriberData_p      = (void *) (HalDisplayHandle);
    Err = STEVT_SubscribeDeviceEvent(((HALDISP_Properties_t *)
                HalDisplayHandle)->EventsHandle,
                ((HALDISP_Properties_t *) HalDisplayHandle)->VideoName,
                VIDDISP_VSYNC_EVT, &STEVT_SubscribeParams);
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
    HALDISP_Properties_t*   HalDispCommonData_p = (HALDISP_Properties_t *)
                                                             HalDisplayHandle;

    /* Unregister VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_UnregisterDeviceEvent(HalDispCommonData_p->EventsHandle,
                                HalDispCommonData_p->VideoName,
                                VIDDISP_UPDATE_GFX_LAYER_SRC_EVT);

    /* Deallocate BlitterDisplayData_t structure */
    SAFE_MEMORY_DEALLOCATE(HalDispCommonData_p->PrivateData_p, HalDispCommonData_p->CPUPartition_p, sizeof(BlitterDisplayData_t));
}

/*******************************************************************************
Name        : SetPresentationChromaFrameBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationChromaFrameBuffer(
                         const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    BlitterDisplayData_t * Data_p       = HAL_CONTEXT_P;
    UNUSED_PARAMETER(LayerType);
    
    /* Store the luma buffer address */
    Data_p->BlitterDisplay.ChromaBuff_p = BufferAddress_p;
    Data_p->PictureIsNew                = TRUE;
}

/*******************************************************************************
Name        : SetPresentationLumaFrameBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SetPresentationLumaFrameBuffer(
                         const HALDISP_Handle_t    HalDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    BlitterDisplayData_t * Data_p       = HAL_CONTEXT_P;
    UNUSED_PARAMETER(LayerType);
    
    /* Store the luma buffer address */
    Data_p->BlitterDisplay.LumaBuff_p   = BufferAddress_p;
    Data_p->PictureIsNew                = TRUE;
}

/*******************************************************************************
Name        : SelectPresentationTopField
Description : Presents the top field
Parameters  : HAL display manager handle
Assumptions :
Limitations : !!! THIS IMPLEMENTATION IS OK ONLY IF THE BLITTER-DISPLAY
        LATENCY IS EVEN (FIELD PRESENTED ON A SAME VSYNC THAN THE CURRENT ONE)
Returns     : None
*******************************************************************************/
static void SelectPresentationTopField(const HALDISP_Handle_t HalDisplayHandle,
                                       const STLAYER_Layer_t  LayerType,
                                       const U32 LayerIdent,
                                       const VIDBUFF_PictureBuffer_t* PictureBuffer_p)
{
    BlitterDisplayData_t *          Data_p = HAL_CONTEXT_P;

    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    
#ifdef GNBvd33476_WA
    Data_p->FirstTopField = TRUE;
#endif
    if(Data_p->VSync == STVTG_TOP)
    {
        /* Top on Top -> No field invertion */
        Data_p->BlitterDisplay.FieldInverted = FALSE;
    }
    else
    {
        /* Top on Bot -> Field invertion */
        Data_p->BlitterDisplay.FieldInverted = TRUE;
    }

    /* Notify VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    if((Data_p->BlitterDisplay.FieldInverted) || (Data_p->PictureIsNew))
    {
        STEVT_Notify(((HALDISP_Properties_t*) HalDisplayHandle)->EventsHandle,
             ((HALDISP_Properties_t*) HalDisplayHandle)->UpdateGfxLayerSrcEvtID,
             STEVT_EVENT_DATA_TYPE_CAST &Data_p->BlitterDisplay);
        /* ... the event is catched by the api module. Api must call the      */
        /* blitter-display thanks to 'stlayer' interface. The blitter-display */
        /* is in charge of the presentation (and mixing)                      */
    }
    if(Data_p->BlitterDisplay.FieldInverted)
    {
        Data_p->PictureIsNew = TRUE;
    }
    else
    {
        Data_p->PictureIsNew = FALSE;
    }
} /* End of SelectPresentationTopField() function */

/*******************************************************************************
Name        : SelectPresentationBottomField
Description : Presents the bottom field
Parameters  : HAL display manager handle
Assumptions :
Limitations : !!! THIS IMPLEMENTATION IS OK ONLY IF THE BLITTER-DISPLAY
        LATENCY IS EVEN (FIELD PRESENTED ON A SAME VSYNC THAN THE CURRENT ONE)
Returns     : None
*******************************************************************************/
static void SelectPresentationBottomField(
                            const HALDISP_Handle_t HalDisplayHandle,
                            const STLAYER_Layer_t LayerType,
                            const U32 LayerIdent,
                            const VIDBUFF_PictureBuffer_t* PictureBuffer_p)
{
    BlitterDisplayData_t *          Data_p = HAL_CONTEXT_P;

    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    
#ifdef GNBvd33476_WA
    if(!Data_p->FirstTopField)
        return;
#endif

    if(Data_p->VSync == STVTG_TOP)
    {
        /* Bot on Top -> Field invertion */
        Data_p->BlitterDisplay.FieldInverted = TRUE;
    }
    else
    {
        /* Bot on Bot -> No Field invertion */
        Data_p->BlitterDisplay.FieldInverted = FALSE;
    }

    /* Notify VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    if((Data_p->PictureIsNew)||(Data_p->BlitterDisplay.FieldInverted))
    {
        STEVT_Notify(((HALDISP_Properties_t*) HalDisplayHandle)->EventsHandle,
             ((HALDISP_Properties_t*) HalDisplayHandle)->UpdateGfxLayerSrcEvtID,
             STEVT_EVENT_DATA_TYPE_CAST &Data_p->BlitterDisplay);
        /* ... the event is catched by the api module. Api must call the      */
        /* blitter-display thanks to 'stlayer' interface. The blitter-display */
        /* is in charge of the presentation (and mixing)                      */
     }
    if(Data_p->BlitterDisplay.FieldInverted)
    {
        Data_p->PictureIsNew = TRUE;
    }
    else
    {
        Data_p->PictureIsNew = FALSE;
    }
} /* End of SelectPresentationBottomField() function */

/*******************************************************************************
Name        : DoNothing
Description : Do Nothing !
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
Name        : DoNothingPrototype2
Description : Do Nothing !
Returns     : None
*******************************************************************************/
static void DoNothingPrototype2(const HALDISP_Handle_t  HalDisplayHandle,
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
Name        : DoNothingPrototype3
Description : Do Nothing !
Returns     : None
*******************************************************************************/
static void DoNothingPrototype3(
                         const HALDISP_Handle_t              DisplayHandle,
                         const STLAYER_Layer_t               LayerType,
                         const VIDDISP_ShowParams_t * const  ShowParams_p)
{
    UNUSED_PARAMETER(DisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(ShowParams_p);
    /* Do Nothing */
}
/*******************************************************************************
Name        : DoNothing
Description : Do Nothing !
Returns     : None
*******************************************************************************/
static void DoNothingPrototype4(const HALDISP_Handle_t    HalDisplayHandle,
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
Name        : DoNothing
Description : Do Nothing !
Returns     : None
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
    /* Do Nothing */
}

/* End of blitdisp.c */
