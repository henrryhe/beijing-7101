/*******************************************************************************

File name   : hv_dis1.c

Description : HAL video display omega 1 family source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
20 Jul 2000        Created                                           HG
25 Jan 2001        Changed CPU memory access to device access        HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */
/* #define TRACE_UART */
#ifdef TRACE_UART
#include "trace.h"
#else
#define TraceBuffer(x)
#endif

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "vid_disp.h"
#include "halv_dis.h"
#include "hv_dis1.h"
#include "hvr_dis1.h"

#include "stsys.h"
#include "sttbx.h"
#include "stgxobj.h"

#include "stevt.h"
#include "stvtg.h"


/* Private Types ------------------------------------------------------------ */

typedef struct {
    STVTG_VSYNC_t  CurrentVSync;
    BOOL           IsCurrentFieldOnDisplayTop;
    BOOL           IsNextFieldOnDisplayTop;
    BOOL           IsFieldToggleAutomatic;
} Omega1Properties_t;

/* Private Constants -------------------------------------------------------- */

#define FRZ_BIT_55XX_NORMAL (VID_DCF_FRZ >> 8)  /* since it's define on a 16 bits words*/
#define FRZ_BIT_55X8        (VID_DCF_FRM >> 8)  /* since it's define on a 16 bits words*/
#define FPO_BIT_55X8        (VID_DCF_FPO >> 8)  /* since it's define on a 16 bits words*/
#define FPO_BIT_5516        (VID_DCF_FPO >> 7)  /* since it's define on a 16 bits words*/


/* Private Variables (static)------------------------------------------------ */


/* Private Macros ----------------------------------------------------------- */

#define HALDISP_Read8(Address)     STSYS_ReadRegDev8((void *) (Address))
#define HALDISP_Read16(Address)    STSYS_ReadRegDev16BE((void *) (Address))
#define HALDISP_Read32(Address)    STSYS_ReadRegDev32BE((void *) (Address))

#define HALDISP_Write8(Address, Value)  STSYS_WriteRegDev8((void *) (Address), (Value))
#define HALDISP_Write16(Address, Value) STSYS_WriteRegDev16BE((void *) (Address), (Value))
#define HALDISP_Write32(Address, Value) STSYS_WriteRegDev32BE((void *) (Address), (Value))

/* Private Function prototypes ---------------------------------------------- */

/* Functions in HALDISP_FunctionsTable_t */
static void DisablePresentationChromaFieldToggle(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static void DisablePresentationLumaFieldToggle(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static void EnablePresentationChromaFieldToggle(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static void EnablePresentationLumaFieldToggle(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static ST_ErrorCode_t Init(const HALDISP_Handle_t DisplayHandle);
static void SelectPresentationChromaBottomField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SelectPresentationChromaTopField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SelectPresentationLumaBottomField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SelectPresentationLumaTopField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
static void SetPresentationChromaFrameBuffer(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         void * const BufferAddress_p);
static void SetPresentationFrameDimensions(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t   LayerType,
                         const U32 HorizontalSize,
                         const U32 VerticalSize);
static void SetPresentationLumaFrameBuffer(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         void * const BufferAddress_p);
static void SetPresentationStoredPictureProperties(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         const VIDDISP_ShowParams_t * const  ShowParams_p);
static void PresentFieldsForNextVSync(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);
static void Term(const HALDISP_Handle_t DisplayHandle);

/* Other static functions */
static void VSyncHALDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);


/* Global Variables --------------------------------------------------------- */

const HALDISP_FunctionsTable_t HALDISP_Omega1Functions = {
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
                         const STLAYER_Layer_t LayerType)
{
    Omega1Properties_t * Data_p;
    U8 DcfHigh;

    UNUSED_PARAMETER(LayerType);

    DcfHigh = HALDISP_Read8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1);

    Data_p = (Omega1Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;
    Data_p->IsFieldToggleAutomatic = FALSE;

    /* Freeze on ST5510-12, nothing to do on 5508-18 */
    switch (((HALDISP_Properties_t *) HALDisplayHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            /* Enable freeze on polarity.   */
            DcfHigh |= FRZ_BIT_55XX_NORMAL;
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
            break;

        default:
            break;
    }
}


/*******************************************************************************
Name        : DisablePresentationLumaFieldToggle
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DisablePresentationLumaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    /* Freeze on ST5510-12, nothing to do on 5508-18 */
    /* DisablePresentationChromaFieldToggle(HALDisplayHandle); */
}


/*******************************************************************************
Name        : EnablePresentationChromaFieldToggle
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnablePresentationChromaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType)
{
    Omega1Properties_t * Data_p;
    U8 DcfHigh;
    UNUSED_PARAMETER(LayerType);
    
    /* de-Freeze */
    DcfHigh = HALDISP_Read8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1);

    /* shortcut */
    Data_p = (Omega1Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;
    Data_p->IsFieldToggleAutomatic = TRUE;

    switch (((HALDISP_Properties_t *) HALDisplayHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
            break;

        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            /* Disable freeze on polarity.   */
            DcfHigh &= ~FRZ_BIT_55XX_NORMAL;
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
            break;

        default:
            /* nothing to do */
            break;
    }
}


/*******************************************************************************
Name        : EnablePresentationLumaFieldToggle
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnablePresentationLumaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    /* de-Freeze */
    /* EnablePresentationChromaFieldToggle(HALDisplayHandle); */
}


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
    Omega1Properties_t * Data_p;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    ST_ErrorCode_t ErrorCode;

    /* Allocate a Omega1Properties_t structure */
    SAFE_MEMORY_ALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                         Omega1Properties_t *,
                         ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                          sizeof(Omega1Properties_t));
    if (((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    Data_p = (Omega1Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;
    Data_p->CurrentVSync = STVTG_TOP;
    Data_p->IsCurrentFieldOnDisplayTop = TRUE;
    Data_p->IsNextFieldOnDisplayTop = TRUE;
    Data_p->IsFieldToggleAutomatic = FALSE;

    /* Subscribe to VSYNC EVT */
    STEVT_SubscribeParams.NotifyCallback = VSyncHALDisplay;
    STEVT_SubscribeParams.SubscriberData_p = (void *) (HALDisplayHandle);
    ErrorCode = STEVT_SubscribeDeviceEvent(((HALDISP_Properties_t *) HALDisplayHandle)->EventsHandle, ((HALDISP_Properties_t *) HALDisplayHandle)->VideoName, VIDDISP_VSYNC_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        Term(HALDisplayHandle);
        return(ST_ERROR_NO_MEMORY);
    }

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : SelectPresentationChromaBottomField
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SelectPresentationChromaBottomField(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    /* Feature not supported */
}


/*******************************************************************************
Name        : SelectPresentationChromaTopField
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SelectPresentationChromaTopField(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    /* Feature not supported */
}


/*******************************************************************************
Name        : SelectPresentationLumaBottomField
Description :
Parameters  : HAL display handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SelectPresentationLumaBottomField(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    Omega1Properties_t * Data_p;
    U8 DcfLow;
    U8 DcfHigh;
    
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    
    TraceBuffer(("PutBot.."));
    /* shortcut */
    Data_p = (Omega1Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;
    /* Update the flag considering we have done the job. */
    Data_p->IsNextFieldOnDisplayTop = FALSE;

    DcfLow = HALDISP_Read8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF0);
    DcfHigh = HALDISP_Read8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1);

    switch (((HALDISP_Properties_t *) HALDisplayHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
            if (Data_p->IsFieldToggleAutomatic == FALSE)
            {
                if (Data_p->IsCurrentFieldOnDisplayTop == FALSE)
                {
                    /* We want to put Bot, and we already display a Bot */
                    /* Enable freeze on polarity.   */
                    TraceBuffer(("..current bot-> freeze..."));
                    DcfHigh |= FRZ_BIT_55XX_NORMAL;
                    HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
                    Data_p->IsNextFieldOnDisplayTop = FALSE;
                }
                else   /* We want to put Bot, but display top */
                {
                    if (Data_p->CurrentVSync == STVTG_TOP)
                    {
                        TraceBuffer(("..current top but vtg allows to defreeze..."));
                        /* polarity with vtg is OK, normal case */
                        /* Disable freeze on polarity.   */
                        DcfHigh &= ~FRZ_BIT_55XX_NORMAL;
                        HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
                        Data_p->IsNextFieldOnDisplayTop = FALSE;
                    }
                    else
                    {
                        TraceBuffer(("..not supported->freeze.."));
                        /* polarity with vtg is KO !! */
                        /* The chip doesn't support this case. */
                        Data_p->IsNextFieldOnDisplayTop = TRUE;
                        /* Enable freeze on polarity.   */
                        DcfHigh |= FRZ_BIT_55XX_NORMAL;
                        HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
                    }
                }
                TraceBuffer(("Declare next= %s\r\n", Data_p->IsNextFieldOnDisplayTop ? "TOP" : "BOT"));
            }
           break;

        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            /* Enable freeze on polarity.   */
            DcfHigh |= FRZ_BIT_55X8;
            /* Freeze on BOTTOM field.      */
            DcfHigh |= FPO_BIT_55X8;
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
            break;

        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            /* Enable freeze on polarity.   */
            DcfHigh |= FRZ_BIT_55XX_NORMAL;
            /* Freeze on BOTTOM field.      */
            DcfLow |= FPO_BIT_5516;
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF0 , DcfLow);
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
            break;

        default :
            /* The chips is undefined. No action to perform. */
            break;
    }

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
                         const STLAYER_Layer_t LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    Omega1Properties_t * Data_p;
    U8 DcfLow;
    U8 DcfHigh;

    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    TraceBuffer(("PutTop.."));
    /* shortcut */
    Data_p = (Omega1Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;
    /* Update the flag considering we have done the job. */
    Data_p->IsNextFieldOnDisplayTop = TRUE;

    DcfLow = HALDISP_Read8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF0);
    DcfHigh = HALDISP_Read8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1);

    switch (((HALDISP_Properties_t *) HALDisplayHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
            if (Data_p->IsFieldToggleAutomatic == FALSE)
            {
                if (Data_p->IsCurrentFieldOnDisplayTop == TRUE)
                {
                    TraceBuffer(("..current top-> freeze..."));
                    /* We want to display Top and already display Top */
                    /* Enable freeze on polarity.   */
                    DcfHigh |= FRZ_BIT_55XX_NORMAL;
                    HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
                    Data_p->IsNextFieldOnDisplayTop = TRUE;

                }
                else  /* We want to display Top but current display is Bot */
                {
                    /* if polarity with vtg is OK : normal case */
                    if (Data_p->CurrentVSync == STVTG_BOTTOM)
                    {
                        TraceBuffer(("..current bot but vtg allows to defreeze..."));
                        /* Disable freeze on polarity.   */
                        DcfHigh &= ~FRZ_BIT_55XX_NORMAL;
                        HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
                        Data_p->IsNextFieldOnDisplayTop = TRUE;                }
                    else
                    {
                        TraceBuffer(("..not supported->freeze.."));
                        /* polarity with vtg is KO */
                        /* The chip doesn't support this case. */
                        Data_p->IsNextFieldOnDisplayTop = FALSE;
                        /* Enable freeze on polarity.   */
                        DcfHigh |= FRZ_BIT_55XX_NORMAL;
                        HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
                    }
                }
                TraceBuffer(("Declare next= %s\r\n", Data_p->IsNextFieldOnDisplayTop ? "TOP" :"BOT" ));
            }
          break;

        case STVID_DEVICE_TYPE_5508_MPEG :
        case STVID_DEVICE_TYPE_5518_MPEG :
            /* Enable freeze on polarity.   */
            DcfHigh |= FRZ_BIT_55X8;
            /* Freeze on TOP field.         */
            DcfHigh &= ~FPO_BIT_55X8;
            /* As the Hardware latches the register's value until the next VSync occurs, this function can write immediately */
            /* on the register. */
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
            break;

        case STVID_DEVICE_TYPE_5516_MPEG :
        case STVID_DEVICE_TYPE_5517_MPEG :
            /* Enable freeze on polarity.   */
            DcfHigh |= FRZ_BIT_55XX_NORMAL;
            /* Freeze on TOP field.         */
            DcfLow &= ~FPO_BIT_5516;
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF0 , DcfLow);
            HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
            break;

        default :
            /* The chips is undefined. No action to perform. */
            break;
    }

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
    UNUSED_PARAMETER(LayerType);
    /* Beginning of the chroma frame buffer to display, in unit of 2 Kbits */
    HALDISP_Write16((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DFC16 , ((U32) BufferAddress_p) / 256);
}


/*******************************************************************************
Name        : SetPresentationFrameDimensions
Description : Set dimensions of the picture to display
Parameters  : HAL display handle
Assumptions : HorizontalSize and VerticalSize are in pixels
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationFrameDimensions(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t   LayerType,
        const U32 HorizontalSize, const U32 VerticalSize)
{
    U8 WidthInMacroBlock = (HorizontalSize + 15) / 16;

    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(VerticalSize);
    
/*    U16 TotalInMacroBlock;*/

/*    TotalInMacroBlock = (Height + 15) / 16;*/
/*    TotalInMacroBlock = TotalInMacroBlock * WidthInMacroBlock;*/

    /* Write width in macroblocks */
     HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_XFW, WidthInMacroBlock);


    /* Write total number of macroblocks (doesn't exist) */
/*    HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_XFS, (U8) (TotalInMacroBlock >> 8));*/
/*    HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_XFS, (U8) TotalInMacroBlock);*/
}


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
    UNUSED_PARAMETER(LayerType);
    
    /* Beginning of the luma frame buffer to display, in unit of 2 Kbits */
    HALDISP_Write16((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DFP16, ((U32) BufferAddress_p) / 256);
}


/*******************************************************************************
Name        : SetPresentationStoredPictureProperties
Description : Not used on Omega 1.
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
    /* Feature not supported */
}

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
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(FieldsForNextVSync);
    
    /* Not used on Omega1 */
}

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
    SAFE_MEMORY_DEALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                           ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                           sizeof(Omega1Properties_t));
}

/*******************************************************************************
Name        : VSyncHALDisplay
Description : Function to subscribe the VSYNC event
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void VSyncHALDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    const HALDISP_Handle_t HALDisplayHandle = (const HALDISP_Handle_t) SubscriberData_p;
    U8 DcfHigh;
    Omega1Properties_t * Data_p;

    STVTG_VSYNC_t VSync = *((int *) EventData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    TraceBuffer(("HAL VTG is %s\r\n"  ,VSync ? "BOT" : "TOP"));

    Data_p = (Omega1Properties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    Data_p->CurrentVSync = (*(VIDDISP_VsyncLayer_t*)EventData_p).VSync;
    Data_p->IsCurrentFieldOnDisplayTop = Data_p->IsNextFieldOnDisplayTop;

    DcfHigh = HALDISP_Read8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1);

    switch (((HALDISP_Properties_t *) HALDisplayHandle)->DeviceType)
    {
        case STVID_DEVICE_TYPE_5510_MPEG :
        case STVID_DEVICE_TYPE_5512_MPEG :
        case STVID_DEVICE_TYPE_5514_MPEG :
            if (Data_p->IsFieldToggleAutomatic == FALSE)
            {
                /* If we are not freezed (normal case) */
                if ((DcfHigh & FRZ_BIT_55XX_NORMAL) == 0)
                {
                    TraceBuffer(("    We are not freezed\r\n"  ,VSync));
                    if (VSync == STVTG_TOP)
                    {
                        Data_p->IsNextFieldOnDisplayTop = TRUE; /* because we are going to freeze */;
                    }
                    else
                    {
                        Data_p->IsNextFieldOnDisplayTop = FALSE; /* because we are going to freeze */;
                    }
                }

                /* Enable freeze on polarity.   */
                DcfHigh |= FRZ_BIT_55XX_NORMAL;
                HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
                TraceBuffer(("    Current:%s   Next:%s\r\n",
                        Data_p->IsCurrentFieldOnDisplayTop ? "TOP" : "BOT",
                        Data_p->IsNextFieldOnDisplayTop    ? "TOP" : "BOT"));
            }
            else
            {
                /* Disable freeze on polarity.   */
                DcfHigh &= ~FRZ_BIT_55XX_NORMAL;
                HALDISP_Write8((U8 *) (((HALDISP_Properties_t *) HALDisplayHandle)->RegistersBaseAddress_p) + VID_DCF1 , DcfHigh);
            }
            break;

        default :
            break;
   }

} /* End of VSyncHALDisplay() function */

/* End of hv_dis1.c */
