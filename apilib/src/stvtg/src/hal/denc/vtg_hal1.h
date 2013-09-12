/*****************************************************************************

File name : vtg_hal1.h

Description :  HAL for omega1 VTG programmation

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
15 Sep 2000        Created                                          HSdLM
16 Nov 2000        Change name to remove paths in includes          HSdLM
08 Feb 2001        New functions exported                           HSdLM
28 Jun 2001        Set prefix stvtg_ on exported symbols            HSdLM
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_HAL1_H
#define __VTG_HAL1_H

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

void stvtg_HALInitVtgInDenc(  stvtg_Device_t * const Device_p);

void stvtg_HALSetCapabilityOnDenc( stvtg_Device_t * const Device_p);

ST_ErrorCode_t stvtg_HALCheckDencSlaveModeParams( const STVTG_SlaveModeParams_t * const SlaveModeParams_p);

ST_ErrorCode_t stvtg_HALUpdateTimingParameters(  stvtg_Device_t *            const Device_p
                                               , const STVTG_TimingMode_t Mode
                                            );

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VTG_HAL1_H */
