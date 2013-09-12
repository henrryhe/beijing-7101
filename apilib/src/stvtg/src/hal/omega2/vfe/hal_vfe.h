/*****************************************************************************

File name : hal_vfe.h

Description :  HAL for VTG programmation on Video Front End

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
02 Jul 2001        Created                                          HSdLM
04 Sep 2001        New interrupt init routine                       HSdLM
09 Oct 2001        Use STINTMR for VFE                              HSdLM
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_HAL_VFE_H
#define __VTG_HAL_VFE_H

/* Includes --------------------------------------------------------------- */
#include "stvtg.h"
#include "vtg_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions  -----------------------------------------------------*/

void stvtg_HALVfeSetCapability(  stvtg_Device_t * const Device_p);

ST_ErrorCode_t stvtg_HALVfeCheckSlaveModeParams( const STVTG_SlaveModeParams_t * const SlaveModeParams_p);

void stvtg_HALVfeSetModeParams(  stvtg_Device_t *         const Device_p
                               , const STVTG_TimingMode_t Mode
                              );

ST_ErrorCode_t stvtg_HALVfeGetVtgId(  const void *    const BaseAddress_p
                                 , STVTG_VtgId_t * const VtgId_p
                                );
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef VTG_HAL_VFE_H */
