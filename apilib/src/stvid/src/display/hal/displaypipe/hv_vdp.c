/*******************************************************************************

File name   : hv_vdp.c

Description : HAL video display pipe displaypipe family source file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
17 Oct 2005        Created                                           DG
*******************************************************************************/

#ifdef TRACE_UART
#include "trace.h"
#else
#define TraceBuffer(x)
#endif

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stdevice.h"

#include "stddefs.h"
#include "stos.h"

#include "vid_disp.h"
#include "halv_dis.h"
#include "hv_vdp.h"
#include "hvr_vdp.h"
#include "vid_com.h"
#include "stsys.h"
#include "sttbx.h"
#include "stgxobj.h"
#include "stevt.h"
#include "stvtg.h"

/* Private Types ------------------------------------------------------------ */

typedef enum LastFieldPresented_e
{
    LAST_FIELD_PRESENTED_NONE,  /* For init state only */
    LAST_FIELD_PRESENTED_TOP,
    LAST_FIELD_PRESENTED_BOTTOM
} LastFieldPresented_t;

typedef struct DisplayPipeProperties_s {
    STVTG_VSYNC_t       CurrentVSync[2];
} DisplayPipeProperties_t;

/* Private Constants -------------------------------------------------------- */

#define LUMA_TOP_BA_OFFSET              8
#define LUMA_BOTTOM_BA_OFFSET           136         /* = 0x88 */
#define LUMA_PROGRESSIVE_BA_OFFSET      8
#define CHROMA_TOP_BA_OFFSET            8
#define CHROMA_BOTTOM_BA_OFFSET         72           /* = 0x48 */
#define CHROMA_PROGRESSIVE_BA_OFFSET    8
#define DISP0_OFFSET                    (0)
#define DISP1_OFFSET                    (DISP1_BASE_ADDRESS - DISP0_BASE_ADDRESS)
#define DISP2_OFFSET                    (DISP2_BASE_ADDRESS - DISP0_BASE_ADDRESS)

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
static void SetPresentationStoredPictureProperties(const HALDISP_Handle_t DisplayHandle, const STLAYER_Layer_t LayerType, const VIDDISP_ShowParams_t * const  ShowParams_p);
static void PresentFieldsForNextVSync(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);
static void Term(const HALDISP_Handle_t DisplayHandle);

/* Other static functions */
static void VSyncHALDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);

static void SetFieldBufferAddress(const HALDISP_Handle_t HALDisplayHandle,
                                                             const HALDISP_Field_t *  Field_p,
                                                             void * LumaRegisterAddress,
                                                             void * ChromaRegisterAddress);
static void SendUpdateVideoDisplayNotification(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);

/* Global Variables --------------------------------------------------------- */

const HALDISP_FunctionsTable_t HALDISP_DisplayPipeFunctions = {
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
Description : Initialize chip
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDISP_Handle_t HALDisplayHandle)
{
    DisplayPipeProperties_t * Data_p;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    ST_ErrorCode_t ErrorCode;

    /* Allocate a DisplayPipeProperties_t structure */
    SAFE_MEMORY_ALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                         HALDISP_Properties_t *,
                         ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                         sizeof(DisplayPipeProperties_t));
    if (((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    Data_p = (DisplayPipeProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = FALSE;

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
                                                const VIDBUFF_PictureBuffer_t * PictureBuffer_p)
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
                                             const VIDBUFF_PictureBuffer_t * PictureBuffer_p)
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
                                              const VIDBUFF_PictureBuffer_t * PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerIdent);

    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Current Luma and Chroma Base Address are now handled in PresentFieldsForNextVSync() */

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
                                           const VIDBUFF_PictureBuffer_t * PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerIdent);

    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

     /* Current Luma and Chroma Base Address are now handled in PresentFieldsForNextVSync() */

} /* End of SelectPresentationLumaTopField() function */


/*******************************************************************************
Name        : SetPresentationChromaFrameBuffer
Description : Set display chroma frame buffer
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationChromaFrameBuffer(
                            const HALDISP_Handle_t  HALDisplayHandle,
                            const STLAYER_Layer_t   LayerType,
                            void * const            BufferAddress_p)
{
   U8* DeiRegistersBaseAddress_p = (U8 *)(((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p);
   HALDISP_Field_t *Fields_p, *CurrField_p=NULL;

    switch (LayerType)
    {
#if defined(ST_7200)
    case STLAYER_DISPLAYPIPE_VIDEO2:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p) +  DISP0_OFFSET;
        break;

    case STLAYER_DISPLAYPIPE_VIDEO3:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p) + DISP1_OFFSET;
        break;

    case STLAYER_DISPLAYPIPE_VIDEO4:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p)  + DISP2_OFFSET;
        break;
#endif /* #if defined(ST_7200) ... */
    default:
        DeiRegistersBaseAddress_p = (U8 *)(((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p);
    } /* switch (LayerType) ... */

   if( ((HALDISP_Properties_t *) HALDisplayHandle)->FieldsUsedAtNextVSync_p !=NULL)
    {
         Fields_p = ((HALDISP_Properties_t *) HALDisplayHandle)->FieldsUsedAtNextVSync_p;
         CurrField_p = &Fields_p[VIDDISP_CURRENT_FIELD];

        if (BufferAddress_p)
        {
                    /* The PictureBuffer_p can be NULL if the Field was not found */
            if ( CurrField_p->PictureBuffer_p != NULL)
            {

                        if ( CurrField_p->FieldType == HALDISP_TOP_FIELD)
                        {
                            /* Chroma */
                        HALDISP_Write32(DeiRegistersBaseAddress_p + VDP_DEI_CCF_BA,
                                                        (U32)BufferAddress_p + CHROMA_TOP_BA_OFFSET);

                        }
                        else
                        {
                            /* Chroma */
                            HALDISP_Write32(DeiRegistersBaseAddress_p + VDP_DEI_CCF_BA,
                                                            (U32)BufferAddress_p + CHROMA_BOTTOM_BA_OFFSET);
                        }
              }
              else
              {
                    /* Chroma */
                    HALDISP_Write32(DeiRegistersBaseAddress_p + VDP_DEI_CCF_BA, 0);
              }

                ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = TRUE;
        }
        else
        {
            /* BufferAddress_p is NULL: We want to hide the picture */
            ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = FALSE;
        }
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
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(HorizontalSize);
    UNUSED_PARAMETER(VerticalSize);

    /* Function not used on VDP */
} /* End of SetPresentationFrameDimensions() function */

/*******************************************************************************
Name        : SetPresentationLumaFrameBuffer
Description : Set display luma frame buffer
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationLumaFrameBuffer(
            const HALDISP_Handle_t  HALDisplayHandle,
            const STLAYER_Layer_t   LayerType,
            void * const            BufferAddress_p)
{

   U8* DeiRegistersBaseAddress_p = (U8 *)(((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p);
   HALDISP_Field_t *Fields_p,*CurrField_p=NULL;

    switch (LayerType)
    {
#if defined(ST_7200)
    case STLAYER_DISPLAYPIPE_VIDEO2:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p) + DISP0_OFFSET;
        break;

    case STLAYER_DISPLAYPIPE_VIDEO3:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p)   + DISP1_OFFSET;
        break;

    case STLAYER_DISPLAYPIPE_VIDEO4:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p)   + DISP2_OFFSET;
        break;
#endif /* #if defined(ST_7200) ... */
    default:
        DeiRegistersBaseAddress_p = (U8 *)(((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p);
    } /* switch (LayerType) ... */

   if( ((HALDISP_Properties_t *) HALDisplayHandle)->FieldsUsedAtNextVSync_p !=NULL)
    {
        Fields_p = ((HALDISP_Properties_t *) HALDisplayHandle)->FieldsUsedAtNextVSync_p;
        CurrField_p = &Fields_p[VIDDISP_CURRENT_FIELD];

        if (BufferAddress_p)
        {
        /* The PictureBuffer_p can be NULL if the Field was not found */
            if ( CurrField_p->PictureBuffer_p != NULL)
            {
                    if ( CurrField_p->FieldType == HALDISP_TOP_FIELD)
                    {
                        /* Luma */
                        HALDISP_Write32(DeiRegistersBaseAddress_p + VDP_DEI_CYF_BA,
                                                        (U32)BufferAddress_p + LUMA_TOP_BA_OFFSET);

                    }
                    else
                    {
                        /* Luma */
                        HALDISP_Write32(DeiRegistersBaseAddress_p + VDP_DEI_CYF_BA,
                                                        (U32)BufferAddress_p + LUMA_BOTTOM_BA_OFFSET);

                    }
            }
            else
            {
                /* Luma */
                HALDISP_Write32(DeiRegistersBaseAddress_p + VDP_DEI_CYF_BA, 0);
            }


            ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = TRUE;
        }
        else
        {
            /* BufferAddress_p is NULL: We want to hide the picture */
            ((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown = FALSE;
        }
   }

} /* End of SetPresentationLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetPresentationStoredPictureProperties
Description : Not used on displaypipe
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
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(ShowParams_p);

    /* Function not used on VDP */
} /* End of SetPresentationStoredPictureProperties() function */

/*******************************************************************************
Name        : PresentFieldsForNextVSync
Description : This function set the Buffer Address (Luma and Chroma) of the Previous, Current and Next Fields.

                    "Current Field"  = Field displayed at next VSync (in upper layers, this was called "NextPicture" but
                                          this is too confusing with the notion of "Previous", "Current" and "Next' Fields).
                    "Previous Field" = Field coming BEFORE the "Current Field" (Warning: it may never be displayed)
                    "Next Field"     = Field coming AFTER the "Current Field" (Warning: it may never be displayed)

                    The Current Field is the one which will be displayed at next VSync. The other Fields are presented
                    in order to be used as reference for Deinterlacing.
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
    U8 *    DeiRegistersBaseAddress_p;

    switch (LayerType)
    {
#if defined(ST_7200)
    case STLAYER_DISPLAYPIPE_VIDEO2:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p)  + DISP0_OFFSET;
        break;

    case STLAYER_DISPLAYPIPE_VIDEO3:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p)   + DISP1_OFFSET;
        break;

    case STLAYER_DISPLAYPIPE_VIDEO4:
        DeiRegistersBaseAddress_p = (U8 *) ( ( (HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p)   + DISP2_OFFSET;
        break;
#endif /* #if defined(ST_7200) ... */
    default:
        DeiRegistersBaseAddress_p = (U8 *)(((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p);
    } /* switch (LayerType) ... */

    /* Set Previous Field (i.e Field 'N-1') */
    SetFieldBufferAddress(HALDisplayHandle,
                          &(FieldsForNextVSync[VIDDISP_PREVIOUS_FIELD]),
                          DeiRegistersBaseAddress_p + VDP_DEI_PYF_BA,
                          DeiRegistersBaseAddress_p + VDP_DEI_PCF_BA);

    /* Set Current Field (i.e Field 'N')  */
    SetFieldBufferAddress(HALDisplayHandle,
                          &(FieldsForNextVSync[VIDDISP_CURRENT_FIELD]),
                          DeiRegistersBaseAddress_p + VDP_DEI_CYF_BA,
                          DeiRegistersBaseAddress_p + VDP_DEI_CCF_BA);

    /* Set Next Field (i.e Field 'N+1')  */
    SetFieldBufferAddress(HALDisplayHandle,
                          &(FieldsForNextVSync[VIDDISP_NEXT_FIELD]),
                          DeiRegistersBaseAddress_p + VDP_DEI_NYF_BA,
                          DeiRegistersBaseAddress_p + VDP_DEI_NCF_BA);

    /* Send a notification to update the display */
    SendUpdateVideoDisplayNotification(HALDisplayHandle, LayerType, LayerIdent, FieldsForNextVSync);

}/* End of PresentFieldsForNextVSync() function */


/*******************************************************************************
Name        : SetFieldBufferAddress
Description :  Set Field Buffer Address for ONE field. This function set the Luma and Chroma
                    addresses corresponding to one Field.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetFieldBufferAddress(const HALDISP_Handle_t HALDisplayHandle,
                                                             const HALDISP_Field_t *  Field_p,
                                                             void * LumaRegisterAddress,
                                                             void * ChromaRegisterAddress)
{
    U32                                 ChromaBufferAddress;      /* Address of the Frame Buffer containing the Chroma */
    U32                                 LumaBufferAddress;          /* Address of the Frame Buffer containing the Luma */
    STGXOBJ_ScanType_t       InputScanType;
    UNUSED_PARAMETER(HALDisplayHandle);

    if (((HALDISP_Properties_t *) HALDisplayHandle)->IsPictureShown)
    {
        return;
    }

    /* The PictureBuffer_p can be NULL if the Field was not found */
    if (Field_p->PictureBuffer_p != NULL)
    {
        /* OLO 01/09/2006: Taken Chedly modification originally done in SetPresentationXXXX */
        if (Field_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.BitmapType ==
                                                                                          STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
        {   /* Only used when Raster format is used: update from STVIN driver */

            /* Luma */
            HALDISP_Write32(LumaRegisterAddress,
                                          (U32) (Field_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Data1_p) );
            /* NB: Chroma Buffer is not needed when working in Raster mode */
        }
        else
        {
            /* Macroblock Format (usual case) */

            /* Set address of Frame Buffer containing the Luma */
            LumaBufferAddress = (U32) (Field_p->PictureBuffer_p->FrameBuffer_p->Allocation.Address_p);

            /* Set address of Frame Buffer containing the Chroma */
            ChromaBufferAddress = (U32) (Field_p->PictureBuffer_p->FrameBuffer_p->Allocation.Address2_p);

            InputScanType = Field_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.ScanType;

#if defined(ST_crc)
            if(VIDCRC_IsCRCRunning(((HALDISP_Properties_t *) HALDisplayHandle)->CRCHandle))
            {
                /* Force interlaced for CRC */
                InputScanType = STGXOBJ_INTERLACED_SCAN;
            }
#endif /*defined(ST_crc) */
            if (InputScanType == STGXOBJ_PROGRESSIVE_SCAN)
            {
                /* Luma */
                HALDISP_Write32(LumaRegisterAddress,
                                              LumaBufferAddress + LUMA_PROGRESSIVE_BA_OFFSET);

                /* Chroma */
                HALDISP_Write32(ChromaRegisterAddress,
                                              ChromaBufferAddress + CHROMA_PROGRESSIVE_BA_OFFSET);
            }
            else
            {
                if (Field_p->FieldType == HALDISP_TOP_FIELD)
                {
                    /* Luma */
                    HALDISP_Write32(LumaRegisterAddress,
                                                  LumaBufferAddress + LUMA_TOP_BA_OFFSET);

                    /* Chroma */
                    HALDISP_Write32(ChromaRegisterAddress,
                                                  ChromaBufferAddress + CHROMA_TOP_BA_OFFSET);
                }
                else
                {
                    /* Luma */
                    HALDISP_Write32(LumaRegisterAddress,
                                                  LumaBufferAddress + LUMA_BOTTOM_BA_OFFSET);

                    /* Chroma */
                    HALDISP_Write32(ChromaRegisterAddress,
                                                  ChromaBufferAddress + CHROMA_BOTTOM_BA_OFFSET);
                }
            }
        }
    }
    else
    {
        /* Luma */
        HALDISP_Write32(LumaRegisterAddress, 0);

        /* Chroma */
        HALDISP_Write32(ChromaRegisterAddress, 0);
    }

}/* End of SetFieldBufferAddress() function */


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
    DisplayPipeProperties_t *       Data_p;
    VIDDISP_LayerUpdateParams_t     UpdateParams;
    HALDISP_FieldType_t             NextVSyncType;

    Data_p = (DisplayPipeProperties_t *) ( (HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

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
    /* Unregister VIDDISP_UPDATE_GFX_LAYER_SRC_EVT */
    STEVT_UnregisterDeviceEvent(((HALDISP_Properties_t *) HALDisplayHandle)->EventsHandle, ((HALDISP_Properties_t *) HALDisplayHandle)->VideoName, VIDDISP_UPDATE_GFX_LAYER_SRC_EVT);

    SAFE_MEMORY_DEALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                           ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                           sizeof(DisplayPipeProperties_t));

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
    DisplayPipeProperties_t * Data_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    Data_p = (DisplayPipeProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    /* Store the VSync type */
    Data_p->CurrentVSync[(*(VIDDISP_VsyncLayer_t*)EventData_p).LayerIdent] = (*(VIDDISP_VsyncLayer_t*)EventData_p).VSync;

} /* End of VSyncHALDisplay() function */


/* End of hv_vdp.c */
