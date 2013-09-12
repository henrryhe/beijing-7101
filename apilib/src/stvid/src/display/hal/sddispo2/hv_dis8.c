/*******************************************************************************

File name   : hv_dis8.c

Description : HAL video display sddispo2 family source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
29 Oct 2002        Created                                           HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */
/* #define TRACE_HAL_DISPLAY */

#ifdef TRACE_HAL_DISPLAY
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif
#endif

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "vid_disp.h"
#include "halv_dis.h"
#include "hv_dis8.h"
#include "hvr_dis8.h"
#include "stsys.h"
#include "sttbx.h"
#include "stgxobj.h"
#include "stevt.h"
#include "stvtg.h"

#ifdef ST_OSWINCE
#include "stdevice.h"
#endif

/* Private Types ------------------------------------------------------------ */

typedef enum LastFieldPresented_e
{
    LAST_FIELD_PRESENTED_NONE,  /* For init state only */
    LAST_FIELD_PRESENTED_TOP,
    LAST_FIELD_PRESENTED_BOTTOM
} LastFieldPresented_t;

typedef struct Sdmpego2Properties_s {
    STVTG_VSYNC_t       CurrentVSync[2];

    /* Decimation */
    STVID_DecimationFactor_t StoredPictureHDecimation;
    STVID_DecimationFactor_t StoredPictureVDecimation;
} Sdmpego2Properties_t;

/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Private Macros ----------------------------------------------------------- */

/*#define HALDISP_Read8(Address)     STSYS_ReadRegDev8((void *) (Address))*/
/*#define HALDISP_Read16(Address)    STSYS_ReadRegDev16BE((void *) (Address))*/
#define HALDISP_Read32(Address)    STSYS_ReadRegDev32LE((void *) (Address))

/*#define HALDISP_Write8(Address, Value)  STSYS_WriteRegDev8((void *) (Address), (Value))*/
/*#define HALDISP_Write16(Address, Value) STSYS_WriteRegDev16BE((void *) (Address), (Value))*/
#define HALDISP_Write32(Address, Value) STSYS_WriteRegDev32LE((void *) (Address), (Value))

#define IS_FIELD_AVAILABLE(Field)  (Field.PictureBuffer_p != NULL)

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
                                                const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SelectPresentationChromaTopField(const HALDISP_Handle_t DisplayHandle,
                                             const STLAYER_Layer_t  LayerType,
                                             const U32 LayerIdent,
                                             const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SelectPresentationLumaBottomField(const HALDISP_Handle_t DisplayHandle,
                                              const STLAYER_Layer_t  LayerType,
                                              const U32 LayerIdent,
                                              const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SelectPresentationLumaTopField(const HALDISP_Handle_t DisplayHandle,
                                           const STLAYER_Layer_t  LayerType,
                                           const U32 LayerIdent,
                                           const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SetPresentationChromaFrameBuffer(const HALDISP_Handle_t DisplayHandle, const STLAYER_Layer_t LayerType, void * const BufferAddress_p);
static void SetPresentationFrameDimensions(const HALDISP_Handle_t DisplayHandle,
                                           const STLAYER_Layer_t   LayerType,
                                           const U32 HorizontalSize,
                                           const U32 VerticalSize);
static void SetPresentationLumaFrameBuffer(const HALDISP_Handle_t DisplayHandle, const STLAYER_Layer_t LayerType, void * const BufferAddress_p);
static void SetPresentationStoredPictureProperties(const HALDISP_Handle_t DisplayHandle, const STLAYER_Layer_t LayerType, const VIDDISP_ShowParams_t * const  ShowParams_p);
static void PresentFieldsForNextVSync(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);
static void Term(const HALDISP_Handle_t DisplayHandle);

/* Other static functions */
static void VSyncHALDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);

static void SendUpdateVideoDisplayNotification(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);

/* Global Variables --------------------------------------------------------- */

const HALDISP_FunctionsTable_t HALDISP_Sddispo2Functions = {
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
Description : Initialise chip
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDISP_Handle_t HALDisplayHandle)
{
    Sdmpego2Properties_t * Data_p;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    ST_ErrorCode_t ErrorCode;

    /* Allocate a Sdmpego2Properties_t structure */
    SAFE_MEMORY_ALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                         HALDISP_Properties_t *,
                         ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                         sizeof(Sdmpego2Properties_t));
    if (((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    Data_p = (Sdmpego2Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;
    /* Initialise allocated structure */
    Data_p->StoredPictureHDecimation = STVID_DECIMATION_FACTOR_NONE;
    Data_p->StoredPictureVDecimation = STVID_DECIMATION_FACTOR_NONE;

    /* Register  VIDDISP_UPDATE_GFX_LAYER_SRC_EVT in order to notify changes of field to the layer */
    ErrorCode = STEVT_RegisterDeviceEvent(((HALDISP_Properties_t *) HALDisplayHandle)->EventsHandle,
                                    ((HALDISP_Properties_t *) HALDisplayHandle)->VideoName,
                                    VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
                                    &(((HALDISP_Properties_t *) HALDisplayHandle)->UpdateGfxLayerSrcEvtID));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_RegisterDeviceEvent failed. Error = 0x%x !", ErrorCode));
    }

    /* Subscribe to VSYNC EVT */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) VSyncHALDisplay;
    STEVT_SubscribeParams.SubscriberData_p = (void *) (HALDisplayHandle);
    ErrorCode = STEVT_SubscribeDeviceEvent(((HALDISP_Properties_t *) HALDisplayHandle)->EventsHandle, ((HALDISP_Properties_t *) HALDisplayHandle)->VideoName, VIDDISP_VSYNC_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        Term(HALDisplayHandle);
        return(ST_ERROR_NO_MEMORY);
    }

/*#ifdef ST_OSWINCE
    // Map both HD and SD displays (cf "RegistersBaseAddress_p + 0x1000" elsewhere in this file!)
    {
        void* RegistersBaseAddress_p = ((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p;

        // check that the parameters are as expected for ST7100
        WCE_VERIFY(RegistersBaseAddress_p == DISP0_BASE_ADDRESS);

        // check that DISP0 (HD) & DISP1 (SD) are indeed contiguous
        WCE_VERIFY(DISP1_BASE_ADDRESS
                    == DISP0_BASE_ADDRESS + DISP_ADDRESS_WIDTH);

        RegistersBaseAddress_p = MapPhysicalToVirtual(RegistersBaseAddress_p,
                                                      2 * DISP_ADDRESS_WIDTH);
        if (RegistersBaseAddress_p == NULL)
        {
            WCE_ERROR("MapPhysicalToVirtual(2 x ST710x_DISP?_BASE_ADDRESS)");
            return ST_ERROR_NO_MEMORY;
        }
        ((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p = RegistersBaseAddress_p;
    }
#endif // ST_OSWINCE*/

    return(ErrorCode);
} /* End of Init() function */


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
                                                const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Current Luma and Chroma Base Address are now handled in PresentFieldsForNextVSync() */

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
                                             const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Current Luma and Chroma Base Address are now handled in PresentFieldsForNextVSync() */

} /* End of SelectPresentationChromaTopField() function */


/*******************************************************************************
Name        : SelectPresentationLumaBottomField
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SelectPresentationLumaBottomField(const HALDISP_Handle_t HALDisplayHandle,
                                              const STLAYER_Layer_t  LayerType,
                                              const U32 LayerIdent,
                                              const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
} /* End of SelectPresentationLumaBottomField() function */


/*******************************************************************************
Name        : SelectPresentationLumaTopField
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SelectPresentationLumaTopField(const HALDISP_Handle_t HALDisplayHandle,
                                           const STLAYER_Layer_t  LayerType,
                                           const U32 LayerIdent,
                                           const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
} /* End of SelectPresentationLumaTopField() function */


/*******************************************************************************
Name        : SetPresentationChromaFrameBuffer
Description : Set display chroma frame buffer
Parameters  : HAL display handle
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationChromaFrameBuffer(
                            const HALDISP_Handle_t  HALDisplayHandle,
                            const STLAYER_Layer_t   LayerType,
                            void * const            BufferAddress_p)
{
    if (BufferAddress_p)
    {
        /* Display Base Address Update */
        if (LayerType == STLAYER_HDDISPO2_VIDEO2)
        {
            HALDISP_Write32((U8 *) ((U32)(((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + 0x1000) +
            DISP_CHR_BA,
            (U32) BufferAddress_p);
        }
        else
        {
            HALDISP_Write32((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) +
            DISP_CHR_BA,
            (U32) BufferAddress_p);
        }
        ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = TRUE;
    }
    else
    {
        /* BufferAddress_p is NULL: We want to hide the picture */
        ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = FALSE;
    }

} /* End of SetPresentationChromaFrameBuffer() function */


/*******************************************************************************
Name        : SetPresentationFrameDimensions
Description : Set dimensions of the picture to display (source)
Parameters  : HAL display handle
Assumptions : HorizontalSize and VerticalSize are in pixels
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationFrameDimensions(const HALDISP_Handle_t HALDisplayHandle,
        const STLAYER_Layer_t   LayerType,
        const U32 HorizontalSize, const U32 VerticalSize)
{
    U16 MemoryPitch;
    Sdmpego2Properties_t * Data_p;

    HALDISP_Properties_t * HALDISP_Properties_p = (HALDISP_Properties_t *) HALDisplayHandle;
    UNUSED_PARAMETER(VerticalSize);

    Data_p = (Sdmpego2Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    if ((HALDISP_Properties_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
        (HALDISP_Properties_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
        (HALDISP_Properties_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS))

    {
        /* MPEG4P2 display buffers are aligned on an even number of macroblocks */
        MemoryPitch = (U16)( (((HorizontalSize + 15)/16 + 1) / 2) * 32);
    }
    else
    {
    /* Case of macro-block 4:2:0 and 4:2:2 reconstruction :
    Use luma width (1 byte per pixel), but align it on macro-block width (16 pix) */
    switch (Data_p->StoredPictureHDecimation)
    {
        case STVID_DECIMATION_FACTOR_H2 :
            MemoryPitch = ((HorizontalSize/2) + 15) & (~15);
            break;

        case STVID_DECIMATION_FACTOR_H4 :
            MemoryPitch = ((HorizontalSize/4) + 15) & (~15);
            break;

        case STVID_DECIMATION_FACTOR_NONE :
        default:
            /* Normal and default case : No decimation usage .*/
            MemoryPitch = (HorizontalSize + 15) & (~15);
            break;
    }
    }

    /* Write pixmap memory pitch */
    if (LayerType == STLAYER_HDDISPO2_VIDEO2)
    {
        HALDISP_Write32((U8 *) ((U32)(((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + 0x1000) +
                DISP_PMP, (U32)(MemoryPitch));
    }
    else
    {
        HALDISP_Write32((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) +
                DISP_PMP, (U32)(MemoryPitch));
    }

} /* End of SetPresentationFrameDimensions() function */


/*******************************************************************************
Name        : SetPresentationLumaFrameBuffer
Description : Set display luma frame buffer
Parameters  : HAL display handle
Assumptions : 5510: Address and size must be aligned on 2 Kbits. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationLumaFrameBuffer(
            const HALDISP_Handle_t  HALDisplayHandle,
            const STLAYER_Layer_t   LayerType,
            void * const            BufferAddress_p)
{

    if (BufferAddress_p)
    {
        /* Display Base Address Update */
        if (LayerType == STLAYER_HDDISPO2_VIDEO2)
        {
            HALDISP_Write32((U8 *) ((U32)(((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + 0x1000) +
            DISP_LUMA_BA,
            (U32) BufferAddress_p);
        }
        else
        {
            HALDISP_Write32((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) +
            DISP_LUMA_BA,
            (U32) BufferAddress_p);
        }
        ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = TRUE;
    }
    else
    {
        /* BufferAddress_p is NULL: We want to hide the picture */
        ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = FALSE;
    }
} /* End of SetPresentationLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetPresentationStoredPictureProperties
Description : Not used on sddispo2
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationStoredPictureProperties(
                                const HALDISP_Handle_t              HALDisplayHandle,
                                const STLAYER_Layer_t               LayerType,
                                const VIDDISP_ShowParams_t * const  ShowParams_p)
{
    Sdmpego2Properties_t * Data_p;
    UNUSED_PARAMETER(LayerType);

    Data_p = (Sdmpego2Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    Data_p->StoredPictureHDecimation = ShowParams_p->HDecimation;
    Data_p->StoredPictureVDecimation = ShowParams_p->VDecimation;

    /* Feature not supported */
} /* End of SetPresentationStoredPictureProperties() function */

/*******************************************************************************
Name        : PresentFieldsForNextVSync
Description : This function set the Buffer Address (Luma and Chroma) of the Previous, Current and Fields.
Parameters  : FieldsForNextVSync : Array of 3 Fields containing the Fields Previous, Current, Next.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PresentFieldsForNextVSync(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync)
{
    U32                         ChromaBufferAddress;      /* Address of the Frame Buffer containing the Chroma */
    U32                         LumaBufferAddress;          /* Address of the Frame Buffer containing the Luma */
    VIDBUFF_PictureBuffer_t *   CurrentPictureBuffer_p;
    Sdmpego2Properties_t *      Data_p;
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(FieldsForNextVSync);

    Data_p = (Sdmpego2Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    if (!((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown)
    {
        CurrentPictureBuffer_p = FieldsForNextVSync[VIDDISP_CURRENT_FIELD].PictureBuffer_p;

        LumaBufferAddress   = (U32) (CurrentPictureBuffer_p->FrameBuffer_p->Allocation.Address_p);
        ChromaBufferAddress = (U32) (CurrentPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p);

        /* Update Current Field Luma Base Address */
        if (LayerType == STLAYER_HDDISPO2_VIDEO2)
        {
            HALDISP_Write32((U8 *) ((U32)(((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + 0x1000) +
                DISP_LUMA_BA,
                LumaBufferAddress);
        }
        else
        {
            HALDISP_Write32((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) +
                DISP_LUMA_BA,
                LumaBufferAddress);
        }

        /* Update Current Field Chroma Base Address */
        if (LayerType == STLAYER_HDDISPO2_VIDEO2)
        {
            HALDISP_Write32((U8 *) ((U32)(((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + 0x1000) +
                DISP_CHR_BA,
                ChromaBufferAddress);
        }
        else
        {
            HALDISP_Write32((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) +
                DISP_CHR_BA,
                ChromaBufferAddress);
        }

        Data_p->StoredPictureHDecimation = CurrentPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors & ~(STVID_DECIMATION_FACTOR_V2|STVID_DECIMATION_FACTOR_V4);
        Data_p->StoredPictureVDecimation = CurrentPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors & ~(STVID_DECIMATION_FACTOR_H2|STVID_DECIMATION_FACTOR_H4);

        /* Set pixmap memory pitch */
        SetPresentationFrameDimensions( HALDisplayHandle,
                                        LayerType,
                                        CurrentPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width,
                                        CurrentPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height);
    }

    /* Send a notification to update the display */
    SendUpdateVideoDisplayNotification(HALDisplayHandle, LayerType, LayerIdent, FieldsForNextVSync);

}

/*******************************************************************************
Name        : SendUpdateVideoDisplayNotification
Description :  This function notifies that new Fields have been presented to the Display.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SendUpdateVideoDisplayNotification(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync)
{
    Sdmpego2Properties_t *          Data_p;
    VIDDISP_LayerUpdateParams_t     UpdateParams;
    HALDISP_FieldType_t             NextVSyncType;

    Data_p = (Sdmpego2Properties_t *) ( (HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    if (Data_p->CurrentVSync[LayerIdent] == STVTG_TOP)
    {
        /* Current VSync is Top so next VSync will be Bot */
        NextVSyncType = HALDISP_BOTTOM_FIELD;
    }
    else
    {
        /* Current VSync is Bot so next VSync will be Top */
        NextVSyncType = HALDISP_TOP_FIELD;
    }

    /* Update DataPointer structure */
    UpdateParams.UpdateReason = VIDDISP_LAYER_UPDATE_BOTTOM_FIELD_ADDRESS;
    UpdateParams.LayerType = LayerType;

    /* Check if the Field prepared for Next VSync has the expected type */
    if (FieldsForNextVSync[VIDDISP_CURRENT_FIELD].FieldType == NextVSyncType)
    {
        UpdateParams.PresentedFieldInverted = FALSE;
    }
    else
    {
        /* field inversion */
        UpdateParams.PresentedFieldInverted = TRUE;
    }

    UpdateParams.PreviousField.FieldType = FieldsForNextVSync[VIDDISP_PREVIOUS_FIELD].FieldType;
    UpdateParams.PreviousField.PictureIndex = FieldsForNextVSync[VIDDISP_PREVIOUS_FIELD].PictureIndex;
    UpdateParams.PreviousField.FieldAvailable = IS_FIELD_AVAILABLE(FieldsForNextVSync[VIDDISP_PREVIOUS_FIELD]);

    UpdateParams.CurrentField.FieldType = FieldsForNextVSync[VIDDISP_CURRENT_FIELD].FieldType;
    UpdateParams.CurrentField.PictureIndex = FieldsForNextVSync[VIDDISP_CURRENT_FIELD].PictureIndex;
    UpdateParams.CurrentField.FieldAvailable = IS_FIELD_AVAILABLE(FieldsForNextVSync[VIDDISP_CURRENT_FIELD]);

    UpdateParams.NextField.FieldType = FieldsForNextVSync[VIDDISP_NEXT_FIELD].FieldType;
    UpdateParams.NextField.PictureIndex = FieldsForNextVSync[VIDDISP_NEXT_FIELD].PictureIndex;
    UpdateParams.NextField.FieldAvailable = IS_FIELD_AVAILABLE(FieldsForNextVSync[VIDDISP_NEXT_FIELD]);

    UpdateParams.DecimationFactors = FieldsForNextVSync[VIDDISP_CURRENT_FIELD].PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;
    UpdateParams.IsDisplayStopped = ( (HALDISP_Properties_t *) HALDisplayHandle)->IsDisplayStopped;
    UpdateParams.PeriodicFieldInversionDueToFRC = ( (HALDISP_Properties_t *) HALDisplayHandle)->PeriodicFieldInversionDueToFRC;

    if (FieldsForNextVSync[VIDDISP_CURRENT_FIELD].PictureBuffer_p != NULL)
    {
        UpdateParams.AdvancedHDecimation = FieldsForNextVSync[VIDDISP_CURRENT_FIELD].PictureBuffer_p->FrameBuffer_p->AdvancedHDecimation;
    }
    else
    {
        UpdateParams.AdvancedHDecimation = FALSE;
    }

    /* Notify VIDDISP_UPDATE_GFX_LAYER_SRC_EVT for layer to computer filter parameters */
    STEVT_Notify( ( (HALDISP_Properties_t *) HALDisplayHandle)->EventsHandle,
                  ( (HALDISP_Properties_t *) HALDisplayHandle)->UpdateGfxLayerSrcEvtID,
                  STEVT_EVENT_DATA_TYPE_CAST &UpdateParams);

} /* End of SendUpdateVideoDisplayNotification() function */


/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : TRUE is empty, FALSE if not empty
*******************************************************************************/
static void Term(const HALDISP_Handle_t HALDisplayHandle)
{
#ifdef ST_OSWINCE
    /* Unmap registers */
    {
        void* RegistersBaseAddress_p = ((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p;

        WCE_VERIFY(RegistersBaseAddress_p != NULL);
        UnmapPhysicalToVirtual(RegistersBaseAddress_p);
    }
#endif /* ST_OSWINCE */

    /* Unregister VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_UnregisterDeviceEvent(((HALDISP_Properties_t *) HALDisplayHandle)->EventsHandle, ((HALDISP_Properties_t *) HALDisplayHandle)->VideoName, VIDDISP_UPDATE_GFX_LAYER_SRC_EVT);

    SAFE_MEMORY_DEALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                           ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                           sizeof(Sdmpego2Properties_t));
} /* End of Term() function */


/* Functions fully internal (not in FunctionsTable) ------------------------- */

/*******************************************************************************
Name        : VSyncHALDisplay
Description : Function to subscribe the VSYNC event
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void VSyncHALDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    const HALDISP_Handle_t HALDisplayHandle = (const HALDISP_Handle_t) SubscriberData_p;
    Sdmpego2Properties_t * Data_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    Data_p = (Sdmpego2Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    /* Store the VSync type */
    Data_p->CurrentVSync[(*(VIDDISP_VsyncLayer_t*)EventData_p).LayerIdent] = (*(VIDDISP_VsyncLayer_t*)EventData_p).VSync;

} /* End of VSyncHALDisplay() function */

/* End of hv_dis8.c */
