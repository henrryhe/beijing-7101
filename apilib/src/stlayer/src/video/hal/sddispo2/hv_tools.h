/*******************************************************************************

File name   : hv_tools.h

Description :

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
 05 Jul 2003        Created                                   Alexandre Nabais
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_LAYER_TOOLS_H
#define __VIDEO_LAYER_TOOLS_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */

#include "hv_lay8.h"
#include "stos.h"
/* Exported Macros ---------------------------------------------------------- */

/* Private DISPLAY management functions ------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void stlayer_GetIncrement(
        STGXOBJ_Rectangle_t * InputRectangle_p,
        STGXOBJ_Rectangle_t * OutputRectangle_p,
        BOOL IsFormat422Not420,
        BOOL IsInputScantypeProgressive,
        BOOL IsOutputScantypeProgressive,
        HALLAYER_Increment_t *Increment_p);

void stlayer_GetPositionDefinition(
        STGXOBJ_Rectangle_t *           InputRectangle_p,
        HALLAYER_Increment_t *          Increment_p,
        BOOL                            IsInputScantypeProgressive,
        BOOL                            IsOutputScantypeProgressive,
        BOOL                            IsFormat422Not420,
        BOOL                            IsFormatRasterNotMacroblock,
        BOOL                            IsFirstLineTopNotBot,
        BOOL                            IsLmuActivated,
        BOOL                            IsPseudo422FromTopField,
        U32                             DecimXOffset,
        U32                             DecimYOffset,
        STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
        STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation,
        BOOL                            advanced_decimation,
        HALLAYER_PositionDefinition_t * PositionDefinition_p);

void stlayer_GetMemorySize(
        STLAYER_Handle_t                    LayerHandle,
        STGXOBJ_Rectangle_t *               InputRectangle_p,
        STGXOBJ_Rectangle_t *               OutputRectangle_p,
        HALLAYER_Increment_t *              Increment_p,
        HALLAYER_Phase_t *                  Phase_p,
        HALLAYER_MemoryPosition_t *         MemoryPosition_p,
        HALLAYER_FirstPixelRepetition_t *   FirstPixelRepetition_p,
        BOOL                                IsInputScantypeProgressive,
        BOOL                                IsOutputScantypeProgressive,
        BOOL                                IsFirstLineTopNotBot,
        BOOL                                IsFormat422Not420,
        BOOL                                IsFormatRasterNotMacroblock,
        HALLAYER_MemorySize_t *             MemorySize_p);

HALLAYER_Filter_t stlayer_SelectHorizontalLumaFilter(U32 Increment, U32 Phase);
HALLAYER_Filter_t stlayer_SelectHorizontalChromaFilter(U32 Increment, U32 Phase);
HALLAYER_Filter_t stlayer_SelectVerticalLumaFilter(U32 Increment, U32 PhaseTop, U32 PhaseBot);
HALLAYER_Filter_t stlayer_SelectVerticalChromaFilter(U32 Increment, U32 PhaseTop, U32 PhaseBot);

ST_ErrorCode_t stlayer_WriteHorizontalFilterCoefficientsToMemory(
            U32 * Memory_p, HALLAYER_Filter_t Filter);
ST_ErrorCode_t stlayer_WriteVerticalFilterCoefficientsToMemory(
            U32 * Memory_p, HALLAYER_Filter_t Filter);

#if defined(HAL_VIDEO_LAYER_FILTER_DISABLE) || defined (STLAYER_USE_CRC)
HALLAYER_Filter_t stlayer_WriteHorizontalNullCoefficientsFilterToMemory(U32* MemoryPointer);
HALLAYER_Filter_t stlayer_WriteVerticalNullCoefficientsFilterToMemory(U32* MemoryPointer);
#endif /* HAL_VIDEO_LAYER_FILTER_DISABLE || STLAYER_USE_CRC */

#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t stlayer_LoadCoefficientsFilters(U32* DestinationBaseAddress, U32 DestinationSize, U32 Alignment);
ST_ErrorCode_t stlayer_GetHorizontalCoefficientsFilterAddress(const U32** FilterAddress, HALLAYER_Filter_t* LoadedLumaFilter, HALLAYER_Filter_t* LoadedChromaFilter, U32 LumaIncrement, U32 LumaPhase);
ST_ErrorCode_t stlayer_GetVerticalCoefficientsFilterAddress(const U32** FilterAddress, HALLAYER_Filter_t* LoadedLumaFilter, HALLAYER_Filter_t* LoadedChromaFilter, U32 LumaIncrement, U32 LumaPhase);
#endif /* defined(DVD_SECURED_CHIP) */

void stlayer_UpdateDisplayRegisters(STLAYER_Handle_t LayerHandle,
        HALLAYER_RegisterMap_t * DisplayRegister_p,
        U8                       EnaHFilterUpdate,
        U8                       EnaVFilterUpdate);

ST_ErrorCode_t GetViewportPsiParameters(const STLAYER_Handle_t LayerHandle, STLAYER_PSI_t * const ViewportPSI_p);
ST_ErrorCode_t SetViewportPsiParameters(const STLAYER_Handle_t LayerHandle, const STLAYER_PSI_t * const ViewportPSI_p);
ST_ErrorCode_t InitFilter (const STLAYER_Handle_t LayerHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_LAYER_TOOLS_H */

