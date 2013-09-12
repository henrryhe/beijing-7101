/*******************************************************************************

File name   : hv_layer.h

Description : HAL video decode omega1 family header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
12 Jul 2000        Created                                           HG
12 Jul 2000        Modifications for Omega1                          AN
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_LAYER_OMEGA1_H
#define __HAL_VIDEO_LAYER_OMEGA1_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stgxobj.h"
#include "halv_lay.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
/* According to the datasheet:      */
/*   XDOMin can't be less than 44   */
/*   last active sample is YDS + 14 */
#define INT_AV_SCALE_CONSTANT             1024

#define MIN_XDO      44
#define MAX_XDS      850

#define MIN_YDO      1
#define MAX_YDS      312  /* nb lines between 2 VSync */

#define VERTICAL_SCALING_MIN       ((3 * INT_AV_SCALE_CONSTANT) / 8)   /* Vx0.375*/
#define HORIZONTAL_SCALING_MIN     ((1 * INT_AV_SCALE_CONSTANT) / 4)   /* Hx0.25 */
#define HORIZONTAL_SCALING_MAX     ((8 * INT_AV_SCALE_CONSTANT) / 1)   /* Hx8    */
#define H_SCALING_OUT_X2  ((1 * INT_AV_SCALE_CONSTANT) / 2)         /* Hx0.5 */



#if defined(HW_5510) || defined(HW_5508) || defined(HW_5518)
#define LAYER_MAX_SCN     0x3F
#endif /* defined(HW_5510) || defined(HW_5508) || defined(HW_5518) */
#if defined(HW_5512) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578)
#define LAYER_MAX_SCN     0x1FF
#endif /* defined(HW_5512) || defined(HW_5514) || defined(HW_5516) || defined(HW_5517) || defined(HW_5578) */
#define LAYER_MAX_PAN     0x7FF

#define HAL_ENABLE												1
#define HAL_DISABLE												0

/* Exported Types ----------------------------------------------------------- */

typedef struct HALLAYER_B2RVerticalFiltersParams_s
{
    BOOL    LumaHorizDown;
    BOOL    LumaInterpolate;
    U8      LumaZoomOut;
    U16     LumaOffsetOdd;
    U16     LumaOffsetEven;
    U16     LumaIncrement;
    BOOL    ChromaHorizDown;
    BOOL    ChromaInterpolate;
    U8      ChromaZoomOut;
    U16     ChromaOffsetOdd;
    U16     ChromaOffsetEven;
    U16     ChromaIncrement;
    BOOL    LumaFrameBased;
    BOOL    ChromaFrameBased;
    BOOL    LumaAndChromaThrowOneLineInTwo;
} HALLAYER_B2RVerticalFiltersParams_t;

typedef struct HALLAYER_B2RVerticalFiltersRegisters_s
{
    U8      LumaRegister0;
    U8      LumaRegister1;
    U8      LumaRegister2;
    U8      LumaRegister3;
    U8      LumaRegister4;
    U8      ChromaRegister0;
    U8      ChromaRegister1;
    U8      ChromaRegister2;
    U8      ChromaRegister3;
    U8      ChromaRegister4;
} HALLAYER_B2RVerticalFiltersRegisters_t;

typedef struct HALLAYER_ComputeB2RVerticalFiltersInputParams_s
{
    BOOL    ProgressiveSource;  /* Progressive or interlaced */
    BOOL    IsFreezing;         /* Cares only if interlaced */
    U32     VerticalZoomFactor; /* Multiplied by INT_AV_SCALE_CONSTANT */
    U32     HorizontalZoomFactor; /* Multiplied by INT_AV_SCALE_CONSTANT */
} HALLAYER_ComputeB2RVerticalFiltersInputParams_t;

/*typedef struct HALLAYER_PrivateData_B2R_s
{
    U16 YOffsetOdd;
    U16 COffsetOdd;
    U16 YOffsetEven;
    U16 COffsetEven;
    U8  LumaZoomOut;
    U8  ChromaZoomOut;
    U16 Increment;
} HALLAYER_PrivateData_B2R_t ;*/

typedef struct HALLAYER_PrivateData_Scaling_s
{
    BOOL                        B2RMode4;
    BOOL                        B2RMode5;
    BOOL                        B2RMode6;
    BOOL                        B2RMode89;
    HALLAYER_B2RVerticalFiltersParams_t B2RVFParams; /* used for 5508/5518 only */
} HALLAYER_PrivateData_Scaling_t ;

typedef struct HALLAYER_PrivateData_OutputParams_s
{
    U32 XValue;
    U32 YValue;
    U32 XStart;
    U32 YStart;
    S8  XOffset;
    S8  YOffset;
    U32 Width;
    U32 Height;
    U32 FrameRate;
    BOOL DisplayEnableFromMixer;
} HALLAYER_PrivateData_OutputParams_t ;

typedef struct HALLAYER_PrivateData_s
{
    STLAYER_Handle_t                        AssociatedLayerHandle;
    BOOL                                    Opened;
    HALLAYER_PrivateData_Scaling_t          Scaling;
    HALLAYER_PrivateData_OutputParams_t     OutputParams;
    STLAYER_LayerParams_t                   LayerParams;
    STLAYER_ViewPortParams_t                ViewportParams;
    U32                                     HorizontalZoomFactor;
    U32                                     VerticalZoomFactor;
    U8                                      AlphaA0;
    BOOL                                    DisplayEnableFromVideo;
    HALLAYER_B2RVerticalFiltersRegisters_t  B2RVerticalFiltersRegisters;
} HALLAYER_PrivateData_t ;

/* Exported Variables ------------------------------------------------------- */
extern const HALLAYER_FunctionsTable_t HALLAYER_Omega1Functions;

/* Exported Macros ---------------------------------------------------------- */
#define HAL_Read8(Address_p)     STSYS_ReadRegDev8((void *) (Address_p))
#define HAL_Read16(Address_p)    STSYS_ReadRegDev16BE((void *) (Address_p))
#define HAL_Read32(Address_p)    STSYS_ReadRegDev32BE((void *) (Address_p))

#define HAL_Write8(Address_p, Value) STSYS_WriteRegDev8((void *) (Address_p), (Value))
#define HAL_Write16(Address_p, Value) STSYS_WriteRegDev16BE((void *) (Address_p), (Value))
#define HAL_Write32(Address_p, Value) STSYS_WriteRegDev32BE((void *) (Address_p), (Value))

#define HAL_SetRegister16Value(Address_p, RegMask, Mask, Emp,Value)                \
{                                                                     \
    U16 tmp16;                                                        \
    tmp16 = HAL_Read16 (Address_p);                                   \
    HAL_Write16 (Address_p, (tmp16 & RegMask & (~((U16)Mask << Emp)) ) | ((U16)Value << Emp));   \
}

#define HAL_SetRegister8Value(Address_p, RegMask, Mask, Emp,Value)                 \
{                                                                     \
    U8 tmp8;                                                          \
    tmp8 = HAL_Read8 (Address_p);                                     \
    HAL_Write8 (Address_p, (tmp8 & RegMask & (~((U8)Mask << Emp)) ) | ((U8)Value << Emp));   \
}

#define HAL_GetRegister16Value(Address_p, Mask, Emp)  (((U16)(HAL_Read16(Address_p))&((U16)(Mask)<<Emp))>>Emp)
#define HAL_GetRegister8Value(Address_p, Mask, Emp)   (((U8)(HAL_Read8(Address_p))&((U8)(Mask)<<Emp))>>Emp)


/* Exported Functions ------------------------------------------------------- */

extern void ComputeB2RVerticalFilters(const HALLAYER_ComputeB2RVerticalFiltersInputParams_t * const InputParams_p,
                                    HALLAYER_B2RVerticalFiltersParams_t * const OutputParams_p);
extern void CompileB2RVerticalFiltersRegisterValues(const HALLAYER_B2RVerticalFiltersParams_t * const OutputParams_p,
                                    HALLAYER_B2RVerticalFiltersRegisters_t * const Registers_p);
extern BOOL IsHorizontalDownUsed(const U32 HorizontalZoomFactor);

void EnableHorizontalScaling(const STLAYER_Handle_t LayerHandle);
void DisableHorizontalScaling(const STLAYER_Handle_t LayerHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_LAYER_OMEGA1_H */

/* End of hv_layer.h */
