/*******************************************************************************

File name   : hv_di_hd.c

Description : HAL video display omega 2 family source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Feb 2001        Created                                           JA
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"
#include "vid_ctcm.h"

#include "halv_dis.h"
#include "hv_di_hd.h"
#include "hvr_dis2.h"



#include "stsys.h"
#include "stgxobj.h"

#define WA_GNBvd12746   /* "With Progressive source, bottom half of the picture has no Chroma". */
#define WA_GNBvd12749   /* "HDin storage in progressive mode is not correct".                   */
#define WA_GNBvd12756   /* "Auxilliary Display shows white or no chroma top field"              */

/* Private Types ------------------------------------------------------------ */
typedef struct {
    STGXOBJ_ScanType_t SourceScanType;      /* Current source scan type.      */
} Omega2HDinProperties_t;

/* Private Constants -------------------------------------------------------- */
#define HALDISP_ENABLE                                    1
#define HALDISP_DISABLE                                   0
#define HALDISP_ERROR                                   0xFF

/* H & V Decimations */
#define        HALDISP_HDECIMATION_MAX_REGVALUE            2
#define        HALDISP_VDECIMATION_MAX_REGVALUE            2
#define        HALDISP_COMPRESSION_MAX_REGVALUE            2
#define        HALDISP_SCAN_TYPE_MAX_REGVALUE              1

/* Private Macros ----------------------------------------------------------- */

#define HAL_Read8(Address_p)     STSYS_ReadRegDev8((void *) (Address_p))
#define HAL_Read16(Address_p)    STSYS_ReadRegDev16LE((void *) (Address_p))
#define HAL_Read32(Address_p)    STSYS_ReadRegDev32LE((void *) (Address_p))

/* Warning : For compliancy constraint, only 32BITS write are allowed         */
/* (see 5517 + STEM 7020 for more details).                                   */
#define HAL_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))

#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp,Value)                                         \
{                                                                                                           \
    U32 tmp32;                                                                                              \
    tmp32 = HAL_Read32 (Address_p);                                                                         \
    HAL_Write32 (Address_p, ((tmp32) & (RegMask) & (~((U32)(Mask) << (Emp))) ) | ((U32)(Value) << (Emp)));  \
}

#define HAL_GetRegister16Value(Address_p, Mask, Emp)  (((U16)(HAL_Read16(Address_p))&((U16)(Mask)<<(Emp)))>>(Emp))
#define HAL_GetRegister8Value(Address_p, Mask, Emp)   (((U8)(HAL_Read8(Address_p))&((U8)(Mask)<<(Emp)))>>(Emp))

/* Private Variables (static)------------------------------------------------ */
static U8 RegValueFromEnum(U8 *Reg_Value_Tab, U8 Enum_Value, U8 Max);

static U8 haldisp_RegValueHDecimation[] = {
    STVID_DECIMATION_FACTOR_NONE,
    STVID_DECIMATION_FACTOR_H2,
    STVID_DECIMATION_FACTOR_H4};
static U8 haldisp_RegValueVDecimation[] = {
    STVID_DECIMATION_FACTOR_NONE,
    STVID_DECIMATION_FACTOR_V2,
    STVID_DECIMATION_FACTOR_V4};
static U8 haldisp_RegValueScanType[] = {
    STGXOBJ_INTERLACED_SCAN,
    STGXOBJ_PROGRESSIVE_SCAN};

#ifdef DEBUG
static void HALDISP_Error(void);
#endif

/* Private Functions (static) ---------------------------------------------- */

static void DisablePresentationChromaFieldToggle(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static void DisablePresentationLumaFieldToggle(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static void EnablePresentationChromaFieldToggle(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static void EnablePresentationLumaFieldToggle(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType);
static ST_ErrorCode_t Init(const HALDISP_Handle_t HALDisplayHandle);
static void SelectPresentationChromaBottomField(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
static void SelectPresentationChromaTopField(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
static void SelectPresentationLumaBottomField(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
static void SelectPresentationLumaTopField(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
static void SetPresentationChromaFrameBuffer(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         void * const BufferAddress_p);
static void SetPresentationFrameDimensions(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t   LayerType,
                         const U32 HorizontalSize,
                         const U32 VerticalSize);
static void SetPresentationLumaFrameBuffer(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         void * const BufferAddress_p);
static void SetPresentationStoredPictureProperties(
                         const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t LayerType,
                         const VIDDISP_ShowParams_t * const  ShowParams_p);
static void PresentFieldsForNextVSync(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);
static void Term(const HALDISP_Handle_t HALDisplayHandle);

/* Global Variables --------------------------------------------------------- */
 const HALDISP_FunctionsTable_t  HALDISP_Omega2FunctionsHighDefinition =
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
Name        : Init
Description : HAL Display manager handle
Parameters  : Hal display handle.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDISP_Handle_t HALDisplayHandle)
{
    Omega2HDinProperties_t * Data_p;

    /* memory allocation for private flags structure if necessary */
    /* Allocate a Omega2HDinProperties_t structure */
    SAFE_MEMORY_ALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                         Omega2HDinProperties_t *,
                         ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                         sizeof(Omega2HDinProperties_t));
    if (((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Default value of this structure.           */
    Data_p = (Omega2HDinProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;
    Data_p->SourceScanType      = STGXOBJ_INTERLACED_SCAN;

    return(ST_NO_ERROR);

} /* End of Init() function. */


/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  : HAL Display manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void Term(const HALDISP_Handle_t HALDisplayHandle)
{
    /* memory deallocation if necessary */
    SAFE_MEMORY_DEALLOCATE(((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p,
                           ((HALDISP_Properties_t *) HALDisplayHandle)->CPUPartition_p,
                           sizeof(Omega2HDinProperties_t));
} /* End of Term() function. */

/*******************************************************************************
Name        : HALDISP_SetDisplayLumaFrameBuffer
Description : Set display luma frame buffer
Parameters  : Display registers address, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationLumaFrameBuffer(
                         const HALDISP_Handle_t    HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    UNUSED_PARAMETER(LayerType);
    
    /* ensure that it is 512 bytes aligned */
    #ifdef DEBUG
    if (((U32) BufferAddress_p) & ~HDIN_PRESENTATION_MASK)
    {
        HALDISP_Error();
    }
    #endif

    /* main layer */
    HAL_Write32((U8 *)(U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFP_32, (U32) BufferAddress_p);
}


/*******************************************************************************
Name        : HALDISP_SetDisplayChromaFrameBuffer
Description : Set display chroma frame buffer
Parameters  : Digital Input registers address, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationChromaFrameBuffer(
                         const HALDISP_Handle_t    HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    UNUSED_PARAMETER(LayerType);
    
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~HDIN_PRESENTATION_MASK)
    {
        HALDISP_Error();
    }
    #endif

    /* main layer */
    /* Beginning of the chroma frame buffer to display, in unit of 512 bytes */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PCHP_32, (U32) BufferAddress_p);
}


/*******************************************************************************
Name        : HALDISP_SetPresentationFrameDimensions
Description : Set dimensions of the Viewport to display
Parameters  : HAL display manager handle, Width, Height
Assumptions : Width and Height are in pixels
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationFrameDimensions(
                         const HALDISP_Handle_t  HALDisplayHandle,
                         const STLAYER_Layer_t   LayerType,
                         const U32               HorizontalSize,
                         const U32               VerticalSize)
{
    U32 WidthInMacroBlock ;
    U32 HeightInMacroBlock ;
    UNUSED_PARAMETER(LayerType);
   
    /* width in macroblocks */
    WidthInMacroBlock = (HorizontalSize + 15) / 16;

    /* Height in macroblocks */
    HeightInMacroBlock = (VerticalSize + 15) / 16;

    /* Write width in macroblocks in presentation width buffer */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFW, WidthInMacroBlock);

#ifdef WA_GNBvd12746
    /* Write Height in macroblocks in presentation Height buffer */
    if ( (HeightInMacroBlock * 2) < VIDn_PFH_MASK)
    {
        HAL_Write32((U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFH,
                HeightInMacroBlock * 2);
    }
    else
    {
        HAL_Write32((U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFH,
                VIDn_PFH_MASK);
    }
#else
#ifdef WA_GNBvd12756
    /* Write Height in macroblocks in presentation Height buffer */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFH, HeightInMacroBlock + 4);
#else
    /* Write Height in macroblocks in presentation Height buffer */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFH, HeightInMacroBlock);
#endif /* WA_GNBvd12756 */
#endif
}

/*******************************************************************************
Name        : SelectPresentationLumaTopField
Description : Presents the top field of the luma part of the frame to both main
              and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically
              toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationLumaTopField(
                        const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    
#ifdef WA_GNBvd12749
    /* Test the pictue source scan type.    */

    if (((Omega2HDinProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p)->SourceScanType
            == STGXOBJ_PROGRESSIVE_SCAN)
    {
        /* If source picture is progressive, always show bottom fields. */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_BTY_MASK,
                HDIN_BTY_EMP,
                HALDISP_ENABLE);
    }
    else
    {
        /* Normal case. Select top field.   */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_BTY_MASK,
                HDIN_BTY_EMP,
                HALDISP_DISABLE);
    }
#else
    /* Main Display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_BTY_MASK,
            HDIN_BTY_EMP,
            HALDISP_DISABLE);
#endif
} /* End of SelectPresentationLumaTopField() function. */

/*******************************************************************************
Name        : SelectPresentationLumaBottomField
Description : Presents the bottom field of the luma part of the frame to both main and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationLumaBottomField(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_BTY_MASK,
            HDIN_BTY_EMP,
            HALDISP_ENABLE);
}

/*******************************************************************************
Name        : SelectPresentationChromaTopField
Description : Presents the top field of the chroma part of the frame to both main and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationChromaTopField(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    
#ifdef WA_GNBvd12749
    /* Test the picture source scan type.    */
    if (((Omega2HDinProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p)->SourceScanType
            == STGXOBJ_PROGRESSIVE_SCAN)
    {
        /* If source picture is progressive, always show bottom fields. */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_BTC_MASK,
                HDIN_BTC_EMP,
                HALDISP_ENABLE);
    }
    else
    {
        /* Normal case. Select top field.   */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_BTC_MASK,
                HDIN_BTC_EMP,
                HALDISP_DISABLE);
    }
#else
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_BTC_MASK,
            HDIN_BTC_EMP,
            HALDISP_DISABLE);
#endif
} /* End of SelectPresentationChromaTopField() function. */


/*******************************************************************************
Name        : SelectPresentationChromaBottomField
Description : Presents the bottom field of the chroma part of the frame to both main and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationChromaBottomField(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);
    
    /* Main Display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_BTC_MASK,
            HDIN_BTC_EMP,
            HALDISP_ENABLE);
}


/*******************************************************************************
Name        : EnablePresentationLumaFieldToggle
Description : Enables the chroma field selection on the presentation frame to toggle automatically on each VSYNC.
Description : HAL display manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void EnablePresentationLumaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(LayerType);
    
#ifdef WA_GNBvd12749
    /* Test the picture source scan type.                           */
    if (((Omega2HDinProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p)->SourceScanType
            == STGXOBJ_PROGRESSIVE_SCAN)
    {
        /* If source picture is progressive, forbid field toggle.   */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_TYEN_MASK,
                HDIN_TYEN_EMP,
                HALDISP_DISABLE);
    }
    else
    {
        /* Normal case. Allow field toggle.                         */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_TYEN_MASK,
                HDIN_TYEN_EMP,
                HALDISP_ENABLE);
    }
#else
    /* main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_TYEN_MASK,
            HDIN_TYEN_EMP,
            HALDISP_ENABLE);
#endif
}


/*******************************************************************************
Name        : DisablePresentationLumaFieldToggle
Description : Disables the luma field selection on the presentation frame to toggle automatically on ecah VSYNC
              the field selected can be frozen or set every field by the application.
Parameters  : HAL display manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void DisablePresentationLumaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(LayerType);
    
    /* Main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_TYEN_MASK,
            HDIN_TYEN_EMP,
            HALDISP_DISABLE);
}

/*******************************************************************************
Name        : EnablePresentationChromaFieldToggle
Description : Enables the chroma field selection on the presentation frame to toggle automatically on each VSYNC.
Description : HAL display manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void EnablePresentationChromaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{    
    UNUSED_PARAMETER(LayerType);
    
#ifdef WA_GNBvd12749
    /* Test the picture source scan type.                           */
    if (((Omega2HDinProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p)->SourceScanType
            == STGXOBJ_PROGRESSIVE_SCAN)
    {
        /* If source picture is progressive, forbid field toggle.   */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_TCEN_MASK,
                HDIN_TCEN_EMP,
                HALDISP_DISABLE);
    }
    else
    {
        /* Normal case. Allow field toggle.                         */
        HAL_SetRegister32Value(
                (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
                HDIN_PFLD_MASK,
                HDIN_TCEN_MASK,
                HDIN_TCEN_EMP,
                HALDISP_ENABLE);
    }
#else
    /* Main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_TCEN_MASK,
            HDIN_TCEN_EMP,
            HALDISP_ENABLE);
#endif
}


/*******************************************************************************
Name        : DisablePresentationChromaFieldToggle
Description : Disables the chroma field selection on the presentation frame to toggle automatically on ecah VSYNC
              the field selected can be frozen or set every field by the application.
Parameters  : HAL display manager handle
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static void DisablePresentationChromaFieldToggle(const HALDISP_Handle_t HALDisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(LayerType);
    
    /* Main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PFLD,
            HDIN_PFLD_MASK,
            HDIN_TCEN_MASK,
            HDIN_TCEN_EMP,
            HALDISP_DISABLE);
}

/*******************************************************************************
Name        : SetPresentationStoredPictureProperties
Description : Shows how the presented picture has been stored.
Parameters  : HAL display manager handle
              Parameters to display (scantype, decimation, ...).
Assumptions : ShowParams_p is not NULL
Limitations :
Returns     :
*******************************************************************************/
static void SetPresentationStoredPictureProperties(
                         const HALDISP_Handle_t              HALDisplayHandle,
                         const STLAYER_Layer_t               LayerType,
                         const VIDDISP_ShowParams_t * const  ShowParams_p)
{
    U16 HDecimationToReg, VDecimationToReg, ScanTypeToReg;
    Omega2HDinProperties_t * Data_p;
    UNUSED_PARAMETER(LayerType);
    
    Data_p = (Omega2HDinProperties_t *) ((HALDISP_Properties_t *) HALDisplayHandle)->PrivateData_p;

    HDecimationToReg = (U16)RegValueFromEnum(haldisp_RegValueHDecimation, ShowParams_p->HDecimation, HALDISP_HDECIMATION_MAX_REGVALUE);
    VDecimationToReg = (U16)RegValueFromEnum(haldisp_RegValueVDecimation, ShowParams_p->VDecimation, HALDISP_VDECIMATION_MAX_REGVALUE);

#ifdef WA_GNBvd12749
    ScanTypeToReg = (U32)RegValueFromEnum(haldisp_RegValueScanType, STGXOBJ_INTERLACED_SCAN, HALDISP_SCAN_TYPE_MAX_REGVALUE);
#else
    ScanTypeToReg = (U16)RegValueFromEnum(haldisp_RegValueScanType, ShowParams_p->ScanType, HALDISP_SCAN_TYPE_MAX_REGVALUE);
#endif

    /* main display */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)HALDisplayHandle)->RegistersBaseAddress_p + HDIN_PMOD_32,
                (ScanTypeToReg << HDIN_MOD_PDC_EMP | ScanTypeToReg << HDIN_MOD_PDL_EMP |
                HDecimationToReg << HDIN_MOD_HDEC_EMP | VDecimationToReg << HDIN_MOD_VDEC_EMP) & HDIN_PMOD_MASK );

    /* Remember the current source picture scantype.    */
    Data_p->SourceScanType = ShowParams_p->ScanType;

} /* End of SetPresentationStoredPictureProperties() function. */

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
    
    /* Not used on Omega2 */
}

/*******************************************************************************
Name        : RegValueFromEnum
Description : Finds the value to write in the corresponding register from The enum Value parameter
Parameters  :
Assumptions :
Limitations :
Returns     : Value to be written in the corresponding register
*******************************************************************************/
static U8 RegValueFromEnum(U8 *RegValueTab, U8 EnumValue, U8 RegValueMax)
{
    U8 i=0;
    while (i <= RegValueMax)
    {
        if (RegValueTab[i] == EnumValue)
        {
            return (i);
        }
    i ++ ;
    }
    #ifdef DEBUG
    HALDISP_Error();
    #endif
    return (HALDISP_ERROR);
}

/*******************************************************************************
Name        : HALDISP_Error
Description : Function called every time an error occurs. A breakpoint here
              should facilitate the debug
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef DEBUG
void HALDISP_Error(void)
{
    while (1);
}
#endif


/* End of hv_di_hd.c */
