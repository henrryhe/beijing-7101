/************************************************************************

COPYRIGHT (C) STMicroelectronics 2000

Source file name : svm.h

VOUT driver module.
************************************************************************/

#ifndef __SVM_H
#define __SVM_H

/*
******************************************************************************
Includes
******************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
******************************************************************************
Exported Types
******************************************************************************
*/
/*
******************************************************************************
Exported Constants
******************************************************************************
*/
/*
******************************************************************************
Exported Variables (Globals, not static)
******************************************************************************
*/
/*
******************************************************************************
Exported Macros
******************************************************************************
*/
/*
******************************************************************************
Exported Functions (not static)
******************************************************************************
*/
ST_ErrorCode_t stvout_HalDisableSVMDac( void* BaseAddress_p, U8 Dac);
ST_ErrorCode_t stvout_HalEnableSVMDac( void* BaseAddress_p, U8 Dac);
ST_ErrorCode_t stvout_HalSVMDelayComp( void* BaseAddress, U32 DelayComp);
ST_ErrorCode_t stvout_HalSVMModulationShapeControl( void* BaseAddress, U32 ShapeType);
ST_ErrorCode_t stvout_HalSVMGainControl( void* BaseAddress, U32 Gain);
ST_ErrorCode_t stvout_HalSVMOSDFactor( void* BaseAddress, U32 Factor);
ST_ErrorCode_t stvout_HalSVMFilter( void* BaseAddress, U32 Filter);
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SVM_H */
