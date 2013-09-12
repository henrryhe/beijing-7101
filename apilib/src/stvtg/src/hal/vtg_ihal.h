/*****************************************************************************

File name : vtg_ihal.h

Description :  HAL interface for VTG programmation. Exported routines declarations.

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
20 Sep 2000        Creation                                         HSdLM
22 Nov 2000        New routine for error tracking                   HSdLM
15 Feb 2001        Updates                                          HSdLM
21 Mar 2001        Remove HALIVTG_SetInterrupt()                    HSdLM
 *                 New HALIVTG_SetOutput()
28 Jun 2001        Set prefix stvtg_ on exported symbols            HSdLM
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_IHAL_H
#define __VTG_IHAL_H

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

ST_ErrorCode_t stvtg_IHALInit(  stvtg_Device_t *           const Device_p
                              , const STVTG_InitParams_t * const InitParams_p
                             );

ST_ErrorCode_t stvtg_IHALGetVtgId(  const STVTG_InitParams_t * const InitParams_p
                                  , STVTG_VtgId_t *            const VtgId_p
                                 );

ST_ErrorCode_t stvtg_IHALUpdateTimingParameters(  stvtg_Device_t *         const Device_p
                                                , const STVTG_TimingMode_t Mode
                                               );

ST_ErrorCode_t stvtg_IHALSetOptionalConfiguration( stvtg_Device_t * const Device_p);

ST_ErrorCode_t stvtg_IHALCheckOptionValidity( stvtg_Device_t * const Device_p);

ST_ErrorCode_t stvtg_IHALCheckSlaveModeParams(  const stvtg_Device_t *          const Device_p
                                              , const STVTG_SlaveModeParams_t * const SlaveModeParams_p
                                             );

ST_ErrorCode_t stvtg_IHALTerm(stvtg_Device_t * const Device_p);

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VTG_IHAL_H */
