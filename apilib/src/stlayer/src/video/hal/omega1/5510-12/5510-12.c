/*******************************************************************************
File name   : 5510-12.c
Description : Video HAL (Hardware Abstraction Layer) access to hardware source file
COPYRIGHT (C) STMicroelectronics 2004
Date               Modification                                     Name
----               ------------                                     ----
10Aug 2000         Created                                          AN
23 Aug 2002        New UpdateScreenParams function.                 GG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include "stddefs.h"
#include "stgxobj.h"
#include "stlayer.h"
#include "sttbx.h"
#include "stsys.h"
#include "halv_lay.h"
#include "5510-12.h"
#include "5512/5512_reg.h"
#include "hv_layer.h"
#include "hv_reg.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
/* Smooth transitions in the vertical scaling */
#define V_TRIGGER1A ((MIN_V_SCALING    +  V_SCALING_OUT_X3) / 2)
#define V_TRIGGER2A ((V_SCALING_OUT_X3 +  V_SCALING_OUT_X2) / 2)
#define V_TRIGGER3A ((V_SCALING_OUT_X2 +  V_SCALING_OUT_A ) / 2)
#define V_TRIGGER4A ((V_SCALING_OUT_A  +  V_SCALING_OUT_B ) / 2)
#define V_TRIGGER5A ((V_SCALING_OUT_B  +  V_NO_SCALING    ) / 2)
#define V_TRIGGER6A ((V_NO_SCALING     +  MAX_V_SCALING   ) / 2)

#define MIN_V_SCALING         ((1 * INT_AV_SCALE_CONSTANT) / 4)         /* 1 / x4 */
#define V_SCALING_OUT_X3      ((3 * INT_AV_SCALE_CONSTANT) / 8)         /* 1 / x3 */
#define V_SCALING_OUT_X2      ((1 * INT_AV_SCALE_CONSTANT) / 2)         /* 1 / x2 */
#define V_SCALING_OUT_A       ((3 * INT_AV_SCALE_CONSTANT) / 4) 		/* x0.75 16:9 LB */
#define V_SCALING_OUT_B       ((14 * INT_AV_SCALE_CONSTANT) / 16)       /* x0.875 14:9 Combined */
#define V_NO_SCALING          ((1 * INT_AV_SCALE_CONSTANT) / 1)         /* x1     */
#define V_SCALING_IN_A        ((2 * INT_AV_SCALE_CONSTANT) / 1)         /* x2     */
#define MAX_V_SCALING         ((2 * INT_AV_SCALE_CONSTANT) / 1)         /* x2     */

#define LIM_V_SCALING       ((3 * INT_AV_SCALE_CONSTANT) / 8)  /* Vx0.375*/
#define MIN_H_SCALING       ((1 * INT_AV_SCALE_CONSTANT) / 4)  /* Hx0.25 */
#define MAX_H_SCALING       ((8 * INT_AV_SCALE_CONSTANT) / 1)  /* Hx8    */

/* Correction computed on 5516 / PAL */
#define VIDEO_PAL_HORIZONTAL_START  (-8)
#define VIDEO_PAL_VERTICAL_START    (-2)
#define VIDEO_NTSC_HORIZONTAL_START (-12)
#define VIDEO_NTSC_VERTICAL_START   (-2)

/* Private Types ------------------------------------------------------------ */
/* Private Macros ----------------------------------------------------------- */
/* Private Variables (static)------------------------------------------------ */
/* Functions ---------------------------------------------------------------- */
void InitialiseLayer(   const STLAYER_Handle_t              LayerHandle,
                        const STLAYER_Layer_t               LayerType);
void UpdateScreenParams(
                        const STLAYER_Handle_t              LayerHandle,
                        const STLAYER_OutputParams_t *      OutputParams_p);
void SetCIFMode(
                        const STLAYER_Handle_t              LayerHandle,
                        const BOOL                          LumaFrameBased,
                        const BOOL                          ChromaFrameBased,
                        const BOOL                          LumaAndChromaThrowOneLineInTwo);
void ApplyVerticalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        BOOL                                Progressive,
                        U32                                 Zx,
                        U32                                 Zy);
void ApplyHorizontalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        U32                                 Zx,
                        U32                                 Zy);
void AdjustHorizontalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        STLAYER_ViewPortParams_t * const    Params_p,
                        U32*                                Zx_p);
void AdjustVerticalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        STLAYER_ViewPortParams_t * const    Params_p,
                        U32*                                Zy_p);
void SetViewPortPositions(
                        const STLAYER_ViewPortHandle_t      ViewportHandle,
                        STGXOBJ_Rectangle_t * const         OutputRectangle_p,
                        const BOOL                          Apply);
extern void SetInputMargins(
                        const  STLAYER_ViewPortHandle_t     ViewportHandle,
                        S32 * const                         LeftMargin_p,
                        S32 * const                         TopMargin_p,
                        const BOOL                          Apply);


/* Global Variables --------------------------------------------------------- */

/*******************************************************************************
 * Name        : InitialiseLayer
 * Description : Function called at init to initialise HW
 * Parameters  : LayerHandle and layer type
 * Assumptions :
 * Limitations :
 * Returns     : Nothing
 * *******************************************************************************/
void InitialiseLayer(const STLAYER_Handle_t             LayerHandle,
                     const STLAYER_Layer_t              LayerType)
{
    /* Nothing to do */
} /* end of InitialiseLayer() function */


/*******************************************************************************
 * Name        : UpdateScreenParams
 * Description : Function called by upper level to inform vtg changes.
 * Parameters  : LayerHandle and ouput parameters pointer.
 * Assumptions : .
 * Limitations : .
 * Returns     : RAS.
 * *******************************************************************************/
void UpdateScreenParams(const STLAYER_Handle_t LayerHandle, const STLAYER_OutputParams_t * OutputParams_p)
{
    /* Nothing to do for those chips. */

} /* End of UpdateScreenParams() function. */

/*******************************************************************************
 * Name        : SetCIFMode
 * Description :
 * Parameters  :
 * Assumptions :
 * Limitations : info LumaAndChromaThrowOneLineInTwo not used on STi5510/12
 * Returns     :
 * *******************************************************************************/
void SetCIFMode(const STLAYER_Handle_t LayerHandle,
                        const BOOL                          LumaFrameBased,
                        const BOOL                          ChromaFrameBased,
                        const BOOL                          LumaAndChromaThrowOneLineInTwo)
{
    U16 DFSValue;
    /*  We have to deal with CIF mode wich is into VID_DFS register and we dont
     *  want to change the other bits */

    DFSValue = (U32)(HAL_Read8(
            ((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DFS_16)) & VID_DFS_MASK1)<<8;
    DFSValue |= (U32)(HAL_Read8(
            ((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DFS_16)) & VID_DFS_MASK2);

    if (LumaFrameBased) /* Could verify one, or both... But luma is the most important ? */
    {
        DFSValue |= VID_DFS_CIF;
    }
    else
    {
        DFSValue &= ~VID_DFS_CIF;
    }
    HAL_Write8(
            (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DFS_16 ,
            (DFSValue >> 8 ) & VID_DFS_MASK1);
    HAL_Write8(
            (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DFS_16 ,
            (DFSValue & 0xFF ) & VID_DFS_MASK2);
    #ifdef GNBvd00462
        #if 0
    /*    Since VID_DFS register is latched on DSYNC, and we want it to be latched on VSYNC
          Write into test reg in order to latch it. This operation is synchronized on VSYNC
          WARNING : This will latch ALL registers    */
        SyncVideoWrite((U32) VideoBaseAddress_p + VID_TST, 1);
        SyncVideoWrite((U32) VideoBaseAddress_p + VID_TST, 0);
        #endif /* 0 */
    #endif
}


/*******************************************************************************
 * Name        : ApplyVerticalFactor
 * Description :
 * Parameters  :
 * Assumptions :
 * Limitations :
 * Returns     : Nothing
 * *******************************************************************************/
void ApplyVerticalFactor(const STLAYER_Handle_t LayerHandle, BOOL Progressive,
                            U32 Zx, U32 Zy)
{
#define VID_DCF_BLL     1
#define VID_DCF_BFL     2
#define VID_DCF_STP_DEC 4
U16     DCFValue;
U32     VFL_Mode, VFC_Mode;
BOOL    CIFMode;
BOOL    LumaFrameBased, ChromaFrameBased;
BOOL    B2RMode4;
BOOL    B2RMode5;
BOOL    B2RMode6;
BOOL    B2RMode89;
HALLAYER_PrivateData_t* LocalPrivateData_p;

  /* Standard state : */
  interrupt_lock();       /* due to OSD/VID concurrent access to VID_DCF16 */

  HAL_SetRegister16Value((U8 *)(
            (HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DCF_16 ,
            VID_DCF_MASK,
            VID_DCF_BLL_MASK,
            VID_DCF_BLL_EMP,
            HAL_DISABLE);

  HAL_SetRegister16Value((U8 *)(
            (HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DCF_16 ,
            VID_DCF_MASK,
            VID_DCF_BFL_MASK,
            VID_DCF_BFL_EMP,
            HAL_DISABLE);

  #ifdef HW_5512
  HAL_SetRegister16Value((U8 *)(
            (HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DCF_16 ,
            VID_DCF_MASK,
            VID_DCF_STP_DEC_MASK,
            VID_DCF_STP_DEC_EMP,
            HAL_DISABLE);
  #endif /* HW_5512 */
   interrupt_unlock();

  /* Initialization of the variables (default value).                        */
  #ifdef GNBvd01320
  B2RMode4 = FALSE;
  B2RMode5 = FALSE;
  B2RMode6 = FALSE;
  B2RMode89 = FALSE;
  #endif /* GNBvd01320 */

  DCFValue = 0;

  /* Used to zoom in x2 with a CIF format (half) */
  CIFMode = FALSE;

  VFL_Mode = 7;
  VFC_Mode = 14;

  switch(Zy)
  {
    case MIN_V_SCALING :                   /* zoom out x4 */
      if (Zx < H_SCALING_OUT_X2)
      {
        VFL_Mode = 31;
        VFC_Mode = 28;
      }
      else
      {
        VFL_Mode = 0;
        VFC_Mode = 3;
      }
      #ifdef HW_5512
      DCFValue |= VID_DCF_STP_DEC;
      #endif /* HW_5512 */
      break;

    case V_SCALING_OUT_X3 :                /* zoom out x3 */
      #if defined(HW_5510)
      if (Zx < H_SCALING_OUT_X2)
      {
        VFL_Mode = 29;
        VFC_Mode = 4;
      }
      else
      {
        VFL_Mode = 2;
        VFC_Mode = 4;
      }
      #ifdef GNBvd01320
      B2RMode4 = TRUE;
      #endif /* GNBvd01320 */
      #endif /* defined(HW_5510) */

      #if defined(HW_5512)
      if (Zx < H_SCALING_OUT_X2)
      {
        VFL_Mode = 29;
        VFC_Mode = 27;
      }
      else
      {
        VFL_Mode = 2;
        VFC_Mode = 4;
      }
      #endif /* defined(HW_5512) */
      break;

    case V_SCALING_OUT_X2 :                /* zoom out x2 */
      VFL_Mode = 3;
      VFC_Mode = 7;
      break;

    case V_SCALING_OUT_A:               /* 16:9 */
      VFL_Mode = 5;
      VFC_Mode = 10;
      DCFValue |= (VID_DCF_BLL | VID_DCF_BFL);
      #ifdef GNBvd01320
      B2RMode5 = TRUE;
      #endif /* GNBvd01320 */
      break;

    case V_SCALING_OUT_B:               /* 14:9 */
      VFL_Mode = 6;
      VFC_Mode = 11;
      DCFValue |= (VID_DCF_BLL | VID_DCF_BFL);
      #ifdef GNBvd01320
      B2RMode6 = TRUE;
      #endif /* GNBvd01320 */
      break;

    case V_NO_SCALING :                 /* full screen */
      VFL_Mode = 7;
      VFC_Mode = 14;
      break;

    /*case V_SCALING_IN_A :*/
    case MAX_V_SCALING :                /* zoom in x2 */
      if (Progressive)
      {
        VFL_Mode = 8;
        VFC_Mode = 14;                  /* 15 ? */
        CIFMode = TRUE;
        #ifdef GNBvd01320
        B2RMode89 = TRUE;
        #endif /* GNBvd01320 */
      }
      else
      {
        VFL_Mode = 14;
        VFC_Mode = 19;
        CIFMode = FALSE;
      }
      break;

    default :
    /* TODO */
    break;
  }

  /* Update the PrivateData structure */
  LocalPrivateData_p = ((HALLAYER_LayerProperties_t *)(LayerHandle))->PrivateData_p ;

  LocalPrivateData_p->Scaling.B2RMode4 = B2RMode4;
  LocalPrivateData_p->Scaling.B2RMode5 = B2RMode5;
  LocalPrivateData_p->Scaling.B2RMode6 = B2RMode6;
  LocalPrivateData_p->Scaling.B2RMode89 = B2RMode89;

  interrupt_lock();       /* due to OSD/VID concurrent access to VID_DCF16 */

  if (DCFValue & VID_DCF_BLL)
  {
    HAL_SetRegister16Value((U8 *)(
            (HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DCF_16 ,
            VID_DCF_MASK,
            VID_DCF_BLL_MASK,
            VID_DCF_BLL_EMP,
            HAL_ENABLE);
  }
  if (DCFValue & VID_DCF_BFL)
  {
    HAL_SetRegister16Value((U8 *)(
            (HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DCF_16 ,
            VID_DCF_MASK,
            VID_DCF_BFL_MASK,
            VID_DCF_BFL_EMP,
            HAL_ENABLE);
  }
  #ifdef HW_5512
  if (DCFValue & VID_DCF_STP_DEC)
  {
    HAL_SetRegister16Value((U8 *)(
            (HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DCF_16 ,
            VID_DCF_MASK,
            VID_DCF_STP_DEC_MASK,
            VID_DCF_STP_DEC_EMP,
            HAL_ENABLE);
  }
  #endif /* HW_5512 */

   interrupt_unlock();

  HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFL_8 ,
            VFL_Mode & VID_VFL_MASK);
  HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFC_8 ,
            VFC_Mode & VID_VFC_MASK);
    if (CIFMode)
    {
        LumaFrameBased = TRUE;
        ChromaFrameBased = TRUE;
    }
    else
    {
        LumaFrameBased = FALSE;
        ChromaFrameBased = FALSE;
    }
    if ((LocalPrivateData_p->Scaling.B2RVFParams.LumaFrameBased != LumaFrameBased) ||
        (LocalPrivateData_p->Scaling.B2RVFParams.ChromaFrameBased != ChromaFrameBased))
    {
        LocalPrivateData_p->Scaling.B2RVFParams.LumaFrameBased = LumaFrameBased;
        LocalPrivateData_p->Scaling.B2RVFParams.ChromaFrameBased = ChromaFrameBased;

        SetCIFMode(LayerHandle, LumaFrameBased, ChromaFrameBased, FALSE);
    }

}


/*******************************************************************************
 * Name        : ApplyHorizontalFactor
 * Description :
 * Parameters  :
 * Assumptions :
 * Limitations :
 * Returns     :
 * *******************************************************************************/
void ApplyHorizontalFactor(
        const STLAYER_Handle_t  LayerHandle,
        U32                     Zx,
        U32                     Zy)
{
U32 AppliedScaling;
U8  VerticalFactor;
U16 LSR_MAX = 511;


  VerticalFactor = 1;
  if (Zy <= LIM_V_SCALING)
  {
    if (Zx < H_SCALING_OUT_X2)
    {
      /* Vertical filter does an horizontal downsampling */
      /* In all the other cases, VerticalFactor egal to 1 */
      VerticalFactor = 2;
    }
  }

  if (Zx != 0)
  {
    AppliedScaling = (256 * INT_AV_SCALE_CONSTANT / Zx);
    if (AppliedScaling > ((U32)VerticalFactor) * LSR_MAX)
    {
            AppliedScaling = ((U32)VerticalFactor) * LSR_MAX;
    }
  }
  else
  {
    AppliedScaling = LSR_MAX;
  }


  if (Zx == INT_AV_SCALE_CONSTANT)
  {
    DisableHorizontalScaling(LayerHandle);
  }
  else
  {
    /* Not the real AppliedScaling but the value to be written in the SRC */
    AppliedScaling = AppliedScaling / VerticalFactor;
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_LSO_8 , 0 );

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_CSO_8 , 0);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_LSR0_8 ,
            (AppliedScaling & 0xFF ) & VID_LSR0_MASK);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_LSR1_8 ,
            ((AppliedScaling & 0x100 )>>8) & VID_LSR1_MASK);
    EnableHorizontalScaling(LayerHandle);
  }
}


/*******************************************************************************
 * Name        : AdjustHorizontalFactor
 * Description :
 * Parameters  :
 * Assumptions :
 * Limitations :
 * Returns     :
 * *******************************************************************************/
void AdjustHorizontalFactor(
        const STLAYER_Handle_t              LayerHandle,
        STLAYER_ViewPortParams_t * const    Params_p,
        U32*                                Zx_p)
{
    /* Horizontal scaling is dependant of the Vertical scaling when X */
    /* from zoom out by 2 to zoom out by 4. In this case, hardware    */
    /* limitations exist : */
    U32 AppliedScaling, Zx, Zy;
    U16 LSR_MAX = 511;
    /* hardware constraint */
    U8  VerticalFactor = 1;
    U32 SourceWidth;
    U32 SourceHeight;
    U32 DestWidth;
    U32 DestHeight;

    SourceWidth     = Params_p->InputRectangle.Width;
    SourceHeight    = Params_p->InputRectangle.Height;
    DestWidth       = Params_p->OutputRectangle.Width;
    DestHeight      = Params_p->OutputRectangle.Height;

    AppliedScaling = 256; /* Affectation by default. */
    if (SourceHeight != 0)
    {
        Zy = (INT_AV_SCALE_CONSTANT * DestHeight) / SourceHeight;
    }
    else
    {
        Zy = INT_AV_SCALE_CONSTANT ;
    }

    if (SourceWidth == DestWidth)
    {
        Zx = INT_AV_SCALE_CONSTANT;
    }
    else
    {
        if (SourceWidth != 0)
        {
            Zx = (INT_AV_SCALE_CONSTANT * DestWidth) / SourceWidth;
        }
        else
        {
            Zx = INT_AV_SCALE_CONSTANT;
        }

        if (Zx > MAX_H_SCALING)
        {
            Zx = MAX_H_SCALING;
        }
        else
        {
            if (Zy <= LIM_V_SCALING)
            {
                if (Zx < MIN_H_SCALING)
                {
                 /* Vertical filter does an horizontal downsampling */
                    Zx = MIN_H_SCALING;
                }
                /*if (Zx < MIN_H_SCALING * 2)*/
                if (Zx < H_SCALING_OUT_X2)
                {
                    /* Vertical filter does an horizontal downsampling */
                    /* In all the other cases, VerticalFactor egal to 1 */
                    VerticalFactor = 2;
                }
            }
            else
            {
                if (Zx < MIN_H_SCALING * 2)
                {
                    Zx = MIN_H_SCALING * 2 ;
                }
            }
        }
        /* Return the effective scaling that will be applied by the device */
        AppliedScaling = (256 * INT_AV_SCALE_CONSTANT / Zx);
        if (AppliedScaling > (U32)(VerticalFactor * LSR_MAX))
        {
            AppliedScaling = VerticalFactor * LSR_MAX;
        }
        Zx = (256 * INT_AV_SCALE_CONSTANT) / AppliedScaling;
   }

   *Zx_p = Zx;
}


/*******************************************************************************
 * Name        : AdjustVerticalFactor
 * Description :
 * Parameters  :
 * Assumptions :
 * Limitations :
 * Returns     :
 * *******************************************************************************/
void AdjustVerticalFactor(  const STLAYER_Handle_t              LayerHandle,
                            STLAYER_ViewPortParams_t * const    Params_p,
                            U32*                                Zy_p)
{
  U32 Zx, Zy;
  U32 SourceWidth;
  U32 SourceHeight;
  U32 DestWidth;
  U32 DestHeight;
  BOOL Progressive;
  STGXOBJ_ScanType_t  ScanType;


  /* Retrieve some parameters of the source which are useful here.           */
  SourceWidth   = Params_p->InputRectangle.Width;
  SourceHeight  = Params_p->InputRectangle.Height;
  DestWidth     = Params_p->OutputRectangle.Width;
  DestHeight    = Params_p->OutputRectangle.Height;
  ScanType    = ((Params_p->Source_p)->Data.VideoStream_p)->ScanType;

  if (ScanType == STGXOBJ_PROGRESSIVE_SCAN)
  {
    Progressive = TRUE;
  }
  else
  {
    Progressive = FALSE;
  }

  if (SourceWidth != 0)
  {
    Zx = (INT_AV_SCALE_CONSTANT * DestWidth) / SourceWidth;
  }
  else
  {
    Zx = INT_AV_SCALE_CONSTANT ;
  }

  if (SourceHeight != 0)
  {
    Zy = (INT_AV_SCALE_CONSTANT * DestHeight) / SourceHeight;
  }
  else
  {
    Zy = INT_AV_SCALE_CONSTANT ;
  }


  if (Zy >= V_TRIGGER6A)
  {
    Zy = MAX_V_SCALING;
  }
  else
  {
    if ((Zy < V_TRIGGER6A) && (Zy >= V_TRIGGER5A))
    {
      Zy = V_NO_SCALING;
    }
    else
    {
      if ((Zy < V_TRIGGER5A) && (Zy >= V_TRIGGER4A))
      {
        Zy = V_SCALING_OUT_B;
      }
      else
      {
        if ((Zy < V_TRIGGER4A) && (Zy >= V_TRIGGER3A))
        {
          Zy = V_SCALING_OUT_A;
        }
        else
        {
          if ((Zy < V_TRIGGER3A) && (Zy >= V_TRIGGER2A))
          {
            Zy = V_SCALING_OUT_X2;
          }
          else
          {
            if ((Zy < V_TRIGGER2A) && (Zy >= V_TRIGGER1A))
            {
              Zy = V_SCALING_OUT_X3;
            }
            else
            {
              if (Zy < V_TRIGGER1A)
              {
                Zy = MIN_V_SCALING;
              }
            }
          }
        }
      }
    }
  }

  *Zy_p = Zy;
}


/*******************************************************************************
Name        : SetViewPortPositions
Description : Hardware writing of XDO, XDS, YDO, YDS.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void SetViewPortPositions(
            const STLAYER_ViewPortHandle_t      ViewportHandle,
            STGXOBJ_Rectangle_t * const         OutputRectangle_p,
            const BOOL                          Apply)
{
BOOL                        B2RMode4;
BOOL                        B2RMode5;
BOOL                        B2RMode6;
BOOL                        B2RMode89;
U32                         XDOPos, XDSPos, YDOPos, YDSPos;
S32                         XOffset, YOffset;
HALLAYER_PrivateData_t      *LayerPrivateData_p;
STLAYER_Handle_t            LayerHandle;
#if defined(HW_5512)
U8 SaveBits;
U16 ScanValue;
U8 tmp;
#endif /* defined(HW_5512) */


    /* Retrieve the LocalPrivateData structure from the ViewPort Handle.     */
    LayerPrivateData_p = (HALLAYER_PrivateData_t*)ViewportHandle;

    /* Viewport initialisations with the Layer Handle flag.                    */
    LayerHandle = LayerPrivateData_p->AssociatedLayerHandle;

    if ( (OutputRectangle_p->Width  == 0) ||
         (OutputRectangle_p->Height == 0) )
    {
        if (Apply)
        {
            /* Register writing.                                                   */
            HAL_SetRegister16Value((U8 *)
                    ((HALLAYER_LayerProperties_t *)LayerHandle)->VideoDisplayBaseAddress_p + VID_DCF_16 ,
                    VID_DCF_MASK,
                    VID_DCF_EVD_MASK,
                    VID_DCF_EVD_EMP,
                    HAL_DISABLE);
        }
    }
    else
    {
        if (LayerPrivateData_p->DisplayEnableFromVideo)
        {
            if (Apply)
            {
                /* Register writing.                                                   */
                HAL_SetRegister16Value((U8 *)
                        ((HALLAYER_LayerProperties_t *)LayerHandle)->VideoDisplayBaseAddress_p + VID_DCF_16 ,
                        VID_DCF_MASK,
                        VID_DCF_EVD_MASK,
                        VID_DCF_EVD_EMP,
                        HAL_ENABLE);
            }
        }

        /* Update the B2RMode boolean from our structure.                        */
        B2RMode4 = LayerPrivateData_p->Scaling.B2RMode4;
        B2RMode5 = LayerPrivateData_p->Scaling.B2RMode5;
        B2RMode6 = LayerPrivateData_p->Scaling.B2RMode6;
        B2RMode89 = LayerPrivateData_p->Scaling.B2RMode89;

        /* Retrieve the original positions XDO, YDO according to measurments.    */
        /* Usage of the Screen Offsets (dependant of the standard PAL/NTSC...    */
        switch(LayerPrivateData_p->OutputParams.FrameRate)
        {
            case 50000: /* PAL */
            XDOPos = LayerPrivateData_p->OutputParams.XValue + OutputRectangle_p->PositionX + VIDEO_PAL_HORIZONTAL_START;
            YDOPos = (LayerPrivateData_p->OutputParams.YValue + OutputRectangle_p->PositionY + VIDEO_PAL_VERTICAL_START)/ 2;
            break;

            default:
            XDOPos = LayerPrivateData_p->OutputParams.XValue + OutputRectangle_p->PositionX + VIDEO_NTSC_HORIZONTAL_START;
            YDOPos = (LayerPrivateData_p->OutputParams.YValue + OutputRectangle_p->PositionY + VIDEO_NTSC_VERTICAL_START)/ 2;
            break;
        }

        /* Check the hardware limitations.                                       */
        /* Min and Max */
        if (XDOPos < MIN_XDO)
        {
            XDOPos = MIN_XDO;
        }
        if (XDOPos > MAX_XDS)
        {
            XDOPos = MAX_XDS;
        }
        if (YDOPos < MIN_YDO)
        {
            YDOPos = MIN_YDO;
        }
        if (YDOPos > MAX_YDS)
        {
            YDOPos = MAX_YDS;
        }

        /* Even value (same parity with the graphic still plane) */
        XDOPos = (XDOPos & (U32)~1);
        switch(LayerPrivateData_p->OutputParams.FrameRate)
        {
            case 50000: /* PAL */
            if (XDOPos > (LayerPrivateData_p->OutputParams.XValue + VIDEO_PAL_HORIZONTAL_START))
            {
                OutputRectangle_p->PositionX = XDOPos - (VIDEO_PAL_HORIZONTAL_START + LayerPrivateData_p->OutputParams.XValue);
            }
            else
            {
                OutputRectangle_p->PositionX = 0;
            }
            break;
            default:
            if (XDOPos > (LayerPrivateData_p->OutputParams.XValue + VIDEO_NTSC_HORIZONTAL_START))
            {
                OutputRectangle_p->PositionX = XDOPos - (VIDEO_NTSC_HORIZONTAL_START + LayerPrivateData_p->OutputParams.XValue);
            }
            else
            {
                OutputRectangle_p->PositionX = 0;
            }
            break;
        }

        if (Apply)
        {
            /* Write the registers XDO, YDO. */
            HAL_Write16((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDO_16 ,
                XDOPos & VID_XD0_MASK);
            #if defined(HW_5512)
            {
                HAL_Write8((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                    VideoDisplayBaseAddress_p + VID_YDO0_8 ,
                    YDOPos & VID_YD00_MASK);
                SaveBits = HAL_Read8(
                    (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                    VideoDisplayBaseAddress_p + VID_YDO1_8);

                /*Workaround : */
                tmp = ~(SaveBits & 0x30) ;
                ScanValue =  (U16)((SaveBits & 0xCE) | (tmp & 0x30) | ((YDOPos & 0x100)>>8 ) );

                /*ScanValue = (U16)( SaveBits | ( (YDO & 0x100)>>8 ) );*/
                HAL_Write8(
                    (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                    VideoDisplayBaseAddress_p + VID_YDO1_8 ,
                    (U8)ScanValue);
            }
            #else /* HW_5510, ... */
            HAL_Write16((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDO_16 ,
                YDOPos & VID_YD0_MASK);
            #endif /* defined(HW_5512) */

            /* Be careful with YDO on 5512 : it could be a bug here with
                * some bits to NOT save. YDO_MASK should be */
        }

        /* Retrieve the final positions XDO, YDO.                                */
        XDSPos = OutputRectangle_p->Width + XDOPos;
        YDSPos = OutputRectangle_p->Height/2 + YDOPos ;

        /* Hardware limitations.                                                */
        YDSPos -= 1 ;      /* Bad last line to */
        if (XDSPos > MAX_XDS)
        {
            XDSPos = MAX_XDS ;
        }
        if (YDSPos > MAX_YDS)
        {
            YDSPos = MAX_YDS;
        }
        if (XDSPos < XDOPos)
        {
            XDSPos = XDOPos;
        }
        if (YDSPos < YDOPos)
        {
            YDSPos = YDOPos;
        }
        /* Hardware bugs Block2Row GNBvd01320. Workarounds :                     */
        /* forbidden YDS-YDO values in some modes.                               */
        /* mode 4 (4/3):                 yds - ydo = 3x + 1                      */
        /* mode 5 (6/8):                 yds - ydo = 3x + 2                      */
        /* mode 6 (7/8):                 yds - ydo = 7x + 2  or  7x + 6          */
        /* modes 8 & 9 (X2, offset 1/4): yds - ydo = 17x                         */
        #ifdef GNBvd01320
        while (
            ((!( ((YDSPos - YDOPos) - 1) % 3)) && B2RMode4) ||
            ((!( ((YDSPos - YDOPos) - 2) % 3)) && B2RMode5) ||
            ((!( ((YDSPos - YDOPos) - 2) % 7)) && B2RMode6) ||
            ((!( ((YDSPos - YDOPos) - 6) % 7)) && B2RMode6) ||
            ((!(  (YDSPos - YDOPos) % 7)) && B2RMode89) )
        {
            YDSPos--;
        }
        #endif /* GNBvd01320*/

        XDSPos = (XDSPos & (U32)~1);
        if (XDSPos > XDOPos)
        {
            OutputRectangle_p->Width = XDSPos - XDOPos;
        }
        else
        {
            OutputRectangle_p->Width = 0;
        }

        if (Apply)
        {
            /* Write the registers XDS, YDS.                                         */
            HAL_Write16(
                (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDS_16 ,
                XDSPos & VID_XDS_MASK);
            HAL_Write16(
                (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDS_16 ,
                YDSPos & VID_YDS_MASK);

            /* Big risk due to a bug : 0x00xx should be writen here after. */

            /* Call to SetInputMargins. Writes the pan&scan vectors.                 */
            XOffset = LayerPrivateData_p->ViewportParams.InputRectangle.PositionX ;
            YOffset = LayerPrivateData_p->ViewportParams.InputRectangle.PositionY ;
            SetInputMargins(LayerHandle, &XOffset, &YOffset, TRUE);
        }
    }
} /* End of SetViewPortPositions() function */
