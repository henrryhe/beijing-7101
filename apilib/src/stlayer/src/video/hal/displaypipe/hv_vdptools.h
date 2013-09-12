/*******************************************************************************

File name   : hv_vdptools.h

Description : Header file for displaypipe tools module

COPYRIGHT (C) STMicroelectronics 2005-2006

Date               Modification                                     Name
----               ------------                                     ----
18 Oct 2005        Created                                          DG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_LAYER_VDP_TOOLS_H
#define __VIDEO_LAYER_VDP_TOOLS_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */

#include "hv_vdplay.h"
#include "stos.h"

/* Exported Macros ---------------------------------------------------------- */

/* Private DISPLAY management functions ------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void stlayer_VDPGetIncrement(STGXOBJ_Rectangle_t * InputRectangle_p, STGXOBJ_Rectangle_t * OutputRectangle_p, BOOL IsFormat422Not420,
        BOOL IsInputScantypeProgressive, BOOL IsOutputScantypeProgressive,
        HALLAYER_VDPIncrement_t *Increment_p);

void stlayer_VDPGetPositionDefinition(
        STGXOBJ_Rectangle_t *           InputRectangle_p,
        HALLAYER_VDPIncrement_t *       Increment_p,
        BOOL                            IsInputScantypeProgressive,
        BOOL                            IsOutputScantypeProgressive,
        BOOL                            IsFormat422Not420,
        BOOL                            IsFormatRasterNotMacroblock,
        BOOL                            IsFirstLineTopNotBot,
        BOOL                            IsDeiActivated,
        BOOL                            IsPseudo422FromTopField,
        U32                             DecimXOffset,
        U32                             DecimYOffset,
        STLAYER_DecimationFactor_t      CurrentPictureHorizontalDecimation,
        STLAYER_DecimationFactor_t      CurrentPictureVerticalDecimation,
        BOOL                            advanced_decimation,
        HALLAYER_VDPPositionDefinition_t * PositionDefinition_p);

HALLAYER_VDPFilter_t stlayer_VDPSelectHorizontalLumaFilter(U32 Increment, U32 Phase);
HALLAYER_VDPFilter_t stlayer_VDPSelectHorizontalChromaFilter(U32 Increment, U32 Phase);
HALLAYER_VDPFilter_t stlayer_VDPSelectVerticalLumaFilter(U32 Increment, U32 PhaseTop, U32 PhaseBot);
HALLAYER_VDPFilter_t stlayer_VDPSelectVerticalChromaFilter(U32 Increment, U32 PhaseTop, U32 PhaseBot);

ST_ErrorCode_t stlayer_VDPWriteHorizontalFilterCoefficientsToMemory(
            U32 * Memory_p, HALLAYER_VDPFilter_t Filter);
ST_ErrorCode_t stlayer_VDPWriteVerticalFilterCoefficientsToMemory(
            U32 * Memory_p, HALLAYER_VDPFilter_t Filter);

#if defined(HAL_VIDEO_LAYER_FILTER_DISABLE)
HALLAYER_VDPFilter_t stlayer_VDPWriteHorizontalNullCoefficientsFilterToMemory(U32* MemoryPointer);
HALLAYER_VDPFilter_t stlayer_VDPWriteVerticalNullCoefficientsFilterToMemory(U32* MemoryPointer);
#endif /* #if defined(HAL_VIDEO_LAYER_FILTER_DISABLE) */

#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t stlayer_VDPLoadCoefficientsFilters(U32* DestinationBaseAddress, U32 DestinationSize, U32 Alignment);
ST_ErrorCode_t stlayer_VDPGetHorizontalCoefficientsFilterAddress(const U32** FilterAddress, HALLAYER_VDPFilter_t* LoadedLumaFilter, HALLAYER_VDPFilter_t* LoadedChromaFilter, U32 LumaIncrement, U32 LumaPhase);
ST_ErrorCode_t stlayer_VDPGetVerticalCoefficientsFilterAddress(const U32** FilterAddress, HALLAYER_VDPFilter_t* LoadedLumaFilter, HALLAYER_VDPFilter_t* LoadedChromaFilter, U32 LumaIncrement, U32 LumaPhase);
#endif /* defined(DVD_SECURED_CHIP) */

void stlayer_VDPUpdateDisplayRegisters(STLAYER_Handle_t LayerHandle,
                    HALLAYER_VDPVhsrcRegisterMap_t *    VhsrcRegister_p,
                    HALLAYER_VDPDeiRegisterMap_t *      DeiRegister_p,
                    U8                                  EnaHFilterUpdate,
                    U8                                  EnaVFilterUpdate);

ST_ErrorCode_t VDPGetViewportPsiParameters(const STLAYER_Handle_t LayerHandle, STLAYER_PSI_t * const ViewportPSI_p);
ST_ErrorCode_t VDPSetViewportPsiParameters(const STLAYER_Handle_t LayerHandle, const STLAYER_PSI_t * const ViewportPSI_p);
ST_ErrorCode_t VDPInitFilter (const STLAYER_Handle_t LayerHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_LAYER_VDP_TOOLS_H */

