/*******************************************************************************

File name   : hv_di_sd.c

Description : HAL video display omega 2 family source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Feb 2001        Created                                           JA

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */
#define GNBvd06059 1
#define GNBvd06183 1
#define WA_GNBvd12756   /* "Auxilliary Display shows white or no chroma top field"              */
/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "halv_dis.h"
#include "hv_di_sd.h"
#include "hvr_dis2.h"

#include "stsys.h"
#include "stgxobj.h"


/* Private Types ------------------------------------------------------------ */

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

#ifdef DEBUG
static void HALDISP_Error(void);
#endif

/* Private Functions (static) ---------------------------------------------- */

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
static ST_ErrorCode_t Init(const HALDISP_Handle_t HALDisplayHandle);
static void SelectPresentationChromaBottomField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
static void SelectPresentationChromaTopField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
static void SelectPresentationLumaBottomField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
static void SelectPresentationLumaTopField(
                         const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p);
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



/* Global Variables --------------------------------------------------------- */
const HALDISP_FunctionsTable_t  HALDISP_Omega2FunctionsStdDefinition =
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
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDISP_Handle_t HALDisplayHandle)
{
    UNUSED_PARAMETER(HALDisplayHandle);
   /* memory allocation for private flags structure if necessary */

   return(ST_NO_ERROR);

}


/*******************************************************************************
Name        : Term
Description : Actions to do on chip when terminating
Parameters  : HAL Display manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void Term(const HALDISP_Handle_t DisplayHandle)
{
    /* memory deallocation if necessary */
    UNUSED_PARAMETER(DisplayHandle);
}

/*******************************************************************************
Name        : HALDISP_SetDisplayLumaFrameBuffer
Description : Set display luma frame buffer
Parameters  : Display registers address, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationLumaFrameBuffer(
                         const HALDISP_Handle_t    DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    UNUSED_PARAMETER(LayerType);

    /* ensure that it is 512 bytes aligned */
    #ifdef DEBUG
    if (((U32) BufferAddress_p) & ~SDINn_PRESENTATION_MASK)
    {
        HALDISP_Error();
    }
    #endif

    /* main layer */
    HAL_Write32((U8 *)(U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFP_32, (U32) BufferAddress_p);
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
                         const HALDISP_Handle_t    DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    UNUSED_PARAMETER(LayerType);

    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~SDINn_PRESENTATION_MASK)
    {
        HALDISP_Error();
    }
    #endif

    /* main layer */
    /* Beginning of the chroma frame buffer to display, in unit of 512 bytes */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PCHP_32, (U32) BufferAddress_p);
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
                         const HALDISP_Handle_t  DisplayHandle,
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

#ifdef GNBvd06059
    HeightInMacroBlock *= 2;
#endif

#ifdef GNBvd06183
    if (((HALDISP_Properties_t *)DisplayHandle)->DecoderNumber == 6)
    {
        U8 * SDIN1_RegistersBaseAddress_p;  /* Base address of the Display registers */
        SDIN1_RegistersBaseAddress_p = (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p;
        SDIN1_RegistersBaseAddress_p -= 0x00000100;
        /* Write width in macroblocks in presentation width buffer */
        HAL_Write32(SDIN1_RegistersBaseAddress_p + SDINn_PFW, WidthInMacroBlock);
    }
#endif

    /* Write width in macroblocks in presentation width buffer */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFW, WidthInMacroBlock);

#ifdef WA_GNBvd12756
    /* Write Height in macroblocks in presentation Height buffer */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFH, HeightInMacroBlock + 4);
#else
    /* Write Height in macroblocks in presentation Height buffer */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFH, HeightInMacroBlock);
#endif

}

/*******************************************************************************
Name        : SelectPresentationLumaTopField
Description : Presents the top field of the luma part of the frame to both main and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationLumaTopField(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Main Display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_BTY_MASK,
            SDINn_BTY_EMP,
            HALDISP_DISABLE);
}


/*******************************************************************************
Name        : SelectPresentationLumaBottomField
Description : Presents the bottom field of the luma part of the frame to both main and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationLumaBottomField(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_BTY_MASK,
            SDINn_BTY_EMP,
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
static void SelectPresentationChromaTopField(const HALDISP_Handle_t  DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_BTC_MASK,
            SDINn_BTC_EMP,
            HALDISP_DISABLE);
}


/*******************************************************************************
Name        : SelectPresentationChromaBottomField
Description : Presents the bottom field of the chroma part of the frame to both main and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationChromaBottomField(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    /* Main Display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_BTC_MASK,
            SDINn_BTC_EMP,
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
static void EnablePresentationLumaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(LayerType);

    /* main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_TYEN_MASK,
            SDINn_TYEN_EMP,
            HALDISP_ENABLE);
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
static void DisablePresentationLumaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(LayerType);

    /* Main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_TYEN_MASK,
            SDINn_TYEN_EMP,
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
static void EnablePresentationChromaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(LayerType);

    /* Main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_TCEN_MASK,
            SDINn_TCEN_EMP,
            HALDISP_ENABLE);
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
static void DisablePresentationChromaFieldToggle(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType)
{
    UNUSED_PARAMETER(LayerType);

    /* Main display */
    HAL_SetRegister32Value(
            (U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PFLD,
            SDINn_PFLD_MASK,
            SDINn_TCEN_MASK,
            SDINn_TCEN_EMP,
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
                         const HALDISP_Handle_t              DisplayHandle,
                         const STLAYER_Layer_t               LayerType,
                         const VIDDISP_ShowParams_t * const  ShowParams_p)
{
    U32 HDecimationToReg;
    UNUSED_PARAMETER(LayerType);

    HDecimationToReg = (U32)RegValueFromEnum(haldisp_RegValueHDecimation, ShowParams_p->HDecimation, HALDISP_HDECIMATION_MAX_REGVALUE);

    /* main display */
    HAL_SetRegister32Value((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + SDINn_PMOD,
                 SDINn_PMOD_MASK,
                 SDINn_MOD_HDEC_MASK,
                 SDINn_MOD_HDEC_EMP,
                 HDecimationToReg);
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


/* End of hv_di_sd.c */
