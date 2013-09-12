/*******************************************************************************

File name   : 551-4678.c

Description : Video HAL (Hardware Abstraction Layer) access to hardware source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
03 Jan 2001        Created                                          AN
16 May 2002        Added 5516 support                               AN
23 Aug 2002        New UpdateScreenParams function.                 GG
12 Dec 2002        Added 5517/78 support                            AN
14 May 2003        Update B2R filtering to use zoom-out4 feature    HG
                   Merged STi5508/18 file into this one as 99% same HG
17 Nov 2003        Dead code deletion in AdjustHorizontalFactor     CD
                   and AdjustVerticalFactor                         CD
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

#define WA_GNBvd03822       /* Pan not usable with B2R horizontal downsample */

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include "stddefs.h"
#include "stlayer.h"
#include "sttbx.h"
#include "stsys.h"
#include "halv_lay.h"
#include "hv_layer.h"
#include "hv_reg.h"
#include "551-4678.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
/* Change constants here to allow more zooms, but this is not guaranteed then. */
#define MIN_V_SCALING       ((1 * INT_AV_SCALE_CONSTANT) / 4)  /* zoom-out by 4 */
#define MAX_V_SCALING       ((8 * INT_AV_SCALE_CONSTANT) / 1)  /* zoom-in by 8 */
#define LIM_V_SCALING       ((3 * INT_AV_SCALE_CONSTANT) / 8)  /* Vx0.375*/
#define MIN_H_SCALING       ((1 * INT_AV_SCALE_CONSTANT) / 4)  /* zoom-out by 4 */
#define MAX_H_SCALING       ((8 * INT_AV_SCALE_CONSTANT) / 1)  /* zoom-in by 8 */

#define LSR_MAX 511

/* Horizontal and vertical start valures for right alignment between planes */
#if defined(HW_5508) || defined(HW_5518)
/* Caution: values for STi5508/18 to be checked ! */
#define VIDEO_PAL_HORIZONTAL_START  (-21)
#define VIDEO_NTSC_HORIZONTAL_START (-21)
#else /* all other STi55xx: 5514/16/17/78 */
#define VIDEO_PAL_HORIZONTAL_START  (-8)
#define VIDEO_NTSC_HORIZONTAL_START (-12)
#endif /* all other STi55xx: 5514/16/17/78 */

#define VIDEO_PAL_VERTICAL_START    (-2)
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
void SetInputMargins(
                        const STLAYER_Handle_t              LayerHandle,
                        S32*                                const LeftMargin_p,
                        S32*                                const TopMargin_p,
                        const BOOL                          Apply);
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
                        STLAYER_ViewPortParams_t*           const Params_p,
                        U32*                                Zx_p);
void AdjustVerticalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        STLAYER_ViewPortParams_t*           const Params_p,
                        U32*                                Zy_p);
void SetViewPortPositions(
                        const STLAYER_ViewPortHandle_t      ViewportHandle,
                        STGXOBJ_Rectangle_t * const         OutputRectangle_p,
                        const BOOL                          Apply);
void WriteB2RRegisters(const  STLAYER_Handle_t  LayerHandleLayerHandle);

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
    /* Initialisation */
#if defined(HW_5508) || defined(HW_5518)

    /* Nothing to do. Do not set the B2R polarity */

#else /* all other STi55xx: 5514/16/17/78 */

    /* Set B2R polarity to 1 for STi5514/16/88/89/78/17/98/78... */
    HAL_SetRegister16Value((U8 *)
        ((HALLAYER_LayerProperties_t *)LayerHandle)->VideoDisplayBaseAddress_p + VID_DCF_16 ,
        VID_DCF_MASK,
        VID_DCF_B2RPOL_MASK,
        VID_DCF_B2RPOL_EMP,
        HAL_ENABLE);

#endif /* all other STi55xx: 5514/16/17/78 */

} /* end of InitialiseLayer() function */


/*******************************************************************************
 * Name        : UpdateScreenParams
 * Description : Function called by upper level to inform output screen changes.
 *               In this case, it's only used to configure CCIR656 ouput window
 *               size and position.
 * Parameters  : LayerHandle and ouput parameters pointer.
 * Assumptions : Only two output format are supported : PAL and NTSC.
 * Limitations : idem.
 * Returns     : RAS.
 * *******************************************************************************/
void UpdateScreenParams(const STLAYER_Handle_t LayerHandle, const STLAYER_OutputParams_t * OutputParams_p)
{
#if defined(HW_5508) || defined(HW_5518)

    /* Nothing to do. 656 not supported on STi5508/18 */

#else /* all other STi55xx: 5514/16/17/78 */

    /* CCIR 656 ouput window size and position. */
    /* --------------  PAL case  -------------- */
    #define XDO_656_PAL     120
    #define XDS_656_PAL     843
    #define YDO_656_PAL     23
#ifdef WA_DIG_OUTPUT_TO_GX1_DVP
    #define YDS_656_PAL     310 /* patched value if output is captured by dvp/gx1 */
#else
    #define YDS_656_PAL     309 /* nominal value */
#endif
    /* -------------  NTSC  case  ------------- */
    #define XDO_656_NTSC     114
    #define XDS_656_NTSC     837
    #define YDO_656_NTSC     18
    #define YDS_656_NTSC     260
    /* ---------------------------------------- */

    /* Patch register set to allow a correct 656 mode.                  */
    /* 3 is given by validation team.                                   */
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_656B  , 3);

    /* Check for output mode                                            */
    /* As only NTSC and PAL are possible, check only the frame rate.    */
    if (OutputParams_p->FrameRate == 50000)
    {
        /* --------------  PAL case  -------------- */
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDO_656_16      , XDO_656_PAL >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDO_656_16 + 1  , XDO_656_PAL);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDS_656_16      , XDS_656_PAL >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDS_656_16 + 1  , XDS_656_PAL);

        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDO_656_16      , YDO_656_PAL >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDO_656_16 + 1  , YDO_656_PAL);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDS_656_16      , YDS_656_PAL >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDS_656_16 + 1  , YDS_656_PAL);
    }
    else
    {
        /* -------------  NTSC  case  ------------- */
        /* ----------  or default case.  ---------- */
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDO_656_16      , XDO_656_NTSC >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDO_656_16 + 1  , XDO_656_NTSC);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDS_656_16      , XDS_656_NTSC >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDS_656_16 + 1  , XDS_656_NTSC);

        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDO_656_16      , YDO_656_NTSC >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDO_656_16 + 1  , YDO_656_NTSC);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDS_656_16      , YDS_656_NTSC >> 8);
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDS_656_16 + 1  , YDS_656_NTSC);
    }

#endif /* all other STi55xx: 5514/16/17/78 */

} /* End of UpdateScreenParams() function. */


/*******************************************************************************
 * Name        : SetCIFMode
 * Description :
 * Parameters  :
 * Assumptions :
 * Limitations :
 * Returns     :
 * *******************************************************************************/
void SetCIFMode(const STLAYER_Handle_t LayerHandle,
                        const BOOL                          LumaFrameBased,
                        const BOOL                          ChromaFrameBased,
                        const BOOL                          LumaAndChromaThrowOneLineInTwo)
{
    U8 DISValue8;
    /*  We have to deal with CIF mode wich is into VID_DFS register and we dont
     *  want to change the other bits */
    DISValue8 = HAL_Read8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->VideoDisplayBaseAddress_p + VID_DIS_8);

    if (LumaFrameBased)
    {
        DISValue8 |= VID_DIS_LFB;
    }
    else
    {
        DISValue8 &= ~ VID_DIS_LFB;
    }
    if (ChromaFrameBased)
    {
        DISValue8 |= VID_DIS_CFB;
    }
    else
    {
        DISValue8 &= ~ VID_DIS_CFB;
    }
    if (LumaAndChromaThrowOneLineInTwo)
    {
        DISValue8 |= VID_DIS_ZOOM4;
    }
    else
    {
        DISValue8 &= ~ VID_DIS_ZOOM4;
    }
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->VideoDisplayBaseAddress_p + VID_DIS_8 , DISValue8 & VID_DIS_MASK);
} /* End of SetCIFMode() function */


/*******************************************************************************
Name        : SetInputMargins
Description : If APPLY is TRUE : set input margins
              If APPLY is FALSE : check if input margins are OK, and
                                  set new value in return pointers.
Parameters  :
Assumptions : Margins are in units of 1/16th pixels.
Limitations :
Returns     : Nothing
*******************************************************************************/
void SetInputMargins(const  STLAYER_Handle_t  LayerHandle,
                            S32 * const LeftMargin_p,
                            S32 * const TopMargin_p,
                            const  BOOL Apply)
{
    S32 ScanInUnitOfMB;
    U32 FractionalScanIn16thPixel, ChromaFractionalScanIn16thPixel; /* in the range of 0 to 15 in unit of line. */
    S32 Pan;
    U8  FractionnalPan;
    U16 IntegerPan;
    U16 YOffsetOdd = 0,  COffsetOdd = 0,  YOffsetEven = 0, COffsetEven = 0;
    U8 LumaZoomOut, ChromaZoomOut;
    BOOL IsThereFractionalScan;
    HALLAYER_PrivateData_t* LocalPrivateData_p;

    /* Retrieve some parameters of the source which are useful here.         */
    LocalPrivateData_p =
        ((HALLAYER_LayerProperties_t *)(LayerHandle))->PrivateData_p ;

    /************************************/
    /* Setup the SCAN vector (vertical) */
    /************************************/
    /* Note :                                               */
    /* The P&S vectors in the stream are in unit of  1/16 p */
    /* 256 =  16 lines of 16 subpixel                       */
    if (*TopMargin_p < 0)
    {
        *TopMargin_p = 0;  /* No support for such scan vectors ! */
    }
    ScanInUnitOfMB = (*TopMargin_p) / 256;
    FractionalScanIn16thPixel = ((*TopMargin_p) - (ScanInUnitOfMB * 256));
    ChromaFractionalScanIn16thPixel = FractionalScanIn16thPixel;
    if (LocalPrivateData_p->Scaling.B2RVFParams.LumaFrameBased)
    {
        /* Based on luma, could be on chroma, anyway this is a register problem, as SCN could be differentiated luma/chroma ! */
        /* Multiply by 2 */
        ScanInUnitOfMB = ScanInUnitOfMB << 1;
    }
    /* Don't divide by 2 in case of line drop (B2RVFParams.LumaAndChromaThrowOneLineInTwo), as SCN if made before this block. */
    if (ScanInUnitOfMB > LAYER_MAX_SCN)
    {
        ScanInUnitOfMB = LAYER_MAX_SCN;
    }

    if (FractionalScanIn16thPixel != 0)
    {
        IsThereFractionalScan = TRUE;

        YOffsetOdd  = LocalPrivateData_p->Scaling.B2RVFParams.LumaOffsetOdd;
        COffsetOdd  = LocalPrivateData_p->Scaling.B2RVFParams.ChromaOffsetOdd;
        YOffsetEven = LocalPrivateData_p->Scaling.B2RVFParams.LumaOffsetEven;
        COffsetEven = LocalPrivateData_p->Scaling.B2RVFParams.ChromaOffsetEven;
        LumaZoomOut = LocalPrivateData_p->Scaling.B2RVFParams.LumaZoomOut;
        ChromaZoomOut = LocalPrivateData_p->Scaling.B2RVFParams.ChromaZoomOut;

        /* Adjust fractional scan according to line drop before B2R */
        if (LocalPrivateData_p->Scaling.B2RVFParams.LumaFrameBased)
        {
            /* Multiply by 2 */
            FractionalScanIn16thPixel = FractionalScanIn16thPixel << 1;
        }
        if (LocalPrivateData_p->Scaling.B2RVFParams.ChromaFrameBased)
        {
            /* Multiply by 2 */
            ChromaFractionalScanIn16thPixel = ChromaFractionalScanIn16thPixel << 1;
        }
        if (LocalPrivateData_p->Scaling.B2RVFParams.LumaAndChromaThrowOneLineInTwo)
        {
            /* Divide by 2 */
            FractionalScanIn16thPixel = FractionalScanIn16thPixel >> 1;
            ChromaFractionalScanIn16thPixel = ChromaFractionalScanIn16thPixel >> 1;
        }

        /* Adjust fractional scan according to pre-compression before B2R */
        if (LumaZoomOut == 1)
        {
            FractionalScanIn16thPixel = FractionalScanIn16thPixel >> 1;
        }
        else if (LumaZoomOut == 2)
        {
            FractionalScanIn16thPixel = (3 * FractionalScanIn16thPixel) >> 3;
        }
        else if (LumaZoomOut == 3)
        {
            FractionalScanIn16thPixel = FractionalScanIn16thPixel >> 2;
        }
        if (ChromaZoomOut == 1)
        {
            ChromaFractionalScanIn16thPixel = ChromaFractionalScanIn16thPixel >> 1;
        }
        else if (ChromaZoomOut == 2)
        {
            ChromaFractionalScanIn16thPixel = (3 * ChromaFractionalScanIn16thPixel) >> 3;
        }
        else if (ChromaZoomOut == 3)
        {
            ChromaFractionalScanIn16thPixel = ChromaFractionalScanIn16thPixel >> 2;
        }

        /* We move from x/2 line on the TOP and x/2 line on the BOT = x line.    */
        YOffsetOdd  = YOffsetOdd + (FractionalScanIn16thPixel << 4);
        if (YOffsetOdd > 0x1FFF)
        {
            YOffsetOdd = 0x1FFF;
        }
        YOffsetEven = YOffsetEven + (FractionalScanIn16thPixel << 4);
        if (YOffsetEven > 0x1FFF)
        {
            YOffsetEven = 0x1FFF;
        }
        /* 2 times less lines in chroma (4:2:0) */
        COffsetOdd  = COffsetOdd + (ChromaFractionalScanIn16thPixel << 3);
        COffsetEven = COffsetEven + (ChromaFractionalScanIn16thPixel << 3);
    }
    else
    {
        IsThereFractionalScan = FALSE;
    }

    /*************************************/
    /* Setup the PAN vector (horizontal) */
    /*************************************/
    /* Note : The 55XX core handle PAN in 1/8th pixels */
    if (*LeftMargin_p < 0)
    {
        *LeftMargin_p = 0; /* No support for such pan vectors ! */
    }
    Pan = *LeftMargin_p >> 1;
    FractionnalPan = (U8)(Pan & 0x7);
    IntegerPan     = (U16)(Pan >> 3); /* In pixels */
    if (IntegerPan > LAYER_MAX_PAN)
    {
        IntegerPan = LAYER_MAX_PAN;
    }

    if (!Apply)
    {
        *LeftMargin_p = IntegerPan * 16; /* in 16th pixel */
    }
    else
    {
        /* Apply settings :                                                  */
        /* Integer part of PAN :                                             */
#ifdef WA_GNBvd03822
        if ((IntegerPan != 0) && (IsHorizontalDownUsed(LocalPrivateData_p->HorizontalZoomFactor)))
        {
            /* Using the Block to Row Converter (B2R) it is not possible to perform
            simultaneously horizontal downsampling and horizontal pan. Some values of
            pan do not work and introduce visable artefacts on the TV. Thus a smooth
            pan is not achievable. Below is the SW work-around.
            Bit 3 should not be used, so translate ranges ending by b0000-b1111
            to ranges b0000-b0111 with half precision of pan (all values used two times) */
            HAL_Write16((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                    VideoDisplayBaseAddress_p + VID_PAN_16,
                    ((IntegerPan & 0x07F0) | ((IntegerPan & 0xF) >> 1)) & VID_PAN_MASK);
        }
        else
        {
        /* Normal case */
#endif /* WA_GNBvd03822 */
        HAL_Write16((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_PAN_16, IntegerPan & VID_PAN_MASK);
#ifdef WA_GNBvd03822
        }
#endif /* WA_GNBvd03822 */

        /* Fractionnal part of PAN :                                         */
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_LSO_8, FractionnalPan);
        /* If the integer part is odd then add 128 */
        if ((IntegerPan & 0x01) == 0x01)
        {
            HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_CSO_8, FractionnalPan / 2 + 128);
        }
        else
        {
            HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_CSO_8, FractionnalPan / 2 );
        }

        /* Integer part of SCAN :                                            */
        HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
                VideoDisplayBaseAddress_p + VID_SCN_8, ScanInUnitOfMB & VID_SCN_MASK);

        /* Fractionnal part of SCAN :                                        */
        if (IsThereFractionalScan)
        {
            LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister1   = (LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister1   & 0x03) | ((YOffsetEven & 0x003F) << 2);
            LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister1 = (LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister1 & 0x03) | ((COffsetEven & 0x003F) << 2);
            LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister2   = ((YOffsetEven & 0x1FC0) >> 6) | ((YOffsetOdd  & 0x0001) << 7);
            LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister2 = ((COffsetEven & 0x1FC0) >> 6) | ((COffsetOdd  & 0x0001) << 7);
            LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister3   = (YOffsetOdd  & 0x01FE)>>1;
            LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister3 = (COffsetOdd  & 0x01FE)>>1;
            LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister4   = (LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister4   & 0xF0) | ((YOffsetOdd & 0x1E00) >> 9);
            LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister4 = (LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister4 & 0xF0) | ((COffsetOdd & 0x1E00) >> 9);
        }
    }
} /* end of SetInputMargins() function */


/*******************************************************************************
Name        : WriteB2RRegisters
Description : This function is used to write informations saved in
              B2RVerticalFiltersRegisters variable into the B2r Registers
Parameters  : Layer's handle
Assumptions :
Limitations :
Returns     : Nothing

WriteB2RRegisters is called at the end of functions SetViewportParams
and AdjustViewportParams after calling SetInputMargins and ApplyVerticalFactor
many times to write in B2RVerticalFiltersRegisters variable.

  STLAYER_AdjustViewportParams
              |                  ------------------->WriteB2RRegisters<---------------------
              v                  |                                                         |
  LAYERVID_AdjustViewportParams  |           ------------------------------------------    |
                             |   |           |                                        |    |
                             |   |           |                                        |    |
                             |   |           v                                        |    |
                AdjustOutputWindow--->SetViewPortPositions------->SetInputMargins     |    |
                ^       ^    |   |                                     ^              |    |
                |       |    v   |                                     |              |    |
                |       AdjustViewportParams--->AdjustInputWindow------|              |    |
                |                                   ^                                 |    |
     ---------->SetViewportParams-------------------|----------------------------------    |
     |           ^              |                   |                                      |
     |           |              |                   v                                      |
     |           OpenViewport   |                ApplyVerticalFactor                       |
     |           ^              ------------------------------------------------------------
     |           |
     |           STLAYER_OpenViewPort
     |
LAYERVID_SetViewportParams
     ^
     |
STLAYER_SetViewPortParams
*******************************************************************************/
void WriteB2RRegisters(const  STLAYER_Handle_t LayerHandle)
{
    HALLAYER_PrivateData_t *                LocalPrivateData_p;

    LocalPrivateData_p = ((HALLAYER_LayerProperties_t *)(LayerHandle))->PrivateData_p;
#if defined(HW_5508) || defined(HW_5518)
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFLMODE0 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister0);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFCMODE0 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister0);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFLMODE1 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister1);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFCMODE1 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister1);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFLMODE2 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister2);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFCMODE2 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister2);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFLMODE3 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister3);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFCMODE3 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister3);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFLMODE4 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister4);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_VFCMODE4 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister4);
#else /* all other STi55xx: 5514/16/17/78 */
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_YMODE0 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister0);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_CMODE0 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister0);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_YMODE1 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister1);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_CMODE1 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister1);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_YMODE2 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister2);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_CMODE2 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister2);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_YMODE3 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister3);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_CMODE3 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister3);

    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_YMODE4 , LocalPrivateData_p->B2RVerticalFiltersRegisters.LumaRegister4);
    HAL_Write8((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_CMODE4 , LocalPrivateData_p->B2RVerticalFiltersRegisters.ChromaRegister4);
#endif
} /* end of WriteB2RRegisters() function */

/*******************************************************************************
Name        : ApplyVerticalFactor
Description : Apply the zoom calculated by the AdjustVerticalFactor function.
Parameters  :
Assumptions : Margins are in units of 1/8th pixels.
Limitations :
Returns     : Nothing
*******************************************************************************/
void ApplyVerticalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        BOOL                                Progressive,
                        U32                                 Zx,
                        U32                                 Zy)
{
    HALLAYER_PrivateData_t *                LocalPrivateData_p;
    HALLAYER_ComputeB2RVerticalFiltersInputParams_t  ComputeB2RVerticalFiltersInputParams;
    HALLAYER_B2RVerticalFiltersParams_t     B2RVerticalFiltersParams;

    /* Retrieve some useful factors.                                           */
    LocalPrivateData_p = ((HALLAYER_LayerProperties_t *)(LayerHandle))->PrivateData_p;

    if (Zy == INT_AV_SCALE_CONSTANT)
    {
        /* Treat particular case of full screen with direct values... */
        B2RVerticalFiltersParams.LumaInterpolate = TRUE;
        B2RVerticalFiltersParams.LumaZoomOut = 0;
        B2RVerticalFiltersParams.ChromaInterpolate = TRUE;
        B2RVerticalFiltersParams.ChromaZoomOut = 0;
        if (Progressive)
        {
            /* Progressive */
            B2RVerticalFiltersParams.LumaOffsetOdd = 1536;
            B2RVerticalFiltersParams.LumaOffsetEven = 1024;
            B2RVerticalFiltersParams.LumaIncrement = 1023;
            B2RVerticalFiltersParams.ChromaOffsetOdd = 640;
            B2RVerticalFiltersParams.ChromaOffsetEven = 384;
            B2RVerticalFiltersParams.ChromaIncrement = 511;
            B2RVerticalFiltersParams.LumaFrameBased = TRUE;
            B2RVerticalFiltersParams.ChromaFrameBased = TRUE;
        }
        else
        {
            /* Interlaced */
            B2RVerticalFiltersParams.LumaOffsetOdd = 512;
            B2RVerticalFiltersParams.LumaOffsetEven = 512;
            B2RVerticalFiltersParams.LumaIncrement = 511;
            B2RVerticalFiltersParams.ChromaOffsetOdd = 64;
            B2RVerticalFiltersParams.ChromaOffsetEven = 192;
            B2RVerticalFiltersParams.ChromaIncrement = 255;
            B2RVerticalFiltersParams.LumaFrameBased = FALSE;
            B2RVerticalFiltersParams.ChromaFrameBased = FALSE;
        }
        if ((Zx != 0) && (Zx < (INT_AV_SCALE_CONSTANT / 2)))
        {
            B2RVerticalFiltersParams.LumaHorizDown = TRUE;
            B2RVerticalFiltersParams.ChromaHorizDown =TRUE;
        }
        else
        {
            B2RVerticalFiltersParams.LumaHorizDown = FALSE;
            B2RVerticalFiltersParams.ChromaHorizDown = FALSE;
        }
        B2RVerticalFiltersParams.LumaAndChromaThrowOneLineInTwo = FALSE;
    }
    else
    {
        /* Standard algorithm for computation of B2R parameters */
        ComputeB2RVerticalFiltersInputParams.ProgressiveSource = Progressive;
        ComputeB2RVerticalFiltersInputParams.IsFreezing = FALSE; /* !!! */
        ComputeB2RVerticalFiltersInputParams.VerticalZoomFactor = Zy;
        ComputeB2RVerticalFiltersInputParams.HorizontalZoomFactor = Zx;
        ComputeB2RVerticalFilters(&ComputeB2RVerticalFiltersInputParams, &B2RVerticalFiltersParams);
    }
    /* Now compile parameters into registers */
    CompileB2RVerticalFiltersRegisterValues(&B2RVerticalFiltersParams, &LocalPrivateData_p->B2RVerticalFiltersRegisters);

    if ((LocalPrivateData_p->Scaling.B2RVFParams.LumaFrameBased != B2RVerticalFiltersParams.LumaFrameBased) ||
        (LocalPrivateData_p->Scaling.B2RVFParams.ChromaFrameBased != B2RVerticalFiltersParams.ChromaFrameBased) ||
        (LocalPrivateData_p->Scaling.B2RVFParams.LumaAndChromaThrowOneLineInTwo != B2RVerticalFiltersParams.LumaAndChromaThrowOneLineInTwo)
       )
    {
        SetCIFMode(LayerHandle, B2RVerticalFiltersParams.LumaFrameBased,
                                B2RVerticalFiltersParams.ChromaFrameBased,
                                B2RVerticalFiltersParams.LumaAndChromaThrowOneLineInTwo);
    }
    /* Copy current parameters in private data */
    LocalPrivateData_p->Scaling.B2RVFParams = B2RVerticalFiltersParams;
} /* end of ApplyVerticalFactor() function */


/*******************************************************************************
 * Name        : ApplyHorizontalFactor
 * Description :
 * Parameters  :
 * Assumptions :
 * Limitations :
 * Returns     :
 * *******************************************************************************/
void ApplyHorizontalFactor(
                        const STLAYER_Handle_t              LayerHandle,
                        U32                                 Zx,
                        U32                                 Zy)
{
  /* Horizontal scaling is dependant of the Vertical scaling when X */
  /* from zoom out by 2 to zoom out by 4. In this case, hardware    */
  /* limitations exist : */
  U32 AppliedScaling ;
    HALLAYER_PrivateData_t * LocalPrivateData_p;

    /* Retrieve some useful factors. */
    LocalPrivateData_p = ((HALLAYER_LayerProperties_t *)(LayerHandle))->PrivateData_p;

  if (Zx == INT_AV_SCALE_CONSTANT)
  {
    DisableHorizontalScaling(LayerHandle);
  }
  else
  {
    /* Not the real AppliedScaling but the value to be written in the SRC */
    if (Zx != 0)
    {
/*        if (LocalPrivateData_p->Scaling.B2RVFParams.LumaHorizDown)*/
        if (IsHorizontalDownUsed(Zx))
        {
            /* Vertical filter does an horizontal downsampling of 2 already */
            AppliedScaling = ((256 * INT_AV_SCALE_CONSTANT) / Zx) / 2;
        }
        else
        {
            AppliedScaling = ((256 * INT_AV_SCALE_CONSTANT) / Zx);
        }
        if (AppliedScaling > LSR_MAX)
        {
            AppliedScaling = LSR_MAX;
        }
    }
    else
    {
        AppliedScaling = LSR_MAX;
    }

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
} /* Enf of ApplyHorizontalFactor() function */


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
        U32* Zx_p)
{
    U32 Zx;
    /* hardware constraint */
    U32 SourceWidth;
    U32 SourceHeight;
    U32 DestWidth;
    U32 DestHeight;
    HALLAYER_PrivateData_t * LocalPrivateData_p;

    /* Retrieve some useful factors. */
    LocalPrivateData_p = ((HALLAYER_LayerProperties_t *)(LayerHandle))->PrivateData_p;

    SourceWidth     = Params_p->InputRectangle.Width;
    SourceHeight    = Params_p->InputRectangle.Height;
    DestWidth       = Params_p->OutputRectangle.Width;
    DestHeight      = Params_p->OutputRectangle.Height;

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
            Zx = INT_AV_SCALE_CONSTANT ;
        }

        if (Zx > MAX_H_SCALING)
        {
            Zx = MAX_H_SCALING;
        }
        else
        {
          if (Zx < MIN_H_SCALING )
          {
           Zx = MIN_H_SCALING ;
          }
        }
        /* Check if horizontal filtering is not at max */
        if (IsHorizontalDownUsed(Zx))
        {
            /* Vertical filter does an horizontal downsampling of 2 already */
            if (((256 * INT_AV_SCALE_CONSTANT) / Zx / 2) > LSR_MAX)
            {
                Zx = (256 * INT_AV_SCALE_CONSTANT) / LSR_MAX / 2;
            }
        }
        else
        {
            if (((256 * INT_AV_SCALE_CONSTANT) / Zx) > LSR_MAX)
            {
                Zx = (256 * INT_AV_SCALE_CONSTANT) / LSR_MAX;
            }
        }
   }

   *Zx_p = Zx;
} /* End of AdjustHorizontalFactor() function */


/*******************************************************************************
Name        : AdjustVerticalFactor
Description : Check if input margins are OK and set new value in return pointers.
Parameters  :
Assumptions : Margins are in units of 1/8th pixels.
Limitations :
Returns     : Nothing
*******************************************************************************/
void AdjustVerticalFactor(  const STLAYER_Handle_t              LayerHandle,
                            STLAYER_ViewPortParams_t * const    Params_p,
                            U32*                                Zy_p)
{
U32                     Zy;
U16                     Increment;
U16                     INC_MAX = 1023;               /* hardware constraint */
U16                     INC_NOSCALE = 511;            /* no scaling applied */
U32                     SourceWidth, SourceHeight, DestWidth, DestHeight;

  SourceWidth    = Params_p->InputRectangle.Width;
  SourceHeight   = Params_p->InputRectangle.Height;
  DestWidth      = Params_p->OutputRectangle.Width;
  DestHeight     = Params_p->OutputRectangle.Height;

  if (SourceHeight == DestHeight)
  {
    Zy = INT_AV_SCALE_CONSTANT;
    Increment = INC_NOSCALE;
  }
  else
  {
    if (SourceHeight != 0)
    {
        Zy = (INT_AV_SCALE_CONSTANT * DestHeight) / SourceHeight;
    }
    else
    {
        Zy = INT_AV_SCALE_CONSTANT;
    }

    if (Zy > MAX_V_SCALING)
    {
      Zy = MAX_V_SCALING;
    }
    else
    {
      if (Zy < MIN_V_SCALING)
      {
        Zy = MIN_V_SCALING;
      }
    }

    /* Return the effective scaling that will be applied by the device */
    Increment = ((512 * INT_AV_SCALE_CONSTANT) / Zy)  - 1;

    if ((Increment > INC_MAX) && (Increment < ((4 * INC_MAX) / 3)))
    {
        /* Zoom Out x2 to Zoom Out x3 */
        Increment = Increment / 2;
        Zy = (512 * INT_AV_SCALE_CONSTANT) / ((2 * Increment) + 1);
    }
    else if ((Increment >= ((4 * INC_MAX) / 3)) && (Increment <= (2 * INC_MAX )))
    {
        /* Zoom Out x3 to Zoom Out x4 */
        Increment = (3 * Increment) / 8;
        Zy = (512 * INT_AV_SCALE_CONSTANT) / (((8 * Increment) / 3) + 1 );
    }
    else if (Increment > (2 * INC_MAX ))
    {
        /* Zoom Out x4 */
        Increment = Increment / 4;
        Zy = (512 * INT_AV_SCALE_CONSTANT) / ((4 * Increment) + 1);
    }
    else
    {
        /* Up to Zoom Out x2 */
        Zy = (512 * INT_AV_SCALE_CONSTANT) / (Increment + 1);
    }
  } /* end of (SourceHeight == DestHeight) */

  *Zy_p = Zy;
} /* end of AdjustVerticalFactor() function */


/*******************************************************************************
Name        : SetViewPortPositions
Description : Hardware writing of XDO, XDS, YDO, YDS.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void SetViewPortPositions(
        const STLAYER_ViewPortHandle_t  ViewportHandle,
        STGXOBJ_Rectangle_t * const     OutputRectangle_p,
        const BOOL                      Apply)
{
BOOL                        B2RMode4;
BOOL                        B2RMode5;
BOOL                        B2RMode6;
BOOL                        B2RMode89;
U32                         XDOPos, XDSPos, YDOPos, YDSPos;
S32                         XOffset, YOffset;
HALLAYER_PrivateData_t      *LayerPrivateData_p;
STLAYER_Handle_t            LayerHandle;

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
            case 25000 :
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
            case 25000 :
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
                VideoDisplayBaseAddress_p + VID_XDO_16,
                XDOPos & VID_XD0_MASK);
#if defined(HW_5508) || defined(HW_5518)
            HAL_Write16((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDO_16,
                YDOPos & VID_YDO_MASK_0818);
#else /* all other STi55xx: 5514/16/17/78 */
            HAL_Write16((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDO_16,
                YDOPos & VID_YDO_MASK);
#endif /* all other STi55xx: 5514/16/17/78 */
        }

        /* Retrieve the final positions XDS, YDS.                                */
        XDSPos = OutputRectangle_p->Width + XDOPos;
        YDSPos = (OutputRectangle_p->Height / 2) + YDOPos - 1;

        /* Hardware limitations. */
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
            /* Write the registers XDS, YDS. */
            HAL_Write16((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                VideoDisplayBaseAddress_p + VID_XDS_16,
                XDSPos & VID_XDS_MASK);
#if defined(HW_5508) || defined(HW_5518)
            HAL_Write16((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDS_16,
                YDSPos & VID_YDS_MASK_0818);
#else /* all other STi55xx: 5514/16/17/78 */
            HAL_Write16((U8 *) ((HALLAYER_LayerProperties_t *) LayerHandle)->
                VideoDisplayBaseAddress_p + VID_YDS_16,
                YDSPos & VID_YDS_MASK);
#endif /* all other STi55xx: 5514/16/17/78 */

            /* Big risk due to a bug : 0x00xx should be writen here after. */

            /* Call to SetInputMargins. Writes the pan&scan vectors. */
            XOffset = LayerPrivateData_p->ViewportParams.InputRectangle.PositionX ;
            YOffset = LayerPrivateData_p->ViewportParams.InputRectangle.PositionY ;
            SetInputMargins(LayerHandle, &XOffset, &YOffset, TRUE);
        }
    }
} /* end of SetViewPortPositions() function */

/* end of file 551-4678.c */
