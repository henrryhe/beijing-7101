/*****************************************************************************

File name : vtg_hal2.h

Description :  HAL for VTG programmation

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
11 Sep 2000        Created                                          HSdLM
16 Nov 2000        Improve error tracking                           HSdLM
18 Jan 2001        Updates                                          HSdLM
21 Mar 2001        Add HALVTG_SetOutput() to inhibit signal         HSdLM
28 Jun 2001        Reorganization : split into vtg_hal2, hal_vos    HSdLM
 *                 and hal_vfe.
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_HAL2_H
#define __VTG_HAL2_H

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

ST_ErrorCode_t stvtg_HALInit(stvtg_Device_t * const Device_p, const STVTG_InitParams_t * const InitParams_p);

ST_ErrorCode_t stvtg_HALSetOptionalConfiguration( stvtg_Device_t * const Device_p);

ST_ErrorCode_t stvtg_HALInterruptTerm(stvtg_Device_t * const Device_p);

void stvtg_HALSetInterrupt(stvtg_Device_t * const Device_p);

void stvtg_HALSetSyncEdges ( const stvtg_Device_t * const Device_p);

ST_ErrorCode_t stvtg_WA_GNBvd47682_VTimer_Raise(STVTG_Handle_t Handle, U32 AVmuteAddress);

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VTG_HAL2_H */
