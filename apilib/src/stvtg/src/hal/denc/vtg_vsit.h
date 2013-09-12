/*******************************************************************************

File name   : vtg_vsit.h

Description : Video Vsync interrupt handler header file, used to simulate STVID handling
 * of VTG interrupts on omega1 chips family

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
 6 Mar 2002        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VTG_VIDEO_VSYNC_INTERRUPT_H
#define __VTG_VIDEO_VSYNC_INTERRUPT_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stvtg.h"
#include "vtg_drv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t stvtg_ActivateVideoVsyncInterrupt( stvtg_Device_t * const Device_p,
                                                  const STVTG_InitParams_t * const InitParams_p);
ST_ErrorCode_t stvtg_DeactivateVideoVsyncInterrupt(const stvtg_Device_t * const Device_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VTG_VIDEO_VSYNC_INTERRUPT_H */

/* End of vtg_vsit.h */
