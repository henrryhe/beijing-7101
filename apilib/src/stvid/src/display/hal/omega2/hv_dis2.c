/*******************************************************************************

File name   : hv_dis2.c

Description : HAL video display omega 2 family source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
23 Feb 2000        Created                                          PLe
25 Jan 2001        Changed CPU memory access to device access        HG
25 Jan 2001        Changed CPU memory access to device access        HG
01 Feb 2001        Changed SetPresentationStoredPictureProperties
                   proto to be Digital Input compatible              JA
15 May 2001        First debug with 7015 cut 1.1                     HG
12 Mar 2002        Separate Layer Main from Layer Aux                AN
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

#define WA_GNBvd12756   /* "Auxilliary Display shows white or no chroma top field"                      */

#define WA_SECONDARY_FRAME_BUFFER                   /* Work around to manage the bit SFB in xPMOD reg.  */
                                                    /* as followong :                                   */
                                                    /*  - 1 if progressive input AND decimation used.   */
                                                    /*  - 0 otherwise.                                  */
#define WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD /* Work around to manage progressive HD stream       */
                                                   /* like interlaced stream (du to Hardware limits.).  */
#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
#define FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED 1080
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */
/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#include "stddefs.h"

#include "halv_dis.h"
#include "hv_dis2.h"
#include "hvr_dis2.h"

#include "stsys.h"
#include "stgxobj.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#define HALDISP_ENABLE                                    1
#define HALDISP_DISABLE                                   0
#define HALDISP_ERROR                                   0xFF

/* Private Macros ----------------------------------------------------------- */

#define HAL_Read8(Address_p)     STSYS_ReadRegDev8((void *) (Address_p))
#define HAL_Read16(Address_p)    STSYS_ReadRegDev16LE((void *) (Address_p))
#define HAL_Read32(Address_p)    STSYS_ReadRegDev32LE((void *) (Address_p))

/* Warning : For compliancy constraint, only 32BITS write are allowed         */
/* (see 5517 + STEM 7020 for more details).                                   */
#define HAL_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))


#define HAL_GetRegister16Value(Address_p, Mask, Emp)  (((U16)(HAL_Read16(Address_p))&((U16)(Mask)<<(Emp)))>>(Emp))
#define HAL_GetRegister8Value(Address_p, Mask, Emp)   (((U8)(HAL_Read8(Address_p))&((U8)(Mask)<<(Emp)))>>(Emp))


/* Private Variables (static)------------------------------------------------ */

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
static ST_ErrorCode_t Init(const HALDISP_Handle_t DisplayHandle);
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
 const HALDISP_FunctionsTable_t  HALDISP_Omega2FunctionsMPEG =
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
    U32 Pfld;

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Reset TCEN bit */
    Pfld &= ~(VIDn_TCEN_MASK << VIDn_TCEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Reset TCEN bit */
    Pfld &= ~(VIDn_TCEN_MASK << VIDn_TCEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of DisablePresentationChromaFieldToggle() function */


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
    U32 Pfld;

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Reset TYEN bit */
    Pfld &= ~(VIDn_TYEN_MASK << VIDn_TYEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Reset TYEN bit */
    Pfld &= ~(VIDn_TYEN_MASK << VIDn_TYEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of DisablePresentationLumaFieldToggle() function */


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
    U32 Pfld;

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Set TCEN bit */
    Pfld |= (VIDn_TCEN_MASK << VIDn_TCEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Set TCEN bit */
    Pfld |= (VIDn_TCEN_MASK << VIDn_TCEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of EnablePresentationChromaFieldToggle() function */


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
    U32 Pfld;

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Set TYEN bit */
    Pfld |= (VIDn_TYEN_MASK << VIDn_TYEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Set TYEN bit */
    Pfld |= (VIDn_TYEN_MASK << VIDn_TYEN_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of EnablePresentationLumaFieldToggle() function */


/*******************************************************************************
Name        : Init
Description : HAL Display manager handle
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t Init(const HALDISP_Handle_t DisplayHandle)
{
    UNUSED_PARAMETER(DisplayHandle);
   /* memory allocation for private flags structure if necessary */

   return(ST_NO_ERROR);
} /* End of Init() function */


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
    U32 Pfld;
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Set BTC bit */
    Pfld |= (VIDn_BTC_MASK << VIDn_BTC_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Set BTC bit */
    Pfld |= (VIDn_BTC_MASK << VIDn_BTC_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of SelectPresentationChromaBottomField() function */


/*******************************************************************************
Name        : SelectPresentationChromaTopField
Description : Presents the top field of the chroma part of the frame to both main and aux display
Parameters  : HAL display manager handle
Assumptions : If toggle is enabled then the selected field will automatically toggle on each VSync.
Limitations :
Returns     : None
*******************************************************************************/
static void SelectPresentationChromaTopField(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const PictureBuffer_p)
{
    U32 Pfld;
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Reset BTC bit */
    Pfld &= ~(VIDn_BTC_MASK << VIDn_BTC_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Reset BTC bit */
    Pfld &= ~(VIDn_BTC_MASK << VIDn_BTC_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of SelectPresentationChromaTopField() function */


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
    U32 Pfld;
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Set BTY bit */
    Pfld |= (VIDn_BTY_MASK << VIDn_BTY_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Set BTY bit */
    Pfld |= (VIDn_BTY_MASK << VIDn_BTY_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of SelectPresentationLumaBottomField() function */


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
    U32 Pfld;
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(PictureBuffer_p);

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
    /* Main display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD);

    /* Reset BTY bit */
    Pfld &= ~(VIDn_BTY_MASK << VIDn_BTY_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFLD, Pfld);

    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
    /* Aux display */
    /* Get the Main presentation field selection register */
    Pfld = HAL_Read32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD);

    /* Reset BTY bit */
    Pfld &= ~(VIDn_BTY_MASK << VIDn_BTY_EMP);

    /* Save its new value. */
    HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFLD, Pfld);
    }
} /* End of SelectPresentationLumaTopField() function */


/*******************************************************************************
Name        : HALDISP_SetDisplayChromaFrameBuffer
Description : Set display chroma frame buffer
Parameters  : Decoder registers address, buffer address
Assumptions : 7015: Address_p and size must be aligned on 512 bytes. If not, it is done as if they were
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SetPresentationChromaFrameBuffer(
                         const HALDISP_Handle_t    DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         void * const              BufferAddress_p)
{
    #ifdef DEBUG
    /* ensure that it is 512 bytes aligned */
    if (((U32) BufferAddress_p) & ~VIDn_PRESENTATION_MASK)
    {
        HALDISP_Error();
    }
    #endif

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
        /* main layer */
        /* Beginning of the chroma frame buffer to display, in unit of 512 bytes */
        HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PCHP_32, (U32) BufferAddress_p);
    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
        /* aux layer */
        /* Beginning of the chroma frame buffer to display, in unit of 512 bytes */
        HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APCHP_32, (U32) BufferAddress_p);
    }
} /* End of SetPresentationChromaFrameBuffer() function */


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

    /* Write width in macroblocks in presentation width buffer */
    HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFW, WidthInMacroBlock);

    /* Write Height in macroblocks in presentation Height buffer */
#if defined WA_GNBvd12756
    HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFH, HeightInMacroBlock + 4);
#else
    HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFH, HeightInMacroBlock);
#endif
} /* End of SetPresentationFrameDimensions() function */


/*******************************************************************************
Name        : SetPresentationLumaFrameBuffer
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
    /* ensure that it is 512 bytes aligned */
    #ifdef DEBUG
    if (((U32) BufferAddress_p) & ~VIDn_PRESENTATION_MASK)
    {
        HALDISP_Error();
    }
    #endif

    if ( (LayerType == STLAYER_OMEGA2_VIDEO1) || (LayerType == STLAYER_7020_VIDEO1) )
    {
        /* main layer */
        HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_PFP_32, (U32) BufferAddress_p);
    }

    if ( (LayerType == STLAYER_OMEGA2_VIDEO2) || (LayerType == STLAYER_7020_VIDEO2) )
    {
        /* aux layer */
        HAL_Write32((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p + VIDn_APFP_32, (U32) BufferAddress_p);
    }
} /* End of SetPresentationLumaFrameBuffer() function */


/*******************************************************************************
Name        : SetPresentationStoredPictureProperties
Description : Shows how the presented picture has to be reconstructed.
Parameters  : HAL display manager handle
              Parameters to display (scantype, decimation, ...).
Assumptions : If any decimation factor is significative, the reconstruction
              will be performed from secondary frame buffers.
              ShowParams_p is not NULL
Limitations : Decimation modes and compression modes are mutually exclusive.
Returns     :
*******************************************************************************/
static void SetPresentationStoredPictureProperties(
                         const HALDISP_Handle_t             DisplayHandle,
                         const STLAYER_Layer_t              LayerType,
                         const VIDDISP_ShowParams_t * const ShowParams_p)
{
    U16 HDecimationToReg, VDecimationToReg;

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!     Warning    !!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    /* As all fields of these registers will be set, the default value is zero, */
    /*  then all bits are set independently.                                    */
    U32 Pmod  = 0x0000;
    U32 APmod = 0x0000;

#ifdef DEBUG
    /* Test error cases for input parameters. */
    if ( ((ShowParams_p->HDecimation != STVID_DECIMATION_FACTOR_NONE)
       || (ShowParams_p->VDecimation != STVID_DECIMATION_FACTOR_NONE))
       && (ShowParams_p->Compression != STVID_COMPRESSION_LEVEL_NONE) )
    {
        HALDISP_Error();
    }
#endif

    /* Convert each input parameter into a compatible register value. */
    switch (ShowParams_p->HDecimation)
    {
        case STVID_DECIMATION_FACTOR_NONE :
            HDecimationToReg = VIDn_MOD_DEC_NONE;
            break;

        case STVID_DECIMATION_FACTOR_H2 :
            HDecimationToReg = VIDn_MOD_DEC_FACTOR2;
            break;

        case STVID_DECIMATION_FACTOR_H4 :
            HDecimationToReg = VIDn_MOD_DEC_FACTOR4;
            break;

        default :
            /* Should never happen */
            HDecimationToReg = VIDn_MOD_DEC_NONE;
#ifdef DEBUG
                HALDISP_Error();
#endif
            break;
    }

    switch (ShowParams_p->VDecimation)
    {
        case STVID_DECIMATION_FACTOR_NONE :
            VDecimationToReg = VIDn_MOD_DEC_NONE;
            break;

        case STVID_DECIMATION_FACTOR_V2 :
            VDecimationToReg = VIDn_MOD_DEC_FACTOR2;
            break;

        case STVID_DECIMATION_FACTOR_V4 :
            VDecimationToReg = VIDn_MOD_DEC_FACTOR4;
            break;

        default :
            /* Should never happen */
            VDecimationToReg = VIDn_MOD_DEC_NONE;
#ifdef DEBUG
                HALDISP_Error();
#endif
            break;
    }

    if (((HALDISP_Properties_t *)DisplayHandle)->DeviceType
            == STVID_DEVICE_TYPE_7015_MPEG)
    {
        /* Bits COMPSFB of PMOD register) only exist on STi7015.        */

        /* Test and set the compression level applied to frame buffers. */
        switch (ShowParams_p->Compression)
        {
            case STVID_COMPRESSION_LEVEL_NONE :
                Pmod |= VIDn_MOD_COMP_NONE;
                break;

            case STVID_COMPRESSION_LEVEL_1 :
                Pmod |= VIDn_MOD_COMP_LEVEL1;
                break;

            case STVID_COMPRESSION_LEVEL_2 :
                Pmod |= VIDn_MOD_COMP_LEVEL2;
                break;

            default :
                /* Should never happen */
                Pmod |= VIDn_MOD_COMP_NONE;
#ifdef DEBUG
                    HALDISP_Error();
#endif
                break;
        }
    }

    /* Test if HD-PIP is required.      */
    if (ShowParams_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB_HDPIP)
    {
        Pmod  |= (VIDn_MOD_HPD_MASK << VIDn_MOD_HPD_EMP);
        APmod |= (VIDn_MOD_HPD_MASK << VIDn_MOD_HPD_EMP);
    }

    /* *****************    Main reconstruction management   ***************** */
    if ( (LayerType == STLAYER_OMEGA2_VIDEO1)
      || (LayerType == STLAYER_7020_VIDEO1) )
    {
        /* Set the new decimation value */
        Pmod  |= ((HDecimationToReg << 2) | VDecimationToReg) << VIDn_MOD_DEC_EMP;

#ifndef WA_SECONDARY_FRAME_BUFFER
        if (((HALDISP_Properties_t *)DisplayHandle)->DeviceType
                == STVID_DEVICE_TYPE_7015_MPEG)
        {
            /* Bits SFB of PMOD/APMOD registers) only exist on STi7015. */

            /* Test if any decimation is to be set */
            if ( (ShowParams_p->HDecimation != STVID_DECIMATION_FACTOR_NONE)
              || (ShowParams_p->VDecimation != STVID_DECIMATION_FACTOR_NONE) )
            {
                /* Then, enable the secondary frame buffers. */
                Pmod  |= (VIDn_MOD_SFB_MASK << VIDn_MOD_SFB_EMP);
            }
        }
#endif /* WA_SECONDARY_FRAME_BUFFER */

        /* Set the scan type of main display for the luma and chroma */
        if (ShowParams_p->ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        {
#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
            if (ShowParams_p->Height < FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED)
            {
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */
                /* Height small enough to the the picture to be considered as progressive.  */
                Pmod  |= ( (VIDn_MOD_PDL_MASK << VIDn_MOD_PDL_EMP ) | (VIDn_MOD_PDC_MASK << VIDn_MOD_PDC_EMP) );
#ifdef WA_SECONDARY_FRAME_BUFFER
                if (((HALDISP_Properties_t *)DisplayHandle)->DeviceType == STVID_DEVICE_TYPE_7015_MPEG)
                {
                    /* Bits SFB of PMOD/APMOD registers) only exist on STi7015. */

                    /* Test if any decimation is to be set */
                    if ( (ShowParams_p->HDecimation != STVID_DECIMATION_FACTOR_NONE)
                      || (ShowParams_p->VDecimation != STVID_DECIMATION_FACTOR_NONE) )
                    {
                        /* Enable the secondary frame buffers.  */
                        Pmod  |= (VIDn_MOD_SFB_MASK << VIDn_MOD_SFB_EMP);
                    }
                }
#endif /* WA_SECONDARY_FRAME_BUFFER */
#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
            }
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */
        }

        /* Set the new presentation memory mode register */
        HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)
                ->RegistersBaseAddress_p + VIDn_PMOD, Pmod );
    } /* End of MAIN */


    /* ***************** Auxiliary reconstruction management ***************** */
    if ( (LayerType == STLAYER_OMEGA2_VIDEO2)
      || (LayerType == STLAYER_7020_VIDEO2) )
    {
        /* Set the new decimation value */
        APmod |= ((HDecimationToReg << 2)
                | VDecimationToReg) << VIDn_MOD_DEC_EMP;

#ifndef WA_SECONDARY_FRAME_BUFFER
        if (((HALDISP_Properties_t *)DisplayHandle)->DeviceType
                == STVID_DEVICE_TYPE_7015_MPEG)
        {
            /* Bits SFB of PMOD/APMOD registers) only exist on STi7015. */
            /* Test if any decimation is to be set */
            if ( (ShowParams_p->HDecimation != STVID_DECIMATION_FACTOR_NONE)
              || (ShowParams_p->VDecimation != STVID_DECIMATION_FACTOR_NONE) )
            {
                /* Then, enable the secondary frame buffers. */
                APmod |= (VIDn_MOD_SFB_MASK << VIDn_MOD_SFB_EMP);
            }
        }
#endif /* WA_SECONDARY_FRAME_BUFFER */

        /* Set the scan type of main display for the luma and chroma */
        if (ShowParams_p->ScanType == STGXOBJ_PROGRESSIVE_SCAN)
        {
#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
            if (ShowParams_p->Height < FIRST_PROGRESSIVE_HEIGHT_TO_MANAGE_AS_INTERLACED)
            {
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */
                /* Height small enough to the the picture to be considered as progressive.  */
                APmod |= ( (VIDn_MOD_PDL_MASK << VIDn_MOD_PDL_EMP )
                      | (VIDn_MOD_PDC_MASK << VIDn_MOD_PDC_EMP) );
#ifdef WA_SECONDARY_FRAME_BUFFER
                if (((HALDISP_Properties_t *)DisplayHandle)->DeviceType
                        == STVID_DEVICE_TYPE_7015_MPEG)
                {
                    /* Bits SFB of PMOD/APMOD registers) only exist on STi7015. */

                    /* Test if any decimation is to be set */
                    if ( (ShowParams_p->HDecimation != STVID_DECIMATION_FACTOR_NONE)
                      || (ShowParams_p->VDecimation != STVID_DECIMATION_FACTOR_NONE) )
                    {
                        /* Enable the secondary frame buffers.  */
                        APmod |= (VIDn_MOD_SFB_MASK << VIDn_MOD_SFB_EMP);
                    }
                }
#endif /* WA_SECONDARY_FRAME_BUFFER */
#ifdef WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD
            }
#endif /* WA_PROGRESSIVE_HD_MANAGED_AS_INTERLACED_HD */
        }
        /* Set the new presentation memory mode register */
        HAL_Write32 ((U8 *)((HALDISP_Properties_t *)DisplayHandle)->RegistersBaseAddress_p
                + VIDn_APMOD, APmod );
    } /* End of AUX */

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
    UNUSED_PARAMETER(HALDisplayHandle);
    UNUSED_PARAMETER(LayerType);
    UNUSED_PARAMETER(LayerIdent);
    UNUSED_PARAMETER(FieldsForNextVSync);
    
    /* Not used on Omega2 */
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
    UNUSED_PARAMETER(DisplayHandle);
    /* memory deallocation if necessary */
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


/* End of hv_dis2.c */

